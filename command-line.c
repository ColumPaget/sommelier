#include "command-line.h"
#include "apps.h"
#include "config.h"
#include <stdlib.h>

static void PrintUsage()
{
    printf("\n");
    printf("sommelier platforms                                print list of supported platforms\n");
    printf("sommelier categories                               print list of application categories\n");
    printf("sommelier list [options]                           print list of apps available for install. use -platform option to display apps for a given platform\n");
    printf("sommelier install <name> [<name>] [options]        install an application by name\n");
    printf("sommelier uninstall <name> [<name>]                uninstall an application by name\n");
    printf("sommelier reconfig <name> [<name>]                 reconfigure an installed application (seek for executables, re-write desktop file)\n");
    printf("sommelier reconfigure <name> [<name>]              reconfigure an installed application (seek for executables, re-write desktop file)\n");
    printf("sommelier run <name>                               run an application by name\n");
    printf("sommelier winecfg <name>                           run 'winecfg' for named wine application\n");
    printf("sommelier regedit <name>                           run 'regedit' for named wine application\n");
    printf("sommelier download <name>                          just download installer/package to current directory\n");
    printf("sommelier set <setting string> <name> [<name>]     change settings of an installed application\n");
    printf("sommelier autostart                                run programs from ~/.config/autostart\n");
    printf("\n");
    printf("options are:\n");
    printf("  -d                            print debugging (there will be a lot!)\n");
    printf("  -debug                        print debugging (there will be a lot!)\n");
    printf("  -c <config file>              specify a config (list of apps) file, rather than using the default\n");
    printf("  -url                          supply an alternative url for an install (this can be an http, https, or ssh url, or just a file path. File paths must be absolute, not relative)\n");
    printf("  -install-name <name>          Name that program will be installed under and called/run under\n");
    printf("  -install-as <name>            Name that program will be installed under and called/run under\n");
    printf("  -f                            force install even if expected sha256 doesn't match the download\n");
    printf("  -force                        force install even if expected sha256 doesn't match the download\n");
    printf("  -proxy <url>                  use a proxy for downloading installs\n");
    printf("  -platform <platform>          platform to use when installing or displaying lists of apps\n");
    printf("  -category <category>          category to use when displaying lists of apps\n");
    printf("  -installed                    display only installed apps when displaying lists of apps\n");
    printf("  -k                            keep installer or .zip file instead of deleting it after install\n");
    printf("  -S                            install app system-wide under /opt, to be run as a normal native app\n");
    printf("  -system                       install app system-wide under /opt, to be run as a normal native app\n");
    printf("  -n <name>                     specify name to install the app under, allows installing multiple instances of the same app\n");
    printf("  -install-name <name>          specify name to install the app under, allows installing multiple instances of the same app\n");
    printf("  -install-as <name>            specify name to install the app under, allows installing multiple instances of the same app\n");
    printf("  -emu <emulator>               specify a specific emulator to use when installing an app\n");
    printf("  -emulator <emulator>          specify a specific emulator to use when installing an app\n");
    printf("  -icache <dir>                 installer cache: download installer to directory'dir' and leave it there\n");
    printf("  -hash                         hash downloads even if they have no expected hash value\n");
    printf("  -no-xrandr                    don't use xrandr to reset screen resolution after running and application\n");
    printf("  -user-agent <agent string>    set user-agent to send when communicating over http\n");
    printf("  -ua <agent string>            set user-agent to send when communicating over http\n");
    printf("  -su                           allow programs to 'su' to root. On linux sommelier sets 'NO_NEW_PRIVS' by default to prevent su/sudo etc to root.\n");
		printf("                                If used with action 'run' then the program runs with the ability to su.\n");
		printf("                                If used with action 'install' then the program is installed with the ability to su.\n");
    printf("  -nosu                         deny programs to 'su' to root. On linux sommelier sets 'NO_NEW_PRIVS' by default to prevent su/sudo etc to root.\n");
		printf("                                If used with action 'run' then the program runs WITHOUT the ability to su.\n");
		printf("                                If used with action 'install' then the program is installed WITHOUT the ability to su.\n");
		printf("                                If used with action 'install' then the program is installed WITHOUT the ability to su.\n");
		printf("  -end                          End of sommelier arguments, anything past this point is arguments for the program to run\n");
		printf("  --                            End of sommelier arguments, anything past this point is arguments for the program to run\n");
    printf("\n");
    printf("Proxy urls have the form: \n");
    printf("     <protocol>:<user>:<password>@<host>:<protocol>. \n");
    printf("'protocol' can be 'socks4', 'socks5' 'https' or 'sshtunnel'. For 'sshtunnel' the names defined in the ~/.ssh/config file can be used, so that  most of the information can be ommited.\n");
    printf("examples:\n");
    printf("   https:bill:secret@proxy.com\n");
    printf("   socks4:proxy.com:1080\n");
    printf("   socks5:bill:secret@proxy.com:1080\n");
    printf("   sshtunnel:bill:secret@ssh_host.com\n");
    printf("   sshtunnel:sshproxy\n");
    printf("\n");
    printf("There are currently only settings for 'wine' that can be configured with the 'set' command:\n");
    printf("vdesk=y/n              run program within a virtual desktop, or not\n");
    printf("vdesk=<resolution>     run program within a virtual desktop with supplied resolution\n");
    printf("vdesk=<resolution>     run program within a virtual desktop with supplied resolution\n");
    printf("winmanage=y/n          allow window manager to decorate and manage windows of this program, or not\n");
    printf("smoothfonts=y/n        use font anti-aliasing, or not\n");
    printf("os-version=<os>        set version of windows to emulate. Choices are: win10, win81, win8, win7, win2008, vista, win2003, winxp, win2k, nt40,  winme, win98, win95, win31\n");
    printf("sound=y/n/sfx           DOOM only: sound on/off, or only effects (no music)\n");
    printf("mouse=y/n               DOOM only: use mouse in-game, or not\n");
    printf("grab=y/n                DOOM only: grab mouse, or not\n");
    printf("\n");
    printf("Environment Variables\n");
    printf("Sommelier looks for the variables SOMMELIER_CA_BUNDLE, CURL_CA_BUNDLE and SSL_VERIFY_FILE, in that order, to discover the path of the Certificate Bundle for certificate verification.\n");
    printf("If SOMMELIER_INSTALLER_CACHE is set, sommelier will download installer and .zip files to the specified directory, and leave them there for future use with the -url option\n");
}


void PrintVersion()
{
    printf("sommelier version %s\n", VERSION);
    printf("compiled: %s %s\n", __DATE__, __TIME__);
    printf("default platform: %s\n", PlatformDefault());
}


static int ParseCommandLineOption(TAction *Act, CMDLINE *CmdLine)
{
    const char *p_Opt;

		//if we don't have an Action yet, we may have one once we've
		//parsed more of the command-line, so return TRUE to keep going
    if (! Act) return(TRUE);

    if (! CmdLine) return(FALSE);

    p_Opt=CommandLineCurr(CmdLine);

		if (strcmp(p_Opt, "--")==0) return(FALSE);
		else if (strcmp(p_Opt, "-end")==0) return(FALSE);
		else if (strcmp(p_Opt, "-c")==0) Config->AppConfigPath=CopyStr(Config->AppConfigPath, CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-su")==0) Config->Flags |= FLAG_ALLOW_SU;
    else if (strcmp(p_Opt, "-nosu")==0) Config->Flags |= FLAG_DENY_SU;
    else if (strcmp(p_Opt, "-S")==0) Config->Flags |=  FLAG_SYSTEM_INSTALL;
    else if (strcmp(p_Opt, "-system")==0) Config->Flags |=  FLAG_SYSTEM_INSTALL;
    else if (strcmp(p_Opt, "-no-xrandr")==0) Config->Flags |= FLAG_NO_XRANDR;
    else if (strcmp(p_Opt, "-f")==0) Act->Flags |=  FLAG_FORCE;
    else if (strcmp(p_Opt, "-force")==0) Act->Flags |=  FLAG_FORCE;
    else if (strcmp(p_Opt, "-s")==0) LoadAppConfigToAct(Act, CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-set")==0) LoadAppConfigToAct(Act, CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-k")==0) Act->Flags |=  FLAG_KEEP_INSTALLER;
    else if (strcmp(p_Opt, "-n")==0) Act->InstallName=CopyStr(Act->InstallName, CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-hash")==0) Act->Flags |= FLAG_HASH_DOWNLOAD;
    else if (strcmp(p_Opt, "-sandbox")==0) Act->Flags |= FLAG_SANDBOX;
    else if (strcmp(p_Opt, "+net")==0) Act->Flags |= FLAG_NET;
    else if (strcmp(p_Opt, "-net")==0) Act->Flags &= ~FLAG_NET;
    else if (strcmp(p_Opt, "-url")==0) Act->URL=realpath(CommandLineNext(CmdLine), NULL);
    else if (strcmp(p_Opt, "-install-name")==0) Act->InstallName=CopyStr(Act->InstallName, CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-install-as")==0) Act->InstallName=CopyStr(Act->InstallName, CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-emu")==0) SetVar(Act->Vars, "required_emulator", CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-emulator")==0) SetVar(Act->Vars, "required_emulator", CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-installed")==0) Act->Flags |= FLAG_INSTALLED;
    else if (strcmp(p_Opt, "-proxy")==0) SetGlobalConnectionChain(CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-user-agent")==0) LibUsefulSetValue("HTTP:UserAgent",CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-ua")==0) LibUsefulSetValue("HTTP:UserAgent",CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-category")==0) SetVar(Act->Vars, "category", CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-secure")==0) SetVar(Act->Vars, "security_level", CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-security")==0) SetVar(Act->Vars, "security_level", CommandLineNext(CmdLine));
    //we cannot unalias the platform here because here, because we won't have loaded platforms yet
    else if (strcmp(p_Opt, "-platform")==0) Act->Platform=CopyStr(Act->Platform, CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-icache")==0)
    {
        Config->InstallerCache=CopyStr(Config->InstallerCache, CommandLineNext(CmdLine));
        Act->Flags |= FLAG_KEEP_INSTALLER;
    }
    else if (
        (strcmp(p_Opt, "-d")==0) ||
        (strcmp(p_Opt, "-debug")==0)
    )
    {
        LibUsefulSetValue("HTTP:Debug","Y");
        Config->Flags |= FLAG_DEBUG;
    }
    else Act->Args=MCatStr(Act->Args, " '",p_Opt,"'",NULL);


		return(TRUE);
}


//this is a 'first run' command-line parser. If we encounter '--' or -end'
//we just end parsing options. We will later call 'ParseSimpleAction' if we
//are doing a 'run' command, and in that case we will handle '--' and '-end'
//more completely
static TAction *CommandLineParseOptions(CMDLINE *CmdLine)
{
    TAction *Options=NULL;
    const char *arg;

    Options=ActionCreate(ACT_INSTALL, "");
    arg=CommandLineNext(CmdLine);
    while (arg)
    {
        if (*arg=='-') 
				{
				if (!	ParseCommandLineOption(Options, CmdLine)) break;
				}
        arg=CommandLineNext(CmdLine);
    }
    return(Options);
}



TAction *ParseSimpleAction(ListNode *Acts, int Type, CMDLINE *CmdLine)
{
    TAction *Act=NULL, *ListAct=NULL;
    const char *arg;

    //list is one of the few actions that doesn't need an argument
    //so if we were asked to list, but there are no arguments, just add
    //a blank 'list all'.
    if ((Type==ACT_LIST) || (Type==ACT_LIST_PLATFORMS) || (Type==ACT_LIST_CATEGORIES))
    {
        Act=ActionCreate(Type, "");
        ListAddItem(Acts, Act);
    }

    arg=CommandLineNext(CmdLine);
    while (arg)
    {
        if (*arg == '-')
        {
						 if	(! ParseCommandLineOption(Act, CmdLine)) break;
        }
        else
        {
            if (Type==ACT_LIST) Act->Name=CopyStr(Act->Name, arg);
            else if ((Type==ACT_RUN) && Act) Act->Args=MCatStr(Act->Args, " ", arg, NULL);
            else
            {
                Act=ActionCreate(Type, arg);
                ListAddItem(Acts, Act);
            }
        }
        arg=CommandLineNext(CmdLine);
    }


		//if we broke out because we hit '--' or '-end' then add any
		//remaining args to the 'Act->Args' value
    arg=CommandLineNext(CmdLine);
		while (arg)
		{
      if (Act) Act->Args=MCatStr(Act->Args, " ", arg, NULL);
    	arg=CommandLineNext(CmdLine);
		}


    return(Act);
}



TAction *ParseStandardCommand(CMDLINE *CmdLine, const char *Cmd, int ActID, ListNode *Acts, TAction *Options)
{
    const char *arg;
    TAction *Act=NULL;

    arg=CommandLineNext(CmdLine);
    if (! arg) printf("Error: No applications given as targets for '%s' command\n", Cmd);
    while (arg)
    {
        if (*arg == '-') ParseCommandLineOption(Options, CmdLine);
        else
        {
            Act=ActionCreate(ActID, arg);
            if (Act) ListAddItem(Acts, Act);
        }
        arg=CommandLineNext(CmdLine);
    }

    return(Act);
}


ListNode *ParseCommandLine(int argc, char *argv[])
{
    CMDLINE *CmdLine;
    ListNode *Acts;
    TAction *Act=NULL, *Options=NULL;
    char *StdDepsPath=NULL, *SettingsStr=NULL;
    int Flags;
    const char *arg;
    ListNode *Curr;

    Acts=ListCreate();
    CmdLine=CommandLineParserCreate(argc, argv);

//do an options run so that options like -c can be used anywhere on command-line
//this makes things very messy, 'cos we have to reparse them when examining the
//command-line proper
    Options=CommandLineParseOptions(CmdLine);
    arg=CommandLineFirst(CmdLine);
    if (arg)
    {
        if (strcmp(arg, "install")==0) Act=ParseStandardCommand(CmdLine, "install", ACT_INSTALL, Acts, Options);
        else if (strncmp(arg, "reconf", 6)==0) Act=ParseStandardCommand(CmdLine, "reconfigure", ACT_RECONFIGURE, Acts, Options);
        else if (strcmp(arg, "uninstall")==0) Act=ParseStandardCommand(CmdLine, "uninstall", ACT_UNINSTALL, Acts, Options);
        else if (strcmp(arg, "download")==0) Act=ParseStandardCommand(CmdLine, "download", ACT_DOWNLOAD, Acts, Options);
        else if (strcmp(arg, "run")==0) ParseSimpleAction(Acts, ACT_RUN, CmdLine);
        else if (strcmp(arg, "list")==0) ParseSimpleAction(Acts, ACT_LIST, CmdLine);
        else if (strcmp(arg, "platforms")==0) ParseSimpleAction(Acts, ACT_LIST_PLATFORMS, CmdLine);
        else if (strcmp(arg, "categories")==0) ParseSimpleAction(Acts, ACT_LIST_CATEGORIES, CmdLine);
        else if (strcmp(arg, "rebuild")==0) ParseSimpleAction(Acts, ACT_REBUILD, CmdLine);
        else if (strcmp(arg, "hashes")==0) ParseSimpleAction(Acts, ACT_REBUILD_HASHES, CmdLine);
        else if (strcmp(arg, "winecfg")==0) ParseSimpleAction(Acts, ACT_WINECFG, CmdLine);
        else if (strcmp(arg, "regedit")==0) ParseSimpleAction(Acts, ACT_REGEDIT, CmdLine);
        else if (strcmp(arg, "fonts")==0) ParseSimpleAction(Acts, ACT_FONTS, CmdLine);
        else if (strcmp(arg, "version")==0) PrintVersion();
        else if (strcmp(arg, "-version")==0) PrintVersion();
        else if (strcmp(arg, "--version")==0) PrintVersion();
        else if (strcmp(arg, "autostart")==0)
        {
            Act=ActionCreate(ACT_AUTOSTART, "");
            ListAddItem(Acts, Act);
        }
        else if (strcmp(arg, "set")==0)
        {
            SettingsStr=CopyStr(SettingsStr, CommandLineNext(CmdLine));
            arg=CommandLineNext(CmdLine);
            if (! arg) printf("Error: No applications given as targets for 'set' command\n");
            while (arg)
            {
                Act=ActionCreate(ACT_SET, arg);
                if (Act)
                {
                    LoadAppConfigToAct(Act, SettingsStr);
                    ListAddItem(Acts, Act);
                }
                arg=CommandLineNext(CmdLine);
            }
        }
				//if no recognized action, print help
        else PrintUsage();
    }
		//if no arguments, print help
    else PrintUsage();


    //some options can be supplied out-of-order so we preload them into 'Options' and
    //retrospectively add them to all apps
    Curr=ListGetNext(Acts);
    while (Curr)
    {
        Act=(TAction *) Curr->Item;
        Act->Flags |= Options->Flags;
        if (StrValid(Options->URL)) Act->URL=CopyStr(Act->URL, Options->URL);
        if (StrValid(Options->Platform)) Act->Platform=CopyStr(Act->Platform, Options->Platform);
        if (StrValid(Options->InstallName)) Act->InstallName=CopyStr(Act->InstallName, Options->InstallName);
        CopyVars(Act->Vars, Options->Vars);

        Curr=ListGetNext(Curr);
    }

    ActionDestroy(Options);
    DestroyString(SettingsStr);
    DestroyString(StdDepsPath);

    return(Acts);
}

