#include "config.h"

#define DEFAULT_PLATFORMS_PATH "$(install_prefix)/etc/sommelier/platforms.conf,$(homedir)/.sommelier.platforms,$(homedir)/.sommelier/platforms.conf"
#define DEFAULT_CATEGORIES_PATH "$(install_prefix)/etc/sommelier/categories.conf,$(homedir)/.sommelier.categories,$(homedir)/.sommelier/categories.conf"
#define DEFAULT_APPCONFIG_PATH "$(install_prefix)/etc/sommelier/*.apps,$(homedir)/.sommelier.apps,$(homedir)/.sommelier/*.apps"


TConfig *Config=NULL;

void ConfigInit()
{
    char *CABundleEnvVars[]= {"SOMMELIER_CA_BUNDLE", "CURL_CA_BUNDLE", "SSL_CERT_FILE", NULL};
    const char *ptr;
    int i;

    Config=(TConfig *) calloc(1, sizeof(TConfig));
    Config->PlatformsPath=CopyStr(Config->PlatformsPath, DEFAULT_PLATFORMS_PATH);
    Config->AppConfigPath=CopyStr(Config->AppConfigPath, DEFAULT_APPCONFIG_PATH);
    Config->CategoriesPath=CopyStr(Config->CategoriesPath, DEFAULT_CATEGORIES_PATH);

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


    //default user agent. Can be overridden with the -user-agent or -us command-line argument
    LibUsefulSetValue("HTTP:UserAgent","Wget/1.19.2");
}

