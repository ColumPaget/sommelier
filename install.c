#include "install.h"
#include "desktopfiles.h"
#include "regedit.h"
#include "download.h"
#include "apps.h"
#include "platforms.h"
#include "config.h"





static void FindFiles(const char *Path, const char *Inclusions, const char *Exclusions, ListNode *Founds)
{
char *Tempstr=NULL;
struct stat FStat;
glob_t Glob;
int i;
const char *IgnoreDirs[]={"windows", "Windows NT", "Internet Explorer", "Windows Media Player", NULL};
const char *ptr, *p_Basename;

Tempstr=CopyStr(Tempstr, Path);
Tempstr=SlashTerminateDirectoryPath(Tempstr);
Tempstr=CatStr(Tempstr, "*");

glob(Tempstr, 0, 0, &Glob);
for (i=0; i < Glob.gl_pathc; i++)
{
	lstat(Glob.gl_pathv[i], &FStat);
	p_Basename=GetBasename(Glob.gl_pathv[i]);
	if (p_Basename)
	{
		if (S_ISLNK(FStat.st_mode)) /*Do nothing, don't follow links */ ;
		else if (S_ISDIR(FStat.st_mode))
		{
			if (MatchTokenFromList(p_Basename, IgnoreDirs, 0) == -1) FindFiles(Glob.gl_pathv[i], Inclusions, Exclusions, Founds);
		}		
		else 
		{
			Tempstr=CopyStr(Tempstr, p_Basename);
			strlwr(Tempstr);
			if ( InList(Tempstr, Inclusions) && (! InList(Tempstr, Exclusions)) ) 
			{
							ListAddNamedItem(Founds, p_Basename, CopyStr(NULL, Glob.gl_pathv[i]));
			}
		}
	}	
}
globfree(&Glob);
Destroy(Tempstr);
}



char *FindSingleFile(char *RetStr, const char *Root, const char *File)
{
ListNode *Files, *Curr;

RetStr=CopyStr(RetStr, "");
Files=ListCreate();


FindFiles(Root, File, "", Files);

Curr=ListGetNext(Files);
if (Curr) RetStr=CopyStr(RetStr, Curr->Item);

ListDestroy(Files, Destroy);
return(RetStr);
}


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

Tempstr=PlatformGetExeSearchPattern(Tempstr, Act->Platform);
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
static char *FindProgram(char *RetStr, TAction *Act)
{
char *Exec=NULL, *Tempstr=NULL;
const char *ptr;

ptr=GetVar(Act->Vars, "exec");
if (StrValid(ptr)) Exec=SubstituteVarsInString(Exec, ptr, Act->Vars, 0);


//full path given to executable, so it must exist at this path
if (StrValid(Exec) && *Exec == '/')
{
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




static void RunInstallers(TAction *Act)
{
const char *ptr;
char *Tempstr=NULL, *Cmd=NULL;
int RegFlags=0;



	ptr=GetVar(Act->Vars, "installer-vdesk");
	if (StrValid(ptr)) 
	{
		if (Config->Flags & FLAG_DEBUG) printf("Running installer in a virtual desktop\n");
		RegFlags |= REG_VDESK;
	}

	if (RegFlags)	RegEdit(Act, RegFlags, NULL, NULL, NULL);

	ptr=GetVar(Act->Vars, "installer-path");
	if (StrValid(ptr)) 
	{
		switch (PlatformType(Act->Platform))
		{
			case PLATFORM_DOS:
			Tempstr=SubstituteVarsInString(Tempstr, "dosbox '$(installer-path)' $(installer-args)", Act->Vars, 0);
			printf("RUN INSTALLER: %s\n",Tempstr);
			Cmd=SubstituteVarsInString(Cmd, Tempstr, Act->Vars, 0);
			RunProgramAndConsumeOutput(Cmd, Act->Flags);
			break;

			case PLATFORM_WINDOWS:
			//windos has windows installer but msdos (Dosbox) executable
			case PLATFORM_GOGWINDOS:
			ptr=strrchr(ptr, '.');
			if (ptr && (strcmp(ptr, ".msi")==0)) Tempstr=SubstituteVarsInString(Tempstr, "WINEPREFIX=$(prefix) WINEDLLOVERRIDES=\"mscoree,mshtml=\" wine msiexec /i '$(installer-path)' $(installer-args)", Act->Vars, 0);
			else Tempstr=SubstituteVarsInString(Tempstr, "WINEPREFIX=$(prefix) WINEDLLOVERRIDES=\"mscoree,mshtml=\" wine '$(installer-path)' $(installer-args)", Act->Vars, 0);

		printf("RUN INSTALLER: %s    in %s\n",Tempstr, get_current_dir_name());
		Cmd=SubstituteVarsInString(Cmd, Tempstr, Act->Vars, 0);
		RunProgramAndConsumeOutput(Cmd, Act->Flags);
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
		RunProgramAndConsumeOutput(Cmd, Act->Flags);
	}

	if (PlatformType(Act->Platform)==PLATFORM_WINDOWS)
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


	switch (PlatformType(Act->Platform))
	{	
		case PLATFORM_SCUMMVM:
			ForcedFileType=FILETYPE_ZIP;
		break;

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

	ptr=GetVar(Act->Vars, "download-type");
	if (StrValid(ptr))
	{
		if (strcasecmp(ptr, "tar.gz")==0) ForcedFileType=FILETYPE_TGZ;
		else if (strcasecmp(ptr, "tar.bz2")==0) ForcedFileType=FILETYPE_TBZ;
		else if (strcasecmp(ptr, "tar.xz")==0) ForcedFileType=FILETYPE_TXZ;
	}


	switch (IdentifyFileType(Path, ForcedFileType))
	{
		case FILETYPE_ZIP:
		Tempstr=MCopyStr(Tempstr, "unzip -o '",Path, "' ", FilesToExtract, NULL);
		printf("unpacking: %s\n",GetBasename(Path));
		RunProgramAndConsumeOutput(Tempstr, Act->Flags);

		//for zipfiles and the like the installer has to be found. Either it's
		//specified in the app config, as 'installer' 
		//or we go looking for certain common filenames

		ptr=GetVar(Act->Vars, "installer");
		if (! ptr) ptr="setup.exe,install.exe,*.msi";
		Tempstr=FindSingleFile(Tempstr, GetVar(Act->Vars, "install-dir"), ptr);
		if (StrValid(Tempstr)) 
		{
			printf("Found installer program: %s\n", Tempstr);
			SetVar(Act->Vars, "installer-path", Tempstr);
			RunInstallers(Act);
		}
		break;

		case FILETYPE_TGZ:
		case FILETYPE_TBZ:
		case FILETYPE_TXZ:
		Tempstr=MCopyStr(Tempstr, "tar -xf '",Path, "' ", FilesToExtract, NULL);
		printf("unpacking: %s\n",GetBasename(Path));
		RunProgramAndConsumeOutput(Tempstr, Act->Flags);
		break;

		case FILETYPE_PE:
		case FILETYPE_MZ:
		if (Act->InstallType == INSTALL_EXECUTABLE)
		{
			//if file has no installer, is just an executable, then do nothing
			SetVar(Act->Vars, "exec", GetBasename(Act->URL));	
		}
		else
		{
			//else file is an installer, set the install-path to point to it
			SetVar(Act->Vars, "installer-path", Path);
			RunInstallers(Act);
		}
		break;

		case FILETYPE_MSI:
		//wine msiexec /i whatever.msi 
		Tempstr=MCopyStr(Tempstr, "WINEPREFIX=", GetVar(Act->Vars, "prefix"), " wine msiexec /i '", Path, "'", NULL);
		RunProgramAndConsumeOutput(Tempstr, Act->Flags);
		break;
	}



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
	glob(Tempstr, 0, 0, &Glob);
	for (i=0; i < Glob.gl_pathc; i++)
	{
		if (Config->Flags & FLAG_DEBUG) printf("DELETE: %s\n", Glob.gl_pathv[i]);
		unlink(Glob.gl_pathv[i]);
	}
}

ptr=GetVar(Act->Vars, "movefiles-from");
if (StrValid(ptr))
{
	Tempstr=SubstituteVarsInString(Tempstr, ptr, Act->Vars, 0);
	glob(Tempstr, 0, 0, &Glob);
	for (i=0; i < Glob.gl_pathc; i++)
	{
		if (Config->Flags & FLAG_DEBUG) printf("MOVE: %s\n", Glob.gl_pathv[i]);
		rename(Glob.gl_pathv[i], GetBasename(Glob.gl_pathv[i]));
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




/*
For DOS and windows executables that we might have downloaded as a Zip or an MSI file, we finalize here and setup
the actual program that we're going to run to execute the application
*/
static void FinalizeExeInstall(TAction *Act)
{
char *Path=NULL, *Tempstr=NULL, *WorkDir=NULL;
const char *ptr;
char *wptr;
int len;

	PostProcessInstall(Act);

	Path=FindProgram(Path, Act);
	switch (PlatformType(Act->Platform))
	{
		case PLATFORM_WINDOWS:

		//reconfigure path to be from our wine 'drive_c' rather than from system root
		ptr=GetVar(Act->Vars, "drive_c");
		len=StrLen(ptr);
		if ( (len > 0) && (strncmp(Path, ptr, len)==0) )
		{
			Tempstr=CopyStr(Tempstr, Path+len);
			Path=CopyStr(Path, Tempstr);	
		}

		ptr=GetVar(Act->Vars,"exec-dir");
		if (! StrValid(ptr)) 
		{
			Tempstr=CopyStr(Tempstr, Path);
			StrRTruncChar(Tempstr, '/');
//			Tempstr=SubstituteVarsInString(Tempstr, "/Program Files/$(name)", Act->Vars, 0);	
			SetVar(Act->Vars, "exec-dir", Tempstr);
		}
    Tempstr=SubstituteVarsInString(Tempstr, "$(drive_c)$(exec-dir)", Act->Vars, 0);
    SetVar(Act->Vars, "working-dir", Tempstr);
		break;

		default:
		//is a working dir set in the app config?
		Tempstr=CopyStr(Tempstr, GetVar(Act->Vars, "working-dir"));

		//if not set in the app config, is a working dir set in the platform config?
		if (! StrValid(Tempstr)) Tempstr=PlatformGetWorkingDir(Tempstr, Act->Platform);

		//if neither of the above, we assume the working dir is the one that we found
		//the executable in
		if (! StrValid(Tempstr))
		{
			Tempstr=CopyStr(Tempstr, Path);
			StrRTruncChar(Tempstr, '/');
		}

		if (StrValid(Tempstr))
		{
			WorkDir=SubstituteVarsInString(WorkDir, Tempstr, Act->Vars, 0);
			SetVar(Act->Vars, "working-dir", WorkDir);
		}

		if (! StrValid(GetVar(Act->Vars, "exec-dir"))) SetVar(Act->Vars, "exec-dir", ".");
		break;
	}


	if (StrValid(Path))
	{
	Tempstr=QuoteCharsInStr(Tempstr, GetBasename(Path), " 	");
	if (! StrValid(GetVar(Act->Vars, "exec")) ) SetVar(Act->Vars, "exec", Tempstr);
	SetVar(Act->Vars, "exec-path", Path);

	//must do this before DesktopFileGenerate, because some of the settings for some platforms are stored in the
	//desktop file as command-line args passed to the emulator
	PlatformApplySettings(Act);
	if (! (Act->Flags & FLAG_DEPENDANCY)) DesktopFileGenerate(Act, Path);
	}
	else 
	{
		Tempstr=MCopyStr(Tempstr, "~rERROR: Failed to find exectuable for ", Act->Name, "~0\n", NULL);
		TerminalPutStr(Tempstr, NULL);
	}

Destroy(Tempstr);
Destroy(WorkDir);
Destroy(Path);
}


static void InstallSingleItemPreProcessInstall(TAction *Act)
{
char *Token=NULL, *Tempstr=NULL;
const char *ptr;

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
const char *p_URL, *ptr;
int InstallResult=FALSE, exitval;
pid_t pid;

//Fork so we don't chdir current app
pid=fork();
if (pid==0)
{
	InstallSingleItemPreProcessInstall(Act);

	InstallPath=CopyStr(InstallPath, GetVar(Act->Vars, "install-dir"));

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
	if ( InstallResult && (! (Act->Flags & FLAG_DEPENDANCY)) ) 
	{
		TerminalPutStr("~eFinding executables~0\n", NULL);
		FinalizeExeInstall(Act);
		InstallBundledItems(Act);
	}
	}


	if (
		(Act->InstallType != INSTALL_EXECUTABLE) &&
		(! (Act->Flags & FLAG_KEEP_INSTALLER)) &&
		(Act->Flags & FLAG_DOWNLOADED)
	) 
	{
		unlink(Act->SrcPath);
	}

	if (Act->Flags |= FLAG_ABORT) _exit(1);
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
		if (Dependancy && AppLoadConfig(Dependancy))
		{
		Dependancy->Flags |= FLAG_DEPENDANCY;
		CopyVars(Dependancy->Vars, Parent->Vars);
		InstallSingleItem(Dependancy);
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
}



void InstallApp(TAction *Act)
{
const char *ptr, *p_Requires;
char *Name=NULL, *Path=NULL, *Tempstr=NULL;

Tempstr=MCopyStr(Tempstr, "\n~e##### Installing ", Act->Name, " #########~0\n", NULL);
TerminalPutStr(Tempstr, NULL);

if (! StrValid(Act->Platform))
{
TerminalPutStr("~r~eERROR: no platform configured for this application~0 Cannot install.\n", NULL);
}
else if (PlatformType(Act->Platform)==PLATFORM_UNKNOWN)
{
Tempstr=FormatStr(Tempstr, "~r~eERROR: Unknown platform type '%s'~0 Cannot install.\n", Act->Platform);
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


Tempstr=CopyStr(Tempstr, "");
Name=PlatformGetInstallMessage(Name, Act->Platform);
if (StrValid(Name)) Name=MCatStr(Name, "\n~r", Name, "~0\n", NULL);

ptr=GetVar(Act->Vars, "warn");
if (StrValid(ptr)) Tempstr=MCatStr(Tempstr, "\n~rWARN: ", ptr, "~0\n", NULL);

TerminalPutStr(Tempstr, NULL);


Path=AppFormatPath(Path, Act);
MakeDirPath(Path, 0700);

if (PlatformType(Act->Platform)==PLATFORM_WINDOWS) InstallSetupWindowsDependancies(Act);
InstallRequiredDependancies(Act);

InstallSingleItem(Act);
printf("%s install complete\n", Act->Name);
}

Destroy(Tempstr);
Destroy(Path);
}



void InstallReconfigure(TAction *Act)
{
const char *ptr, *p_Requires;
char *Name=NULL, *Path=NULL, *Tempstr=NULL;

Tempstr=MCopyStr(Tempstr, "\n~e##### Reconfigure ", Act->Name, " #########~0\n", NULL);
TerminalPutStr(Tempstr, NULL);

if (! StrValid(Act->Platform))
{
TerminalPutStr("~r~eERROR: no platform configured for this application~0\n", NULL);
}
else if (PlatformType(Act->Platform)==PLATFORM_UNKNOWN)
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


