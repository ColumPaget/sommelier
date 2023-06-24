#include "platforms.h"
#include "config.h"
#include "regedit.h"
#include "doom.h"

ListNode *Platforms=NULL;

//make an educated guess what platform we're running on, by checking compiler macros
//that should be set on 64-bit linux. if we're on 64-bit linux we won't, by default
//look for 32-bit linux apps, and vis-a-versa
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
    if (strcasecmp(Platform, "scummvm")==0) return(PLATFORM_SCUMMVM);
    if (strcasecmp(Platform, "gog.com:linux")==0) return(PLATFORM_GOGLINUX);
    if (strcasecmp(Platform, "gog.com:linux64")==0) return(PLATFORM_GOGLINUX64);
    if (strcasecmp(Platform, "gog.com:scummvm")==0) return(PLATFORM_GOGSCUMMVM);
    if (strcasecmp(Platform, "gog.com:lindos")==0) return(PLATFORM_GOGDOS);
    if (strcasecmp(Platform, "gog.com:windos")==0) return(PLATFORM_GOGWINDOS);
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


static TPlatform *PlatformsAdd(const char *Names, int IDnum, const char *Emulators, const char *WorkingDir, const char *ExeSearchPattern, const char *Exe64SearchPattern)
{
    TPlatform *Plt;

    Plt=(TPlatform *) calloc(1, sizeof(TPlatform));
    Plt->ID=IDnum;
    Plt->Emulators=CopyStr(Plt->Emulators, Emulators);
    Plt->WorkingDir=CopyStr(Plt->WorkingDir, WorkingDir);
    Plt->ExeSearchPattern=CopyStr(Plt->ExeSearchPattern, ExeSearchPattern);
    Plt->Exe64SearchPattern=CopyStr(Plt->Exe64SearchPattern, Exe64SearchPattern);

    ListAddNamedItem(Platforms, Names, Plt);

    return(Plt);
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
            ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
        }

        ListAddNamedItem(Platforms, Aliases, Plt);
    }

    Destroy(Name);
    Destroy(Value);
    Destroy(Aliases);

    return(Plt);
}




TPlatform *PlatformFind(const char *Name)
{
    ListNode *Curr;
    char *Token=NULL;
    const char *ptr;

    Curr=ListGetNext(Platforms);
    while (Curr)
    {
        ptr=GetToken(Curr->Tag, ",", &Token, 0);
        while (ptr)
        {
            if (strcasecmp(Token, Name)==0)
            {
                Destroy(Token);
                return(Curr->Item);
            }
            ptr=GetToken(ptr, ",", &Token, 0);
        }

        Curr=ListGetNext(Curr);
    }

    return(NULL);
}


void PlatformsList()
{
    ListNode *Curr;
    TPlatform *Plat;
    char *EmuList=NULL, *Name=NULL;
    const char *ptr;


    Curr=ListGetNext(Platforms);
    while (Curr)
    {
        Plat=(TPlatform *) Curr->Item;
        ptr=GetToken(Curr->Tag, ",", &Name, 0);
        EmuList=PlatformFindEmulatorNames(EmuList, Name);
        printf("%-15s  aliases: %-20s   emulators: %s\n", Name, ptr, EmuList);
        Curr=ListGetNext(Curr);
    }

    Destroy(EmuList);
    Destroy(Name);
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




char *PlatformFindEmulator(char *RetStr, char *PlatformName)
{
    char *Tempstr=NULL, *Emulators=NULL, *EmuInvoke=NULL, *EmuName=NULL;
    const char *ptr;

//seems a bit strange, but the idea here is that we'll return NULL
//if an emulator isn't found, but return empty string if one isn't needed
    Destroy(RetStr);
    RetStr=NULL;

    Emulators=PlatformLookupInfo(Emulators, PlatformName, PLATFORM_INFO_EMULATORS);
    if (Emulators)
    {
        if (! StrValid(Emulators)) RetStr=CopyStr(RetStr, "");
        else switch (PlatformType(PlatformName))
            {
            case PLATFORM_LINUX32:
            case PLATFORM_LINUX64:
                RetStr=CopyStr(RetStr, Emulators);
                break;

            default:
                ptr=GetToken(Emulators, ",", &EmuInvoke, 0);
                while (ptr)
                {
                    GetToken(EmuInvoke, "\\S", &EmuName, 0);
                    Tempstr=FindFileInPath(Tempstr, EmuName, getenv("PATH"));
                    if (StrValid(Tempstr))
                    {
                        //don't copy the executable name, copy the entire emulator invokaction
                        RetStr=CopyStr(RetStr, EmuInvoke);
                        break;
                    }
                    ptr=GetToken(ptr, ",", &EmuInvoke, 0);
                }
                break;
            }
    }

    Destroy(Emulators);
    Destroy(EmuName);
    Destroy(Tempstr);
    Destroy(EmuInvoke);

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




void PlatformApplySettings(TAction *Act)
{
    switch (Act->PlatformID)
    {
    case PLATFORM_WINDOWS:
        RegEditApplySettings(Act);
        break;

    case PLATFORM_DOOM:
        DoomApplySettings(Act);
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


