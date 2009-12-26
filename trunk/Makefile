CC    = gcc
EXECS = mphidflash
OBJS  = main.o hex.o

ifeq ($(shell uname -s),Darwin)
# Rules for Mac OS X
  OBJS    += usb-osx.o
  CFLAGS   = -fast
  LDFLAGS  = -Wl,-framework,IOKit,-framework,CoreFoundation
  SYSTEM = osx
else
# Rules for Linux, etc.
  OBJS    += usb-linux.o
  CFLAGS   = -O3 
  LDFLAGS  = -lhid -lusb
  SYSTEM = linux
endif

all: $(EXECS)

*.o: mphidflash.h

.c.o:
	$(CC) $(CFLAGS) -c $*.c
 
mphidflash: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o mphidflash
	strip mphidflash

# Must install as root; e.g. 'sudo make install'
install: mphidflash
	cp mphidflash /usr/local/bin

clean:
	rm -f $(EXECS) *.o core

bindist: $(EXECS)
	tar cvzf mphidflash-bin-$(SYSTEM).tar.gz README.txt CHANGELOG COPYING $(EXECS)

