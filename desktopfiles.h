
#ifndef SOMMELIER_DESTKTOP_FILES_H
#define SOMMELIER_DESTKTOP_FILES_H

#include "common.h"

int DesktopFileDelete(TAction *Act);
int DesktopFileRead(TAction *Act);
void DesktopFileGenerate(TAction *Act, const char *Path);

#endif

