#include "run-application.h"
#include "platforms.h"
#include "config.h"
#include "native.h"
#include "apps.h"
#include "desktopfiles.h"
#include "emulator.h"
#include "xrandr.h"
#include "sandbox.h"

static void GenerateMissingLibs(TAction *Act, const char *MissingLibs)
{
    char *Token=NULL, *Src=NULL, *Dest=NULL, *Lib=NULL, *LibPath=NULL;
    const char *ptr;

    Token=FormatStr(Token,"~rExecutable requires missing libs: %s~0\n", MissingLibs);
    TerminalPutStr(Token, NULL);
    LibPath=NativeLoadLibPath(LibPath);
    ptr=GetToken(MissingLibs, " ", &Token, 0);
    while (ptr)
    {
        Lib=CopyStr(Lib, Token);
        StrTruncChar(Lib, '.');
        Lib=CatStr(Lib, ".so");

        Src=FindFileInPath(Src, Lib, LibPath);
        if (StrValid(Src))
        {
            Dest=MCopyStr(Dest, GetVar(Act->Vars, "sommelier_patches_dir"), "/", Token, NULL);
            MakeDirPath(Dest, 0700);
            printf("Found: %s. Attempting 'hail mary' link to this library\n", Src);
            symlink(Src, Dest);
        }

        ptr=GetToken(ptr, " ", &Token, 0);
    }

    Destroy(Src);
    Destroy(Dest);
    Destroy(Token);
    Destroy(Lib);
}


static char *GenerateLD_LIBRARY_PATH(char *RetStr, TAction *Act)
{
    char *Tempstr=NULL;
    const char *ptr;


    if (PlatformBitWidth(Act->Platform) == 32) ptr=GetVar(Act->Vars, "x86_ld_library_path");
    else ptr=GetVar(Act->Vars, "x86_64_ld_library_path");

    if (StrValid(ptr)) Tempstr=MCatStr(Tempstr, ptr, ":", NULL);
    ptr=GetVar(Act->Vars, "sommelier_patches_dir");
    if (StrValid(ptr)) Tempstr=MCatStr(Tempstr, ptr, ":", NULL);

    if (StrValid(Tempstr)) RetStr=MCatStr(RetStr, "LD_LIBRARY_PATH=", Tempstr, " ", NULL);

    Destroy(Tempstr);
    return(RetStr);
}


static char *GenerateApplicationCommandLine(char *CommandLine, TAction *Act)
{
    char *Template=NULL, *Tempstr=NULL, *Token=NULL;
    const char *ptr;

    SetVar(Act->Vars, "exec", Act->Exec);
    PlatformSetupEmulatorArgs(Act, "exec-args", Act->Args);

    ptr=GetToken(Act->Exec, "\\S", &Token, 0);
    while (ptr)
    {
        if (! strchr(Token, '='))
        {
            NativeExecutableCheckLibs(Token, &Tempstr);
            break;
        }
        ptr=GetToken(ptr, "\\S", &Token, 0);
    }

    if (StrValid(Tempstr)) GenerateMissingLibs(Act, Tempstr);

    //this will add library paths for our patches, or for applications
    //that ship with libraries in subdirectories
    Template=GenerateLD_LIBRARY_PATH(Template, Act);

    //if there's anything to LD_PRELOAD, then add that
    ptr=GetVar(Act->Vars, "ld_preload");
    if (StrValid(ptr)) Template=MCatStr(Template, "LD_PRELOAD=", ptr, " ", NULL);

    Template=CatStr(Template, "WINEPREFIX=$(prefix) $(exec) $(exec-args)");

    CommandLine=SubstituteVarsInString(CommandLine, Template, Act->Vars, 0);

    printf("Command: %s\n", CommandLine);
    Destroy(Template);
    Destroy(Tempstr);
    Destroy(Token);

    return(CommandLine);
}




pid_t LaunchApplication(TAction *Act)
{
    char *Cmd=NULL, *SpawnConfig=NULL, *Secure=NULL, *Tempstr=NULL;
    TPlatform *Platform;
    const char *ptr;
    pid_t pid=-1;

    Platform=(TPlatform *) PlatformFind(Act->Platform);
    ptr=GetVar(Act->Vars, "saves-dir");
    if (StrValid(ptr))
    {
        MakeDirPath(ptr, 0700);
    }

    ptr=GetVar(Act->Vars, "working-dir");
    if (StrValid(ptr))
    {
        printf("switching to working directory: %s\n", ptr);
        if (chdir(ptr) !=0) perror("ERROR switching to directory: ");
    }

    Cmd=GenerateApplicationCommandLine(Cmd, Act);
    if (Platform && (Platform->Flags & PLATFORM_FLAG_NOSTDERR))
    {
        if (! (Config->Flags & CONF_DEBUG)) Cmd=CatStr(Cmd, " >/dev/null");
    }

    Tempstr=EmulatorGetHelp(Tempstr, Act);
    if (StrValid(Tempstr)) TerminalPutStr(Tempstr, NULL);


    Secure=SeccompSandboxGetLevel(Secure, Act);
    SpawnConfig=CopyStr(SpawnConfig, "+stderr");

    if (StrValid(Secure)) SpawnConfig=MCatStr(SpawnConfig, " security='", Secure, "' ", NULL);

    // if CONF_DENY_NET is set, we will already have done 'deny'
    if (! (Config->Flags & CONF_DENY_NET))
    {
        if (! AppAllowNet(Act)) SpawnConfig=CatStr(SpawnConfig, "nonet ");
    }

    // if CONF_DENY_NET is set, we will already have done 'deny'
    if (! (Config->Flags & CONF_DENY_PID))
    {
        if (! AppAllowPids(Act)) SpawnConfig=CatStr(SpawnConfig, "nopid ");
    }


    Tempstr=CopyStr(Tempstr, SommelierSecurity);
    Tempstr=CatStr(Tempstr, Secure);
    Secure=CopyStr(Secure, Tempstr);

    if (StrValid(Secure)) Tempstr=FormatStr(Tempstr, "~gRunning:~0 ~e'%s'~0 (%s) in dir '%s' with security level ~e'%s'~0\n", Act->Name, Act->Exec, GetVar(Act->Vars, "working-dir"), Secure);
    else Tempstr=FormatStr(Tempstr, "~gRunning:~0 ~e'%s'~0 (%s) in dir '%s'~0\n", Act->Name, Act->Exec, GetVar(Act->Vars, "working-dir"));

    if (AppAllowSU(Act)) Tempstr=CatStr(Tempstr, "~ySU ALLOWED:~0 This app is allowed to switch-user/super-user via su/sudo/suid etc\n");
    TerminalPutStr(Tempstr, NULL);

    pid=Spawn(Cmd, SpawnConfig);

    Destroy(SpawnConfig);
    Destroy(Secure);
    Destroy(Tempstr);
    Destroy(Cmd);

    return(pid);
}


//AppImages are such a bad idea, hard to sandbox becuase they need mount!
static int IsAppImage(const char *Invocation)
{
    char *Token=NULL;
    const char *ptr, *extn;
    int RetVal=FALSE;

    ptr=GetToken(Invocation, "\\S", &Token, GETTOKEN_QUOTES);
    while (ptr)
    {
        StripQuotes(Token);
        extn=strrchr(Token, '.');
        if (StrValid(extn) && (strcasecmp(extn, ".AppImage")==0))  RetVal=TRUE;
        ptr=GetToken(ptr, "\\S", &Token, GETTOKEN_QUOTES);
    }

    Destroy(Token);
    return(RetVal);
}


void RunApplication(TAction *Act)
{
    char *Tempstr=NULL;
    const char *ptr;
    int width, height;
    pid_t pid;

    if (AppIsInstalled(Act))
    {
        if (StrValid(Act->Exec))
        {
            if (IsAppImage(Act->Exec))
            {
                TerminalPutStr("~yWARN: Application is an AppImage and therefore requires enhanced privileges to boot. no SU/Seccomp sandboxing active~0\n", NULL);
                Config->Flags |= CONF_ALLOW_SU;
            }

            if (! AppAllowSU(Act)) SetNoSU();

            ptr=GetVar(Act->Vars, "runwarn");
            if (StrValid(ptr))
            {
                Tempstr=MCopyStr(Tempstr, "~yWARN: ", ptr, "~0\n", NULL);
                TerminalPutStr(Tempstr, NULL);
            }

            if (fork()==0)
            {
                XRandrGetResolution(&width, &height);
                pid=LaunchApplication(Act);

                if ( ( ! (Config->Flags & CONF_NO_XRANDR)) &&  (pid > 0) )
                {
                    //wait for the application to exit, and collect any other
                    //pids while we do so
                    while (1)
                    {
                        if (waitpid(-1, NULL, 0) == pid) break;
                    }
                    XRandrSetResolution(width, height);
                }
                _exit(0);
            }
        }
        else fprintf(stderr, "ERROR: Failed to open .desktop file for application\n");
    }
    else fprintf(stderr, "ERROR: application not installed\n");

    //sleep for a while so that we can print out all launch output before returning to shell
    sleep(3);

    Destroy(Tempstr);
}


void RunApplicationFromDesktopFile(TAction *Act)
{
    if (DesktopFileLoad(Act))
    {
        RunApplication(Act);
    }
}


void RunWineUtility(TAction *Act, const char *Utility)
{
    char *Tempstr=NULL;

    if (DesktopFileLoad(Act))
    {
        Tempstr=AppFindInstalled(Tempstr,  Act);
        Act->Exec=CopyStr(Act->Exec, Utility);
        LaunchApplication(Act);
    }
    else fprintf(stderr, "ERROR: Failed to open .desktop file for application\n");

    Destroy(Tempstr);
}
