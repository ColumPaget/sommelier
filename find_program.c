#include "find_program.h"
#include "config.h"
#include "platforms.h"
#include "find_files.h"

static char *FindProgramGoFishing(char *RetStr, TAction *Act, int IgnoreBitWidth)
{
    ListNode *Exes, *Curr;
    char *Tempstr=NULL, *SearchPatterns=NULL, *IgnorePatterns=NULL;
    const char *ptr;
    int BitWidth=0;


    RetStr=CopyStr(RetStr, "");
    Exes=ListCreate();
    BitWidth=PlatformBitWidth(Act->Platform);


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

    if (BitWidth == 64) Tempstr=PlatformGetExe64SearchPattern(Tempstr, Act->Platform);
    else Tempstr=PlatformGetExeSearchPattern(Tempstr, Act->Platform);

    SearchPatterns=CatStr(SearchPatterns, Tempstr);

    //do the actual 'Find'
    if (IgnoreBitWidth) FindItems(GetVar(Act->Vars, "drive_c"), SearchPatterns, IgnorePatterns, 0, Exes);
    else FindItems(GetVar(Act->Vars, "drive_c"), SearchPatterns, IgnorePatterns, BitWidth, Exes);

    if (Config->Flags & FLAG_DEBUG) fprintf(stderr, "Find: [%s] [%s] [%s]\n", GetVar(Act->Vars, "drive_c"), SearchPatterns, IgnorePatterns);

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



//this won't be called for downloads that just download an exectuable
//it will only be called for downloads where we have to actually search
//for the executable
static char *FindProgramSearchInstallDir(char *Exec, TAction *Act)
{
    char *Tempstr=NULL;
    const char *ptr;

    ptr=GetVar(Act->Vars, "exec");
    if (StrValid(ptr)) Exec=SubstituteVarsInString(Exec, ptr, Act->Vars, 0);

    //full path given to executable, so it must exist at this path, but we have to
    //rationalized the path to the install-dir that it's under
    if (StrValid(Exec) && (*Exec == '/'))
    {
        ptr=GetVar(Act->Vars, "install-dir");
        if (StrValid(ptr))
        {
            Tempstr=MCopyStr(Tempstr, ptr, "/", Exec, NULL);
            Exec=CopyStr(Exec, Tempstr);
        }
        //do nothing
    }
    //full path not given, we have to go searching for a matching application file
    else
    {
        Exec=FindProgramGoFishing(Exec, Act, FALSE);
        if (! StrValid(Exec))
        {
            //Try Again ignoring bitwidth of program
            Exec=FindProgramGoFishing(Exec, Act, TRUE);

            if (StrValid(Exec))
            {
                Tempstr=MCopyStr(Tempstr, "~y~eProgram Found, but has wrong bit-width for system.~w  Using '", Exec, "' in hope this is a multilib/multiarch system~0\n", NULL);
                TerminalPutStr(Tempstr, NULL);
            }
        }
    }

    return(Exec);
}


//did we find the executable, does it have appropriate permissions?
//if it exists but lacks permissions, try to add them, but even if we can't set them
//count it as the exectuable anyway
static int CheckExecutable(TAction *Act, const char *Exec)
{
    char *Tempstr=NULL;
    struct stat Stat;
    int RetVal=FALSE;

    if (! StrValid(Exec)) return(FALSE);

    if (access(Exec, F_OK) == 0)
    {
        RetVal=TRUE;

        //we will chmod downloaded executables at the end of install anyways
        if ( (Act->InstallType == INSTALL_EXECUTABLE) || (access(Exec, X_OK) == 0) )
        {
            Tempstr=MCopyStr(Tempstr, "~g~eFound Program: ~w", Exec, "~0\n", NULL);
            TerminalPutStr(Tempstr, NULL);
        }
        else
        {
            Tempstr=MCopyStr(Tempstr, "~y~eWARN: ~w Found Program: '", Exec, "' but the file lacks execute permissions~0\nTrying to set permissions... ", NULL);
            if (chmod(Exec, 0770)==0)  Tempstr=CatStr(Tempstr, "~gPemissions set~0\n");
            else Tempstr=CatStr(Tempstr, "~rFAILED~0\n");
            TerminalPutStr(Tempstr, NULL);
        }
    }

    Destroy(Tempstr);

    return(RetVal);
}



/*
Try to find the actual executable that we'll run with 'sommelier'
*/
char *FindProgram(char *RetStr, TAction *Act)
{
    char *Exec=NULL, *Tempstr=NULL;
    const char *ptr;


    RetStr=CopyStr(RetStr, "");

    //if we are installing a downloaded executable then the downloaded file is the actual exectuable
    //and we don't need to go searching
    if (Act->InstallType == INSTALL_EXECUTABLE)
    {
        Exec=CopyStr(Exec, Act->SrcPath);

        //if we don't have a full path to the downloaded executable
        //then we are likely being called as 'reconfigure' or some other
        //action that doesn't require/include a download. So we're going
        //to have to 'guess' the exec name from the URL, and 'go fishing'
        if (! StrValid(Exec))
        {
            Tempstr=URLBasename(Tempstr, Act->URL);
            SetVar(Act->Vars, "exec", Tempstr);
        }
    }
    //otherwise we need to search through the unpacked item we've downloaded
    else Exec=FindProgramSearchInstallDir(Exec, Act);

    if (CheckExecutable(Act, Exec)) RetStr=CopyStr(RetStr, Exec);

    Destroy(Tempstr);
    Destroy(Exec);

    return(RetStr);
}

