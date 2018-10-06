#ifndef SOMMELIER_APPS_H
#define SOMMELIER_APPS_H

#include "common.h"

int AppsOutputList(const char *ConfigPath);
void LoadAppConfigToAct(TAction *Act, const char *Config);
TAction *AppActionCreate(int Action, const char *AppName, const char *ConfigPath);
char *AppFormatPath(char *Path, TAction *Act);

#endif
