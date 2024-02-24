#include "msdos.h"

void MSDOSApplySettings(TAction *Act)
{
    ListNode *Curr;
    char *EmulatorArgs=NULL, *Token=NULL, *Tempstr=NULL;
    const char *ptr;

    EmulatorArgs=CopyStr(EmulatorArgs, GetVar(Act->Vars, "emulator-args"));
		//if there are a list of commands to be run in dosbox, then build those into 'installer-args'
		//each command has to be supplieda as a seperate '-c <command>' argument
		ptr=GetToken(GetVar(Act->Vars, "commands"), ";", &Token, GETTOKEN_QUOTES);
		while (ptr)
		{
				StripTrailingWhitespace(Token);
				StripLeadingWhitespace(Token);
				EmulatorArgs=MCatStr(EmulatorArgs, " -c '", Token, "' ", NULL);
				ptr=GetToken(ptr, ";", &Token, GETTOKEN_QUOTES);
		}
							

    Curr=ListGetNext(Act->Vars);
    while (Curr)
    {
        ptr=Curr->Item;

        if (strcasecmp(Curr->Tag, "fullscreen")==0)
        {
            if (strcasecmp(ptr, "y") == 0) EmulatorArgs=CatStr(EmulatorArgs, " -fullscreen");
        }

        Curr=ListGetNext(Curr);
    }

    SetVar(Act->Vars, "emulator-args", EmulatorArgs);

    Destroy(EmulatorArgs);
    Destroy(Tempstr);
    Destroy(Token);
}
