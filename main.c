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
#include "rebuild-app-file.h"
#include "regedit.h"
#include "wine-fonts.h"
#include "remote-store.h"
#include "sandbox.h"





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


		//apply a level of security to anything we do
		//mount/umount are needed for 
    if (! AppsListAllowSU(Acts)) Tempstr=CopyStr(Tempstr, "nosu nopid security='syscall_deny=group:keyring minimal'");
		if (Config->Flags & CONF_DENY_NET) Tempstr=CatStr(Tempstr, "nonet");

		if (StrValid(Tempstr)) ProcessApplyConfig(Tempstr);

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

            case ACT_CHECK_APPS:
                RebuildAppList(Act);
                break;

            case ACT_REBUILD:
                RebuildAppList(Act);
                break;

            case ACT_REBUILD_HASHES:
                RebuildAppList(Act);
                break;

            case ACT_LIST:
                AppsOutputList(Act);
                break;

            case ACT_LIST_PLATFORMS:
                PlatformsList(Act);
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
                if (! (Config->Flags & CONF_ALLOW_SU)) SetNoSU();
                RunWineUtility(Act, "winecfg");
                break;

            case ACT_REGEDIT:
                if (! (Config->Flags & CONF_ALLOW_SU)) SetNoSU();
                RunWineUtility(Act, "regedit");
                break;

            case ACT_FONTS:
                AppLoadConfig(Act);
                WineFonts(Act);
                break;

            case ACT_ADD_STORE:
                RemoteStoreAdd(Act);
                break;

            case ACT_REFRESH:
                RemoteStoresRefresh(Act);
                break;

            }
        }

        Curr=ListGetNext(Curr);
    }
    Destroy(Tempstr);

    return(0);
}
