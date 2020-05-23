
#include "common.h"
#include "config.h"

#define DEFAULT_SOMMELIER_ROOT "$(homedir)/.sommelier/"
#define DEFAULT_WINEPREFIX "$(sommelier_root)$(name)/"


char *URLBasename(char *RetStr, const char *URL)
{
char *Tempstr=NULL;
char *ptr;

Tempstr=CopyStr(Tempstr, URL);
StrTruncChar(Tempstr, '?');
StripDirectorySlash(Tempstr);
RetStr=CopyStr(RetStr, GetBasename(Tempstr));

Destroy(Tempstr);
return(RetStr);
}



int InList(const char *Item, const char *List)
{
char *Match=NULL;
const char *ptr;

ptr=GetToken(List, ",", &Match, GETTOKEN_QUOTES);
while (ptr)
{
strlwr(Match);
if (fnmatch(Match, Item, 0)==0)
{
  Destroy(Match);
  return(TRUE);
}
ptr=GetToken(ptr, ",", &Match, GETTOKEN_QUOTES);
}

Destroy(Match);

return(FALSE);
}




TAction *ActionCreate(int Type, const char *Name)
{
TAction *Act;

Act=(TAction *) calloc(1, sizeof(TAction));
Act->Type=Type;
Act->Vars=ListCreate();
Act->Name=CopyStr(Act->Name, Name);
if (StrValid(Name)) SetVar(Act->Vars, "name", Name);
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



int IdentifyFileType(const char *Path, int ForcedFileType)
{
STREAM *S;
char *Tempstr=NULL;
int FT=FILETYPE_UNKNOWN;

//this looks strange, calling this function just to return a passed in
//value, but it lets you write tidy code like 
//  switch(IdentifyFileType(Path, FILETYPE_UNKNOWN))
//instead of having a lot of 'if this and that' clauses
if (ForcedFileType != FILETYPE_UNKNOWN) return(ForcedFileType);

S=STREAMOpen(Path, "r");
if (S)
{
Tempstr=SetStrLen(Tempstr, 255);
STREAMReadBytes(S, Tempstr, 4);

if (strcmp(Tempstr, "\x50\x4b\x03\x04")==0) FT=FILETYPE_ZIP;
else if (strcmp(Tempstr, "\xd0\xcf\x11\xe0")==0) FT=FILETYPE_MSI;
else if (strncmp(Tempstr, "PE", 2)==0) FT=FILETYPE_PE;
else if (strncmp(Tempstr, "MZ", 2)==0) FT=FILETYPE_MZ;

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


p_ExpectedHash=GetVar(Act->Vars, "sha256");
if (StrValid(p_ExpectedHash))
{
	HashFile(&Hash, "sha256", Act->SrcPath, ENCODE_HEX);
	if (strcmp(Hash, p_ExpectedHash)==0) result=TRUE;
	Tempstr=CopyStr(Tempstr, "");
	printf("    expected sha256: [%s]\n",p_ExpectedHash);	
	printf("    actual   sha256: [%s]\n",Hash);	
	Tempstr=CopyStr(Tempstr, "");
	if (result) TerminalPutStr("~g~eOKAY:~w Hashes match~0\n",NULL);
	else TerminalPutStr("~rERROR: Downloaded file does not match expected hash~0\n",NULL);
}
else
{
	TerminalPutStr("~mNo expected hash value is configured for this download~0. This probably means that the source link is updated when a new version is released, but this means sommelier cannot confirm your download's integrity or security.\n", NULL);
	if (Act->Flags & FLAG_HASH_DOWNLOAD) 
	{
		HashFile(&Hash, "sha256", Act->SrcPath, ENCODE_HEX);
		printf("    actual   sha256: [%s]\n",Hash);	
	}
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
void RunProgramAndConsumeOutput(const char *Cmd)
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
	if (Config->Flags & FLAG_DEBUG) printf("%s", Tempstr);
  	Tempstr=STREAMReadLine(Tempstr, S);
  }
  STREAMClose(S);
}
Destroy(Tempstr);

while (wait(0) > 1);
}


