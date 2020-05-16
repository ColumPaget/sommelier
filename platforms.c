#include "platforms.h"
#include "config.h"
#include "regedit.h"
#include "doom.h"

ListNode *Platforms=NULL;


int PlatformType(const char *Platform)
{
const char *WindowsPlatforms[]={"win","win16","win32","win64","windows","gog:win","gog:windows", NULL};
const char *DosPlatforms[]={"dos","msdos", NULL};


if (! StrValid(Platform)) return(PLATFORM_WINDOWS);
if (MatchTokenFromList(Platform, WindowsPlatforms, 0) > -1) return(PLATFORM_WINDOWS);
if (MatchTokenFromList(Platform, DosPlatforms, 0) > -1) return(PLATFORM_DOS);
if (strcasecmp(Platform, "zx48")==0) return(PLATFORM_ZXSPECTRUM);
if (strcasecmp(Platform, "spectrum")==0) return(PLATFORM_ZXSPECTRUM);
if (strcasecmp(Platform, "scummvm")==0) return(PLATFORM_SCUMMVM);
if (strcasecmp(Platform, "gog:lin")==0) return(PLATFORM_GOGLINUX);
if (strcasecmp(Platform, "gog:linux")==0) return(PLATFORM_GOGLINUX);
if (strcasecmp(Platform, "gog:scummvm")==0) return(PLATFORM_GOGSCUMMVM);
if (strcasecmp(Platform, "gog:lindos")==0) return(PLATFORM_GOGDOS);
if (strcasecmp(Platform, "gog:windos")==0) return(PLATFORM_GOGWINDOS);
if (strcasecmp(Platform, "doom")==0) return(PLATFORM_DOOM);

return(PLATFORM_UNKNOWN);
}



static TPlatform *PlatformsAdd(const char *Names, int IDnum, const char *Emulators, const char *WorkingDir, const char *ExeSearchPattern, const char *Exe64SearchPattern)
{
TPlatform *Plt;

Plt=(TPlatform *) calloc(1, sizeof(TPlatform));
Plt->ID=IDnum;
Plt->Emulators=CopyStr(Plt->Emulators, Emulators);
Plt->WorkingDir=CopyStr(Plt->WorkingDir, WorkingDir);
Plt->ExeSearchPattern=CopyStr(Plt->ExeSearchPattern, ExeSearchPattern);

ListAddNamedItem(Platforms, Names, Plt);

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




char *PlatformFindEmulator(char *RetStr, char *Name)
{
char *Tempstr=NULL, *Emulators=NULL, *EmuInvoke=NULL, *EmuName=NULL;
const char *ptr;
TPlatform *Plt;

//seems a bit strange, but the idea here is that we'll return NULL
//if an emulator isn't found, but return empty string if one isn't needed
Destroy(RetStr);
RetStr=NULL;

Emulators=PlatformLookupInfo(Emulators, Name, PLATFORM_INFO_EMULATORS);
if (Emulators)
{
	if (! StrValid(Emulators)) RetStr=CopyStr(RetStr, "");
	else
	{
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



char *PlatformSelectForURL(char *RetStr, const char *URL)
{
ListNode *Curr;
TPlatform *Plt;
const char *p_filename;

RetStr=CopyStr(RetStr, "");
p_filename=GetBasename(URL);
if (! StrValid(p_filename)) return(RetStr);

Curr=ListGetNext(Platforms);
while (Curr)
{
	Plt=(TPlatform *) Curr->Item;
	if (StrValid(Plt->InstallerPattern))
	{
		if (InList(p_filename, Plt->InstallerPattern)) RetStr=MCatStr(RetStr, Curr->Tag, ",", NULL);
	}

	Curr=ListGetNext(Curr);
}

return(RetStr);
}




void PlatformApplySettings(TAction *Act)
{
switch (PlatformType(Act->Platform))
{
	case PLATFORM_WINDOWS:
		RegEditApplySettings(Act);
	break;

	case PLATFORM_DOOM:
		DoomApplySettings(Act);
	break;
}
}




void PlatformsInit()
{
TPlatform *Plt;

Platforms=ListCreate();

Plt=PlatformsAdd("dos,msdos", PLATFORM_DOS, "dosbox $(emulator-args) -exit '$(exec-path)' $(exec-args)", "", "*.exe","");

Plt=PlatformsAdd("windows,win,win16,win32,win64", PLATFORM_WINDOWS, "wine", "", "*.exe","");


Plt=PlatformsAdd("gog:lindos", PLATFORM_GOGDOS, "dosbox $(emulator-args) -exit '$(exec-path)' $(exec-args)", "", "*.exe","");
Plt->InstallerPattern=CopyStr(Plt->InstallerPattern, "*.sh");
Plt=PlatformsAdd("gog:windos", PLATFORM_GOGWINDOS, "dosbox $(emulator-args) -exit '$(exec-path)' $(exec-args)", "", "*.exe","");
Plt->InstallerPattern=CopyStr(Plt->InstallerPattern, "*.exe");

Plt=PlatformsAdd("gog:win,gog:windows", PLATFORM_WINDOWS, "wine", "", "*.exe","");
Plt->InstallMessage=CopyStr(Plt->InstallMessage, "Some gog.com windows installers display error messages at the end of the install. Just click through these, the game should have installed okay.");
Plt->InstallerPattern=CopyStr(Plt->InstallerPattern, "*.exe");

Plt=PlatformsAdd("scummvm", PLATFORM_SCUMMVM, "scummvm --path='$(working-dir)' --auto-detect", "$(install-dir)", "","");
Plt->InstallerPattern=CopyStr(Plt->InstallerPattern, "*.sh");

Plt=PlatformsAdd("gog:scummvm", PLATFORM_GOGSCUMMVM, "scummvm --path='$(working-dir)' --auto-detect", "$(install-dir)/data/noarch/data", "","");

Plt=PlatformsAdd("gog:lin,gog:linux", PLATFORM_GOGLINUX, "", "", "*.x86","*.x86_64");
Plt->InstallerPattern=CopyStr(Plt->InstallerPattern, "*.sh");

Plt=PlatformsAdd("doom", PLATFORM_DOOM, "crispy-doom $(emulator-args) $(wads),chocolate-doom $(emulator-args) $(wads)", "",  "*.wad","");
Plt=PlatformsAdd("spectrum,zx48", PLATFORM_ZXSPECTRUM, "fuse $(exec-path),zesarux $(exec-path)", "",  "*.z80","");
}
