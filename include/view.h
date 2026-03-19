#ifndef VIEW_H
#define VIEW_H

#include "common.h"

void View_Init(void);
void View_LoadAssets(void);
void View_UnloadAssets(void);
void View_UpdateAudio(void);
void View_PlaySFX(void);
void View_Draw(int loadingProgress);

#endif // VIEW_H