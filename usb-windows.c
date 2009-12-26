/****************************************************************************
 File        : usb-linux.c
 Description : Encapsulates all nonportable, Linux-specific USB I/O code
               within the mphidflash program.  Each supported operating
               system has its own source file, providing a common calling
               syntax to the portable sections of the code.

 History     : 2009-12-26  Thomas Fischl, Dominik Fisch (www.FundF.net)
                 * Initial windows support
               
 License     : Copyright (C) 2009 Thomas Fischl, Dominik Fisch (www.FundF.net)

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

#include <stdio.h>
#include <windows.h>
#include <setupapi.h>
#include <ddk/hidsdi.h>
#include <ddk/hidpi.h>
#include "mphidflash.h"

HANDLE usbdevhandle = INVALID_HANDLE_VALUE; 
unsigned char        usbBufX[65];
unsigned char *      usbBuf = &usbBufX[1];

HIDP_CAPS       Capabilities;   
PHIDP_PREPARSED_DATA        HidParsedData;   

ErrorCode usbOpen(
  const unsigned short vendorID,
  const unsigned short productID)
{
	ErrorCode      status = ERR_DEVICE_NOT_FOUND;
	int i;
	GUID                                hidGuid;
	HDEVINFO deviceInfoList;	
	SP_DEVICE_INTERFACE_DATA deviceInfo;
	SP_DEVICE_INTERFACE_DETAIL_DATA     *deviceDetails = NULL;
	DWORD                               size;
	
	HIDD_ATTRIBUTES                     deviceAttributes;

	HidD_GetHidGuid(&hidGuid);
	deviceInfoList = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);

	deviceInfo.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	for (i = 0;;i++) {

		/* if there is a assigned handler, close it */
		if (usbdevhandle != INVALID_HANDLE_VALUE) {
            		CloseHandle(usbdevhandle);
            		usbdevhandle = INVALID_HANDLE_VALUE;
        	}

		if (!SetupDiEnumDeviceInterfaces(deviceInfoList, 0, &hidGuid, i, &deviceInfo)) {
			/* finished walk through device list */
            		break;  
		}

		/* get size for detail structure */
		SetupDiGetDeviceInterfaceDetail(deviceInfoList, &deviceInfo, NULL, 0, &size, NULL);
		if (deviceDetails != NULL)
		    free(deviceDetails);
		deviceDetails = malloc(size);
		deviceDetails->cbSize = sizeof(*deviceDetails);

		/* now get details */
	        SetupDiGetDeviceInterfaceDetail(deviceInfoList, &deviceInfo, deviceDetails, size, &size, NULL);

		/* try to open device */
        	usbdevhandle = CreateFile(deviceDetails->DevicePath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        	if (usbdevhandle == INVALID_HANDLE_VALUE) {
			/* cannot open device, go to next */
			continue;
		}

		/* get attributes for this device */
		deviceAttributes.Size = sizeof(deviceAttributes);
        	HidD_GetAttributes(usbdevhandle, &deviceAttributes);
        	if (deviceAttributes.VendorID != vendorID || deviceAttributes.ProductID != productID) {
			/* device doesn't match, go to next */
            		continue;
		}

		 HidD_GetPreparsedData(usbdevhandle, &HidParsedData);   
            
         /* extract the capabilities info */   
         HidP_GetCaps(HidParsedData ,&Capabilities);   
            
         /* Free the memory allocated when getting the preparsed data */   
         HidD_FreePreparsedData(HidParsedData);         

		/* okay, here we found our device */
		status = ERR_NONE;
		break;
	}

	SetupDiDestroyDeviceInfoList(deviceInfoList);

	if (deviceDetails != NULL)
        	free(deviceDetails);

	return status;
}


ErrorCode usbWrite(
  const char len,
  const char read)
{

	BOOLEAN rval = 0;
	DWORD   bytesWritten = 0;
	DWORD   bytesRead = 0;

#ifdef DEBUG
	int i;
	(void)puts("Sending:");
	for(i=0;i<8;i++) (void)printf("%02x ",((unsigned char *)usbBuf)[i]);
	(void)printf(": ");
	for(;i<64;i++) (void)printf("%02x ",((unsigned char *)usbBuf)[i]);
	(void)putchar('\n'); fflush(stdout);
	DEBUGMSG("\nAbout to write");
	printf("usbdevhandle: %d, usbBuf: %d, len: %d, %d\n", usbdevhandle, usbBuf, len, Capabilities.OutputReportByteLength);
#endif


	/* report id */
	usbBufX[0] = 0;

	if (WriteFile(usbdevhandle, usbBufX, Capabilities.OutputReportByteLength, &bytesWritten, 0) == 0) {
//		printf("usb write failed, Error %u\n", GetLastError());

		return ERR_USB_WRITE;
	}

	DEBUGMSG("Done w/write");


	if (read) {
		if (ReadFile(usbdevhandle, usbBufX, Capabilities.OutputReportByteLength, &bytesRead, 0) == 0) {
//			printf("usb read failed, Error %u\n", GetLastError());
			return ERR_USB_READ;
		}
	
#ifdef DEBUG
		(void)puts("Done reading\nReceived:");
		for(i=0;i<8;i++) (void)printf("%02x ",usbBuf[i]);
		(void)printf(": ");
		for(;i<64;i++) (void)printf("%02x ",usbBuf[i]);
		(void)putchar('\n'); fflush(stdout);
#endif
	}

	return ERR_NONE;
}

void usbClose(void)
{
	CloseHandle(usbdevhandle);
}
