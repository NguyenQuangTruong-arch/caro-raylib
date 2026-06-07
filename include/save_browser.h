#ifndef SAVE_BROWSER_H
#define SAVE_BROWSER_H

#include "config.h"

#define BROWSER_SAVE 1
#define BROWSER_LOAD 2

void OpenSaveBrowser(AppScreen* currentScreen, int mode, AppScreen returnScreen);
void UpdateSaveBrowser(AppScreen* currentScreen);
void DrawSaveBrowser(void);

#endif