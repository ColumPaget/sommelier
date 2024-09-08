#ifndef SOMMELIER_APPS_H
#define SOMMELIER_APPS_H

#include "common.h"
#include "platforms.h"

ListNode *AppsGetList();
char *AppsListExpand(char *FileList, const char *ConfigFiles);
ListNode *AppsLoad(const char *ConfigFiles);
int AppsOutputList(TAction *Act);
void LoadAppConfigToAct(TAction *Act, const char *Config);
char *AppFormatPath(char *Path, TAction *Act);
int AppLoadConfig(TAction *App);
void AppSetLocale(TAction *App, const char *LocaleStr);
int AppIsInstalled(TAction *App);

#endif
