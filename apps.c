#include "apps.h"
#include "config.h"
#include "categories.h"
#include <fnmatch.h>

#define DEFAULT_PREFIX "$(sommelier_root)$(name)-$(platform)/"
#define OLD_VERSION_PREFIX "$(sommelier_root)$(name)/"


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


int AppMatchCompareVars(TAction *App, const char *VarName, const char *Match)
{
    const char *ptr;

    if (! StrValid(Match)) return(TRUE);

    ptr=GetVar(App->Vars, VarName);
    if (InList(Match, ptr)) return(TRUE);
    return(FALSE);
}


int AppMatches(TAction *Template, TAction *App)
{
    if (! StrValid(Template->Platform)) return(TRUE);
    if (! AppPlatformMatches(App, Template->Platform) ) return(FALSE);
    if (! AppMatchCompareVars(App, "category", GetVar(Template->Vars, "category"))) return(FALSE);
    if ((Template->Flags & FLAG_INSTALLED) && (! AppIsInstalled(App)) ) return(FALSE);

    if (StrValid(Template->Name))
    {
        if (! pmatch_one(Template->Name, App->Name, StrLen(App->Name), NULL, NULL, 0)) return(FALSE);
    }

    return(TRUE);
}


//do some alterations and actions on certain settings
static void AppPostProcessConfigItem(TAction *Act, const char *Name, const char *Value)
{
    if (strcasecmp(Name,"install-path")==0) Act->InstallPath=CopyStr(Act->InstallPath, Value);
    else if (strcasecmp(Name,"install-name")==0) Act->InstallName=CopyStr(Act->InstallName, Value);
    else if (strcasecmp(Name,"dlname")==0) Act->DownName=CopyStr(Act->DownName, Value);
    else if (strcasecmp(Name,"install-type")==0)
    {
        if (strcasecmp(Value,"unpack")==0) Act->InstallType=INSTALL_UNPACK;
        if (strcasecmp(Value,"executable")==0) Act->InstallType=INSTALL_EXECUTABLE;
    }
    else if (strcasecmp(Name,"bundled")==0)
    {
        //app is bundled with another app! Set the bundled flag, and set a variable
        Act->Flags |= FLAG_BUNDLED;
        SetVar(Act->Vars, "bundled-with", Value);
    }
    else if (strcasecmp(Name,"parent")==0) Act->Parent=CopyStr(Act->Parent, Value);
    else if (strcasecmp(Name,"install_stage2")==0) SetVar(Act->Vars, "install-stage2", Value);
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
                Act->Platform=CopyStr(Act->Platform, PlatformUnAlias(Value));
                SetVar(Act->Vars, "platform", Act->Platform);
                Plt=PlatformFind(Act->Platform);
                if (Plt)
                {
                    Act->PlatformID=Plt->ID;
                    if (Plt->Flags & PLATFORM_FLAG_NOEXEC) Act->Flags |= FLAG_NOEXEC;
                    if (StrValid(Plt->UnpackDir)) SetVar(Act->Vars, "unpack-dir", Plt->UnpackDir);
                }
                else fprintf(stderr, "PLATFORM NOT FOUND: '%s' for %s\n", Act->Platform, Act->Name);
            }
            //locale is a special case, we want to be able to use 'locale=' to set a locale string, but we
            //also want to be able to use $(locale) as a variable in config. Further, we want to be able to say
            //'locale="$(lang):$(country)". But when we do the substitution and set locale to it's final value, we
            //destroy the variables by subsituting them. So if we want to try a different locale in future, we now
            //can't do that. Thus we set 'locale_template' and use that as the template to substitute into 'locale',
            //so 'locale_template' always reamins unchanged.
            else if (strcmp(Name, "locale")==0) SetVar(Act->Vars, "locale_template", Value);
            else if (strcasecmp(Name,"dlc")==0) Act->Flags |= FLAG_DLC;
            else if (strcasecmp(Name,"su")==0) Act->Flags |= FLAG_ALLOW_SU;
            else if (strcasecmp(Name,"allow-su")==0) Act->Flags |= FLAG_ALLOW_SU;
            else if (strcasecmp(Name,"copyfiles-from")==0) Act->PostProcess=MCatStr(Act->PostProcess, Name, "=\"", Value, "\" ", NULL);
            else if (strcasecmp(Name,"copyfiles-to")==0) Act->PostProcess=MCatStr(Act->PostProcess, Name, "=\"", Value, "\" ", NULL);
            else if (strcasecmp(Name,"movefiles-from")==0) Act->PostProcess=MCatStr(Act->PostProcess, Name, "=\"", Value, "\" ", NULL);
            else if (strcasecmp(Name,"movefiles-to")==0) Act->PostProcess=MCatStr(Act->PostProcess, Name, "=\"", Value, "\" ", NULL);
            else if (strcasecmp(Name,"chext")==0) Act->PostProcess=MCatStr(Act->PostProcess, Name, "=", Value, " ", NULL);
            else if (strcasecmp(Name,"delete")==0) Act->PostProcess=MCatStr(Act->PostProcess, Name, "=\"", Value, "\" ", NULL);
            else if (strcasecmp(Name,"rename")==0) Act->PostProcess=MCatStr(Act->PostProcess, Name, "=\"", Value, "\" ", NULL);
            else if (strcasecmp(Name,"link")==0) Act->PostProcess=MCatStr(Act->PostProcess, Name, "=\"", Value, "\" ", NULL);
            else if (strcasecmp(Name,"zip")==0) Act->PostProcess=MCatStr(Act->PostProcess, Name, "=\"", Value, "\" ", NULL);
            else if (strcasecmp(Name,"category")==0)
            {
                Tempstr=CategoriesExpand(Tempstr, Value);
                SetVar(Act->Vars, "category", Tempstr);
            }
            else if (strcasecmp(Name,"extra_category")==0)
            {
                Tempstr=CategoriesExpand(Tempstr, Value);
                AppendVar(Act->Vars, "category", Tempstr);
            }
            else if (strcasecmp(Name,"sha256")==0)
            {
                strlwr(Value);
                SetVar(Act->Vars, Name, Value);
            }
            else SetVar(Act->Vars, Name, Value);

            AppPostProcessConfigItem(Act, Name, Value);
        }

        ptr=GetNameValuePair(ptr," ", "=", &Name, &Value);
    }


		//post process some values
		ptr=GetVar(Act->Vars, "exec64");
		if (! StrValid(ptr)) SetVar(Act->Vars, "exec64", GetVar(Act->Vars, "exec"));

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
            if (strcmp(Parent->Platform, App->Platform)==0)
            {
                Tempstr=CopyStr(Tempstr, GetVar(Parent->Vars, "bundles"));
                Tempstr=MCatStr(Tempstr, " ", App->Name, NULL);
                SetVar(Parent->Vars, "bundles", Tempstr);
            }
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



char *AppFormatPath(char *Path, TAction *Act, const char *PrefixTemplate)
{
    char *Tempstr=NULL;
    const char *ptr;

//first generate sommelier root (usually ~/.sommelier) if it doesn't exist

    ptr=GetVar(Act->Vars, "sommelier_root");
    if (! StrValid(ptr))
    {
        if (Config->Flags & FLAG_SYSTEM_INSTALL) Path=SubstituteVarsInString(Path, "/opt/", Act->Vars, 0);
        else Path=SubstituteVarsInString(Path, "$(homedir)/.sommelier/", Act->Vars, 0);
        SetVar(Act->Vars, "sommelier_root", Path);
    }

    Tempstr=MCopyStr(Tempstr, GetVar(Act->Vars, "sommelier_root"), "/patches", NULL);
    SetVar(Act->Vars, "sommelier_patches_dir", Tempstr);

    if (StrValid(Act->InstallPath)) Path=CopyStr(Path, Act->InstallPath);
    else if (StrValid(PrefixTemplate))
    {
        Tempstr=CopyStr(Tempstr, PrefixTemplate);
        Path=SubstituteVarsInString(Path, Tempstr, Act->Vars, 0);
        Path=SlashTerminateDirectoryPath(Path);
        strrep(Path, ':', '_');
        SetVar(Act->Vars, "prefix", Path);
    }
    else Path=CopyStr(Path, GetVar(Act->Vars, "prefix"));


//for dos and golang/go path==prefix
//for wine path is more complex
    switch (Act->PlatformID)
    {
    case PLATFORM_WINDOWS:
    case PLATFORM_GOGWINDOS:
        Path=CatStr(Path,"drive_c/");
        SetVar(Act->Vars, "drive_c",Path);

        if (StrValid(Act->InstallPath)) Path=SubstituteVarsInString(Path, Act->InstallPath, Act->Vars, 0);
        else Path=SubstituteVarsInString(Path, "$(drive_c)/Program Files/$(name)", Act->Vars, 0);
        break;

    default:
        SetVar(Act->Vars, "drive_c",Path);
        break;
    }

    Path=SlashTerminateDirectoryPath(Path);
    SetVar(Act->Vars, "install-dir",Path);

    ResolveVar(Act->Vars, "emu-dir");

    DestroyString(Tempstr);
    return(Path);
}


char *AppFindInstalled(char *Path, TAction *App)
{
    Path=AppFormatPath(Path, App, DEFAULT_PREFIX);
    if (access(Path, F_OK) !=0)
    {
        Path=AppFormatPath(Path, App, OLD_VERSION_PREFIX);
        if (access(Path, F_OK) !=0) Path=CopyStr(Path, "");
    }

    return(Path);
}


int AppIsInstalled(TAction *App)
{
    char *Tempstr=NULL;
    int result=FALSE;

    Tempstr=AppFindInstalled(Tempstr, App);
    if (StrValid(Tempstr)) result=TRUE;

    Destroy(Tempstr);
    return(result);
}


int AppFindConfig(TAction *App, const char *Platforms)
{
    TAction *AppConfig;
    ListNode *Curr;
    int result=FALSE;


    Curr=ListGetNext(Apps);
    while (Curr)
    {
        if (StrValid(Curr->Tag) && (strcasecmp(App->Name, Curr->Tag)==0))
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
                App->DownName=CopyStr(App->DownName, AppConfig->DownName);
                App->Parent=CopyStr(App->Parent, AppConfig->Parent);
                App->PlatformID=AppConfig->PlatformID;
                App->PostProcess=CopyStr(App->PostProcess, AppConfig->PostProcess);
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

    if (StrValid(App->Parent)) SetVar(App->Vars, "name", App->Parent);
    else if (StrValid(App->InstallName)) SetVar(App->Vars, "name", App->InstallName);

    ptr=getenv("LANGUAGE");
    if (! StrValid(ptr)) ptr=getenv("LANG");
    if (! StrValid(ptr)) ptr="en_US";
    AppSetLocale(App, ptr);

    //we don't need the path that is returned here, but this function sets a lot of default variables and paths
    //we set them using 'DEFAULT_PREFIX' here, anything involving installed apps will call 'AppIsInstalled' which
    //will consider both DEFAULT_PREFIX and OLD_VERSION_PREFIX
    if (result) Tempstr=AppFormatPath(Tempstr, App, DEFAULT_PREFIX);

    Destroy(Tempstr);

    return(result);
}


int AppsOutputList(TAction *Template)
{
    ListNode *Curr;
    int result=FALSE;
    TAction *App;
    const char *p_dl;
    char *Flags=NULL, *Tempstr=NULL;

    Curr=ListGetNext(Apps);
    while (Curr)
    {
        App=(TAction *) Curr->Item;
        if (AppMatches(Template, App))
        {
            Flags=CopyStr(Flags, "");
            if (StrValid(App->URL)) Flags=CatStr(Flags, "www,");
            if (AppIsInstalled(App)) Flags=CatStr(Flags,"i,");

            printf("%-25s  %12s %-12s  %-12s   ", App->Name, App->Platform, Flags, GetVar(App->Vars, "category"));
            printf("%s\n",GetVar(App->Vars, "comment"));
        }

        Curr=ListGetNext(Curr);
    }

    Destroy(Tempstr);
    Destroy(Flags);
    return(result);
}


int AppAllowSU(TAction *App)
{
    if (Config->Flags & FLAG_DENY_SU) return(FALSE);
    if (Config->Flags & FLAG_ALLOW_SU) return(TRUE);
    if (App && (App->Flags & FLAG_DENY_SU)) return(FALSE);
    if (App && (App->Flags & FLAG_ALLOW_SU)) return(TRUE);
    return(FALSE);
}
