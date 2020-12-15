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
#define REG_GDI3D 128
#define REG_OPENGL3D 256
#define REG_OPENGLSL 512

void RegEdit(TAction *Act, int Flags, const char *OSVersion, const char *Resolution, const char *DLLOverrides);
void RegEditApplySettings(TAction *Act);


#endif
