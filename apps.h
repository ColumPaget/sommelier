#ifndef SOMMELIER_APPS_H
#define SOMMELIER_APPS_H

#include "common.h"
#include "platforms.h"

#define AppAllowSU(App) AppAllowDeny((App), CONF_ALLOW_SU, CONF_DENY_SU, FLAG_ALLOW_SU, 0, FALSE)
#define AppAllowNet(App) AppAllowDeny((App), CONF_ALLOW_NET, CONF_DENY_NET, FLAG_ALLOW_NET, FLAG_DENY_NET, TRUE)
#define AppAllowPids(App) AppAllowDeny((App), CONF_ALLOW_PID, CONF_DENY_PID, FLAG_ALLOW_PID, FLAG_DENY_PID, FALSE)


ListNode *AppsGetList();
char *AppsListExpand(char *FileList, const char *ConfigFiles);
void AppsLoadFromFile(const char *Path, ListNode *Apps);
ListNode *AppsLoad(const char *ConfigFiles);
int AppsOutputList(TAction *Act);
void LoadAppConfigToAct(TAction *Act, const char *Config);
int AppLoadConfig(TAction *App);
void AppSetLocale(TAction *App, const char *LocaleStr);

char *AppFormatPath(char *Path, TAction *Act, const char *InstallPrefix);
char *AppFindInstalled(char *, TAction *App);
int AppIsInstalled(TAction *App);
int AppPlatformMatches(TAction *App, const char *Platforms);

int AppAllowDeny(TAction *App, int ConfAllow, int ConfigDeny, int AppAllow, int AppDeny, int Default);
int AppsListAllowSU(ListNode *Apps);
#endif
