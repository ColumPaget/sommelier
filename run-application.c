#include "run-application.h"
#include "platforms.h"
#include "config.h"
#include "native.h"
#include "apps.h"
#include "desktopfiles.h"
#include "xrandr.h"


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



pid_t RunSandboxed(TAction *Act)
{
    char *SpawnConfig=NULL, *Tempstr=NULL;
    pid_t pid;


    seteuid(0);
    SpawnConfig=FormatStr(SpawnConfig, "noshell +stderr container=/tmp/container/ uid=%d gid=%d dir='%s' ", getuid(), getgid(), GetVar(Act->Vars,"working-dir"));

    if (Act->Flags & FLAG_NET) SpawnConfig=CatStr(SpawnConfig, "+net ");
    else SpawnConfig=CatStr(SpawnConfig, "-net ");

    SpawnConfig=MCatStr(SpawnConfig,"setenv='LD_LIBRARY_PATH=/lib:/usr/lib:/opt/wine-3.5_GL/lib' ",NULL);

    SetVar(Act->Vars,"path",getenv("PATH"));
    SetVar(Act->Vars,"display",getenv("DISPLAY"));
    Tempstr=SubstituteVarsInString(Tempstr, "setenv='WINEPREFIX=$(prefix)' setenv='DISPLAY=$(display)' setenv='PATH=$(path)'",Act->Vars, 0);
    SpawnConfig=MCatStr(SpawnConfig, Tempstr," ",NULL);

    Tempstr=SubstituteVarsInString(Tempstr, "mnt=/etc/,/bin,/lib,/usr,/opt/wine-3.5_GL wmnt=$(prefix),/dev",Act->Vars,0);
    SpawnConfig=MCatStr(SpawnConfig, Tempstr," ",NULL);

    SpawnConfig=MCatStr(SpawnConfig, "plink=/tmp/.X11-unix/X0", " ", NULL);
    //SpawnConfig=MCatStr(SpawnConfig, "pclone=/dev/urandom,/dev/dsp,/dev/mixer", " ", NULL);

    /*
    if (Settings.Flags & FLAG_ISOCUBE) SpawnConfig=MCatStr(SpawnConfig, "isocube=",Settings.Dir," ",NULL);
    else SpawnConfig=MCatStr(SpawnConfig, "container=",Settings.Dir," ",NULL);

    if (StrValid(Settings.FileClones)) SpawnConfig=MCatStr(SpawnConfig, "pclone=",Settings.FileClones, " ", NULL);
    if (StrValid(Settings.JailSetup)) SpawnConfig=MCatStr(SpawnConfig, "jailsetup=",Settings.JailSetup, " ",NULL);
    */

    Tempstr=CopyStr(Tempstr, Act->Exec);
    if (! (Config->Flags & FLAG_DEBUG)) Tempstr=CatStr(Tempstr, " >/dev/null");
    pid=Spawn(Tempstr, SpawnConfig);

    Destroy(Tempstr);
    Destroy(SpawnConfig);

    return(pid);
}



pid_t RunNormal(TAction *Act)
{
    char *Cmd=NULL, *SpawnConfig=NULL, *Tempstr=NULL;
    const char *ptr;
    pid_t pid=-1;

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
    if (! (Config->Flags & FLAG_DEBUG)) Cmd=CatStr(Cmd, " >/dev/null");

    SpawnConfig=CopyStr(SpawnConfig, "+stderr");
    ptr=GetVar(Act->Vars, "security_level");
    if (! StrValid(ptr)) ptr="minimal";

    //still being worked on due to the need to support wine32 under linux64bit
    //SpawnConfig=MCatStr(SpawnConfig, " security='", ptr, "'", NULL);
    //Tempstr=FormatStr(Tempstr, "~gRunning:~0 ~e'%s'~0 (%s) in dir '%s' with security level ~e'%s'~0\n", Act->Name, Act->Exec, GetVar(Act->Vars, "working-dir"), ptr);

    Tempstr=FormatStr(Tempstr, "~gRunning:~0 ~e'%s'~0 (%s) in dir '%s'~0\n", Act->Name, Act->Exec, GetVar(Act->Vars, "working-dir"));
    TerminalPutStr(Tempstr, NULL);

    pid=Spawn(Cmd, SpawnConfig);

    Destroy(SpawnConfig);
    Destroy(Tempstr);
    Destroy(Cmd);

    return(pid);
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

            ptr=GetVar(Act->Vars, "runwarn");
            if (StrValid(ptr))
            {
                Tempstr=MCopyStr(Tempstr, "~yWARN: ", ptr, "~0\n", NULL);
                TerminalPutStr(Tempstr, NULL);
            }

            if (fork()==0)
            {
                XRandrGetResolution(&width, &height);
                if (Act->Flags & FLAG_SANDBOX) RunSandboxed(Act);
                else pid=RunNormal(Act);

                if ( ( ! (Config->Flags & FLAG_NO_XRANDR)) &&  (pid > 0) )
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

    Destroy(Tempstr);
}


void RunApplicationFromDesktopFile(TAction *Act)
{
    if (DesktopFileLoad(Act))
    {
        RunApplication(Act);
    }
}


void RunWineCfg(TAction *Act)
{
    char *Tempstr=NULL;

    if (DesktopFileLoad(Act))
    {
        Tempstr=AppFindInstalled(Tempstr,  Act);
        Act->Exec=CopyStr(Act->Exec, "winecfg");
        RunNormal(Act);
    }
    else fprintf(stderr, "ERROR: Failed to open .desktop file for application\n");

    Destroy(Tempstr);
}
