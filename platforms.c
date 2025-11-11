#include "platforms.h"
#include "config.h"
#include "regedit.h"
#include "doom.h"
#include "msdos.h"

ListNode *Platforms=NULL;

//make an educated guess what platform we're running on, by checking compiler macros
//that should be set on 64-bit linux. if we're on 64-bit linux we won't, by default
//look for 32-bit linux apps, and vis-a-versa


int NativeBitWidth()
{
#ifdef __x86_64__
    return(64);
#endif

#ifdef _____LP64_____
    return(64);
#endif

    return(32);
}



const char *PlatformDefault()
{
#ifdef __x86_64__
    return("!linux32");
#endif

#ifdef _____LP64_____
    return("!linux32");
#endif

    return("!linux64");
}


int PlatformType(const char *Platform)
{
    const char *WindowsPlatforms[]= {"win","win16","win32","win64","windows","windows32","windows64","gog:win","gog:windows","gog:windows64","wine", NULL};

    if (! StrValid(Platform)) return(PLATFORM_WINDOWS);
    if (MatchTokenFromList(Platform, WindowsPlatforms, 0) > -1) return(PLATFORM_WINDOWS);
    if (strcasecmp(Platform, "dos")==0) return(PLATFORM_DOS);
    if (strcasecmp(Platform, "msdos")==0) return(PLATFORM_DOS);
    if (strcasecmp(Platform, "scummvm")==0) return(PLATFORM_SCUMMVM);
    if (strcasecmp(Platform, "gog:linux")==0) return(PLATFORM_GOGLINUX);
    if (strcasecmp(Platform, "gog:linux64")==0) return(PLATFORM_GOGLINUX64);
    if (strcasecmp(Platform, "gog:scummvm")==0) return(PLATFORM_GOGSCUMMVM);
    if (strcasecmp(Platform, "gog:lindos")==0) return(PLATFORM_GOGDOS);
    if (strcasecmp(Platform, "gog:windos")==0) return(PLATFORM_GOGWINDOS);
    if (strcasecmp(Platform, "gog:neogeo")==0) return(PLATFORM_GOGNEOGEO);
    if (strcasecmp(Platform, "linux32")==0) return(PLATFORM_LINUX32);
    if (strcasecmp(Platform, "linux64")==0) return(PLATFORM_LINUX64);
    if (strcasecmp(Platform, "doom")==0) return(PLATFORM_DOOM);

    return(PLATFORM_UNKNOWN);
}


int PlatformBitWidth(const char *Platform)
{
    if (! StrValid(Platform)) return(0);
    if (strcasecmp(Platform, "linux32")==0) return(32);
    if (strcasecmp(Platform, "linux64")==0) return(64);
    if (strcasecmp(Platform, "win32")==0) return(32);
    if (strcasecmp(Platform, "windows32")==0) return(32);
    if (strcasecmp(Platform, "win64")==0) return(64);
    if (strcasecmp(Platform, "windows64")==0) return(64);

//if we've been told NOT 32-bit linux, then this is a default platform type
//and we assume 64 bit
    if (strcasecmp(Platform, "!linux32")==0) return(64);

//if we've been told NOT 64-bit linux, then this is a default platform type
//and we assume 32 bit
    if (strcasecmp(Platform, "!linux64")==0) return(32);

    if (strcasecmp(Platform, "gog:linux64")==0) return(64);
    if (strcasecmp(Platform, "gog:windows64")==0) return(64);
    if (strcasecmp(Platform, "gog:linux")==0)
    {
#ifdef __x86_64__
        return(64);
#endif

#ifdef _____LP64_____
        return(64);
#endif

        return(32);
    }

    return(0);
}




static TPlatform *PlatformsParse(const char *Line)
{
    TPlatform *Plt;
    char *Aliases=NULL, *Name=NULL, *Value=NULL;
    const char *ptr;

    if (StrValid(Line) && (Line[0] != '#'))
    {
        Plt=(TPlatform *) calloc(1, sizeof(TPlatform));
        Plt->Emulators=CopyStr(Plt->Emulators, "");
        ptr=GetToken(Line, "\\S", &Aliases, GETTOKEN_QUOTES);
        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
        while (ptr)
        {
            if (strcmp(Name, "platform")==0)
            {
                Plt->Name=CopyStr(Plt->Name, Value);
                Plt->ID=PlatformType(Value);
                if (Plt->ID==PLATFORM_UNKNOWN) Plt->ID=PLATFORM_GENERIC;
            }
            if (strcmp(Name, "emu")==0) Plt->Emulators=MCatStr(Plt->Emulators, Value, ",", NULL);
            if (strcmp(Name, "emulator")==0) Plt->Emulators=MCatStr(Plt->Emulators, Value, ",", NULL);
            if (strcmp(Name, "installer")==0) Plt->InstallerPattern=CopyStr(Plt->InstallerPattern, Value);
            if (strcmp(Name, "dir")==0) Plt->WorkingDir=CopyStr(Plt->WorkingDir, Value);
            if (strcmp(Name, "unpack-dir")==0) Plt->UnpackDir=CopyStr(Plt->UnpackDir, Value);
            if (strcmp(Name, "exec")==0) Plt->ExeSearchPattern=CopyStr(Plt->ExeSearchPattern, Value);
            if (strcmp(Name, "exec64")==0) Plt->Exe64SearchPattern=CopyStr(Plt->Exe64SearchPattern, Value);
            if (strcmp(Name, "noexec")==0) Plt->Flags |= PLATFORM_FLAG_NOEXEC;
            if (strcmp(Name, "arg")==0) Plt->Args=MCatStr(Plt->Args, "'", Value, "' ", NULL);
            if (strcmp(Name, "help")==0) Plt->HelpFile=MCatStr(Plt->HelpFile, "'", Value, "' ", NULL);
            ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
        }

        ListAddNamedItem(Platforms, Aliases, Plt);
    }

    Destroy(Name);
    Destroy(Value);
    Destroy(Aliases);

    return(Plt);
}


char *PlatformEmulatorSelect(char *RetStr,  const char *Emulators, const char *RequiredEmulator)
{
    char *EmuInvoke=NULL;
    char *Tempstr=NULL;
    char *EmuName=NULL;
    const char *ptr;

    RetStr=CopyStr(RetStr, "");
    ptr=GetToken(Emulators, ",", &EmuInvoke, 0);
    while (ptr)
    {
        GetToken(EmuInvoke, "\\S", &EmuName, 0);
        if (StrValid(EmuName))
        {
            if ( (! StrValid(RequiredEmulator)) || (strcmp(RequiredEmulator, EmuName)==0) )
            {
                Tempstr=FindFileInPath(Tempstr, EmuName, getenv("PATH"));
                if (StrValid(Tempstr))
                {
                    //don't copy the executable name, copy the entire emulator invoke action
                    RetStr=CopyStr(RetStr, EmuInvoke);
                    break;
                }
            }
        }
        ptr=GetToken(ptr, ",", &EmuInvoke, 0);
    }

    Destroy(EmuInvoke);
    Destroy(EmuName);
    Destroy(Tempstr);

    return(RetStr);
}



int PlatformAvailable(TPlatform *Plt, const char *Emulator)
{
    char *Path=NULL;
    const char *ptr;
    int result=FALSE;

    if (! StrValid(Plt->Emulators)) return(TRUE);

    Path=PlatformEmulatorSelect(Path, Plt->Emulators, Emulator);
    if (StrValid(Path)) result=TRUE;

    Destroy(Path);

    return(result);
}


TPlatform *PlatformFindWithEmulator(const char *Name, const char *Emulator, int CheckAvailable)
{
    ListNode *Curr;
    TPlatform *Plt;

    Curr=ListGetNext(Platforms);
    while (Curr)
    {
        if (InList(Name, Curr->Tag))
        {
            Plt=(TPlatform *) Curr->Item;
            if (! CheckAvailable) return(Plt);
            if (PlatformAvailable(Plt, Emulator)) return(Plt);
        }
        Curr=ListGetNext(Curr);
    }

    return(NULL);
}


const char *PlatformUnAlias(const char *Alias)
{
    TPlatform *Platform;

    Platform=PlatformFind(Alias);
    if (! Platform) return(Alias);
    return(Platform->Name);
}



void PlatformsList()
{
    ListNode *Curr;
    TPlatform *Plat;
    char *EmuList=NULL, *ArgList=NULL, *Token=NULL, *Item=NULL, *Name=NULL;
    const char *ptr, *aliases;

    Curr=ListGetNext(Platforms);
    while (Curr)
    {
        Plat=(TPlatform *) Curr->Item;
        aliases=GetToken(Curr->Tag, ",", &Name, 0);

        EmuList=CopyStr(EmuList, "");
        ptr=GetToken(Plat->Emulators, ",", &Token, GETTOKEN_QUOTES);
        while (ptr)
        {
            GetToken(Token, "\\S", &Item, GETTOKEN_QUOTES);
            EmuList=MCatStr(EmuList, GetBasename(Item), " ", NULL);
            ptr=GetToken(ptr, ",", &Token, GETTOKEN_QUOTES);
        }

        ArgList=CopyStr(ArgList, "");
        ptr=GetToken(Plat->Args, "\\S", &Token, GETTOKEN_QUOTES);
        while (ptr)
        {
            GetToken(Token, ":", &Item, 0);
            ArgList=MCatStr(ArgList, "-", Item, " ", NULL);
            ptr=GetToken(ptr, "\\S", &Token, GETTOKEN_QUOTES);
        }

        printf("%-15s  aliases: %-20s   emulators: %-20s  ", Name, aliases, EmuList);
        if (StrValid(ArgList)) printf("args: %s", ArgList);
        printf("\n");
        Curr=ListGetNext(Curr);
    }

    Destroy(EmuList);
    Destroy(ArgList);
    Destroy(Token);
    Destroy(Name);
    Destroy(Item);
}



char *PlatformLookupInfo(char *RetStr, const char *Name, int Info)
{
    TPlatform *Plt;

    RetStr=CopyStr(RetStr, "");
    Plt=PlatformFind(Name);
    if (Plt)
    {
        switch (Info)
        {
        case PLATFORM_INFO_INSTALL_MESSAGE:
            RetStr=CopyStr(RetStr, Plt->InstallMessage);
            break;

        case PLATFORM_INFO_EXE_SEARCH_PATTERN:
            RetStr=CopyStr(RetStr, Plt->ExeSearchPattern);
            break;

        case PLATFORM_INFO_EXE64_SEARCH_PATTERN:
            RetStr=CopyStr(RetStr, Plt->Exe64SearchPattern);
            break;

        case PLATFORM_INFO_EMULATORS:
            RetStr=CopyStr(RetStr, Plt->Emulators);
            break;

        case PLATFORM_INFO_WORKING_DIR:
            RetStr=CopyStr(RetStr, Plt->WorkingDir);
            break;

        }
    }

    return(RetStr);
}




char *PlatformFindEmulator(char *RetStr, const char *PlatformName, const char *Emulator)
{
    char *Tempstr=NULL, *Emulators=NULL;
    ListNode *Curr;
    TPlatform *Plt;
    const char *ptr;

//seems a bit strange, but the idea here is that we'll return NULL
//if an emulator isn't found, but return empty string if one isn't needed
    Destroy(RetStr);
    RetStr=NULL;

    Curr=ListGetNext(Platforms);
    while (Curr)
    {
        Plt=(TPlatform *) Curr->Item;
        if (strcmp(Plt->Name, PlatformName)==0)
        {
            if (StrValid(Plt->Emulators)) Emulators=MCatStr(Emulators, Plt->Emulators, ",", NULL);
        }
        Curr=ListGetNext(Curr);
    }

    switch (PlatformType(PlatformName))
    {
    case PLATFORM_GOGLINUX:
    case PLATFORM_GOGLINUX64:
    case PLATFORM_LINUX32:
    case PLATFORM_LINUX64:
        RetStr=CopyStr(RetStr, "");
        break;

    default:
        if (StrValid(Emulators)) RetStr=PlatformEmulatorSelect(RetStr, Emulators, Emulator);
        break;
    }

    Destroy(Emulators);
    Destroy(Tempstr);

    return(RetStr);
}


char *PlatformFindEmulatorNames(char *RetStr, const char *PlatformName)
{
    char *Emulators=NULL, *Token=NULL, *EmuName=NULL;
    const char *ptr;

    RetStr=CopyStr(RetStr, "");
    Emulators=PlatformLookupInfo(Emulators, PlatformName, PLATFORM_INFO_EMULATORS);
    ptr=GetToken(Emulators, ",", &Token, 0);
    while (ptr)
    {
        GetToken(Token, "\\S", &EmuName, 0);
        if (StrValid(RetStr)) RetStr=MCatStr(RetStr, ", ", EmuName, NULL);
        else RetStr=CatStr(RetStr, EmuName);

        ptr=GetToken(ptr, ",", &Token, 0);
    }

    Destroy(Emulators);
    Destroy(EmuName);
    Destroy(Token);

    return(RetStr);
}



char *PlatformSelect(char *RetStr, TAction *Act)
{
    ListNode *Curr;
    TPlatform *Plt;
    const char *p_filename;

    if (StrValid(Act->Platform) && (Act->Platform[0] != '!') ) return(CopyStr(RetStr, Act->Platform));

    RetStr=CopyStr(RetStr, "");
    p_filename=GetBasename(Act->URL);
    if (StrValid(p_filename))
    {
        Curr=ListGetNext(Platforms);
        while (Curr)
        {
            Plt=(TPlatform *) Curr->Item;
            if (
                StrValid(Act->Platform) &&
                (*Act->Platform=='!') &&
                (strcmp(Curr->Tag, Act->Platform+1)==0)
            )
            {
                /*do nothing */
            }
            else if (StrValid(Plt->InstallerPattern))
            {
                if (InList(p_filename, Plt->InstallerPattern)) RetStr=MCatStr(RetStr, Curr->Tag, ",", NULL);
            }

            Curr=ListGetNext(Curr);
        }
    }

    if (! StrValid(RetStr)) RetStr=CopyStr(RetStr, PlatformDefault());
    return(RetStr);
}


char *PlatformSubstituteArg(char *RetStr, const char *Arg, const char *PlatformArgs)
{
    char *Name=NULL, *Value=NULL, *EmulatorArgs=NULL;
    char *Item=NULL;
    const char *ptr, *p_Arg;


    ptr=GetToken(PlatformArgs, "\\S", &Item, GETTOKEN_QUOTES);
    while (ptr)
    {
        p_Arg=Arg;
        if (*p_Arg == '-') p_Arg++;
        Value=CopyStr(Value, GetToken(Item, ":", &Name, 0));
        if (strcmp(p_Arg, Name)==0) EmulatorArgs=MCatStr(EmulatorArgs, " ", Value, NULL);
        ptr=GetToken(ptr, "\\S", &Item, GETTOKEN_QUOTES);
    }

    if (! StrValid(EmulatorArgs)) EmulatorArgs=MCatStr(EmulatorArgs, " ", Arg, NULL);
    RetStr=CatStr(RetStr, EmulatorArgs);

    Destroy(Name);
    Destroy(Value);
    Destroy(Item);
    Destroy(EmulatorArgs);


    return(RetStr);
}


void PlatformSetupEmulatorArgs(TAction *Act, const char *Name, const char *Args)
{
    TPlatform *Plt;
    char *Arg=NULL, *EmulatorArgs=NULL;
    const char *ptr;

    Plt=PlatformFindWithEmulator(GetVar(Act->Vars, "platform"), GetVar(Act->Vars, "required_emulator"), TRUE);
    if (Plt)
    {
        ptr=GetToken(Args, "\\S", &Arg, GETTOKEN_QUOTES);
        while (ptr)
        {
            EmulatorArgs=PlatformSubstituteArg(EmulatorArgs, Arg, Plt->Args);
            ptr=GetToken(ptr, "\\S", &Arg, GETTOKEN_QUOTES);
        }
    }

    SetVar(Act->Vars, Name, EmulatorArgs);
    ResolveVar(Act->Vars, Name);

    Destroy(EmulatorArgs);
    Destroy(Arg);
}


void PlatformApplySettings(TAction *Act)
{
    const char *ptr;
    char *Tempstr=NULL;

    PlatformSetupEmulatorArgs(Act, "emulator-args", "");
    switch (Act->PlatformID)
    {
    case PLATFORM_WINDOWS:
        RegEditApplySettings(Act);
        break;

    case PLATFORM_DOOM:
        DoomApplySettings(Act);
        break;

    case PLATFORM_DOS:
    case PLATFORM_GOGDOS:
    case PLATFORM_GOGWINDOS:
        MSDOSApplySettings(Act);
        break;
    }
}


void PlatformFindIcon(TAction *Act)
{
    char *Tempstr=NULL;

    switch (Act->PlatformID)
    {
    case PLATFORM_ZXSPECTRUM:
        break;
    }
    Destroy(Tempstr);
}




void PlatformsInit(const char *Path)
{
    STREAM *S;
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;

    Platforms=ListCreate();
    Tempstr=FormatPath(Tempstr, Path);
    ptr=GetToken(Tempstr, ",", &Token, GETTOKEN_QUOTES);
    while (ptr)
    {
        S=STREAMOpen(Token, "");
        if (S) break;
        ptr=GetToken(ptr, ",", &Token, GETTOKEN_QUOTES);
    }

    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            PlatformsParse(Tempstr);
            Tempstr=STREAMReadLine(Tempstr, S);
        }
        STREAMClose(S);
    }
    else
    {
        Tempstr=FormatStr(Tempstr, "~rERROR: no platforms.conf found in:  %s~0\n", Path);
        TerminalPutStr(Tempstr, NULL);
    }

    Destroy(Tempstr);
    Destroy(Token);
}


