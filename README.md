[![Build Status](https://travis-ci.com/ColumPaget/Sommelier.svg?branch=master)](https://travis-ci.com/ColumPaget/Sommelier)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

SYNOPSIS
=========

Sommelier is an installer program that downloads and installs packages/programs that run under an emulator. As the name implies, the current target emulator is wine, though sommelier can also install some dosbox applications, a few linux native applications since version 4.7, and since version 4.0 some doom wads, scummvm games and gog.com games. Sommelier downloads an application and any dependancies (MFC, VB6) that it may have. Downloads are checked against an expected sha256 sum. Each application is installed in it's own directory-structure (AKA 'wine-bottle') under ~/.sommelier.  Sommelier creates a .desktop file for each application in ~/.local/share/applications. If needed, registry entries are changed within the wine-bottle (e.g. some applications may need to be run in virtual-desktop mode, or may need to disallow window managing by the system window-manager. The wine-bottle approach allows registry changes to be made independantly for each application). The .desktop file is used to run the application by the 'sommelier run' command.

Sommelier can install some gog.com games, that either run under wine, dosbox, native linux or scummvm. If a game is in the list it means I've had it install and run successfully. For some reason I find fewer native games work for me than emulated games under wine. Unfortuantely you currently have to download all the files for a gog game with your browser and then install them with './sommelier install <game name> -url <path to installer>'.


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

various settings for an installed application can be changed using:

`sommelier set <setting> <application>`

and an installed application can be reconfigured using:

`sommelier reconfigure <application>`


`sommelier uninstall <application>`

to uninstall.


DETAILED USAGE
==============

```
sommelier list [options]                           print list of apps available for install
sommelier install <name> [<name>] [options]        install an application by name
sommelier uninstall <name> [<name>]                uninstall an application by name
sommelier reconfigure <name> [<name>]              reconfigure an application by name
sommelier download <name> [<name>]                 download installers/packages to the current directory
sommelier run <name> [<options>]                   run an application by name
sommelier set <setting string> <name> [<name>]     change settings of installed applications listed by name

options are:
  -d                            print debugging (there will be a lot!)
  -c <config file>              specify a config (list of apps) file, rather than using the default
  -url                          supply an alternative url for an install (this can be an http, https, or ssh url, or just a file path
  -f                            force install even if expected sha256 doesn't match the download
  -force                        force install even if expected sha256 doesn't match the download
  -proxy <url>                  use a proxy for downloading installs
  -platform <platform>          specify platform to display when listing apps
  -k                            keep installer or .zip file instead of deleting it after install
  -icache <dir>                 installer cache: download installer to directory'dir' and leave it there
  -hash                         hash downloads even if they have no expected hash value
  -s                            set a value at install
  -set                          set a value at install

```

The 'set' command can change various settings of an installed application on a setting-by-setting bases (see 'SETTINGS' below). Settings are also used in the config files for installing apps. The 'reconfigure' command sets all the settings from the configuration file, and rebuilds the .desktop file that specifies how to run an application, allowing the whole setup of the installed application to be changed.

The 'run' command can take arguments that are passed to the program. For instance, to run Telegram desktop in 'start in systray' mode use:

```
	sommelier run Telegram -startintray
```


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


ENVIRONMENT VARIABLES
=====================

Sommelier looks for the variables `SOMMELIER_CA_BUNDLE`, `CURL_CA_BUNDLE` and `SSL_VERIFY_FILE`, in that order, to discover the path of the Certificate Bundle for certificate verification.
If `SOMMELIER_INSTALLER_CACHE` is set, sommelier will download installer and .zip files to the specified directory, and leave them there for future use with the `-url` option.


TO DO
=====

* Application Sandboxing
* Commands to add to application list
* Set default audio device with 'sommelier set'

