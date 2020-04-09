#include "regedit.h"
#include "apps.h"
#include "config.h"

void RegEdit(TAction *Act, int Flags, const char *OSVersion, const char *iResolution, const char *DLLOverrides)
{
char *Tempstr=NULL, *Path=NULL, *Resolution=NULL, *Token=NULL;
const char *ptr;
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

if (Flags & (REG_WINMANAGER | REG_NO_WINMANAGER | REG_NO_GRAB))
{
STREAMWriteLine("[HKEY_CURRENT_USER\\Software\\Wine\\X11 Driver]\r\n", S);
if (Flags & REG_WINMANAGER)
{
	STREAMWriteLine("\"Decorated\"=\"Y\"\r\n", S);
	STREAMWriteLine("\"Managed\"=\"Y\"\r\n", S);
	if (Config->Flags & FLAG_DEBUG) printf("Configure: Allow windowmanager to manage this app\n");
}

if (Flags & REG_NO_WINMANAGER)
{
	STREAMWriteLine("\"Decorated\"=\"N\"\r\n", S);
	STREAMWriteLine("\"Managed\"=\"N\"\r\n", S);
	if (Config->Flags & FLAG_DEBUG) printf("Configure: Disallow windowmanager to manage this app\n");
}


if (Flags & REG_NO_GRAB)
{
	STREAMWriteLine("\"GrabFullscreen\"=\"N\"\r\n", S);
	STREAMWriteLine("\"GrabPointer\"=\"N\"\r\n", S);
	if (Config->Flags & FLAG_DEBUG) printf("Configure: Disallow grabbing pointer or screen\n");
}
}


if (Flags & REG_VDESK)
{
	STREAMWriteLine("[HKEY_CURRENT_USER\\Software\\Wine\\Explorer]\r\n", S);
	STREAMWriteLine("\"Desktop\"=\"Default\"\r\n", S);

	STREAMWriteLine("[HKEY_CURRENT_USER\\Software\\Wine\\Explorer\\Desktops]\r\n", S);
	Tempstr=MCopyStr(Tempstr, "\"Default\"=\"", Resolution, "\"\r\n", S);
	STREAMWriteLine(Tempstr, S);
	if (Config->Flags & FLAG_DEBUG) printf("Configure: Use virtual desktop, resolution %s\n", Resolution);
}
else if (Flags & REG_NO_VDESK)
{
	STREAMWriteLine("[HKEY_CURRENT_USER\\Software\\Wine\\Explorer]\r\n", S);
	STREAMWriteLine("\"Desktop\"=\"0\"\r\n", S);
	if (Config->Flags & FLAG_DEBUG) printf("Configure: DON'T use virtual desktop\n");
}


if (Flags & REG_FONT_SMOOTH)
{
	STREAMWriteLine("[HKEY_CURRENT_USER\\Control Panel\\Desktop]\r\n", S);
	STREAMWriteLine("\"FontSmoothing\"=\"2\"\r\n", S);
	STREAMWriteLine("\"FontSmoothingOrientation\"=dword:0000000\r\n", S);
	STREAMWriteLine("\"FontSmoothingType\"=dword:00000002\r\n", S);
	STREAMWriteLine("\"FontSmoothingGamma\"=dword:00000578\r\n", S);
	if (Config->Flags & FLAG_DEBUG) printf("Configure: Use smooth fonts\n");
}
else if (Flags & REG_NO_FONT_SMOOTH)
{
	STREAMWriteLine("[HKEY_CURRENT_USER\\Control Panel\\Desktop]\r\n", S);
	STREAMWriteLine("\"FontSmoothing\"=\"0\"\r\n", S);
	STREAMWriteLine("\"FontSmoothingOrientation\"=dword:0000000\r\n", S);
	STREAMWriteLine("\"FontSmoothingType\"=dword:00000000\r\n", S);
	if (Config->Flags & FLAG_DEBUG) printf("Configure: DON'T use smooth fonts\n");
}

if (Flags & (REG_GDI3D | REG_OPENGL3D))
{
printf("SET OPENGL\n");
	STREAMWriteLine("[HKEY_CURRENT_USER\\Software\\Wine\\Direct3D]\r\n", S);
	if (Flags & REG_GDI3D)
	{
		STREAMWriteLine("\"DirectDrawRenderer\"=\"gdi\"\r\n", S);
		if (Config->Flags & FLAG_DEBUG) printf("Configure: DON'T use OpenGL for 3D\n");
	}	
	else
	{
		STREAMWriteLine("\"DirectDrawRenderer\"=\"opengl\"\r\n", S);
		STREAMWriteLine("\"RenderTargetLockMode\"=\"auto\"\r\n", S);
		STREAMWriteLine("\"CheckFloatConstants\"=\"enabled\"\r\n", S);
		STREAMWriteLine("\"csmt\"=\"0x1\"\r\n", S);
		if (Flags & REG_OPENGLSL)
		{
		STREAMWriteLine("\"UseGLSL\"=\"enabled\"\r\n", S);
		if (Config->Flags & FLAG_DEBUG) printf("Configure: Use OpenGL Shading Language\n");
		}
		else
		{
		STREAMWriteLine("\"UseGLSL\"=\"disabled\"\r\n", S);
		if (Config->Flags & FLAG_DEBUG) printf("Configure: DON'T use OpenGL Shading Language\n");
		}
	}

}


if (StrValid(OSVersion))
{
	STREAMWriteLine("[HKEY_CURRENT_USER\\Software\\Wine]\r\n", S);

	Tempstr=MCopyStr(Tempstr, "\"Version\"=\"", OSVersion, "\"\r\n", NULL);
	STREAMWriteLine(Tempstr, S);
	if (Config->Flags & FLAG_DEBUG) printf("Configure: OSVersion: %s\n", OSVersion);
}

if (StrValid(DLLOverrides))
{
	STREAMWriteLine("[HKEY_CURRENT_USER\\Software\\Wine\\DllOverrides]\r\n", S);

	ptr=GetToken(DLLOverrides, ",", &Token, 0);
	while (ptr)
	{
	Tempstr=MCatStr(Tempstr, "\"", Token, "\"=\"native\"\r\n", NULL);
	ptr=GetToken(ptr, ",", &Token, 0);
	}
	STREAMWriteLine(Tempstr, S);

	if (Config->Flags & FLAG_DEBUG) printf("Configure: DLLOverrides: %s\n", DLLOverrides);
}


STREAMClose(S);

Tempstr=SubstituteVarsInString(Tempstr, "WINEPREFIX=$(prefix) wine regedit ", Act->Vars, 0);
Tempstr=CatStr(Tempstr,Path);

RunProgramAndConsumeOutput(Tempstr, Act->Flags);
}

unlink(Path);

Destroy(Tempstr);
Destroy(Token);
Destroy(Resolution);
Destroy(Path);
}



void RegEditApplySettings(TAction *Act)
{
char *OSVersion=NULL, *Resolution=NULL, *DLLOverrides=NULL, *Tempstr=NULL;
ListNode *Curr;
const char *p_Value;
int Flags=0;

//only do this for windows apps
if (PlatformType(Act->Platform) != PLATFORM_WINDOWS) return;

//don't need the path, just need some vars set in Act->Vars
Tempstr=AppFormatPath(Tempstr, Act);

Curr=ListGetNext(Act->Vars);
while (Curr)
{
if (Config->Flags & FLAG_DEBUG) printf("configure: '%s' set to '%s'\n", Curr->Tag, (const char *) Curr->Item);
p_Value=(const char *) Curr->Item;
if (StrValid(Curr->Tag))
{
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
else if (strcasecmp(Curr->Tag, "direct3d")==0)
{
	if (strcasecmp(p_Value, "gdi")==0) Flags |= REG_GDI3D;
	if (strcasecmp(p_Value, "gl")==0) Flags |= REG_OPENGL3D;
	if (strcasecmp(p_Value, "opengl")==0) Flags |= REG_OPENGL3D;
	if (strcasecmp(p_Value, "openglsl")==0) Flags |= REG_OPENGL3D | REG_OPENGLSL;
	if (strcasecmp(p_Value, "glsl")==0) Flags |= REG_OPENGL3D | REG_OPENGLSL;
}
else if (strcmp(Curr->Tag, "os-version")==0) OSVersion=CopyStr(OSVersion, p_Value);
else if (strcmp(Curr->Tag, "dll-overrides")==0) DLLOverrides=CopyStr(DLLOverrides, p_Value);
}

Curr=ListGetNext(Curr);
}

RegEdit(Act, Flags, OSVersion, Resolution, DLLOverrides);

Destroy(DLLOverrides);
Destroy(Resolution);
Destroy(OSVersion);
Destroy(Tempstr);
}
