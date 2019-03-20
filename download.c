#include "download.h"

static void DownloadCallback(const char *URL, int bytes, int total)
{
    static float last_perc=0;
    float perc;
    char *ProgBar=NULL;
    int len=0;

    if (bytes==0) last_perc=0;

    if ( total > 0)
    {
    		perc=(float) bytes * 100.0 / (float) total;
        if ((bytes==total) || ((perc - last_perc) > 0.01))
        {
            for (len=0; len < (int) (perc/5); len++) ProgBar=AddCharToBuffer(ProgBar, len, '*');
            for (; len < 20; len++) ProgBar=AddCharToBuffer(ProgBar, len, ' ');

            printf("%0.2f%%  [%s] ", perc, ProgBar);
            printf("(%s/", ToMetric((double) bytes, 2));
            printf("%s)               \r", ToMetric((double) total, 2));
            fflush(NULL);
    				last_perc=perc;
        }
    }
		else if ((perc-last_perc) > 10)
		{
			printf("%s                   \r", ToMetric((double) bytes, 2));
			perc=0;
		}				
	  else perc++;	

    Destroy(ProgBar);
}


int Download(TAction *Act)
{
    char *Tempstr=NULL;
    int result=0;

    if (StrValid(Act->URL))
    {
        Tempstr=TerminalFormatStr(Tempstr, "~eDownloading:~0  ",NULL);
        printf("%s %s\n",Tempstr, Act->URL);

				if (! StrValid(Act->DownName)) Act->DownName=URLBasename(Act->DownName, Act->URL);

        if (StrValid(Act->DownName))
        {
            result=FileCopyWithProgress(Act->URL, Act->DownName, DownloadCallback);

            SetVar(Act->Vars, "dlfile", Act->DownName);
            printf("\n");
        }
        else
        {
            Act->DownName=TerminalFormatStr(Act->DownName, "~rERROR: failed to extract basename from: ~0  ",NULL);
            printf("%s %s\n", Act->DownName, Act->URL);
        }
    }
    else
    {
        Tempstr=TerminalFormatStr(Tempstr, "~rERROR: no download URL configured~0  ",NULL);
        printf("%s\n", Tempstr);
    }
    DestroyString(Tempstr);

    return(result);
}



