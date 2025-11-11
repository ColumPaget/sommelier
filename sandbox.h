#ifndef SOMMELIER_SANDBOX_H
#define SOMMELIER_SANDBOX_H

#include "common.h"

//on linux we can tell the kernel not to allow switch-user/super-user for this application
int SetNoSU();
char *SeccompSandboxGetLevel(char *RetStr, TAction *Act);

#endif
