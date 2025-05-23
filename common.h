
#ifndef SOMMELIER_COMMON_H
#define SOMMELIER_COMMON_H

#if defined(USE_LIBUSEFUL_BUNDLED)
#include "libUseful-bundled/libUseful.h"
#elif defined(HAVE_LIBUSEFUL5)
#include "libUseful-5/libUseful.h"
#else
#include "libUseful-4/libUseful.h"
#endif

#include <wait.h>
#include <glob.h>


#define VERSION "10.5"


#define INSTALL_RUN          0
#define INSTALL_UNPACK       1
#define INSTALL_EXECUTABLE   2

#define ACT_NONE             0
#define ACT_INSTALL          1
#define ACT_UNINSTALL        2
#define ACT_RUN              3
#define ACT_LIST             5
#define ACT_LIST_PLATFORMS   6
#define ACT_LIST_CATEGORIES  7
#define ACT_SET              8
#define ACT_REBUILD         13
#define ACT_REBUILD_HASHES  14
#define ACT_DOWNLOAD        15
#define ACT_RECONFIGURE     16
#define ACT_WINECFG         17
#define ACT_AUTOSTART       18

#define FLAG_DEBUG            1
#define FLAG_FORCE            2
#define FLAG_DEPENDANCY       4
#define FLAG_SYSTEM_INSTALL   8
#define FLAG_INSTALLED       16
#define FLAG_NO_XRANDR       32
#define FLAG_SANDBOX         64
#define FLAG_NET            128
#define FLAG_DOWNLOADED     256
#define FLAG_KEEP_INSTALLER 512
#define FLAG_HASH_DOWNLOAD 1024
#define FLAG_ABORT         2048
#define FLAG_BUNDLED       4096
#define FLAG_NOEXEC        8192
#define FLAG_DLC          16384
#define FLAG_ALLOWSU      32768

typedef enum {FILETYPE_UNKNOWN, FILETYPE_MZ, FILETYPE_PE, FILETYPE_ZIP, FILETYPE_MSI, FILETYPE_RAR, FILETYPE_TGZ, FILETYPE_TBZ, FILETYPE_TXZ, FILETYPE_7ZIP, FILETYPE_DEB, FILETYPE_CAB} TEnumFileTypes;

typedef struct
{
    int Type;
    int Flags;
    int InstallType;
    int PlatformID;
    char *Name;
    char *URL;
    char *Root;
    char *DownName;
    char *InstallName;
    char *InstallPath;
    char *SrcPath;
    char *Exec;
    char *Exec64;
    char *Args;
    char *Platform;
    char *OSVersion;
    char *Parent;
    char *PostProcess;
    ListNode *Vars;
} TAction;


extern char *CurrUserName;
extern char *CurrUserHome;

void SetupCurrUser();
const char *ResolveVar(ListNode *Vars, const char *VarName);
char *FormatPath(char *RetStr, const char *Fmt);
char *URLBasename(char *RetStr, const char *URL);
int InList(const char *Item, const char *List);
TAction *ActionCreate(int Type, const char *Name);
void ActionDestroy(TAction *Act);
int CompareSha256(TAction *Act);


int IdentifyFileType(const char *Path, int ForcedFileType);

//Some installers fork into background, perhaps calling 'setsid', which means we
//will no longer consider them child processes and will no longer wait for them.
//Holding open a pipe for their output seems to overcome this, and also allows us
//to suppress a lot of crap that they might print out.
void RunProgramAndConsumeOutput(const char *Cmd, const char *SpawnConfig);

int ParseBool(const char *Value);
int GetBoolVar(ListNode *Vars, const char *Name);
char *GlobNoCase(char *RetStr, const char *Dir, const char *Name);

#endif
