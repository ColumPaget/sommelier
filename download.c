#include "download.h"
#include "config.h"
#include "apps.h"
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



static int DownloadOpen(TAction *Act, const char *Path)
{


}


static int DownloadCopyFile(TAction *Act)
{
    char *Tempstr=NULL, *URL=NULL, *cwd=NULL;
    const char *ptr;
    STREAM *S;
    int bytes=0;

    URL=SubstituteVarsInString(URL, Act->URL, Act->Vars, 0);
    Tempstr=MCopyStr(Tempstr, "~eDownloading:~0  ~b~e", URL, "~0\n", NULL);
    TerminalPutStr(Tempstr, NULL);


    //must do this even if url proves to be a file on disk, as we will want to ignore it
    //if it's an .exe file, so that we don't mistake it for and installed exectuable
    if (! StrValid(Act->DownName)) Act->DownName=URLBasename(Act->DownName, URL);
    if (StrValid(Act->DownName)) SetVar(Act->Vars, "dlfile", Act->DownName);


    if (StrValid(Act->DownName))
    {
        ptr=GetVar(Act->Vars, "referer");
        if (StrValid(ptr))
        {
            S=STREAMOpen(ptr, "r");
            Tempstr=STREAMReadDocument(Tempstr, S);
            STREAMClose(S);

            Tempstr=MCopyStr(Tempstr, "r Referer=", ptr, NULL);
            S=STREAMOpen(URL, Tempstr);
        }
        else S=STREAMOpen(URL, "r");

        if (S)
        {
            ptr=STREAMGetValue(S, "HTTP:ResponseCode");
            if (StrValid(ptr) && *ptr=='2')
            {
                if (strncasecmp(URL, "https:", 6)==0) DownloadShowSSLStatus(S);
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
            }
            STREAMClose(S);
        }
        else
        {
            Tempstr=MCopyStr(Tempstr, "~rERROR: failed to download:~0  ", URL, NULL);
            TerminalPutStr(Tempstr, NULL);
        }


        //terminate 'downloading' message
        printf("\n");
    }
    else
    {
        Tempstr=MCopyStr(Tempstr, "~rERROR: failed to extract basename from:~0  ", URL, NULL);
        TerminalPutStr(Tempstr, NULL);
    }

    DestroyString(Tempstr);
    DestroyString(URL);
    DestroyString(cwd);

    return(bytes);
}



static char *ExtractURLFromHRef(char *RetStr, const char *Data)
{
    char *Key=NULL, *Value=NULL;
    const char *ptr;

    ptr=GetNameValuePair(Data, "\\S", "=", &Key, &Value);
    while (ptr)
    {
        if (strcasecmp(Key, "href")==0)
        {
            if (Config->Flags & FLAG_DEBUG) printf("DEBUG: Found HREF: %s\n", Value);
            RetStr=CopyStr(RetStr, Value);
            break;
        }
        ptr=GetNameValuePair(ptr, "\\S", "=", &Key, &Value);
    }

    Destroy(Value);
    Destroy(Key);

    return(RetStr);
}


static char *ExtractURLFromWebsite(char *RetStr, TAction *Act)
{
    char *Tempstr=NULL, *SrcPage=NULL, *Template=NULL, *Tag=NULL, *Data=NULL, *URL=NULL;
    char *Proto=NULL, *Host=NULL, *Port=NULL, *Path=NULL;
    const char *ptr;
    STREAM *S;

    ptr=GetToken(Act->URL, ":", &Tempstr, 0);
    ptr=GetToken(ptr, ":", &Tempstr, GETTOKEN_QUOTES);
    SrcPage=UnQuoteStr(SrcPage, Tempstr);
    ptr=GetToken(ptr, ":", &Template, GETTOKEN_QUOTES);

    ParseURL(SrcPage, &Proto, &Host, &Port, NULL, NULL, &Path, NULL);

    //do this late in the process so that we can say Act->URL=ExtractURLFromWebsite(Act->URL, Act);
    //and copy over Act->URL after we've extracted the SrcPage from it
    RetStr=CopyStr(RetStr, "");

    S=STREAMOpen(SrcPage, "");
    if (S)
    {
        Tempstr=STREAMReadDocument(Tempstr, S);
        STREAMClose(S);
    }

    ptr=XMLGetTag(Tempstr, NULL, &Tag, &Data);
    while (ptr)
    {
        if (strcasecmp(Tag, "a")==0)
        {
            if (Config->Flags & FLAG_DEBUG) printf("DEBUG: extract url from 'a' tag: %s\n", Data);
            URL=ExtractURLFromHRef(URL, Data);
            if (StrValid(URL) && (fnmatch(Template, URL, 0)==0))
            {
                if (Config->Flags & FLAG_DEBUG) printf("DEBUG: Found URL [%s] matching template [%s]\n", URL, Template);

                if (*URL=='/') RetStr=FormatStr(RetStr, "%s://%s:%s/%s", Proto, Host, Port, URL);
                else if (strncmp(URL, "../", 3)==0)
                {
                    StrRTruncChar(Path, '/');
                    StrRTruncChar(Path, '/');
                    RetStr=FormatStr(RetStr, "%s://%s:%s/%s/%s", Proto, Host, Port, Path, URL+3);
                }
                else RetStr=CopyStr(RetStr, URL);
                break;
            }
        }
        ptr=XMLGetTag(ptr, NULL, &Tag, &Data);
    }

    if (StrValid(RetStr)) Tempstr=MCopyStr(Tempstr, "~eExtracted URL From Webpage: ~b", RetStr, "~0\n", NULL);
    else Tempstr=MCopyStr(Tempstr, "~rInstall is configured for URL extracted from website, but no matching URL found:~0\n", NULL);
    TerminalPutStr(Tempstr, NULL);

    Destroy(Tempstr);
    Destroy(SrcPage);
    Destroy(Template);
    Destroy(Proto);
    Destroy(Host);
    Destroy(Path);
    Destroy(Data);
    Destroy(Tag);
    Destroy(URL);

    return(RetStr);
}




int DownloadCheck(TAction *Act)
{
    STREAM *S;
    char *Hash=NULL, *URL=NULL;
    const char *ptr;
    int result;

    if (strncmp(Act->URL, "extracted:", 10) ==0) URL=ExtractURLFromWebsite(URL, Act);
    else URL=CopyStr(URL, Act->URL);


    if (! StrValid(URL))
    {
        fprintf(stderr, "ERROR: FAILED URL %s %s\n", Act->Name, Act->URL);
        Destroy(URL);
        return(FALSE);
    }

    S=STREAMOpen(URL, "r");
    if (! S)
    {
        fprintf(stderr, "ERROR: FAILED TO GET %s %s\n", Act->Name, URL);
        Destroy(URL);
        return(FALSE);
    }

    ptr=STREAMGetValue(S, "HTTP:ResponseCode");
    if (*ptr != '2')
    {
        fprintf(stderr, "ERROR: FAILED TO GET %s %s\n", Act->Name, URL);
        STREAMClose(S);
        Destroy(URL);
        return(FALSE);
    }

    result=HashSTREAM(&Hash, "sha256", S, ENCODE_HEX);
    STREAMClose(S);

    if (Hash)
    {
        ptr=GetVar(Act->Vars, "sha256");
        if (StrValid(ptr))
        {
            if (strcmp(ptr, Hash) !=0)
            {
                fprintf(stderr,"ERROR: Hash mismatch for %s %s\n",Act->Name, Act->URL);
            }
        }
        else SetVar(Act->Vars, "sha256", Hash);
    }
    else fprintf(stderr,"ERROR: No hash!\n");

    waitpid(-1,NULL,WNOHANG);

    DestroyString(Hash);
    return(result);
}



int Download(TAction *Act)
{
    char *Tempstr=NULL, *Dest=NULL, *Token=NULL;
    const char *ptr;
    struct stat Stat;
    size_t bytes=0;


    if (StrValid(Act->URL))
    {
        if (strncmp(Act->URL, "extracted:", 10) ==0) Act->URL=ExtractURLFromWebsite(Act->URL, Act);

        if (stat(Act->URL, &Stat)==0)
        {
            printf("source path is an existing file on disk, skipping download\n");
            Act->SrcPath=CopyStr(Act->SrcPath, Act->URL);
            bytes=Stat.st_size;
        }
        else
        {
            bytes=DownloadCopyFile(Act);
            if (bytes < 1)
            {
                TerminalPutStr("~rERROR: URL download failed. Trying again with default lang/locale setting~0\n",NULL);
                AppSetLocale(Act, "en_US");
                bytes=DownloadCopyFile(Act);
            }
        }

        if (bytes > 0)
        {
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
        else TerminalPutStr("~rERROR: URL download failed.~0\n",NULL);
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



