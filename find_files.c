#include "find_files.h"
#include "elf.h"

static int FindCheckBitWidth(const char *Path, int BitWidth)
{
    int FileWidth;

    FileWidth=ELFFileGetBitWidth(Path);
    if ( (BitWidth==32) && (FileWidth==64)) return(FALSE);
    if ( (BitWidth==64) && (FileWidth==32)) return(FALSE);

    return(TRUE);
}


void FindItems(const char *Path, const char *Inclusions, const char *Exclusions, int BitWidth, ListNode *Founds)
{
    char *Tempstr=NULL;
    struct stat FStat;
    glob_t Glob;
    int i;
    const char *IgnoreDirs[]= {"windows", "Windows NT", "Internet Explorer", "Windows Media Player", NULL};
    const char *ptr, *p_Basename;

    Tempstr=CopyStr(Tempstr, Path);
    Tempstr=SlashTerminateDirectoryPath(Tempstr);
    Tempstr=CatStr(Tempstr, "*");


    glob(Tempstr, 0, 0, &Glob);
    for (i=0; i < Glob.gl_pathc; i++)
    {
        lstat(Glob.gl_pathv[i], &FStat);
        p_Basename=GetBasename(Glob.gl_pathv[i]);
        if (p_Basename)
        {
            if (S_ISLNK(FStat.st_mode)) /*Do nothing, don't follow links */ ;
            else if (S_ISDIR(FStat.st_mode))
            {
                if (MatchTokenFromList(p_Basename, IgnoreDirs, 0) == -1) FindItems(Glob.gl_pathv[i], Inclusions, Exclusions, BitWidth, Founds);
            }
            else
            {
                Tempstr=CopyStr(Tempstr, p_Basename);
                //must strlwr here, as InList will do so to Inclusions and Exclusions
                strlwr(Tempstr);
                if ( InList(Tempstr, Inclusions) && (! InList(Tempstr, Exclusions)) && FindCheckBitWidth(Glob.gl_pathv[i], BitWidth))
                {
                    //can't use 'SetVar' here as multiple files might have the same basename, but different paths
                    ListAddNamedItem(Founds, p_Basename, CopyStr(NULL, Glob.gl_pathv[i]));
                }
            }
        }
    }
    globfree(&Glob);
    Destroy(Tempstr);
}


void FindFiles(const char *Path, const char *Inclusions, const char *Exclusions, ListNode *Founds)
{
    return(FindItems(Path, Inclusions, Exclusions, 0, Founds));
}


char *FindSingleFile(char *RetStr, const char *Root, const char *File)
{
    ListNode *Files, *Curr;

    RetStr=CopyStr(RetStr, "");
    Files=ListCreate();

    FindFiles(Root, File, "", Files);

    Curr=ListGetNext(Files);
    if (Curr) RetStr=CopyStr(RetStr, Curr->Item);

    ListDestroy(Files, Destroy);
    return(RetStr);
}

