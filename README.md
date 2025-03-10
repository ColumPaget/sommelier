[![Build Status](https://travis-ci.com/ColumPaget/Sommelier.svg?branch=master)](https://travis-ci.com/ColumPaget/Sommelier)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

SYNOPSIS
=========

Sommelier is an installer program that downloads and installs packages/programs that run under an emulator. As the name implies, the main target emulator is wine, (mostly 32-bit wine) though sommelier can also install some dosbox applications, a few linux native applications since version 4.7, and since version 4.0 some doom wads, scummvm games and numerous gog.com games. Sommelier downloads an application and any dependancies (MFC, VB6) that it may have. Downloads are checked against an expected sha256 sum. Each application is installed in it's own directory-structure (AKA 'wine-bottle') under ~/.sommelier, unless '-S' or '-system' options are used.  Sommelier creates a .desktop file for each application in ~/.local/share/applications. If needed, registry entries are changed within the wine-bottle (e.g. some applications may need to be run in virtual-desktop mode, or may need to disallow window managing by the system window-manager. The wine-bottle approach allows registry changes to be made independantly for each application). The .desktop file is used to run the application by the 'sommelier run' command.

When installing native linux applications, sommelier tries to detect whether the system is a 32bit linux, or a 64bit one. It does this by checking which platform it was compiled for. This means you can run sommelier on a 32bit linux running on a 64bit processor, or even on a system that has a 64bit kernel but 32bit applications, and so long as sommelier has been compiled 32bit, it will install 32bit versions of apps. If sommelier is compiled for 64bit, it will install linux apps for 64bit rather than 32bit.

Sommelier can install many gog.com games, that either run under wine, native linux, dosbox, scummvm, or a neogeo emulator. If a game is in the list it means I've had it install and run successfully. For some reason I find fewer native games work for me than emulated games under wine. Unfortuantely you currently have to download all the files for a gog game with your browser and then install them with './sommelier install <game name> -url <path to installer>'.

Many wine apps from gog are 32-bit applications, so you'll need to 'apt get wine32' to run those on most 64-bit distros.

As of version 5.0 sommelier has been seen to install apps that are automatically imported in the XFCE, MATE and Moksha desktop environments, with the apps appearing in the system menu (unfortuantely most apps currently lack an icon to go with the entry).

The '-S' and '-system' command-line options are intended for use with native linux apps, and these are installed system-wide in a subdirectory of '/opt' rather than under '.sommelier' in the user's home directory. This will require sommelier to have write access to '/opt', which normally means running it as root. Applications run in this fashion aren't expected to be run by sommelier using 'sommelier run <name>' but instead to be added to the users PATH and run as normal native apps.

LICENSE
=======

Sommelier is released under the GPLv3 license.


AUTHOR
======

Sommelier is written by Colum Paget. All patches/bugreports/requests should be sent to colums.projects@gmail.com, or handled through the project's github page.


INSTALL
=======

The install consists of the main executable and a number of '.apps' files (wine.apps, msdos.apps, scummvm.apps, gog.com.apps) that configure installable applications. 

The usual install process is:

```
./configure --enable-ssl
make
make install
```

This will place the sommelier executable in `$(HOME)/bin` and the '.apps' configuration files in `$(HOME)/.sommelier`. If `$(HOME)/bin` is not in your path then you may with to move the executable to somewhere that is. Note '--enable-ssl' is generally required so that sommelier can download https:// links

For a 'global' install that applies to all users do:

```
./configure --prefix=<prefix> --enable-ssl
make
make install_global
```

This will place the sommelier executable in `<prefix>/bin` and combine the '.apps' config files into `<prefix>/etc/sommelier`. This install method will also honor the `$DESTDIR` environment variable that is by package used creators to install under a specified 'root' directory so that everything under that directroy can be combined into a distributable package.

If you want to install the executable at one prefix, and the configuration at another, say putting the executable in `/usr/local/bin` but the config files in `/etc/sommelier` then you can do:

```
./configure --prefix=/ --exec-prefix=/usr/local --enable-ssl
make
make install_global
```


USAGE
=====

Normally you will just want to do:

`sommelier list` 

to see a list of applications, and then:

`sommelier install <application>`

to install one. Then the program can be run with:

`sommelier run <application>`



if sommelier installed a version of the application for the wrong platform use:

`sommelier install <application> -platform <platform>`



for gog games, or any installer that's at a specific path/url you can use:

`sommelier install <application> -url <url>`

gog game installers will expect to find any files they need (e.g. '.bin' files) in the same directory as themselves 



various settings for an installed application can be changed using:

`sommelier set <setting> <application>`



if something is changed in the '.app' file that defines an application, then an installed application can be reconfigured to use the latest setup using:

`sommelier reconfigure <application>`



finally an application can be uninstalled with

`sommelier uninstall <application>`




DETAILED USAGE
==============

```
sommelier platforms                                print list of supported platforms
sommelier categories                               print list of application categories
sommelier list [options]                           print list of apps available for install
sommelier install <name> [<name>] [options]        install an application by name
sommelier uninstall <name> [<name>]                uninstall an application by name
sommelier reconfig <name> [<name>]                 reconfigure an installed application (seek for executables, re-write desktop file)
sommelier reconfigure <name> [<name>]              reconfigure an application by name
sommelier download <name> [<name>]                 download installers/packages to the current directory
sommelier run <name> [<options>]                   run an application by name
sommelier winecfg <name> [<options>]               run winecfg for named wine application
sommelier set <setting string> <name> [<name>]     change settings of installed applications listed by name
sommelier autostart                                load programs out of ~/.config/autostart

options are:
  -d                            print debugging (there will be a lot!)
  -debug                        print debugging (there will be a lot!)
  -c <config file>              specify a config (list of apps) file, rather than using the default
  -url                          supply an alternative url for an install (this can be an http, https, or ssh url, or just a file path. File paths must be absolute, not relative)
  -n <name>                     Name that program will be installed under and called/run under
  -install-name <name>          Name that program will be installed under and called/run under
  -install-as <name>            Name that program will be installed under and called/run under
  -f                            force install even if expected sha256 doesn't match the download
  -force                        force install even if expected sha256 doesn't match the download
  -proxy <url>                  use a proxy for downloading installs
  -platform <platform>          platform to use when installing or displaying lists of apps
  -category <category>          category to use when displaying lists of apps
  -installed                    display only installed app when displaying lists of apps
  -k                            keep installer or .zip file instead of deleting it after install
  -S                            install app system-wide under /opt, to be run as a normal native app
  -system                       install app system-wide under /opt, to be run as a normal native app
  -icache <dir>                 installer cache: download installer to directory'dir' and leave it there
  -emu <emulator>               specify a specific emulator to use when installing an app
  -emulator <emulator>          specify a specific emulator to use when installing an app
  -hash                         hash downloads even if they have no expected hash value
  -no-xrandr                    don't use xrandr to reset screen resolution after running and application
  -user-agent <agent string>    set user-agent to send when communicating over http
  -ua <agent string>            set user-agent to send when communicating over http
```


The 'set' command can change various settings of an installed application on a setting-by-setting bases (see 'SETTINGS' below). Settings are also used in the config files for installing apps. The 'reconfigure' command sets all the settings from the configuration file, and rebuilds the .desktop file that specifies how to run an application, allowing the whole setup of the installed application to be changed.

The 'run' command can take arguments that are passed to the program. For instance, to run Telegram desktop in 'start in systray' mode use:

```
	sommelier run Telegram -startintray
```

Some emulators also have 'extra' arguments that can be used with the 'run' command. `sommelier platforms` should show those.



PLATFORMS
=========

Platforms are defined in the `platforms.conf` file that's installed into the `.sommelier` directory. A platform is a configuration that defines a type of program and the runtime/emulator needed to run it. Each of these platforms can have multiple emulators added with the 'emu' variable.

The following platforms are defined by the default `platforms.conf` file. A platform value not only relates to the emulator that an app will be run under but also to some configuration that might be set for an emulator and also to how a program should be unpacked and installed. Platforms are defined in the platforms.conf file that is installed into the .sommelier config directory along with the '.apps' config files.

dos
 : msdos apps that run under dosbox
msdos
 : msdos apps that run under dosbox
win
 : windows apps that run under wine (equivalent to win32)
windows
 : windows apps that run under wine (equivalent to win32)
wine
 : windows apps that run under wine (equivalent to win32)
win16
 : 16-bit windows apps that run under wine
win32
 : 32-bit windows apps that run under wine
win64
 : 64-bit windows apps that run under wine
linux32
 : 32-bit native linux application
linux64
 : 64-bit native linux application
doom
 : doom wad that can be run under a doom source port
spectrum
 : spectrum app that can be run under the fuse emulator
zx48
 : spectrum app that can be run under the fuse emulator
scummvm
 : adventure game that can run under the scummvm emulator
gog:win
 : windows app from gog.com that runs under wine
gog:windows
 : windows app from gog.com that runs under wine
gog:lin
 : linux app from gog.com that runs natively
gog:linux
 : linux app from gog.com that runs natively
gog:lindos
 : msdos app from gog.com that's packaged for linux
gog:windos
 : msdos app from gog.com that's packaged for windows
gog:scummvm
 : scummvm adventure game from gog.com that's packaged for linux
gog:neogeo
 : neogeno game from gog.com that's packaged for linux, run using mame, xmame or gngeo emulators
megadrive
 : sega megadrive games run under the dgen or mednafen emulators
segamaster
 : sega master system run under the mednafen emulator
gba
 : game boy advanced games run under mgba or mednafen emulators
nes
 : nintendo entertainment system games run under the dnes or mednafen emulators
3ds
 : 3ds games run under the citra emulator
vectrex
 : vectrex games run using the 'vecx' emulator
ps1
 : playstation 1 games that run under the mednafen emulator
turbografx
 : turbografx/pc-engine games that run under the mednafen emulator
wonderswan
 : wonderswan games that run under the mednafen emulator
atarilynx
 : atari lynx games that run under the mednafen emulator
atarist
 : atari ST games that run under the hatari emulator
mame:
 : apps that run under the mame emulator
mono
 : apps that run under the mono C# runtime
java
 : apps that run under the java runtime




PROXY URLS
==========

The `-proxy` command accepts proxy urls of the form:
```
     <protocol>:<user>:<password>@<host>:<protocol>. 
```

'protocol' can be 'socks4', 'socks5' 'https' or 'sshtunnel'. For 'sshtunnel' the names defined in the ~/.ssh/config file can be used, so that  most of the information can be ommited.

examples:
```
   https:bill:secret@proxy.com
   socks4:proxy.com:1080
   socks5:bill:secret@proxy.com:1080
   sshtunnel:bill:secret@ssh_host.com
   sshtunnel:sshproxy
```



SETTINGS
========

There are a number of settings that can be lastingly configured with the 'set' command or using the `-set` option when installing an application. Currently these settings only relate to programs run under wine, or doom wads run under chocolate-doom or crispy-doom.

```
vdesk=y/n/<geometry>    run program within a window/virtual desktop
fullscreen=y/n          run program at fullscreen, or else within a virtual desktop
winmanage=y/n           Wine only: allow window manager to decorate and manage windows of this program
smoothfonts=y/n         Wine only: use font anti-aliasing
os-version=<version>    Wine only: set OS version to one of those supported by wine
sound=y/n/sfx           DOOM only: sound on/off, or only effects (no music)
mouse=y/n               DOOM only: use mouse in-game, or not
grab=y/n                DOOM only: grab mouse, or not
```

For both DOOM and Wine you can set the size of the window using the vdesk setting, in the style `vdesk=600x300`.



ADDING APPLICATIONS
===================

Application config files for each platform are stored in `.sommelier` and named `<platform>.apps`. Each app is configured with a line that looks like:


```
Basilisk platform=windows url=http://eu.basilisk-browser.org/release/basilisk-latest.win32.installer.exe exec='basilisk.exe' type='webrowser' sha256='05240bb65daf43fdda01cc45b483a4a9326cd3f8e4f2123e8ec7d0b7ec958341' 
```

These config lines can contain the following values:

```
platform               platform that this app runs on. Required.
category               program category for desktop files.
url                    download url if one exists.
sha256                 sha256 sum/hash of the installer or package file.
installer              if the downloaded package contains an installer or setup program, then sets its name.
requires               if another package must be installed for this to run, then this is the name of that package.
referer                if a website only allows downloads referred from a specific webpage. e.g. referer='http://www.singlecellsoftware.com/caustic'
exec                   the executable file (or emulator rom file) that should be run. If not supplied sommelier will try to auto-find one. Optional, but recommended.
exec64                 the executable file (or emulator rom file) that should be run for 64bit systems. If not supplied sommelier will try to auto-find one. Optional, but recommended.
exec-args              Command-line arguments to be supplied to app. e.g. Darkstone platform=gog:windows exec=Darkstone.exe exec-args="-nointro" warn="Intro video disabled as it blocks the game"
exec-dir               Directory that contains the executable. If set in the config file it is used to distinguish between multiple executables with the same name. Otherwise it is automatically set when the executable is found.
no-exec-arg            A boolean flag that indicates the path to the executable is not to be passed to the emulator. This is mostly used for dosbox emulation where a list of commands is passed instead. In that case, the 'exec' option is still used to find the directory that contains all the program files, but the program is run using dosboxe's '-c' option, and so does not need to be suplied on the command-line.
emulator-args          Command-line arguments to be supplied to the emulator or vm that runs this app. e.g. Tyrian2000 platform=gog:windos exec="tyrian.exe" emulator-args="-conf dosboxT2K.conf"
installer-args         Args to pass to the installer program.
working-dir            Directory to run the program in if it needs to be run in a directory other than the one it was installed in. e.g.  working-dir="$(install-dir)/data/noarch/game"
dlc                    this package is DownLoadable Content for another package. e.g. 'ColorOfMadness platform=gog:linux dlc parent=DarkestDungeon'
parent                 for DLC this is the parent app. e.g. 'ColorOfMadness platform=gog:linux dlc parent=DarkestDungeon'
os-version             WINE only. Set the version of windows to emulate for this app.
winmanager             WINE only. Allow system window manager to manage this app. 'y' or 'n'. Default 'y'.
vdesk                  WINE and DOOM, run within a virtual desktop, 'y', 'n' or '<geometry>'. Default 'n'.
installer-vdesk        run the installer within a virtual desktop, 'y', 'n'. Default 'n'.
download-type          filetype of downloaded file.
dlname                 name of downloaded file. Used when the url path does not include the correct filename.
warn                   warnings to be displayed when the program is installed.
warn-missingpath       warning to be displayed if a path is missing on the system. e.g.  exec=kitty warn-missingpath='/usr/share/X11/xkb:This program requires directory /usr/share/X11/xkb'
bundled                If this app is bundled with another one, then this is the name of the other one.
delete                 After install delete these files. e.g. "delete='tmpfiles/*'"       
movefiles-from         After install move files to the install directory. e.g. `movefiles-from='gearhead-1/*'`
movefiles-to           After install move files from install directory. e.g. `movefiles-to=*/msftedit.dll:$(drive_c)/windows/system32`
copyfiles-from         After install copy files to the install directory. e.g. `copyfiles-from='gearhead-1/*'`
copyfiles-to           After install copy files from install directory. e.g. `copyfiles-to=*/msftedit.dll:$(drive_c)/windows/system32`
link                   After install create a symlink from one path to another. *THIS REPLACES ANYTHING OCCUPYING THE SYMLINKS PATH*. 
ld_preload             libraries to preload before running exectuable. e.g. `ld_preload="$(sommelier_patches_dir)/anomaly_warzone_patch.so"`
type                   type of program.
comment                comment.
```

Any value not recognized from the above list will simply be set as a variable that can be used in other values, so for instance some app configs contain variables like 'patch-dir' which are used in other values like 'copyfiles-to'.



ADDING EMULATORS
================

emulators can be added in 'platforms.conf'. These have the format:


```�
<names> platform=<platform> emu=<emulator command line> exec=<exec patterns> 
```�

where:

```�
names           list of names that this platform is known under
platform        the actual platform name that sommelier understands and acts on
emu             emulator command line, which can contain variables
exec            patterns that match the 'executable' file to pass to the emulator, so '*.jar' for java, '*.z80' for zxspectrum

```�

the 'exec' pattern is used to find the 'executable' or main rom to pass to the emulator, of which only one is expected. this appears as a variable called 'exec-path' which can then be included in the 'emu' option.



EMULATOR ROMS
=============

Some emulators require a rom file. For these you'll have to make a script that runs the emulator with the correct args. E.g. for 'vecx' we'd need a script like:


```
#!/bin/sh
  
vecx --bios /usr/share/vecx/bios.bin "$@"
```

and change the 'emu' entry in platforms.conf to run the script instead of the emulator directly.




SAVES DIR
=========

A new 'saves directory' system tries to redirect the 'saved games' folder of each game to '.game-saves' in the user's home directory. This is intended to reduce pollution of the users home and '~/Documents' directories with game-data, and more generally to ensure that game-state is separated from game-runtime so that the game can be completely removed/reinstalled without losing saved games. It is currently supported for windows games and a few gog dos games, including  Albion, ArchimedianDynasty, DungeonKeeper, FantasyGeneral, HeroesOfMightAndMagic, HeroesOfMightAndMagic2, and TombRaider,




ENVIRONMENT VARIABLES
=====================

Sommelier looks for the variables `SOMMELIER_CA_BUNDLE`, `CURL_CA_BUNDLE` and `SSL_VERIFY_FILE`, in that order, to discover the path of the Certificate Bundle for certificate verification.
If `SOMMELIER_INSTALLER_CACHE` is set, sommelier will download installer and .zip files to the specified directory, and leave them there for future use with the `-url` option.



AUTOSTART
=========

sommelier can emulate a desktop environment that autostarts programs using .desktop files in `~/.config/autostart`. This is to assist window-managers that don't support this feature.


TO DO
=====

* Application Sandboxing
* Commands to add to application list
* Set default audio device with 'sommelier set'

