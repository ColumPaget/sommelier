#include "emulator.h"

char *EmulatorGetHelp(char *RetStr, TAction *Act)
{
    char *Tempstr=NULL, *Emu=NULL;
    STREAM *S;

    RetStr=CopyStr(RetStr, "");
    Tempstr=CopyStr(Tempstr, GetVar(Act->Vars, "emulator"));
    GetToken(Tempstr, "\\S", &Emu, 0);

    Tempstr=MCopyStr(Tempstr, GetVar(Act->Vars, "sommelier_root"), "/emuhelp/", Emu, ".emuhelp", NULL);

    S=STREAMOpen(Tempstr, "r");
    if (S)
    {
        RetStr=STREAMReadDocument(RetStr, S);
        STREAMClose(S);
    }

    Destroy(Tempstr);

    return(RetStr);
}
