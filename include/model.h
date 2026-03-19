#ifndef MODEL_H
#define MODEL_H

#include "common.h"

void Model_Init(void);
void Model_ResetBoard(void);
bool Model_CheckWin(int player);
bool Model_CheckStalemate(void);
void Model_SaveGame(int slot);
bool Model_LoadGame(int slot);
bool Model_SaveExists(int slot);
void Model_MakeAIMove(void);

#endif // MODEL_H