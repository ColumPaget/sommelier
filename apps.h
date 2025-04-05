#ifndef SOMMELIER_APPS_H
#define SOMMELIER_APPS_H

#include "common.h"
#include "platforms.h"

ListNode *AppsGetList();
char *AppsListExpand(char *FileList, const char *ConfigFiles);
ListNode *AppsLoad(const char *ConfigFiles);
int AppsOutputList(TAction *Act);
void LoadAppConfigToAct(TAction *Act, const char *Config);
int AppLoadConfig(TAction *App);
void AppSetLocale(TAction *App, const char *LocaleStr);

char *AppFormatPath(char *Path, TAction *Act, const char *InstallPrefix);
char *AppFindInstalled(char *, TAction *App);
int AppIsInstalled(TAction *App);

#endif
