#include "regedit.h"
#include "apps.h"

void RegEdit(TAction *Act, int Flags)
{
char *Tempstr=NULL, *Path=NULL;
STREAM *S;

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
	STREAMWriteLine("\"Default\"=\"800x600\"\r\n", S);
	if (Act->Flags & FLAG_DEBUG) printf("Configure: Use virtual desktop\n");
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

STREAMClose(S);

Tempstr=SubstituteVarsInString(Tempstr, "WINEPREFIX=$(prefix) wine regedit ", Act->Vars, 0);
Tempstr=CatStr(Tempstr,Path);

RunProgramAndConsumeOutput(Tempstr, Act->Flags);
}

unlink(Path);

Destroy(Tempstr);
Destroy(Path);
}



void RegeditApplySettings(TAction *Act)
{
char *Name=NULL, *Value=NULL;
const char *ptr;

//don't need the path, just need some vars set in Act->Vars
Value=AppFormatPath(Value, Act);

ptr=GetVar(Act->Vars, "settings");
ptr=GetNameValuePair(ptr, ",", "=", &Name, &Value);
while (ptr)
{
printf("APPLY: '%s' set to '%s'\n", Name, Value);
if (strcmp(Name, "vdesk")==0) 
{
	if (strcasecmp(Value, "y")==0) RegEdit(Act, REG_VDESK);
	if (strcasecmp(Value, "n")==0) RegEdit(Act, REG_NO_VDESK);
}
else if (strcmp(Name, "smoothfonts")==0) 
{
	if (strcasecmp(Value, "y")==0) RegEdit(Act, REG_FONT_SMOOTH);
	if (strcasecmp(Value, "n")==0) RegEdit(Act, REG_NO_FONT_SMOOTH);
}
else if (strcmp(Name, "winmanage")==0)
{
	if (strcasecmp(Value, "y")==0) RegEdit(Act, REG_WINMANAGER);
	if (strcasecmp(Value, "n")==0) RegEdit(Act, REG_NO_WINMANAGER);
}

ptr=GetNameValuePair(ptr, ",", "=", &Name, &Value);
}

Destroy(Name);
Destroy(Value);
}
