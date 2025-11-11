#include "desktopfiles.h"
#include "platforms.h"
#include "run-application.h"
#include "config.h"
#include "sandbox.h"

//Make a path that we will install a desktop file at.
//Handle 'modern' sommelier desktop files that have the format $(program name)-$(platform).desktop
//and also handle old (version 1) desktop files, with format $(program name).desktop
static char *DesktopFileMakeInstallPath(char *RetStr, TAction *Act, int Version)
{
    char *Tempstr=NULL;

    if (Config->Flags & FLAG_SYSTEM_INSTALL) Tempstr=CopyStr(Tempstr, "/opt/share/applications/");
    else Tempstr=SubstituteVarsInString(Tempstr, "$(homedir)/.local/share/applications/", Act->Vars, 0);

    SetVar(Act->Vars, "desktop-path", Tempstr);
    if (Version==1) RetStr=SubstituteVarsInString(RetStr, "$(desktop-path)/$(name).desktop",Act->Vars, 0);
    else RetStr=SubstituteVarsInString(RetStr, "$(desktop-path)/$(name)-$(platform).desktop",Act->Vars, 0);

    Destroy(Tempstr);

    return(RetStr);
}


//Search for a desktop file.
//Handle 'modern' sommelier desktop files that have the format $(program name)-$(platform).desktop
//and also handle old (version 1) desktop files, with format $(program name).desktop
static char *DesktopFileSearchPath(char *RetStr, TAction *Act, int Version)
{
    char *Tempstr=NULL, *Path=NULL, *Dir=NULL, *FName=NULL;
    const char *SearchPath="$(homedir)/.local/share/applications/:/opt/share/applications/";
    const char *ptr;

    RetStr=CopyStr(RetStr, "");

    ptr=GetVar(Act->Vars, "platform");
    if (! StrValid(ptr)) ptr="*";

    if (Version==1) FName=MCopyStr(FName, GetVar(Act->Vars, "name"), ".desktop", NULL);
    else FName=MCopyStr(FName, GetVar(Act->Vars, "name"), "-", ptr, ".desktop", NULL);

    ptr=GetToken(SearchPath, ":", &Path, 0);
    while (ptr)
    {
        Dir=SubstituteVarsInString(Dir, Path, Act->Vars, 0);

        Tempstr=GlobNoCase(Tempstr, Dir, FName);
        if (StrValid(Tempstr))
        {
            RetStr=CopyStr(RetStr, Tempstr);
            break;
        }
        ptr=GetToken(ptr, ":", &Path, 0);
    }

    Destroy(Tempstr);
    Destroy(FName);
    Destroy(Path);
    Destroy(Dir);

    return(RetStr);
}


int DesktopFileDelete(TAction *Act)
{
    char *Tempstr=NULL;
    int result;

    Tempstr=DesktopFileMakeInstallPath(Tempstr, Act, 0);
    result=unlink(Tempstr);
    if (result != 0)
    {
        Tempstr=DesktopFileMakeInstallPath(Tempstr, Act, 1);
        result=unlink(Tempstr);
    }

    Destroy(Tempstr);
    if (result==0) return(TRUE);
    return(FALSE);
}


static void DesktopParseVar(TAction *Act, const char *Name, const char *Value, int Force)
{
char *Token=NULL;
const char *ptr;

//don't overwrite anything that's been set on the command-line
if (! Force)
{
ptr=GetVar(Act->Vars, Name);
if (StrValid(ptr)) return;
}

Token=CopyStr(Token, Value);
StripQuotes(Token);
SetVar(Act->Vars, Name, Token);

Destroy(Token);
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
            else if (strcasecmp(Name, "SommelierInstallPath")==0)
            {
                Token=CopyStr(Token, ptr);
                StripQuotes(Token);
                Act->InstallPath=CopyStr(Act->InstallPath, Token);
            }
            else if (strcasecmp(Name,"SommelierAllowSU")==0)
            {
                Token=CopyStr(Token, ptr);
                StripQuotes(Token);
                if (strtobool(Token)) Act->Flags |= FLAG_ALLOW_SU;
            }
            else if (strcasecmp(Name,"Sommelier_X86_LD_LIBRARY_PATH")==0) DesktopParseVar(Act, "x86_ld_library_path", ptr, FALSE);
            else if (strcasecmp(Name,"Sommelier_X86_64_LD_LIBRARY_PATH")==0) DesktopParseVar(Act, "x86_64_ld_library_path", ptr, FALSE);
            else if (strcasecmp(Name,"SommelierSecurityLevel")==0) DesktopParseVar(Act, "security_level", ptr, FALSE);
            else if (strcasecmp(Name,"SommelierRunWarn")==0) DesktopParseVar(Act, "runwarn", ptr, FALSE);
            else if (strcasecmp(Name,"Icon")==0) DesktopParseVar(Act, "icon", ptr, FALSE);
            else if (strcasecmp(Name,"Platform")==0) DesktopParseVar(Act, "platform", ptr, FALSE);
            else if (strcasecmp(Name,"Path")==0) DesktopParseVar(Act, "working-dir", ptr, FALSE);
            else if (strcasecmp(Name,"Emulator")==0)
            {
                //naughty reuse of name here
                Name=CopyStr(Name, ptr);
                StripQuotes(Name);
                SetVar(Act->Vars, "emulator", Name);
                GetToken(Name, "\\S", &Token, 0);
                SetVar(Act->Vars, "required_emulator", GetBasename(Token));
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

    Tempstr=DesktopFileSearchPath(Tempstr, Act, 0);
    if (! StrValid(Tempstr)) Tempstr=DesktopFileSearchPath(Tempstr, Act, 1);

    if (StrValid(Tempstr)) result=DesktopFileRead(Tempstr, Act);
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
    char *Tempstr=NULL, *Path=NULL, *Hash=NULL;
    const char *ptr;


    Path=DesktopFileMakeInstallPath(Path, Act, 0);
    Tempstr=FormatStr(Tempstr, "~yGenerating desktop File  %s for %s~0\n", Path, Act->Name);
    TerminalPutStr(Tempstr, NULL);

    MakeDirPath(Path, 0744);
    S=STREAMOpen(Path, "w mode=0744");
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

        //don't use AppAllowSU to check here,  because that checks if switch user/superuser is allowed
        //for the app, or for this run of sommelier. It must be explicitly set against the app.
        if (AppAllowSU(Act)) SetVar(Act->Vars, "allow-su", "Y");

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


        Tempstr=SubstituteVarsInString(Tempstr, "[Desktop Entry]\nName=$(name)\nType=Application\nTerminal=false\nPlatform=$(platform)\nEmulator=$(emulator)\nComment=$(comment)\nSHA256=$(exec-sha256)\nPath=$(invoke-dir)\nExec=sommelier run $(name)\nSommelierExec=$(invocation)\nSommelierInstallPath=$(invoke-dir)\nSommelier_X86_LD_LIBRARY_PATH='$(x86_ld_library_path)'\nSommelier_X86_64_LD_LIBRARY_PATH='$(x86_64_ld_library_path)'\nSommelierAllowSU=$(allow-su)\nSommelierSecurityLevel=$(security_level)\nSommelierRunWarn=$(runwarn)\nIcon=$(app-icon)\nPlatform=$(platform)\nRunsWith=$(runswith)\n",Act->Vars, 0);
        STREAMWriteLine(Tempstr, S);
        Tempstr=SubstituteVarsInString(Tempstr, "Categories=$(category)\nCategory=$(category)\n",Act->Vars, 0);
        STREAMWriteLine(Tempstr, S);
        STREAMClose(S);

    }
		else TerminalPutStr("~e~rERROR:~0 failed to open desktop file for writing!\n", NULL);

    DestroyString(Tempstr);
    DestroyString(Hash);
    DestroyString(Path);
}
