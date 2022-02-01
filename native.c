#include "native.h"



char *NativeLoadLibPath(char *RetStr)
{
    STREAM *S;
    char *Tempstr=NULL;

    S=STREAMOpen("/etc/ld.so.conf", "r");
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            RetStr=MCatStr(RetStr, Tempstr, ":", NULL);
            Tempstr=STREAMReadLine(Tempstr, S);
        }
        STREAMClose(S);
    }

    RetStr=CatStr(RetStr, getenv("LD_LIBRARY_PATH"));

    Destroy(Tempstr);
    return(RetStr);
}


char *NativeFindLib(char *RetStr, const char *Lib)
{
    char *Tempstr=NULL;

    RetStr=CopyStr(RetStr, "");
    Tempstr=NativeLoadLibPath(Tempstr);
    RetStr=FindFileInPath(RetStr, Lib, Tempstr);

    Destroy(Tempstr);
    return(RetStr);
}


int NativeExecutableCheckLibs(const char *Path, char **Missing)
{
    char *Tempstr=NULL, *LibName=NULL;
    const char *ptr;
    STREAM *S;
    int RetVal=TRUE; //false if missing libs

    if (Missing) *Missing=CopyStr(*Missing, "");
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
                if (Missing) *Missing=MCatStr(*Missing, LibName, " ", NULL);
                RetVal=FALSE;
            }
            Tempstr=STREAMReadLine(Tempstr, S);
        }
        STREAMClose(S);
    }

    Destroy(LibName);
    Destroy(Tempstr);

    return(RetVal);
}
