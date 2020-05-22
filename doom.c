#include "doom.h"

void DoomApplySettings(TAction *Act)
{
ListNode *Curr;
char *EmulatorArgs=NULL;
const char *ptr;

EmulatorArgs=CopyStr(EmulatorArgs, GetVar(Act->Vars, "emulator-args"));
Curr=ListGetNext(Act->Vars);
while (Curr)
{
ptr=Curr->Item;

if (strcasecmp(Curr->Tag, "vdesk")==0)
{
	if (strcasecmp(ptr, "y") == 0) EmulatorArgs=CatStr(EmulatorArgs, " -nofullscreen");
	else if (strcasecmp(ptr, "n") == 0) EmulatorArgs=CatStr(EmulatorArgs, " -fullscreen");
	else if (isdigit(*ptr)) EmulatorArgs=MCatStr(EmulatorArgs, " -geometry ", ptr, NULL);
}
else if (strcasecmp(Curr->Tag, "fullscreen")==0)
{
	if (strcasecmp(ptr, "y") == 0) EmulatorArgs=CatStr(EmulatorArgs, " -fullscreen");
	else if (strcasecmp(ptr, "n") == 0) EmulatorArgs=CatStr(EmulatorArgs, " -nofullscreen");
}
else if (strcasecmp(Curr->Tag, "sound")==0)
{
	if (strcasecmp(ptr, "n") == 0) EmulatorArgs=CatStr(EmulatorArgs, " -nosound");
	else if (strcasecmp(ptr, "sfx") == 0) EmulatorArgs=CatStr(EmulatorArgs, " -nomusic");
}
else if (strcasecmp(Curr->Tag, "grab")==0)
{
	if (strcasecmp(ptr, "n") == 0) EmulatorArgs=CatStr(EmulatorArgs, " -nograbmouse");
}
else if (strcasecmp(Curr->Tag, "mouse")==0)
{
	if (strcasecmp(ptr, "n") == 0) EmulatorArgs=CatStr(EmulatorArgs, " -nomouse");
}

Curr=ListGetNext(Curr);
}

SetVar(Act->Vars, "emulator-args", EmulatorArgs);

//as doom settings are stored in the emulator args in the emulator args 
//within the .desktop file, so we will have to reconfigure to rewrite 
//everything, unless we are doing an install (in which case all this
//gets done at the end of the install anyway)
//if (Act->Type != ACT_INSTALL) InstallReconfigure(Act);

Destroy(EmulatorArgs);
}
