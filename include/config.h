#ifndef CONFIG_H
#define CONFIG_H

#include "raylib.h"

#define VIRTUAL_WIDTH 1280
#define VIRTUAL_HEIGHT 720

#define SCREEN_WIDTH VIRTUAL_WIDTH
#define SCREEN_HEIGHT VIRTUAL_HEIGHT

#define BOARD_SIZE 15
#define CELL_SIZE 36

#define BOARD_X 80
#define BOARD_Y 90
#define BOARD_PIXEL_SIZE (BOARD_SIZE * CELL_SIZE)

#define WIN_LENGTH 5

typedef enum AppScreen
{
    SCREEN_MENU = 0,
    SCREEN_NEW_GAME_MODE,
    SCREEN_PLAYER_NAMES,
    SCREEN_BOT_DIFFICULTY,
    SCREEN_GAME,
    SCREEN_PAUSE,
    SCREEN_SETTINGS,
    SCREEN_GAME_OVER,
    SCREEN_SAVE,
    SCREEN_LOAD,
    SCREEN_EXIT
} AppScreen;

typedef enum UIFontRole
{
    UI_FONT_TITLE = 0,
    UI_FONT_BUTTON,
    UI_FONT_HEADING,
    UI_FONT_TEXT,
    UI_FONT_COUNT
} UIFontRole;

typedef enum GameSoundEffect
{
    GAME_SOUND_CHECK = 0,
    GAME_SOUND_WIN,
    GAME_SOUND_GAME_OVER,
    GAME_SOUND_COUNT
} GameSoundEffect;

typedef struct UITextInputState
{
    int cursor;
    int selectionStart;
    int selectionEnd;
    int selectionAnchor;
} UITextInputState;

float GetScreenScaleFactor(void);
Vector2 GetScreenOffset(void);
Vector2 GetVirtualMousePosition(void);

void LoadGameFonts(void);
void UnloadGameFonts(void);

void LoadGameAudio(void);
void UnloadGameAudio(void);
void UpdateGameAudioForScreen(AppScreen screen);
void PlayGameSoundEffect(GameSoundEffect effect);
float GetGameMusicVolume(void);
float GetGameSoundVolume(void);
void SetGameMusicVolume(float volume);
void SetGameSoundVolume(float volume);

int IsTurnTimerEnabled(void);
int GetTurnTimeLimitSeconds(void);
void SetTurnTimerEnabled(int enabled);
void SetTurnTimeLimitSeconds(int seconds);

void DrawUIFont(UIFontRole role, const char* text, int posX, int posY, int fontSize, Color color);
int MeasureUIFont(UIFontRole role, const char* text, int fontSize);

void InitUITextInputState(UITextInputState* state);
void SetUITextInputCursorToEnd(UITextInputState* state, const char* text);
void UpdateUITextInput(UITextInputState* state, char* buffer, int capacity, Rectangle rect, int active, int fontSize);
void DrawUITextInputBox(Rectangle rect, const UITextInputState* state, const char* text, int active, int fontSize);

#define DrawText(text, posX, posY, fontSize, color) \
    DrawUIFont(UI_FONT_TEXT, text, posX, posY, fontSize, color)

#define MeasureText(text, fontSize) \
    MeasureUIFont(UI_FONT_TEXT, text, fontSize)

#endif
