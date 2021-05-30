#ifndef SOMMELIER_PACKAGES_H
#define SOMMELIER_PACKAGES_H

//This module handles various types of packaging for installable programs

#include "common.h"

void PackageUnpack(TAction *Act, const char *Path, int ForcedFileType, const char *FilesToExtract);

#endif
