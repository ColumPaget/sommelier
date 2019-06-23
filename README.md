SYNOPSIS
=========

Sommelier is an installer program that downloads and installs packages/programs that run under an emulator. As the name implies, the current target emulator is wine, though sommelier can also install some dosbox applications, and since version 4.0 some doom wads, scummvm games and gog.com applications. Sommelier downloads an application and any dependancies (MFC, VB6) that it may have. Downloads are checked against an expected sha256 sum. Each application is installed in it's own directory-structure (AKA 'wine-bottle') under ~/.sommelier.  Sommelier creates a .desktop file for each application in ~/.local/share/applications. If needed, registry entries are changed within the wine-bottle (e.g. some applications may need to be run in virtual-desktop mode, or may need to disallow window managing by the system window-manager. The wine-bottle approach allows registry changes to be made independantly for each application). The .desktop file is used to run the application by the 'sommelier run' command.

Sommelier can install some gog.com games, that either run under wine, native linux or scummvm. If a game is in the list it means I've had it install and run successfully. For some reason I find fewer native games work for me than emulated games under wine. Unfortuantely you currently have to download all the files for a gog game with your browser and then install them with './sommelier install <game name> -url <path to installer>'.


LICENSE
=======

Sommelier is released under the GPLv3 license.


AUTHOR
======

Sommelier is written by Colum Paget. All patches/bugreports/requests should be sent to colums.projects@gmail.com, or handled through the project's github page.


INSTALL
=======

```
./configure
make
```

You should then place the program `sommelier` somewhere in your PATH. You can set an install prefix using './configure --prefix' and run 'make install'. 

Applications are configured in the various '.apps' files (wine.apps, msdos.apps, scummvm.apps, gog.com.apps) that come with the source code. These should be copied to a '.sommelier' sub-directory of your home directory. This '~/.sommelier' directory is also where the applications are stored, with as subdirectory for each one.



USAGE
=====

Normally you will just want to do:

`sommelier list` 

to see a list of applications, and then:

`sommelier install <application>`

to install one. Then:

`sommelier run <application>`

will run it. Finally

`sommelier uninstall <application>`

to uninstall.


DETAILED USAGE
==============

```
sommelier list [options]                           print list of apps available for install
sommelier install <name> [<name>] [options]        install an application by name
sommelier uninstall <name> [<name>]                uninstall an application by name
sommelier run <name>                               run an application by name
sommelier set <setting string> <name> [<name>]     change settings of an installed application

options are:
  -d                            print debugging (there will be a lot!)
  -c <config file>              specify a config (list of apps) file, rather than using the default
  -url                          supply an alternative url for an install (this can be an http, https, or ssh url, or just a file path
  -f                            force install even if expected sha256 doesn't match the download
  -force                        force install even if expected sha256 doesn't match the download
  -proxy <url>                  use a proxy for downloading installs

Proxy urls have the form: 
     <protocol>:<user>:<password>@<host>:<protocol>. 
'protocol' can be 'socks4', 'socks5' 'https' or 'sshtunnel'. For 'sshtunnel' the names defined in the ~/.ssh/config file can be used, so that  most of the information can be ommited.

examples:
   https:bill:secret@proxy.com
   socks4:proxy.com:1080
   socks5:bill:secret@proxy.com:1080
   sshtunnel:bill:secret@ssh_host.com
   sshtunnel:sshproxy

There are currently only three settings that can be configured with the 'set' command. All of them take 'y' or 'n' for 'yes' or 'no':
vdesk=y/n              run program within a virtual desktop
winmanage=y/n          allow window manager to decorate and manage windows of this program
smoothfonts=y/n        use font anti-aliasing
```

TO DO
=====

* Application Sandboxing
* Commands to add to application list
* Set default audio device with 'sommelier set'

