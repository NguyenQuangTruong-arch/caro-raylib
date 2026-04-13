#include "controller.h"
#include "model.h"
#include "view.h"
#include <stdlib.h> 
#include <ctype.h>
#include <string.h>

static int loadingFrames = 0;
static const int MAX_LOADING_FRAMES = 90;
static float backspaceTimer = 0.0f; // Biến hỗ trợ giữ nút xóa

static void HandleMenu(Vector2 mousePos, bool mouseClicked, bool enterPressed);
static void HandleModeSelect(Vector2 mousePos, bool mouseClicked, bool enterPressed); 
static void HandleSettings(Vector2 mousePos, bool mouseClicked, bool enterPressed);
static void HandleSaveLoad(Vector2 mousePos, bool mouseClicked, bool enterPressed);
static void HandlePlaying(Vector2 mousePos, bool mouseClicked, bool enterPressed);
static void HandlePaused(Vector2 mousePos, bool mouseClicked, bool enterPressed);
static void HandleGameOver(Vector2 mousePos, bool mouseClicked, bool enterPressed);

int Controller_GetLoadingProgress(void) {
    return (loadingFrames * 100) / MAX_LOADING_FRAMES;
}

static bool StringContainsCaseInsensitive(const char* text, const char* search) {
    if (search[0] == '\0') return true;
    for (int i = 0; text[i] != '\0'; i++) {
        int j = 0;
        while (search[j] != '\0' && text[i + j] != '\0' && tolower((unsigned char)text[i + j]) == tolower((unsigned char)search[j])) {
            j++;
        }
        if (search[j] == '\0') return true;
    }
    return false;
}

bool Controller_Update(void) {
    if (WindowShouldClose()) return true;

    Vector2 mousePos = GetMousePosition();
    bool mouseClicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool enterPressed = IsKeyPressed(KEY_ENTER);

    if (game.state == STATE_LOADING) {
        loadingFrames++;
        if (loadingFrames == 2) View_LoadAssets();
        if (loadingFrames >= MAX_LOADING_FRAMES) game.state = STATE_MENU;
        return false;
    }

    if (IsKeyPressed(KEY_ESCAPE) && game.state != STATE_SAVE_MENU && game.state != STATE_LOAD_MENU) {
        if (game.state == STATE_PLAYING) {
            ui.pauseMenuIndex = 0;
            game.state = STATE_PAUSED;
        }
        else if (game.state == STATE_PAUSED) game.state = STATE_PLAYING;
    }

    static GameState lastState = STATE_LOADING;
    if (game.state != lastState) {
        if (game.state == STATE_SAVE_MENU || game.state == STATE_LOAD_MENU) {
            ui.inputText[0] = '\0';
            ui.letterCount = 0;
            ui.slotMenuIndex = 0;
            ui.saveListSelectedIndex = 0;
            ui.isInputMode = false; // Reset Input Mode
            Model_RefreshSaveFiles();
        }
        lastState = game.state;
    }

    switch (game.state) {
    case STATE_MENU: HandleMenu(mousePos, mouseClicked, enterPressed); break;
    case STATE_MODE_SELECT: HandleModeSelect(mousePos, mouseClicked, enterPressed); break;
    case STATE_SETTINGS: HandleSettings(mousePos, mouseClicked, enterPressed); break;
    case STATE_SAVE_MENU:
    case STATE_LOAD_MENU: HandleSaveLoad(mousePos, mouseClicked, enterPressed); break;
    case STATE_PLAYING: HandlePlaying(mousePos, mouseClicked, enterPressed); break;
    case STATE_PAUSED: HandlePaused(mousePos, mouseClicked, enterPressed); break;
    case STATE_GAME_OVER: HandleGameOver(mousePos, mouseClicked, enterPressed); break;
    default: break;
    }

    return false;
}

// Xử lý giữ nút xóa liên tục
static void HandleTextInput(void) {
    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= 32) && (key <= 125) && (ui.letterCount < 63)) {
            ui.inputText[ui.letterCount] = (char)key;
            ui.inputText[ui.letterCount + 1] = '\0';
            ui.letterCount++;
        }
        key = GetCharPressed();
    }

    if (IsKeyDown(KEY_BACKSPACE)) {
        backspaceTimer += GetFrameTime();
        if (IsKeyPressed(KEY_BACKSPACE) || backspaceTimer > 0.4f) {
            if (ui.letterCount > 0) {
                ui.letterCount--;
                ui.inputText[ui.letterCount] = '\0';
            }
            if (backspaceTimer > 0.4f) backspaceTimer -= 0.05f; // Xóa liên tục tốc độ cao
        }
    }
    else {
        backspaceTimer = 0.0f;
    }
}

static void HandleSaveLoad(Vector2 mousePos, bool mouseClicked, bool enterPressed) {
    int screenW = GetScreenWidth();
    Rectangle txtBox = { screenW / 2 - 200, 200, 400, 60 };
    Rectangle btnBack = { screenW / 2 - 150, 620, 300, 60 };

    // BƯỚC 1: NẾU ĐANG GÕ CHỮ (INPUT MODE) -> NGỪNG ĐIỀU HƯỚNG
    if (ui.isInputMode) {
        HandleTextInput();

        if (IsKeyPressed(KEY_ESCAPE)) {
            ui.isInputMode = false; // Tắt gõ chữ
        }

        // Nếu nhấn Enter trong lúc gõ -> Thực hiện Lưu/Tải ngay lập tức
        if (enterPressed && ui.letterCount > 0) {
            ui.isInputMode = false;
            if (game.state == STATE_SAVE_MENU) {
                Model_SaveGame(ui.inputText);
                game.state = game.previousState;
            }
            else {
                if (Model_SaveExists(ui.inputText)) {
                    Model_LoadGame(ui.inputText);
                    game.state = STATE_PLAYING;
                }
            }
        }
        return; // Dừng tại đây, không cho WASD nhảy danh sách
    }

    // BƯỚC 2: CẬP NHẬT DANH SÁCH LỌC
    ui.displayFilesCount = 0;
    for (int i = 0; i < saveFilesCount; i++) {
        if (ui.letterCount == 0 || StringContainsCaseInsensitive(saveFilesList[i], ui.inputText)) {
            if (ui.displayFilesCount < 5) {
                ui.displayFilesIndices[ui.displayFilesCount] = i;
                ui.displayFilesCount++;
            }
        }
    }

    // BƯỚC 3: ĐIỀU HƯỚNG BẰNG CHUỘT
    if (mouseClicked) {
        if (CheckCollisionPointRec(mousePos, txtBox)) {
            ui.slotMenuIndex = 0;
            ui.isInputMode = true; // Click vào là gõ được luôn
        }
        else if (CheckCollisionPointRec(mousePos, btnBack)) ui.slotMenuIndex = 1;
        else {
            for (int i = 0; i < ui.displayFilesCount; i++) {
                Rectangle fileRect = { screenW / 2 - 200, 320 + i * 50, 400, 40 };
                if (CheckCollisionPointRec(mousePos, fileRect)) {
                    ui.slotMenuIndex = 2;
                    ui.saveListSelectedIndex = i;
                    strncpy(ui.inputText, saveFilesList[ui.displayFilesIndices[i]], 63);
                    ui.letterCount = strlen(ui.inputText);
                    break;
                }
            }
        }
    }

    // BƯỚC 4: ĐIỀU HƯỚNG BẰNG PHÍM W, A, S, D, Lên, Xuống
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_TAB)) {
        if (ui.slotMenuIndex == 0) {
            if (ui.displayFilesCount > 0) { ui.slotMenuIndex = 2; ui.saveListSelectedIndex = 0; }
            else ui.slotMenuIndex = 1;
        }
        else if (ui.slotMenuIndex == 2) {
            if (ui.saveListSelectedIndex < ui.displayFilesCount - 1) ui.saveListSelectedIndex++;
            else ui.slotMenuIndex = 1;
        }
        else ui.slotMenuIndex = 0;
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        if (ui.slotMenuIndex == 1) {
            if (ui.displayFilesCount > 0) { ui.slotMenuIndex = 2; ui.saveListSelectedIndex = ui.displayFilesCount - 1; }
            else ui.slotMenuIndex = 0;
        }
        else if (ui.slotMenuIndex == 2) {
            if (ui.saveListSelectedIndex > 0) ui.saveListSelectedIndex--;
            else ui.slotMenuIndex = 0;
        }
        else ui.slotMenuIndex = 1;
    }

    // BƯỚC 5: TÍNH NĂNG XÓA FILE BẰNG PHÍM DELETE
    if (ui.slotMenuIndex == 2 && ui.displayFilesCount > 0 && IsKeyPressed(KEY_DELETE)) {
        Model_DeleteSave(saveFilesList[ui.displayFilesIndices[ui.saveListSelectedIndex]]);
        if (ui.saveListSelectedIndex >= ui.displayFilesCount - 1 && ui.saveListSelectedIndex > 0) {
            ui.saveListSelectedIndex--; // Chỉnh lại con trỏ nếu xóa file cuối
        }
    }

    // BƯỚC 6: NHẤN ENTER ĐỂ TƯƠNG TÁC HOẶC THOÁT
    if (enterPressed) {
        if (ui.slotMenuIndex == 0) {
            ui.isInputMode = true; // Bật chế độ gõ chữ
        }
        else if (ui.slotMenuIndex == 2 && ui.displayFilesCount > 0) {
            // Nhấn Enter trên File -> Tự điền và Load luôn
            strncpy(ui.inputText, saveFilesList[ui.displayFilesIndices[ui.saveListSelectedIndex]], 63);
            ui.letterCount = strlen(ui.inputText);

            if (game.state == STATE_SAVE_MENU) {
                Model_SaveGame(ui.inputText);
                game.state = game.previousState;
            }
            else {
                if (Model_SaveExists(ui.inputText)) {
                    Model_LoadGame(ui.inputText);
                    game.state = STATE_PLAYING;
                }
            }
        }
        else if (ui.slotMenuIndex == 1) {
            game.state = game.previousState;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE) && !ui.isInputMode) {
        game.state = game.previousState;
    }
}

static void HandleMenu(Vector2 mousePos, bool mouseClicked, bool enterPressed) {
    int screenW = GetScreenWidth();
    Rectangle btnPlay = { screenW / 2 - 150, 350, 300, 60 };
    Rectangle btnLoadMenuMain = { screenW / 2 - 150, 430, 300, 60 };
    Rectangle btnSettings = { screenW / 2 - 150, 510, 300, 60 };
    Rectangle btnQuit = { screenW / 2 - 150, 590, 300, 60 };

    if (CheckCollisionPointRec(mousePos, btnPlay)) ui.mainMenuIndex = 0;
    else if (CheckCollisionPointRec(mousePos, btnLoadMenuMain)) ui.mainMenuIndex = 1;
    else if (CheckCollisionPointRec(mousePos, btnSettings)) ui.mainMenuIndex = 2;
    else if (CheckCollisionPointRec(mousePos, btnQuit)) ui.mainMenuIndex = 3;

    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
        ui.mainMenuIndex = (ui.mainMenuIndex > 0) ? ui.mainMenuIndex - 1 : 3;
        View_PlaySFX();
    }
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
        ui.mainMenuIndex = (ui.mainMenuIndex < 3) ? ui.mainMenuIndex + 1 : 0;
        View_PlaySFX();
    }

    if (enterPressed || (mouseClicked && (CheckCollisionPointRec(mousePos, btnPlay) || CheckCollisionPointRec(mousePos, btnLoadMenuMain) || CheckCollisionPointRec(mousePos, btnSettings) || CheckCollisionPointRec(mousePos, btnQuit)))) {
        if (ui.mainMenuIndex == 0) { ui.modeMenuIndex = 0; game.state = STATE_MODE_SELECT; }
        else if (ui.mainMenuIndex == 1) { game.previousState = STATE_MENU; game.state = STATE_LOAD_MENU; }
        else if (ui.mainMenuIndex == 2) { game.previousState = STATE_MENU; ui.settingsIndex = 0; game.state = STATE_SETTINGS; }
        else if (ui.mainMenuIndex == 3) exit(0);
    }
}

static void HandleModeSelect(Vector2 mousePos, bool mouseClicked, bool enterPressed) {
    int screenW = GetScreenWidth();
    Rectangle btnPvP = { screenW / 2 - 150, 350, 300, 60 };
    Rectangle btnPvE = { screenW / 2 - 150, 430, 300, 60 };
    Rectangle btnBackMode = { screenW / 2 - 150, 510, 300, 60 };

    if (CheckCollisionPointRec(mousePos, btnPvP)) ui.modeMenuIndex = 0;
    else if (CheckCollisionPointRec(mousePos, btnPvE)) ui.modeMenuIndex = 1;
    else if (CheckCollisionPointRec(mousePos, btnBackMode)) ui.modeMenuIndex = 2;

    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
        ui.modeMenuIndex = (ui.modeMenuIndex > 0) ? ui.modeMenuIndex - 1 : 2;
        View_PlaySFX();
    }
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
        ui.modeMenuIndex = (ui.modeMenuIndex < 2) ? ui.modeMenuIndex + 1 : 0;
        View_PlaySFX();
    }

    if (enterPressed || (mouseClicked && (CheckCollisionPointRec(mousePos, btnPvP) || CheckCollisionPointRec(mousePos, btnPvE) || CheckCollisionPointRec(mousePos, btnBackMode)))) {
        if (ui.modeMenuIndex == 0) { game.mode = MODE_PVP; Model_ResetBoard(); game.state = STATE_PLAYING; }
        else if (ui.modeMenuIndex == 1) { game.mode = MODE_PVE; Model_ResetBoard(); game.state = STATE_PLAYING; }
        else if (ui.modeMenuIndex == 2) { game.state = STATE_MENU; }
    }
}

static void HandleSettings(Vector2 mousePos, bool mouseClicked, bool enterPressed) {
    int screenW = GetScreenWidth();
    Rectangle audioToggleBtn = { screenW / 2 - 150, 380, 40, 40 };
    Rectangle volSliderTrack = { screenW / 2 - 150, 480, 300, 30 };
    Rectangle btnSettingsBack = { screenW / 2 - 150, 580, 300, 60 };

    if (CheckCollisionPointRec(mousePos, audioToggleBtn)) ui.settingsIndex = 0;
    else if (CheckCollisionPointRec(mousePos, volSliderTrack)) ui.settingsIndex = 1;
    else if (CheckCollisionPointRec(mousePos, btnSettingsBack)) ui.settingsIndex = 2;

    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
        ui.settingsIndex = (ui.settingsIndex > 0) ? ui.settingsIndex - 1 : 2;
        View_PlaySFX();
    }
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
        ui.settingsIndex = (ui.settingsIndex < 2) ? ui.settingsIndex + 1 : 0;
        View_PlaySFX();
    }

    if (ui.settingsIndex == 1) {
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
            game.volume -= 0.5f * GetFrameTime();
            if (game.volume < 0.0f) game.volume = 0.0f;
        }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
            game.volume += 0.5f * GetFrameTime();
            if (game.volume > 1.0f) game.volume = 1.0f;
        }
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, (Rectangle) { volSliderTrack.x - 20, volSliderTrack.y - 20, volSliderTrack.width + 40, volSliderTrack.height + 40 })) {
            game.volume = (mousePos.x - volSliderTrack.x) / volSliderTrack.width;
            if (game.volume < 0.0f) game.volume = 0.0f;
            if (game.volume > 1.0f) game.volume = 1.0f;
        }
    }

    if (enterPressed || mouseClicked) {
        if (ui.settingsIndex == 0 && (enterPressed || CheckCollisionPointRec(mousePos, audioToggleBtn))) game.soundEnabled = !game.soundEnabled;
        else if (ui.settingsIndex == 2 && (enterPressed || CheckCollisionPointRec(mousePos, btnSettingsBack))) game.state = game.previousState;
    }
}


static void HandlePlaying(Vector2 mousePos, bool mouseClicked, bool enterPressed) {
    game.timeRemaining -= GetFrameTime();

    if (game.timeRemaining <= 0) {
        game.winner = (game.currentPlayer == 1) ? 2 : 1;
        game.state = STATE_GAME_OVER;
        ui.gameOverIndex = 0;
        return;
    }

    if (game.mode == MODE_PVE && game.currentPlayer == 2) {
        Model_MakeAIMove();

        if (Model_CheckWin(game.currentPlayer)) {
            game.winner = game.currentPlayer;
            game.state = STATE_GAME_OVER;
            ui.gameOverIndex = 0;
        }
        else if (Model_CheckStalemate()) {
            game.winner = 0;
            game.state = STATE_GAME_OVER;
            ui.gameOverIndex = 0;
        }
        else {
            game.currentPlayer = 1; 
            game.timeRemaining = TURN_TIME_LIMIT;
        }
        return; 
    }

    if (IsKeyPressed(KEY_W) && game.selectorY > 0) game.selectorY--;
    if (IsKeyPressed(KEY_S) && game.selectorY < BOARD_SIZE - 1) game.selectorY++;
    if (IsKeyPressed(KEY_A) && game.selectorX > 0) game.selectorX--;
    if (IsKeyPressed(KEY_D) && game.selectorX < BOARD_SIZE - 1) game.selectorX++;

    bool piecePlaced = false;
    bool actionPressed = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);

    if (actionPressed) {
        if (game.board[game.selectorY][game.selectorX] == 0) {
            game.board[game.selectorY][game.selectorX] = game.currentPlayer;
            piecePlaced = true;
        }
    }

    if (mouseClicked) {
        int gridX = (mousePos.x - MARGIN_LEFT) / CELL_SIZE;
        int gridY = (mousePos.y - MARGIN_TOP) / CELL_SIZE;

        if (gridX >= 0 && gridX < BOARD_SIZE && gridY >= 0 && gridY < BOARD_SIZE) {
            if (game.board[gridY][gridX] == 0) {
                game.board[gridY][gridX] = game.currentPlayer;
                game.selectorX = gridX;
                game.selectorY = gridY;
                piecePlaced = true;
            }
        }
    }

    if (piecePlaced) {
        if (Model_CheckWin(game.currentPlayer)) {
            game.winner = game.currentPlayer;
            game.state = STATE_GAME_OVER;
            ui.gameOverIndex = 0;
        }
        else if (Model_CheckStalemate()) {
            game.winner = 0;
            game.state = STATE_GAME_OVER;
            ui.gameOverIndex = 0;
        }
        else {
            game.currentPlayer = (game.currentPlayer == 1) ? 2 : 1;
            game.timeRemaining = TURN_TIME_LIMIT;
        }
    }
}

static void HandlePaused(Vector2 mousePos, bool mouseClicked, bool enterPressed) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    Rectangle btnPauseContinue = { screenW / 2 - 150, screenH / 2 - 120, 300, 60 };
    Rectangle btnPauseSave = { screenW / 2 - 150, screenH / 2 - 40, 300, 60 };
    Rectangle btnPauseLoad = { screenW / 2 - 150, screenH / 2 + 40, 300, 60 };
    Rectangle btnPauseQuit = { screenW / 2 - 150, screenH / 2 + 120, 300, 60 };

    if (CheckCollisionPointRec(mousePos, btnPauseContinue)) ui.pauseMenuIndex = 0;
    else if (CheckCollisionPointRec(mousePos, btnPauseSave)) ui.pauseMenuIndex = 1;
    else if (CheckCollisionPointRec(mousePos, btnPauseLoad)) ui.pauseMenuIndex = 2;
    else if (CheckCollisionPointRec(mousePos, btnPauseQuit)) ui.pauseMenuIndex = 3;

    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
        ui.pauseMenuIndex = (ui.pauseMenuIndex > 0) ? ui.pauseMenuIndex - 1 : 3;
        View_PlaySFX();
    }
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
        ui.pauseMenuIndex = (ui.pauseMenuIndex < 3) ? ui.pauseMenuIndex + 1 : 0;
        View_PlaySFX();
    }

    if (enterPressed || mouseClicked) {
        if ((ui.pauseMenuIndex == 0 && enterPressed) || (mouseClicked && CheckCollisionPointRec(mousePos, btnPauseContinue))) {
            game.state = STATE_PLAYING;
        }
        else if ((ui.pauseMenuIndex == 1 && enterPressed) || (mouseClicked && CheckCollisionPointRec(mousePos, btnPauseSave))) {
            game.previousState = STATE_PAUSED; game.state = STATE_SAVE_MENU;
        }
        else if ((ui.pauseMenuIndex == 2 && enterPressed) || (mouseClicked && CheckCollisionPointRec(mousePos, btnPauseLoad))) {
            game.previousState = STATE_PAUSED; game.state = STATE_LOAD_MENU;
        }
        else if ((ui.pauseMenuIndex == 3 && enterPressed) || (mouseClicked && CheckCollisionPointRec(mousePos, btnPauseQuit))) {
            game.state = STATE_MENU; ui.mainMenuIndex = 0;
        }
    }
}

static void HandleGameOver(Vector2 mousePos, bool mouseClicked, bool enterPressed) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    Rectangle btnPlayAgainGO = { screenW / 2 - 150, screenH / 2 + 100, 300, 50 };
    Rectangle btnLoadGO = { screenW / 2 - 150, screenH / 2 + 170, 300, 50 };
    Rectangle btnMenuGO = { screenW / 2 - 150, screenH / 2 + 240, 300, 50 };

    if (CheckCollisionPointRec(mousePos, btnPlayAgainGO)) ui.gameOverIndex = 0;
    else if (CheckCollisionPointRec(mousePos, btnLoadGO)) ui.gameOverIndex = 1;
    else if (CheckCollisionPointRec(mousePos, btnMenuGO)) ui.gameOverIndex = 2;

    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
        ui.gameOverIndex = (ui.gameOverIndex > 0) ? ui.gameOverIndex - 1 : 2;
        View_PlaySFX();
    }
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
        ui.gameOverIndex = (ui.gameOverIndex < 2) ? ui.gameOverIndex + 1 : 0;
        View_PlaySFX();
    }

    if (enterPressed || mouseClicked) {
        if ((ui.gameOverIndex == 0 && enterPressed) || (mouseClicked && CheckCollisionPointRec(mousePos, btnPlayAgainGO))) {
            Model_ResetBoard(); game.state = STATE_PLAYING;
        }
        else if ((ui.gameOverIndex == 1 && enterPressed) || (mouseClicked && CheckCollisionPointRec(mousePos, btnLoadGO))) {
            game.previousState = STATE_GAME_OVER; game.state = STATE_LOAD_MENU;
        }
        else if ((ui.gameOverIndex == 2 && enterPressed) || (mouseClicked && CheckCollisionPointRec(mousePos, btnMenuGO))) {
            game.state = STATE_MENU; ui.mainMenuIndex = 0;
        }
    }
}