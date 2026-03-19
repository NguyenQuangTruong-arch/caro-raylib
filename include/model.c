#include "model.h"
#include <stdlib.h>

GameData game;
UIState ui;

void Model_ResetBoard(void) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) game.board[i][j] = 0;
    }
    game.currentPlayer = 1;
    game.timeRemaining = TURN_TIME_LIMIT;
    game.winner = 0;
    game.selectorX = BOARD_SIZE / 2;
    game.selectorY = BOARD_SIZE / 2;
}

void Model_Init(void) {
    game.soundEnabled = true;
    game.volume = 0.5f;
    game.state = STATE_LOADING;
    game.previousState = STATE_MENU;
    game.mode = MODE_PVP;

    ui.mainMenuIndex = 0;
    ui.modeMenuIndex = 0;
    ui.settingsIndex = 0;
    ui.slotMenuIndex = 0;
    ui.gameOverIndex = 0;
    ui.pauseMenuIndex = 0;

    Model_ResetBoard();
}

bool Model_CheckWin(int player) {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (game.board[y][x] != player) continue;
            if (x <= BOARD_SIZE - 5 && game.board[y][x + 1] == player && game.board[y][x + 2] == player && game.board[y][x + 3] == player && game.board[y][x + 4] == player) return true;
            if (y <= BOARD_SIZE - 5 && game.board[y + 1][x] == player && game.board[y + 2][x] == player && game.board[y + 3][x] == player && game.board[y + 4][x] == player) return true;
            if (x <= BOARD_SIZE - 5 && y <= BOARD_SIZE - 5 && game.board[y + 1][x + 1] == player && game.board[y + 2][x + 2] == player && game.board[y + 3][x + 3] == player && game.board[y + 4][x + 4] == player) return true;
            if (x >= 4 && y <= BOARD_SIZE - 5 && game.board[y + 1][x - 1] == player && game.board[y + 2][x - 2] == player && game.board[y + 3][x - 3] == player && game.board[y + 4][x - 4] == player) return true;
        }
    }
    return false;
}

bool Model_CheckStalemate(void) {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (game.board[y][x] == 0) return false;
        }
    }
    return true;
}

void Model_SaveGame(int slot) {
    char filename[64];
    snprintf(filename, sizeof(filename), "saveFiles/save_slot_%d.dat", slot);
    FILE* file = fopen(filename, "wb");
    if (file) {
        GameState tempState = game.state;
        game.state = STATE_PLAYING;
        fwrite(&game, sizeof(GameData), 1, file);
        game.state = tempState;
        fclose(file);
    }
}

bool Model_LoadGame(int slot) {
    char filename[64];
    snprintf(filename, sizeof(filename), "saveFiles/save_slot_%d.dat", slot);
    FILE* file = fopen(filename, "rb");
    if (file) {
        fread(&game, sizeof(GameData), 1, file);
        fclose(file);
        return true;
    }
    return false;
}

bool Model_SaveExists(int slot) {
    char filename[32];
    snprintf(filename, sizeof(filename), "saveFiles/save_slot_%d.dat", slot);
    return FileExists(filename);
}

static int GetCellScore(int x, int y, int player) {
    int score = 0;
    int dx[] = { 1, 0, 1, 1 };
    int dy[] = { 0, 1, 1, -1 };

    for (int d = 0; d < 4; d++) {
        int count = 0;
        int open = 0;

        for (int i = 1; i <= 4; i++) {
            int nx = x + dx[d] * i, ny = y + dy[d] * i;
            if (nx < 0 || ny < 0 || nx >= BOARD_SIZE || ny >= BOARD_SIZE) break;
            if (game.board[ny][nx] == player) count++;
            else if (game.board[ny][nx] == 0) { open++; break; }
            else break;
        }
        for (int i = 1; i <= 4; i++) {
            int nx = x - dx[d] * i, ny = y - dy[d] * i;
            if (nx < 0 || ny < 0 || nx >= BOARD_SIZE || ny >= BOARD_SIZE) break;
            if (game.board[ny][nx] == player) count++;
            else if (game.board[ny][nx] == 0) { open++; break; }
            else break;
        }

        if (count >= 4) score += 100000; 
        else if (count == 3 && open == 2) score += 10000;
        else if (count == 3 && open == 1) score += 1000;
        else if (count == 2 && open == 2) score += 500;
        else if (count == 2 && open == 1) score += 50;
        else if (count == 1 && open == 2) score += 10;
    }
    return score;
}

void Model_MakeAIMove(void) {
    int bestScore = -1;
    int bestX = BOARD_SIZE / 2;
    int bestY = BOARD_SIZE / 2;

    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (game.board[i][j] == 0) {
                int attackScore = GetCellScore(j, i, 2);
                int defendScore = GetCellScore(j, i, 1);

                int totalScore = attackScore + (int)(defendScore * 1.2f);

                totalScore += GetRandomValue(0, 5);

                if (totalScore > bestScore) {
                    bestScore = totalScore;
                    bestX = j;
                    bestY = i;
                }
            }
        }
    }

    game.board[bestY][bestX] = 2;
}