// In include/ui_manager.h
#pragma once

#include "raylib.h"
#include "game_state.h" // For GameScreen
#include "constants.h"  // For font/color constants

class UIManager {
public:
    Texture2D titleBg;
    Texture2D howToPlayBg;          // Background for the entire How to Play screen
    Texture2D howToPlayInstructions1; // Image for the first page of instructions
    Texture2D howToPlayInstructions2; // Image for the second page of instructions
    Texture2D gameOverBg;

    Font titleTextFont;  // Used for the main game title AND "How to Play" screen title
    Font bodyTextFont;   // For button text, etc.
    Music mainMenuMusic; // Music for the main menu
    Sound buttonClickSound; // Sound for button clicks
    // Font hudTextFont;  // If you have it

    int currentInstructionPage; // To track which instruction page is visible (1 or 2)

    UIManager();
    void LoadAssets();
    void UnloadAssets();

    void DrawMainMenu(GameScreen& currentScreen, bool& quitGameFlag, bool& wantsToStartNewGame);
    void DrawHowToPlay(GameScreen& currentScreen); // Signature remains the same
    void DrawInGameHUD(float timer, int hidersLeft, float sprintValue);
    void DrawPauseMenu(GameScreen& currentScreen, bool& quitGame, bool& restartGame);
    void DrawGameOverScreen(GameScreen& currentScreen, bool playerWon, float finalTime, bool& wantsToPlayAgain);

private:
    bool DrawButton(Rectangle bounds, const char* text, int fontSize, Color baseColor, Color hoverColor, Color textColor);
};
