#ifndef SOMMELIER_APPS_H
#define SOMMELIER_APPS_H

#include "common.h"
#include "platforms.h"

ListNode *AppsGetList();
ListNode *AppsLoad(const char *ConfigFiles);
int AppsOutputList(TAction *Act);
void LoadAppConfigToAct(TAction *Act, const char *Config);
TAction *AppActionCreate(int Action, const char *AppName, const char *Platform);
char *AppFormatPath(char *Path, TAction *Act);
int AppLoadConfig(TAction *App);

#endif
