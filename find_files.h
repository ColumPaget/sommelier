#ifndef SOMMELIER_FIND_FILES_H
#define SOMMELIER_FIND_FILES_H

#include "common.h"

void FindFiles(const char *Path, const char *Inclusions, const char *Exclusions, ListNode *Founds);
char *FindSingleFile(char *RetStr, const char *Root, const char *File);

#endif
