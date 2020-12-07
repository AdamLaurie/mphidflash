/****************************************************************************
 File        : usb-libusb.c
 Description : Encapsulates all nonportable, libusb USB I/O code
               within the mphidflash program.

 History     : 2009-02-19  Phillip Burgess
                 * Initial implementation of usb-linux with libhid
               2009-12-28  Thomas Fischl, Dominik Fisch (www.FundF.net)
                 * Support for libusb without dependencies to libhid

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
#include <stdio.h>
#include <usb.h>

#include "mphidflash.h"

unsigned char   usbBufX[64];
unsigned char   *usbBuf = usbBufX;
usb_dev_handle  *usbdevice = NULL;

ErrorCode usbOpen(
  const unsigned short vendorID,
  const unsigned short productID)
{
    struct usb_bus      *bus;
    struct usb_device   *dev;

    usb_init();
    usb_find_busses();
    usb_find_devices();

    for (bus=usb_get_busses(); bus; bus=bus->next) {
        for (dev=bus->devices; dev; dev=dev->next) {
            if (dev->descriptor.idVendor == vendorID && dev->descriptor.idProduct == productID) {

                usbdevice = usb_open(dev);
                if (!usbdevice) {
                    fprintf(stderr, "Warning: matching device found, but cannot open usb device: %s\n", usb_strerror());
                    continue;
                }

                if (usb_claim_interface(usbdevice, 0) != 0) {
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
                    usb_detach_kernel_driver_np(usbdevice, 0);
#endif
                    if (usb_claim_interface(usbdevice, 0) != 0) {
                        usb_close(usbdevice);
                        usbdevice = NULL;
                        fprintf(stderr, "Warning: cannot claim interface: %s\n", usb_strerror());
                        continue;
                    }
                }

                return ERR_NONE;
            }
        }
    }

    return ERR_DEVICE_NOT_FOUND;
}

ErrorCode usbWrite(
  const char len,
  const char read)
{
    int bytesSent;
    int bytesRead;

    bytesSent = usb_interrupt_write(usbdevice, 0x01, usbBuf, len, 5000);
    if (bytesSent < 0)
        return ERR_USB_WRITE;

    if (read) {

        bytesRead = usb_interrupt_read(usbdevice, 0x81, usbBuf, 64, 5000);
        if (bytesRead < 0)
            return ERR_USB_READ;
    }

    return ERR_NONE;
}

void usbClose(void)
{
    if (usbdevice != NULL) {
        usb_release_interface(usbdevice, 0);
        usb_close(usbdevice);
    }
}
