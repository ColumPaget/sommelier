OBJ=common.o config.o apps.o platforms.o command-line.o desktopfiles.o regedit.o doom.o download.o install.o uninstall.o
LIBS=libUseful-4/libUseful.a -lssl -lcrypto  
prefix=/usr/local
exec_prefix=${prefix}
CFLAGS=-g -O2 -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_LIBCRYPTO=1 -DHAVE_LIBSSL=1 -DINSTALL_PREFIX=\"/usr/local\"

all: $(OBJ) main.c libUseful-4/libUseful.a
	$(CC) $(CFLAGS) -osommelier $(OBJ) $(LIBUSEFUL) $(LIBS) main.c 

libUseful-4/libUseful.a:
	$(MAKE) -C libUseful-4

common.o: common.h common.c
	$(CC) $(CFLAGS) -c common.c

config.o: config.h config.c
	$(CC) $(CFLAGS) -c config.c

apps.o: apps.h apps.c
	$(CC) $(CFLAGS) -c apps.c

doom.o: doom.h doom.c
	$(CC) $(CFLAGS) -c doom.c

platforms.o: platforms.h platforms.c
	$(CC) $(CFLAGS) -c platforms.c

command-line.o: command-line.h command-line.c
	$(CC) $(CFLAGS) -c command-line.c

desktopfiles.o: desktopfiles.h desktopfiles.c
	$(CC) $(CFLAGS) -c desktopfiles.c

download.o: download.h download.c
	$(CC) $(CFLAGS) -c download.c

install.o: install.h install.c
	$(CC) $(CFLAGS) -c install.c

uninstall.o: uninstall.h uninstall.c
	$(CC) $(CFLAGS) -c uninstall.c


regedit.o: regedit.h regedit.c
	$(CC) $(CFLAGS) -c regedit.c


clean:
	rm -f sommelier *.o libUseful-4/*.o libUseful-4/*.so libUseful-4/*.a

install:
	mkdir -p $(HOME)/bin
	mkdir -p $(HOME)/.sommelier
	cp sommelier $(HOME)/bin/
	cp *.apps *.conf $(HOME)/.sommelier/

install_global:
	mkdir -p $(DESTDIR)$(exec_prefix)/bin
	mkdir -p $(DESTDIR)$(exec_prefix)/etc/sommelier
	cp sommelier $(DESTDIR)$(exec_prefix)/bin/
	cp *.apps *.conf $(DESTDIR)$(exec_prefix)/etc/sommelier/
	cp sommelier.1 $(DESTDIR)$(exec_prefix)/share/man/man1/

test:
	echo "no tests"
