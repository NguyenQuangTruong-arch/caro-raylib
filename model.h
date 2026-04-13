#ifndef MODEL_H
#define MODEL_H

#include "common.h"

extern char saveFilesList[100][64];
extern int saveFilesCount;

void Model_Init(void);
void Model_ResetBoard(void);
bool Model_CheckWin(int player);
bool Model_CheckStalemate(void);
void Model_SaveGame(const char* filename);
bool Model_LoadGame(const char* filename);
bool Model_SaveExists(const char* filename);
void Model_DeleteSave(const char* filename); // <-- HÀM MỚI
void Model_MakeAIMove(void);
void Model_RefreshSaveFiles(void);

#endif // MODEL_H