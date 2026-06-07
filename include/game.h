#ifndef GAME_H
#define GAME_H

#include "config.h"

typedef enum GameMode
{
    GAME_MODE_PVP = 0,
    GAME_MODE_BOT
} GameMode;

typedef enum BotDifficulty
{
    BOT_DIFFICULTY_EASY = 0,
    BOT_DIFFICULTY_MEDIUM,
    BOT_DIFFICULTY_HARD
} BotDifficulty;

void InitGame(void);
void ResetGame(void);
void ResetMatchScore(void);
void StartNewGame(GameMode mode, BotDifficulty difficulty);
void StartNewGameWithNames(GameMode mode, BotDifficulty difficulty, const char* nameX, const char* nameO);

void UpdateGame(AppScreen* currentScreen);
void UpdateGameOver(AppScreen* currentScreen);

void UpdatePauseMenu(AppScreen* currentScreen);
void DrawPauseMenu(void);

void DrawGame(void);

int SaveGameToFile(const char* fileName);
int LoadGameFromFile(const char* fileName);

#endif
