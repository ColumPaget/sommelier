//This module handles various types of packaging for installable programs


#include "packages.h"
#include "find_files.h"

static int PackageTypeFromExtn(const char *extn)
{
    int FT=FILETYPE_UNKNOWN;

    if (*extn == '.') extn++;

    if (strcasecmp(extn, "deb")==0) FT=FILETYPE_DEB;
    else if (strcasecmp(extn, "tgz")==0) FT=FILETYPE_TGZ;
    else if (strcasecmp(extn, "tbz")==0) FT=FILETYPE_TBZ;
    else if (strcasecmp(extn, "tbz2")==0) FT=FILETYPE_TBZ;
    else if (strcasecmp(extn, "txz")==0) FT=FILETYPE_TXZ;
    else if (strcasecmp(extn, "zip")==0) FT=FILETYPE_ZIP;
    else if (strcasecmp(extn, "7z")==0) FT=FILETYPE_7ZIP;
    else if (strcasecmp(extn, "7za")==0) FT=FILETYPE_7ZIP;
    else if (strcasecmp(extn, "7zip")==0) FT=FILETYPE_7ZIP;
    else if (strcasecmp(extn, "tar.gz")==0) FT=FILETYPE_TGZ;
    else if (strcasecmp(extn, "tar.bz2")==0) FT=FILETYPE_TBZ;
    else if (strcasecmp(extn, "tar.xz")==0) FT=FILETYPE_TXZ;
    else if (strcasecmp(extn, "msi")==0) FT=FILETYPE_MSI;
    else if (strcasecmp(extn, "cab")==0) FT=FILETYPE_CAB;

    return(FT);
}


int PackageTypeFromFile(const char *Path)
{
    STREAM *S;
    char *Tempstr=NULL;
    const char *extn;
    int FT=FILETYPE_UNKNOWN;

    //first try to identify it from the first few bytes of the file
    S=STREAMOpen(Path, "r");
    if (S)
    {
        Tempstr=SetStrLen(Tempstr, 255);
        STREAMReadBytes(S, Tempstr, 4);

        if (strcmp(Tempstr, "\x50\x4b\x03\x04")==0) FT=FILETYPE_ZIP;
        else if (strcmp(Tempstr, "\xd0\xcf\x11\xe0")==0) FT=FILETYPE_MSI;
        else if (strncmp(Tempstr, "PE", 2)==0) FT=FILETYPE_PE;
        else if (strncmp(Tempstr, "MZ", 2)==0) FT=FILETYPE_MZ;
        else if (strncmp(Tempstr, "7z", 2)==0) FT=FILETYPE_7ZIP;

        STREAMClose(S);
    }

    //if type is still unknown, try matching the file extension for certain filetypes that don't have
    //a magic that definitely describes them
    if (FT==FILETYPE_UNKNOWN)
    {
        extn=strrchr(Path, '.');
        if (StrValid(extn)) FT=PackageTypeFromExtn(extn);
    }

    DestroyString(Tempstr);

    return(FT);
}



static int PackageResolveType(TAction *Act, const char *Path, int ForcedType)
{
    int PackageType=FILETYPE_UNKNOWN;
    const char *ptr;

    if (ForcedType != FILETYPE_UNKNOWN) return(ForcedType);

    //we can force the type of the downloaded file, which allows us to override things like .zip files that have
    //a self-extracting program stub on the front
    ptr=GetVar(Act->Vars, "download-type");
    if (StrValid(ptr)) PackageType=PackageTypeFromExtn(ptr);
    else PackageType=PackageTypeFromFile(Path);

    return(PackageType);
}



static int PackageRunUnpacker(const char *Path, TAction *Act, int PackageType, const char *FilesToExtract)
{
    char *CmdLine=NULL, *Tempstr=NULL, *CmdConfig=NULL;
    const char *ptr;


    CmdConfig=CopyStr(CmdConfig, "noshell");
    switch (PackageType)
    {
    case FILETYPE_7ZIP:
        CmdLine=MCopyStr(CmdLine, "7za x -y '",Path, "' ", FilesToExtract, NULL);
        break;


    case FILETYPE_ZIP:
        CmdLine=MCopyStr(CmdLine, "unzip -o '",Path, "' ", FilesToExtract, NULL);
        break;

    case FILETYPE_TGZ:
    case FILETYPE_TBZ:
    case FILETYPE_TXZ:
        CmdLine=MCopyStr(CmdLine, "tar -xf '",Path, "' ", FilesToExtract, NULL);
        break;

    case FILETYPE_MSI:
        //wine msiexec /i whatever.msi
        CmdLine=MCopyStr(CmdLine, "WINEPREFIX=", GetVar(Act->Vars, "prefix"), " wine msiexec /i '", Path, "'", NULL);
        CmdConfig=CopyStr(CmdConfig, "");
        break;

    case FILETYPE_DEB:
        CmdLine=MCopyStr(CmdLine, "ar x '",Path, "' ", NULL);
        printf("unpacking: %s\n",GetBasename(Path));
        RunProgramAndConsumeOutput(CmdLine, "noshell");

        Tempstr=FindSingleFile(Tempstr, GetVar(Act->Vars, "install-dir"), "data.tar,data.tar.gz,data.bz2,data.tar.xz");
        if (StrValid(Tempstr)) CmdLine=MCopyStr(CmdLine, "tar -xf '",Path, "' ", FilesToExtract, NULL);
        else CmdLine=CopyStr(CmdLine, "");
        break;

    case FILETYPE_CAB:
        ptr=GetVar(Act->Vars, "extract-filter");
        if (StrValid(ptr)) Tempstr=MCopyStr(Tempstr, " -F '", ptr, "' ", NULL);
        else Tempstr=CopyStr(Tempstr, "");
        CmdLine=MCopyStr(CmdLine, "cabextract ", Tempstr, " '", Path, "'", NULL);
        break;
    }

    if (StrValid(CmdLine))
    {
        printf("unpacking: %s\n",GetBasename(Path));
        printf("unpacking: %s into %s\n",CmdLine, get_current_dir_name());
        RunProgramAndConsumeOutput(CmdLine, CmdConfig);
    }

    Destroy(CmdLine);
    Destroy(CmdConfig);
    Destroy(Tempstr);
}


//Some packages contain an installer program. Find that and set 'installer-path'
//for use later
static void PackageFindAndRunInstaller(TAction *Act)
{
    const char *ptr;
    char *Tempstr=NULL;

    //for zipfiles and the like the installer has to be found. Either it's
    //specified in the app config, as 'installer'
    //or we go looking for certain common filenames

    ptr=GetVar(Act->Vars, "installer");
    if (! ptr) ptr="setup.exe,install.exe,*.msi";
    Tempstr=FindSingleFile(Tempstr, GetVar(Act->Vars, "install-dir"), ptr);
    if (StrValid(Tempstr))
    {
        printf("Found installer program: %s\n", Tempstr);
        SetVar(Act->Vars, "installer-path", Tempstr);
    }

    Destroy(Tempstr);
}


//This function handles situations where a package contains another package, possibly in a different format
void PackageUnpackInner(TAction *Act, const char *Path, int ForcedFileType, const char *FilesToExtract)
{
    const char *ptr;
    char *Tempstr=NULL, *PkgType=NULL;

    ptr=GetVar(Act->Vars, "inner-package");
    if (StrValid(ptr))
    {
        if (strchr(ptr, ':')) ptr=GetToken(ptr, ":", &PkgType, GETTOKEN_QUOTES);
        if (StrValid(PkgType)) ForcedFileType=PackageTypeFromExtn(PkgType);
        Tempstr=FindSingleFile(Tempstr, GetVar(Act->Vars, "prefix"), ptr);
        if (StrValid(Tempstr))
        {
            PackageUnpack(Act, Tempstr, ForcedFileType, FilesToExtract);
        }
    }

    Destroy(Tempstr);
    Destroy(PkgType);
}


void PackageUnpack(TAction *Act, const char *Path, int ForcedFileType, const char *FilesToExtract)
{
    int FT=FILETYPE_UNKNOWN;


//java .jar file are zips, but we don't want to unpack them
    if (Act->InstallType == INSTALL_EXECUTABLE) SetVar(Act->Vars, "exec", GetBasename(Act->URL));
    else
    {

//if the install/platform type specifies that the file is a given file type then obey that
//otherwise figure out the package file type
        if (ForcedFileType != FILETYPE_UNKNOWN) FT=ForcedFileType;
        else FT=PackageResolveType(Act, Path, FT);

//actually unpack the file. This will only work on known package types, so FILETYPE_PE and FILETYPE_MZ will not
//be messed with, and are prepared for final install below.
        PackageRunUnpacker(Path, Act, FT, FilesToExtract);

// these are things we do after the package has unpacked
        switch (FT)
        {
        case FILETYPE_7ZIP:
        case FILETYPE_ZIP:
            //find and run installer program. This will default to searching for 'setup.exe' which might be
            //a problem, but hasn't been so yet
            PackageFindAndRunInstaller(Act);
            break;

        case FILETYPE_PE:
        case FILETYPE_MZ:
            //downloaded file is an installer, set the install-path to point to it for use later
            SetVar(Act->Vars, "installer-path", Path);
            break;

        }
    }
}


