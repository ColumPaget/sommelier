#include "categories.h"
#include "apps.h"

ListNode *CatMap=NULL;

static int CategoriesLoadFromFile(const char *CatFile)
{
    char *Tempstr=NULL, *Name=NULL;
    const char *ptr;
    int RetVal=FALSE;
    STREAM *S;

    S=STREAMOpen(CatFile, "r");
    if (S)
    {
        RetVal=TRUE;
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            StripLeadingWhitespace(Tempstr);
            StripTrailingWhitespace(Tempstr);
            ptr=GetToken(Tempstr, "\\S", &Name, 0);
            SetVar(CatMap, Name, ptr);
            Tempstr=STREAMReadLine(Tempstr, S);
        }
        STREAMClose(S);
    }
    Destroy(Tempstr);
    Destroy(Name);

    return(RetVal);
}

int CategoriesLoad(const char *FilesPath)
{
    char *Token=NULL, *Path=NULL;
    const char *ptr;

    if (! CatMap) CatMap=MapCreate(1024, 0);
    ptr=GetToken(FilesPath, ",", &Token, 0);
    while (ptr)
    {
        Path=FormatPath(Path, Token);
        CategoriesLoadFromFile(Path);
        ptr=GetToken(ptr, ",", &Token, 0);
    }

    Destroy(Token);
    Destroy(Path);

    return(TRUE);
}

static void CategoriesAddExpansions(ListNode *Cats, const char *Cat)
{
    char *Expanse=NULL, *Name=NULL;
    const char *ptr;

    Expanse=CopyStr(Expanse, GetVar(CatMap, Cat));
    if (StrValid(Expanse))
    {
        ptr=GetToken(Expanse, ";", &Name, 0);
        while (ptr)
        {
            if (StrValid(Name)) SetVar(Cats, Name, Name);
            ptr=GetToken(ptr, ";", &Name, 0);
        }

    }

    Destroy(Expanse);
    Destroy(Name);
}


char *CategoriesExpand(char *RetStr, const char *Input)
{
    ListNode *Cats, *Curr, *Node;
    const char *ptr;
    char *Cat=NULL;

    Cats=ListCreate();

    ptr=GetToken(Input, ";", &Cat, 0);
    while (ptr)
    {
	if (StrValid(Cat))
	{
        SetVar(Cats, Cat, Cat);
        CategoriesAddExpansions(Cats, Cat);
	}

        ptr=GetToken(ptr, ";", &Cat, 0);
    }

    RetStr=CopyStr(RetStr, "");
    Curr=ListGetNext(Cats);
    while (Curr)
    {
        RetStr=MCatStr(RetStr, Curr->Tag, ";", NULL);
	//we do this here, not earlier in the process,
	//in order to be sure we only count once
        Node=ListFindNamedItem(CatMap, Curr->Tag);
	if (! Node) Node=ListAddNamedItem(CatMap, Curr->Tag, NULL);
	
	//we are abusing item type here, because ListNodeHits records number of lookups
	//and there´s no way of using it as a counter, without changing libuseful
	Node->ItemType++;

        Curr=ListGetNext(Curr);
    }


    ListDestroy(Cats, Destroy);
    Destroy(Cat);

    return(RetStr);
}


void CategoriesList()
{
ListNode *Curr;
char *Tempstr=NULL;
TAction *App;

Curr=ListGetNext(AppsGetList());
while (Curr)
{
  App=(TAction *) Curr->Item;
  Tempstr=CategoriesExpand(Tempstr, GetVar(App->Vars, "category"));
  Curr=ListGetNext(Curr);
}


Curr=ListGetNext(CatMap);
while (Curr)
{
if (Curr->ItemType > 0) printf("%30s  %6d items\n", Curr->Tag, Curr->ItemType);
Curr=ListGetNext(Curr);
}

Destroy(Tempstr);
}
