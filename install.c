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

printf("Found Program: %s\n", RetStr);

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
			FilesToExtract=CopyStr(FilesToExtract, "data/noarch/game/*");
		break;

		case PLATFORM_GOGSCUMMVM:
		case PLATFORM_GOGDOS:
			ForcedFileType=FILETYPE_ZIP;
			FilesToExtract=CopyStr(FilesToExtract, "data/noarch/data/*");
		break;

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

Tempstr=TerminalFormatStr(Tempstr, "~eFinalizing installation.~0", NULL);
printf("%s\n", Tempstr);

ptr=GetVar(Act->Vars, "delete");
if (StrValid(ptr))
{
	Tempstr=SubstituteVarsInString(Tempstr, ptr, Act->Vars, 0);
	glob(Tempstr, 0, 0, &Glob);
	for (i=0; i < Glob.gl_pathc; i++)
	{
		printf("DELETE: %s\n", Glob.gl_pathv[i]);
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
		printf("MOVE: %s\n", Glob.gl_pathv[i]);
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

	printf("RENAME: '%s' -> '%s'\n", From, To);
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


	Tempstr=QuoteCharsInStr(Tempstr, GetBasename(Path), " 	");
	if (! StrValid(GetVar(Act->Vars, "exec")) ) SetVar(Act->Vars, "exec", Tempstr);
	SetVar(Act->Vars, "exec-path", Path);

Destroy(Path);
Destroy(Tempstr);
Destroy(WorkDir);
}


static void InstallSingleItemPreProcessInstall(TAction *Act)
{
char *Token=NULL, *Tempstr=NULL;
const char *ptr;

ptr=GetVar(Act->Vars, "dll-overrides");
if (StrValid(ptr))
{
	//run fake installer to ensure a wine bottle is created
	SetVar(Act->Vars, "installer", "this-does-not-exist.exe");
	RunInstallers(Act);

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


static void InstallSingleItem(TAction *Act)
{
char *Path=NULL, *InstallPath=NULL, *Tempstr=NULL;
const char *p_URL, *ptr;
int InstallResult=FALSE;

//Fork so we don't chdir current app
if (fork()==0)
{
	InstallSingleItemPreProcessInstall(Act);

	InstallPath=CopyStr(InstallPath, GetVar(Act->Vars, "install-dir"));

	switch (PlatformType(Act->Platform))
	{
	case PLATFORM_GO:
		p_URL=Act->URL;
		if (strncmp(p_URL, "http:",5)==0)  p_URL+=5;
		if (strncmp(p_URL, "https:",6)==0) p_URL+=6;
		while (*p_URL=='/') p_URL++;
		Tempstr=MCopyStr(Tempstr, "export GOPATH=",InstallPath,"; go get ",p_URL, NULL);
		chdir(InstallPath);
		system(Tempstr);
printf("%s\n",Tempstr);
		InstallResult=TRUE;
	break;

	default:
		chdir(InstallPath);
		if (Download(Act)==0)
		{
			printf("ERROR: Download Failed, '0' bytes received!\n");
		}
		else 
		{
			Tempstr=TerminalFormatStr(Tempstr, "~eValidating download~0", NULL);
			printf("%s\n", Tempstr);
			if ( Act->Flags & FLAG_FORCE) printf("install forced, not validating download\n");
			else if (! CompareSha256(Act)) 
			{
					printf("ERROR: Download Hash mismatch!\n");
					printf("File we downloaded was not the one we expected. Run with -force if you want to risk it.\n");
			}
		}

		InstallResult=InstallAppFromFile(Act, Act->SrcPath);
		if ( InstallResult && (! (Act->Flags & FLAG_DEPENDANCY)) ) FinalizeExeInstall(Act);
	break;
	}

	if (InstallResult && (! (Act->Flags & FLAG_DEPENDANCY)) ) DesktopFileGenerate(Act, Path);

	RegEditApplySettings(Act);

	_exit(0);

}

wait(NULL);


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

Name=MCopyStr(Name, "\n~e##### Installing ", Act->Name, " #########~0\n", NULL);

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

Tempstr=PlatformGetInstallMessage(Tempstr, Act->Platform);
if (StrValid(Tempstr)) Name=MCatStr(Name, "\n~r", Tempstr, "~0\n", NULL);

ptr=GetVar(Act->Vars, "warn");
if (StrValid(ptr)) Name=MCatStr(Name, "\n~rWARN: ", ptr, "~0\n", NULL);

Tempstr=TerminalFormatStr(Tempstr, Name, NULL);
printf("%s\n", Tempstr);


Path=AppFormatPath(Path, Act);
MakeDirPath(Path, 0700);

if (PlatformType(Act->Platform)==PLATFORM_WINDOWS) InstallSetupWindowsDependancies(Act);
InstallRequiredDependancies(Act);

InstallSingleItem(Act);

Destroy(Tempstr);
Destroy(Path);
}


