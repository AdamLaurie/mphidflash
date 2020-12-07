/****************************************************************************
 File        : main.c
 Description : Main source file for 'mphidflash,' a simple command-line tool for
               communicating with Microchips USB HID-Bootloader and downloading new
               firmware.

 History     : 2009-02-19  Phillip Burgess
                 * Initial implementation
               2009-12-26  Thomas Fischl, Dominik Fisch (www.FundF.net)
                 * Renamed 'ubw32' to 'mphidflash'
               2010-12-28  Petr Olivka
                 * program and verify only data for defined memory areas
                 * send only even length of data to PIC
               2017-08-16  Tony Naggs
                 * Sanity checks on QUERY_DEVICE results, good practice
                   and should reject custom Booloaders with private info
                   in the memory block list, e.g. Pinguino PIC32
                 * after Unlock op refetch memory block info, per documentation

 License     : Copyright (C) 2009 Phillip Burgess
               Copyright (C) 2009 Thomas Fischl, Dominik Fisch (www.FundF.net)
               Copyright (C) 2010 Petr Olivka

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
#endif

#include <stdio.h>
#include <string.h>
#include "mphidflash.h"

sQuery devQuery;

extern unsigned char * usbBuf;  /* In usb code */

/* Program's actions aren't necessarily performed in command-line order.
   Bit flags keep track of options set or cleared during input parsing,
   then are singularly checked as actions are performed.  Some actions
   (such as writing) don't have corresponding bits here; certain non-NULL
   string values indicate such actions should occur. */
#define ACTION_UNLOCK (1 << 0)
#define ACTION_ERASE  (1 << 1)
#define ACTION_VERIFY (1 << 2)
#define ACTION_RESET  (1 << 3)
#define ACTION_SIGN   (1 << 4)

/*
 * deviceQueryProcessResult() - parses memory block & other info in QUERY_DEVICE result
 * call 1st time with doExtendedQuery <> 0 to detect Bootloader v1.01 (requires SIGN_FLASH
 * after programming), and supports QUERY_EXTENDED_INFO command.
 * If Unlocking Config Memory in device should call again, (with doExtendedQuery == 0)
 * to ensure we have correct details of Config Memory location.
 */
static ErrorCode deviceQueryProcessResult(
  char doExtendedQuery)
{
    char         hasProgramMemory = 0;
    char         hasConfigMemory = 0;
    int          i = 0, j;
    char         *name = NULL;
    unsigned int tempaddr;

    memcpy( &devQuery, usbBuf, 64 );

    // bad values here probably indicate a garbage response
    if (QUERY_DEVICE != devQuery.Command)
        return ERR_RESPONSE_HAS_WRONG_COMMAND;

    if (56 != devQuery.PacketDataFieldSize) {
        // we have hardcoded assumptions this will be 56 & total buffer of 64
        fprintf(stderr, "Reported Packet Data Field Size is %d\n",
            devQuery.PacketDataFieldSize);
        return ERR_BAD_PACKET_DATA_FIELD_SIZE;
    }

    // count how many memory blocks are defined
    while ( (devQuery.mem[ i ].Type != TypeEndOfTypeList) && (i < MAX_DATA_REGIONS) )
        i++;
    devQuery.memBlocks = i;

    switch (devQuery.DeviceFamily)
        {
        case DEVICE_FAMILY_PIC18: // PIC16 per experience
            hexSetBytesPerAddress(1);
            name = "PIC18 (or PIC16)";
            break;
        case DEVICE_FAMILY_PIC24:
            hexSetBytesPerAddress(2);
            name = "PIC24 or dsPIC33";
            break;
        case DEVICE_FAMILY_PIC32:
            hexSetBytesPerAddress(1);
            name = "PIC32";
            break;
        case DEVICE_FAMILY_PIC16: // PIC16 per documentation
            hexSetBytesPerAddress(2);
            name = "PIC16";
            break;
        default:
            hexSetBytesPerAddress(1);
            name = "Unknown. Bytes per address set to 1.";
            break;
    }
    if (doExtendedQuery)
        (void)printf("Device family: %s\n", name);
    else
        (void)puts("Refetching memory block information...");

    if (!doExtendedQuery && !hasConfigMemory)
        (void)puts("No Config Memory found after unlocking");

    // endian conversion, report available memory
    for ( i = 0; i < devQuery.memBlocks; i++ ) {
        name = NULL;
        devQuery.mem[i].Address = convertEndian(devQuery.mem[i].Address);
        devQuery.mem[i].Length = convertEndian(devQuery.mem[i].Length);
        switch(devQuery.mem[i].Type)
            {
            case TypeProgramMemory:
                name = "Program";
                hasProgramMemory = 1;
                break;
            case TypeEEPROM:
                name = "EEPROM";
                break;
            case TypeConfigWords:
                name = "Config";
                hasConfigMemory = 1;
                break;
            case TypeUserID:
                name = "User ID";
                break;
            default:
                break;
        }

        if (name)
            (void)printf("%s", name);
        else
            (void)printf("Unknown (type id %d)", devQuery.mem[i].Type);
        (void)printf(" memory at 0x%x: %d bytes free\n",
            devQuery.mem[i].Address, devQuery.mem[i].Length);
    }

    // sanity checks
    for ( i = 0; i < devQuery.memBlocks; i++ ) {
        // memory type 0 not defined by Microchip, & used in mphidflash to disable programming
        if(0 == devQuery.mem[i].Type)
            return ERR_BAD_MEM_TYPE;

        // sane info for block length?
        if (devQuery.mem[i].Length == 0) {
            return ERR_BAD_MEM_LENGTH;
        } else if ((bytesPerAddress > 1) &&
                (0 != (devQuery.mem[i].Length % bytesPerAddress))) {
            return ERR_BAD_MEM_LENGTH2;
        }

        // TODO check memory address makes sense for PIC18, PIC24, etc

        // each block type is defined max one time, must not overlap other blocks
        for (j = 0; j < i; j++) {
            // we expect each memory block type to be listed only once
            // (based on reviewing Microchip's published reference code)
            if ((devQuery.mem[i].Type) == (devQuery.mem[j].Type)) {
                return ERR_MEM_BLOCK_TYPE_REPEATS;
            }
            // memory blocks should not overlap
            if (devQuery.mem[i].Address == devQuery.mem[j].Address) {
                return ERR_OVERLAPPING_MEM_BLOCKS;
            } else if (devQuery.mem[i].Address < devQuery.mem[j].Address) {
                tempaddr = devQuery.mem[i].Address + (devQuery.mem[i].Length / bytesPerAddress);
                if (tempaddr >= devQuery.mem[j].Address) {
                    return ERR_OVERLAPPING_MEM_BLOCKS;
                }
            } else {
                tempaddr = devQuery.mem[j].Address + (devQuery.mem[j].Length / bytesPerAddress);
                if (tempaddr >= devQuery.mem[i].Address) {
                    return ERR_OVERLAPPING_MEM_BLOCKS;
                }
            }
        }
    } // sanity iter over blocks

    if (!hasProgramMemory) {
        return ERR_NO_PROGRAM_MEMORY;
    }

    // TODO check for Bootloader v1.01: supports SIGN_FLASH & QUERY_EXTENDED_INFO
    return ERR_NONE;
}


/****************************************************************************
 Function    : main
 Description : mphidflash program startup; parse command-line input and issue
               commands as needed; return program status.
 Returns     : int  0 on success, else various numeric error codes.
 ****************************************************************************/
int main(
  int   argc,
  char *argv[])
{
    char        *hexFile   = NULL,
                 actions   = ACTION_VERIFY,
                 eol;        /* 1 = last command-line arg */
    ErrorCode    status    = ERR_NONE;
    int          i;
    // default USB Vendor & Product Ids for various Microchip Bootloaders
    unsigned int vendorID  = 0x04d8,
                 productID = 0x003c;

    const char * const errorString[ERR_EOL] = {
        "Missing or malformed command-line argument",
        "Command not recognized",
        "Device not found (is device attached and in Bootloader mode?)",
        "USB initialization failed (phase 1)",
        "USB initialization failed (phase 2)",
        "Device could not be opened for I/O",
        "USB write error",
        "USB read error",
        "Could not open hex file for input",
        "Could not query hex file size",
        "Could not map hex file to memory",
        "Unrecognized or invalid hex file syntax",
        "Bad end-of-line checksum in hex file",
        "Unsupported record type in hex file",
        "Verify failed (is device connected to a powered root or hub?)",
        "Bad memory type (0) in device memory list",
        "Memory block length is bad (0, too big)",
        "Memory block length is not multiple of 2", // for PIC24
        "Memory blocks overlap",
        "Memory block type defined more than once",
        "Device has no program memory block",
        "Device response has unexpected command value",
        "Device reports unexpected Packet Data Field Size",
        "Signing Flash failed",
    };

    /* To create a sensible sequence of operations, all command-line
       input is processed prior to taking any actions.  The sequence
       of actions performed may not always directly correspond to the
       order or quantity of input; commands follow precedence, not
       input order.  For example, the action corresponding to the "-u"
       (unlock) command must take place early on, before any erase or
       write operations, even if specified late on the command line;
       conversely, "-r" (reset) should always be performed last
       regardless of input order.  In the case of duplicitous
       commands (e.g. if multiple "-w" (write) commands are present),
       only the last one will take effect.

       The precedence of commands (first to last) is:

       -v and -p <hex>  USB vendor and/or product IDs
       -u               Unlock configuration memory
       -e               Erase program memory
       -n               No verify after write
       -w <file>        Write program memory
       -s               Sign code
       -r               Reset */

    for(i=1;(i < argc) && (ERR_NONE == status);i++) {
        eol = (i >= (argc - 1));
        if(!strncasecmp(argv[i],"-v",2)) {
            if(eol || (1 != sscanf(argv[++i],"%x",&vendorID)))
                status = ERR_CMD_ARG;
        } else if(!strncasecmp(argv[i],"-p",2)) {
            if(eol || (1 != sscanf(argv[++i],"%x",&productID)))
                status = ERR_CMD_ARG;
        } else if(!strncasecmp(argv[i],"-u",2)) {
            actions |= ACTION_UNLOCK;
        } else if(!strncasecmp(argv[i],"-e",2)) {
            actions |= ACTION_ERASE;
        } else if(!strncasecmp(argv[i],"-n",2)) {
            actions &= ~ACTION_VERIFY;
        } else if(!strncasecmp(argv[i],"-w",2)) {
            if(eol) {
                status   = ERR_CMD_ARG;
            } else {
                hexFile  = argv[++i];
                actions |= ACTION_ERASE;
            }
        } else if(!strncasecmp(argv[i],"-s",2)) {
            actions |= ACTION_SIGN;
        } else if(!strncasecmp(argv[i],"-r",2)) {
            actions |= ACTION_RESET;
        } else if(!strncasecmp(argv[i],"-h",2) ||
                    !strncmp(argv[i],"-?",2)) {
            (void)printf(
"mphidflash v%d.%d: a Microchip HID Bootloader utility\n"
"Option     Description                                      Default\n"
"-------------------------------------------------------------------------\n"
"-w <file>  Write hex file to device (will erase first)      None\n"
"-e         Erase device code space (implicit if -w)         No erase\n"
"-r         Reset device on program exit                     No reset\n"
"-n         No verify after write                            Verify on\n"
"-u         Unlock configuration memory before erase/write   Keep lock state\n"
"-s         Sign flash. This option is required by later     No signing\n"
"           versions of the bootloader.\n"
"-v <hex>   USB device vendor ID                             %04x\n"
"-p <hex>   USB device product ID                            %04x\n"
"-h or -?   Help\n", VERSION_MAIN, VERSION_SUB, vendorID, productID);
            return 0;
        } else {
            status = ERR_CMD_UNKNOWN;
        }
    }

    /* After successful command-line parsage, find/open USB device. */

    if((ERR_NONE == status) &&
            (ERR_NONE == (status = usbOpen(vendorID,productID)))) {

        /* And start doing stuff... */

        (void)printf("USB HID device found");
        usbBuf[0] = QUERY_DEVICE;
        if(ERR_NONE == (status = usbWrite(1,1))) {
            status = deviceQueryProcessResult(1);
        }
        (void)putchar('\n');

        if((ERR_NONE == status) && (actions & ACTION_UNLOCK)) {
            (void)puts("Unlocking configuration memory...");
            usbBuf[0] = UNLOCK_CONFIG;
            usbBuf[1] = UNLOCKCONFIG;
            status    = usbWrite(2,0);
            // redo DEVICE_QUERY & process memory block info again
            // for Bootloader versions that only report Config mem after Unlock
            usbBuf[0] = QUERY_DEVICE;
            if((ERR_NONE == status) && (ERR_NONE == (status = usbWrite(1,1)))) {
                status = deviceQueryProcessResult(0);
            }
        }
        // disable all configuration blocks in devQuery if locked
        if ( !( actions & ACTION_UNLOCK ) ) {
            for ( i = 0; i < devQuery.memBlocks; i++ )
                if ( devQuery.mem[ i ].Type == TypeConfigWords )
                    devQuery.mem[ i ].Type = 0;
        }

        /* Although the next actual operation is ACTION_ERASE,
           if we anticipate hex-writing in a subsequent step,
           attempt opening file now so we can display any error
           message quickly rather than waiting through the whole
           erase operation (it's usually a simple filename typo). */
        if((ERR_NONE == status) && hexFile &&
                (ERR_NONE != (status = hexOpen(hexFile))))
            hexFile = NULL;  /* Open or mmap error */

        if((ERR_NONE == status) && (actions & ACTION_ERASE)) {
            (void)puts("Erasing...");
            usbBuf[0] = ERASE_DEVICE;
            status    = usbWrite(1,0);
            if (ERR_NONE == status) {
                /* The query here isn't needed for any technical
                   reason, just makes the presentation better.
                   The ERASE_DEVICE command above returns
                   immediately...subsequent commands can be made
                   but will pause until the erase cycle completes.
                   So this query just keeps the "Writing" message
                   or others from being displayed prematurely. */
                usbBuf[0] = QUERY_DEVICE;
                status    = usbWrite(1,1);
            }
        }

        if(hexFile) {
            if(ERR_NONE == status) {
                (void)printf("Writing hex file '%s':",hexFile);
                status = hexWrite((actions & ACTION_VERIFY) != 0);
                (void)putchar('\n');
            }
            hexClose();
        }

        if((ERR_NONE == status) && (actions & ACTION_SIGN)) {
            (void)puts("Signing flash...");
            usbBuf[0] = SIGN_FLASH;
            status = usbWrite(1,0);
            if (ERR_NONE == status) {
                /* Send another Query as it gives a result when
                   device is ready, but Sign Flash doesn't by itself */
                usbBuf[0] = QUERY_DEVICE;
                status    = usbWrite(1,1);
            }
            if (ERR_NONE != status) {
                status = ERR_SIGN_FLASH;
            }
        }

        if((ERR_NONE == status) && (actions & ACTION_RESET)) {
            (void)puts("Resetting device...");
            usbBuf[0] = RESET_DEVICE;
            status = usbWrite(1,0);
        }

        usbClose();
    }

    if(ERR_NONE != status) {
        (void)printf("%s Error",argv[0]);
        if(status <= ERR_EOL)
            (void)printf(": %s\n",errorString[status - 1]);
        else
            (void)puts(" of indeterminate type.");
    }

    return (int)status;
}
