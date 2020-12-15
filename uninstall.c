#include "uninstall.h"
#include "desktopfiles.h"
#include "apps.h"
#include "config.h"

int UninstallDir(TAction *Act, const char *Dir)
{
    char *Tempstr=NULL;
    const char *p_Path, *p_Base;
    glob_t Glob;
    struct stat Stat;
    int i;

    FileSystemUnMount(Dir, "");
    Tempstr=MCatStr(Tempstr, Dir, "/*", NULL);
    glob(Tempstr, GLOB_PERIOD, 0, &Glob);
    for (i=0; i < Glob.gl_pathc; i++)
    {
        p_Path=Glob.gl_pathv[i];
        p_Base=GetBasename(p_Path);
        if ((strcmp(p_Base, ".") !=0) && (strcmp(p_Base, "..") !=0) )
        {
            lstat(p_Path, &Stat);
            if (S_ISDIR(Stat.st_mode))
            {
                UninstallDir(Act, p_Path);
            }
            else
            {
                unlink(p_Path);
                if (Config->Flags & FLAG_DEBUG) printf("Deleted file: [%s]\n", p_Path);
            }
        }
    }

    if (rmdir(Dir)==0)
    {
        if (Config->Flags & FLAG_DEBUG) printf("Deleted directory: [%s]\n",Dir);
        Destroy(Tempstr);
        return(TRUE);
    }


    Tempstr=FormatStr(Tempstr, "ERROR: Can't delete directory [%s]: ", Dir);
    perror(Tempstr);

    Destroy(Tempstr);
    return(FALSE);
}


void UnInstallApp(TAction *Act)
{
    char *Tempstr=NULL;

    printf("Uninstall: %s\n", Act->Name);
//We don't need this path, but this function also sets the 'prefix' var,
//which we do need for this operation
    Tempstr=AppFormatPath(Tempstr, Act);
    Tempstr=CopyStr(Tempstr, GetVar(Act->Vars, "prefix"));
    UninstallDir(Act, Tempstr);
    DesktopFileDelete(Act);

    Destroy(Tempstr);
}
