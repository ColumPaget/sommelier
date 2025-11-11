#include "wine-fonts.h"
#include "regedit.h"


ListNode *WineDefaultFontSubs=NULL;


ListNode *WineFontFind(ListNode *Fonts, const char *Name)
{
    char *Tempstr=NULL;
    ListNode *Node;

    Tempstr=MCopyStr(Tempstr, Name, " (TrueType)", NULL);
    Node=ListFindNamedItem(Fonts, Tempstr);
    if (! Node) Node=ListFindNamedItem(Fonts, Name);

    Destroy(Tempstr);

    return(Node);
}

void LookupNewSub(ListNode *InstalledFonts, ListNode *FontSubs, const char *Font)
{
    ListNode *Curr;
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;

    ptr=GetVar(WineDefaultFontSubs, Font);
    if (StrValid(ptr))
    {
        ptr=GetToken(ptr, ",", &Token, 0);
        while (ptr)
        {
            if (WineFontFind(InstalledFonts, Token))
            {
                printf("[%s] > [%s]\n", Font, Token);
                SetVar(FontSubs, Font, Token);
                break;
            }
            ptr=GetToken(ptr, ",", &Token, 0);
        }

    }
    else printf("No Default Font Subs For: [%s]\n", Font);

    Destroy(Tempstr);
    Destroy(Token);
}




void FontNeedsSub(ListNode *Fonts, ListNode *FontSubs, const char *Font)
{
    if (ListFindNamedItem(FontSubs, Font)) /* already subbed, do nothing */ ;
    else if (WineFontFind(Fonts, Font)) fprintf(stderr, "INSTALLED: %s\n", Font);
    else LookupNewSub(Fonts, FontSubs, Font);
}


void WineFonts(TAction *Act)
{
    PARSER *P, *Fonts, *FontSubs, *Curr;
    ListNode *NewSubs=NULL;

    WineDefaultFontSubs=ListCreate();
    SetVar(WineDefaultFontSubs, "Arial", "Roboto,Liberation Sans,DejaVu Sans,Open Sans,Nimbus Sans,FreeSans");
    SetVar(WineDefaultFontSubs, "Helv", "Roboto,Liberation Sans,DejaVu Sans,Open Sans,Nimbus Sans,FreeSans");
    SetVar(WineDefaultFontSubs, "Helvetica", "Roboto,Liberation Sans,DejaVu Sans,Open Sans,Nimbus Sans,FreeSans");
    SetVar(WineDefaultFontSubs, "Verdana", "Roboto,Liberation Sans,DejaVu Sans,Open Sans,Nimbus Sans,FreeSans");
    SetVar(WineDefaultFontSubs, "Tahoma", "Roboto,Liberation Sans,DejaVu Sans,Open Sans,Nimbus Sans,FreeSans");
    SetVar(WineDefaultFontSubs, "Courier", "Inconsolata,Hack,Ubuntu Mono,Liberation Mono,DejaVu Sans Mono,FreeMono");
    SetVar(WineDefaultFontSubs, "Courier New", "Inconsolata,Hack,Ubuntu Mono,Liberation Mono,DejaVu Sans Mono,FreeMono");
    SetVar(WineDefaultFontSubs, "MS Shell Dlg", "Tahoma,Roboto,Verdana,Arial,Helvetica,Liberation Sans,DejaVu Sans,Nimbus Sans,FreeSans");
    SetVar(WineDefaultFontSubs, "MS Shell Dlg 2", "Tahoma,Roboto,Verdana,Arial,Helvetica,Liberation Sans,DejaVu Sans,Nimbus Sans,FreeSans");
    SetVar(WineDefaultFontSubs, "Times New Roman", "EB Garamond,Merriweather,Source Serif,Charis SIL,Liberation Serif,DejaVu Serif,FreeSerif");


    NewSubs=ListCreate();

    P=RegEditExport(Act);

    if (P != NULL)
    {
        Fonts=ParserOpenItem(P, "HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts");
        if (Fonts)
        {
            Curr=ListGetNext(Fonts);
            while (Curr)
            {
                fprintf(stderr, "Font: [%s] -> [%s]\n", Curr->Tag, (const char *) Curr->Item);
                Curr=ListGetNext(Curr);
            }
        }



        FontSubs=ParserOpenItem(P, "HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes");
        if (FontSubs)
        {
            Curr=ListGetNext(FontSubs);
            while (Curr)
            {
                if (WineFontFind(Fonts, Curr->Item)) fprintf(stderr, "AVAILABLE: [%s] -> [%s]\n", Curr->Tag, (const char *) Curr->Item);
                else FontNeedsSub(Fonts, FontSubs, Curr->Tag);

                Curr=ListGetNext(Curr);
            }
        }
    }

    FontNeedsSub(Fonts, FontSubs, "Tahoma");
    FontNeedsSub(Fonts, FontSubs, "Verdana");
    FontNeedsSub(Fonts, FontSubs, "Courier");
    FontNeedsSub(Fonts, FontSubs, "Courier New");
    FontNeedsSub(Fonts, FontSubs, "Times New Roman");
}
