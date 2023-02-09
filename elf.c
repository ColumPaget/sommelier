#include "elf.h"

int ELFFileGetBitWidth(const char *Path)
{
    STREAM *S;
    char HDR[10];
    int result, RetVal=0;

    S=STREAMOpen(Path, "r");
    if (S)
    {
        result=STREAMReadBytes(S, HDR, 5);
        if ((result > 4) && (memcmp(HDR+1, "ELF", 3)==0) )
        {
            if (HDR[4]==1) RetVal=32;
            else if (HDR[4]==2) RetVal=64;
        }
        STREAMClose(S);
    }

    return(RetVal);
}
