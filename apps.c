#include "apps.h"
#include "config.h"
#include <fnmatch.h>

ListNode *Apps=NULL;

ListNode *AppsGetList()
{
    return(Apps);
}



int AppPlatformMatches(TAction *App, const char *Platforms)
{
    if (! StrValid(Platforms)) return (TRUE);
    if (InList(App->Platform, Platforms)) return(TRUE);

    if ( (PlatformType(App->Platform)==PLATFORM_WINDOWS) && InList("windows", Platforms) ) return(TRUE);

    return(FALSE);
}



int AppMatches(TAction *Template, TAction *App)
{
    int result=FALSE;
    char *Token=NULL;
    const char *ptr;

    if (! AppPlatformMatches(App, Template->Platform) ) return(FALSE);

    /* doesn't yet exist!
    if (StrValid(Template->Category))
    {
    	ptr=GetToken(App->Category, ";", &Token, 0)
    	while (ptr)
    	{
    		if (strcasecmp(Token, Template->Category)==0) result=TRUE;
    	ptr=GetToken(ptr, ";", &Token, 0)
    	}
    }
    */

    Destroy(Token);
    return(TRUE);
}

void LoadAppConfigToAct(TAction *Act, const char *Config)
{
    char *Name=NULL, *Value=NULL, *Tempstr=NULL;
    const char *ptr;
    TPlatform *Plt;

    ptr=GetNameValuePair(Config," ", "=", &Name, &Value);
    while (ptr)
    {
        StripQuotes(Name);
        StripQuotes(Value);

        if (StrValid(Name))
        {

            if (strcmp(Name,"url")==0)
            {
                Act->URL=CopyStr(Act->URL, Value);
                Tempstr=URLBasename(Tempstr, Act->URL);
                SetVar(Act->Vars, "url-basename", Tempstr);
                Tempstr=CopyStr(Tempstr, Act->URL);
                StrRTruncChar(Tempstr, '?');
                StrRTruncChar(Tempstr, '/');
                SetVar(Act->Vars, "url-path", Tempstr);
            }
            else if (strcmp(Name,"platform")==0)
            {
                Act->Platform=CopyStr(Act->Platform, Value);
                Plt=PlatformFind(Act->Platform);
                if (Plt)
                {
                    Act->PlatformID=Plt->ID;
                    if (Plt->Flags & PLATFORM_FLAG_NOEXEC) Act->Flags |= FLAG_NOEXEC;
                    if (StrValid(Plt->UnpackDir)) SetVar(Act->Vars, "unpack-dir", Plt->UnpackDir);
                }
                else fprintf(stderr, "PLATFORM NOT FOUND: %s\n", Act->Platform);
            }
            //locale is a special case, we want to be able to use 'locale=' to set a locale string, but we
            //also want to be able to use $(locale) as a variable in config. Further, we want to be able to say
            //'locale="$(lang):$(country)". But when we do the substitution and set locale to it's final value, we
            //destroy the variables by subsituting them. So if we want to try a different locale in future, we now
            //can't do that. Thus we set 'locale_template' and use that as the template to substitute into 'locale',
            //so 'locale_template' always reamins unchanged.
            else if (strcmp(Name, "locale")==0) SetVar(Act->Vars, "locale_template", Value);
            else SetVar(Act->Vars, Name, Value);



            if (strcmp(Name,"install-path")==0) Act->InstallPath=CopyStr(Act->InstallPath, Value);
            else if (strcmp(Name,"install-name")==0) Act->InstallName=CopyStr(Act->InstallName, Value);
            else if (strcmp(Name,"dlname")==0) Act->DownName=CopyStr(Act->DownName, Value);
            else if (strcmp(Name,"install-type")==0)
            {
                if (strcasecmp(Value,"unpack")==0) Act->InstallType=INSTALL_UNPACK;
                if (strcasecmp(Value,"executable")==0) Act->InstallType=INSTALL_EXECUTABLE;
            }
            else if (strcmp(Name,"sha256")==0)
            {
                strlwr(Value);
                SetVar(Act->Vars, Name, Value);
            }
            else if (strcmp(Name,"bundled")==0)
            {
                //app is bundled with another app! Set the bundled flag, and set a variable
                Act->Flags |= FLAG_BUNDLED;
                SetVar(Act->Vars, "bundled-with", Value);
            }
        }

        ptr=GetNameValuePair(ptr," ", "=", &Name, &Value);
    }

    Destroy(Tempstr);
    Destroy(Value);
    Destroy(Name);
}


static TAction *AppConfigure(const char *Name, const char *Settings, ListNode *FileWideSettings)
{
    ListNode *Curr;
    TAction *Act;
    char *Config=NULL;

    Act=ActionCreate(ACT_NONE, Name);
    LoadAppConfigToAct(Act, Settings);
    Curr=ListGetNext(FileWideSettings);
    while (Curr)
    {
        if (strcmp(Curr->Tag, "*")==0) Config=MCatStr(Config, (const char *) Curr->Item, " ", NULL);

        //it's possible for an app to not have a url if it comes bundled with something else, so be sure to
        //check Act->URL exists before feeding to fnmatch
        if (Act->URL &&  (strncmp(Curr->Tag, "url=",4)==0) && (fnmatch(Curr->Tag+4, Act->URL, 0)==0) ) Config=MCatStr(Config, (const char *) Curr->Item, " ", NULL);
        Curr=ListGetNext(Curr);
    }
    Config=CatStr(Config, Settings);

    LoadAppConfigToAct(Act, Config);
    Destroy(Config);

    return(Act);
}



void AppsLoadFromFile(const char *Path, ListNode *Apps)
{
    STREAM *S;
    ListNode *Node;
    TAction *Act;
    char *Tempstr=NULL, *Token=NULL, *AppConfig=NULL;
    ListNode *FileWideSettings;
    const char *ptr;

    FileWideSettings=ListCreate();
    S=STREAMOpen(Path, "r");
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            if ((StrLen(Tempstr) > 0) && (*Tempstr != '#'))
            {
                ptr=GetToken(Tempstr, "\\S", &Token, GETTOKEN_QUOTES);
                // '*' for the app name means this line applies to all apps in this file
                if (
                    (strcmp(Token, "*")==0) ||
                    (strncmp(Token, "url=", 4)==0)
                ) SetVar(FileWideSettings, Token, ptr);
                else if (StrValid(Token))
                {
                    Act=AppConfigure(Token, ptr, FileWideSettings);
                    ListAddNamedItem(Apps, Token, Act);
                }
            }
            Tempstr=STREAMReadLine(Tempstr, S);
        }

        STREAMClose(S);
    }

    ListDestroy(FileWideSettings, Destroy);
    Destroy(AppConfig);
    Destroy(Tempstr);
    Destroy(Token);
}


//bundles are situations were more than one application is supplied in the same package
void AppAddToBundle(ListNode *Apps, TAction *App, const char *ParentName)
{
    ListNode *Curr;
    TAction *Parent;
    char *Tempstr=NULL;

//find the parent app/install that this item is bundled with
    Curr=ListFindNamedItem(Apps, ParentName);
    while (Curr)
    {
        if (strcmp(Curr->Tag, ParentName)==0)
        {
            Parent=(TAction *) Curr->Item;
            Tempstr=CopyStr(Tempstr, GetVar(Parent->Vars, "bundles"));
            Tempstr=MCatStr(Tempstr, " ", App->Name, NULL);
            SetVar(Parent->Vars, "bundles", Tempstr);
        }
        Curr=ListGetNext(Curr);
    }

    Destroy(Tempstr);
}


void AppsProcessBundles(ListNode *Apps)
{
    ListNode *Curr;
    TAction *App;
    const char *ptr;

    Curr=ListGetNext(Apps);
    while (Curr)
    {
        App=(TAction *) Curr->Item;
        if (App->Flags & FLAG_BUNDLED)
        {
            ptr=GetVar(App->Vars, "bundled-with");
            if (StrValid(ptr)) AppAddToBundle(Apps, App, ptr);
        }
        Curr=ListGetNext(Curr);
    }

}



char *AppsListExpand(char *FileList, const char *ConfigFiles)
{
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;
    glob_t Glob;
    int i;

    FileList=CopyStr(FileList, "");
    Tempstr=FormatPath(Tempstr, ConfigFiles);
    ptr=GetToken(Tempstr, ",", &Token, 0);
    while (ptr)
    {
        glob(Token, 0, 0, &Glob);
        for (i=0; i < Glob.gl_pathc; i++) FileList=MCatStr(FileList, Glob.gl_pathv[i], ",", NULL);
        globfree(&Glob);
        ptr=GetToken(ptr, ",", &Token, 0);
    }

    Destroy(Tempstr);
    Destroy(Token);

    return(FileList);
}



ListNode *AppsLoad(const char *ConfigFiles)
{
    char *FileList=NULL, *Token=NULL;
    const char *ptr;

    if (! Apps) Apps=ListCreate();

    FileList=AppsListExpand(FileList, ConfigFiles);

    ptr=GetToken(FileList, ",", &Token, 0);
    while (ptr)
    {
        AppsLoadFromFile(Token, Apps);
        ptr=GetToken(ptr, ",", &Token, 0);
    }

    AppsProcessBundles(Apps);

    Destroy(FileList);
    Destroy(Token);

    return(Apps);
}



char *AppFormatPath(char *Path, TAction *Act)
{
    char *Tempstr=NULL;
    const char *ptr;

//first generate sommelier root (usually ~/.sommelier) if it doesn't exist

    ptr=GetVar(Act->Vars, "sommelier_root");
    if (! StrValid(ptr))
    {
        if (Config->Flags & FLAG_SYSTEM_INSTALL) Path=SubstituteVarsInString(Path, "/opt/", Act->Vars, 0);
        else Path=SubstituteVarsInString(Path, "$(homedir)/.sommelier/", Act->Vars, 0);
        SetVar(Act->Vars, "sommelier_root",Path);
    }

    Tempstr=MCopyStr(Tempstr, GetVar(Act->Vars, "sommelier_root"), "/patches", NULL);
    SetVar(Act->Vars, "sommelier_patches_dir", Tempstr);

    ptr=GetVar(Act->Vars, "prefix");
    if (! StrValid(ptr))
    {
        Tempstr=CopyStr(Tempstr, GetVar(Act->Vars, "prefix_template"));
        Path=SubstituteVarsInString(Path, Tempstr, Act->Vars, 0);
        Path=SlashTerminateDirectoryPath(Path);
        SetVar(Act->Vars, "prefix",Path);
    }
    else Path=CopyStr(Path,ptr);


//for dos and golang/go path==prefix
//for wine path is more complex
    if (Act->PlatformID==PLATFORM_WINDOWS)
    {
        Path=CatStr(Path,"drive_c/");
        SetVar(Act->Vars, "drive_c",Path);

        if (StrValid(Act->InstallPath)) Path=SubstituteVarsInString(Path, Act->InstallPath, Act->Vars, 0);
        else Path=SubstituteVarsInString(Path, "$(drive_c)/Program Files/$(name)", Act->Vars, 0);
    }
    else
    {
        SetVar(Act->Vars, "drive_c",Path);
        SetVar(Act->Vars, "exec-dir", Path);
    }

    Path=SlashTerminateDirectoryPath(Path);
    SetVar(Act->Vars, "install-dir",Path);

    DestroyString(Tempstr);
    return(Path);
}


int AppFindConfig(TAction *App, const char *Platforms)
{
    TAction *AppConfig;
    ListNode *Curr;
    int result=FALSE;


    Curr=ListGetNext(Apps);
    while (Curr)
    {
        if (StrValid(Curr->Tag) && (strcmp(App->Name, Curr->Tag)==0))
        {
            AppConfig=(TAction *) Curr->Item;

            //if no platform requested, or app platform matches requested
            //then we've found the right one

            if (AppPlatformMatches(AppConfig, Platforms))
            {
                //flags can already have been set by the command-line, so
                //we have to | these
                App->Flags |= AppConfig->Flags;
                App->InstallType |= AppConfig->InstallType;
                App->Platform=CopyStr(App->Platform, AppConfig->Platform);
                App->PlatformID=AppConfig->PlatformID;
                if (! StrValid(App->URL)) App->URL=CopyStr(App->URL, AppConfig->URL);
                CopyVars(App->Vars, AppConfig->Vars);
                result=TRUE;
                break;
            }
        }

        Curr=ListGetNext(Curr);
    }

    return(result);
}




void AppSetLocale(TAction *App, const char *LocaleStr)
{
    char *Lang=NULL, *Country=NULL;
    const char *ptr;

    ptr=GetToken(LocaleStr, "_", &Lang, 0);
    Country=CopyStr(Country, ptr);
    StrRTruncChar(Country, '.');

    SetVar(App->Vars, "lang", Lang);
    SetVar(App->Vars, "country", Country);
    strlwr(Country);
    SetVar(App->Vars, "country:lwr", Country);

    ptr=GetVar(App->Vars, "locale_template");
    if (StrValid(ptr))
    {
        Lang=SubstituteVarsInString(Lang, ptr, App->Vars, 0);
        SetVar(App->Vars, "locale", Lang);
    }

    Destroy(Country);
    Destroy(Lang);
}



int AppLoadConfig(TAction *App)
{
    int result=FALSE;
    char *Tempstr=NULL;
    const char *ptr;

    Tempstr=PlatformSelect(Tempstr, App);
    if (StrValid(Tempstr)) printf("Selected Platforms: %s\n", Tempstr);

//if no platform specified this will use the first matching app config it finds for any platform
    result=AppFindConfig(App, Tempstr);

    if (StrValid(App->InstallName))
    {
        SetVar(App->Vars, "name", App->InstallName);
    }

    ptr=getenv("LANGUAGE");
    if (! StrValid(ptr)) ptr=getenv("LANG");
    if (! StrValid(ptr)) ptr="en_US";
    AppSetLocale(App, ptr);

//we don't need the path that is returned here, but this function sets a lot of default variables and paths
    if (result) Tempstr=AppFormatPath(Tempstr, App);

    Destroy(Tempstr);

    return(result);
}


int AppsOutputList(TAction *Template)
{
    ListNode *Curr;
    int result=FALSE;
    TAction *App;
    char *p_dl;

    Curr=ListGetNext(Apps);
    while (Curr)
    {
        App=(TAction *) Curr->Item;
        if (AppMatches(Template, App))
        {
            if (StrValid(App->URL)) p_dl="www";
            else p_dl="";
            printf("%- 25s  %- 12s  %- 12s %- 3s   ", App->Name, App->Platform, GetVar(App->Vars, "category"), p_dl);
            printf("%s\n",GetVar(App->Vars, "comment"));
        }

        Curr=ListGetNext(Curr);
    }

    return(result);
}



