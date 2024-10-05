#include "common.h"
#include "config.h"
#include <fnmatch.h>

#define DEFAULT_WINEPREFIX "$(sommelier_root)$(name)-$(platform)/"

const char *ResolveVar(ListNode *Vars, const char *VarName)
{
    char *Tempstr=NULL;
    const char *ptr;

    ptr=GetVar(Vars, VarName);
    Tempstr=SubstituteVarsInString(Tempstr, ptr, Vars, 0);
    SetVar(Vars, VarName, Tempstr);

    Destroy(Tempstr);
    return(GetVar(Vars, VarName));
}

char *FormatPath(char *RetStr, const char *Fmt)
{
    ListNode *Vars;

    Vars=ListCreate();
    SetVar(Vars, "homedir", GetCurrUserHomeDir());
    SetVar(Vars, "install_prefix", INSTALL_PREFIX);
    RetStr=SubstituteVarsInString(RetStr, Fmt, Vars, 0);
    ListDestroy(Vars, Destroy);

    return(RetStr);
}


char *URLBasename(char *RetStr, const char *URL)
{
    char *Tempstr=NULL;
    char *ptr;

    Tempstr=CopyStr(Tempstr, URL);
    StrTruncChar(Tempstr, '?');
    StripDirectorySlash(Tempstr);
    RetStr=CopyStr(RetStr, GetBasename(Tempstr));

    Destroy(Tempstr);
    return(RetStr);
}



int InList(const char *Item, const char *List)
{
    char *Match=NULL, *ItemLwr=NULL;
    const char *ptr;

    if (! StrValid(Item)) return(FALSE);

    //strlwr everything to handle case
    ItemLwr=CopyStr(ItemLwr, Item);
    strlwr(ItemLwr);

    ptr=GetToken(List, ",|;", &Match, GETTOKEN_QUOTES|GETTOKEN_MULTI_SEP);
    while (ptr)
    {
        strlwr(Match);

//if match starts with ! this inverts the sense of the match, so anything
//that *doesn't match* matches, and we have an exclusion
        if (StrValid(Match))
        {
            if (*Match=='!')
            {
                if (fnmatch(Match+1, ItemLwr, 0) != 0)
                {
                    Destroy(ItemLwr);
                    Destroy(Match);
                    return(TRUE);
                }
            }
            else if (fnmatch(Match, ItemLwr, 0) == 0)
            {
                Destroy(ItemLwr);
                Destroy(Match);
                return(TRUE);
            }
        }

        ptr=GetToken(ptr, ",|;", &Match, GETTOKEN_QUOTES|GETTOKEN_MULTI_SEP);
    }

    Destroy(ItemLwr);
    Destroy(Match);

    return(FALSE);
}




TAction *ActionCreate(int Type, const char *Name)
{
    TAction *Act;

    Act=(TAction *) calloc(1, sizeof(TAction));
    Act->Type=Type;
    Act->Vars=ListCreate();
    Act->Name=CopyStr(Act->Name, Name);
    if (StrValid(Name)) SetVar(Act->Vars, "name", Name);

    SetVar(Act->Vars, "prefix_template", DEFAULT_WINEPREFIX);
    SetVar(Act->Vars, "user", LookupUserName(getuid()));
    SetVar(Act->Vars, "homedir", GetCurrUserHomeDir());
    return(Act);
}



void ActionDestroy(TAction *Act)
{
    Destroy(Act->Name);
    Destroy(Act->URL);
    Destroy(Act->DownName);
    Destroy(Act->InstallPath);
    Destroy(Act->Exec);

    ListDestroy(Act->Vars, Destroy);
    free(Act);
}






int CompareSha256(TAction *Act)
{
    char *Hash=NULL, *Tempstr=NULL;
    const char *p_ExpectedHash;
    int result=FALSE;


    p_ExpectedHash=GetVar(Act->Vars, "sha256");
    if (StrValid(p_ExpectedHash))
    {
        HashFile(&Hash, "sha256", Act->SrcPath, ENCODE_HEX);
        if (strcmp(Hash, p_ExpectedHash)==0) result=TRUE;
        Tempstr=CopyStr(Tempstr, "");
        printf("    expected sha256: [%s]\n",p_ExpectedHash);
        printf("    actual   sha256: [%s]\n",Hash);
        Tempstr=CopyStr(Tempstr, "");
        if (result) TerminalPutStr("~g~eOKAY:~w Hashes match~0\n",NULL);
        else TerminalPutStr("~rERROR: Downloaded file does not match expected hash~0\n",NULL);
    }
    else
    {
        TerminalPutStr("~mNo expected hash value is configured for this download~0. This probably means that the source link is updated when a new version is released, but this means sommelier cannot confirm your download's integrity or security.\n", NULL);
        if (Act->Flags & FLAG_HASH_DOWNLOAD)
        {
            HashFile(&Hash, "sha256", Act->SrcPath, ENCODE_HEX);
            printf("    actual   sha256: [%s]\n",Hash);
        }
        result=TRUE;
    }

    Destroy(Tempstr);
    Destroy(Hash);

    return(result);
}


//Some installers fork into background, perhaps calling 'setsid', which means we
//will no longer consider them child processes and will no longer wait for them.
//Holding open a pipe for their output seems to overcome this, and also allows us
//to suppress a lot of crap that they might print out.
void RunProgramAndConsumeOutput(const char *Cmd, const char *SpawnConfig)
{
    STREAM *S;
    char *Tempstr=NULL;

    S=STREAMSpawnCommand(Cmd, SpawnConfig);
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            if (Config->Flags & FLAG_DEBUG) printf("DBG: %s", Tempstr);
            Tempstr=STREAMReadLine(Tempstr, S);
        }
        STREAMClose(S);
    }
    Destroy(Tempstr);

    while (wait(0) > 1);
}

int ParseBool(const char *Value)
{
    if (! StrValid(Value)) return(FALSE);
    if (atoi(Value) > 0) return(TRUE);
    if (*Value=='y') return(TRUE);
    if (*Value=='Y') return(TRUE);
    if (strcasecmp(Value, "true")==0) return(TRUE);

    return(FALSE);
}

int GetBoolVar(ListNode *Vars, const char *Name)
{
    return(ParseBool(GetVar(Vars, Name)));
}


char *GlobNoCase(char *RetStr, const char *Dir, const char *Name)
{
    char *Tempstr=NULL;
    const char *ptr;
    glob_t Glob;
    int i;

    RetStr=CopyStr(RetStr, "");
    //we go through all the files, and find any that match the request
    //but we do a case insensitive match
    Tempstr=MCopyStr(Tempstr, Dir, "/*", NULL);
    glob(Tempstr, 0, 0, &Glob);
    for (i=0; i < Glob.gl_pathc; i++)
    {
        ptr=GetBasename(Glob.gl_pathv[i]);
        if (pmatch_one(Name, ptr, StrLen(ptr), NULL, NULL, PMATCH_NOCASE))
        {
            RetStr=CopyStr(RetStr, Glob.gl_pathv[i]);
            break;
        }
    }
    globfree(&Glob);

    Destroy(Tempstr);

    return(RetStr);
}
