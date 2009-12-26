'mphidflash' is a simple command-line tool for communicating with Microchips 
USB HID-Bootloader and downloading new firmware. mphidflash supports Linux, 
Mac OS X (Leopard and later), and Windows.

The 'mphidflash' project is hosted on
http://code.google.com/p/mphidflash/

Building and Installing
=======================

Linux
-----
For Linux, you'll need the 'libhid' development library (and associated
dependencies) installed, which can be handled by most package managers or
from the command line:

	sudo apt-get install libhid-dev

Assuming you're reading this as the README.txt alongside the source code,
to compile mphidflash, in the Terminal window type:

	make

Then install with the command:

	sudo make install

This will copy the 'mphidflash' executable to /usr/local/bin so you don't need
to specify a complete path to the program each time.


Mac OS X
--------
To build mphidflash for Mac, you'll need to have Xcode (Apple's
developer tools) already installed.  The IDE portion is not used here, just
the command line interface.

Assuming you're reading this as the README.txt alongside the source code,
to compile mphidflash, in the Terminal window type:

	make

Then install with the command:

	sudo make install

This will copy the 'mphidflash' executable to /usr/local/bin so you don't need
to specify a complete path to the program each time.


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

This will create 'mphidflash.exe' which can called from the Windows commmand
line.


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
-vendor <hex>	Use given USB vendor id instead of default id
-product <hex>	Use given USB product id instead of default id

Example: To upload the program test.hex to the PIC and to reset the PIC thereafter
the following command line can be used:

	mphidflash -write test.hex -reset

