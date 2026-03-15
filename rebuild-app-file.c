#include "rebuild-app-file.h"
#include "config.h"
#include "apps.h"
#include "download.h"

static void CheckApp(TAction *Act)
{
    char *Tempstr=NULL, *Reason=NULL;

    if (StrValid(Act->URL))
    {
        LibUsefulSetValue("Error:Silent", "Y");

        if (DownloadCheck(Act, &Reason)) Tempstr=MCopyStr(Tempstr, Act->Name, " ~gOK~0\n", NULL);
        else Tempstr=MCopyStr(Tempstr, Act->Name, " ~rFAIL~0: ", Reason, "\n", NULL);

        TerminalPutStr(Tempstr, NULL);
    }

    Destroy(Tempstr);
    Destroy(Reason);
}


static int RebuildApp(TAction *Act)
{
    const char *ptr;

    if ((Act->Type==ACT_REBUILD_HASHES) && (! (Act->Flags & FLAG_FORCE)))
    {
        ptr=GetVar(Act->Vars, "actual-sha256");
        if (StrValid(ptr)) SetVar(Act->Vars, "sha256", ptr);


        if (StrValid(GetVar(Act->Vars, "sha256"))) return(TRUE);
    }

    return(DownloadCheck(Act, NULL));
}



static void RebuildOutputApp(TAction *Act, const char *IgnoreVars[])
{
    char *Tempstr=NULL;
    const char *ptr;
    ListNode *Curr;


    ptr=GetVar(Act->Vars, "actual-sha256");
    if (StrValid(ptr)) SetVar(Act->Vars, "sha256", ptr);

    Tempstr=MCopyStr(Tempstr, Act->Name, " ", NULL);
    if (StrValid(Act->Platform)) Tempstr=MCatStr(Tempstr, "platform=", Act->Platform, " ", NULL);
    Tempstr=MCatStr(Tempstr, "url=\"", Act->URL, "\" ", NULL);
    Curr=ListGetNext(Act->Vars);
    while (Curr)
    {
        if (StrValid(Curr->Item))
        {
            if (MatchTokenFromList(Curr->Tag, IgnoreVars, 0) ==-1) Tempstr=MCatStr(Tempstr, Curr->Tag, "=\"", (char *) Curr->Item,"\" ",NULL);
        }

        Curr=ListGetNext(Curr);
    }
    printf("%s\n",Tempstr);
    fflush(NULL);

    Destroy(Tempstr);
}


static void RebuildAppListFile(TAction *RebuildAct, const char *Path)
{
    STREAM *In, *Out;
    char *Tempstr=NULL;
    const char *ptr;
    TAction *Act;
    const char *IgnoreVars[]= {"name", "sommelier_root_template","sommelier_root","prefix_template","homedir","url-basename","url-path","user","actual-sha256","dlfile",  NULL};


    Out=STREAMFromFD(1);
    In=STREAMOpen(Path, "r");
    if (In)
    {
        Tempstr=STREAMReadLine(Tempstr, In);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            if ((StrValid(Tempstr)) && (*Tempstr != '#'))
            {
                //fprintf(stderr, "REBUILD: [%s]\n", Tempstr);
                Act=ActionCreate(RebuildAct->Type, "");
                Act->Flags=RebuildAct->Flags;
                ptr=GetToken(Tempstr, " ", &Act->Name, 0);
                LoadAppConfigToAct(Act, ptr);

                if (StrLen(GetVar(Act->Vars,"name"))==0) SetVar(Act->Vars, "name", Act->Name);


                if (Act->Type==ACT_CHECK_APPS) CheckApp(Act);
                else if (! StrValid(Act->URL)) printf("%s\n", Tempstr);
                else if (RebuildApp(Act)) RebuildOutputApp(Act, IgnoreVars);
                else printf("ERROR: Rebuild Failed: %s\n", Tempstr);

                if (! (Config->Flags & FLAG_KEEP_INSTALLER)) unlink(Act->SrcPath);
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


