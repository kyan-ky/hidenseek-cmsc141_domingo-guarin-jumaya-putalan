#include "raylib.h"
#include "resource_dir.h" // From template
#include "constants.h"
#include "game_manager.h"
#include <iostream> // For debugging

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, GAME_TITLE);
    SetTargetFPS(60);

    // Initialize audio device
    InitAudioDevice();

    if (!SearchAndSetResourceDir("resources")) {
        std::cout << "Warning: Could not find or set 'resources' directory. Asset loading might fail." << std::endl;
        // If you want to be strict, you can exit here or try a default path.
        // For now, we'll let it continue and see if Raylib can find files relative to executable.
    } else {
        std::cout << "Resource directory set to: " << GetWorkingDirectory() << std::endl;
    }

    GameManager gameManager;
    gameManager.InitGame(); // Initialize game state, load assets, etc.

    while (!WindowShouldClose() && !gameManager.quitGame) {
        gameManager.Update();
        gameManager.Draw();
    }

    CloseAudioDevice(); // Close audio device before closing window
    CloseWindow();
    return 0;
}
