# MPHidFlash #

## Description ##
MPHidFlash is a command-line tool for communicating with the USB-Bootloader in PIC microcontrollers. It supports Microchips HID-Bootloader protocol and runs on various plattforms (Windows, Linux, MacOS X).

This tool is based on the programm 'ubw32' developed by Phillip Burgess.

## Download ##
The latest stable release is MPHidFlash version 1.6.

### Version 1.6 for Linux 32/64 Bit, Mac OS-X 32/64 Bit and Windows 32 Bit (2014-07-07) ###
  * [MPHidFlash version 1.6 binaries - tar archive](https://mphidflash.googlecode.com/svn/trunk/dist/mphidflash-1.6-bin.tar.gz)
  * [MPHidFlash version 1.6 binaries - zip archive](https://mphidflash.googlecode.com/svn/trunk/dist/mphidflash-1.6-bin.zip)
  * [MPHidFlash version 1.6 source - tar archive](https://mphidflash.googlecode.com/svn/trunk/dist/mphidflash-1.6-src.tar.gz)
  * [MPHidFlash version 1.6 source - zip archive](https://mphidflash.googlecode.com/svn/trunk/dist/mphidflash-1.6-src.zip)

Older versions can be found here:

### Version 1.5 Binaries ###
  * [linux 64 bit](https://mphidflash.googlecode.com/svn/trunk/binaries/mphidflash-1.5-linux-64)
  * [mac osx 32 bit](https://mphidflash.googlecode.com/svn/trunk/binaries/mphidflash-1.5-osx-32)
  * [mac osx 64 bit](https://mphidflash.googlecode.com/svn/trunk/binaries/mphidflash-1.5-osx-64)
  * [windows 32 bit](https://mphidflash.googlecode.com/svn/trunk/binaries/mphidflash-1.5-win-32.zip)

### Version 1.4 emergency patch (2010-12-28) ###
  * [Issue #4 with workaround for Verify Error](https://code.google.com/p/mphidflash/issues/detail?id=4&can=1)

### Version 1.3 (2009-12-26) ###
  * [MPHidFlash version 1.3 source code](http://mphidflash.googlecode.com/files/mphidflash-1.3-src.tar.gz)
  * [MPHidFlash version 1.3 windows binary](http://mphidflash.googlecode.com/files/mphidflash-1.3-bin-win.zip)
  * [MPHidFlash version 1.3 linux binary (needs installed libhid)](http://mphidflash.googlecode.com/files/mphidflash-1.3-bin-linux.tar.gz)
  * [MPHidFlash version 1.3 mac os x binary](http://mphidflash.googlecode.com/files/mphidflash-1.3-bin-osx.tar.gz)

## Usage ##
To upload a new program to your PIC, it must be connected to your computer and
set into bootloader mode. For example, to upload the program test.hex to the PIC and to reset the PIC thereafter the following command line can be used:
```
mphidflash -write test.hex -reset
```

For a description of all available options type:
```
mphidflash -help
```


## Bootloader Firmware ##
MPHidFlash supports bootloaders which uses Microchips HID class bootloader protocol. Application notes and examples of firmware for PIC18, PIC24 and PIC32 are included in the [Microchip MCHPFSUSB Framework](http://www.microchip.com/stellent/idcplg?IdcService=SS_GET_PAGE&nodeId=2680&dDocName=en537044). You find it in the folder "USB Device - Bootloaders/HID - Bootloader".

### Devices with preprogrammed HID-Bootloader ###
  * [PIC18F46J50 FS USB Demo Board](http://www.microchip.com/stellent/idcplg?IdcService=SS_GET_PAGE&nodeId=1406&dDocName=en540669)
  * [UBW32 (32 bit PIC32 based USB Bit Whacker)](http://www.schmalzhaus.com/UBW32/)
  * [USBnub - A small PIC develoment board](http://www.fundf.net/usbnub/)
  * [RFIDler - A Software Defined RFID Reader/Writer/Emulator](https://github.com/ApertureLabsLtd/RFIDler)