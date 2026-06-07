#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"
#include "rlgl.h"

#include "config.h"
#include "menu.h"
#include "game.h"
#include "save_browser.h"

#define TITLE_FONT_PATH "assets/fonts/Nabila.ttf"
#define BUTTON_FONT_PATH "assets/fonts/Cucho.otf"
#define HEADING_FONT_PATH "assets/fonts/iCielPanton-Black.otf"
#define TEXT_FONT_PATH "assets/fonts/AndesRoundedLight.otf"

#define MENU_MUSIC_PATH "assets/sounds/bgm.mp3"
#define MATCH_MUSIC_PATH "assets/sounds/ongoing_match.mp3"
#define CHECK_SOUND_PATH "assets/sounds/check.mp3"
#define WIN_SOUND_PATH "assets/sounds/win.mp3"
#define GAME_OVER_SOUND_PATH "assets/sounds/GameOver.mp3"

typedef enum GameMusicTrack
{
    GAME_MUSIC_NONE = -1,
    GAME_MUSIC_MENU = 0,
    GAME_MUSIC_MATCH,
    GAME_MUSIC_COUNT
} GameMusicTrack;

static Font uiFonts[UI_FONT_COUNT];
static int uiFontLoaded[UI_FONT_COUNT] = { 0 };
static int gameFontsLoaded = 0;

static Music gameMusic[GAME_MUSIC_COUNT];
static int gameMusicLoaded[GAME_MUSIC_COUNT] = { 0 };
static Sound gameSounds[GAME_SOUND_COUNT];
static int gameSoundLoaded[GAME_SOUND_COUNT] = { 0 };
static int gameAudioLoaded = 0;
static GameMusicTrack activeMusicTrack = GAME_MUSIC_NONE;

static float gameMusicVolume = 0.45f;
static float gameSoundVolume = 0.85f;

static int turnTimerEnabled = 0;
static int turnTimeLimitSeconds = 30;
static char turnTimeInput[8] = "30";
static UITextInputState turnTimeInputState;
static int settingsInputInitialized = 0;
static int activeSettingsInput = -1;
static int activeSettingsSlider = -1;

static const char* UI_FONT_CODEPOINT_TEXT =
    " !\"#$%&'()*+,-./0123456789:;<=>?@"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
    "abcdefghijklmnopqrstuvwxyz{|}~"
    "ÀÁÂÃÈÉÊÌÍÒÓÔÕÙÚÝàáâãèéêìíòóôõùúý"
    "ĂăĐđĨĩŨũƠơƯư"
    "ẠạẢảẤấẦầẨẩẪẫẬậẮắẰằẲẳẴẵẶặ"
    "ẸẹẺẻẼẽẾếỀềỂểỄễỆệ"
    "ỈỉỊị"
    "ỌọỎỏỐốỒồỔổỖỗỘộỚớỜờỞởỠỡỢợ"
    "ỤụỦủỨứỪừỬửỮữỰự"
    "ỲỳỴỵỶỷỸỹ";

static Font LoadUIFontFromFile(const char* fileName, int* loaded)
{
    Font font = GetFontDefault();
    int codepointCount = 0;
    int* codepoints = NULL;

    *loaded = 0;

    if (!FileExists(fileName))
    {
        return font;
    }

    codepoints = LoadCodepoints(UI_FONT_CODEPOINT_TEXT, &codepointCount);
    font = LoadFontEx(fileName, 96, codepoints, codepointCount);

    if (codepoints != NULL)
    {
        UnloadCodepoints(codepoints);
    }

    if (font.texture.id == 0)
    {
        return GetFontDefault();
    }

    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    *loaded = 1;

    return font;
}

void LoadGameFonts(void)
{
    if (gameFontsLoaded) return;

    uiFonts[UI_FONT_TITLE] = LoadUIFontFromFile(TITLE_FONT_PATH, &uiFontLoaded[UI_FONT_TITLE]);
    uiFonts[UI_FONT_BUTTON] = LoadUIFontFromFile(BUTTON_FONT_PATH, &uiFontLoaded[UI_FONT_BUTTON]);
    uiFonts[UI_FONT_HEADING] = LoadUIFontFromFile(HEADING_FONT_PATH, &uiFontLoaded[UI_FONT_HEADING]);
    uiFonts[UI_FONT_TEXT] = LoadUIFontFromFile(TEXT_FONT_PATH, &uiFontLoaded[UI_FONT_TEXT]);

    gameFontsLoaded = 1;
}

void UnloadGameFonts(void)
{
    int i;

    if (!gameFontsLoaded) return;

    for (i = 0; i < UI_FONT_COUNT; i++)
    {
        if (uiFontLoaded[i])
        {
            UnloadFont(uiFonts[i]);
            uiFontLoaded[i] = 0;
        }
    }

    gameFontsLoaded = 0;
}

static Font GetUIFont(UIFontRole role)
{
    if (!gameFontsLoaded || role < UI_FONT_TITLE || role >= UI_FONT_COUNT)
    {
        return GetFontDefault();
    }

    return uiFonts[role];
}

void DrawUIFont(UIFontRole role, const char* text, int posX, int posY, int fontSize, Color color)
{
    DrawTextEx(
        GetUIFont(role),
        text,
        (Vector2){ (float)posX, (float)posY },
        (float)fontSize,
        1.0f,
        color
    );
}

int MeasureUIFont(UIFontRole role, const char* text, int fontSize)
{
    Vector2 size = MeasureTextEx(GetUIFont(role), text, (float)fontSize, 1.0f);
    return (int)(size.x + 0.5f);
}

static Music LoadGameMusicStream(const char* fileName, int* loaded, float volume)
{
    Music music = { 0 };

    *loaded = 0;

    if (!FileExists(fileName))
    {
        return music;
    }

    music = LoadMusicStream(fileName);

    if (!IsMusicValid(music))
    {
        return music;
    }

    music.looping = true;
    SetMusicVolume(music, volume);
    *loaded = 1;

    return music;
}

static Sound LoadGameSoundFromFile(const char* fileName, int* loaded, float volume)
{
    Sound sound = { 0 };

    *loaded = 0;

    if (!FileExists(fileName))
    {
        return sound;
    }

    sound = LoadSound(fileName);

    if (!IsSoundValid(sound))
    {
        return sound;
    }

    SetSoundVolume(sound, volume);
    *loaded = 1;

    return sound;
}

void LoadGameAudio(void)
{
    if (gameAudioLoaded) return;
    if (!IsAudioDeviceReady()) return;

    gameMusic[GAME_MUSIC_MENU] = LoadGameMusicStream(MENU_MUSIC_PATH, &gameMusicLoaded[GAME_MUSIC_MENU], gameMusicVolume);
    gameMusic[GAME_MUSIC_MATCH] = LoadGameMusicStream(MATCH_MUSIC_PATH, &gameMusicLoaded[GAME_MUSIC_MATCH], gameMusicVolume);

    gameSounds[GAME_SOUND_CHECK] = LoadGameSoundFromFile(CHECK_SOUND_PATH, &gameSoundLoaded[GAME_SOUND_CHECK], gameSoundVolume);
    gameSounds[GAME_SOUND_WIN] = LoadGameSoundFromFile(WIN_SOUND_PATH, &gameSoundLoaded[GAME_SOUND_WIN], gameSoundVolume);
    gameSounds[GAME_SOUND_GAME_OVER] = LoadGameSoundFromFile(GAME_OVER_SOUND_PATH, &gameSoundLoaded[GAME_SOUND_GAME_OVER], gameSoundVolume);

    gameAudioLoaded = 1;
}

static void StopActiveMusic(void)
{
    if (activeMusicTrack != GAME_MUSIC_NONE &&
        gameMusicLoaded[activeMusicTrack] &&
        IsMusicStreamPlaying(gameMusic[activeMusicTrack]))
    {
        StopMusicStream(gameMusic[activeMusicTrack]);
    }

    activeMusicTrack = GAME_MUSIC_NONE;
}

void UnloadGameAudio(void)
{
    int i;

    if (!gameAudioLoaded) return;

    StopActiveMusic();

    for (i = 0; i < GAME_MUSIC_COUNT; i++)
    {
        if (gameMusicLoaded[i])
        {
            UnloadMusicStream(gameMusic[i]);
            gameMusicLoaded[i] = 0;
        }
    }

    for (i = 0; i < GAME_SOUND_COUNT; i++)
    {
        if (gameSoundLoaded[i])
        {
            UnloadSound(gameSounds[i]);
            gameSoundLoaded[i] = 0;
        }
    }

    gameAudioLoaded = 0;
}

static GameMusicTrack GetMusicTrackForScreen(AppScreen screen)
{
    if (screen == SCREEN_GAME || screen == SCREEN_PAUSE)
    {
        return GAME_MUSIC_MATCH;
    }

    if (screen == SCREEN_MENU ||
        screen == SCREEN_NEW_GAME_MODE ||
        screen == SCREEN_PLAYER_NAMES ||
        screen == SCREEN_BOT_DIFFICULTY ||
        screen == SCREEN_SETTINGS ||
        screen == SCREEN_SAVE ||
        screen == SCREEN_LOAD)
    {
        return GAME_MUSIC_MENU;
    }

    return GAME_MUSIC_NONE;
}

void UpdateGameAudioForScreen(AppScreen screen)
{
    GameMusicTrack targetTrack;

    if (!gameAudioLoaded) return;

    targetTrack = GetMusicTrackForScreen(screen);

    if (targetTrack != activeMusicTrack)
    {
        StopActiveMusic();

        if (targetTrack != GAME_MUSIC_NONE && gameMusicLoaded[targetTrack])
        {
            SeekMusicStream(gameMusic[targetTrack], 0.0f);
            PlayMusicStream(gameMusic[targetTrack]);
            activeMusicTrack = targetTrack;
        }
    }

    if (activeMusicTrack != GAME_MUSIC_NONE && gameMusicLoaded[activeMusicTrack])
    {
        UpdateMusicStream(gameMusic[activeMusicTrack]);
    }
}

void PlayGameSoundEffect(GameSoundEffect effect)
{
    if (!gameAudioLoaded) return;
    if (effect < 0 || effect >= GAME_SOUND_COUNT) return;
    if (!gameSoundLoaded[effect]) return;

    PlaySound(gameSounds[effect]);
}

static float Clamp01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;

    return value;
}

float GetGameMusicVolume(void)
{
    return gameMusicVolume;
}

float GetGameSoundVolume(void)
{
    return gameSoundVolume;
}

void SetGameMusicVolume(float volume)
{
    int i;

    gameMusicVolume = Clamp01(volume);

    for (i = 0; i < GAME_MUSIC_COUNT; i++)
    {
        if (gameMusicLoaded[i])
        {
            SetMusicVolume(gameMusic[i], gameMusicVolume);
        }
    }
}

void SetGameSoundVolume(float volume)
{
    int i;

    gameSoundVolume = Clamp01(volume);

    for (i = 0; i < GAME_SOUND_COUNT; i++)
    {
        if (gameSoundLoaded[i])
        {
            SetSoundVolume(gameSounds[i], gameSoundVolume);
        }
    }
}

int IsTurnTimerEnabled(void)
{
    return turnTimerEnabled;
}

int GetTurnTimeLimitSeconds(void)
{
    return turnTimeLimitSeconds;
}

void SetTurnTimerEnabled(int enabled)
{
    turnTimerEnabled = enabled ? 1 : 0;
}

void SetTurnTimeLimitSeconds(int seconds)
{
    if (seconds < 1) seconds = 1;
    if (seconds > 999) seconds = 999;

    turnTimeLimitSeconds = seconds;
}

static int PreviousUtf8Index(const char* text, int index)
{
    if (index <= 0) return 0;

    index--;

    while (index > 0 && ((unsigned char)text[index] & 0xC0) == 0x80)
    {
        index--;
    }

    return index;
}

static int NextUtf8Index(const char* text, int index)
{
    int len = (int)strlen(text);
    int codepointSize = 0;

    if (index >= len) return len;

    GetCodepointNext(text + index, &codepointSize);

    if (codepointSize <= 0) codepointSize = 1;
    if (index + codepointSize > len) return len;

    return index + codepointSize;
}

static void ClampUITextInputState(UITextInputState* state, const char* text)
{
    int len = (int)strlen(text);

    if (state->cursor < 0) state->cursor = 0;
    if (state->cursor > len) state->cursor = len;

    if (state->selectionStart < 0) state->selectionStart = 0;
    if (state->selectionStart > len) state->selectionStart = len;

    if (state->selectionEnd < 0) state->selectionEnd = 0;
    if (state->selectionEnd > len) state->selectionEnd = len;

    if (state->selectionAnchor > len) state->selectionAnchor = len;
}

static int HasUITextSelection(const UITextInputState* state)
{
    return state->selectionStart != state->selectionEnd;
}

static void ClearUITextSelection(UITextInputState* state)
{
    state->selectionStart = state->cursor;
    state->selectionEnd = state->cursor;
    state->selectionAnchor = -1;
}

static void SetUITextSelectionFromAnchor(UITextInputState* state)
{
    if (state->selectionAnchor < 0)
    {
        state->selectionAnchor = state->cursor;
    }

    if (state->cursor < state->selectionAnchor)
    {
        state->selectionStart = state->cursor;
        state->selectionEnd = state->selectionAnchor;
    }
    else
    {
        state->selectionStart = state->selectionAnchor;
        state->selectionEnd = state->cursor;
    }
}

static void DeleteUITextSelection(UITextInputState* state, char* buffer)
{
    int len = (int)strlen(buffer);
    int start = state->selectionStart;
    int end = state->selectionEnd;

    if (start > end)
    {
        int temp = start;
        start = end;
        end = temp;
    }

    if (start < 0) start = 0;
    if (end > len) end = len;
    if (start >= end) return;

    memmove(buffer + start, buffer + end, len - end + 1);
    state->cursor = start;
    ClearUITextSelection(state);
}

static int InsertUITextBytes(UITextInputState* state, char* buffer, int capacity, const char* bytes, int byteCount)
{
    int len;

    if (bytes == NULL || byteCount <= 0) return 0;

    if (HasUITextSelection(state))
    {
        DeleteUITextSelection(state, buffer);
    }

    len = (int)strlen(buffer);

    if (len + byteCount >= capacity) return 0;

    memmove(buffer + state->cursor + byteCount,
            buffer + state->cursor,
            len - state->cursor + 1);
    memcpy(buffer + state->cursor, bytes, byteCount);
    state->cursor += byteCount;
    ClearUITextSelection(state);

    return 1;
}

static void InsertUITextClipboard(UITextInputState* state, char* buffer, int capacity)
{
    const char* clipboard = GetClipboardText();
    int index = 0;

    if (clipboard == NULL) return;

    if (HasUITextSelection(state))
    {
        DeleteUITextSelection(state, buffer);
    }

    while (clipboard[index] != '\0')
    {
        int codepointSize = 0;
        int codepoint = GetCodepointNext(clipboard + index, &codepointSize);

        if (codepointSize <= 0) codepointSize = 1;

        if (codepoint >= 32)
        {
            if (!InsertUITextBytes(state, buffer, capacity, clipboard + index, codepointSize))
            {
                break;
            }
        }

        index += codepointSize;
    }
}

static void CopyUITextSelectionToClipboard(const UITextInputState* state, const char* buffer)
{
    char selectedText[512];
    int start = state->selectionStart;
    int end = state->selectionEnd;
    int count;

    if (start > end)
    {
        int temp = start;
        start = end;
        end = temp;
    }

    count = end - start;

    if (count <= 0) return;
    if (count >= (int)sizeof(selectedText)) count = (int)sizeof(selectedText) - 1;

    memcpy(selectedText, buffer + start, count);
    selectedText[count] = '\0';

    SetClipboardText(selectedText);
}

static int MeasureUITextPrefix(const char* text, int byteCount, int fontSize)
{
    char temp[512];

    if (byteCount <= 0) return 0;
    if (byteCount >= (int)sizeof(temp)) byteCount = (int)sizeof(temp) - 1;

    memcpy(temp, text, byteCount);
    temp[byteCount] = '\0';

    return MeasureUIFont(UI_FONT_TEXT, temp, fontSize);
}

static int GetUITextCursorFromMouseX(const char* text, Rectangle rect, int fontSize, float mouseX)
{
    int len = (int)strlen(text);
    int previousIndex = 0;
    int index = 0;
    float localX = mouseX - rect.x - 12.0f;

    if (localX <= 0.0f) return 0;

    while (index < len)
    {
        int nextIndex = NextUtf8Index(text, index);
        int prevWidth = MeasureUITextPrefix(text, previousIndex, fontSize);
        int nextWidth = MeasureUITextPrefix(text, nextIndex, fontSize);
        float mid = ((float)prevWidth + (float)nextWidth) * 0.5f;

        if (localX < mid)
        {
            return previousIndex;
        }

        previousIndex = nextIndex;
        index = nextIndex;
    }

    return len;
}

static int PreviousUITextWordIndex(const char* text, int index)
{
    index = PreviousUtf8Index(text, index);

    while (index > 0 && text[index] == ' ')
    {
        index = PreviousUtf8Index(text, index);
    }

    while (index > 0)
    {
        int previousIndex = PreviousUtf8Index(text, index);

        if (text[previousIndex] == ' ')
        {
            break;
        }

        index = previousIndex;
    }

    return index;
}

static int NextUITextWordIndex(const char* text, int index)
{
    int len = (int)strlen(text);

    while (index < len && text[index] == ' ')
    {
        index = NextUtf8Index(text, index);
    }

    while (index < len)
    {
        int nextIndex = NextUtf8Index(text, index);

        if (text[nextIndex] == ' ')
        {
            index = nextIndex;
            break;
        }

        index = nextIndex;
    }

    while (index < len && text[index] == ' ')
    {
        index = NextUtf8Index(text, index);
    }

    return index;
}

static int IsKeyPressedOrRepeated(int key)
{
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

void InitUITextInputState(UITextInputState* state)
{
    if (state == NULL) return;

    state->cursor = 0;
    state->selectionStart = 0;
    state->selectionEnd = 0;
    state->selectionAnchor = -1;
}

void SetUITextInputCursorToEnd(UITextInputState* state, const char* text)
{
    if (state == NULL) return;

    state->cursor = text == NULL ? 0 : (int)strlen(text);
    state->selectionStart = state->cursor;
    state->selectionEnd = state->cursor;
    state->selectionAnchor = -1;
}

void UpdateUITextInput(UITextInputState* state, char* buffer, int capacity, Rectangle rect, int active, int fontSize)
{
    int ctrlDown;
    int shiftDown;
    int codepoint;

    if (state == NULL || buffer == NULL || capacity <= 0) return;

    ClampUITextInputState(state, buffer);

    if (!active) return;

    ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
        CheckCollisionPointRec(GetVirtualMousePosition(), rect))
    {
        state->cursor = GetUITextCursorFromMouseX(buffer, rect, fontSize, GetVirtualMousePosition().x);
        ClearUITextSelection(state);
    }

    if (ctrlDown && IsKeyPressed(KEY_A))
    {
        state->cursor = (int)strlen(buffer);
        state->selectionStart = 0;
        state->selectionEnd = state->cursor;
        state->selectionAnchor = 0;
    }

    if (ctrlDown && IsKeyPressed(KEY_C) && HasUITextSelection(state))
    {
        CopyUITextSelectionToClipboard(state, buffer);
    }

    if (ctrlDown && IsKeyPressed(KEY_X) && HasUITextSelection(state))
    {
        CopyUITextSelectionToClipboard(state, buffer);
        DeleteUITextSelection(state, buffer);
    }

    if (ctrlDown && IsKeyPressed(KEY_V))
    {
        InsertUITextClipboard(state, buffer, capacity);
    }

    if (IsKeyPressedOrRepeated(KEY_HOME))
    {
        if (shiftDown)
        {
            if (state->selectionAnchor < 0) state->selectionAnchor = state->cursor;
            state->cursor = 0;
            SetUITextSelectionFromAnchor(state);
        }
        else
        {
            state->cursor = 0;
            ClearUITextSelection(state);
        }
    }

    if (IsKeyPressedOrRepeated(KEY_END))
    {
        if (shiftDown)
        {
            if (state->selectionAnchor < 0) state->selectionAnchor = state->cursor;
            state->cursor = (int)strlen(buffer);
            SetUITextSelectionFromAnchor(state);
        }
        else
        {
            state->cursor = (int)strlen(buffer);
            ClearUITextSelection(state);
        }
    }

    if (IsKeyPressedOrRepeated(KEY_LEFT))
    {
        if (HasUITextSelection(state) && !shiftDown)
        {
            state->cursor = state->selectionStart;
            ClearUITextSelection(state);
        }
        else
        {
            if (shiftDown && state->selectionAnchor < 0) state->selectionAnchor = state->cursor;
            state->cursor = ctrlDown ? PreviousUITextWordIndex(buffer, state->cursor) : PreviousUtf8Index(buffer, state->cursor);
            if (shiftDown) SetUITextSelectionFromAnchor(state);
            else ClearUITextSelection(state);
        }
    }

    if (IsKeyPressedOrRepeated(KEY_RIGHT))
    {
        if (HasUITextSelection(state) && !shiftDown)
        {
            state->cursor = state->selectionEnd;
            ClearUITextSelection(state);
        }
        else
        {
            if (shiftDown && state->selectionAnchor < 0) state->selectionAnchor = state->cursor;
            state->cursor = ctrlDown ? NextUITextWordIndex(buffer, state->cursor) : NextUtf8Index(buffer, state->cursor);
            if (shiftDown) SetUITextSelectionFromAnchor(state);
            else ClearUITextSelection(state);
        }
    }

    if (IsKeyPressedOrRepeated(KEY_BACKSPACE))
    {
        if (HasUITextSelection(state))
        {
            DeleteUITextSelection(state, buffer);
        }
        else if (state->cursor > 0)
        {
            int previousIndex = ctrlDown ? PreviousUITextWordIndex(buffer, state->cursor) : PreviousUtf8Index(buffer, state->cursor);
            int len = (int)strlen(buffer);

            memmove(buffer + previousIndex, buffer + state->cursor, len - state->cursor + 1);
            state->cursor = previousIndex;
            ClearUITextSelection(state);
        }
    }

    if (IsKeyPressedOrRepeated(KEY_DELETE))
    {
        if (HasUITextSelection(state))
        {
            DeleteUITextSelection(state, buffer);
        }
        else if (state->cursor < (int)strlen(buffer))
        {
            int nextIndex = ctrlDown ? NextUITextWordIndex(buffer, state->cursor) : NextUtf8Index(buffer, state->cursor);
            int len = (int)strlen(buffer);

            memmove(buffer + state->cursor, buffer + nextIndex, len - nextIndex + 1);
            ClearUITextSelection(state);
        }
    }

    if (!ctrlDown)
    {
        codepoint = GetCharPressed();

        while (codepoint > 0)
        {
            int utf8Size = 0;
            const char* utf8Text = CodepointToUTF8(codepoint, &utf8Size);

            if (codepoint >= 32 && utf8Text != NULL && utf8Size > 0)
            {
                InsertUITextBytes(state, buffer, capacity, utf8Text, utf8Size);
            }

            codepoint = GetCharPressed();
        }
    }
}

void DrawUITextInputBox(Rectangle rect, const UITextInputState* state, const char* text, int active, int fontSize)
{
    int selectionStart = 0;
    int selectionEnd = 0;

    DrawRectangleRec(rect, RAYWHITE);
    DrawRectangleLinesEx(rect, 2, active ? BLUE : DARKGRAY);

    if (state != NULL && state->selectionStart != state->selectionEnd)
    {
        int xStart;
        int xEnd;

        selectionStart = state->selectionStart;
        selectionEnd = state->selectionEnd;

        if (selectionStart > selectionEnd)
        {
            int temp = selectionStart;
            selectionStart = selectionEnd;
            selectionEnd = temp;
        }

        xStart = (int)rect.x + 12 + MeasureUITextPrefix(text, selectionStart, fontSize);
        xEnd = (int)rect.x + 12 + MeasureUITextPrefix(text, selectionEnd, fontSize);

        DrawRectangle(xStart, (int)rect.y + 8, xEnd - xStart, (int)rect.height - 16, (Color){ 185, 215, 255, 255 });
    }

    DrawText(text, (int)rect.x + 12, (int)(rect.y + rect.height / 2 - fontSize / 2), fontSize, DARKGRAY);

    if (active && state != NULL && ((int)(GetTime() * 2.0) % 2 == 0))
    {
        int cursorX = (int)rect.x + 14 + MeasureUITextPrefix(text, state->cursor, fontSize);
        DrawText("|", cursorX, (int)(rect.y + rect.height / 2 - fontSize / 2), fontSize, DARKBLUE);
    }
}

float GetScreenScaleFactor(void)
{
    float scaleX = (float)GetScreenWidth() / (float)VIRTUAL_WIDTH;
    float scaleY = (float)GetScreenHeight() / (float)VIRTUAL_HEIGHT;

    return scaleX < scaleY ? scaleX : scaleY;
}

Vector2 GetScreenOffset(void)
{
    float scale = GetScreenScaleFactor();

    Vector2 offset;
    offset.x = ((float)GetScreenWidth() - (float)VIRTUAL_WIDTH * scale) * 0.5f;
    offset.y = ((float)GetScreenHeight() - (float)VIRTUAL_HEIGHT * scale) * 0.5f;

    return offset;
}

Vector2 GetVirtualMousePosition(void)
{
    Vector2 mouse = GetMousePosition();
    Vector2 offset = GetScreenOffset();
    float scale = GetScreenScaleFactor();

    Vector2 virtualMouse;

    if (scale <= 0.0f)
    {
        virtualMouse.x = 0.0f;
        virtualMouse.y = 0.0f;
        return virtualMouse;
    }

    virtualMouse.x = (mouse.x - offset.x) / scale;
    virtualMouse.y = (mouse.y - offset.y) / scale;

    return virtualMouse;
}

static void BeginVirtualCanvas(void)
{
    float scale = GetScreenScaleFactor();
    Vector2 offset = GetScreenOffset();

    int viewX = (int)offset.x;
    int viewY = (int)offset.y;
    int viewW = (int)((float)VIRTUAL_WIDTH * scale);
    int viewH = (int)((float)VIRTUAL_HEIGHT * scale);

    BeginScissorMode(viewX, viewY, viewW, viewH);

    rlPushMatrix();
    rlTranslatef(offset.x, offset.y, 0.0f);
    rlScalef(scale, scale, 1.0f);
}

static void EndVirtualCanvas(void)
{
    rlPopMatrix();
    EndScissorMode();
}

static void DrawOuterBackground(void)
{
    /*
        Sau này nếu có ảnh background fit to screen,
        ta sẽ vẽ nó ở đây bằng kích thước GetScreenWidth(), GetScreenHeight().
    */
    ClearBackground((Color){ 230, 226, 210, 255 });
}

static void DrawVirtualBackground(void)
{
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, (Color){ 245, 245, 238, 255 });
}

static Rectangle GetSettingsBackButtonRect(void)
{
    Rectangle rect = { 490.0f, 590.0f, 300.0f, 52.0f };
    return rect;
}

static int IsSettingsBackButtonClicked(void)
{
    Vector2 mouse = GetVirtualMousePosition();
    Rectangle backButton = GetSettingsBackButtonRect();

    return CheckCollisionPointRec(mouse, backButton) &&
           IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static void DrawSettingsButton(Rectangle rect, const char* text)
{
    Vector2 mouse = GetVirtualMousePosition();
    int hover = CheckCollisionPointRec(mouse, rect);

    DrawRectangleRec(rect, hover ? SKYBLUE : LIGHTGRAY);
    DrawRectangleLinesEx(rect, 2, hover ? BLUE : DARKGRAY);

    DrawUIFont(UI_FONT_BUTTON,
               text,
               (int)(rect.x + rect.width / 2 - MeasureUIFont(UI_FONT_BUTTON, text, 22) / 2),
               (int)(rect.y + rect.height / 2 - 11),
               22,
               DARKBLUE);
}

static Rectangle GetSettingsSliderRect(int index)
{
    Rectangle rect = { 455.0f, 210.0f + index * 74.0f, 360.0f, 10.0f };
    return rect;
}

static Rectangle GetSettingsToggleRect(void)
{
    Rectangle rect = { 455.0f, 370.0f, 74.0f, 34.0f };
    return rect;
}

static Rectangle GetSettingsTimeInputRect(void)
{
    Rectangle rect = { 455.0f, 472.0f, 170.0f, 46.0f };
    return rect;
}

static void EnsureSettingsControlsInitialized(void)
{
    if (settingsInputInitialized) return;

    snprintf(turnTimeInput, sizeof(turnTimeInput), "%d", GetTurnTimeLimitSeconds());
    SetUITextInputCursorToEnd(&turnTimeInputState, turnTimeInput);
    settingsInputInitialized = 1;
}

static float GetSettingsSliderValue(Rectangle rect)
{
    Vector2 mouse = GetVirtualMousePosition();
    float value = (mouse.x - rect.x) / rect.width;

    return Clamp01(value);
}

static void UpdateSettingsSlider(int index, Rectangle rect)
{
    Rectangle hitRect = { rect.x - 12.0f, rect.y - 18.0f, rect.width + 24.0f, rect.height + 36.0f };
    Vector2 mouse = GetVirtualMousePosition();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, hitRect))
    {
        activeSettingsSlider = index;
        activeSettingsInput = -1;
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && activeSettingsSlider == index)
    {
        activeSettingsSlider = -1;
    }

    if (activeSettingsSlider == index && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
    {
        float value = GetSettingsSliderValue(rect);

        if (index == 0)
        {
            SetGameMusicVolume(value);
        }
        else
        {
            SetGameSoundVolume(value);
        }
    }
}

static void DrawSettingsSlider(Rectangle rect, const char* label, float value)
{
    float clampedValue = Clamp01(value);
    int percent = (int)(clampedValue * 100.0f + 0.5f);
    int knobX = (int)(rect.x + rect.width * clampedValue);

    DrawText(label, (int)rect.x, (int)rect.y - 34, 20, DARKGRAY);
    DrawText(TextFormat("%d%%", percent), (int)(rect.x + rect.width + 28), (int)rect.y - 34, 20, DARKBLUE);

    DrawRectangleRec(rect, LIGHTGRAY);
    DrawRectangle((int)rect.x, (int)rect.y, knobX - (int)rect.x, (int)rect.height, SKYBLUE);
    DrawRectangleLinesEx(rect, 2, DARKGRAY);
    DrawCircle(knobX, (int)(rect.y + rect.height / 2), 12.0f, BLUE);
    DrawCircleLines(knobX, (int)(rect.y + rect.height / 2), 12.0f, DARKBLUE);
}

static void DrawSettingsToggle(Rectangle rect, int enabled)
{
    int knobX = enabled ? (int)(rect.x + rect.width - 17.0f) : (int)(rect.x + 17.0f);
    Color bg = enabled ? (Color){ 70, 180, 115, 255 } : LIGHTGRAY;

    DrawRectangleRounded(rect, 0.5f, 16, bg);
    DrawRectangleRoundedLinesEx(rect, 0.5f, 16, 2.0f, enabled ? DARKGREEN : DARKGRAY);
    DrawCircle(knobX, (int)(rect.y + rect.height / 2), 13.0f, RAYWHITE);
    DrawCircleLines(knobX, (int)(rect.y + rect.height / 2), 13.0f, DARKGRAY);
}

static void SanitizeTurnTimeInput(void)
{
    char sanitized[sizeof(turnTimeInput)];
    int readIndex;
    int writeIndex = 0;

    for (readIndex = 0; turnTimeInput[readIndex] != '\0' && writeIndex < (int)sizeof(sanitized) - 1; readIndex++)
    {
        if (turnTimeInput[readIndex] >= '0' && turnTimeInput[readIndex] <= '9')
        {
            sanitized[writeIndex++] = turnTimeInput[readIndex];
        }
    }

    sanitized[writeIndex] = '\0';

    if (strcmp(sanitized, turnTimeInput) != 0)
    {
        strcpy(turnTimeInput, sanitized);
        SetUITextInputCursorToEnd(&turnTimeInputState, turnTimeInput);
    }
}

static void ApplyTurnTimeInput(void)
{
    int value;

    SanitizeTurnTimeInput();

    if (turnTimeInput[0] == '\0')
    {
        return;
    }

    value = atoi(turnTimeInput);
    SetTurnTimeLimitSeconds(value);

    if (value != GetTurnTimeLimitSeconds())
    {
        snprintf(turnTimeInput, sizeof(turnTimeInput), "%d", GetTurnTimeLimitSeconds());
        SetUITextInputCursorToEnd(&turnTimeInputState, turnTimeInput);
    }
}

static void UpdateAndDrawSettings(AppScreen* currentScreen)
{
    const char* title = "CÀI ĐẶT";
    Rectangle musicSlider = GetSettingsSliderRect(0);
    Rectangle soundSlider = GetSettingsSliderRect(1);
    Rectangle timerToggle = GetSettingsToggleRect();
    Rectangle timeInput = GetSettingsTimeInputRect();
    Rectangle backButton = GetSettingsBackButtonRect();
    Vector2 mouse = GetVirtualMousePosition();

    EnsureSettingsControlsInitialized();

    UpdateSettingsSlider(0, musicSlider);
    UpdateSettingsSlider(1, soundSlider);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        if (CheckCollisionPointRec(mouse, timerToggle))
        {
            SetTurnTimerEnabled(!IsTurnTimerEnabled());
            activeSettingsInput = -1;
        }
        else if (CheckCollisionPointRec(mouse, timeInput))
        {
            activeSettingsInput = 0;
        }
        else
        {
            activeSettingsInput = -1;
        }
    }

    UpdateUITextInput(&turnTimeInputState,
                      turnTimeInput,
                      sizeof(turnTimeInput),
                      timeInput,
                      activeSettingsInput == 0,
                      18);
    ApplyTurnTimeInput();

    if (IsSettingsBackButtonClicked())
    {
        if (turnTimeInput[0] == '\0')
        {
            snprintf(turnTimeInput, sizeof(turnTimeInput), "%d", GetTurnTimeLimitSeconds());
            SetUITextInputCursorToEnd(&turnTimeInputState, turnTimeInput);
        }

        activeSettingsInput = -1;
        activeSettingsSlider = -1;
        *currentScreen = SCREEN_MENU;
        return;
    }

    DrawUIFont(UI_FONT_HEADING,
               title,
               SCREEN_WIDTH / 2 - MeasureUIFont(UI_FONT_HEADING, title, 44) / 2,
               82,
               44,
               DARKBLUE);

    DrawUIFont(UI_FONT_HEADING, "Âm thanh", 310, 158, 28, DARKBLUE);
    DrawSettingsSlider(musicSlider, "Nhạc nền", GetGameMusicVolume());
    DrawSettingsSlider(soundSlider, "Hiệu ứng", GetGameSoundVolume());

    DrawUIFont(UI_FONT_HEADING, "Thời gian", 310, 328, 28, DARKBLUE);
    DrawText("Giới hạn thời gian mỗi lượt", 455, 337, 20, DARKGRAY);
    DrawSettingsToggle(timerToggle, IsTurnTimerEnabled());
    DrawText(IsTurnTimerEnabled() ? "Bật" : "Tắt", 550, 376, 20, IsTurnTimerEnabled() ? DARKGREEN : DARKGRAY);

    DrawText("Số giây mỗi lượt", 455, 440, 20, DARKGRAY);
    DrawUITextInputBox(timeInput, &turnTimeInputState, turnTimeInput, activeSettingsInput == 0, 18);
    DrawText("giây", 640, 485, 18, DARKGRAY);

    DrawSettingsButton(backButton, "Quay lại");
}

int main(void)
{
    AppScreen currentScreen = SCREEN_MENU;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, "Đồ án Caro - Raylib C");
    InitAudioDevice();
    LoadGameAudio();
    LoadGameFonts();

    SetWindowMinSize(800, 450);
    SetExitKey(KEY_NULL);

    SetTargetFPS(60);

    InitGame();

    while (!WindowShouldClose() && currentScreen != SCREEN_EXIT)
    {
        BeginDrawing();

        DrawOuterBackground();

        BeginVirtualCanvas();

        DrawVirtualBackground();

        if (currentScreen == SCREEN_MENU)
        {
            int menuAction = UpdateMenu();

            if (menuAction == MENU_NEW_GAME)
            {
                currentScreen = SCREEN_NEW_GAME_MODE;
            }
            else if (menuAction == MENU_LOAD_GAME)
            {
                OpenSaveBrowser(&currentScreen, BROWSER_LOAD, SCREEN_MENU);
            }
            else if (menuAction == MENU_SETTINGS)
            {
                currentScreen = SCREEN_SETTINGS;
            }
            else if (menuAction == MENU_EXIT)
            {
                currentScreen = SCREEN_EXIT;
            }

            DrawMenu();
        }
        else if (currentScreen == SCREEN_NEW_GAME_MODE)
        {
            int newGameAction = UpdateNewGameModeMenu();

            if (newGameAction == NEW_GAME_PVP)
            {
                ResetPlayerNameMenu();
                currentScreen = SCREEN_PLAYER_NAMES;
            }
            else if (newGameAction == NEW_GAME_BOT)
            {
                currentScreen = SCREEN_BOT_DIFFICULTY;
            }
            else if (newGameAction == NEW_GAME_BACK)
            {
                currentScreen = SCREEN_MENU;
            }

            DrawNewGameModeMenu();
        }
        else if (currentScreen == SCREEN_PLAYER_NAMES)
        {
            int playerNameAction = UpdatePlayerNameMenu();

            if (playerNameAction == PLAYER_NAMES_START)
            {
                StartNewGameWithNames(
                    GAME_MODE_PVP,
                    BOT_DIFFICULTY_EASY,
                    GetPlayerNameXInput(),
                    GetPlayerNameOInput()
                );
                currentScreen = SCREEN_GAME;
            }
            else if (playerNameAction == PLAYER_NAMES_BACK)
            {
                currentScreen = SCREEN_NEW_GAME_MODE;
            }

            DrawPlayerNameMenu();
        }
        else if (currentScreen == SCREEN_BOT_DIFFICULTY)
        {
            int difficultyAction = UpdateBotDifficultyMenu();

            if (difficultyAction == BOT_DIFFICULTY_EASY_ACTION)
            {
                StartNewGame(GAME_MODE_BOT, BOT_DIFFICULTY_EASY);
                currentScreen = SCREEN_GAME;
            }
            else if (difficultyAction == BOT_DIFFICULTY_MEDIUM_ACTION)
            {
                StartNewGame(GAME_MODE_BOT, BOT_DIFFICULTY_MEDIUM);
                currentScreen = SCREEN_GAME;
            }
            else if (difficultyAction == BOT_DIFFICULTY_HARD_ACTION)
            {
                StartNewGame(GAME_MODE_BOT, BOT_DIFFICULTY_HARD);
                currentScreen = SCREEN_GAME;
            }
            else if (difficultyAction == BOT_DIFFICULTY_BACK)
            {
                currentScreen = SCREEN_NEW_GAME_MODE;
            }

            DrawBotDifficultyMenu();
        }
        else if (currentScreen == SCREEN_GAME)
        {
            UpdateGame(&currentScreen);
            DrawGame();
        }
        else if (currentScreen == SCREEN_PAUSE)
        {
            UpdatePauseMenu(&currentScreen);
            DrawGame();
            DrawPauseMenu();
        }
        else if (currentScreen == SCREEN_GAME_OVER)
        {
            UpdateGameOver(&currentScreen);
            DrawGame();
        }
        else if (currentScreen == SCREEN_SAVE || currentScreen == SCREEN_LOAD)
        {
            UpdateSaveBrowser(&currentScreen);
            DrawSaveBrowser();
        }
        else if (currentScreen == SCREEN_SETTINGS)
        {
            UpdateAndDrawSettings(&currentScreen);
        }

        UpdateGameAudioForScreen(currentScreen);

        EndVirtualCanvas();

        EndDrawing();
    }

    UnloadGameAudio();
    if (IsAudioDeviceReady())
    {
        CloseAudioDevice();
    }
    UnloadGameFonts();
    CloseWindow();

    return 0;
}
