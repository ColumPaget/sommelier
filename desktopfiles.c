#include "desktopfiles.h"

#define DESKTOP_PATH "$(homedir)/.local//share/applications/"

static char *DesktopFileMakePath(char *RetStr, TAction *Act)
{
char *Tempstr=NULL;

Tempstr=SubstituteVarsInString(Tempstr, DESKTOP_PATH, Act->Vars, 0);
SetVar(Act->Vars, "desktop-path", Tempstr);
RetStr=SubstituteVarsInString(RetStr, "$(desktop-path)/$(name).desktop",Act->Vars, 0);

Destroy(Tempstr);

return(RetStr);
}


int DesktopFileDelete(TAction *Act)
{
char *Tempstr=NULL;
int result;

Tempstr=DesktopFileMakePath(Tempstr, Act);
result=unlink(Tempstr);

Destroy(Tempstr);
if (result==0) return(TRUE);
return(FALSE);
}

int DesktopFileRead(TAction *Act)
{
char *Tempstr=NULL, *Token=NULL;
const char *ptr;
int result=FALSE;
STREAM *S;

Tempstr=DesktopFileMakePath(Tempstr, Act);
S=STREAMOpen(Tempstr, "r");
if (S)
{
	result=TRUE;
	Tempstr=STREAMReadLine(Tempstr, S);
	while (Tempstr)
	{
		StripTrailingWhitespace(Tempstr);
		ptr=GetToken(Tempstr, "=", &Token, 0);
		if (strcasecmp(Token,"exec")==0) 
		{
			Act->Exec=CopyStr(Act->Exec, ptr);	
			StripQuotes(Act->Exec);
		}
		if (strcasecmp(Token,"icon")==0) 
		{
			Token=CopyStr(Token, ptr);
			StripQuotes(Token);
			SetVar(Act->Vars, "icon", Token);
		}
		if (strcasecmp(Token,"path")==0) 
		{
			Token=CopyStr(Token, ptr);
			StripQuotes(Token);
			SetVar(Act->Vars, "working-dir", Token);
		}
		Tempstr=STREAMReadLine(Tempstr, S);
	}
	STREAMClose(S);
}
else fprintf(stderr, "ERROR: Failed to open .desktop file '%s' for application\n", Tempstr);

Tempstr=CopyStr(Tempstr, Act->Exec);
Act->Exec=CopyStr(Act->Exec, "");
ptr=GetToken(Tempstr,"\\S",&Token,GETTOKEN_HONOR_QUOTES);
while (ptr)
{
if (strncmp(Token,"WINEPREFIX=",11)==0) SetVar(Act->Vars, "prefix", Token+11);
else Act->Exec=MCatStr(Act->Exec, Token, " ",NULL);
ptr=GetToken(ptr,"\\S",&Token, GETTOKEN_HONOR_QUOTES);
}

DestroyString(Token);
DestroyString(Tempstr);

return(result);
}



void DesktopFileGenerate(TAction *Act, const char *Path)
{
STREAM *S;
char *Tempstr=NULL, *Hash=NULL;

printf("Generating desktop File.\n");

Tempstr=DesktopFileMakePath(Tempstr, Act);
MakeDirPath(Tempstr, 0744);
S=STREAMOpen(Tempstr, "w mode=0744");
if (S)
{
fchmod(S->out_fd, 0744);

if (
    (strcasecmp(Act->Platform, "win64")==0) ||
    (strcasecmp(Act->Platform, "win32")==0) ||
    (strcasecmp(Act->Platform, "win16")==0) ||
    (strcasecmp(Act->Platform, "windows")==0)
	) 
{
				Tempstr=SubstituteVarsInString(Tempstr, "$(drive_c)$(exec-dir)", Act->Vars, 0);
				SetVar(Act->Vars, "working-dir", Tempstr);

				Tempstr=SubstituteVarsInString(Tempstr, "C:\\$(exec-dir)\\$(exec)", Act->Vars, 0);
				strrep(Tempstr, '/', '\\');
}
else Tempstr=SubstituteVarsInString(Tempstr, "$(exec-dir)/$(exec)", Act->Vars, 0);

SetVar(Act->Vars,"exec-path",Tempstr);

if (strcmp(Act->Platform, "win64")==0) Tempstr=SubstituteVarsInString(Tempstr, "WINEARCH=win64 WINEPREFIX=$(prefix) wine '$(exec-path)'", Act->Vars, 0);
else if (strcmp(Act->Platform, "win32")==0) Tempstr=SubstituteVarsInString(Tempstr, "WINEARCH=win32 WINEPREFIX=$(prefix) wine '$(exec-path)'", Act->Vars, 0);
else if (strcmp(Act->Platform, "dos")==0) Tempstr=SubstituteVarsInString(Tempstr, "dosbox -exit '$(exec-path)'", Act->Vars, 0);
else Tempstr=SubstituteVarsInString(Tempstr, "WINEPREFIX=$(prefix) wine '$(exec-path)'", Act->Vars, 0);
SetVar(Act->Vars, "invocation", Tempstr);

HashFile(&Hash, "sha256", GetVar(Act->Vars, "exec"), ENCODE_HEX);
SetVar(Act->Vars, "exec-sha256", Hash);

Tempstr=SubstituteVarsInString(Tempstr, "[Desktop Entry]\nName=$(name)\nType=Application\nTerminal=false\nComment=\"$(comment)\"\nSHA256=\"$(exec-sha256)\"\nPath=\"$(working-dir)\"\nExec=\"$(invocation)\"\nIcon=\"$(Icon)\"\nRunsWith=\"$(runswith)\"\n",Act->Vars, 0);
STREAMWriteLine(Tempstr, S);
Tempstr=SubstituteVarsInString(Tempstr, "Categories=$(category)\nCategory=$(category)\n",Act->Vars, 0);
STREAMWriteLine(Tempstr, S);
STREAMClose(S);
}

DestroyString(Tempstr);
DestroyString(Hash);
}
