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
#include "uninstall.h"


static void RunInstallerForPlatform(TAction *Act, const char *Path, const char *Args)
{
    char *Tempstr=NULL, *Cmd=NULL, *CmdConfig=NULL;
    const char *ptr;

    if (StrValid(Path))
    {
        if (Config->Flags & FLAG_DEBUG) CmdConfig=CopyStr(CmdConfig, "+stderr");
        else CmdConfig=CopyStr(CmdConfig, "outnull");


        SetVar(Act->Vars, "run-installer-path", Path);
        SetVar(Act->Vars, "run-installer-args", Args);
        switch (Act->PlatformID)
        {
        case PLATFORM_DOS:
            Tempstr=SubstituteVarsInString(Tempstr, "dosbox '$(run-installer-path)' $(run-installer-args)", Act->Vars, 0);
            break;

        case PLATFORM_WINDOWS:
        //windos has windows installer but msdos (Dosbox) executable
        case PLATFORM_GOGWINDOS:
            ptr=strrchr(Path, '.');
            if (ptr && (strcmp(ptr, ".msi")==0)) Tempstr=SubstituteVarsInString(Tempstr, "WINEPREFIX=$(prefix) WINEDLLOVERRIDES=\"mscoree,mshtml=\" wine msiexec /i '$(run-installer-path)' $(run-installer-args)", Act->Vars, 0);
            else Tempstr=SubstituteVarsInString(Tempstr, "WINEPREFIX=$(prefix) WINEDLLOVERRIDES=\"mscoree,mshtml=\" wine '$(run-installer-path)' $(run-installer-args)", Act->Vars, 0);
            break;

        default:
            Tempstr=SubstituteVarsInString(Tempstr, "'$(run-installer-path)' $(run-installer-args)", Act->Vars, 0);
            break;
        }

        Cmd=SubstituteVarsInString(Cmd, Tempstr, Act->Vars, 0);
        printf("RUN INSTALLER: %s    in %s\n", Cmd, get_current_dir_name());
        fflush(NULL);
        RunProgramAndConsumeOutput(Cmd, CmdConfig);
    }

    Destroy(CmdConfig);
    Destroy(Tempstr);
    Destroy(Cmd);
}



static void RunInstallers(TAction *Act)
{
    char *Tempstr=NULL, *Path=NULL, *Args=NULL;
    const char *ptr;
    int RegFlags=0;

    //only run main installer if it actually exists
    //even if it doesn't though, there might be a stage-2 installer contained within a zip etc
    ptr=GetVar(Act->Vars, "installer-path");
    if (StrValid(ptr))
    {
        ptr=GetVar(Act->Vars, "installer-vdesk");
        if (StrValid(ptr))
        {
            if (Config->Flags & FLAG_DEBUG) printf("Running installer in a virtual desktop\n");
            RegFlags |= REG_VDESK;
        }

        if (RegFlags) RegEdit(Act, RegFlags, NULL, NULL, NULL);

        Path=CopyStr(Path, GetVar(Act->Vars, "installer-path"));
        Args=CopyStr(Args, GetVar(Act->Vars, "installer-args"));
        RunInstallerForPlatform(Act, Path, Args);
    }

    ptr=GetVar(Act->Vars, "install-stage2");
    if (StrValid(ptr))
    {
        Path=FindSingleFile(Path, GetVar(Act->Vars, "prefix"), ptr);
        Args=CopyStr(Args, GetVar(Act->Vars, "stage2-args"));
        RunInstallerForPlatform(Act, Path, Args);
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

    Destroy(Tempstr);
    Destroy(Path);
    Destroy(Args);
}



// This function installs from a downloaded file. This will either mean unzipping a downloaded .zip file
// or running an installer .exe or .msi file.
// Finding the resulting exe that we are going to run when we start the app is done in 'FinalizeExeInstall'
static int InstallAppFromFile(TAction *Act, const char *Path)
{
    char *Tempstr=NULL, *FilesToExtract=NULL;
    const char *ptr;
    int ForcedFileType=FILETYPE_UNKNOWN;

    if (! StrValid(Path)) return(FALSE);

    ptr=GetVar(Act->Vars, "extract-filter");
    FilesToExtract=SubstituteVarsInString(FilesToExtract, ptr, Act->Vars, 0);

    switch (Act->PlatformID)
    {
    case PLATFORM_LINUX32:
    case PLATFORM_LINUX64:
        ptr=strrchr(Act->SrcPath, '.');
        if (ptr && (strcmp(ptr, ".AppImage")==0)) Act->InstallType=INSTALL_EXECUTABLE;
        break;

    case PLATFORM_SCUMMVM:
        ForcedFileType=FILETYPE_ZIP;
        break;

    case PLATFORM_GOGLINUX64:
        if (strcmp(PlatformDefault(), "!linux64")==0)
        {
            Tempstr=MCopyStr(Tempstr, "~rWARN: Program to be installed may be 64-bit only, and you seem to be installing it on a 32-bit linux system.~0\n", NULL);
            TerminalPutStr(Tempstr, NULL);
        }
    //fall through to 'PLATFORM_GOGLINUX'

    case PLATFORM_GOGLINUX:
    case PLATFORM_GOGSCUMMVM:
    case PLATFORM_GOGDOS:
    case PLATFORM_GOGNEOGEO:
        ForcedFileType=FILETYPE_ZIP;
        if (! StrValid(FilesToExtract)) FilesToExtract=SubstituteVarsInString(FilesToExtract, "data/noarch/data/* data/noarch/game/* data/noarch/scummvm/* data/noarch/docs/* $(extra-files)", Act->Vars, 0);
        FilesToExtract=CatStr(FilesToExtract, " data/noarch/support/icon.png");
        Tempstr=SubstituteVarsInString(Tempstr, "$(install-dir)/data/noarch/support/icon.png", Act->Vars, 0);
        SetVar(Act->Vars, "app-icon", Tempstr);
        break;
    }

    PackageUnpack(Act, Path, ForcedFileType, FilesToExtract);

    //if there's a package within the package, this will unpack it
    PackageUnpackInner(Act, GetVar(Act->Vars, "inner-package"), ForcedFileType, GetVar(Act->Vars, "inner-extract"));
    RunInstallers(Act);

    Destroy(Tempstr);
    Destroy(FilesToExtract);

    return(TRUE);
}



static void PostProcessSetupDirVar(TAction *Act, const char *VarName)
{
    const char *ptr;

    ptr=ResolveVar(Act->Vars, VarName);
    if (StrValid(ptr))
    {
        if (Config->Flags & FLAG_DEBUG) printf("%s: [%s]\n", VarName, ptr);
        MakeDirPath(ptr, 0770);
    }
}


static void PostProcessDelete(TAction *Act, const char *PostProc)
{
    char *Tempstr=NULL, *Value=NULL;
    const char *ptr;
    glob_t Glob;
    int i;

    if (StrValid(PostProc))
    {
        Tempstr=SubstituteVarsInString(Tempstr, PostProc, Act->Vars, 0);
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

    Destroy(Tempstr);
    Destroy(Value);
}


static void PostProcessCopyFilesFrom(TAction *Act, const char *PostProc)
{
    char *Tempstr=NULL, *Value=NULL, *From=NULL;
    const char *ptr;
    glob_t Glob;
    int i;

    if (StrValid(PostProc))
    {
        From=SubstituteVarsInString(From, PostProc, Act->Vars, 0);
        glob(From, 0, 0, &Glob);
        for (i=0; i < Glob.gl_pathc; i++)
        {
            ptr=Glob.gl_pathv[i];
            if (Config->Flags & FLAG_DEBUG) printf("COPY: [%s] [%s]\n", ptr, GetBasename(ptr));
            FileCopy(ptr, GetBasename(ptr));
        }
    }

    Destroy(Tempstr);
    Destroy(Value);
    Destroy(From);
}



static void PostProcessCopyFilesTo(TAction *Act, const char *PostProc)
{
    char *Tempstr=NULL, *Value=NULL, *From=NULL, *To=NULL;
    const char *ptr;
    glob_t Glob;
    int i;

    if (StrValid(PostProc))
    {
        Tempstr=SubstituteVarsInString(Tempstr, PostProc, Act->Vars, 0);
        ptr=GetToken(Tempstr, ":", &From, 0);
        To=CopyStr(To, ptr);
        To=SlashTerminateDirectoryPath(To);

        if (Config->Flags & FLAG_DEBUG) printf("copyfiles-to: [%s] [%s]\n", From, To);
        MakeDirPath(To, 0766);

        glob(From, 0, 0, &Glob);
        for (i=0; i < Glob.gl_pathc; i++)
        {
            ptr=Glob.gl_pathv[i];
            Tempstr=MCopyStr(Tempstr, To, GetBasename(ptr), NULL);
            if (Config->Flags & FLAG_DEBUG) printf("COPY: [%s] [%s]\n", ptr, Tempstr);
            FileCopy(ptr, Tempstr);
        }
    }

    Destroy(Tempstr);
    Destroy(Value);
    Destroy(From);
    Destroy(To);
}


static void PostProcessMoveFilesFrom(TAction *Act, const char *PostProc)
{
    char *Tempstr=NULL, *Value=NULL, *From=NULL, *To=NULL;
    const char *ptr;
    glob_t Glob;
    int i;

    if (StrValid(PostProc))
    {
        From=SubstituteVarsInString(From, PostProc, Act->Vars, 0);
        glob(From, 0, 0, &Glob);
        for (i=0; i < Glob.gl_pathc; i++)
        {
            ptr=Glob.gl_pathv[i];
            if (Config->Flags & FLAG_DEBUG) printf("MOVE: [%s] [%s]\n", ptr, GetBasename(ptr));
            rename(ptr, GetBasename(ptr));
        }
    }

    Destroy(Tempstr);
    Destroy(Value);
    Destroy(From);
    Destroy(To);
}


static void PostProcessMoveFilesTo(TAction *Act, const char *PostProc)
{
    char *Tempstr=NULL, *Value=NULL, *From=NULL, *To=NULL;
    const char *ptr;
    glob_t Glob;
    int i;


    if (StrValid(PostProc))
    {
        ptr=GetToken(PostProc, ":", &Tempstr, 0);
        From=SubstituteVarsInString(From, Tempstr, Act->Vars, 0);

        To=SubstituteVarsInString(To, ptr, Act->Vars, 0);
        To=SlashTerminateDirectoryPath(To);

        if (Config->Flags & FLAG_DEBUG) printf("movefiles-to: [%s] [%s]\n", From, To);
        MakeDirPath(To, 0766);

        glob(From, 0, 0, &Glob);
        for (i=0; i < Glob.gl_pathc; i++)
        {
            ptr=Glob.gl_pathv[i];
            Tempstr=MCopyStr(Tempstr, To, GetBasename(ptr), NULL);
            if (Config->Flags & FLAG_DEBUG) printf("MOVE: [%s] [%s]\n", ptr, Tempstr);
            rename(ptr, Tempstr);
        }
    }

    Destroy(Tempstr);
    Destroy(Value);
    Destroy(From);
    Destroy(To);
}


static void PostProcessChExt(TAction *Act, const char *PostProc)
{
    char *Tempstr=NULL, *Value=NULL, *From=NULL, *To=NULL;
    const char *ptr;
    glob_t Glob;
    int i;

    if (StrValid(PostProc))
    {
        ptr=GetToken(PostProc, ":", &Tempstr, 0);
        From=SubstituteVarsInString(From, Tempstr, Act->Vars, 0);
        To=SubstituteVarsInString(To, ptr, Act->Vars, 0);

        if (Config->Flags & FLAG_DEBUG) printf("chext: [%s] [%s]\n", From, To);
        MakeDirPath(To, 0766);

        glob(From, 0, 0, &Glob);
        for (i=0; i < Glob.gl_pathc; i++)
        {
            ptr=Glob.gl_pathv[i];
            if (Config->Flags & FLAG_DEBUG) printf("CHEXT: [%s] [%s]\n", ptr, To);
            FileChangeExtension(ptr, To);
        }
    }

    Destroy(Tempstr);
    Destroy(Value);
    Destroy(From);
    Destroy(To);
}


static void PostProcessLinkExt(TAction *Act, const char *PostProc)
{
    char *Tempstr=NULL, *Value=NULL, *From=NULL, *To=NULL;
    const char *ptr;
    glob_t Glob;
    int i;

    if (StrValid(PostProc))
    {
        ptr=GetToken(PostProc, ":", &Tempstr, 0);
        From=SubstituteVarsInString(From, Tempstr, Act->Vars, 0);
        To=SubstituteVarsInString(To, ptr, Act->Vars, 0);

        if (Config->Flags & FLAG_DEBUG) printf("chext: [%s] [%s]\n", From, To);
        MakeDirPath(To, 0766);

        glob(From, 0, 0, &Glob);
        for (i=0; i < Glob.gl_pathc; i++)
        {
            ptr=Glob.gl_pathv[i];
            if (Config->Flags & FLAG_DEBUG) printf("CHEXT: [%s] [%s]\n", ptr, To);
            link(ptr, To);
        }
    }

    Destroy(Tempstr);
    Destroy(Value);
    Destroy(From);
    Destroy(To);
}



static void PostProcessRename(TAction *Act, const char *PostProc)
{
    char *Tempstr=NULL, *Value=NULL, *From=NULL, *To=NULL;
    const char *ptr;
    glob_t Glob;
    int i, result;


    if (StrValid(PostProc))
    {
        Tempstr=SubstituteVarsInString(Tempstr, PostProc, Act->Vars, 0);
        ptr=GetToken(Tempstr, "\\S", &Value, GETTOKEN_QUOTES);
        From=UnQuoteStr(From, Value);
        ptr=GetToken(ptr, "\\S", &Value, GETTOKEN_QUOTES);
        To=UnQuoteStr(To, Value);

        if (Config->Flags & FLAG_DEBUG) printf("RENAME: '%s' -> '%s'\n", From, To);
        result=rename(From, To);
    }

    Destroy(Tempstr);
    Destroy(Value);
    Destroy(From);
    Destroy(To);
}


static void PostProcessLink(TAction *Act, const char *PostProc)
{
    char *Tempstr=NULL, *Value=NULL, *From=NULL, *To=NULL;
    const char *ptr;
    glob_t Glob;
    int i, result;

    if (StrValid(PostProc))
    {
        Tempstr=SubstituteVarsInString(Tempstr, PostProc, Act->Vars, 0);
        ptr=GetToken(Tempstr, ":", &Value, GETTOKEN_QUOTES);
        From=UnQuoteStr(From, Value);
        To=UnQuoteStr(To, ptr);

        if (Config->Flags & FLAG_DEBUG) printf("LINK: '%s' -> '%s'\n", From, To);
        //UninstallDir(Act, To);
        rmdir(To);
        unlink(To);
        result=symlink(From, To);
    }

    Destroy(Tempstr);
    Destroy(Value);
    Destroy(From);
    Destroy(To);
}


static void PostProcessZip(TAction *Act, const char *PostProc)
{
    char *Tempstr=NULL, *Value=NULL, *Zip=NULL;
    STREAM *S;
    const char *ptr;


    if (StrValid(PostProc))
    {
        Tempstr=SubstituteVarsInString(Tempstr, PostProc, Act->Vars, 0);
        strrep(Tempstr, ',', ' ');
        ptr=GetToken(Tempstr, ":", &Value, GETTOKEN_QUOTES);
        Zip=MCopyStr(Zip, "zip ", Value, " ", ptr, NULL);
        printf("packing into zip: %s'\n", Zip);
        Tempstr=MCopyStr(Tempstr, "cmd:", Zip, NULL);
        S=STREAMOpen(Tempstr, "rw setsid");
        if (S)
        {
            Tempstr=STREAMReadDocument(Tempstr, S);
            STREAMClose(S);
        }
    }

    Destroy(Tempstr);
    Destroy(Value);
    Destroy(Zip);
}




static void PostProcessInstall(TAction *Act)
{
    char *From=NULL, *To=NULL, *Tempstr=NULL, *Item=NULL,  *Name=NULL, *Value=NULL;
    const char *ptr;
    int i, result;

    PostProcessSetupDirVar(Act, "saves-dir");
    PostProcessSetupDirVar(Act, "patches-dir");

    ptr=GetNameValuePair(Act->PostProcess, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        if (strcasecmp(Name, "delete")==0) PostProcessDelete(Act, Value);
        if (strcasecmp(Name, "copyfiles-from")==0) PostProcessCopyFilesFrom(Act, Value);
        if (strcasecmp(Name, "copyfiles-to")==0) PostProcessCopyFilesTo(Act, Value);
        if (strcasecmp(Name, "movefiles-from")==0) PostProcessMoveFilesFrom(Act, Value);
        if (strcasecmp(Name, "movefiles-to")==0) PostProcessMoveFilesTo(Act, Value);
        if (strcasecmp(Name, "chext")==0) PostProcessChExt(Act, Value);
        if (strcasecmp(Name, "lnext")==0) PostProcessLinkExt(Act, Value);
        if (strcasecmp(Name, "rename")==0) PostProcessRename(Act, Value);
        if (strcasecmp(Name, "link")==0) PostProcessLink(Act, Value);
        if (strcasecmp(Name, "zip")==0) PostProcessZip(Act, Value);

        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    ptr=GetVar(Act->Vars, "winmanager");
    if (StrValid(ptr)) RegEdit(Act, REG_NO_WINMANAGER, NULL, NULL, NULL);


    Destroy(Tempstr);
    Destroy(Name);
    Destroy(Value);
    Destroy(From);
    Destroy(To);
}


void InstallFindIcon(TAction *Act)
{
    ListNode *Founds, *Curr;
    char *Tempstr=NULL;

    if (StrValid(GetVar(Act->Vars, "app-icon"))) return;

    Founds=ListCreate();
    FindFiles(GetVar(Act->Vars, "install-dir"), GetVar(Act->Vars, "icon"), "", Founds);
    Curr=ListGetNext(Founds);
    if (Curr)
    {
        Tempstr=MCopyStr(Tempstr, "~g~eFound AppIcon: ~w", Curr->Item, "~0\n", NULL);
        TerminalPutStr(Tempstr, NULL);
        SetVar(Act->Vars, "app-icon", Curr->Item);
    }
    ListDestroy(Founds, Destroy);
    DestroyString(Tempstr);
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

        if (! StrValid(Tempstr)) Tempstr=CopyStr(Tempstr, GetVar(Act->Vars, "install-dir"));

        if (StrValid(Tempstr))
        {
            WorkDir=SubstituteVarsInString(WorkDir, Tempstr, Act->Vars, 0);
            SetVar(Act->Vars, "working-dir", WorkDir);
            if (Act->Flags & FLAG_NOEXEC) Path=CopyStr(Path, WorkDir);
        }
        break;
    }

    if (StrValid(Path) || (Act->Flags & FLAG_NOEXEC))
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
        if (PlatformBitWidth(Act->Platform)==64) SetVar(Act->Vars, "exec", ptr);
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
            Child->Platform=CopyStr(Child->Platform, Parent->Platform);
            SetVar(Child->Vars, "install-dir", GetVar(Parent->Vars, "install-dir"));
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
            printf("Remove installer: %s\n", Act->SrcPath);
            unlink(Act->SrcPath);
        }

        if (Act->Flags & FLAG_ABORT) _exit(1);
        //PostProcessInstall(Act);

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
    result=DesktopFileLoad(Parent);
    ActionDestroy(Parent);

    return(result);
}



static int InstallFindEmulator(TAction *Act)
{
    char *Tempstr=NULL, *Emulator=NULL;
    int result=TRUE;

    //is an emulator installed for this platform? NULL means one is required by can't be found,
    //empty string means none is required
    Emulator=PlatformFindEmulator(Emulator, Act->Platform, GetVar(Act->Vars, "required_emulator"));

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
        result=FALSE;
    }
    else Tempstr=CopyStr(Tempstr, "No emulator required\n");

    TerminalPutStr(Tempstr, NULL);

    Destroy(Tempstr);
    Destroy(Emulator);

    return(result);
}


void InstallApp(TAction *Act)
{
    const char *ptr;
    char *Name=NULL, *Path=NULL, *Tempstr=NULL;

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
    else if (InstallFindEmulator(Act))
    {

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
        if (InstallFindEmulator(Act))
        {
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
    }

    Destroy(Tempstr);
    Destroy(Path);
}


