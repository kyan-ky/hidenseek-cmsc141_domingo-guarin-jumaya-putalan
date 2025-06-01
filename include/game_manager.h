#pragma once

#include "raylib.h"
#include "game_state.h"
#include "player.h"
#include "hider.h"
#include "map.h"
#include "ui_manager.h"
#include <vector>

class GameManager {
public:
    GameScreen currentScreen;
    GamePhase currentPhase;

    Player player;
    std::vector<Hider> hiders;
    Map gameMap;
    UIManager uiManager;

    float gameTimer; // Used for both hiding and seeking phases
    float hidingPhaseElapsed;
    int hidersRemaining;
    bool playerWon;
    float lastGameTime; // To display on game over

    bool quitGame; // Flag to exit game loop
    bool restartGameFlag; // Flag to re-initialize game

    GameManager();
    ~GameManager();

    void InitGame(); // Initializes/Resets the game state for a new round
    void Update();
    void Draw();

private:
    void UpdateMainMenu();
    void UpdateHowToPlay();
    void UpdateInGame();
    void UpdatePauseMenu();
    void UpdateGameOver();

    void DrawMainMenu();
    void DrawHowToPlay();
    void DrawInGame();
    void DrawPauseMenu();
    void DrawGameOver();

    //void CheckWinLossConditions();
    void CheckWinLossConditions(bool playerGotTagged);
    void ResetGameValues();
    void StartHidingPhase();
    void StartSeekingPhase();
};

