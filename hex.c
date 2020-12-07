/****************************************************************************
 File        : hex.c
 Description : Code for dealing with Intel hex files.

 History     : 2009-02-19  Phillip Burgess
                 * Initial implementation
               2009-12-26  Thomas Fischl, Dominik Fisch (www.FundF.net)
                 * Ported mmap functions to windows

 License     : Copyright (C) 2009 Phillip Burgess
               Copyright (C) 2009 Thomas Fischl, Dominik Fisch (www.FundF.net)

               This file is part of 'mphidflash' program.

               'mphidflash' is free software: you can redistribute it and/or
               modify it under the terms of the GNU General Public License
               as published by the Free Software Foundation, either version
               3 of the License, or (at your option) any later version.

               'mphidflash' is distributed in the hope that it will be useful,
               but WITHOUT ANY WARRANTY; without even the implied warranty
               of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
               See the GNU General Public License for more details.

               You should have received a copy of the GNU General Public
               License along with 'mphidflash' source code.  If not,
               see <http://www.gnu.org/licenses/>.

 ****************************************************************************/

#if defined(_MSC_VER)
// MSVC - disable 'non-secure function' warnings for sscanf() etc before including stdio.h
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN
#endif

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#if defined(_MSC_VER)
#include <io.h>
#else
#include <unistd.h>
#endif
#include <string.h>

#ifndef WIN
#include <sys/mman.h>
#else
#include <windows.h>
#endif

#include <sys/stat.h>
#include "mphidflash.h"

static char          *hexFileData = NULL; /* Memory-mapped hex file data */
static char          *hexPlusOne;         /* Saves a lot of "+1" math    */
static int            hexFd;              /* Open hex file descriptor    */
static size_t         hexFileSize;        /* Save for use by munmap()    */
static unsigned char  hexBuf[56];         /* Data read/written to USB    */
extern unsigned char *usbBuf;             /* In usb code                 */
unsigned char bytesPerAddress = 1;        /* Bytes in flash per address  */
static char Flushed= 1;                   /* Do we need to flush buffer? */

/****************************************************************************
Function : hexSetBytesPerAddress
Description : Sets given byte width
Parameters : unsigned char Bytes per address
Returns : Nothing (void)
****************************************************************************/
void hexSetBytesPerAddress(unsigned char bytes)
{
    bytesPerAddress = bytes;
}

/****************************************************************************
Function : hexGetBytesPerAddress
Description : returns set byte width
Parameters : Nothing (void)
Returns : unsigned char Bytes per address
****************************************************************************/
unsigned char hexGetBytesPerAddress(void)
{
    return bytesPerAddress;
}

/****************************************************************************
 Function    : hexOpen
 Description : Open and memory-map an Intel hex file.
 Parameters  : char*      Filename (must be non-NULL).
 Returns     : ErrorCode  ERR_NONE     Success
                          ERR_HEX_OPEN File not found or no read permission
                          ERR_HEX_STAT fstat() call failed for some reason
                          ERR_HEX_MMAP Memory-mapping failed
 ****************************************************************************/
ErrorCode hexOpen(char * const filename)
{
    ErrorCode status = ERR_HEX_OPEN;

    if((hexFd = open(filename,O_RDONLY)) >= 0) {

        struct stat filestat;

        status = ERR_HEX_STAT;
        if(!fstat(hexFd,&filestat)) {

            status      = ERR_HEX_MMAP;
            hexFileSize = filestat.st_size;

#ifndef WIN
            if((hexFileData = mmap(0,hexFileSize,PROT_READ,
                    MAP_FILE | MAP_SHARED,hexFd,0)) != (void *)(-1)) {
                hexPlusOne = &hexFileData[1];
                return ERR_NONE;
            }
#else
            {
                HANDLE handle;
                handle = CreateFileMapping((HANDLE)_get_osfhandle(hexFd), NULL, PAGE_WRITECOPY, 0, 0, NULL);
                if (handle != NULL) {
                    hexFileData = (char *) MapViewOfFile(handle, FILE_MAP_COPY, 0, 0, hexFileSize);
                    hexPlusOne = &hexFileData[1];
                    CloseHandle(handle); 
                    return ERR_NONE;
                }
            }
#endif

            /* Else clean up and return error code */
            hexFileData = NULL;
        }
        (void)close(hexFd);
    }

    return status;
}

/* check memory address & length are in a programmable memory area, as reported by device's Bootloader */
static int verifyBlockProgrammable( unsigned int *addr, unsigned int *len )
{
    int i, isA, isL;
    unsigned int MA , ML;

    for ( i = 0; i < devQuery.memBlocks; i++ )
    {
        /* only look at programmable memory blocks */
        if ( !devQuery.mem[ i ].Type )
            continue;

        /* calc if first or last address is in this block */
        MA = devQuery.mem[ i ].Address;
        ML = devQuery.mem[ i ].Length;
        isA = ( *addr >= MA ) && ( *addr < MA + ML );
        isL = ( *addr + *len > MA ) && ( *addr + *len <= MA + ML );

        /* loop if neither */
        if ( !isA && !isL )
            continue;

        /* both addresses is fine */
        if ( isA && isL )
            return 0;

        /* if only start address we adjust length to end of block */
        if ( isA )
            *len = ( MA + ML ) - *addr;

        /* if only last address we adjust start to start of block */
        if ( isL ) {
            *len = ( *addr + *len ) - MA;
            *addr = MA;
        }

        return 0;
    }

    return 1;
}

/****************************************************************************
 Function    : atoh (inline pseudo-function)
 Description : Converts two adjacent ASCII characters (representing an 8-bit
               hexadecimal value) to a numeric value.
 Parameters  : int            Index (within global hex array) to start of
                              input string; must contain at least two chars.
 Returns     : unsigned char  Numeric result; 0 to 255.
 Notes       : Range checking of input characters is somewhat slovenly;
               all input is assumed to be in the '0' to '9' and 'A' to 'F'
               range.  But if any such shenanigans were to occur, the line
               checksum will likely catch it.
 ****************************************************************************/
#define atoh(pos) \
  ((((hexFileData[pos] <= '9') ? (hexFileData[pos] - '0') : \
     (0x0a + toupper(hexFileData[pos]) - 'A')) << 4) |      \
   ( (hexPlusOne [pos] <= '9') ? (hexPlusOne [pos] - '0') : \
     (0x0a + toupper(hexPlusOne [pos]) - 'A')      ))

/****************************************************************************
 Function    : issueBlock
 Description : Send data over USB bus to device.
 Parameters  : unsigned int  Destination address on PIC device.
               unsigned int  Byte count (max 56).
               char          Verify vs. write.
 Returns     : ErrorCode     ERR_NONE on success, or error code as returned
                             from usbWrite();
 ****************************************************************************/
static ErrorCode issueBlock(
  unsigned int  addr,
  unsigned int  len,
  char          verify)
{
    ErrorCode status;

#ifdef DEBUG
    (void)printf("Address: %08x  Len %d\n",addr,len);
#else
    (void)putchar('.'); fflush(stdout);
#endif

    // check device memory blocks are programmable
    if ( verifyBlockProgrammable( &addr, &len ) ) {
#ifdef DEBUG
        printf( "Skip data on address %04x with length %d\n", addr, len );
#endif
        return ERR_NONE;
    }
    // length must be even
    if ( len & 1 ) {
#ifdef DEBUG
        printf( "Add one byte to data on address %04x with length %d\n", addr, len );
#endif
        hexBuf[ len++ ] = 0xff;
    }

    /* Short data packets need flushing */
    if (!verify && len == 0 && !Flushed) {
        DEBUGMSG("Completing");
        usbBuf[0] = PROGRAM_COMPLETE;
        status = usbWrite(1,0);
        Flushed= 1;
        return status;
    }

    bufWrite32(usbBuf,1,addr / bytesPerAddress);
    usbBuf[5] = len;

    if(verify) {
        DEBUGMSG("Verifying");
        usbBuf[0] = GET_DATA;
        if(ERR_NONE == (status = usbWrite(6,1))) {
#ifdef DEBUG
            unsigned int i;
            if(memcmp(&usbBuf[64 - len],hexBuf,len)) {
                (void)puts("Verify FAIL\nExpected:");
                (void)printf("NA NA NA NA NA NA NA NA - ");
                for(i=0;i<(56-len);i++) (void)printf("NA ");
                for(i=0;i<len;i++)
                    (void)printf("%02x ",hexBuf[i]);
                (void)putchar('\n'); fflush(stdout);
                return ERR_VERIFY;
            } else {
                (void)puts("Verify OK");
                return ERR_NONE;
            }
#else
            return (memcmp(&usbBuf[64 - len],hexBuf,len) ?
                ERR_VERIFY : ERR_NONE);
#endif
        }
    } else {
        DEBUGMSG("Writing");
        usbBuf[0] = PROGRAM_DEVICE;
        /* Regardless of actual byte count, data packet is always
           64 bytes.  Following the header, the bootloader wants the
           data portion 'right justified' within packet.  Odd. */
        memcpy(&usbBuf[64 - len],hexBuf,len);
        if((ERR_NONE == (status = usbWrite(64,0))) && (len < 56))
        {
            /* Short data packets need flushing */
            DEBUGMSG("Completing");
            usbBuf[0] = PROGRAM_COMPLETE;
            status    = usbWrite(1,0);
        }
        // flag if external code may need to flush before next write
        Flushed= (len < 56);
    }

#ifdef DEBUG
    if(status != ERR_NONE) (void)puts("ERROR");
#endif

    return status;
}

/****************************************************************************
 Function    : hexWrite
 Description : Writes (and optionally verifies) currently-open hex file to
               device.
 Parameters  : char       Verify (1) vs. write (0).
 Returns     : ErrorCode  ERR_NONE on success, else various other values as
                          defined in mphidflash.h.
 Notes       : USB device and hex file are both assumed already open and
               valid; no checks performed here.
 ****************************************************************************/
ErrorCode hexWrite(const char verify)
{
    char         *ptr,pass;
    ErrorCode     status;
    int           checksum,i,end,offset;
    unsigned int  len,bufLen,type,addrHi,addrLo,addr32,addrSave;

    for(pass=0;pass<=verify;pass++) {
        offset   = 0; /* Start at beginning of hex file         */
        bufLen   = 0; /* Hex buffer initially empty             */
        addrHi   = 0; /* Initial address high bits              */
        addrSave = 0; /* PIC start addr for hex buffer contents */
        addr32   = 0;

        if(pass) (void)printf("\nVerifying:");

        for(;;) {  /* Each line in file */

            /* Line start contains length, 16-bit address and type */
            if(3 != sscanf(&hexFileData[offset],":%02x%04x%02x",
                    &len,&addrLo,&type))
                return ERR_HEX_SYNTAX;

            /* Position of %02x checksum at end of line */
            end = offset + 9 + len * 2;

            /* Verify checksum on first (write) pass */
            if(!pass) {
                for(checksum = 0,i = offset + 1;i < end;
                    checksum = (checksum + (0x100 - atoh(i))) & 0xff,i += 2);
                if(atoh(end) != checksum) return ERR_HEX_CHECKSUM;
            }

            /* Process different hex record types.  Using if/else rather
               than a switch in order to better handle EOF cases (allows
               simple 'break' rather than goto or other nasties). */

            if(0 == type) { /* Data record */

                /* If new record address is not contiguous with prior record,
                   issue accumulated hex data (if any) and start anew. */
                if((addrHi + addrLo) != addr32) {
                    // flush previous write
                    if(!Flushed && ERR_NONE != (status = issueBlock(addrSave,0,pass)))
                        return status;
                    addr32 = addrHi + addrLo;
                    if(bufLen) {
                        if(ERR_NONE != (status = issueBlock(addrSave,bufLen,pass)))
                            return status;
                        bufLen = 0;
                    }
                    addrSave = addr32;
                }

                /* Parse bytes from line into hexBuf */
                for(i = offset + 9;i < end;i += 2) {
                    hexBuf[bufLen++] = atoh(i); /* Add to hex buffer */
                    /* If buffer is full, issue block and start anew */
                    if(sizeof(hexBuf) == bufLen) {
                        if(ERR_NONE != (status = issueBlock(addrSave,bufLen,pass)))
                            return status;
                        bufLen = 0;
                    }

                    /* Increment address, wraparound as per hexfile spec */
                    if(0xffffffff == addr32) {
                        /* Wraparound.  If any hex data, issue and start anew. */
                        if(bufLen) {
                            if(ERR_NONE !=
                                    (status = issueBlock(addrSave,bufLen,pass)))
                                return status;
                            bufLen = 0;
                        }
                        addr32 = 0;
                    } else {
                        addr32++;
                    }

                    /* If issueBlock() used, save new address for next block */
                    if(!bufLen) addrSave = addr32;
                }

            } else if(1 == type) { /* EOF record */

                break;

            } else if(4 == type) { /* Extended linear address record */

                if(1 != sscanf(&hexFileData[offset+9],"%04x",&addrHi))
                    return ERR_HEX_SYNTAX;
                addrHi <<= 16;
                addr32 = addrHi;
                /* Assume this means a noncontiguous address jump; issue block
                   and start anew.  The prior noncontiguous address code should
                   already have this covered, but in the freak case of an
                   extended address record with no subsequent data, make sure
                   the last of the data is issued. */
                // flush previous write
                if(!Flushed && ERR_NONE != (status = issueBlock(addrSave,0,pass)))
                    return status;
                if(bufLen) {
                    if(ERR_NONE != (status = issueBlock(addrSave,bufLen,pass)))
                        return status;
                    bufLen   = 0;
                }
                addrSave = addr32;


            } else if(5 == type) { /* Start address */

                /* Ignore */

            } else { /* Unsupported record type */
                return ERR_HEX_RECORD;
            }

            /* Advance to start of next line (skip CR/LF/etc.), unless EOF */
            if(NULL == (ptr = strchr(&hexFileData[end+2],':'))) break;

            offset = ptr - hexFileData;
        }

        /* At end of file, issue any residual data (counters reset at top) */
        if(bufLen &&
                (ERR_NONE != (status = issueBlock(addrSave,bufLen,pass))))
            return status;

        /* Make sure last data is flushed */
        if(!pass && !Flushed)
            return issueBlock(addrSave,0,pass);

#ifdef DEBUG
        (void)printf("PASS %d of %d COMPLETE\n",pass,verify);
#endif
    }

    return ERR_NONE;
}

/****************************************************************************
 Function    : hexClose
 Description : Unmaps and closes previously-opened hex file.
 Parameters  : None (void)
 Returns     : Nothing (void)
 Notes       : File is assumed to have already been successfully opened
               by the time this function is called; no checks performed here.
 ****************************************************************************/
void hexClose(void)
{
#ifndef WIN
    (void)munmap(hexFileData,hexFileSize);
#else
    UnmapViewOfFile(hexFileData);
#endif
    hexFileData = NULL;
    (void)close(hexFd);
}

