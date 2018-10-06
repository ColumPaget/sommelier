#include "install.h"
#include "desktopfiles.h"
#include "regedit.h"
#include "download.h"
#include "apps.h"





static int InList(const char *Item, const char *List)
{
char *Match=NULL;
const char *ptr;

ptr=GetToken(List, ",", &Match, GETTOKEN_QUOTES);
while (ptr)
{
strlwr(Match);
if (fnmatch(Match, Item, 0)==0) 
{
Destroy(Match);
return(TRUE);
}
ptr=GetToken(ptr, ",", &Match, GETTOKEN_QUOTES);
}

Destroy(Match);

return(FALSE);
}


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
							ListAddNamedItem(Founds, Tempstr, CopyStr(NULL, Glob.gl_pathv[i]));
			}
		}
	}	
}
globfree(&Glob);
Destroy(Tempstr);
}



/*
Try to find the actual executable that we'll run with 'sommelier'
*/
static char *FindProgram(char *RetStr, TAction *Act)
{
ListNode *Exes, *Curr;
char *Tempstr=NULL;
const char *ptr;

RetStr=CopyStr(RetStr, "");
Exes=ListCreate();
Tempstr=MCopyStr(Tempstr, "setup.exe,unin*.exe,crash*.exe,",GetVar(Act->Vars, "dlfile"),",",GetVar(Act->Vars,"installer"),NULL);

FindFiles(GetVar(Act->Vars, "drive_c"), "*.exe", Tempstr, Exes);

Curr=NULL;
ptr=GetVar(Act->Vars, "exec");
if (StrValid(ptr)) Curr=ListFindNamedItem(Exes, ptr);
if (! Curr) Curr=ListGetNext(Exes);
if (Curr) RetStr=CopyStr(RetStr, (const char *) Curr->Item);

printf("Found Program: %s\n", RetStr);

ListDestroy(Exes, Destroy);
DestroyString(Tempstr);

return(RetStr);
}



static char *FindInstaller(char *RetStr, const char *Path, TAction *Act)
{
ListNode *Exes, *Curr;
const char *p_installdir;

RetStr=CopyStr(RetStr,"");
Exes=ListCreate();

p_installdir=GetVar(Act->Vars, "install-dir");
FindFiles(p_installdir, "setup.exe,install.exe,*.msi", "", Exes);
Curr=ListGetNext(Exes);
if (Curr) RetStr=CopyStr(RetStr, ((const char *) Curr->Item) + StrLen(p_installdir));

ListDestroy(Exes, NULL);

if (StrLen(RetStr)) printf("Found installer program: %s\n", RetStr);
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
		printf("Running installer in a virtual desktop\n");
		RegFlags |= REG_VDESK;
	}

	if (RegFlags)	RegEdit(Act, RegFlags);

	ptr=GetVar(Act->Vars, "installer");
	if (StrValid(ptr)) 
	{
		if (strcmp(Act->Platform, "dos")==0) Tempstr=SubstituteVarsInString(Tempstr, "dosbox '$(install-dir)/$(installer)' $(installer-args)", Act->Vars, 0);
		else 
		{
			ptr=strrchr(ptr, '.');
			if (ptr && (strcmp(ptr, ".msi")==0)) Tempstr=SubstituteVarsInString(Tempstr, "WINEPREFIX=$(prefix) wine msiexec /i '$(install-dir)/$(installer)' $(installer-args)", Act->Vars, 0);
			else Tempstr=SubstituteVarsInString(Tempstr, "WINEPREFIX=$(prefix) wine '$(install-dir)/$(installer)' $(installer-args)", Act->Vars, 0);
		}

		Cmd=SubstituteVarsInString(Cmd, Tempstr, Act->Vars, 0);
		RunProgramAndConsumeOutput(Cmd, Act->Flags);
	}

	ptr=GetVar(Act->Vars, "install_stage2");
	if (StrValid(ptr)) 
	{
		Tempstr=SubstituteVarsInString(Tempstr, "WINEPREFIX=$(prefix) wine '$(install-dir)/$(install_stage2)'", Act->Vars, 0);
		//printf("STAGE2: [%s]\n",Tempstr);
		RunProgramAndConsumeOutput(Tempstr, Act->Flags);
	}

	if (strncasecmp(Act->Platform, "win", 3)==0)
	{
	if (RegFlags & REG_VDESK)	
	{
		RegFlags &= ~REG_VDESK;
		RegFlags |= REG_NO_VDESK;
	}
	RegFlags |= REG_FONT_SMOOTH;
	RegEdit(Act, RegFlags);
	}

	Destroy(Tempstr);
	Destroy(Cmd);
}



static int InstallAppFromFile(TAction *Act, const char *Path)
{
char *Tempstr=NULL;
const char *ptr;

	if (strncasecmp(Act->Platform, "win", 3)==0)
	{	
		ptr=GetVar(Act->Vars,"exec-dir");
		if (! StrValid(ptr)) 
		{
			Tempstr=SubstituteVarsInString(Tempstr, "/Program Files/$(name)", Act->Vars, 0);	
			SetVar(Act->Vars, "exec-dir", Tempstr);
		}
	}

	switch (IdentifyFileType(Path))
	{
		case FILETYPE_ZIP:
		Tempstr=MCopyStr(Tempstr, "unzip -o '",Path, "'", NULL);
		RunProgramAndConsumeOutput(Tempstr, Act->Flags);
		ptr=GetVar(Act->Vars, "installer");
		if (! StrValid(ptr))
		{
			Tempstr=FindInstaller(Tempstr, Path, Act);
			SetVar(Act->Vars, "installer", Tempstr);
		}
		RunInstallers(Act);
		break;

		case FILETYPE_PE:
		case FILETYPE_MZ:
		if (Act->InstallType == INSTALL_EXECUTABLE)
		{
			SetVar(Act->Vars, "exec", GetBasename(Act->URL));	
		}
		else
		{
		ptr=GetVar(Act->Vars, "installer");
		if (! StrValid(ptr)) SetVar(Act->Vars, "installer", GetBasename(Path));
		RunInstallers(Act);
		}
		break;

		case FILETYPE_MSI:
		//wine msiexec /i whatever.msi 
		Tempstr=MCopyStr(Tempstr, "WINEPREFIX=", GetVar(Act->Vars, "prefix"), " wine msiexec /i './", Path, "'", NULL);
		RunProgramAndConsumeOutput(Tempstr, Act->Flags);
		break;
	}

Destroy(Tempstr);

return(TRUE);
}






static void PostProcessInstall(TAction *Act)
{
glob_t Glob;
const char *ptr;
int i;

ptr=GetVar(Act->Vars, "movefiles-from");
if (StrValid(ptr))
{
	glob(ptr, 0, 0, &Glob);
	for (i=0; i < Glob.gl_pathc; i++)
	{
		rename(Glob.gl_pathv[i], GetBasename(Glob.gl_pathv[i]));
	}
}

ptr=GetVar(Act->Vars, "winmanager");
if (StrValid(ptr)) RegEdit(Act, REG_NO_WINMANAGER);

}




/*
For DOS and windows executables that we might have downloaded as a Zip or an MSI file, we finalize here and setup
the actual program that we're going to run to execute the application
*/
static void FinalizeExeInstall(TAction *Act)
{
char *Path=NULL, *Tempstr=NULL;
const char *ptr;
char *wptr;

Tempstr=TerminalFormatStr(Tempstr, "~eFinalizing installation.~0", NULL);
printf("%s\n", Tempstr);

		PostProcessInstall(Act);
		Path=FindProgram(Path, Act);

		Tempstr=CopyStr(Tempstr, GetVar(Act->Vars, "drive_c"));	

		if (StrLen(Path) > StrLen(Tempstr))
		{
			ptr=Path+StrLen(Tempstr);
			Tempstr=CopyStr(Tempstr, ptr);
			SetVar(Act->Vars, "exec", GetBasename(Tempstr));
			wptr=strrchr(Tempstr, '/');
			if (StrRTruncChar(Tempstr, '/')) SetVar(Act->Vars, "exec-dir", Tempstr);
		}

Destroy(Path);
Destroy(Tempstr);
}


static void InstallSingleItem(TAction *Act)
{
char *Path=NULL, *InstallPath=NULL, *Tempstr=NULL;
const char *p_URL, *ptr;
int InstallResult=FALSE;

//Fork so we don't chdir current app
if (fork()==0)
{
	InstallPath=CopyStr(InstallPath, GetVar(Act->Vars, "install-dir"));
	if (strcasecmp(Act->Platform, "go")==0)
	{
		p_URL=Act->URL;
		if (strncmp(p_URL, "http:",5)==0)  p_URL+=5;
		if (strncmp(p_URL, "https:",6)==0) p_URL+=6;
		while (*p_URL=='/') p_URL++;
		Tempstr=MCopyStr(Tempstr, "export GOPATH=",InstallPath,"; go get ",p_URL, NULL);
		chdir(InstallPath);
		system(Tempstr);
printf("%s\n",Tempstr);
		InstallResult=TRUE;
	}
	else
	{
		chdir(InstallPath);
/*
		if (
			 (strncmp(Act->URL, "ssh:",4)==0) ||
			 (strncmp(Act->URL, "http:",5)==0) ||
		   (strncmp(Act->URL, "https:",6)==0) 
	   )
*/
		{
			if (Download(Act)==0)
			{
				printf("ERROR: Download Failed, '0' bytes received!\n");
			}
			else if ( (! (Act->Flags & FLAG_FORCE)) && (! CompareSha256(Act)) )
			{
				printf("ERROR: Download Hash mismatch!\n");
				printf("File we downloaded was not the one we expected. Run with -force if you want to risk it.\n");
			}
			else
			{
			Tempstr=URLBasename(Tempstr, Act->URL);
			InstallResult=InstallAppFromFile(Act, Tempstr);
			}
		}
		//else InstallResult=InstallAppFromFile(Act, Act->URL);


		if (InstallResult) FinalizeExeInstall(Act);
	}

	if (InstallResult && (! (Act->Flags & FLAG_DEPENDANCY)) ) DesktopFileGenerate(Act, Path);
	_exit(0);
}

wait(NULL);


DestroyString(Tempstr);
DestroyString(InstallPath);
DestroyString(Path);
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
TAction *Dependancy;
char *Name=NULL;

p_Requires=GetVar(Act->Vars, "requires");
if (p_Requires)
{
	ptr=GetToken(p_Requires,",",&Name,0);
	while (ptr)
	{
		printf("installing dependancy '%s'\n", Name);
		Dependancy=AppActionCreate(ACT_INSTALL, Name, Act->ConfigPath);
		if (Dependancy)
		{
		Dependancy->Flags |= FLAG_DEPENDANCY;
		SetVar(Dependancy->Vars, "prefix", GetVar(Act->Vars, "prefix"));
		SetVar(Dependancy->Vars, "install-dir", GetVar(Act->Vars, "install-dir"));
		SetVar(Dependancy->Vars, "exec-dir", GetVar(Act->Vars, "exec-dir"));
		InstallSingleItem(Dependancy);
		ActionDestroy(Dependancy);
		}

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
Tempstr=TerminalFormatStr(Tempstr, Name, NULL);
printf("%s\n", Tempstr);

Path=AppFormatPath(Path, Act);
MakeDirPath(Path, 0700);

if (strncasecmp(Act->Platform, "win",3)==0) InstallSetupWindowsDependancies(Act);
InstallRequiredDependancies(Act);

InstallSingleItem(Act);

Destroy(Tempstr);
Destroy(Path);
}


