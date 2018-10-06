#include "download.h"

static void DownloadCallback(const char *URL, int bytes, int total)
{
static float last_perc=0;
float perc;
char *ProgBar=NULL;
int len=0;

perc=(float) bytes * 100.0 / (float) total;
if (bytes==0) last_perc=0;
if ( total > 0) 
{
	if ((bytes==total) || 	((perc - last_perc) > 0.01))
	{
		for (len=0; len < (int) (perc/5); len++) ProgBar=AddCharToBuffer(ProgBar, len, '*');
		for (;len < 20; len++) ProgBar=AddCharToBuffer(ProgBar, len, ' ');

		printf("%0.2f%%  [%s] ", perc, ProgBar);
		printf("(%s/", ToMetric((double) bytes, 2));
		printf("%s)               \r", ToMetric((double) total, 2));
		fflush(NULL);
	}
}

last_perc=perc;

Destroy(ProgBar);
}


int Download(TAction *Act)
{
char *Tempstr=NULL;
int result;

Tempstr=TerminalFormatStr(Tempstr, "~eDownloading:~0  ",NULL);
printf("%s %s\n",Tempstr, Act->URL);
Tempstr=URLBasename(Tempstr, Act->URL);
result=FileCopyWithProgress(Act->URL, Tempstr, DownloadCallback);

SetVar(Act->Vars, "dlfile", Tempstr);
printf("\n");

DestroyString(Tempstr);

return(result);
}



