#include "view.h"
#include <math.h>

static Texture2D bgTexture;
static Texture2D texX;
static Texture2D texO;
static Music bgm;
static Sound sfxMenu;

static Color gridColor = { 200, 200, 200, 150 };
static Color uiColor = { 220, 225, 220, 255 };
static Color textColor = { 240, 245, 240, 255 };
static Color disabledColor = { 100, 105, 100, 255 };
static Color overlayColor = { 10, 15, 20, 230 };
static Color highlightColor = { 0, 255, 255, 255 };

void View_Init(void) {
    bgTexture.id = 0;
}

void View_LoadAssets(void) {
    bgTexture = LoadTexture("resources/background.png");
    texX = LoadTexture("resources/X.png");
    texO = LoadTexture("resources/O.png");
    SetTextureFilter(bgTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(texX, TEXTURE_FILTER_POINT);
    SetTextureFilter(texO, TEXTURE_FILTER_POINT);
    bgm = LoadMusicStream("resources/bgm.mp3");
    sfxMenu = LoadSound("resources/sfx.mp3");
    PlayMusicStream(bgm);
}

void View_UnloadAssets(void) {
    UnloadTexture(bgTexture);
    UnloadTexture(texX);
    UnloadTexture(texO);
    UnloadMusicStream(bgm);
    UnloadSound(sfxMenu);
}

void View_UpdateAudio(void) {
    if (game.soundEnabled) {
        UpdateMusicStream(bgm);
        SetMusicVolume(bgm, game.volume);
        SetSoundVolume(sfxMenu, game.volume);
    }
    else {
        SetMusicVolume(bgm, 0.0f);
        SetSoundVolume(sfxMenu, 0.0f);
    }
}

void View_PlaySFX(void) {
    if (game.soundEnabled)
        PlaySound(sfxMenu);
}

static void DrawTextCentered(Rectangle rect, const char* text, int fontSize, Color color) {
    int textWidth = MeasureText(text, fontSize);
    int x = rect.x + (rect.width - textWidth) / 2;
    int y = rect.y + (rect.height - fontSize) / 2;
    DrawText(text, x, y, fontSize, color);
}

void View_Draw(int loadingProgress) {
    BeginDrawing();
    ClearBackground(BLACK);

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    if (game.state == STATE_LOADING) {
        DrawTextCentered((Rectangle) { screenW / 2 - 200, screenH / 2 - 50, 400, 100 }, "LOADING...", 40, textColor);
        DrawRectangleLines(screenW / 2 - 150, screenH / 2 + 50, 300, 30, textColor);
        DrawRectangle(screenW / 2 - 150, screenH / 2 + 50, (300 * loadingProgress) / 100, 30, textColor);
        EndDrawing();
        return;
    }

    Rectangle sourceBG = { 0, 0, (float)bgTexture.width, (float)bgTexture.height };
    Rectangle destBG = { 0, 0, screenW, screenH };
    DrawTexturePro(bgTexture, sourceBG, destBG, (Vector2) { 0, 0 }, 0.0f, WHITE);

    if (game.state == STATE_MENU) {
        DrawRectangle(0, 0, screenW, screenH, overlayColor);
        DrawTextCentered((Rectangle) { screenW / 2 - 200, 150, 400, 100 }, "LIMINAL CARO", 60, textColor);

        Rectangle btnPlay = { screenW / 2 - 150, 350, 300, 60 };
        Rectangle btnLoadMenuMain = { screenW / 2 - 150, 430, 300, 60 };
        Rectangle btnSettings = { screenW / 2 - 150, 510, 300, 60 };
        Rectangle btnQuit = { screenW / 2 - 150, 590, 300, 60 };

        DrawRectangleLinesEx(btnPlay, 2, ui.mainMenuIndex == 0 ? highlightColor : uiColor);
        DrawTextCentered(btnPlay, "PLAY", 20, ui.mainMenuIndex == 0 ? highlightColor : textColor);
        DrawRectangleLinesEx(btnLoadMenuMain, 2, ui.mainMenuIndex == 1 ? highlightColor : uiColor);
        DrawTextCentered(btnLoadMenuMain, "LOAD", 20, ui.mainMenuIndex == 1 ? highlightColor : textColor);
        DrawRectangleLinesEx(btnSettings, 2, ui.mainMenuIndex == 2 ? highlightColor : uiColor);
        DrawTextCentered(btnSettings, "SETTINGS", 20, ui.mainMenuIndex == 2 ? highlightColor : textColor);
        DrawRectangleLinesEx(btnQuit, 2, ui.mainMenuIndex == 3 ? highlightColor : uiColor);
        DrawTextCentered(btnQuit, "QUIT", 20, ui.mainMenuIndex == 3 ? highlightColor : textColor);
    }
    else if (game.state == STATE_MODE_SELECT) {
        DrawRectangle(0, 0, screenW, screenH, overlayColor);
        DrawTextCentered((Rectangle) { screenW / 2 - 200, 150, 400, 100 }, "SELECT MODE", 50, textColor);

        Rectangle btnPvP = { screenW / 2 - 150, 350, 300, 60 };
        Rectangle btnPvE = { screenW / 2 - 150, 430, 300, 60 };
        Rectangle btnBackMode = { screenW / 2 - 150, 510, 300, 60 };

        DrawRectangleLinesEx(btnPvP, 2, ui.modeMenuIndex == 0 ? highlightColor : uiColor);
        DrawTextCentered(btnPvP, "PLAYER VS PLAYER", 20, ui.modeMenuIndex == 0 ? highlightColor : textColor);

        DrawRectangleLinesEx(btnPvE, 2, ui.modeMenuIndex == 1 ? highlightColor : uiColor);
        DrawTextCentered(btnPvE, "PLAYER VS AI", 20, ui.modeMenuIndex == 1 ? highlightColor : textColor);

        DrawRectangleLinesEx(btnBackMode, 2, ui.modeMenuIndex == 2 ? highlightColor : uiColor);
        DrawTextCentered(btnBackMode, "BACK", 20, ui.modeMenuIndex == 2 ? highlightColor : textColor);
    }
    else if (game.state == STATE_SETTINGS) {
        DrawRectangle(0, 0, screenW, screenH, overlayColor);
        DrawTextCentered((Rectangle) { screenW / 2 - 200, 150, 400, 100 }, "SETTINGS", 50, textColor);

        Rectangle audioToggleBtn = { screenW / 2 - 150, 380, 40, 40 };
        Rectangle volSliderTrack = { screenW / 2 - 150, 480, 300, 30 };
        Rectangle btnSettingsBack = { screenW / 2 - 150, 580, 300, 60 };

        DrawText("AUDIO ENABLED:", audioToggleBtn.x, audioToggleBtn.y - 30, 20, textColor);
        DrawRectangleLinesEx(audioToggleBtn, 2, ui.settingsIndex == 0 ? highlightColor : uiColor);
        if (game.soundEnabled)
            DrawRectangle(audioToggleBtn.x + 8, audioToggleBtn.y + 8, 24, 24, ui.settingsIndex == 0 ? highlightColor : textColor);

        DrawText("MASTER VOLUME (A/D to adjust):", volSliderTrack.x, volSliderTrack.y - 30, 20, textColor);
        DrawRectangleLinesEx(volSliderTrack, 2, ui.settingsIndex == 1 ? highlightColor : uiColor);
        Rectangle sliderHandle = { volSliderTrack.x + (game.volume * volSliderTrack.width) - 10, volSliderTrack.y - 10, 20, 50 };
        DrawRectangleRec(sliderHandle, ui.settingsIndex == 1 ? highlightColor : textColor);

        DrawRectangleLinesEx(btnSettingsBack, 2, ui.settingsIndex == 2 ? highlightColor : uiColor);
        DrawTextCentered(btnSettingsBack, "BACK", 20, ui.settingsIndex == 2 ? highlightColor : textColor);
    }
    else if (game.state == STATE_SAVE_MENU || game.state == STATE_LOAD_MENU) {
        DrawRectangle(0, 0, screenW, screenH, overlayColor);
        DrawTextCentered((Rectangle) { screenW / 2 - 200, 100, 400, 100 }, game.state == STATE_SAVE_MENU ? "SAVE NEW GAME" : "SEARCH / LOAD GAME", 40, textColor);

        extern char saveFilesList[100][64];

        // 1. Ô Textbox
        Rectangle txtBox = { screenW / 2 - 200, 200, 400, 60 };

        // Đổi màu viền khung nến đang bật chế độ nhập
        Color boxColor = uiColor;
        if (ui.slotMenuIndex == 0) boxColor = highlightColor;
        if (ui.isInputMode) boxColor = GREEN;

        DrawRectangleLinesEx(txtBox, ui.isInputMode ? 3 : 2, boxColor);

        // Văn bản hướng dẫn
        if (ui.slotMenuIndex == 0) {
            DrawTextCentered((Rectangle) { screenW / 2 - 200, 265, 400, 20 },
                ui.isInputMode ? "[ESC] to Stop Typing  |  [ENTER] to Confirm" : "[ENTER] to Start Typing", 16, GRAY);
        }

        // Hiện chữ và con trỏ nhấp nháy
        char displayStr[128];
        if (ui.isInputMode && (int)(GetTime() * 2) % 2 == 0) {
            snprintf(displayStr, sizeof(displayStr), "%s_", ui.inputText);
        }
        else {
            snprintf(displayStr, sizeof(displayStr), "%s", ui.inputText);
        }
        DrawTextCentered(txtBox, displayStr, 30, textColor);

        // 2. Thông báo file
        if (game.state == STATE_LOAD_MENU && ui.letterCount > 0 && !Model_SaveExists(ui.inputText) && !ui.isInputMode) {
            DrawTextCentered((Rectangle) { screenW / 2 - 200, 285, 400, 30 }, "File does not exist!", 20, RED);
        }
        else if (game.state == STATE_SAVE_MENU && ui.letterCount > 0 && Model_SaveExists(ui.inputText) && !ui.isInputMode) {
            DrawTextCentered((Rectangle) { screenW / 2 - 200, 285, 400, 30 }, "Warning: File will be overwritten!", 20, ORANGE);
        }

        // 3. Danh sách File
        int startY = 320;
        if (ui.displayFilesCount == 0 && game.state == STATE_LOAD_MENU) {
            DrawTextCentered((Rectangle) { screenW / 2 - 200, startY, 400, 40 }, "No files found", 20, disabledColor);
        }
        else {
            if (ui.slotMenuIndex == 2) {
                DrawTextCentered((Rectangle) { screenW / 2 - 200, startY - 25, 400, 20 }, "[DELETE] to Remove Save", 16, RED);
            }

            for (int i = 0; i < ui.displayFilesCount; i++) {
                int fileIdx = ui.displayFilesIndices[i];
                Rectangle fileRect = { screenW / 2 - 200, startY + i * 50, 400, 40 };

                bool isSelected = (ui.slotMenuIndex == 2 && ui.saveListSelectedIndex == i);
                DrawRectangleRec(fileRect, isSelected ? Fade(highlightColor, 0.3f) : Fade(uiColor, 0.1f));
                DrawRectangleLinesEx(fileRect, 1, isSelected ? highlightColor : uiColor);
                DrawTextCentered(fileRect, saveFilesList[fileIdx], 20, isSelected ? highlightColor : textColor);
            }
        }

        // 4. Nút Back
        Rectangle btnBack = { screenW / 2 - 150, 620, 300, 60 };
        DrawRectangleLinesEx(btnBack, 2, ui.slotMenuIndex == 1 ? highlightColor : uiColor);
        DrawTextCentered(btnBack, "BACK", 20, ui.slotMenuIndex == 1 ? highlightColor : textColor);

    }
    else { // <-- Giữ nguyên phần vẽ bàn cờ và Game Over ở dưới...
        DrawRectangle(0, 0, screenW, screenH, (Color) { 0, 0, 0, 180 });

        const char* turnText = (game.currentPlayer == 1) ? "TURN: PLAYER X" : "TURN: PLAYER O";
        DrawText(turnText, MARGIN_LEFT, 30, 40, textColor);
        DrawText(TextFormat("TIME LIMIT: %.1f", game.timeRemaining), MARGIN_LEFT + 550, 35, 30, (game.timeRemaining <= 5.0f) ? RED : textColor);

        Color colorX = (game.currentPlayer == 1) ? WHITE : Fade(WHITE, 0.2f);
        Color colorO = (game.currentPlayer == 2) ? WHITE : Fade(WHITE, 0.2f);

        Rectangle sourceTexX = { 0, 0, texX.width, texX.height };
        Rectangle destBigX = { 220 - 125, 450 - 125, 250, 250 };
        DrawTexturePro(texX, sourceTexX, destBigX, (Vector2) { 0, 0 }, 0.0f, colorX);

        Rectangle sourceTexO = { 0, 0, texO.width, texO.height };
        Rectangle destBigO = { 1380 - 125, 450 - 125, 250, 250 };
        DrawTexturePro(texO, sourceTexO, destBigO, (Vector2) { 0, 0 }, 0.0f, colorO);

        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                Rectangle cell = { MARGIN_LEFT + j * CELL_SIZE, MARGIN_TOP + i * CELL_SIZE, CELL_SIZE, CELL_SIZE };
                DrawRectangleLinesEx(cell, 1, gridColor);

                if (game.board[i][j] == 1)
                    DrawTextCentered(cell, "X", 40, RED);
                else if (game.board[i][j] == 2)
                    DrawTextCentered(cell, "O", 40, BLUE);
            }
        }

        if (game.state == STATE_PLAYING) {
            Rectangle selectorRect = { MARGIN_LEFT + game.selectorX * CELL_SIZE, MARGIN_TOP + game.selectorY * CELL_SIZE, CELL_SIZE, CELL_SIZE };
            DrawRectangleLinesEx(selectorRect, 3, highlightColor);
        }

        if (game.state == STATE_PAUSED) {
            DrawRectangle(0, 0, screenW, screenH, overlayColor);
            DrawTextCentered((Rectangle) { screenW / 2 - 200, screenH / 2 - 250, 400, 100 }, "PAUSED", 60, textColor);

            Rectangle btnPauseContinue = { screenW / 2 - 150, screenH / 2 - 120, 300, 60 };
            Rectangle btnPauseSave = { screenW / 2 - 150, screenH / 2 - 40, 300, 60 };
            Rectangle btnPauseLoad = { screenW / 2 - 150, screenH / 2 + 40, 300, 60 };
            Rectangle btnPauseQuit = { screenW / 2 - 150, screenH / 2 + 120, 300, 60 };

            DrawRectangleLinesEx(btnPauseContinue, 2, ui.pauseMenuIndex == 0 ? highlightColor : uiColor);
            DrawTextCentered(btnPauseContinue, "CONTINUE", 20, ui.pauseMenuIndex == 0 ? highlightColor : textColor);

            DrawRectangleLinesEx(btnPauseSave, 2, ui.pauseMenuIndex == 1 ? highlightColor : uiColor);
            DrawTextCentered(btnPauseSave, "SAVE GAME", 20, ui.pauseMenuIndex == 1 ? highlightColor : textColor);

            DrawRectangleLinesEx(btnPauseLoad, 2, ui.pauseMenuIndex == 2 ? highlightColor : uiColor);
            DrawTextCentered(btnPauseLoad, "LOAD GAME", 20, ui.pauseMenuIndex == 2 ? highlightColor : textColor);

            DrawRectangleLinesEx(btnPauseQuit, 2, ui.pauseMenuIndex == 3 ? highlightColor : uiColor);
            DrawTextCentered(btnPauseQuit, "QUIT TO MENU", 20, ui.pauseMenuIndex == 3 ? highlightColor : textColor);
        }
        else if (game.state == STATE_GAME_OVER) {
            DrawRectangle(0, 0, screenW, screenH, (Color) { 0, 0, 0, 220 });

            float t = (float)GetTime();

            if (game.winner != 0) {
                for (int i = 0; i < 80; i++) {
                    float speed = 150.0f + (i % 50);
                    float fallY = fmodf(t * speed + (i * 87.3f), screenH + 100.0f) - 50.0f;
                    float swayX = sinf(t * 2.0f + i) * 60.0f;
                    float posX = fmodf((i * 13.7f) * screenW, screenW) + swayX;
                    float rot = t * (100.0f + i % 30);

                    Color confColor = ColorFromHSV(fmodf(i * 50.0f + t * 40.0f, 360.0f), 0.8f, 0.9f);

                    Rectangle confRec = { posX, fallY, 12.0f + (i % 8), 12.0f + (i % 8) };
                    DrawRectanglePro(confRec, (Vector2) { confRec.width / 2, confRec.height / 2 }, rot, confColor);
                }

                Texture2D winTex = (game.winner == 1) ? texX : texO;
                Color baseColor = (game.winner == 1) ? RED : BLUE;

                float scaleX = 2.5f + 0.1f * sinf(t * 4.0f);
                float scaleY = 2.5f + 0.1f * cosf(t * 4.0f);
                float rotation = sinf(t * 1.5f) * 10.0f;
                float bobY = sinf(t * 3.0f) * 20.0f;

                Rectangle source = { 0, 0, (float)winTex.width, (float)winTex.height };
                Rectangle dest = { screenW / 2.0f, 280.0f + bobY, winTex.width * scaleX, winTex.height * scaleY };
                Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };

                for (int i = 0; i < 4; i++) {
                    float glowPulse = sinf(t * 8.0f + i) * 10.0f;
                    Rectangle glowDest = { dest.x, dest.y, dest.width + i * 40 + glowPulse, dest.height + i * 40 + glowPulse };
                    Vector2 glowOrigin = { glowDest.width / 2.0f, glowDest.height / 2.0f };
                    DrawTexturePro(winTex, source, glowDest, glowOrigin, rotation, Fade(baseColor, 0.15f - i * 0.03f));
                }

                DrawTexturePro(winTex, source, dest, origin, rotation, WHITE);

                const char* winText = TextFormat("PLAYER %c WINS!", game.winner == 1 ? 'X' : 'O');
                float fontSize = 70.0f + 5.0f * sinf(t * 12.0f);
                int textW = MeasureText(winText, (int)fontSize);

                Color textPulse = ColorFromHSV(game.winner == 1 ? 0.0f : 220.0f, 0.6f, 0.8f + 0.2f * sinf(t * 10.0f));
                DrawText(winText, screenW / 2 - textW / 2, 450 + (int)(sinf(t * 5.0f) * 15.0f), (int)fontSize, textPulse);

            }
            else {
                DrawTextCentered((Rectangle) { screenW / 2 - 200, 200, 400, 100 }, "STALEMATE.", 60, GRAY);
            }

            Rectangle btnPlayAgainGO = { screenW / 2 - 150, screenH / 2 + 100, 300, 50 };
            Rectangle btnLoadGO = { screenW / 2 - 150, screenH / 2 + 170, 300, 50 };
            Rectangle btnMenuGO = { screenW / 2 - 150, screenH / 2 + 240, 300, 50 };

            DrawRectangleLinesEx(btnPlayAgainGO, 2, ui.gameOverIndex == 0 ? highlightColor : uiColor);
            DrawTextCentered(btnPlayAgainGO, "PLAY AGAIN", 20, ui.gameOverIndex == 0 ? highlightColor : textColor);

            DrawRectangleLinesEx(btnLoadGO, 2, ui.gameOverIndex == 1 ? highlightColor : uiColor);
            DrawTextCentered(btnLoadGO, "LOAD GAME", 20, ui.gameOverIndex == 1 ? highlightColor : textColor);

            DrawRectangleLinesEx(btnMenuGO, 2, ui.gameOverIndex == 2 ? highlightColor : uiColor);
            DrawTextCentered(btnMenuGO, "MAIN MENU", 20, ui.gameOverIndex == 2 ? highlightColor : textColor);
        }
    }
    EndDrawing();
}