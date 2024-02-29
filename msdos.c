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
        Tempstr=SubstituteVarsInString(Tempstr, Token, Act->Vars, 0);
        EmulatorArgs=MCatStr(EmulatorArgs, " -c '", Tempstr, "' ", NULL);
        ptr=GetToken(ptr, ";", &Token, GETTOKEN_QUOTES);
    }

    if (GetBoolVar(Act->Vars, "no-exec-arg")) EmulatorArgs=MCatStr(EmulatorArgs, " -c 'exit' ", NULL);


    if (GetBoolVar(Act->Vars, "fullscreen")) EmulatorArgs=CatStr(EmulatorArgs, " -fullscreen");
    SetVar(Act->Vars, "emulator-args", EmulatorArgs);

    Destroy(EmulatorArgs);
    Destroy(Tempstr);
    Destroy(Token);
}
