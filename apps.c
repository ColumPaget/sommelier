#include "apps.h"

void LoadAppConfigToAct(TAction *Act, const char *Config)
{
char *Name=NULL, *Value=NULL, *Tempstr=NULL;
const char *ptr;

ptr=GetNameValuePair(Config," ", "=", &Name, &Value);
while (ptr)
{
StripQuotes(Name);
StripQuotes(Value);
if (strcmp(Name,"url")==0) 
{
	Act->URL=CopyStr(Act->URL, Value);
	Tempstr=URLBasename(Tempstr, Act->URL);
	//SetVar(Act->Vars, "exec", Tempstr);
}
else if (strcmp(Name,"install-path")==0) Act->InstallPath=CopyStr(Act->InstallPath, Value);
else if (strcmp(Name,"platform")==0) Act->Platform=CopyStr(Act->Platform, Value);
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
else 
{
	SetVar(Act->Vars, Name, Value);
}
ptr=GetNameValuePair(ptr," ", "=", &Name, &Value);
}

Destroy(Tempstr);
Destroy(Value);
Destroy(Name);
}


void LoadAppsFromFile(const char *Path, ListNode *Apps)
{
STREAM *S;
ListNode *Node;
TAction *Act;
char *Tempstr=NULL, *Token=NULL;
const char *ptr;

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
	Node=ListFindNamedItem(Apps, Token);
	if (Node) Act=(TAction *) Node->Item;
	else 
	{
		Act=ActionCreate(ACT_NONE, Token);
		ListAddNamedItem(Apps, Token, Act);
	}
	LoadAppConfigToAct(Act, ptr);
	}
Tempstr=STREAMReadLine(Tempstr, S);
}

STREAMClose(S);
}

Destroy(Tempstr);
Destroy(Token);
}


ListNode *LoadApps(const char *ConfigFiles)
{
char *Tempstr=NULL, *Token=NULL;
const char *ptr;
ListNode *Apps, *Vars;

Apps=ListCreate();
Vars=ListCreate();
SetVar(Vars, "homedir", GetCurrUserHomeDir());

ptr=GetToken(ConfigFiles, ",", &Token, 0);
while (ptr)
{
Tempstr=SubstituteVarsInString(Tempstr, Token, Vars, 0);
LoadAppsFromFile(Tempstr, Apps);
ptr=GetToken(ptr, ",", &Token, 0);
}

ListDestroy(Vars, Destroy);
Destroy(Tempstr);
Destroy(Token);

return(Apps);
}


TAction *AppActionCreate(int Action, const char *AppName, const char *ConfigPath)
{
TAction *Act=NULL;
ListNode *Apps, *Node;

Apps=LoadApps(ConfigPath);
Node=ListFindNamedItem(Apps, AppName);
if (Node) 
{
	Act=(TAction *) Node->Item;
	Act->Type=Action;
}
else printf("ERROR: sorry, app '%s' is unknown\n", AppName);

return(Act);
}


char *AppFormatPath(char *Path, TAction *Act)
{
char *Tempstr=NULL;

//first generate wine prefix

Tempstr=CopyStr(Tempstr, GetVar(Act->Vars, "sommelier_root_template"));
Path=SubstituteVarsInString(Path, Tempstr, Act->Vars, 0);
Path=SlashTerminateDirectoryPath(Path);
SetVar(Act->Vars, "sommelier_root",Path);

Tempstr=CopyStr(Tempstr, GetVar(Act->Vars, "prefix_template"));
Path=SubstituteVarsInString(Path, Tempstr, Act->Vars, 0);
Path=SlashTerminateDirectoryPath(Path);
SetVar(Act->Vars, "prefix",Path);

//for dos and golang/go path==prefix
//for wine path is more complex
if (
    (! StrValid(Act->Platform)) ||
    (strcasecmp(Act->Platform, "win64")==0) ||
    (strcasecmp(Act->Platform, "win32")==0) ||
    (strcasecmp(Act->Platform, "win16")==0) ||
    (strcasecmp(Act->Platform, "windows")==0)
   )
{
	Path=CatStr(Path,"drive_c/");
	SetVar(Act->Vars, "drive_c",Path);

  if (StrValid(Act->InstallPath)) Path=SubstituteVarsInString(Path, Act->InstallPath, Act->Vars, 0);
  else Path=SubstituteVarsInString(Path, "$(drive_c)/Program Files/$(name)", Act->Vars, 0);

  Tempstr=SubstituteVarsInString(Tempstr, "/Program Files/$(name)", Act->Vars, 0);
  SetVar(Act->Vars, "exec-dir", Tempstr);
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



int AppsOutputList(const char *ConfigPath)
{
ListNode *Apps, *Curr;
int result=FALSE;
TAction *App;

Apps=LoadApps(ConfigPath);

Curr=ListGetNext(Apps);
while (Curr)
{
	App=(TAction *) Curr->Item;
	printf("%- 25s  %- 10s  %- 10s   ", App->Name, App->Platform, GetVar(App->Vars, "category"));
	printf("%s\n",GetVar(App->Vars, "comment"));
	//GetVar(App->Vars, "platform"));
	Curr=ListGetNext(Curr);
}

return(result);
}
