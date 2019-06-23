#include "config.h"

#define DEFAULT_APPCONFIG_PATH "/etc/sommelier.apps,/etc/sommelier/*.apps,$(homedir)/.sommelier.apps,$(homedir)/.sommelier/*.apps"


TConfig *Config=NULL;

void ConfigInit()
{
Config=(TConfig *) calloc(1, sizeof(TConfig));
Config->AppConfigPath=CopyStr(Config->AppConfigPath, DEFAULT_APPCONFIG_PATH);
}

