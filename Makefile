OBJ=common.o config.o apps.o platforms.o elf.o command-line.o desktopfiles.o regedit.o doom.o download.o find_files.o find_program.o packages.o native.o install.o uninstall.o run-application.o xrandr.o
LIBS= -lssl -lcrypto -lUseful-5  
prefix=/usr/local
exec_prefix=${prefix}
CFLAGS=-g -O2 -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_LIBUSEFUL_5_LIBUSEFUL_H=1 -DHAVE_LIBUSEFUL5=1 -DHAVE_LIBUSEFUL_5=1 -DHAVE_LIBCRYPTO=1 -DHAVE_LIBSSL=1 -DINSTALL_PREFIX=\"/usr/local\"

all: $(OBJ) patches-subdir main.c 
	$(CC) $(CFLAGS) -osommelier $(OBJ) $(LIBUSEFUL) $(LIBS) main.c 

libUseful-5/libUseful.a:
	$(MAKE) -C libUseful-5

common.o: common.h common.c
	$(CC) $(CFLAGS) -c common.c

config.o: config.h config.c
	$(CC) $(CFLAGS) -c config.c

apps.o: apps.h apps.c
	$(CC) $(CFLAGS) -c apps.c

doom.o: doom.h doom.c
	$(CC) $(CFLAGS) -c doom.c

elf.o: elf.h elf.c
	$(CC) $(CFLAGS) -c elf.c

platforms.o: platforms.h platforms.c
	$(CC) $(CFLAGS) -c platforms.c

command-line.o: command-line.h command-line.c
	$(CC) $(CFLAGS) -c command-line.c

desktopfiles.o: desktopfiles.h desktopfiles.c
	$(CC) $(CFLAGS) -c desktopfiles.c

download.o: download.h download.c
	$(CC) $(CFLAGS) -c download.c

find_files.o: find_files.h find_files.c
	$(CC) $(CFLAGS) -c find_files.c

find_program.o: find_program.h find_program.c
	$(CC) $(CFLAGS) -c find_program.c

packages.o: packages.h packages.c
	$(CC) $(CFLAGS) -c packages.c

native.o: native.h native.c
	$(CC) $(CFLAGS) -c native.c

install.o: install.h install.c
	$(CC) $(CFLAGS) -c install.c

uninstall.o: uninstall.h uninstall.c
	$(CC) $(CFLAGS) -c uninstall.c

regedit.o: regedit.h regedit.c
	$(CC) $(CFLAGS) -c regedit.c

run-application.o: run-application.h run-application.c
	$(CC) $(CFLAGS) -c run-application.c

xrandr.o: xrandr.h xrandr.c
	$(CC) $(CFLAGS) -c xrandr.c


patches-subdir:
	- $(MAKE) -C patches

clean:
	rm -f sommelier *.o libUseful-5/*.o libUseful-5/*.so libUseful-5/*.a patches/*.so

install:
	mkdir -p $(HOME)/bin
	mkdir -p $(HOME)/.sommelier
	mkdir -p $(HOME)/.sommelier/patches
	cp sommelier $(HOME)/bin/
	cp config/*.apps config/*.conf $(HOME)/.sommelier/
	-cp patches/*.so $(HOME)/.sommelier/patches

install_global:
	mkdir -p $(DESTDIR)$(exec_prefix)/bin
	mkdir -p $(DESTDIR)$(exec_prefix)/etc/sommelier
	cp sommelier $(DESTDIR)$(exec_prefix)/bin/
	cp config/*.apps config/*.conf $(DESTDIR)$(exec_prefix)/etc/sommelier/
	cp sommelier.1 $(DESTDIR)$(exec_prefix)/share/man/man1/

test:
	echo "no tests"
