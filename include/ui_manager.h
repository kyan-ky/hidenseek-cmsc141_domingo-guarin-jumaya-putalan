#pragma once

#include "raylib.h"
#include "game_state.h"
#include "constants.h"

class UIManager {
public:
    Texture2D titleBg;
    Texture2D howToPlayBg;
    Texture2D gameOverBg;
    //Font gameFont; // Optional, otherwise uses default
    Font titleTextFont;
    Font bodyTextFont;

    UIManager();
    void LoadAssets();
    void UnloadAssets();

    void DrawMainMenu(GameScreen& currentScreen, bool& quitGameFlag);
    void DrawHowToPlay(GameScreen& currentScreen);
    void DrawInGameHUD(float timer, int hidersLeft, float sprintValue);
    void DrawPauseMenu(GameScreen& currentScreen, bool& quitGame, bool& restartGame);
    void DrawGameOverScreen(GameScreen& currentScreen, bool playerWon, float finalTime);

private:
    // Helper for drawing buttons
    bool DrawButton(Rectangle bounds, const char* text, int fontSize, Color baseColor, Color hoverColor, Color textColor);
};

