#include "xrandr.h"



int XRandrGetResolution(int *width, int *height)
{
char *Tempstr=NULL, *Token=NULL;
const char *ptr;
STREAM *S;

S=STREAMSpawnCommand("xrandr -q", "");
if (S)
{
	Tempstr=STREAMReadLine(Tempstr, S);
	ptr=GetToken(Tempstr, " ", &Token, 0);
	while (ptr)
	{
	if (strcmp(Token, "current")==0)
	{
	ptr=GetToken(ptr, " ", &Token, 0);
	*width=atoi(Token);
	ptr=GetToken(ptr, " ", &Token, 0);
	ptr=GetToken(ptr, " ", &Token, 0);
	*height=atoi(Token);
	}
	ptr=GetToken(ptr, " ", &Token, 0);
	}
	STREAMClose(S);
}

Destroy(Tempstr);
}



int XRandrSetResolution(int width, int height)
{
char *Tempstr=NULL;

Tempstr=FormatStr(Tempstr, "xrandr -s %dx%d", width, height);
printf("XRANDR: %s\n", Tempstr);
Spawn(Tempstr, "");

Destroy(Tempstr);
}
