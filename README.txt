'mphidflash' is a simple command-line tool for communicating with Microchips 
USB HID-Bootloader and downloading new firmware. mphidflash supports Linux, 
Mac OS X (Leopard and later), and Windows.

Supports Microchip USB HID Bootloader for PIC16, PIC18, PIC24 distributed in
the Microchip USB Library for Application, and Bootloader ports to the PIC32
such as for the UBW32 (Ultimate Bit Whacker 32). The default USB Vendor Identity
(VID) is 0x04D8 and Product Identity (PID) 0x003C.

Note that 'mphidflash' does NOT support the Microchip PIC32 HID Bootloader
distributed in the Microchip MPLAB Harmony, and previously in source code
accompanying Application Note AN1388. Neither does 'mphidflash' support the
Bootloader in the Microchip PICDEM FS-USB demonstration application. The
communication protocol in these products is very different, and unfortunately
the PIC32 HID Bootloader uses the same default USB Product Id as the supported
HID Bootloader. If used in error either 'mphidflash' or the development board
may freeze or indicate an error.

The 'mphidflash' project is hosted at https://github.com/AdamLaurie/mphidflash
(migrated from http://code.google.com/p/mphidflash/)

Building and Installing
=======================

Linux
-----
For Linux, you'll need the 'libhid' development library (and associated
dependencies) installed, which can be handled by most package managers or
from the command line:

	sudo apt-get install libhid-dev

Assuming you're reading this as the README.txt alongside the source code,
to compile mphidflash for a 32 or 64 bit system, in the Terminal window type:

	make mphidflash32

  or

	make mphidflash64

Then install with the command:

	sudo make install32

  or

	sudo make install64

This will copy the appropriate executable to /usr/local/bin/mphidflash so you 
don't need to specify a complete path to the program each time.


Mac OS X
--------
To build mphidflash for Mac, you'll need to have Xcode (Apple's
developer tools) already installed.  The IDE portion is not used here, just
the command line interface.

Assuming you're reading this as the README.txt alongside the source code,
to compile mphidflash for a 32 or 64 bit system, in the Terminal window type:

	make mphidflash32

  or

	make mphidflash64

Then install with the command:

	sudo make install32

  or

	sudo make install64

This will copy the appropriate executable to /usr/local/bin/mphidflash so you
don't need to specify a complete path to the program each time.


Windows
-------
To build mphidflash for Windows, you can use mingw32 (cross) compiler. For
cross compiling on Linux, you'll need the 'mingw32' toolchain (and associated
dependencies) installed, which can be handled by most package mangager or
from the command line:

	sudo apt-get install mingw32

Assuming you're reading this as the README.txt alongside the source code,
to compile mphidflash, in the Terminal window type:

	make -f Makefile.win

This will create a .exe in the binaries sub-directory, which can be called
from the Windows commmand line - e.g. 'mphidflash-1.6-win-32.exe'. You should 
copy this file somewhere on your executable path and rename it to 'mphidflash.exe'.


Usage
=====
To upload a new program to your PIC, it must be connected to your computer and
set into bootloader mode. mphidflash can then be used with the following 
options:

-help			Display help screen (alternately: -?)
-write <file>	Upload given file to PIC 
-reset			Reset PIC
-noverify		Skip verification step
-erase			Erase PIC memory
-sign			Sign flash
-vendor <hex>	Use given USB vendor id instead of default id
-product <hex>	Use given USB product id instead of default id

Example: To upload the program test.hex to the PIC and to reset the PIC thereafter
the following command line can be used:

	mphidflash -write test.hex -reset

Tips
====
For programming or erase connect the development board directly to the PC or a
powered hub, and consider using a short USB cable. The development board can
rapidly change the amount of current drawn, and long cables can cause dips or
spikes in the 5V connection. So these operations are more likely to fail if
there are unpowered hubs or extensions connecting the board to the PC.
