#include "download.h"
#include "config.h"

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


int DownloadCopyFile(TAction *Act)
{
    char *Tempstr=NULL, *cwd=NULL;
		const char *ptr;
		STREAM *S;
    int bytes=0, result;

    Tempstr=TerminalFormatStr(Tempstr, "~eDownloading:~0  ",NULL);
    printf("%s %s\n",Tempstr, Act->URL);

    if (StrValid(Act->DownName))
    {
			ptr=GetVar(Act->Vars, "referer");
			if (StrValid(ptr))
			{
				S=STREAMOpen(ptr, "r");
				Tempstr=STREAMReadDocument(Tempstr, S);
				STREAMClose(S);

				Tempstr=MCopyStr(Tempstr, "r Referer=", ptr, NULL);
				S=STREAMOpen(Act->URL, Tempstr);
			}
			else S=STREAMOpen(Act->URL, "r");

    	STREAMAddProgressCallback(S, DownloadCallback);
    	result=STREAMCopy(S,  Act->DownName);
    	STREAMClose(S);


			//this function allocs memory
			cwd=get_current_dir_name();
			Act->SrcPath=MCopyStr(Act->SrcPath, cwd, "/", Act->DownName, NULL);

      printf("\n");
     }
     else
     {
       Tempstr=TerminalFormatStr(Tempstr, "~rERROR: failed to extract basename from: ~0  ",NULL);
       printf("%s %s\n", Tempstr, Act->URL);
     }

    DestroyString(Tempstr);
    DestroyString(cwd);
		
		return(bytes);
}


int Download(TAction *Act)
{
    char *Tempstr=NULL, *cwd=NULL;
		struct stat Stat;
    size_t bytes=0;


    if (StrValid(Act->URL))
    {
			//must do this even if url proves to be a file on disk, as we will want to ignore it
			//if it's an .exe file, so that we don't mistake it for and installed exectuable
			if (! StrValid(Act->DownName)) Act->DownName=URLBasename(Act->DownName, Act->URL);
      if (StrValid(Act->DownName)) SetVar(Act->Vars, "dlfile", Act->DownName);

			if (stat(Act->URL, &Stat)==0)
			{
				printf("source path is an existing file on disk, skipping download\n");
				Act->SrcPath=CopyStr(Act->SrcPath, Act->URL);
				bytes=Stat.st_size;
			}
			else bytes=DownloadCopyFile(Act);
    }
    else
    {
        Tempstr=TerminalFormatStr(Tempstr, "~rERROR: no download URL configured~0  ",NULL);
        printf("%s\n", Tempstr);
    }
    DestroyString(Tempstr);

    return(bytes);
}



