#ifndef COMMON_H
#define COMMON_H

#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>

#define BOARD_SIZE 12
#define CELL_SIZE 60
#define MARGIN_TOP 90
#define MARGIN_LEFT 440
#define TURN_TIME_LIMIT 15.0f

typedef enum { MODE_PVP, MODE_PVE } GameMode;

typedef enum {
    STATE_LOADING,
    STATE_MENU,
    STATE_MODE_SELECT,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER,
    STATE_SAVE_MENU,
    STATE_LOAD_MENU,
    STATE_SETTINGS
} GameState;

typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];
    int currentPlayer;
    float timeRemaining;
    GameState state;
    GameState previousState;
    GameMode mode;
    int winner;
    bool soundEnabled;
    float volume;
    int selectorX;
    int selectorY;
} GameData;

typedef struct {
    int mainMenuIndex;
    int modeMenuIndex;
    int settingsIndex;
    int slotMenuIndex;
    int gameOverIndex;
    int pauseMenuIndex;
} UIState;

extern GameData game;
extern UIState ui;

#endif // COMMON_H