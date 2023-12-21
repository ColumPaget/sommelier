
#ifndef SOMMELIER_DESTKTOP_FILES_H
#define SOMMELIER_DESTKTOP_FILES_H

#include "common.h"

int DesktopFileDelete(TAction *Act);
int DesktopFileLoad(TAction *Act);
void DesktopFileGenerate(TAction *Act);
void DesktopFileDirectoryRunAll(const char *DirPath);

#endif

