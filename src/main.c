#include "raylib.h"
#include "common.h"
#include "model.h"
#include "view.h"
#include "controller.h"

int main(void) {
    const int screenWidth = 1600;
    const int screenHeight = 900;

    InitWindow(screenWidth, screenHeight, "Liminal Caro");
    SetExitKey(KEY_NULL);
    InitAudioDevice();
    SetTargetFPS(60);

    Model_Init();
    View_Init();

    bool exitGame = false;

    while (!exitGame) {
        View_UpdateAudio(); 

        exitGame = Controller_Update();

        int loadingProgress = Controller_GetLoadingProgress();

        View_Draw(loadingProgress);
    }

    View_UnloadAssets();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}