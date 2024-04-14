#include "desktopfiles.h"
#include "platforms.h"
#include "run-application.h"
#include "config.h"


static char *DesktopFileMakeInstallPath(char *RetStr, TAction *Act)
{
    char *Tempstr=NULL;

    if (Config->Flags & FLAG_SYSTEM_INSTALL) Tempstr=CopyStr(Tempstr, "/opt/share/applications/");
    else Tempstr=SubstituteVarsInString(Tempstr, "$(homedir)/.local/share/applications/", Act->Vars, 0);

    SetVar(Act->Vars, "desktop-path", Tempstr);
    RetStr=SubstituteVarsInString(RetStr, "$(desktop-path)/$(name).desktop",Act->Vars, 0);

    Destroy(Tempstr);

    return(RetStr);
}


static char *DesktopFileMakeSearchPath(char *RetStr, TAction *Act)
{
    char *Tempstr=NULL, *Path=NULL;
    const char *SearchPath="$(homedir)/.local/share/applications/$(name).desktop:/opt/share/applications/$(name).desktop";
    const char *ptr;

    RetStr=CopyStr(RetStr, "");
    ptr=GetToken(SearchPath, ":", &Path, 0);
    while (ptr)
    {
        Tempstr=SubstituteVarsInString(Tempstr, Path, Act->Vars, 0);
        if (access(Tempstr, F_OK)==0)
        {
            RetStr=CopyStr(RetStr, Tempstr);
            break;
        }
        ptr=GetToken(ptr, ":", &Path, 0);
    }

    Destroy(Tempstr);
    Destroy(Path);

    return(RetStr);
}


int DesktopFileDelete(TAction *Act)
{
    char *Tempstr=NULL;
    int result;

    Tempstr=DesktopFileMakeInstallPath(Tempstr, Act);
    result=unlink(Tempstr);

    Destroy(Tempstr);
    if (result==0) return(TRUE);
    return(FALSE);
}

int DesktopFileRead(const char *Path, TAction *Act)
{
    char *Tempstr=NULL, *Name=NULL, *Token=NULL, *Exec=NULL;
    const char *ptr;
    int result=FALSE;
    STREAM *S;

    S=STREAMOpen(Path, "r");
    if (S)
    {
        result=TRUE;
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            ptr=GetToken(Tempstr, "=", &Name, 0);
            if (strcasecmp(Name,"SommelierExec")==0)
            {
                Act->Exec=CopyStr(Act->Exec, ptr);
                StripQuotes(Act->Exec);
            }
            else if (strcasecmp(Name,"Exec")==0)
            {
                Exec=CopyStr(Exec, ptr);
                StripQuotes(Exec);
            }
            else if (strcasecmp(Name,"Icon")==0)
            {
                Name=CopyStr(Name, ptr);
                StripQuotes(Name);
                SetVar(Act->Vars, "icon", Name);
            }
            else if (strcasecmp(Name,"Platform")==0)
            {
                Name=CopyStr(Name, ptr);
                StripQuotes(Name);
                SetVar(Act->Vars, "platform", Name);
            }
            else if (strcasecmp(Name,"Emulator")==0)
            {
                Name=CopyStr(Name, ptr);
                StripQuotes(Name);
                SetVar(Act->Vars, "emulator", Name);
								GetToken(Name, "\\S", &Token, 0);
								SetVar(Act->Vars, "required_emulator", GetBasename(Token));	
            }
            else if (strcasecmp(Name,"Path")==0)
            {
                Name=CopyStr(Name, ptr);
                StripQuotes(Name);
                SetVar(Act->Vars, "working-dir", Name);
            }
            Tempstr=STREAMReadLine(Tempstr, S);
        }
        STREAMClose(S);
    }
    else fprintf(stderr, "ERROR: Failed to open .desktop file '%s' for application\n", Path);

//if we didn't find a 'SommelierExec' entry then we must be dealing with
//a setup that doesn't use sommelier to run the app. Thus the 'Exec' entry
//is the thing that we run
    if (! StrValid(Act->Exec))
    {
        //make sure we don't run another copy of sommelier! This would cause a fork bomb.
        GetToken(Exec, "\\S", &Token, 0);
        if (strcmp(GetBasename(Token), "sommelier") !=0) Act->Exec=CopyStr(Act->Exec, Exec);
    }

//now we rebuild the exec, extracting 'WINEPREFIX' from it if such exists
    Tempstr=CopyStr(Tempstr, Act->Exec);
    Act->Exec=CopyStr(Act->Exec, "");
    ptr=GetToken(Tempstr,"\\S",&Token,GETTOKEN_HONOR_QUOTES);
    while (ptr)
    {
        if (strncmp(Token,"WINEPREFIX=",11)==0) SetVar(Act->Vars, "prefix", Token+11);
        else Act->Exec=MCatStr(Act->Exec, Token, " ",NULL);
        ptr=GetToken(ptr,"\\S",&Token, GETTOKEN_HONOR_QUOTES);
    }

    DestroyString(Name);
    DestroyString(Exec);
    DestroyString(Token);
    DestroyString(Tempstr);

    return(result);
}


int DesktopFileLoad(TAction *Act)
{
    char *Tempstr=NULL;
    int result=FALSE;

    Tempstr=DesktopFileMakeSearchPath(Tempstr, Act);
    if (StrValid(Tempstr))
    {
        result=DesktopFileRead(Tempstr, Act);
    }
    else fprintf(stderr, "ERROR: Failed to find .desktop file '%s' for application\n", Tempstr);

    Destroy(Tempstr);

    return(result);
}


void DesktopFileDirectoryRunAll(const char *DirPath)
{
    TAction *Act;
    glob_t Glob;
    char *Tempstr=NULL;
    int i;

    Tempstr=MCopyStr(Tempstr, DirPath, "/*", NULL);
    glob(Tempstr, 0, 0, &Glob);
    for (i=0; i < Glob.gl_pathc; i++)
    {
        Act=ActionCreate(ACT_RUN, "startup");
        DesktopFileRead(Glob.gl_pathv[i], Act);
        RunApplication(Act);
        ActionDestroy(Act);
    }
    globfree(&Glob);

    Destroy(Tempstr);
}


//setup configuration for a native linux app
static void DesktopFileConfigureNative(TAction *Act)
{
    const char *ptr;
    char *Tempstr=NULL;

    ptr=GetVar(Act->Vars, "exec");
    if (StrValid(ptr))
    {
        Tempstr=SubstituteVarsInString(Tempstr, GetVar(Act->Vars, "ld_preload"), Act->Vars, 0);
        SetVar(Act->Vars, "ld_preload", Tempstr);

        if (GetBoolVar(Act->Vars, "no-exec-arg")) Tempstr=SubstituteVarsInString(Tempstr, "LD_PRELOAD=$(ld_preload) $(platform-vars) $(exec-vars)", Act->Vars, 0);
        else Tempstr=SubstituteVarsInString(Tempstr, "LD_PRELOAD=$(ld_preload) $(platform-vars) $(exec-vars) \"$(exec-path)\" $(exec-args)", Act->Vars, 0);
        StripLeadingWhitespace(Tempstr);
        StripTrailingWhitespace(Tempstr);

        SetVar(Act->Vars, "invocation", Tempstr);
    }

    ptr=GetVar(Act->Vars, "exec64");
    if (StrValid(ptr))
    {
        Tempstr=SubstituteVarsInString(Tempstr, "\"$(exec-dir)/$(exec64)\"", Act->Vars, 0);
        StripLeadingWhitespace(Tempstr);
        StripTrailingWhitespace(Tempstr);

        SetVar(Act->Vars, "invocation64", Tempstr);
    }
    SetVar(Act->Vars, "invoke-dir", GetVar(Act->Vars, "working-dir"));

    Destroy(Tempstr);
}


//setup configuration for a windows app run via WINE
static void DesktopFileConfigureWine(TAction *Act)
{
    char *Tempstr=NULL, *Quoted=NULL;

    //for windows we must override the found exec-path to be in windows format
    Tempstr=SubstituteVarsInString(Tempstr, "C:\\$(exec-dir)\\$(exec)", Act->Vars, 0);
    strrep(Tempstr, '/', '\\');
    Quoted=QuoteCharsInStr(Quoted, Tempstr, "\"");
    SetVar(Act->Vars,"exec-path", Quoted);

    if (strcmp(Act->Platform, "win64")==0) Tempstr=SubstituteVarsInString(Tempstr, "WINEARCH=win64 WINEPREFIX=$(prefix) wine \"$(exec-path)\" $(exec-args)", Act->Vars, 0);
    else if (strcmp(Act->Platform, "win32")==0) Tempstr=SubstituteVarsInString(Tempstr, "WINEARCH=win32 WINEPREFIX=$(prefix) wine \"$(exec-path)\" $(exec-args)", Act->Vars, 0);
    else Tempstr=SubstituteVarsInString(Tempstr, "WINEPREFIX=$(prefix) wine \"$(exec-path)\" $(exec-args)", Act->Vars, 0);

    SetVar(Act->Vars, "invocation", Tempstr);
    SetVar(Act->Vars, "invoke-dir", GetVar(Act->Vars, "working-dir"));

    Destroy(Tempstr);
    Destroy(Quoted);
}


//setup configuration for an app run via an emulator
static void DesktopFileConfigureEmulator(TAction *Act)
{
    char *EmuInvoke=NULL, *Tempstr=NULL;
    const char *ptr;

    EmuInvoke=PlatformFindEmulator(EmuInvoke, Act->Platform, GetVar(Act->Vars, "required_emulator"));
    if (GetBoolVar(Act->Vars, "no-exec-arg"))
    {
        SetVar(Act->Vars, "exec", "");
        SetVar(Act->Vars, "exec-path", "");
        SetVar(Act->Vars, "exec-args", "");
    }

    Tempstr=SubstituteVarsInString(Tempstr, EmuInvoke, Act->Vars, 0);
    StripLeadingWhitespace(Tempstr);
    StripTrailingWhitespace(Tempstr);
    SetVar(Act->Vars, "invocation", Tempstr);
    SetVar(Act->Vars, "invoke-dir", GetVar(Act->Vars, "working-dir"));

    Destroy(EmuInvoke);
    Destroy(Tempstr);
}


void DesktopFileGenerate(TAction *Act)
{
    STREAM *S;
    char *Tempstr=NULL, *Hash=NULL;
    const char *ptr;


    Tempstr=DesktopFileMakeInstallPath(Tempstr, Act);
    printf("Generating desktop File %s\n", Tempstr);
    MakeDirPath(Tempstr, 0744);
    S=STREAMOpen(Tempstr, "w mode=0744");
    if (S)
    {
        fchmod(S->out_fd, 0744);

        switch (Act->PlatformID)
        {
        case PLATFORM_WINDOWS:
            DesktopFileConfigureWine(Act);
            break;

        //native apps
        case PLATFORM_LINUX32:
        case PLATFORM_LINUX64:
        case PLATFORM_GOGLINUX:
        case PLATFORM_GOGLINUX64:
            DesktopFileConfigureNative(Act);
            break;

        default:
        //emulated apps that aren't 'wine'
        case PLATFORM_DOS:
        case PLATFORM_SCUMMVM:
        case PLATFORM_GOGSCUMMVM:
        case PLATFORM_GOGDOS:
        case PLATFORM_GOGWINDOS:
        case PLATFORM_GOGNEOGEO:
        case PLATFORM_DOOM:
            DesktopFileConfigureEmulator(Act);
            break;
        }


        HashFile(&Hash, "sha256", GetVar(Act->Vars, "exec"), ENCODE_HEX);
        SetVar(Act->Vars, "exec-sha256", Hash);
        SetVar(Act->Vars, "platform", Act->Platform);

//did we download or otherwise obtain an application icon? If not, then consider the 'icon' setting
//which may point to an icon for this app on local disk
        ptr=GetVar(Act->Vars, "app-icon");
        if (! StrValid(ptr))
        {
            ptr=GetVar(Act->Vars, "icon");
            if (StrValid(ptr))
            {
                Tempstr=SubstituteVarsInString(Tempstr, ptr, Act->Vars, 0);
                if (StrValid(Tempstr) && (access(Tempstr, F_OK)==0)) SetVar(Act->Vars, "app-icon", Tempstr);
            }
        }


        Tempstr=SubstituteVarsInString(Tempstr, "[Desktop Entry]\nName=$(name)\nType=Application\nTerminal=false\nPlatform=$(platform)\nEmulator=$(emulator)\nComment=$(comment)\nSHA256=$(exec-sha256)\nPath=$(invoke-dir)\nExec=sommelier run $(name)\nSommelierExec=$(invocation)\nIcon=$(app-icon)\nRunsWith=$(runswith)\n",Act->Vars, 0);
        STREAMWriteLine(Tempstr, S);
        Tempstr=SubstituteVarsInString(Tempstr, "Categories=$(category)\nCategory=$(category)\n",Act->Vars, 0);
        STREAMWriteLine(Tempstr, S);
        STREAMClose(S);

    }

    DestroyString(Tempstr);
    DestroyString(Hash);
}
