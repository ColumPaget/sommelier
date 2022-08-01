#include "find_program.h"
#include "config.h"
#include "platforms.h"

static char *FindProgramGoFishing(char *RetStr, TAction *Act)
{
    ListNode *Exes, *Curr;
    char *Tempstr=NULL, *SearchPatterns=NULL, *IgnorePatterns=NULL;
    const char *ptr;


    RetStr=CopyStr(RetStr, "");
    Exes=ListCreate();

//this needs to be configured in 'platforms.c' eventually
    IgnorePatterns=MCopyStr(IgnorePatterns, "setup*.exe,unin*.exe,dosbox.exe,crash*.exe,",NULL);


//if we are not downloading and installing a straightforward executable then we are likely
//downloading some kind of installer which we don't want to 'find' in the search process and
//confuse with the real executable
    if (Act->InstallType != INSTALL_EXECUTABLE )
    {
        ptr=GetVar(Act->Vars, "dlfile");
        if (StrValid(ptr)) IgnorePatterns=MCatStr(IgnorePatterns, ptr, ",", NULL);

        ptr=GetVar(Act->Vars,"installer");
        if (StrValid(ptr)) IgnorePatterns=MCatStr(IgnorePatterns, ptr, ",", NULL);
    }

    ptr=GetVar(Act->Vars, "exec");
    SearchPatterns=MCopyStr(SearchPatterns, ptr, ",", NULL);

    if (PlatformBitWidth(Act->Platform)==64) Tempstr=PlatformGetExe64SearchPattern(Tempstr, Act->Platform);
    else Tempstr=PlatformGetExeSearchPattern(Tempstr, Act->Platform);

    SearchPatterns=CatStr(SearchPatterns, Tempstr);

    FindFiles(GetVar(Act->Vars, "drive_c"), SearchPatterns, IgnorePatterns, Exes);
    if (Config->Flags & FLAG_DEBUG) printf("Find: [%s] [%s] [%s]\n", GetVar(Act->Vars, "drive_c"), SearchPatterns, IgnorePatterns);

    Curr=NULL;
    ptr=GetVar(Act->Vars, "exec");
    if (StrValid(ptr)) Curr=ListFindNamedItem(Exes, ptr);
    if (! Curr) Curr=ListGetNext(Exes);

    if (Curr) RetStr=CopyStr(RetStr, Curr->Item);

    ListDestroy(Exes, Destroy);
    DestroyString(SearchPatterns);
    DestroyString(IgnorePatterns);
    DestroyString(Tempstr);

    return(RetStr);
}


/*
Try to find the actual executable that we'll run with 'sommelier'
*/
char *FindProgram(char *RetStr, TAction *Act)
{
    char *Exec=NULL, *Tempstr=NULL;
    const char *ptr;

    ptr=GetVar(Act->Vars, "exec");
    if (StrValid(ptr)) Exec=SubstituteVarsInString(Exec, ptr, Act->Vars, 0);

//full path given to executable, so it must exist at this path
    if (StrValid(Exec) && *Exec == '/')
    {
        //do nothing
    }
    else Exec=FindProgramGoFishing(Exec, Act);

    RetStr=CopyStr(RetStr, Exec);

    if (StrValid(RetStr))
    {
        Tempstr=MCopyStr(Tempstr, "~g~eFound Program: ~w", RetStr, "~0\n", NULL);
        TerminalPutStr(Tempstr, NULL);
    }


    Destroy(Tempstr);
    Destroy(Exec);

    return(RetStr);
}

