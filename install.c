#include "install.h"
#include "desktopfiles.h"
#include "regedit.h"
#include "download.h"
#include "apps.h"
#include "platforms.h"
#include "packages.h"
#include "find_files.h"
#include "find_program.h"
#include "config.h"
#include "native.h"





static void RunInstallers(TAction *Act)
{
    const char *ptr;
    char *Tempstr=NULL, *Cmd=NULL, *CmdConfig=NULL;
    int RegFlags=0;

    if (Config->Flags & FLAG_DEBUG) CmdConfig=CopyStr(CmdConfig, "+stderr");
    else CmdConfig=CopyStr(CmdConfig, "outnull");

    ptr=GetVar(Act->Vars, "installer-vdesk");
    if (StrValid(ptr))
    {
        if (Config->Flags & FLAG_DEBUG) printf("Running installer in a virtual desktop\n");
        RegFlags |= REG_VDESK;
    }

    if (RegFlags) RegEdit(Act, RegFlags, NULL, NULL, NULL);

    ptr=GetVar(Act->Vars, "installer-path");
    printf("INSTALLER PATH: %s\n", ptr);
    if (StrValid(ptr))
    {
        switch (Act->PlatformID)
        {
        case PLATFORM_DOS:
            Tempstr=SubstituteVarsInString(Tempstr, "dosbox '$(installer-path)' $(installer-args)", Act->Vars, 0);
            printf("RUN INSTALLER: %s\n",Tempstr);
            Cmd=SubstituteVarsInString(Cmd, Tempstr, Act->Vars, 0);
            RunProgramAndConsumeOutput(Cmd, CmdConfig);
            break;

        case PLATFORM_WINDOWS:
        //windos has windows installer but msdos (Dosbox) executable
        case PLATFORM_GOGWINDOS:
            ptr=strrchr(ptr, '.');
            if (ptr && (strcmp(ptr, ".msi")==0)) Tempstr=SubstituteVarsInString(Tempstr, "WINEPREFIX=$(prefix) WINEDLLOVERRIDES=\"mscoree,mshtml=\" wine msiexec /i '$(installer-path)' $(installer-args)", Act->Vars, 0);
            else Tempstr=SubstituteVarsInString(Tempstr, "WINEPREFIX=$(prefix) WINEDLLOVERRIDES=\"mscoree,mshtml=\" wine '$(installer-path)' $(installer-args)", Act->Vars, 0);

            printf("RUN INSTALLER: %s    in %s\n",Tempstr, get_current_dir_name());
            Cmd=SubstituteVarsInString(Cmd, Tempstr, Act->Vars, 0);
            RunProgramAndConsumeOutput(Cmd, CmdConfig);
            break;
        }
    }

    ptr=GetVar(Act->Vars, "install_stage2");
    if (StrValid(ptr))
    {
        Tempstr=FindSingleFile(Tempstr, GetVar(Act->Vars, "prefix"), ptr);
        SetVar(Act->Vars, "installer-path", Tempstr);
        Cmd=SubstituteVarsInString(Cmd, "WINEPREFIX=$(prefix) wine '$(installer-path)'", Act->Vars, 0);
        printf("RUN INSTALL STAGE2: %s\n", Cmd);
        RunProgramAndConsumeOutput(Cmd, CmdConfig);
    }

    if (Act->PlatformID==PLATFORM_WINDOWS)
    {
        if (RegFlags & REG_VDESK)
        {
            RegFlags &= ~REG_VDESK;
            RegFlags |= REG_NO_VDESK;
        }
        RegFlags |= REG_FONT_SMOOTH;
        RegEdit(Act, RegFlags, NULL, NULL, NULL);
    }

    Destroy(CmdConfig);
    Destroy(Tempstr);
    Destroy(Cmd);
}



// This function installs from a downloaded file. This will either mean unzipping a downloaded .zip file
// or running an installer .exe or .msi file.
// Finding the resulting exe that we are going to run when we start the app is done in 'FinalizeExeInstall'
static int InstallAppFromFile(TAction *Act, const char *Path)
{
    char *Tempstr=NULL, *FilesToExtract=NULL;
    const char *ptr;
    int ForcedFileType=FILETYPE_UNKNOWN;

    switch (Act->PlatformID)
    {
    case PLATFORM_SCUMMVM:
        ForcedFileType=FILETYPE_ZIP;
        break;

    case PLATFORM_GOGLINUX64:
        if (strcmp(PlatformDefault(), "!linux64")==0)
        {
            Tempstr=MCopyStr(Tempstr, "~rWARN: Program to be installed may be 64-bit only, and you seem to be installing it on a 32-bit linux system.~0\n", NULL);
            TerminalPutStr(Tempstr, NULL);
        }

    case PLATFORM_GOGLINUX:
        ForcedFileType=FILETYPE_ZIP;
        FilesToExtract=SubstituteVarsInString(FilesToExtract, "data/noarch/game/* $(extra-files)", Act->Vars, 0);
        break;

    case PLATFORM_GOGSCUMMVM:
    case PLATFORM_GOGDOS:
        ForcedFileType=FILETYPE_ZIP;
        FilesToExtract=SubstituteVarsInString(FilesToExtract, "data/noarch/data/* $(extra-files)", Act->Vars, 0);
        break;
    }

    PackageUnpack(Act, Path, ForcedFileType, FilesToExtract);
    //if there's a package within the package, this will unpack it
    PackageUnpackInner(Act, Path, ForcedFileType, FilesToExtract);

    //if the package contained an installer program within it then  we run that
    ptr=GetVar(Act->Vars, "installer-path");
    if (StrValid(ptr)) RunInstallers(Act);

    Destroy(Tempstr);
    Destroy(FilesToExtract);

    return(TRUE);
}






static void PostProcessInstall(TAction *Act)
{
    glob_t Glob;
    char *From=NULL, *To=NULL, *Tempstr=NULL, *Value=NULL;
    const char *ptr;
    int i, result;

    ptr=GetVar(Act->Vars, "delete");
    if (StrValid(ptr))
    {
        Tempstr=SubstituteVarsInString(Tempstr, ptr, Act->Vars, 0);
        ptr=GetToken(Tempstr, ",", &Value, GETTOKEN_QUOTES);
        while (ptr)
        {
            glob(Value, 0, 0, &Glob);
            for (i=0; i < Glob.gl_pathc; i++)
            {
                if (Config->Flags & FLAG_DEBUG) printf("DELETE: %s\n", Glob.gl_pathv[i]);
                unlink(Glob.gl_pathv[i]);
            }
            ptr=GetToken(ptr, ",", &Value, GETTOKEN_QUOTES);
        }
    }

    ptr=GetVar(Act->Vars, "movefiles-from");
    if (StrValid(ptr))
    {
        From=SubstituteVarsInString(From, ptr, Act->Vars, 0);
        glob(From, 0, 0, &Glob);
        for (i=0; i < Glob.gl_pathc; i++)
        {
            if (Config->Flags & FLAG_DEBUG) printf("MOVE: %s\n", Glob.gl_pathv[i]);
            rename(Glob.gl_pathv[i], GetBasename(Glob.gl_pathv[i]));
        }
    }

    ptr=GetVar(Act->Vars, "movefiles-to");
    if (StrValid(ptr))
    {
        ptr=GetToken(ptr, ":", &Tempstr, 0);
        From=SubstituteVarsInString(From, Tempstr, Act->Vars, 0);

        To=SubstituteVarsInString(To, ptr, Act->Vars, 0);
        To=SlashTerminateDirectoryPath(To);

        if (Config->Flags & FLAG_DEBUG) printf("movefiles-to: [%s] [%s]\n", From, To);
        MakeDirPath(To, 0766);

        glob(From, 0, 0, &Glob);
        for (i=0; i < Glob.gl_pathc; i++)
        {
            Tempstr=MCopyStr(Tempstr, To, "/", GetBasename(Glob.gl_pathv[i]), NULL);
            if (Config->Flags & FLAG_DEBUG) printf("MOVE: %s\n", Glob.gl_pathv[i]);
            rename(Glob.gl_pathv[i], Tempstr);
        }
    }

    ptr=GetVar(Act->Vars, "rename");
    if (StrValid(ptr))
    {
        Tempstr=SubstituteVarsInString(Tempstr, ptr, Act->Vars, 0);
        ptr=GetToken(Tempstr, "\\S", &Value, GETTOKEN_QUOTES);
        From=UnQuoteStr(From, Value);
        ptr=GetToken(ptr, "\\S", &Value, GETTOKEN_QUOTES);
        To=UnQuoteStr(To, Value);

        if (Config->Flags & FLAG_DEBUG) printf("RENAME: '%s' -> '%s'\n", From, To);
        result=rename(From, To);
    }



    ptr=GetVar(Act->Vars, "winmanager");
    if (StrValid(ptr)) RegEdit(Act, REG_NO_WINMANAGER, NULL, NULL, NULL);


    Destroy(Tempstr);
    Destroy(Value);
    Destroy(From);
    Destroy(To);
}


void InstallFindIcon(TAction *Act)
{
    ListNode *Founds, *Curr;

    if (StrValid(GetVar(Act->Vars, "app-icon"))) return;

    Founds=ListCreate();
    FindFiles(GetVar(Act->Vars, "drive_c"), GetVar(Act->Vars, "icon"), "", Founds);
    Curr=ListGetNext(Founds);
    if (Curr)
    {
        SetVar(Act->Vars, "app-icon", Curr->Item);
    }
    ListDestroy(Founds, Destroy);
}



void InstallCheckEnvironment(TAction *Act)
{
    const char *ptr;
    char *Path=NULL, *Tempstr=NULL, *Libs=NULL;

    switch (Act->PlatformID)
    {
    case PLATFORM_LINUX32:
    case PLATFORM_LINUX64:
        NativeExecutableCheckLibs(GetVar(Act->Vars, "exec-path"), &Libs);
        if (StrValid(Libs))
        {
            Tempstr=FormatStr(Tempstr, "~yWARN: Executable requires missing libraries:~0 '%s'. Sommelier will attempt to find substitutes at runtime.\n", Libs);
            TerminalPutStr(Tempstr, NULL);
        }
        break;
    }

    ptr=GetVar(Act->Vars, "warn-missingpath");
    if (StrValid(ptr))
    {
        ptr=GetToken(ptr, ":", &Path, 0);
        if (access(Path, F_OK) !=0)
        {
            Tempstr=MCopyStr(Tempstr, "~rWARN: ", ptr, "~0\n", NULL);
            TerminalPutStr(Tempstr, NULL);
        }
    }

    Destroy(Tempstr);
    Destroy(Path);
    Destroy(Libs);
}



char *OffsetPathFromDir(char *Path, const char *Dir)
{
    char *Tempstr=NULL;
    int len;

    len=StrLen(Dir);
    if ( (len > 0) && (strncmp(Path, Dir, len)==0) )
    {
        Tempstr=CopyStr(Tempstr, Path+len);
        Path=CopyStr(Path, Tempstr);
    }

    return(Path);
}


/*
For DOS and windows executables that we might have downloaded as a Zip or an MSI file, we finalize here and setup
the actual program that we're going to run to execute the application
*/
static void FinalizeExeInstall(TAction *Act)
{
    char *Path=NULL, *Tempstr=NULL, *ExecDir=NULL, *WorkDir=NULL;
    const char *ptr;
    int len;

    PostProcessInstall(Act);

    Path=FindProgram(Path, Act);
    SetVar(Act->Vars, "exec-path", Path);

    //if no exec-dir is set, then set this to the path that we found the executable in
    ExecDir=CopyStr(ExecDir, GetVar(Act->Vars,"exec-dir"));
    if (! StrValid(ExecDir))
    {
        ExecDir=CopyStr(ExecDir, Path);
        StrRTruncChar(ExecDir, '/');
        SetVar(Act->Vars, "exec-dir", ExecDir);
    }

    switch (Act->PlatformID)
    {
    case PLATFORM_WINDOWS:
        //reconfigure path to be from our wine 'drive_c' rather than from system root
        Path=OffsetPathFromDir(Path, GetVar(Act->Vars, "drive_c"));

        Tempstr=CopyStr(Tempstr, ExecDir);
        Tempstr=OffsetPathFromDir(Tempstr, GetVar(Act->Vars, "drive_c"));
        SetVar(Act->Vars, "exec-dir", Tempstr);
        SetVar(Act->Vars, "working-dir", ExecDir);
        break;

    case PLATFORM_GOGSCUMMVM:
        Path=MCopyStr(Path, GetVar(Act->Vars, "install-dir"), "/data/noarch/data", NULL);
        SetVar(Act->Vars, "working-dir", Path);
        break;

    default:
        //is a working dir set in the app config?
        Tempstr=CopyStr(Tempstr, GetVar(Act->Vars, "working-dir"));

        //if not set in the app config, is a working dir set in the platform config?
        if (! StrValid(Tempstr)) Tempstr=PlatformGetWorkingDir(Tempstr, Act->Platform);

        //if still no joy, then assume that the working dir is the 'exec dir' (where we found the executable)
        if (! StrValid(Tempstr)) Tempstr=CopyStr(Tempstr, GetVar(Act->Vars, "exec-dir"));

        if (StrValid(Tempstr))
        {
            WorkDir=SubstituteVarsInString(WorkDir, Tempstr, Act->Vars, 0);
            SetVar(Act->Vars, "working-dir", WorkDir);
            if (Act->Flags & FLAG_NOEXEC) Path=CopyStr(Path, WorkDir);
        }
        break;
    }


    if (StrValid(Path))
    {
        //Tempstr=QuoteCharsInStr(Tempstr, GetBasename(Path), " 	");
        if (! StrValid(GetVar(Act->Vars, "exec")) ) SetVar(Act->Vars, "exec", GetBasename(Path));

        InstallCheckEnvironment(Act);
        //must do this before DesktopFileGenerate, because some of the settings for some platforms are stored in the
        //desktop file as command-line args passed to the emulator
        PlatformApplySettings(Act);

        InstallFindIcon(Act);


        if (! (Act->Flags & FLAG_DEPENDANCY)) DesktopFileGenerate(Act);
    }
    else
    {
        Tempstr=MCopyStr(Tempstr, "~rERROR: Failed to find executable for ", Act->Name, "~0\n", NULL);
        TerminalPutStr(Tempstr, NULL);
    }

    Destroy(Tempstr);
    Destroy(WorkDir);
    Destroy(ExecDir);
    Destroy(Path);
}


static void InstallSingleItemPreProcessInstall(TAction *Act)
{
    char *Token=NULL, *Tempstr=NULL;
    const char *ptr;

    if (StrValid(Act->InstallName))
    {
        Act->Name=CopyStr(Act->Name, Act->InstallName);
    }

//if package supports both 32 and 64 bit architectures (or only 64, but the package type could support 32-bit, as with GOG games) then if we are running on a 64bit architecture we want to use the
//64bit version
    ptr=GetVar(Act->Vars, "exec64");
    if (StrValid(ptr))
    {
        if (PlatformBitWidth(Act->Platform)==64)
        {
            SetVar(Act->Vars, "exec", ptr);
            printf("EXEC64: %s\n", ptr);
        }
    }


//dll-overrides creates a wine bottle with certain dlls missing, forcing the installer to install it's own
//this is for sitations where the installer will supply a better choice of DLL for this app than wine's own
    ptr=GetVar(Act->Vars, "dll-overrides");
    if (StrValid(ptr))
    {
        //run fake installer to ensure a wine bottle is created, so that
        //we can clear DLLs out of it before doing the install
        SetVar(Act->Vars, "installer", "this-does-not-exist.exe");
        RunInstallers(Act);

        //go through list of DLLs, deleting them so that the real installer, when we run it, will install it's own
        ptr=GetToken(ptr, ",", &Token, 0);
        while (ptr)
        {
            SetVar(Act->Vars, "delete-dll", Token);
            Tempstr=SubstituteVarsInString(Tempstr, "$(drive_c)/windows/system32/$(delete-dll).dll", Act->Vars, 0);
            unlink(Tempstr);
            ptr=GetToken(ptr, ",", &Token, 0);
        }
    }

    Destroy(Tempstr);
    Destroy(Token);
}



static void InstallBundledItems(TAction *Parent)
{
    char *Name=NULL;
    const char *ptr;
    ListNode *Node;
    TAction *Child;

    ptr=GetToken(GetVar(Parent->Vars, "bundles"), " ", &Name, 0);
    while (ptr)
    {
        Node=ListFindNamedItem(AppsGetList(), Name);
        if (Node)
        {
            Child=(TAction *) Node->Item;
            SetVar(Child->Vars, "drive_c", GetVar(Parent->Vars, "drive_c"));
            SetVar(Child->Vars, "prefix", GetVar(Parent->Vars, "prefix"));
            FinalizeExeInstall(Child);
        }
        ptr=GetToken(ptr, " ", &Name, 0);
    }

    Destroy(Name);
}


static void InstallSingleItem(TAction *Act)
{
    char *Path=NULL, *InstallPath=NULL, *Tempstr=NULL;
    int InstallResult=FALSE, exitval;
    pid_t pid;

//Fork so we don't chdir current app
    pid=fork();
    if (pid==0)
    {
        InstallSingleItemPreProcessInstall(Act);

        Tempstr=CopyStr(Tempstr, GetVar(Act->Vars, "unpack-dir"));
        InstallPath=SubstituteVarsInString(InstallPath, Tempstr, Act->Vars, 0);
        if (! StrValid(InstallPath)) InstallPath=CopyStr(InstallPath, GetVar(Act->Vars, "install-dir"));
        mkdir(InstallPath, 0700);
        chdir(InstallPath);

        if (Download(Act)==0) TerminalPutStr("~r~eERROR: Download Failed, '0' bytes received!~0\n", NULL);
        else
        {
            TerminalPutStr("~eValidating download~0\n", NULL);

            if ( Act->Flags & FLAG_FORCE) printf("install forced, not validating download\n");
            else if (! CompareSha256(Act))
            {
                TerminalPutStr("~r~eERROR: Download Hash mismatch!~0\n", NULL);
                TerminalPutStr("File we downloaded was not the one we expected. Run with -force if you want to risk it.\n", NULL);
                Act->Flags |= FLAG_ABORT;
            }
        }

        if (! (Act->Flags & FLAG_ABORT))
        {
            InstallResult=InstallAppFromFile(Act, Act->SrcPath);
            if ( InstallResult && (! (Act->Flags & (FLAG_DEPENDANCY | FLAG_DLC))) )
            {
                TerminalPutStr("~eFinding executables~0\n", NULL);
                FinalizeExeInstall(Act);
                InstallBundledItems(Act);
            }
        }


        //make sure executables are... executable
        if (Act->InstallType == INSTALL_EXECUTABLE) chmod(Act->SrcPath, 0770);
        else if (
            (! (Act->Flags & FLAG_KEEP_INSTALLER)) &&
            (Act->Flags & FLAG_DOWNLOADED)
        )
        {
            unlink(Act->SrcPath);
        }

        if (Act->Flags & FLAG_ABORT) _exit(1);
        PostProcessInstall(Act);

        _exit(0);
    }

    waitpid(pid, &exitval, 0);
    if (exitval==1)
    {
        Act->Flags |= FLAG_ABORT;
        Tempstr=MCopyStr(Tempstr, "~r~eInstall Aborted for ~0", Act->Name, "\n", NULL);
        TerminalPutStr(Tempstr, NULL);
    }


    DestroyString(Tempstr);
    DestroyString(InstallPath);
    DestroyString(Path);
}



static int InstallDependancy(TAction *Parent, const char *Name)
{
    TAction *Dependancy;

    Dependancy=ActionCreate(ACT_INSTALL, Name);
    if (Dependancy)
    {
        Dependancy->Flags |= FLAG_DEPENDANCY;
        CopyVars(Dependancy->Vars, Parent->Vars);
        //do AppLoadConfig after copy vars, so vars set in dependancy config
        //override those inherited from calling app
        if (AppLoadConfig(Dependancy)) InstallSingleItem(Dependancy);
        ActionDestroy(Dependancy);
    }

    return(TRUE);
}


static char *InstallStandardDependancies(char *RetStr)
{
    TAction *Act;
    char *Tempstr=NULL;

    Act=ActionCreate(ACT_INSTALL, "StandardDependancies");
    Act->Platform=CopyStr(Act->Platform, "windows");
    Tempstr=AppFormatPath(Tempstr,  Act);
    if (access(Tempstr, F_OK) !=0)
    {
        Act->Flags |= FLAG_DEPENDANCY;
        printf("Running wine to download/install standard dependancies (mono, gecko, etc)\n");
        MakeDirPath(Tempstr, 0700);
        SetVar(Act->Vars, "installer", "this-does-not-exist.exe");
        RunInstallers(Act);
    }
    RetStr=CopyStr(RetStr, GetVar(Act->Vars, "prefix"));

    Destroy(Tempstr);

    return(RetStr);
}



static void InstallSetupWindowsDependancies(TAction *Act)
{
    char *Tempstr=NULL, *StdDepsPath=NULL, *Path=NULL;

    StdDepsPath=InstallStandardDependancies(StdDepsPath);
    if (StrValid(StdDepsPath))
    {
        Tempstr=MCopyStr(Tempstr, StdDepsPath, "/drive_c/windows/mono/mono-2.0", NULL);
        Path=MCopyStr(Path, GetVar(Act->Vars, "prefix"), "/drive_c/windows/mono/mono-2.0", NULL);
        MakeDirPath(Path, 0700);
        symlink(Tempstr, Path);

        Tempstr=MCopyStr(Tempstr, StdDepsPath, "/drive_c/windows/system32/gecko", NULL);
        Path=MCopyStr(Path, GetVar(Act->Vars, "prefix"), "/drive_c/windows/system32/gecko", NULL);
        MakeDirPath(Path, 0700);
        symlink(Tempstr, Path);
    }

    Destroy(StdDepsPath);
    Destroy(Tempstr);
    Destroy(Path);
}


static int InstallRequiredDependancies(TAction *Act)
{
    const char *ptr, *p_Requires;
    char *Name=NULL;

    p_Requires=GetVar(Act->Vars, "requires");
    if (p_Requires)
    {
        ptr=GetToken(p_Requires,",",&Name,0);
        while (ptr)
        {
            printf("Installing dependancy: '%s'\n", Name);
            InstallDependancy(Act, Name);

            ptr=GetToken(ptr,",",&Name,0);
        }
    }
    Destroy(Name);
    return(TRUE);
}


static int CheckDLC(TAction *Act)
{
    TAction *Parent;
    int result=FALSE;

    Parent=ActionCreate(ACT_INSTALL, Act->Parent);
    result=DesktopFileRead(Parent);
    ActionDestroy(Parent);

    return(result);
}


void InstallApp(TAction *Act)
{
    const char *ptr;
    char *Name=NULL, *Path=NULL, *Tempstr=NULL, *Emulator=NULL;

    if (StrValid(Act->InstallName)) Tempstr=MCopyStr(Tempstr, "\n~e##### Installing ", Act->Name, " as ", Act->InstallName, " #########~0\n", NULL);
    else Tempstr=MCopyStr(Tempstr, "\n~e##### Installing ", Act->Name, " #########~0\n", NULL);
    TerminalPutStr(Tempstr, NULL);
    Tempstr=CopyStr(Tempstr, "");


    if (! StrValid(Act->Platform))
    {
        TerminalPutStr("~r~eERROR: no platform configured for this application~0 Cannot install.\n", NULL);
    }
    else if (Act->PlatformID==PLATFORM_UNKNOWN)
    {
        Tempstr=FormatStr(Tempstr, "~r~eERROR: Unknown platform type '%s'~0 Cannot install.\n", Act->Platform);
        TerminalPutStr(Tempstr, NULL);
    }
    else if ( (Act->Flags & FLAG_DLC) && (! CheckDLC(Act)))
    {
        Tempstr=FormatStr(Tempstr, "~r~eERROR: '%s' is DLC for '%s', but parent package is not installed.~0\n", Act->Name, Act->Parent);
        TerminalPutStr(Tempstr, NULL);
    }
    else
    {
        //is an emulator installed for this platform? NULL means one is required by can't be found,
        //empty string means none is required
        Emulator=PlatformFindEmulator(Emulator, Act->Platform);

        if (StrValid(Emulator))
        {
            Tempstr=MCopyStr(Tempstr, "~gFound suitable emulator '", Emulator, "'~0\n", NULL);
            SetVar(Act->Vars, "emulator", Emulator);
        }
        else if (! Emulator)
        {
            Emulator=PlatformFindEmulatorNames(Emulator, Act->Platform);
            Tempstr=MCopyStr(Tempstr, "\n~rWARN: No emulator found for platform '", Act->Platform, "'~0\n", NULL);
            Tempstr=MCatStr(Tempstr, "Please install one of: '", Emulator, "'\n", NULL);
        }


        Name=PlatformGetInstallMessage(Name, Act->Platform);
        if (StrValid(Name)) Tempstr=MCatStr(Tempstr, "\n~r", Name, "~0\n", NULL);

        ptr=GetVar(Act->Vars, "warn");
        if (StrValid(ptr)) Tempstr=MCatStr(Tempstr, "\n~rWARN: ", ptr, "~0\n", NULL);

        TerminalPutStr(Tempstr, NULL);


        Path=AppFormatPath(Path, Act);
        MakeDirPath(Path, 0700);

        if (Act->PlatformID==PLATFORM_WINDOWS) InstallSetupWindowsDependancies(Act);
        InstallRequiredDependancies(Act);

        InstallSingleItem(Act);
        Tempstr=MCopyStr(Tempstr, "~e~g", Act->Name, " install complete~0\n", NULL);
        ptr=GetVar(Act->Vars, "donate");
        if (StrValid(ptr)) Tempstr=MCatStr(Tempstr, "~y~eDONATE~0: if you like this software, please consider donating at ~e", ptr, "~0\n", NULL);
        TerminalPutStr(Tempstr, NULL);
    }

    Destroy(Emulator);
    Destroy(Tempstr);
    Destroy(Path);
    Destroy(Name);
}



void InstallReconfigure(TAction *Act)
{
    char *Name=NULL, *Path=NULL, *Tempstr=NULL;

    Tempstr=MCopyStr(Tempstr, "\n~e##### Reconfigure ", Act->Name, " #########~0\n", NULL);
    TerminalPutStr(Tempstr, NULL);

    if (! StrValid(Act->Platform))
    {
        TerminalPutStr("~r~eERROR: no platform configured for this application~0\n", NULL);
    }
    else if (Act->PlatformID==PLATFORM_UNKNOWN)
    {
        Tempstr=FormatStr(Tempstr, "~r~eERROR: Unknown platform type '%s'~0\n", Act->Platform);
        TerminalPutStr(Tempstr, NULL);
    }
    else
    {
//is an emulator installed for this platform? NULL means one is required by can't be found,
//empty string means none is required
        Tempstr=PlatformFindEmulator(Tempstr, Act->Platform);

        if (StrValid(Tempstr)) Name=MCatStr(Name, "Found suitable emulator '", Tempstr, "'\n", NULL);
        else if (! Tempstr)
        {
            Name=MCatStr(Name, "\n~rWARN: No emulator found for platform '", Act->Platform, "'~0\n", NULL);
            Tempstr=PlatformFindEmulatorNames(Tempstr, Act->Platform);

            Name=MCatStr(Name, "Please install one of: '", Tempstr, "'\n", NULL);
        }


        Path=AppFormatPath(Path, Act);
        MakeDirPath(Path, 0700);

        InstallSingleItemPreProcessInstall(Act);
        Path=CopyStr(Path, GetVar(Act->Vars, "install-dir"));

        chdir(Path);
        TerminalPutStr("~eFinding executables~0\n", NULL);
        FinalizeExeInstall(Act);
        InstallBundledItems(Act);

        printf("%s reconfigure complete\n", Act->Name);
    }

    Destroy(Tempstr);
    Destroy(Path);
}


