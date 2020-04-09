#ifndef SOMMELIER_CONFIG_H
#define SOMMELIER_CONFIG_H

#include "common.h"

typedef struct
{
int Flags;
char *AppConfigPath;
char *InstallerCache;
} TConfig;

extern TConfig *Config;

void ConfigInit();

#endif
