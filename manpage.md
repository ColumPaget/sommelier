title: sommelier
mansection: 1
date: 2020/05/17


NAME
====

sommelier - installer for apps that run under wine or other emulators


SYNOPSIS
========

```
sommelier addstore <url>                           add a remote store of apps at an ssh: or http: url
sommelier refresh                                  re-download files registered with 'addstore'
sommelier platforms                                print list of supported platforms
sommelier categories                               print list of application categories
sommelier list [options]                           print list of apps available for install. use -platform option to display apps for a given platform, -install to show those installed.
sommelier install <name> [<name>] [options]        install an application by name
sommelier uninstall <name> [<name>]                uninstall an application by name
sommelier reconfig <name> [<name>]                 reconfigure an installed application (seek for executables, re-write desktop file)
sommelier reconfigure <name> [<name>]              reconfigure an installed application (seek for executables, re-write desktop file)
sommelier run <name>                               run an application by name
sommelier winecfg <name>                           run 'winecfg' for named wine application
sommelier regedit <name>                           run 'regedit' for named wine application
sommelier download <name>                          just download installer/package to current directory
sommelier set <setting string> <name> [<name>]     change settings of an installed application
sommelier autostart                                run programs from ~/.config/autostart
```



    

DESCRIPTION
===========

Sommelier is an installer for downloadable applications that can be run under a number of emulators. The main focus is windows applications that can be run under the 'wine' emulator, but currently it also supports ms-dos applications that can be run under 'dosbox'; adventure games that can be run under the'scummvm' emulator; zx-spectrum apps that can be run under the 'fuse', 'fbzx' and 'zesarux' emulators; downloads from gog.com that can be run either under wine, dosbox, a neogeo emulator, or natively on linux; and doom wads that can be run under 'chocolate-doom' or 'crispy-doom'. A few other emulators and program types are supported, but less focused on.

Installed applications are stored under a '.sommelier' directory in the user's home directory, each in a subdirectory of their own (what's known as a 'wine bottle' for wine apps). Also in this directory are stored '.apps' configuration files that describe the list of installable apps.


OPTIONS
=======

-d
 : print debugging (there will be a lot!)

-debug
 : print debugging (there will be a lot!)

-verbose
 : print EVEN MORE debugging (there will be a lot!)

-c <config file>
 : specify a config (list of apps) file, rather than using the default search path

-url
 : supply an alternative url for an install (this can be an http, https, or ssh url, or just a file path

-f
 : force install even if expected sha256 doesn't match the download

-force
 : force install even if expected sha256 doesn't match the download

-n <name>
 : Name that program will be installed under and called/run under

-install-name <name>
 : Name that program will be installed under and called/run under

-install-as <name>
 : Name that program will be installed under and called/run under

-proxy <url>
 : use a proxy for downloading installs

-platform <platform>
 : specify platform to display when listing apps, or if installing an app that has multiple platforms

-emu <emulator>
 : specify a specific emulator to use when installing an app

-emulator <emulator>
 : specify a specific emulator to use when installing an app

-category <category>
 : category to use when displaying lists of apps

-installed
 : display only installed apps when displaying lists of apps, or only platforms with emulators installed when displaying platform list

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

-su
 : allow programs to 'su' to root (default on linux is to prevent this using the 'NO_NEW_PRIVS' prctl, if supported). If this is used with 'install' then the program will be installed with 'allow SU' set. THIS ALSO DISABLES ALL 'SECCOMP' SECURITY

-nosu
 : prevent programs doing 'su' to root, even if they are configured with such support in config files. If this is used with 'install' then program will be installed with SU blocked.

-pid
 : use linux namespaces to deny access to other processes (e.g. via 'kill' to send signals)

-nopid
 : use linux namespaces to deny access to other processes (e.g. via 'kill' to send signals)

+pid
 : allow  access to other processes (e.g. via 'kill' to send signals)

-net
 : use linux namespaces to deny network access. If this is used with 'install' then the program will be installed with 'deny network'.

+net
 : allow network access. If this is used with 'install' then the program will be installed with 'allow network'.

-nonet
 : use linux namespaces to deny network access. If this is used with 'install' then the program will be installed with 'deny network'.

-client
 : use linux seccomp to prevent app running a TCP server.

-secure <security string>
 : Set libUseful 'ProcessApplyConfig' security level.

-security <security string>
 : Set libUseful 'ProcessApplyConfig' security level.




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

Install app for a particular platform and using a particular emulator when it's available in more than one
 : sommelier install BlazingStar -platform gog:neogeo -emu gngeo -url gog_blazing_star_2.0.0.1.sh

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

The following platforms are supported for use in the '-platform' option, or in application configurations in the config files. A platform value not only relates to the emulator that an app will be run under but also to some configuration that might be set for an emulator and also to how a program should be unpacked and installed.

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
 : neogeo game from gog.com that's packaged for linux

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

The '-proxy' option is used with the 'download' or 'install' commands and accepts proxy urls of the form:

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

There are a number of settings that can be lastingly configured with the 'set' command or using the '-set' option when installing an application. Currently these settings only relate to programs run under wine, or doom wads run under chocolate-doom or crispy-doom.

allow-su=y/n
 : allow/deny application to raise priviledges to root user

allow-net=y/n
 : allow/deny application to access network (uses namespaces to deny)

allow-pid=y/n
 : allow/deny application to send signals or otherwise access other processes (uses namespaces to deny)

vdesk=y/n/<resolution>
 : run program within a window/virtual desktop, if a resolution like '1280x1024' is supplied, then run a virtual desktop in a window of that size.

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


For both DOOM and Wine you can set the size of the window using the vdesk setting, in the style 'vdesk=600x300'. 'fullscreen' also works for doom and msdos apps.


ENVIRONMENT VARIABLES
=====================

Sommelier looks for the variables 'SOMMELIER_CA_BUNDLE', 'CURL_CA_BUNDLE' and 'SSL_VERIFY_FILE', in that order, to discover the path of the Certificate Bundle for certificate verification.
If 'SOMMELIER_INSTALLER_CACHE' is set, sommelier will download installer and .zip files to the specified directory, and leave them there for future use with the '-url' option.



APPS FILES
==========

Installable apps are specified using 'apps files'. These are files that contain one-line configurations for each app that can be installed. Each line starts with an application name, followed by a list of name-value pairs that specify configuration variables. The only required variable is 'platform' which specifies the platform for the app, e.g. "linux32", "winxp", "zxspectrum", etc. 'url' is also usually provided, which specifies the url that the app package can be downloaded from, however this can be ommitted if the url is provided at install time using the `-url` command-line option (as in gog.com apps).

There are some special types of line that can be put in the file. Instead of starting with an application name, they start with a single symbol. These symbols are:

```
#    a comment line
!    a line of variables that apply to everything below it in the file
*    a line of variables that apply to everything below it in the file UNTIL another '*' line is encountered
```

For example, here is a file the configures a few 3ds homebrew games. Note how 'platform' and 'category' are set using a '\*' line to prevent typing them over and over for each app.



```
* platform=3ds category=Game;3DS

Breakout url=https://github.com/Magicrafter13/Breakout/releases/download/B_1.07.02/Breakout.3dsx install-type='executable' sha256='84c052828e6eac1d78831df9f4cac9eb8d5e580c766e567ed3e029ee75b77ac6' category=BreakoutGame;3DS
ColossalCaveAdventure url=https://github.com/nop90/Colossal-Cave-Adventure-2.5-for-3ds/releases/download/v1.1/Advent_3DSX_v1.1.zip sha256='57de0c3df074b0afdb1da75f43bc85d99fc07a65b13fea809a4a4bf3f3d13a6c' category=DungeonCrawl;3DS
GriffonLegend url=https://github.com/nop90/Griffon-Legend-3DS/releases/download/v1.0/GriffonLegend.v1.0.zip sha256='be29b374dcd722715fe24bae835174abb446e136ac306a5507dda92b5661c24f'
```


Custom variables can be used in these configs, and then called up using '$(variable_name)' in other variables. However, the following variables are the ones that mean something to sommelier.


platform
: the platform/emulator-type this app will run on

url
: the url this app can be installed from

category
: semi-colon separated list of categories the app belongs to, like 'Chat' or 'Game' etc. The standard categories for desktop files are supported, but custom ones can also be used

extra_category
: if category has been specified using a '\*' line, then add the category/categories specified here to it

sha256
: the sha256 sum of the package file that is downloaded, sommelier will throw an error if this specified, but doesn't match

requires
: name of another app that must be installed before this one, usually a library

bundled
: name of an app that this app is bundled with, install the 'parent app' but run the executable specified here

installer
: specify the 'installer' file for packages that, when unpacked, contain an installer app that must be run.

installer-args
: command-line args to be passed to an installer

installer-vdesk
: for some windows applications run under 'wine' the installer needs to be run in a 'vdesk' (virtual desktop), in which case specify 'installer-vdesk=y'

install-stage2
: some installers unpack and then a second installer needs to be run, this specifies the file to be run for that stage2 installer

stage2-args
: arguments to pass to stage2 installer

inner-package
: some packages have a second package inside them, specify the filename of such a package with this variable

exec
: the executable file that is to be found when the package has been unpacked. Sommelier has some ability to auto-find such files, but it's better to specify them. 

exec64
: this allows one to specify a 64-bit executable, that will be searched for first on 64-bit platforms. This can be used in combination with the 'exec' variable to specify both 32-bit and 64-bit executables within the same package.

exec-args
: any arguments to be passed to the executable when it's run

emulator-args
: any arguments to be passed to an emulator when it runs this app

install-type
: the installation type. 'executable' for apps that are just downloaded and run as executables, otherwise a package-type will be expected.

download-type
: package type of the download. This can be 'zip', 'deb', '7za', 'tar.xz', 'tar.bz2', 'tar.gz'. This tells sommelier it needs to unpack the file using the appropriate type of unpacker.

working-dir
: usually the main directory that contains the application is decided by the platform, but sometimes one has to specify it using this argument

link
: during install create a symlink to a file/directory. Value has the format "<link path>:<target path>"

rename
: during install rename a file/directory. Argument has the format "<src>:<dest>"

delete
: during install delete files matching this pattern. e.g. to delete video files that don't play under wine.

copyfiles-to
: during install copy files from one directory to another. Value has the format "<src dir>:<dest dir>"

movefiles-from
: Argument is a match pattern that specifies files to be moved into 'working directory' for the application. Working directory is normally specific to a platform

chext
: during install change file-type extension of specified files. Value has the format "<match pattern>:<new extension>". Example: chext="video/m4v:mp4"

zip
: during install zip files up into a zipfile. Used for emulators that expect to 'run' a zipfile. Value is "<zipfile name>:<file match pattern>"

icon
: icon file for the app. This will be specified in the generated .desktop files for this application.

warn
: a warning that is displayed when the app is installed or run

runwarn
: a warning that is displayed when the app is run

winmanager
: for wine applications. Whether the X11 window manager should 'manage' this window. On rare occasions the window manager can get confused by the behavior of a wine app, so set this to 'n' to deal with that.

vdesk
: run a windows app in a virtual desktop window. Value can be 'y', 'n' or a desktop size like '800x600'.

os-version
: for windows/wine apps, specify the os-version to run the app as. Wine supports versions like "win3.1", "winxp", "vista", "win8" etc. Consule wine documentation for a full list.

comment
: specify a comment that will be displayed against an app in the 'sommelier list' command

donate
: url to a webpage where one can donate to an app's project

secure
: libUseful 'ProcessApplyConfig' security string, overriding default app security.





REMOTE STORES
=============

sommelier supports 'remote stores', directories containing app list files that can be downloaded from an http, https or ssh server. These are registered using:

```
	sommelier addstore <url>
```

where the url points to an apps list file. App packages can be stored in the same directory as the apps list file, and their urls can be specified using a variable called 'server_path' which specifies the directory part of the URL the apps list file was installed from. 

Installed apps files can be re-downloaded and updated using

```
	sommelier refresh
```




