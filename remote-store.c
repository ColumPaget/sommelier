#include "remote-store.h"
#include "apps.h"

static int RemoteStoreGetAppFile(TAction *Act, const char *URL)
{
    char *Template=NULL, *Tempstr=NULL, *Path=NULL;
    ListNode *Apps=NULL;
    const char *ptr;
    STREAM *In, *Out;
    int RetVal=FALSE;


    In=STREAMOpen(URL, "r nostderr");
    if (In == NULL)
    {
        Tempstr=FormatStr(Tempstr, "~rFAIL~0: cannot open url '%s'\n", URL);
        TerminalPutStr(Tempstr, NULL);

        return(FALSE);
    }

    Tempstr=CopyStr(Tempstr, URL);
    strrep(Tempstr, '/','_');
    strrep(Tempstr, ':','_');

    Template=MCopyStr(Template, "$(sommelier_root)/apps/", Tempstr, NULL);
    Path=SubstituteVarsInString(Path, Template, Act->Vars, 0);
    Out=STREAMOpen(Path, "w");
    if (Out)
    {
        Template=CopyStr(Template, URL);
        StrRTruncChar(Template, '/');
        Tempstr=MCopyStr(Tempstr, "! server_path=\"", Template, "\" server_apps_file=\"", URL, "\"\n\n", NULL);
        STREAMWriteLine(Tempstr, Out);

        STREAMSendFile(In, Out, 0, SENDFILE_LOOP);
        STREAMClose(Out);

        Apps=ListCreate();
        AppsLoadFromFile(Path, Apps);
        Tempstr=FormatStr(Tempstr, "~gOKAY~0: Apps File installed from '~e%s~0'. ~e%d~0 apps registered.\n", URL, ListSize(Apps));
        TerminalPutStr(Tempstr, NULL);
    }
    else
    {
        Tempstr=FormatStr(Tempstr, "~rFAIL~0: cannot write to file '%s'\n", Path);
        TerminalPutStr(Tempstr, NULL);
    }

    STREAMClose(In);

    Destroy(Template);
    Destroy(Tempstr);
    Destroy(Path);

    return(RetVal);
}


int RemoteStoreAdd(TAction *Act)
{
    char *URL=NULL;
    const char *ptr;

    ptr=GetToken(Act->Args, "\\S", &URL, GETTOKEN_QUOTES);
    while (ptr)
    {
        RemoteStoreGetAppFile(Act, URL);
        ptr=GetToken(ptr, "\\S", &URL, GETTOKEN_QUOTES);
    }

    Destroy(URL);
}



int RemoteStoresRefresh(TAction *Act)
{
    ListNode *ServerPaths, *Curr;
    const char *ptr;
    TAction *App;

    ServerPaths=ListCreate();
    Curr=ListGetNext(AppsGetList());
    while (Curr)
    {
        App=(TAction *) Curr->Item;
        ptr=GetVar(App->Vars, "server_apps_file");
        if (StrValid(ptr)) SetVar(ServerPaths, ptr, "");

        Curr=ListGetNext(Curr);
    }

    Curr=ListGetNext(ServerPaths);
    while (Curr)
    {
        RemoteStoreGetAppFile(Act, Curr->Tag);
        Curr=ListGetNext(Curr);
    }

    ListDestroy(ServerPaths, Destroy);
}
