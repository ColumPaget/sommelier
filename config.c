#include "config.h"

#define DEFAULT_APPCONFIG_PATH "/etc/sommelier.apps,/etc/sommelier/*.apps,$(homedir)/.sommelier.apps,$(homedir)/.sommelier/*.apps"


TConfig *Config=NULL;

void ConfigInit()
{
char *CABundleEnvVars[]={"SOMMELIER_CA_BUNDLE", "CURL_CA_BUNDLE", "SSL_CERT_FILE", NULL};
const char *ptr;
int i;

Config=(TConfig *) calloc(1, sizeof(TConfig));
Config->AppConfigPath=CopyStr(Config->AppConfigPath, DEFAULT_APPCONFIG_PATH);

for (i=0; CABundleEnvVars[i] !=NULL; i++)
{
ptr=getenv(CABundleEnvVars[i]);
if (StrValid(ptr)) 
{
	LibUsefulSetValue("SSL:VerifyCertFile", ptr);
//	printf("Confirming SSL certificates using '%s' from '%s'\n", ptr, CABundleEnvVars[i]);
	break;
}
}

ptr=getenv("SOMMELIER_INSTALLER_CACHE");
if (StrValid(ptr)) Config->InstallerCache=CopyStr(Config->InstallerCache, ptr);
}

