#ifndef SOMMELIER_RUN_APPLICATION_H
#define SOMMELIER_RUN_APPLICATION_H

#include "common.h"

void RunApplication(TAction *Act);
void RunApplicationFromDesktopFile(TAction *Act);
void RunWineUtility(TAction *Act, const char *Utility);

#endif
