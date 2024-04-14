#ifndef SOMMELIER_PLATFORMS_H
#define SOMMELIER_PLATFORMS_H

#include "common.h"

#define PLATFORM_FLAG_NOEXEC 1

#define PLATFORM_UNKNOWN 0
#define PLATFORM_WINDOWS 1
#define PLATFORM_DOS 2
#define PLATFORM_DOOM 4
#define PLATFORM_SCUMMVM    5
#define PLATFORM_GOGLINUX   10
#define PLATFORM_GOGSCUMMVM 11
#define PLATFORM_GOGDOS     12
#define PLATFORM_GOGWINDOS  13
#define PLATFORM_GOGLINUX64 14
#define PLATFORM_GOGNEOGEO  15
#define PLATFORM_LINUX32    32
#define PLATFORM_LINUX64    64
#define PLATFORM_ZXSPECTRUM  90
#define PLATFORM_GENERIC  100

#define PLATFORM_INFO_INSTALL_MESSAGE 1
#define PLATFORM_INFO_EXE_SEARCH_PATTERN 2
#define PLATFORM_INFO_EMULATORS 3
#define PLATFORM_INFO_WORKING_DIR 4
#define PLATFORM_INFO_EXE64_SEARCH_PATTERN 64

#define PlatformGetInstallMessage(RetStr, Name) (PlatformLookupInfo((RetStr), (Name), PLATFORM_INFO_INSTALL_MESSAGE))
#define PlatformGetExeSearchPattern(RetStr, Name) (PlatformLookupInfo((RetStr), (Name), PLATFORM_INFO_EXE_SEARCH_PATTERN))
#define PlatformGetExe64SearchPattern(RetStr, Name) (PlatformLookupInfo((RetStr), (Name), PLATFORM_INFO_EXE64_SEARCH_PATTERN))
#define PlatformGetEmulators(RetStr, Name) (PlatformLookupInfo((RetStr), (Name), PLATFORM_INFO_EMULATORS))
#define PlatformGetWorkingDir(RetStr, Name) (PlatformLookupInfo((RetStr), (Name), PLATFORM_INFO_WORKING_DIR))

#define PlatformFind(Name) (PlatformFindWithEmulator((Name), "",  FALSE))
#define PlatformFindAvailable(Name) (PlatformFindWithEmulator((Name), "", TRUE))

typedef struct
{
    int Flags;
    int ID;
		char *Name;
    char *Emulators;
    char *InstallMessage;
    char *WorkingDir;
    char *UnpackDir;
    char *ExeSearchPattern;
    char *Exe64SearchPattern;
    char *InstallerPattern;
    char *Args;
} TPlatform;

const char *PlatformDefault();
TPlatform *PlatformFindWithEmulator(const char *Name, const char *Emulator, int CheckAvailable);
const char *PlatformUnAlias(const char *Alias);
int PlatformType(const char *Platform);
int PlatformBitWidth(const char *Platform);
char *PlatformLookupInfo(char *RetStr, const char *Name, int Info);
char *PlatformFindEmulator(char *RetStr, const char *Name, const char *Emulator);
char *PlatformFindEmulatorNames(char *RetStr, const char *PlatformName);
void PlatformSetupEmulatorArgs(TAction *Act, const char *Name, const char *Args);
char *PlatformSelect(char *RetStr, TAction *Act);
void PlatformsList();



void PlatformApplySettings(TAction *Act);
void PlatformsInit(const char *Path);

#endif
