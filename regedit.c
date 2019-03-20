#include "regedit.h"
#include "apps.h"

void RegEdit(TAction *Act, int Flags, const char *OSVersion, const char *iResolution)
{
char *Tempstr=NULL, *Path=NULL, *Resolution=NULL;
STREAM *S;


if (StrValid(iResolution)) Resolution=CopyStr(Resolution, iResolution);
else Resolution=CopyStr(Resolution, "800x600");

//Create a windows registry file to change wine settings in the
//registry

Path=FormatStr(Path,"/tmp/%d.reg",getpid());

S=STREAMOpen(Path, "w");
if (S)
{
STREAMWriteLine("Windows Registry Editor Version 5.00\r\n", S);

STREAMWriteLine("[HKEY_CURRENT_USER\\Software\\Wine\\X11 Driver]\r\n", S);


if (Flags & REG_WINMANAGER)
{
	STREAMWriteLine("\"Decorated\"=\"Y\"\r\n", S);
	STREAMWriteLine("\"Managed\"=\"Y\"\r\n", S);
	if (Act->Flags & FLAG_DEBUG) printf("Configure: Allow windowmanager to manage this app\n");
}

if (Flags & REG_NO_WINMANAGER)
{
	STREAMWriteLine("\"Decorated\"=\"N\"\r\n", S);
	STREAMWriteLine("\"Managed\"=\"N\"\r\n", S);
	if (Act->Flags & FLAG_DEBUG) printf("Configure: Disallow windowmanager to manage this app\n");
}


if (Flags & REG_NO_GRAB)
{
	STREAMWriteLine("\"GrabFullscreen\"=\"N\"\r\n", S);
	STREAMWriteLine("\"GrabPointer\"=\"N\"\r\n", S);
	if (Act->Flags & FLAG_DEBUG) printf("Configure: Disallow grabbing pointer or screen\n");
}


if (Flags & REG_VDESK)
{
	STREAMWriteLine("[HKEY_CURRENT_USER\\Software\\Wine\\Explorer]\r\n", S);
	STREAMWriteLine("\"Desktop\"=\"Default\"\r\n", S);

	STREAMWriteLine("[HKEY_CURRENT_USER\\Software\\Wine\\Explorer\\Desktops]\r\n", S);
	Tempstr=MCopyStr(Tempstr, "\"Default\"=\"", Resolution, "\"\r\n", S);
	STREAMWriteLine(Tempstr, S);
	if (Act->Flags & FLAG_DEBUG) printf("Configure: Use virtual desktop, resolution %s\n", Resolution);
}

if (Flags & REG_NO_VDESK)
{
	STREAMWriteLine("[HKEY_CURRENT_USER\\Software\\Wine\\Explorer]\r\n", S);
	STREAMWriteLine("\"Desktop\"=\"0\"\r\n", S);
	if (Act->Flags & FLAG_DEBUG) printf("Configure: DON'T use virtual desktop\n");
}


if (Flags & REG_FONT_SMOOTH)
{
	STREAMWriteLine("HKEY_CURRENT_USER\\Control Panel\\Desktop]\r\n", S);
	STREAMWriteLine("\"FontSmoothing\"=\"2\"\r\n", S);
	STREAMWriteLine("\"FontSmoothingOrientation\"=dword:0000000\r\n", S);
	STREAMWriteLine("\"FontSmoothingType\"=dword:00000002\r\n", S);
	STREAMWriteLine("\"FontSmoothingGamma\"=dword:00000578\r\n", S);
	if (Act->Flags & FLAG_DEBUG) printf("Configure: Use smooth fonts\n");
}

if (Flags & REG_NO_FONT_SMOOTH)
{
	STREAMWriteLine("HKEY_CURRENT_USER\\Control Panel\\Desktop]\r\n", S);
	STREAMWriteLine("\"FontSmoothing\"=\"0\"\r\n", S);
	STREAMWriteLine("\"FontSmoothingOrientation\"=dword:0000000\r\n", S);
	STREAMWriteLine("\"FontSmoothingType\"=dword:00000000\r\n", S);
	if (Act->Flags & FLAG_DEBUG) printf("Configure: DON'T use smooth fonts\n");
}


if (StrValid(OSVersion))
{
	STREAMWriteLine("[HKEY_CURRENT_USER\\Software\\Wine]\r\n", S);

	Tempstr=MCopyStr(Tempstr, "\"Version\"=\"", OSVersion, "\"\r\n", NULL);
	STREAMWriteLine(Tempstr, S);
	if (Act->Flags & FLAG_DEBUG) printf("Configure: OSVersion: %s\n", OSVersion);
}
STREAMClose(S);

Tempstr=SubstituteVarsInString(Tempstr, "WINEPREFIX=$(prefix) wine regedit ", Act->Vars, 0);
Tempstr=CatStr(Tempstr,Path);

RunProgramAndConsumeOutput(Tempstr, Act->Flags);
}

unlink(Path);

Destroy(Tempstr);
Destroy(Resolution);
Destroy(Path);
}



void RegeditApplySettings(TAction *Act)
{
char *OSVersion=NULL, *Resolution=NULL, *Tempstr=NULL;
ListNode *Curr;
const char *p_Value;
int Flags=0;

//don't need the path, just need some vars set in Act->Vars
Tempstr=AppFormatPath(Tempstr, Act);

Curr=ListGetNext(Act->Vars);
while (Curr)
{
printf("configure: '%s' set to '%s'\n", Curr->Tag, (const char *) Curr->Item);
p_Value=(const char *) Curr->Item;
if (strcmp(Curr->Tag, "vdesk")==0) 
{
	if (strcasecmp(p_Value, "y")==0) Flags |= REG_VDESK;
	else if (strcasecmp(p_Value, "n")==0) Flags |= REG_NO_VDESK;
	else if (isdigit(*p_Value)) 
	{
		Flags |= REG_VDESK;
		Resolution=CopyStr(Resolution, p_Value);
	}
}
else if (strcmp(Curr->Tag, "smoothfonts")==0) 
{
	if (strcasecmp(p_Value, "y")==0) Flags |= REG_FONT_SMOOTH;
	else if (strcasecmp(p_Value, "n")==0) Flags |= REG_NO_FONT_SMOOTH;
}
else if (strcmp(Curr->Tag, "winmanage")==0)
{
	if (strcasecmp(p_Value, "y")==0) Flags |= REG_WINMANAGER;
	if (strcasecmp(p_Value, "n")==0) Flags |= REG_NO_WINMANAGER;
}
else if (strcmp(Curr->Tag, "os-version")==0) OSVersion=CopyStr(OSVersion, p_Value);

Curr=ListGetNext(Curr);
}

RegEdit(Act, Flags, OSVersion, Resolution);

Destroy(OSVersion);
Destroy(Resolution);
Destroy(Tempstr);
}
