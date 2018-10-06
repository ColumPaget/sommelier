#ifndef SOMMELIER_REGEDIT_H
#define SOMMELIER_REGEDIT_H

#include "common.h"

#define REG_WINMANAGER 1
#define REG_NO_WINMANAGER 2
#define REG_NO_GRAB 4
#define REG_VDESK   8
#define REG_NO_VDESK   16
#define REG_FONT_SMOOTH 32 
#define REG_NO_FONT_SMOOTH 64

void RegEdit(TAction *Act, int Flags);
void RegeditApplySettings(TAction *Act);


#endif
