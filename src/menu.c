#include "raylib.h"
#include "menu.h"
#include "config.h"

#include <string.h>

#define PLAYER_NAME_MAX 48

static char playerNameXInput[PLAYER_NAME_MAX] = "";
static char playerNameOInput[PLAYER_NAME_MAX] = "";
static UITextInputState playerNameInputStates[2];
static int activePlayerNameInput = 0;

static Rectangle GetButtonRect(int index)
{
    float width = 390;
    float height = 55;
    float x = SCREEN_WIDTH / 2.0f - width / 2.0f;
    float y = 250 + index * 75;

    Rectangle rect = { x, y, width, height };
    return rect;
}

static Rectangle GetChoiceButtonRect(int index)
{
    float width = 360;
    float height = 58;
    float x = SCREEN_WIDTH / 2.0f - width / 2.0f;
    float y = 255 + index * 82;

    Rectangle rect = { x, y, width, height };
    return rect;
}

static Rectangle GetBackButtonRect(void)
{
    float width = 240;
    float height = 48;
    float x = SCREEN_WIDTH / 2.0f - width / 2.0f;
    float y = 525;

    Rectangle rect = { x, y, width, height };
    return rect;
}

static Rectangle GetPlayerNameInputRect(int index)
{
    float width = 440.0f;
    float height = 50.0f;
    float x = SCREEN_WIDTH / 2.0f - width / 2.0f;
    float y = 230.0f + index * 105.0f;

    Rectangle rect = { x, y, width, height };
    return rect;
}

static Rectangle GetPlayerNameStartButtonRect(void)
{
    float width = 260.0f;
    float height = 52.0f;
    float x = SCREEN_WIDTH / 2.0f - width / 2.0f;
    float y = 460.0f;

    Rectangle rect = { x, y, width, height };
    return rect;
}

static int IsButtonClicked(Rectangle rect)
{
    Vector2 mouse = GetVirtualMousePosition();

    return CheckCollisionPointRec(mouse, rect) &&
           IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static void DrawButton(Rectangle rect, const char* text)
{
    Vector2 mouse = GetVirtualMousePosition();
    bool hover = CheckCollisionPointRec(mouse, rect);

    Color bgColor = hover ? SKYBLUE : LIGHTGRAY;
    Color borderColor = hover ? BLUE : DARKGRAY;

    DrawRectangleRec(rect, bgColor);
    DrawRectangleLinesEx(rect, 3, borderColor);

    int fontSize = 26;
    int textWidth = MeasureUIFont(UI_FONT_BUTTON, text, fontSize);

    DrawUIFont(
        UI_FONT_BUTTON,
        text,
        (int)(rect.x + rect.width / 2 - textWidth / 2),
        (int)(rect.y + rect.height / 2 - fontSize / 2),
        fontSize,
        DARKBLUE
    );
}

int UpdateMenu(void)
{
    Rectangle newGameButton = GetButtonRect(0);
    Rectangle loadGameButton = GetButtonRect(1);
    Rectangle settingsButton = GetButtonRect(2);
    Rectangle exitButton = GetButtonRect(3);

    if (IsButtonClicked(newGameButton)) return MENU_NEW_GAME;
    if (IsButtonClicked(loadGameButton)) return MENU_LOAD_GAME;
    if (IsButtonClicked(settingsButton)) return MENU_SETTINGS;
    if (IsButtonClicked(exitButton)) return MENU_EXIT;

    return MENU_NONE;
}

void DrawMenu(void)
{
    const char* title = "CỜ CARO";

    int titleSize = 64;

    DrawUIFont(UI_FONT_TITLE,
               title,
               SCREEN_WIDTH / 2 - MeasureUIFont(UI_FONT_TITLE, title, titleSize) / 2,
               90,
               titleSize,
               DARKBLUE);
    DrawButton(GetButtonRect(0), "Chơi mới");
    DrawButton(GetButtonRect(1), "Tiếp tục");
    DrawButton(GetButtonRect(2), "Cài đặt");
    DrawButton(GetButtonRect(3), "Thoát");
}

int UpdateNewGameModeMenu(void)
{
    Rectangle pvpButton = GetChoiceButtonRect(0);
    Rectangle botButton = GetChoiceButtonRect(1);
    Rectangle backButton = GetBackButtonRect();

    if (IsButtonClicked(pvpButton)) return NEW_GAME_PVP;
    if (IsButtonClicked(botButton)) return NEW_GAME_BOT;
    if (IsButtonClicked(backButton)) return NEW_GAME_BACK;

    return NEW_GAME_NONE;
}

void DrawNewGameModeMenu(void)
{
    const char* title = "CHỌN CHẾ ĐỘ";
    int titleSize = 48;

    DrawUIFont(UI_FONT_HEADING,
               title,
               SCREEN_WIDTH / 2 - MeasureUIFont(UI_FONT_HEADING, title, titleSize) / 2,
               115,
               titleSize,
               DARKBLUE);

    DrawButton(GetChoiceButtonRect(0), "Chơi với người");
    DrawButton(GetChoiceButtonRect(1), "Chơi với máy");
    DrawButton(GetBackButtonRect(), "Quay lại");
}

void ResetPlayerNameMenu(void)
{
    playerNameXInput[0] = '\0';
    playerNameOInput[0] = '\0';
    InitUITextInputState(&playerNameInputStates[0]);
    InitUITextInputState(&playerNameInputStates[1]);
    activePlayerNameInput = 0;
}

int UpdatePlayerNameMenu(void)
{
    Rectangle inputX = GetPlayerNameInputRect(0);
    Rectangle inputO = GetPlayerNameInputRect(1);
    Rectangle startButton = GetPlayerNameStartButtonRect();
    Rectangle backButton = GetBackButtonRect();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        Vector2 mouse = GetVirtualMousePosition();

        if (CheckCollisionPointRec(mouse, inputX))
        {
            activePlayerNameInput = 0;
        }
        else if (CheckCollisionPointRec(mouse, inputO))
        {
            activePlayerNameInput = 1;
        }
    }

    UpdateUITextInput(&playerNameInputStates[0],
                      playerNameXInput,
                      PLAYER_NAME_MAX,
                      inputX,
                      activePlayerNameInput == 0,
                      20);
    UpdateUITextInput(&playerNameInputStates[1],
                      playerNameOInput,
                      PLAYER_NAME_MAX,
                      inputO,
                      activePlayerNameInput == 1,
                      20);

    if (IsButtonClicked(startButton)) return PLAYER_NAMES_START;
    if (IsButtonClicked(backButton)) return PLAYER_NAMES_BACK;

    return PLAYER_NAMES_NONE;
}

void DrawPlayerNameMenu(void)
{
    const char* title = "TÊN NGƯỜI CHƠI";
    int titleSize = 48;

    DrawUIFont(UI_FONT_HEADING,
               title,
               SCREEN_WIDTH / 2 - MeasureUIFont(UI_FONT_HEADING, title, titleSize) / 2,
               95,
               titleSize,
               DARKBLUE);

    DrawText("Người chơi X", (int)GetPlayerNameInputRect(0).x, 200, 20, RED);
    DrawUITextInputBox(GetPlayerNameInputRect(0),
                       &playerNameInputStates[0],
                       playerNameXInput,
                       activePlayerNameInput == 0,
                       20);

    DrawText("Người chơi O", (int)GetPlayerNameInputRect(1).x, 305, 20, BLUE);
    DrawUITextInputBox(GetPlayerNameInputRect(1),
                       &playerNameInputStates[1],
                       playerNameOInput,
                       activePlayerNameInput == 1,
                       20);

    DrawButton(GetPlayerNameStartButtonRect(), "Bắt đầu");
    DrawButton(GetBackButtonRect(), "Quay lại");
}

const char* GetPlayerNameXInput(void)
{
    return playerNameXInput;
}

const char* GetPlayerNameOInput(void)
{
    return playerNameOInput;
}

int UpdateBotDifficultyMenu(void)
{
    Rectangle easyButton = GetChoiceButtonRect(0);
    Rectangle mediumButton = GetChoiceButtonRect(1);
    Rectangle hardButton = GetChoiceButtonRect(2);
    Rectangle backButton = GetBackButtonRect();

    if (IsButtonClicked(easyButton)) return BOT_DIFFICULTY_EASY_ACTION;
    if (IsButtonClicked(mediumButton)) return BOT_DIFFICULTY_MEDIUM_ACTION;
    if (IsButtonClicked(hardButton)) return BOT_DIFFICULTY_HARD_ACTION;
    if (IsButtonClicked(backButton)) return BOT_DIFFICULTY_BACK;

    return BOT_DIFFICULTY_NONE;
}

void DrawBotDifficultyMenu(void)
{
    const char* title = "CHỌN ĐỘ KHÓ";
    int titleSize = 48;

    DrawUIFont(UI_FONT_HEADING,
               title,
               SCREEN_WIDTH / 2 - MeasureUIFont(UI_FONT_HEADING, title, titleSize) / 2,
               95,
               titleSize,
               DARKBLUE);

    DrawText("Người chơi là X, bot là O",
             SCREEN_WIDTH / 2 - MeasureText("Người chơi là X, bot là O", 22) / 2,
             165,
             22,
             DARKGRAY);

    DrawButton(GetChoiceButtonRect(0), "Dễ");
    DrawButton(GetChoiceButtonRect(1), "Vừa");
    DrawButton(GetChoiceButtonRect(2), "Khó");
    DrawButton(GetBackButtonRect(), "Quay lại");
}
