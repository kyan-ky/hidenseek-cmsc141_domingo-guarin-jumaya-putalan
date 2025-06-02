#pragma once

#include "raylib.h"
#include "game_state.h" 
#include "constants.h"  

class GameManager; // Forward declaration

class UIManager {
public:
    Texture2D titleBg;
    Texture2D howToPlayBg;          
    Texture2D howToPlayInstructions1; 
    Texture2D howToPlayInstructions2; 
    Texture2D gameOverBg;

    Font titleTextFont;  
    Font bodyTextFont;   
    // Font hudTextFont; // Declare if you plan to use a separate HUD font

    int currentInstructionPage; 
    GameManager* gameManagerPtr; // Pointer to GameManager

    UIManager();
    void SetGameManager(GameManager* gm); 
    void LoadAssets();
    void UnloadAssets();

    void DrawMainMenu(GameScreen& currentScreen, bool& quitGameFlag, bool& wantsToStartNewGame);
    void DrawHowToPlay(GameScreen& currentScreen); 
    void DrawInGameHUD(float timer, int hidersLeft, float sprintValue);
    void DrawPauseMenu(GameScreen& currentScreen, bool& quitGame, bool& restartGame);
    void DrawGameOverScreen(GameScreen& currentScreen, bool playerWon, float finalTime, bool& wantsToPlayAgain);

private:
    bool DrawButton(Rectangle bounds, const char* text, int fontSize, Color baseColor, Color hoverColor, Color textColor);
};
