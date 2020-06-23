#include "download.h"
#include "config.h"
#include <fnmatch.h>

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



static void DownloadShowSSLStatus(STREAM *S)
{
char *Valid=NULL, *Tempstr=NULL;
char *Issuer=NULL;
const char *ptr;

ptr=STREAMGetValue(S, "SSL:CertificateVerify");
if (StrValid(ptr) && strcmp(ptr, "OK")==0) Valid=MCopyStr(Valid, "~g", ptr, "~0", NULL);
else Valid=MCopyStr(Valid, "~r", ptr, "~0", NULL);

Tempstr=MCopyStr(Tempstr, "~e~mSSL Connection:~0 for ~e", STREAMGetValue(S, "SSL:CertificateCommonName"), "~0  Certificate Verification: ", Valid, "\n", NULL); 
TerminalPutStr(Tempstr, NULL);

ptr=GetToken(STREAMGetValue(S, "SSL:CertificateIssuer"), "/", &Tempstr, 0);
while (ptr)
{
	if (strncmp(Tempstr, "CN=", 3)==0) Issuer=CopyStr(Issuer, Tempstr+3);
	ptr=GetToken(ptr, "/", &Tempstr, 0);
}


Tempstr=MCopyStr(Tempstr, "~e~mCertificate Issued by:~0 ~e", Issuer, "~0 for ", STREAMGetValue(S, "SSL:CertificateSubject"), "\n", NULL);
TerminalPutStr(Tempstr, NULL);


Destroy(Valid);
Destroy(Tempstr);
}


static int DownloadCopyFile(TAction *Act)
{
    char *Tempstr=NULL, *cwd=NULL;
		const char *ptr;
		STREAM *S;
    int bytes=0;

    Tempstr=MCopyStr(Tempstr, "~eDownloading:~0  ~b~e", Act->URL, "~0\n", NULL);
    TerminalPutStr(Tempstr, NULL);

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

			if (S)
			{
				if (strncasecmp(Act->URL, "https:", 6)==0) DownloadShowSSLStatus(S);
 	   		STREAMAddProgressCallback(S, DownloadCallback);

				if (StrValid(Config->InstallerCache))
				{
					Act->SrcPath=MCopyStr(Act->SrcPath, Config->InstallerCache, "/", Act->DownName, NULL);
					MakeDirPath(Act->SrcPath, 0770);
				}
				else
				{
					//this function allocs memory
					cwd=get_current_dir_name();
					Act->SrcPath=MCopyStr(Act->SrcPath, cwd, "/", Act->DownName, NULL);
				}
				Act->Flags |= FLAG_DOWNLOADED;
    		bytes=STREAMCopy(S,  Act->SrcPath);
    		STREAMClose(S);
			}
			else 
			{
	     Tempstr=MCopyStr(Tempstr, "~rERROR: failed to download:~0  ", Act->URL, NULL);
			 TerminalPutStr(Tempstr, NULL);
			}


			//terminate 'downloading' message
      printf("\n");
     }
     else
     {
       Tempstr=MCopyStr(Tempstr, "~rERROR: failed to extract basename from:~0  ", Act->URL, NULL);
			 TerminalPutStr(Tempstr, NULL);
     }

    DestroyString(Tempstr);
    DestroyString(cwd);
		
		return(bytes);
}



char *ExtractURLFromHRef(char *RetStr, const char *Data)
{
char *Key=NULL, *Value=NULL;
const char *ptr;

ptr=GetNameValuePair(Data, "\\S", "=", &Key, &Value);
while (ptr)
{
	if (strcmp(Key, "href")==0)
	{
	RetStr=CopyStr(RetStr, Value);
	break;
	}
	ptr=GetNameValuePair(ptr, "\\S", "=", &Key, &Value);
}

Destroy(Value);
Destroy(Key);

return(RetStr);
}


void ExtractURLFromWebsite(TAction *Act)
{
char *Tempstr=NULL, *SrcPage=NULL, *Template=NULL, *Tag=NULL, *Data=NULL, *URL=NULL;
char *Proto=NULL, *Host=NULL, *Port=NULL;
const char *ptr;
STREAM *S;

ptr=GetToken(Act->URL, ":", &Tempstr, 0);
ptr=GetToken(ptr, ":", &Tempstr, GETTOKEN_QUOTES);
SrcPage=UnQuoteStr(SrcPage, Tempstr);
ptr=GetToken(ptr, ":", &Template, GETTOKEN_QUOTES);

ParseURL(SrcPage, &Proto, &Host, &Port, NULL, NULL, NULL, NULL);

printf("extracting download url from %s\n", SrcPage);

S=STREAMOpen(SrcPage, "");
if (S)
{
Tempstr=STREAMReadDocument(Tempstr, S);
STREAMClose(S);
}

ptr=XMLGetTag(Tempstr, NULL, &Tag, &Data);
while (ptr)
{
	if (strcmp(Tag, "a")==0) 
	{
		URL=ExtractURLFromHRef(URL, Data);
		if (fnmatch(Template, URL, 0)==0)
		{
			if (*URL=='/') Act->URL=FormatStr(Act->URL, "%s://%s:%s/%s", Proto, Host, Port, URL);
			else Act->URL=CopyStr(Act->URL, URL);
			printf("download url found: %s\n", Act->URL);
			break;
		}
	}
	ptr=XMLGetTag(ptr, NULL, &Tag, &Data);
}


Destroy(Tempstr);
Destroy(SrcPage);
Destroy(Template);
Destroy(Proto);
Destroy(Host);
Destroy(Data);
Destroy(Tag);
Destroy(URL);
}



int Download(TAction *Act)
{
  char *Tempstr=NULL, *Dest=NULL, *Token=NULL;
	const char *ptr;
	struct stat Stat;
  size_t bytes=0;


    if (StrValid(Act->URL))
    {
			if (strncmp(Act->URL, "extracted:", 10) ==0) ExtractURLFromWebsite(Act);

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

			//having downloaded the app, we might also download an icon for it 
			ptr=GetVar(Act->Vars, "icon");
			if (StrValid(ptr))
			{
				Tempstr=SubstituteVarsInString(Tempstr, ptr, Act->Vars, 0);
				GetToken(Tempstr, ":", &Token, 0);
				if (
					(strcasecmp(Token, "https")==0) ||	
					(strcasecmp(Token, "http")==0) ||	
					(strcasecmp(Token, "ssh")==0)
				)
				{
				FileCopy(Tempstr, GetBasename(Tempstr));
				Dest=MCopyStr(Dest, GetVar(Act->Vars, "install-dir"), "/", GetBasename(Tempstr), NULL);
				SetVar(Act->Vars, "app-icon", Dest); 
				}
			}
    }
    else
    {
        TerminalPutStr("~rERROR: no download URL configured~0\n",NULL);
    }


    DestroyString(Tempstr);
    DestroyString(Token);
    DestroyString(Dest);

    return(bytes);
}



