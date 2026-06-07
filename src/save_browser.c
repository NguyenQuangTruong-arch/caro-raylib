#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#define MAKE_DIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define MAKE_DIR(path) mkdir(path, 0777)
#endif

#include <sys/stat.h>

#include "raylib.h"
#include "save_browser.h"
#include "game.h"
#include "config.h"

#define SAVE_FOLDER "saves"
#define SAVE_EXTENSION ".sav"

#define MAX_SAVE_NAME 64
#define MAX_SAVE_PATH 260

#define LIST_X 80
#define LIST_Y 170
#define LIST_W 560
#define ROW_H 68
#define VISIBLE_ROWS 6

#define ACTION_X 690
#define ACTION_Y 155
#define BUTTON_W 300
#define BUTTON_H 44

static char** saveNames = NULL;
static char** saveMetas = NULL;
static int saveCount = 0;

static int selectedIndex = 0;
static int scrollOffset = 0;

static int browserMode = BROWSER_LOAD;
static AppScreen browserReturnScreen = SCREEN_MENU;

static char saveNameInput[MAX_SAVE_NAME] = "save_1";
static char renameInput[MAX_SAVE_NAME] = "";
static UITextInputState saveNameInputState;
static UITextInputState renameInputState;

static int renameMode = 0;
static int deleteConfirmIndex = -1;

static int overwriteConfirm = 0;
static char pendingOverwriteFileName[MAX_SAVE_NAME + 8] = "";

static char browserMessage[160] = "";
static float browserMessageTimer = 0.0f;

static char* DuplicateText(const char* text)
{
    int len;
    char* result;

    if (text == NULL) return NULL;

    len = (int)strlen(text);
    result = (char*)malloc((len + 1) * sizeof(char));

    if (result == NULL) return NULL;

    strcpy(result, text);
    return result;
}

static void FormatFileModifiedTime(const char* path, char* outTime, int outSize)
{
    struct stat fileInfo;
    struct tm* localTime;

    if (stat(path, &fileInfo) != 0)
    {
        strncpy(outTime, "Không rõ", outSize - 1);
        outTime[outSize - 1] = '\0';
        return;
    }

    localTime = localtime(&fileInfo.st_mtime);

    if (localTime == NULL)
    {
        strncpy(outTime, "Không rõ", outSize - 1);
        outTime[outSize - 1] = '\0';
        return;
    }

    strftime(outTime, outSize, "%Y-%m-%d %H:%M", localTime);
}

static void BuildSaveMetadata(const char* fileName, char* outMeta, int outSize)
{
    char path[MAX_SAVE_PATH];
    char header[64];
    char saveTime[32];
    char savedName[64];
    int saveVersion = 1;
    int boardSize;
    int currentPlayerValue;
    int selectedRowValue;
    int selectedColValue;
    int winnerValue;
    int savedGameMode = GAME_MODE_PVP;
    int savedBotDifficulty;
    int savedHumanPlayer;
    int moveX;
    int moveO;
    int scoreXValue = 0;
    int scoreOValue = 0;
    FILE* file;

    snprintf(path, sizeof(path), "%s/%s", SAVE_FOLDER, fileName);
    FormatFileModifiedTime(path, saveTime, sizeof(saveTime));

    file = fopen(path, "r");

    if (file == NULL)
    {
        snprintf(outMeta, outSize, "Không đọc được thông tin");
        return;
    }

    if (fscanf(file, "%63s", header) != 1)
    {
        fclose(file);
        snprintf(outMeta, outSize, "Bản lưu bị lỗi");
        return;
    }

    if (strcmp(header, "CARO_SAVE_FILE_V4") == 0)
    {
        saveVersion = 4;
    }
    else if (strcmp(header, "CARO_SAVE_FILE_V3") == 0)
    {
        saveVersion = 3;
    }
    else if (strcmp(header, "CARO_SAVE_FILE_V2") == 0)
    {
        saveVersion = 2;
    }
    else if (strcmp(header, "CARO_SAVE_FILE") == 0)
    {
        saveVersion = 1;
    }
    else
    {
        fclose(file);
        snprintf(outMeta, outSize, "Bản lưu không hợp lệ");
        return;
    }

    if (fscanf(file, "%d", &boardSize) != 1 ||
        fscanf(file, "%d %d %d %d",
               &currentPlayerValue,
               &selectedRowValue,
               &selectedColValue,
               &winnerValue) != 4)
    {
        fclose(file);
        snprintf(outMeta, outSize, "Bản lưu thiếu thông tin");
        return;
    }

    if (saveVersion >= 2)
    {
        if (fscanf(file, "%d %d %d",
                   &savedGameMode,
                   &savedBotDifficulty,
                   &savedHumanPlayer) != 3)
        {
            fclose(file);
            snprintf(outMeta, outSize, "Bản lưu thiếu chế độ");
            return;
        }
    }

    if (saveVersion >= 3)
    {
        if (fscanf(file, " %63[^\n]", savedName) != 1 ||
            fscanf(file, " %63[^\n]", savedName) != 1 ||
            fscanf(file, " %31[^\n]", saveTime) != 1)
        {
            fclose(file);
            snprintf(outMeta, outSize, "Bản lưu thiếu metadata");
            return;
        }
    }

    if (fscanf(file, "%d %d", &moveX, &moveO) != 2 ||
        fscanf(file, "%d %d", &scoreXValue, &scoreOValue) != 2)
    {
        fclose(file);
        snprintf(outMeta, outSize, "Bản lưu thiếu tỉ số");
        return;
    }

    fclose(file);

    snprintf(
        outMeta,
        outSize,
        "Tỉ số X/O: %d - %d | %s | %s",
        scoreXValue,
        scoreOValue,
        saveTime,
        savedGameMode == GAME_MODE_BOT ? "Chơi với máy" : "Chơi với người"
    );
}

static void SetBrowserMessage(const char* message)
{
    strncpy(browserMessage, message, sizeof(browserMessage) - 1);
    browserMessage[sizeof(browserMessage) - 1] = '\0';
    browserMessageTimer = 2.5f;
}

static int EndsWithIgnoreCase(const char* text, const char* suffix)
{
    int textLen;
    int suffixLen;
    int i;

    if (text == NULL || suffix == NULL) return 0;

    textLen = (int)strlen(text);
    suffixLen = (int)strlen(suffix);

    if (suffixLen > textLen) return 0;

    for (i = 0; i < suffixLen; i++)
    {
        char a = (char)tolower((unsigned char)text[textLen - suffixLen + i]);
        char b = (char)tolower((unsigned char)suffix[i]);

        if (a != b) return 0;
    }

    return 1;
}

static void FreeSaveList(void)
{
    int i;

    for (i = 0; i < saveCount; i++)
    {
        free(saveNames[i]);
        free(saveMetas[i]);
    }

    free(saveNames);
    free(saveMetas);
    saveNames = NULL;
    saveMetas = NULL;
    saveCount = 0;
}

static void AddSaveName(const char* fileName)
{
    char** newList;
    char** newMetaList;
    char metadata[192];

    newList = (char**)realloc(saveNames, (saveCount + 1) * sizeof(char*));
    if (newList == NULL) return;

    newMetaList = (char**)realloc(saveMetas, (saveCount + 1) * sizeof(char*));
    if (newMetaList == NULL)
    {
        saveNames = newList;
        return;
    }

    BuildSaveMetadata(fileName, metadata, sizeof(metadata));

    saveNames = newList;
    saveMetas = newMetaList;
    saveNames[saveCount] = DuplicateText(fileName);
    saveMetas[saveCount] = DuplicateText(metadata);

    if (saveNames[saveCount] != NULL && saveMetas[saveCount] != NULL)
    {
        saveCount++;
    }
    else
    {
        free(saveNames[saveCount]);
        free(saveMetas[saveCount]);
    }
}

static void SortSaveNames(void)
{
    int i, j;

    for (i = 0; i < saveCount - 1; i++)
    {
        for (j = i + 1; j < saveCount; j++)
        {
            if (strcmp(saveNames[i], saveNames[j]) > 0)
            {
                char* temp = saveNames[i];
                char* tempMeta = saveMetas[i];
                saveNames[i] = saveNames[j];
                saveMetas[i] = saveMetas[j];
                saveNames[j] = temp;
                saveMetas[j] = tempMeta;
            }
        }
    }
}

static void EnsureSaveFolder(void)
{
    MAKE_DIR(SAVE_FOLDER);
}

static void ClampScroll(void)
{
    int maxScroll = saveCount - VISIBLE_ROWS;

    if (maxScroll < 0) maxScroll = 0;

    if (scrollOffset < 0) scrollOffset = 0;
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;

    if (selectedIndex < 0) selectedIndex = 0;
    if (selectedIndex >= saveCount) selectedIndex = saveCount - 1;

    if (selectedIndex < 0) selectedIndex = 0;

    if (selectedIndex < scrollOffset)
    {
        scrollOffset = selectedIndex;
    }

    if (selectedIndex >= scrollOffset + VISIBLE_ROWS)
    {
        scrollOffset = selectedIndex - VISIBLE_ROWS + 1;
    }

    if (scrollOffset < 0) scrollOffset = 0;
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;
}

static void RefreshSaveFiles(void)
{
    FilePathList files;
    unsigned int i;

    EnsureSaveFolder();
    FreeSaveList();

    files = LoadDirectoryFilesEx(SAVE_FOLDER, SAVE_EXTENSION, false);

    for (i = 0; i < files.count; i++)
    {
        const char* fileName = GetFileName(files.paths[i]);

        if (EndsWithIgnoreCase(fileName, SAVE_EXTENSION))
        {
            AddSaveName(fileName);
        }
    }

    UnloadDirectoryFiles(files);

    SortSaveNames();
    ClampScroll();
}

static void BuildSavePath(const char* fileName, char* outPath, int outSize)
{
    snprintf(outPath, outSize, "%s/%s", SAVE_FOLDER, fileName);
}

static void RemoveSaveExtension(const char* fileName, char* outName, int outSize)
{
    int len;

    strncpy(outName, fileName, outSize - 1);
    outName[outSize - 1] = '\0';

    len = (int)strlen(outName);

    if (len >= 4 && EndsWithIgnoreCase(outName, SAVE_EXTENSION))
    {
        outName[len - 4] = '\0';
    }
}

static int IsInvalidFileNameChar(char c)
{
    return c == '\\' || c == '/' || c == ':' || c == '*' ||
           c == '?' || c == '"' || c == '<' || c == '>' || c == '|';
}

static void MakeSafeSaveFileName(const char* rawName, char* outFileName, int outSize)
{
    int i;
    int len = 0;

    if (rawName == NULL || rawName[0] == '\0')
    {
        strncpy(outFileName, "save.sav", outSize - 1);
        outFileName[outSize - 1] = '\0';
        return;
    }

    for (i = 0; rawName[i] != '\0' && len < outSize - 1; i++)
    {
        char c = rawName[i];

        if (IsInvalidFileNameChar(c))
        {
            c = '_';
        }

        if ((unsigned char)c < 32)
        {
            continue;
        }

        outFileName[len++] = c;
    }

    outFileName[len] = '\0';

    while (len > 0 && outFileName[len - 1] == ' ')
    {
        outFileName[len - 1] = '\0';
        len--;
    }

    while (outFileName[0] == ' ')
    {
        memmove(outFileName, outFileName + 1, strlen(outFileName));
        len--;
    }

    if (outFileName[0] == '\0')
    {
        strncpy(outFileName, "save.sav", outSize - 1);
        outFileName[outSize - 1] = '\0';
        return;
    }

    if (!EndsWithIgnoreCase(outFileName, SAVE_EXTENSION))
    {
        if ((int)strlen(outFileName) + 4 < outSize)
        {
            strcat(outFileName, SAVE_EXTENSION);
        }
    }
}

static int FindSaveIndexByName(const char* fileName)
{
    int i;

    for (i = 0; i < saveCount; i++)
    {
        if (strcmp(saveNames[i], fileName) == 0)
        {
            return i;
        }
    }

    return -1;
}

static const char* GetSelectedSaveName(void)
{
    if (saveCount <= 0) return NULL;
    if (selectedIndex < 0 || selectedIndex >= saveCount) return NULL;

    return saveNames[selectedIndex];
}

static int IsButtonClicked(Rectangle rect)
{
    Vector2 mouse = GetVirtualMousePosition();

    return CheckCollisionPointRec(mouse, rect) &&
           IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static Rectangle GetBrowserNameInputRect(void)
{
    Rectangle rect = { ACTION_X, ACTION_Y + 45, BUTTON_W, 44 };
    return rect;
}

static Rectangle GetBrowserRenameInputRect(void)
{
    float y = browserMode == BROWSER_LOAD ? LIST_Y + 95.0f : ACTION_Y + 195.0f;
    Rectangle rect = { ACTION_X, y, BUTTON_W, 44 };
    return rect;
}

static Rectangle GetBrowserPrimaryButtonRect(void)
{
    float y = browserMode == BROWSER_LOAD ? LIST_Y : ACTION_Y + 105.0f;
    Rectangle rect = { ACTION_X, y, BUTTON_W, BUTTON_H };
    return rect;
}

static Rectangle GetBrowserRenameButtonRect(void)
{
    float y;

    if (browserMode == BROWSER_LOAD)
    {
        y = renameMode ? LIST_Y + 155.0f : LIST_Y + 100.0f;
    }
    else
    {
        y = ACTION_Y + 260.0f;
    }

    return (Rectangle){ ACTION_X, y, BUTTON_W, BUTTON_H };
}

static Rectangle GetBrowserCancelRenameButtonRect(void)
{
    float y = browserMode == BROWSER_LOAD ? LIST_Y + 210.0f : ACTION_Y + 315.0f;
    Rectangle rect = { ACTION_X, y, BUTTON_W, BUTTON_H };
    return rect;
}

static Rectangle GetBrowserDeleteButtonRect(void)
{
    float y = browserMode == BROWSER_LOAD ? LIST_Y + 200.0f : ACTION_Y + 370.0f;
    Rectangle rect = { ACTION_X, y, BUTTON_W, BUTTON_H };
    return rect;
}

static Rectangle GetBrowserBackButtonRect(void)
{
    float y = browserMode == BROWSER_LOAD ? LIST_Y + 290.0f : ACTION_Y + 450.0f;
    Rectangle rect = { ACTION_X, y, BUTTON_W, BUTTON_H };
    return rect;
}

static int GetBrowserRenameLabelY(void)
{
    return browserMode == BROWSER_LOAD ? LIST_Y + 65 : ACTION_Y + 165;
}

static void SelectSaveIndex(int index)
{
    if (index < 0 || index >= saveCount) return;

    if (selectedIndex != index)
    {
        renameMode = 0;
        deleteConfirmIndex = -1;
        overwriteConfirm = 0;
        pendingOverwriteFileName[0] = '\0';
    }

    selectedIndex = index;
    ClampScroll();
}

void OpenSaveBrowser(AppScreen* currentScreen, int mode, AppScreen returnScreen)
{
    browserMode = mode;
    browserReturnScreen = returnScreen;

    selectedIndex = 0;
    scrollOffset = 0;
    renameMode = 0;
    deleteConfirmIndex = -1;
    overwriteConfirm = 0;
    pendingOverwriteFileName[0] = '\0';
    browserMessage[0] = '\0';
    browserMessageTimer = 0.0f;

    RefreshSaveFiles();

    snprintf(saveNameInput, sizeof(saveNameInput), "save_%d", saveCount + 1);
    SetUITextInputCursorToEnd(&saveNameInputState, saveNameInput);
    InitUITextInputState(&renameInputState);

    if (mode == BROWSER_SAVE)
    {
        *currentScreen = SCREEN_SAVE;
    }
    else
    {
        *currentScreen = SCREEN_LOAD;
    }
}

static void UpdateSaveListInput(void)
{
    Vector2 mouse = GetVirtualMousePosition();
    Rectangle listRect = { LIST_X, LIST_Y, LIST_W, ROW_H * VISIBLE_ROWS };
    float wheel;
    int clickedRow;
    int index;

    if (CheckCollisionPointRec(mouse, listRect))
    {
        wheel = GetMouseWheelMove();

        if (wheel > 0)
        {
            scrollOffset--;
        }
        else if (wheel < 0)
        {
            scrollOffset++;
        }

        ClampScroll();

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            clickedRow = (int)((mouse.y - LIST_Y) / ROW_H);
            index = scrollOffset + clickedRow;

            if (index >= 0 && index < saveCount)
            {
                SelectSaveIndex(index);
            }
        }
    }

}

static void SaveNewFile(void)
{
    char fileName[MAX_SAVE_NAME + 8];
    char path[MAX_SAVE_PATH];
    int newIndex;
    int existingIndex;

    EnsureSaveFolder();

    MakeSafeSaveFileName(saveNameInput, fileName, sizeof(fileName));
    BuildSavePath(fileName, path, sizeof(path));

    if (FileExists(path))
    {
        existingIndex = FindSaveIndexByName(fileName);

        if (existingIndex >= 0)
        {
            SelectSaveIndex(existingIndex);
        }

        if (!overwriteConfirm || strcmp(pendingOverwriteFileName, fileName) != 0)
        {
            overwriteConfirm = 1;
            strcpy(pendingOverwriteFileName, fileName);
            SetBrowserMessage("Tên bản lưu đã tồn tại. Bấm Lưu lần nữa để ghi đè.");
            return;
        }
    }

    if (SaveGameToFile(path))
    {
        RefreshSaveFiles();

        newIndex = FindSaveIndexByName(fileName);
        if (newIndex >= 0)
        {
            SelectSaveIndex(newIndex);
        }

        overwriteConfirm = 0;
        pendingOverwriteFileName[0] = '\0';

        SetBrowserMessage("Đã lưu bản lưu.");
    }
    else
    {
        SetBrowserMessage("Không thể lưu bản lưu.");
    }
}

static void LoadSelectedFile(AppScreen* currentScreen)
{
    const char* selectedName;
    char path[MAX_SAVE_PATH];

    selectedName = GetSelectedSaveName();

    if (selectedName == NULL)
    {
        SetBrowserMessage("Chưa chọn bản lưu để tải.");
        return;
    }

    BuildSavePath(selectedName, path, sizeof(path));

    if (LoadGameFromFile(path))
    {
        SetBrowserMessage("Đã tải bản lưu.");
        *currentScreen = SCREEN_GAME;
    }
    else
    {
        SetBrowserMessage("Không thể tải bản lưu.");
    }
}

static void StartOrConfirmRename(void)
{
    const char* selectedName;
    char oldPath[MAX_SAVE_PATH];
    char newPath[MAX_SAVE_PATH];
    char newFileName[MAX_SAVE_NAME + 8];

    selectedName = GetSelectedSaveName();

    if (selectedName == NULL)
    {
        SetBrowserMessage("Chưa chọn bản lưu để đổi tên.");
        return;
    }

    if (!renameMode)
    {
        RemoveSaveExtension(selectedName, renameInput, sizeof(renameInput));
        SetUITextInputCursorToEnd(&renameInputState, renameInput);
        renameMode = 1;
        deleteConfirmIndex = -1;
        SetBrowserMessage("Nhập tên mới, rồi bấm Đổi tên lần nữa.");
        return;
    }

    MakeSafeSaveFileName(renameInput, newFileName, sizeof(newFileName));

    if (strcmp(selectedName, newFileName) == 0)
    {
        renameMode = 0;
        SetBrowserMessage("Tên bản lưu không thay đổi.");
        return;
    }

    BuildSavePath(selectedName, oldPath, sizeof(oldPath));
    BuildSavePath(newFileName, newPath, sizeof(newPath));

    if (FileExists(newPath))
    {
        SetBrowserMessage("Tên mới đã tồn tại.");
        return;
    }

    if (rename(oldPath, newPath) == 0)
    {
        int newIndex;

        RefreshSaveFiles();

        newIndex = FindSaveIndexByName(newFileName);
        if (newIndex >= 0)
        {
            SelectSaveIndex(newIndex);
        }

        renameMode = 0;
        SetBrowserMessage("Đã đổi tên bản lưu.");
    }
    else
    {
        SetBrowserMessage("Không thể đổi tên bản lưu.");
    }
}

static void DeleteSelectedFile(void)
{
    const char* selectedName;
    char path[MAX_SAVE_PATH];

    selectedName = GetSelectedSaveName();

    if (selectedName == NULL)
    {
        SetBrowserMessage("Chưa chọn bản lưu để xóa.");
        return;
    }

    if (deleteConfirmIndex != selectedIndex)
    {
        deleteConfirmIndex = selectedIndex;
        renameMode = 0;
        SetBrowserMessage("Bấm Xóa lần nữa để xác nhận.");
        return;
    }

    BuildSavePath(selectedName, path, sizeof(path));

    if (remove(path) == 0)
    {
        RefreshSaveFiles();

        if (selectedIndex >= saveCount)
        {
            selectedIndex = saveCount - 1;
        }

        if (selectedIndex < 0)
        {
            selectedIndex = 0;
        }

        ClampScroll();

        deleteConfirmIndex = -1;
        SetBrowserMessage("Đã xóa bản lưu.");
    }
    else
    {
        SetBrowserMessage("Không thể xóa bản lưu.");
    }
}

void UpdateSaveBrowser(AppScreen* currentScreen)
{
    Rectangle nameInputRect = GetBrowserNameInputRect();
    Rectangle renameInputRect = GetBrowserRenameInputRect();

    Rectangle saveNewButton = GetBrowserPrimaryButtonRect();
    Rectangle loadButton = GetBrowserPrimaryButtonRect();
    Rectangle renameButton = GetBrowserRenameButtonRect();
    Rectangle cancelRenameButton = GetBrowserCancelRenameButtonRect();
    Rectangle deleteButton = GetBrowserDeleteButtonRect();
    Rectangle backButton = GetBrowserBackButtonRect();

    char oldSaveName[MAX_SAVE_NAME];

    if (browserMessageTimer > 0.0f)
    {
        browserMessageTimer -= GetFrameTime();

        if (browserMessageTimer <= 0.0f)
        {
            browserMessage[0] = '\0';
        }
    }

    UpdateSaveListInput();

    if (renameMode)
    {
        UpdateUITextInput(&renameInputState,
                          renameInput,
                          sizeof(renameInput),
                          renameInputRect,
                          1,
                          18);
    }
    else if (browserMode == BROWSER_SAVE)
    {
        strcpy(oldSaveName, saveNameInput);

        UpdateUITextInput(&saveNameInputState,
                          saveNameInput,
                          sizeof(saveNameInput),
                          nameInputRect,
                          1,
                          18);

        if (strcmp(oldSaveName, saveNameInput) != 0)
        {
            overwriteConfirm = 0;
            pendingOverwriteFileName[0] = '\0';
        }
    }

    if (browserMode == BROWSER_SAVE)
    {
        if (IsButtonClicked(saveNewButton))
        {
            SaveNewFile();
        }
    }
    else
    {
        if (IsButtonClicked(loadButton))
        {
            LoadSelectedFile(currentScreen);
            return;
        }

    }

    if (IsButtonClicked(renameButton))
    {
        StartOrConfirmRename();
    }

    if (renameMode && IsButtonClicked(cancelRenameButton))
    {
        renameMode = 0;
        SetBrowserMessage("Đã hủy đổi tên.");
    }

    if (!renameMode && IsButtonClicked(deleteButton))
    {
        DeleteSelectedFile();
    }

    if (IsButtonClicked(backButton))
    {
        renameMode = 0;
        deleteConfirmIndex = -1;
        *currentScreen = browserReturnScreen;
    }
}

static void DrawBrowserButton(Rectangle rect, const char* text, int enabled)
{
    Vector2 mouse = GetVirtualMousePosition();
    int hover = enabled && CheckCollisionPointRec(mouse, rect);

    Color bg = enabled ? (hover ? SKYBLUE : LIGHTGRAY) : GRAY;
    Color border = enabled ? (hover ? BLUE : DARKGRAY) : DARKGRAY;
    Color textColor = enabled ? DARKBLUE : DARKGRAY;

    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, 2, border);

    DrawUIFont(
        UI_FONT_BUTTON,
        text,
        (int)(rect.x + rect.width / 2 - MeasureUIFont(UI_FONT_BUTTON, text, 20) / 2),
        (int)(rect.y + rect.height / 2 - 10),
        20,
        textColor
    );
}

static void DrawSaveList(void)
{
    int i;
    int index;
    int rowY;
    Rectangle listBorder = { LIST_X, LIST_Y, LIST_W, ROW_H * VISIBLE_ROWS };

    DrawRectangleRec(listBorder, (Color){ 240, 236, 220, 255 });
    DrawRectangleLinesEx(listBorder, 3, DARKBROWN);

    if (saveCount == 0)
    {
        DrawText("Chưa có bản lưu nào.", LIST_X + 25, LIST_Y + 30, 24, DARKGRAY);
        return;
    }

    for (i = 0; i < VISIBLE_ROWS; i++)
    {
        index = scrollOffset + i;

        if (index >= saveCount) break;

        rowY = LIST_Y + i * ROW_H;

        if (index == selectedIndex)
        {
            DrawRectangle(LIST_X + 4, rowY + 4, LIST_W - 8, ROW_H - 8, (Color){ 180, 220, 255, 255 });
        }
        else
        {
            DrawRectangle(LIST_X + 4, rowY + 4, LIST_W - 8, ROW_H - 8, (Color){ 250, 245, 225, 255 });
        }
        {
            char displayName[MAX_SAVE_NAME];

            RemoveSaveExtension(saveNames[index], displayName, sizeof(displayName));

            DrawText(TextFormat("%d. %s", index + 1, displayName),
                     LIST_X + 18, rowY + 10, 20, DARKGRAY);

            if (saveMetas[index] != NULL)
            {
                DrawText(saveMetas[index],
                         LIST_X + 36,
                         rowY + 38,
                         15,
                         DARKGRAY);
            }
        }
    }

    if (saveCount > VISIBLE_ROWS)
    {
        int trackX = LIST_X + LIST_W + 12;
        int trackY = LIST_Y;
        int trackH = ROW_H * VISIBLE_ROWS;
        int thumbH = (VISIBLE_ROWS * trackH) / saveCount;
        int maxScroll = saveCount - VISIBLE_ROWS;
        int thumbY;

        if (thumbH < 35) thumbH = 35;

        thumbY = trackY;

        if (maxScroll > 0)
        {
            thumbY = trackY + (scrollOffset * (trackH - thumbH)) / maxScroll;
        }

        DrawRectangle(trackX, trackY, 10, trackH, LIGHTGRAY);
        DrawRectangle(trackX, thumbY, 10, thumbH, DARKGRAY);
    }
}

void DrawSaveBrowser(void)
{
    Rectangle nameInputRect = GetBrowserNameInputRect();
    Rectangle renameInputRect = GetBrowserRenameInputRect();

    Rectangle saveNewButton = GetBrowserPrimaryButtonRect();
    Rectangle loadButton = GetBrowserPrimaryButtonRect();
    Rectangle renameButton = GetBrowserRenameButtonRect();
    Rectangle cancelRenameButton = GetBrowserCancelRenameButtonRect();
    Rectangle deleteButton = GetBrowserDeleteButtonRect();
    Rectangle backButton = GetBrowserBackButtonRect();

    const char* title = browserMode == BROWSER_SAVE ? "LƯU VÁN" : "TẢI VÁN";

    DrawUIFont(UI_FONT_HEADING, title, 80, 45, 46, DARKBLUE);
    DrawText(TextFormat("Tổng số bản lưu: %d", saveCount), 80, 105, 22, DARKGRAY);

    DrawUIFont(UI_FONT_HEADING, "Danh sách bản lưu:", LIST_X, LIST_Y - 30, 22, DARKBROWN);
    DrawSaveList();

    if (browserMode == BROWSER_SAVE)
    {
        DrawText("Tên bản lưu mới:", ACTION_X, ACTION_Y + 15, 18, DARKBLUE);
        DrawUITextInputBox(nameInputRect,
                           &saveNameInputState,
                           saveNameInput,
                           !renameMode,
                           18);

    if (overwriteConfirm)
    {
        DrawBrowserButton(saveNewButton, "Xác nhận ghi đè", 1);
    }
    else
    {
        DrawBrowserButton(saveNewButton, "Lưu bản lưu", 1);
    }
    }
    else
    {
        DrawBrowserButton(loadButton, "Tải bản lưu", saveCount > 0);
    }

    if (renameMode)
    {
        DrawText("Tên mới:", ACTION_X, GetBrowserRenameLabelY(), 18, DARKBLUE);
        DrawUITextInputBox(renameInputRect,
                           &renameInputState,
                           renameInput,
                           1,
                           18);
        DrawBrowserButton(renameButton, "Xác nhận đổi tên", saveCount > 0);
        DrawBrowserButton(cancelRenameButton, "Hủy đổi tên", 1);
    }
    else
    {
        DrawBrowserButton(renameButton, "Đổi tên", saveCount > 0);
    }

    if (!renameMode)
    {
        if (deleteConfirmIndex == selectedIndex && saveCount > 0)
        {
            DrawBrowserButton(deleteButton, "Xác nhận xóa", 1);
        }
        else
        {
            DrawBrowserButton(deleteButton, "Xóa", saveCount > 0);
        }
    }

    DrawBrowserButton(backButton, "Quay lại", 1);

    if (browserMessage[0] != '\0')
    {
        DrawRectangle(70, 650, 860, 40, (Color){ 255, 245, 200, 255 });
        DrawRectangleLines(70, 650, 860, 40, DARKBROWN);
        DrawText(browserMessage, 85, 660, 20, MAROON);
    }

}
