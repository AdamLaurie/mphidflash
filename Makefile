VERSION_MAIN = 1
VERSION_SUB  = 8

CC       = gcc
OBJS     = main.o hex.o
EXECPATH = binaries
DISTPATH = dist
STRIP   := strip

ifeq ($(shell uname -s),Darwin)
# Rules for Mac OS X
  OBJS    += usb-osx.o
  CFLAGS   = -fast
  LDFLAGS  = -Wl,-framework,IOKit,-framework,CoreFoundation
  SYSTEM = osx
else
# Rules for Linux, etc.
  OBJS    += usb-libusb.o
  CFLAGS   = -O3 
  LDFLAGS  = -lusb
  SYSTEM = linux
endif

CFLAGS += -DVERSION_MAIN=$(VERSION_MAIN) -DVERSION_SUB=$(VERSION_SUB)
#CFLAGS += -DDEBUG

all: 
	@echo
	@echo Please make 
	@echo - 'mphidflash32' for i386 version
	@echo - 'mphidflash64' for amd64 version
	@echo - 'mphidflash-armel' for armel version
	@echo - 'mphidflash-armhf' for armhf version
	@echo

*.o: mphidflash.h

.c.o:
	$(CC) $(CFLAGS) -c $*.c

mphidflash64: CFLAGS += -m64
mphidflash64: LDFLAGS += -m64
mphidflash64: EXEC = mphidflash-$(VERSION_MAIN).$(VERSION_SUB)-$(SYSTEM)-64
mphidflash64: mphidflash

mphidflash-armel: EXEC = mphidflash-$(VERSION_MAIN).$(VERSION_SUB)-armel
mphidflash-armel: CC = arm-linux-gnueabi-gcc-10
mphidflash-armel: LDFLAGS += -L=/usr/lib/arm-linux-gnueabi/
mphidflash-armel: mphidflash

mphidflash-armhf: EXEC = mphidflash-$(VERSION_MAIN).$(VERSION_SUB)-armhf
mphidflash-armhf: CC = arm-linux-gnueabihf-gcc-10
mphidflash-armhf: LDFLAGS += -L=/usr/lib/arm-linux-gnueabihf/
mphidflash-armhf: mphidflash

mphidflash32: CFLAGS += -m32 
mphidflash32: LDFLAGS += -m32
mphidflash32: EXEC = mphidflash-$(VERSION_MAIN).$(VERSION_SUB)-$(SYSTEM)-32
mphidflash32: mphidflash

mphidflash: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(EXECPATH)/$(EXEC)
	if [ "$(CFLAGS)" = *"-m64"* ] || [ "$(CFLAGS)" = *"-m32"* ] ; then \
		$(STRIP) $(EXECPATH)/$(EXEC) ;\
	fi


install:
	@echo
	@echo Please make 'install32 or install64' to install 32 or 64 bit target
	@echo

# Must install as root; e.g. 'sudo make install'
install32: mphidflash32
	cp $(EXECPATH)/mphidflash-$(VERSION_MAIN).$(VERSION_SUB)-$(SYSTEM)-32 /usr/local/bin/mphidflash

install64: mphidflash64
	cp $(EXECPATH)/mphidflash-$(VERSION_MAIN).$(VERSION_SUB)-$(SYSTEM)-64 /usr/local/bin/mphidflash

clean:
	rm -f *.o core


bindist: tarball zipfile

srcdist: src-tarball src-zipfile

tarball:
	tar cvzf $(DISTPATH)/mphidflash-$(VERSION_MAIN).$(VERSION_SUB)-bin.tar.gz README.txt CHANGELOG COPYING $(EXECPATH)/*$(VERSION_MAIN).$(VERSION_SUB)*

src-tarball:
	tar cvzf $(DISTPATH)/mphidflash-$(VERSION_MAIN).$(VERSION_SUB)-src.tar.gz README.txt CHANGELOG COPYING Makefile* *.c *.h

zipfile:
	rm -f $(DISTPATH)/mphidflash-$(VERSION_MAIN).$(VERSION_SUB)-bin.zip
	zip $(DISTPATH)/mphidflash-$(VERSION_MAIN).$(VERSION_SUB)-bin.zip README.txt CHANGELOG COPYING $(EXECPATH)/*$(VERSION_MAIN).$(VERSION_SUB)*

src-zipfile:
	rm -f $(DISTPATH)/mphidflash-$(VERSION_MAIN).$(VERSION_SUB)-src.zip
	zip $(DISTPATH)/mphidflash-$(VERSION_MAIN).$(VERSION_SUB)-src.zip README.txt CHANGELOG COPYING Makefile* *.c *.h

