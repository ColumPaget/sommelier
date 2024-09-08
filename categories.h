#ifndef SOMMELIER_CATEGORIES_H
#define SOMMELIER_CATEGORIES_H

#include "common.h"

int CategoriesLoad(const char *CatFile);
char *CategoriesExpand(char *RetStr, const char *Input);
char *CategoriesRegisterFromApp(char *RetStr, const char *Input);
void CategoriesList();

#endif
