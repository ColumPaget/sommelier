#include "libUseful-4/libUseful.h"
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
#include "apps.h"



//#define DESKTOP_PATH "/usr/share/applications/"
#define DESKTOP_PATH "$(homedir)/.local//share/applications/"




void RunApplication(TAction *Act)
{
    const char *ptr;
    char *SpawnConfig=NULL, *Tempstr=NULL;

    if (DesktopFileRead(Act))
    {

        if (StrValid(Act->Exec))
        {
            if (Act->Flags & FLAG_SANDBOX)
            {
                seteuid(0);
                SpawnConfig=FormatStr(SpawnConfig, "noshell +stderr container=/tmp/container/ uid=%d gid=%d dir='%s' ", getuid(), getgid(), GetVar(Act->Vars,"working-dir"));

                if (Act->Flags & FLAG_NET) SpawnConfig=CatStr(SpawnConfig, "+net ");
                else SpawnConfig=CatStr(SpawnConfig, "-net ");

                SpawnConfig=MCatStr(SpawnConfig,"setenv='LD_LIBRARY_PATH=/lib:/usr/lib:/opt/wine-3.5_GL/lib' ",NULL);

                SetVar(Act->Vars,"path",getenv("PATH"));
                SetVar(Act->Vars,"display",getenv("DISPLAY"));
                Tempstr=SubstituteVarsInString(Tempstr, "setenv='WINEPREFIX=$(prefix)' setenv='DISPLAY=$(display)' setenv='PATH=$(path)'",Act->Vars, 0);
                SpawnConfig=MCatStr(SpawnConfig, Tempstr," ",NULL);

                Tempstr=SubstituteVarsInString(Tempstr, "mnt=/etc/,/bin,/lib,/usr,/opt/wine-3.5_GL wmnt=$(prefix),/dev",Act->Vars,0);
                SpawnConfig=MCatStr(SpawnConfig, Tempstr," ",NULL);

                SpawnConfig=MCatStr(SpawnConfig, "plink=/tmp/.X11-unix/X0", " ", NULL);
                //SpawnConfig=MCatStr(SpawnConfig, "pclone=/dev/urandom,/dev/dsp,/dev/mixer", " ", NULL);

                /*
                if (Settings.Flags & FLAG_ISOCUBE) SpawnConfig=MCatStr(SpawnConfig, "isocube=",Settings.Dir," ",NULL);
                else SpawnConfig=MCatStr(SpawnConfig, "container=",Settings.Dir," ",NULL);

                if (StrValid(Settings.FileClones)) SpawnConfig=MCatStr(SpawnConfig, "pclone=",Settings.FileClones, " ", NULL);
                if (StrValid(Settings.JailSetup)) SpawnConfig=MCatStr(SpawnConfig, "jailsetup=",Settings.JailSetup, " ",NULL);
                */

                Tempstr=CopyStr(Tempstr, Act->Exec);
                if (! (Config->Flags & FLAG_DEBUG)) Tempstr=CatStr(Tempstr, " >/dev/null");
                Spawn(Tempstr, SpawnConfig);
            }
            else
            {
                ptr=GetVar(Act->Vars, "working-dir");
                if (StrValid(ptr))
                {
                    if (chdir(ptr) !=0) perror("ERROR switching to directory: ");
                }

                printf("Running '%s' (%s)\n", Act->Name, Act->Exec);

                SetVar(Act->Vars, "exec", Act->Exec);
                SetVar(Act->Vars, "exec-args", Act->Args);
                Tempstr=SubstituteVarsInString(Tempstr, "WINEPREFIX=$(prefix) $(exec) $(exec-args)",Act->Vars, 0);
                if (! (Config->Flags & FLAG_DEBUG)) Tempstr=CatStr(Tempstr, " >/dev/null");
                SpawnConfig=CopyStr(SpawnConfig, "+stderr");
                Spawn(Tempstr, SpawnConfig);
            }
        }
        else fprintf(stderr, "ERROR: Failed to open .desktop file for application\n");
    }

    Destroy(Tempstr);
    Destroy(SpawnConfig);
}






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
    const char *IgnoreVars[]={"name", "sommelier_root_template","prefix_template","homedir","url-basename","url-path",NULL};


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


int main(int argc, char *argv[])
{
    ListNode *Acts, *Curr;
    TAction *Act;

//we will not need root permissions unless we sandbox, and we'll reclaim them as needed then
    if (geteuid()==0) seteuid(getuid());


//LibUsefulSetValue("HTTP:UserAgent","Mozilla/5.0 (Windows NT 6.3; Trident/7.0; rv:11.0) like Gecko");
    LibUsefulSetValue("HTTP:UserAgent","Wget/1.19.2");
//LibUsefulSetValue("HTTP:Debug", "Y");

    ConfigInit();

    Acts=ParseCommandLine(argc, argv);
    PlatformsInit(Config->PlatformsPath);
    AppsLoad(Config->AppConfigPath);

    Curr=ListGetNext(Acts);
    while (Curr)
    {
        Act=(TAction *) Curr->Item;
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
                UnInstallApp(Act);
                break;

            case ACT_SET:
                if (! AppLoadConfig(Act)) printf("ERROR: no config found for app '%s'\n", Act->Name);
                else PlatformApplySettings(Act);
                break;

            case ACT_RUN:
                RunApplication(Act);
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

            case ACT_DOWNLOAD:
                if (! AppLoadConfig(Act)) printf("ERROR: no config found for app '%s'\n", Act->Name);
                else Download(Act);
                break;
            }
        }

        Curr=ListGetNext(Curr);
    }

    return(0);
}
