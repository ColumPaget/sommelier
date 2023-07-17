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


static char *GenerateApplicationCommandLine(char *CommandLine, TAction *Act)
{
    char *Template=NULL, *Tempstr=NULL, *Token=NULL;
    const char *ptr;

    SetVar(Act->Vars, "exec", Act->Exec);
    SetVar(Act->Vars, "exec-args", Act->Args);

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

    if (StrValid(Tempstr))
    {
        GenerateMissingLibs(Act, Tempstr);
        Template=MCatStr(Template, "LD_LIBRARY_PATH=", GetVar(Act->Vars, "sommelier_patches_dir"), " ", NULL);
    }

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
    char *SpawnConfig=NULL, *Tempstr=NULL;
    const char *ptr;
    pid_t pid=-1;


    ptr=GetVar(Act->Vars, "working-dir");
    if (StrValid(ptr))
    {
        if (chdir(ptr) !=0) perror("ERROR switching to directory: ");
    }

    printf("Running '%s' (%s) in dir %s\n", Act->Name, Act->Exec, ptr);
    Tempstr=GenerateApplicationCommandLine(Tempstr, Act);

    if (! (Config->Flags & FLAG_DEBUG)) Tempstr=CatStr(Tempstr, " >/dev/null");
    SpawnConfig=CopyStr(SpawnConfig, "+stderr");
    pid=Spawn(Tempstr, SpawnConfig);

    Destroy(Tempstr);
    Destroy(SpawnConfig);

    return(pid);
}


void RunApplication(TAction *Act)
{
    char *Tempstr=NULL;
    int width, height;
    pid_t pid;

    if (DesktopFileRead(Act))
    {
        Tempstr=AppFormatPath(Tempstr,  Act);
        if (StrValid(Act->Exec))
        {
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

    Destroy(Tempstr);
}


void RunWineCfg(TAction *Act)
{
    char *Tempstr=NULL;

    if (DesktopFileRead(Act))
    {
        Tempstr=AppFormatPath(Tempstr,  Act);
        Act->Exec=CopyStr(Act->Exec, "winecfg");
        RunNormal(Act);
    }
    else fprintf(stderr, "ERROR: Failed to open .desktop file for application\n");

    Destroy(Tempstr);
}
