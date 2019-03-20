#include "common.h"

#define DEFAULT_CONFIG_PATH "/etc/sommelier.apps,$(homedir)/.sommelier.apps"
#define DEFAULT_SOMMELIER_ROOT "$(homedir)/.sommelier/"
#define DEFAULT_WINEPREFIX "$(sommelier_root)$(name)/"


char *URLBasename(char *RetStr, const char *URL)
{
char *ptr;

RetStr=CopyStr(RetStr, GetBasename(URL));
StrTruncChar(RetStr, '?');

return(RetStr);
}



TAction *ActionCreate(int Type, const char *Name)
{
TAction *Act;

Act=(TAction *) calloc(1, sizeof(TAction));
Act->Type=Type;
Act->Vars=ListCreate();
Act->Name=CopyStr(Act->Name, Name);
Act->ConfigPath=CopyStr(Act->ConfigPath, DEFAULT_CONFIG_PATH);
SetVar(Act->Vars, "name", Name);
SetVar(Act->Vars, "sommelier_root_template", DEFAULT_SOMMELIER_ROOT);
SetVar(Act->Vars, "prefix_template", DEFAULT_WINEPREFIX);
SetVar(Act->Vars, "homedir", GetCurrUserHomeDir());
return(Act);
}



void ActionDestroy(TAction *Act)
{
Destroy(Act->Name);
Destroy(Act->URL);
Destroy(Act->DownName);
Destroy(Act->InstallPath);
Destroy(Act->Exec);

ListDestroy(Act->Vars, Destroy);
free(Act);
}



int IdentifyFileType(const char *Path)
{
STREAM *S;
char *Tempstr=NULL;
int FT=FILETYPE_UNKNOWN;

S=STREAMOpen(Path, "r");
if (S)
{
Tempstr=SetStrLen(Tempstr, 255);
STREAMReadBytes(S, Tempstr, 4);

if (strcmp(Tempstr, "\x50\x4b\x03\x04")==0) FT=FILETYPE_ZIP;
else if (strcmp(Tempstr, "\xd0\xcf\x11\xe0")==0) FT=FILETYPE_MSI;
else if (strncmp(Tempstr, "PE", 2)==0) FT=FILETYPE_PE;
else if (strncmp(Tempstr, "MZ", 2)==0) 
{
	FT=FILETYPE_MZ;
}
STREAMClose(S);
}

DestroyString(Tempstr);

return(FT);
}



int CompareSha256(TAction *Act)
{
char *Hash=NULL, *Tempstr=NULL;
const char *p_ExpectedHash;
int result=FALSE;

HashFile(&Hash, "sha256", Act->DownName, ENCODE_HEX);

p_ExpectedHash=GetVar(Act->Vars, "sha256");
if (StrValid(p_ExpectedHash))
{
	if (strcmp(Hash, p_ExpectedHash)==0) result=TRUE;
	Tempstr=CopyStr(Tempstr, "");
	Tempstr=TerminalFormatStr(Tempstr, "~eChecking Download integrity...~0",NULL);
	printf("%s\n", Tempstr);
	printf("    expected sha256: [%s]\n",p_ExpectedHash);	
	printf("    actual   sha256: [%s]\n",Hash);	
	Tempstr=CopyStr(Tempstr, "");
	if (result) Tempstr=TerminalFormatStr(Tempstr, "~gOKAY:~0 Hashes match\n",NULL);
	else Tempstr=TerminalFormatStr(Tempstr, "~rERROR:~0 Downloaded file does not match expected hash\n",NULL);
	printf("%s\n",Tempstr);
}
else 
{
	printf("No expected hash value is configured for this download\n");
	printf("    actual   sha256: [%s]\n",Hash);	
	result=TRUE;
}

Destroy(Tempstr);
Destroy(Hash);

return(result);
}


//Some installers fork into background, perhaps calling 'setsid', which means we
//will no longer consider them child processes and will no longer wait for them.
//Holding open a pipe for their output seems to overcome this, and also allows us
//to suppress a lot of crap that they might print out.
void RunProgramAndConsumeOutput(const char *Cmd, int Flags)
{
STREAM *S;
char *Tempstr=NULL;

Tempstr=MCopyStr(Tempstr, Cmd, " 2>&1", NULL);
S=STREAMSpawnCommand(Tempstr, "");
if (S)
{
  Tempstr=STREAMReadLine(Tempstr, S);
  while (Tempstr)
  {
	if (Flags & FLAG_DEBUG) printf("%s", Tempstr);
  Tempstr=STREAMReadLine(Tempstr, S);
  }
  STREAMClose(S);
}
Destroy(Tempstr);

while (wait(0) > 1);
}

