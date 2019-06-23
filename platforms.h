#ifndef SOMMELIER_PLATFORMS_H
#define SOMMELIER_PLATFORMS_H

#include "common.h"

#define PLATFORM_UNKNOWN 0
#define PLATFORM_WINDOWS 1
#define PLATFORM_DOS 2
#define PLATFORM_GO 3
#define PLATFORM_DOOM 4
#define PLATFORM_SCUMMVM    5
#define PLATFORM_GOGLINUX   10
#define PLATFORM_GOGSCUMMVM 11
#define PLATFORM_GOGDOS  12
#define PLATFORM_GOGWINDOS  13

#define PLATFORM_INFO_INSTALL_MESSAGE 1
#define PLATFORM_INFO_EXE_SEARCH_PATTERN 2
#define PLATFORM_INFO_EMULATORS 3
#define PLATFORM_INFO_WORKING_DIR 4

#define PlatformGetInstallMessage(RetStr, Name) (PlatformLookupInfo((RetStr), (Name), PLATFORM_INFO_INSTALL_MESSAGE))
#define PlatformGetExeSearchPattern(RetStr, Name) (PlatformLookupInfo((RetStr), (Name), PLATFORM_INFO_EXE_SEARCH_PATTERN))
#define PlatformGetEmulators(RetStr, Name) (PlatformLookupInfo((RetStr), (Name), PLATFORM_INFO_EMULATORS))
#define PlatformGetWorkingDir(RetStr, Name) (PlatformLookupInfo((RetStr), (Name), PLATFORM_INFO_WORKING_DIR))


typedef struct
{
int ID;
char *Emulators;
char *InstallMessage;
char *WorkingDir;
char *ExeSearchPattern;
char *Exe64SearchPattern;
char *InstallerPattern;
} TPlatform;

int PlatformType(const char *Platform);
char *PlatformLookupInfo(char *RetStr, const char *Name, int Info);
char *PlatformFindEmulator(char *RetStr, char *Name);
char *PlatformFindEmulatorNames(char *RetStr, const char *PlatformName);
char *PlatformSelectForURL(char *RetStr, const char *URL);

void PlatformsInit();

#endif