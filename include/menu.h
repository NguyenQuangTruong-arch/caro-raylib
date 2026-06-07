#ifndef MENU_H
#define MENU_H

enum
{
    MENU_NONE = 0,
    MENU_NEW_GAME,
    MENU_LOAD_GAME,
    MENU_SETTINGS,
    MENU_EXIT
};

enum
{
    NEW_GAME_NONE = 0,
    NEW_GAME_PVP,
    NEW_GAME_BOT,
    NEW_GAME_BACK
};

enum
{
    BOT_DIFFICULTY_NONE = 0,
    BOT_DIFFICULTY_EASY_ACTION,
    BOT_DIFFICULTY_MEDIUM_ACTION,
    BOT_DIFFICULTY_HARD_ACTION,
    BOT_DIFFICULTY_BACK
};

enum
{
    PLAYER_NAMES_NONE = 0,
    PLAYER_NAMES_START,
    PLAYER_NAMES_BACK
};

int UpdateMenu(void);
void DrawMenu(void);

int UpdateNewGameModeMenu(void);
void DrawNewGameModeMenu(void);

int UpdateBotDifficultyMenu(void);
void DrawBotDifficultyMenu(void);

void ResetPlayerNameMenu(void);
int UpdatePlayerNameMenu(void);
void DrawPlayerNameMenu(void);
const char* GetPlayerNameXInput(void);
const char* GetPlayerNameOInput(void);

#endif
