#ifndef SOMMELIER_NATIVE_H
#define SOMMELIER_NATIVE_H

#include "common.h"

char *NativeLoadLibPath(char *RetStr);
char *NativeFindLib(char *RetStr, const char *Lib);
int NativeExecutableCheckLibs(const char *Path, char **Missing);

#endif
