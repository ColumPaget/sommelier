#include "command-line.h"
#include "apps.h"
#include "config.h"

static void PrintUsage()
{
    printf("\n");
    printf("sommelier list [options]                           print list of apps available for install. use -platform option to display apps for a given platform\n");
    printf("sommelier install <name> [<name>] [options]        install an application by name\n");
    printf("sommelier uninstall <name> [<name>]                uninstall an application by name\n");
    printf("sommelier reconfig <name> [<name>]                 reconfigure an installed application (seek for executables, re-write desktop file)\n");
    printf("sommelier reconfigure <name> [<name>]                 reconfigure an installed application (seek for executables, re-write desktop file)\n");
    printf("sommelier run <name>                               run an application by name\n");
    printf("sommelier download <name>                          just download installer/package to current directory\n");
    printf("sommelier set <setting string> <name> [<name>]     change settings of an installed application\n");
    printf("\n");
    printf("options are:\n");
    printf("  -d                            print debugging (there will be a lot!)\n");
    printf("  -c <config file>              specify a config (list of apps) file, rather than using the default\n");
    printf("  -url                          supply an alternative url for an install (this can be an http, https, or ssh url, or just a file path. File paths must be absolute, not relative)\n");
    printf("  -install-name <name>          Name that program will be installed under and called/run under\n");
    printf("  -f                            force install even if expected sha256 doesn't match the download\n");
    printf("  -force                        force install even if expected sha256 doesn't match the download\n");
    printf("  -proxy <url>                  use a proxy for downloading installs\n");
    printf("  -platform <platform>          platform to use when displaying lists of apps\n");
    printf("  -k                            keep installer or .zip file instead of deleting it after install\n");
    printf("  -S                            install app system-wide under /opt, to be run as a normal native app\n");
    printf("  -system                       install app system-wide under /opt, to be run as a normal native app\n");
    printf("  -icache <dir>                 installer cache: download installer to directory'dir' and leave it there\n");
    printf("  -hash                         hash downloads even if they have no expected hash value\n");
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
    printf("winmanage=y/n          allow window manager to decorate and manage windows of this program, or not\n");
    printf("smoothfonts=y/n        use font anti-aliasing, or not\n");
    printf("os-version=<os>        set version of windows to emulate. Choices are: win10, win81, win8, win7, win2008, vista, win2003, winxp, win2k, nt40,  winme, win98, win95, win31\n");
    printf("\n");
    printf("Environment Variables\n");
    printf("Sommelier looks for the variables SOMMELIER_CA_BUNDLE, CURL_CA_BUNDLE and SSL_VERIFY_FILE, in that order, to discover the path of the Certificate Bundle for certificate verification.\n");
    printf("If SOMMELIER_INSTALLER_CACHE is set, sommelier will download installer and .zip files to the specified directory, and leave them there for future use with the -url option\n");
}


static void ParseCommandLineOption(TAction *Act, CMDLINE *CmdLine)
{
    const char *p_Opt;

    if (! Act) return;
    if (! CmdLine) return;

    p_Opt=CommandLineCurr(CmdLine);

    if (strcmp(p_Opt, "-c")==0) Config->AppConfigPath=CopyStr(Config->AppConfigPath, CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-f")==0) Act->Flags |=  FLAG_FORCE;
    else if (strcmp(p_Opt, "-force")==0) Act->Flags |=  FLAG_FORCE;
    else if (strcmp(p_Opt, "-s")==0) LoadAppConfigToAct(Act, CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-set")==0) LoadAppConfigToAct(Act, CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-k")==0) Act->Flags |=  FLAG_KEEP_INSTALLER;
    else if (strcmp(p_Opt, "-S")==0) Config->Flags |=  FLAG_SYSTEM_INSTALL;
    else if (strcmp(p_Opt, "-system")==0) Config->Flags |=  FLAG_SYSTEM_INSTALL;
    else if (strcmp(p_Opt, "-n")==0) Act->InstallName=CopyStr(Act->InstallName, CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-icache")==0)
    {
        Config->InstallerCache=CopyStr(Config->InstallerCache, CommandLineNext(CmdLine));
        Act->Flags |= FLAG_KEEP_INSTALLER;
    }
    else if (strcmp(p_Opt, "-hash")==0) Act->Flags |= FLAG_HASH_DOWNLOAD;
    else if (strcmp(p_Opt, "-sandbox")==0) Act->Flags |= FLAG_SANDBOX;
    else if (strcmp(p_Opt, "+net")==0) Act->Flags |= FLAG_NET;
    else if (strcmp(p_Opt, "-net")==0) Act->Flags &= ~FLAG_NET;
    else if (strcmp(p_Opt, "-url")==0) Act->URL=CopyStr(Act->URL, CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-install-name")==0) Act->InstallName=CopyStr(Act->InstallName, CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-install-as")==0) Act->InstallName=CopyStr(Act->InstallName, CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-platform")==0) Act->Platform=CopyStr(Act->Platform, CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-proxy")==0) SetGlobalConnectionChain(CommandLineNext(CmdLine));
    else if (strcmp(p_Opt, "-d")==0)
    {
        LibUsefulSetValue("HTTP:Debug","Y");
        Config->Flags |= FLAG_DEBUG;
    }
    else Act->Args=MCatStr(Act->Args, " '",p_Opt,"'",NULL);
}


static TAction *CommandLineParseOptions(CMDLINE *CmdLine)
{
    TAction *Options=NULL;
    const char *arg;

    Options=ActionCreate(ACT_INSTALL, "");
    arg=CommandLineNext(CmdLine);
    while (arg)
    {
        if (*arg=='-') ParseCommandLineOption(Options, CmdLine);
        arg=CommandLineNext(CmdLine);
    }
    return(Options);
}


TAction *ParseSimpleAction(ListNode *Acts, int Type, const char *Arg, CMDLINE *CmdLine)
{
    TAction *Act=NULL;
    const char *arg;

    Act=ActionCreate(Type, Arg);

    if (Act)
    {
        ListAddItem(Acts, Act);
        arg=CommandLineNext(CmdLine);
        while (arg)
        {
            ParseCommandLineOption(Act, CmdLine);
            arg=CommandLineNext(CmdLine);
        }
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
//this makes things very messy, 'cos we have to reparse them when examinging the
//command-line proper
    Options=CommandLineParseOptions(CmdLine);
    arg=CommandLineFirst(CmdLine);
    if (arg)
    {
        if (strcmp(arg, "install")==0) Act=ParseStandardCommand(CmdLine, "install", ACT_INSTALL, Acts, Options);
        else if (strncmp(arg, "reconf", 6)==0) Act=ParseStandardCommand(CmdLine, "reconfigure", ACT_RECONFIGURE, Acts, Options);
        else if (strcmp(arg, "uninstall")==0) Act=ParseStandardCommand(CmdLine, "uninstall", ACT_UNINSTALL, Acts, Options);
        else if (strcmp(arg, "download")==0) Act=ParseStandardCommand(CmdLine, "download", ACT_DOWNLOAD, Acts, Options);
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
        else
        {
            if (strcmp(arg, "run")==0) ParseSimpleAction(Acts, ACT_RUN, CommandLineNext(CmdLine), CmdLine);
            else if (strcmp(arg, "list")==0) ParseSimpleAction(Acts, ACT_LIST, "", CmdLine);
            else if (strcmp(arg, "rebuild")==0) ParseSimpleAction(Acts, ACT_REBUILD, "", CmdLine);
            else if (strcmp(arg, "hashes")==0) ParseSimpleAction(Acts, ACT_REBUILD_HASHES, "", CmdLine);
            else if (strcmp(arg, "version")==0) printf("sommelier version %s\n", VERSION);
            else if (strcmp(arg, "-version")==0) printf("sommelier version %s\n", VERSION);
            else if (strcmp(arg, "--version")==0) printf("sommelier version %s\n", VERSION);
            else PrintUsage();
        }
    }
    else PrintUsage();

    Curr=ListGetNext(Acts);
    while (Curr)
    {
        Act=(TAction *) Curr->Item;
        Act->Flags |= Options->Flags;
        if (StrValid(Options->URL)) Act->URL=CopyStr(Act->URL, Options->URL);
        if (StrValid(Options->Platform)) Act->Platform=CopyStr(Act->Platform, Options->Platform);
        if (StrValid(Options->InstallName)) Act->InstallName=CopyStr(Act->InstallName, Options->InstallName);
        CopyVars(Act->Vars, Options->Vars);

        if (! StrValid(Act->Platform)) Act->Platform=CopyStr(Act->Platform, PlatformDefault());
        Curr=ListGetNext(Curr);
    }


    DestroyString(SettingsStr);
    DestroyString(StdDepsPath);

    return(Acts);
}

