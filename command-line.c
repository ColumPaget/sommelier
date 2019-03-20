#include "command-line.h"
#include "apps.h"

static void PrintUsage()
{
printf("\n");
printf("sommelier list [options]                           print list of apps available for install\n");
printf("sommelier install <name> [<name>] [options]        install an application by name\n");
printf("sommelier uninstall <name> [<name>]                uninstall an application by name\n");
printf("sommelier run <name>                               run an application by name\n");
printf("sommelier set <setting string> <name> [<name>]     change settings of an installed application\n");
printf("\n");
printf("options are:\n");
printf("  -d                            print debugging (there will be a lot!)\n");
printf("  -c <config file>              specify a config (list of apps) file, rather than using the default\n");
printf("  -url                          supply an alternative url for an install (this can be an http, https, or ssh url, or just a file path\n");
printf("  -f                            force install even if expected sha256 doesn't match the download\n");
printf("  -force                        force install even if expected sha256 doesn't match the download\n");
printf("  -proxy <url>                  use a proxy for downloading installs\n");
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
printf("There are currently only three settings that can be configured with the 'set' command. All of them take 'y' or 'n' for 'yes' or 'no':\n");
printf("vdesk=y/n              run program within a virtual desktop\n");
printf("winmanage=y/n          allow window manager to decorate and manage windows of this program\n");
printf("smoothfonts=y/n        use font anti-aliasing\n");
}


static void ParseCommandLineOption(TAction *Act, CMDLINE *CmdLine)
{
const char *p_Opt;

	if (! Act) return;
	if (! CmdLine) return;

	p_Opt=CommandLineCurr(CmdLine);

	if (strcmp(p_Opt, "-c")==0) Act->ConfigPath=CopyStr(Act->ConfigPath, CommandLineNext(CmdLine));
	else if (strcmp(p_Opt, "-f")==0) Act->Flags |=  FLAG_FORCE;
	else if (strcmp(p_Opt, "-force")==0) Act->Flags |=  FLAG_FORCE;
	else if (strcmp(p_Opt, "-sandbox")==0) Act->Flags |= FLAG_SANDBOX;
	else if (strcmp(p_Opt, "+net")==0) Act->Flags |= FLAG_NET;
	else if (strcmp(p_Opt, "-net")==0) Act->Flags &= ~FLAG_NET;
	else if (strcmp(p_Opt, "-url")==0) Act->URL=CopyStr(Act->URL, CommandLineNext(CmdLine));
	else if (strcmp(p_Opt, "-proxy")==0) SetGlobalConnectionChain(CommandLineNext(CmdLine));
	else if (strcmp(p_Opt, "-d")==0) 
	{
		LibUsefulSetValue("HTTP:Debug","Y");
		Act->Flags |= FLAG_DEBUG;
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

ListNode *ParseCommandLine(int argc, char *argv[])
{
CMDLINE *CmdLine;
ListNode *Acts;
TAction *Act=NULL, *Options=NULL;
char *StdDepsPath=NULL, *SettingsStr=NULL;
int Flags;
const char *arg;

Acts=ListCreate();
CmdLine=CommandLineParserCreate(argc, argv);

Options=CommandLineParseOptions(CmdLine);
arg=CommandLineFirst(CmdLine);
if (arg)
{
if (strcmp(arg, "install")==0) 
{
	arg=CommandLineNext(CmdLine);
	if (! arg) printf("Error: No applications given as targets for 'install' command\n");
	while (arg)
	{
	if (*arg=='-') ParseCommandLineOption(Act, CmdLine);
	else	
	{
		Act=AppActionCreate(ACT_INSTALL, arg, Options->ConfigPath);
		if (Act) ListAddItem(Acts, Act);
	}
	arg=CommandLineNext(CmdLine);
	}
}
else if (strcmp(arg, "uninstall")==0) 
{
	arg=CommandLineNext(CmdLine);
	if (! arg) printf("Error: No applications given as targets for 'uninstall' command\n");
	while (arg)
	{
	if (*arg=='-') ParseCommandLineOption(Act, CmdLine);
	else 
	{
		Act=ActionCreate(ACT_UNINSTALL, arg);
		if (Act) ListAddItem(Acts, Act);
	}
	arg=CommandLineNext(CmdLine);
	}
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

DestroyString(SettingsStr);
DestroyString(StdDepsPath);

return(Acts);
}

