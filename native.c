#include "native.h"

int NativeExecutableCheckLibs(const char *Path)
{
char *Tempstr=NULL, *LibName=NULL;
const char *ptr;
STREAM *S;

Tempstr=MCopyStr(Tempstr, "cmd:ldd ", Path, NULL);
S=STREAMOpen(Tempstr, "");
if (S)
{
Tempstr=STREAMReadLine(Tempstr, S);
while (Tempstr)
{
StripLeadingWhitespace(Tempstr);
StripTrailingWhitespace(Tempstr);
ptr=GetToken(Tempstr, " => ", &LibName, 0);
if (strcmp(ptr, "not found")==0)
{
Tempstr=FormatStr(Tempstr, "~rERROR: Executable requires library~0 '~e%s~0'\n", LibName);
TerminalPutStr(Tempstr, NULL);
}
Tempstr=STREAMReadLine(Tempstr, S);
}
STREAMClose(S);
}

Destroy(LibName);
Destroy(Tempstr);
}
