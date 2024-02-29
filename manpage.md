title: sommelier
mansection: 1
date: 2020/05/17


NAME
====

sommelier - installer for apps that run under wine or other emulators


SYNOPSIS
========

```
sommelier platforms                                print list of supported platforms
sommelier list [options]                           print list of apps available for install. use -platform option to display apps for a given platform
sommelier install <name> [<name>] [options]        install an application by name
sommelier uninstall <name> [<name>]                uninstall an application by name
sommelier reconfig <name> [<name>]                 reconfigure an installed application (seek for executables, re-write desktop file)
sommelier reconfigure <name> [<name>]                 reconfigure an installed application (seek for executables, re-write desktop file)
sommelier run <name>                               run an application by name
sommelier winecfg <name>                           run 'winecfg' for named wine application
sommelier download <name>                          just download installer/package to current directory
sommelier set <setting string> <name> [<name>]     change settings of an installed application
sommelier autostart                                run programs from ~/.config/autostart

```

DESCRIPTION
===========

Sommelier is an installer for downloadable applications that can be run under a number of emulators. The main focus is windows applications that can be run under the 'wine' emulator, but currently it also supports ms-dos applications that can be run under 'dosbox'; adventure games that can be run under the'scummvm' emulator; zx-spectrum apps that can be run under the 'fuse' emulator; downloads from gog.com that can be run either under wine, dosbox, or natively on linux; and doom wads that can be run under 'chocolate-doom' or 'crispy-doom'. 

Installed applications are stored under a '.sommelier' directory in the user's home directory, each in a subdirectory of their own (what's known as a 'wine bottle' for wine apps). Also in this directory are stored '.apps' configuration files that describe the list of installable apps.


OPTIONS
=======

-d
 : print debugging (there will be a lot!)
-c <config file>
 : specify a config (list of apps) file, rather than using the default search path
-url
 : supply an alternative url for an install (this can be an http, https, or ssh url, or just a file path
-f
 : force install even if expected sha256 doesn't match the download
-force
 : force install even if expected sha256 doesn't match the download
-install-name <name>
 : Name that program will be installed under and called/run under
-install-as <name>
 : Name that program will be installed under and called/run under
-proxy <url>
 : use a proxy for downloading installs
-platform <platform>
 : specify platform to display when listing apps, or if installing an app that has multiple platforms
-category <category>
 : category to use when displaying lists of apps
-installed
 : display only installed app when displaying lists of apps
-k
 : keep installer or .zip file instead of deleting it after install
-S
 : install app system-wide under /opt, to be run as a normal native app
-system
 : install app system-wide under /opt, to be run as a normal native app
-icache <dir>
 : installer cache: download installer to directory 'dir' and leave it there
-hash
 : hash downloads even if they have no expected hash value
-s
 : set a value at install
-set
 : set a value at install
-no-xrandr
 : don't use xrandr to reset screen resolution after running and application
-user-agent <agent string>
 : set user-agent to send when communicating over http
-ua <agent string>
 : set user-agent to send when communicating over http

EXAMPLE USAGE
=============

List installable applications
 : sommelier list 
List installable applications for a given platform
 : sommelier list -platform doom
Install apps 
 : sommelier install SeaMonkey PaleMoon
Install app from a predownloaded file
 : sommelier install SpaceCom -url setup_spacecom_2.4.0.6.exe
Install app for a particular platform when it's available in more than one
 : sommelier install DarkestDungeon -platform gog:linux -url setup_darkest_dungeon.sh
Install app from a predownloaded file
 : sommelier install SpaceCom -url setup_spacecom_2.4.0.6.exe
Install app and set a setting
 : sommelier install Doom:Strain -s fullscreen=n
Uninstall application
 : sommelier uninstall PaleMoon
Run application
 : sommelier run PaleMoon
Run application and pass it arguments
 : sommelier run Telegram -startintray
Set a setting for some apps
 : sommelier set fullscreen=n Doom:Strain Doom:Hacx


PLATFORMS
=========

The following platforms are supported for use in the `-platform` option, or in application configurations in the config files. A platform value not only relates to the emulator that an app will be run under but also to some configuration that might be set for an emulator and also to how a program should be unpacked and installed.

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
 : msdos app from from gog.com that's packaged for linux
gog:windos
 : msdos app from from gog.com that's packaged for windows
gog:scummvm
 : scummvm adventure game from from gog.com that's packaged for linux
megadrive
 : sega megadrive games run under the dgen emulator
segamaster
 : sega master system run under the mednafen emulator
gba
 : game boy advanced games run under mgba or mednafen emulators
nes
 : nintendo entertainment system games run under the dnes emulator




PROXY URLS
==========

The `-proxy` option is used with the 'download' or 'install' commands and accepts proxy urls of the form:

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

vdesk=y/n/<geometry>
 : run program within a window/virtual desktop
fullscreen=y/n
 : run program at fullscreen, or else within a virtual desktop
winmanage=y/n
 : wine only: allow window manager to decorate and manage windows of this program
smoothfonts=y/n
 : wine only: use font anti-aliasing
os-version=<version>
 : wine only: set OS version to one of those supported by wine
sound=y/n/sfx
 : DOOM only: sound on/off, or only effects (no music)
mouse=y/n
 : DOOM only: use mouse in-game, or not
grab=y/n
 : DOOM only: grab mouse, or not

For both DOOM and Wine you can set the size of the window using the vdesk setting, in the style `vdesk=600x300`.


ENVIRONMENT VARIABLES
=====================

Sommelier looks for the variables `SOMMELIER_CA_BUNDLE`, `CURL_CA_BUNDLE` and `SSL_VERIFY_FILE`, in that order, to discover the path of the Certificate Bundle for certificate verification.
If `SOMMELIER_INSTALLER_CACHE` is set, sommelier will download installer and .zip files to the specified directory, and leave them there for future use with the `-url` option.
