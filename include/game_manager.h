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
    Camera2D camera; 
    RenderTexture2D visionOverlay; 

    float gameTimer; 
    float hidingPhaseElapsed;
    int hidersRemaining;
    bool playerWon;
    float lastGameTime;

    bool quitGame; 
    bool restartGameFlag; 
                          
    Music mainMenuMusic;
    Sound sfxButtonClick;
    Music countdownMusic;
    Music inGameSeekingMusic;
    Sound sfxTag;
    Music gameOverMusic;
    Music gameWinMusic;

    GameManager();
    ~GameManager();

    void InitGame();
    void Update();
    void Draw();
    void PlayButtonSound();

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

    void CheckWinLossConditions(bool playerGotTagged);
    void ResetGameValues();
    void StartHidingPhase();
    void StartSeekingPhase();

    void LoadAudioAssets();
    void UnloadAudioAssets();
    void PlayMainMenuMusic();
    void StopMainMenuMusic();
};

