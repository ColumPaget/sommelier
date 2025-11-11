#include <wait.h>
#include <glob.h>

#include "common.h"
#include "config.h"
#include "desktopfiles.h"
#include "install.h"
#include "uninstall.h"
#include "command-line.h"
#include "download.h"
#include "platforms.h"
#include "categories.h"
#include "apps.h"
#include "run-application.h"
#include "regedit.h"
#include "wine-fonts.h"
#include "sandbox.h"



int RebuildApp(TAction *Act)
{
    if ((Act->Type==ACT_REBUILD_HASHES) && (! (Act->Flags & FLAG_FORCE)))
    {
        if (StrValid(GetVar(Act->Vars, "sha256"))) return(TRUE);
    }

    return(DownloadCheck(Act));
}




void RebuildAppListFile(TAction *RebuildAct, const char *Path)
{
    STREAM *In, *Out;
    char *Tempstr=NULL;
    const char *ptr;
    ListNode *Curr;
    TAction *Act;
    const char *IgnoreVars[]= {"name", "sommelier_root_template","prefix_template","homedir","url-basename","url-path",NULL};


    Out=STREAMFromFD(1);
    In=STREAMOpen(Path, "r");
    if (In)
    {
        Tempstr=STREAMReadLine(Tempstr, In);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            if ((StrLen(Tempstr) > 0) && (*Tempstr != '#'))
            {
                Act=ActionCreate(RebuildAct->Type, "");
                Act->Flags=RebuildAct->Flags;
                ptr=GetToken(Tempstr, " ", &Act->Name, 0);
                LoadAppConfigToAct(Act, ptr);

                if (StrLen(GetVar(Act->Vars,"name"))==0) SetVar(Act->Vars, "name", Act->Name);
                if (! StrValid(Act->URL)) printf("%s\n", Tempstr);
                else if (RebuildApp(Act))
                {
                    Tempstr=MCopyStr(Tempstr, Act->Name, " ", NULL);
                    if (StrValid(Act->Platform)) Tempstr=MCatStr(Tempstr, "platform=", Act->Platform, " ", NULL);
                    Tempstr=MCatStr(Tempstr, "url=", Act->URL, " ", NULL);
                    Curr=ListGetNext(Act->Vars);
                    while (Curr)
                    {
                        if (MatchTokenFromList(Curr->Tag, IgnoreVars, 0) ==-1) Tempstr=MCatStr(Tempstr, Curr->Tag, "='", (char *) Curr->Item,"' ",NULL);
                        Curr=ListGetNext(Curr);
                    }
                    printf("%s\n",Tempstr);
                    fflush(NULL);
                }
                else printf("ERROR: Rebuild Failed: %s\n", Tempstr);

                ActionDestroy(Act);
            }
            else printf("%s\n", Tempstr);
            Tempstr=STREAMReadLine(Tempstr, In);
        }
        STREAMClose(In);
    }

    STREAMDestroy(Out);
    Destroy(Tempstr);
}



void RebuildAppList(TAction *RebuildAct)
{
    char *FileList=NULL, *Path=NULL;
    char *Tempstr=NULL;
    const char *ptr;

    ptr=GetToken(RebuildAct->Args, "\\S", &Path, GETTOKEN_QUOTES);
    while (ptr)
    {
        if (StrValid(Path)) Tempstr=MCatStr(Tempstr, Path, ",", NULL);
        ptr=GetToken(ptr, "\\S", &Path, GETTOKEN_QUOTES);
    }

    FileList=AppsListExpand(FileList, Tempstr);
    ptr=GetToken(FileList, ",", &Path, 0);
    while (ptr)
    {
        RebuildAppListFile(RebuildAct, Path);
        ptr=GetToken(ptr, ",", &Path, 0);
    }

    Destroy(Tempstr);
    Destroy(FileList);
    Destroy(Path);
}



//there's a chicken-and-egg situation in parsing the command-line, as some things depend on config files that
//can be changed by command-line switches. We do those things here, as early as possible after loading config files
void ActionPrepare(TAction *Act)
{
    char *Tempstr=NULL;

    if (StrValid(Act->Platform))
    {
        Tempstr=CopyStr(Tempstr, Act->Platform);
        Act->Platform=CopyStr(Act->Platform, PlatformUnAlias(Tempstr));
        //we must only set the platform variable if the user has specified a platform,
        //as this is how we distinguish between 'the user insists this platform'
        //and 'dunno, go with the first thing you can find'
        SetVar(Act->Vars, "platform", Act->Platform);
    }
    //PlatformDefault will tend to be something like "!linux32" rather than an
    //actual platform, so we mustn't set the 'platform' variable to this
    else Act->Platform=CopyStr(Act->Platform, PlatformDefault());

    Destroy(Tempstr);
}



int main(int argc, char *argv[])
{
    ListNode *Acts, *Curr;
    TAction *Act;
    char *Tempstr=NULL;

    SetupCurrUser();
    ConfigInit();

    Acts=ParseCommandLine(argc, argv);

    PlatformsInit(Config->PlatformsPath);
    CategoriesLoad(Config->CategoriesPath);
    AppsLoad(Config->AppConfigPath);

    Curr=ListGetNext(Acts);
    while (Curr)
    {
        Act=(TAction *) Curr->Item;
        ActionPrepare(Act);

        if (Act)
        {

            switch (Act->Type)
            {
            case ACT_INSTALL:
                if (! AppLoadConfig(Act)) printf("ERROR: no config found for app '%s'\n", Act->Name);
                else InstallApp(Act);
                break;

            case ACT_RECONFIGURE:
                if (! AppLoadConfig(Act)) printf("ERROR: no config found for app '%s'\n", Act->Name);
                else InstallReconfigure(Act);
                break;

            case ACT_UNINSTALL:
                AppLoadConfig(Act);
                UnInstallApp(Act);
                break;

            case ACT_SET:
                if (! AppLoadConfig(Act)) printf("ERROR: no config found for app '%s'\n", Act->Name);
                else PlatformApplySettings(Act);
                break;

            case ACT_REBUILD_HASHES:
                RebuildAppList(Act);
                break;

            case ACT_REBUILD:
                RebuildAppList(Act);
                break;

            case ACT_LIST:
                AppsOutputList(Act);
                break;

            case ACT_LIST_PLATFORMS:
                PlatformsList();
                break;

            case ACT_LIST_CATEGORIES:
                CategoriesList();
                break;

            case ACT_DOWNLOAD:
                if (! AppLoadConfig(Act)) printf("ERROR: no config found for app '%s'\n", Act->Name);
                else Download(Act);
                break;

            case ACT_RUN:
                RunApplicationFromDesktopFile(Act);
                break;

            case ACT_AUTOSTART:
                Tempstr=MCopyStr(Tempstr, GetCurrUserHomeDir(), "/.config/autostart/", NULL);
                DesktopFileDirectoryRunAll(Tempstr);
                break;

            case ACT_WINECFG:
                if (! (Config->Flags & FLAG_ALLOW_SU)) SetNoSU();
                RunWineUtility(Act, "winecfg");
                break;

            case ACT_REGEDIT:
                if (! (Config->Flags & FLAG_ALLOW_SU)) SetNoSU();
                RunWineUtility(Act, "regedit");
                break;

            case ACT_FONTS:
                AppLoadConfig(Act);
                WineFonts(Act);
                break;
            }
        }

        Curr=ListGetNext(Curr);
    }
    Destroy(Tempstr);

    return(0);
}
