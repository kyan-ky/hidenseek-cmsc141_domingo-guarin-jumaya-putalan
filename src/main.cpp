#include "raylib.h"
#include "resource_dir.h" 
#include "constants.h"
#include "game_manager.h"
#include <iostream> 

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, GAME_TITLE);

    InitAudioDevice();
    SetTargetFPS(60);

    if (!SearchAndSetResourceDir("resources")) {
        std::cout << "Warning: Could not find or set 'resources' directory. Asset loading might fail." << std::endl;
    } else {
        std::cout << "Resource directory set to: " << GetWorkingDirectory() << std::endl;
    }


    GameManager gameManager;
    // gameManager.InitGame(); 

    while (!WindowShouldClose() && !gameManager.quitGame) {
        gameManager.Update();
        gameManager.Draw();
    }
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
