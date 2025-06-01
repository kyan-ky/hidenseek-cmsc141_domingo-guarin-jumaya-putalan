#include "game_manager.h"
#include "constants.h"
#include "raymath.h"
#include <cstdlib>   // For srand, rand
#include <ctime>     // For time for srand
#include <algorithm> // For std::all_of
#include <cstdio>

GameManager::GameManager() : currentScreen(GameScreen::MAIN_MENU), currentPhase(GamePhase::HIDING),
                             gameTimer(0.0f), hidersRemaining(0), playerWon(false),
                             quitGame(false), restartGameFlag(false), lastGameTime(0.0f), hidingPhaseElapsed(0.0f) {
    srand((unsigned int)time(NULL)); // Seed random number generator
    uiManager.LoadAssets(); // Load UI assets once
    gameMap.Load();
}

GameManager::~GameManager() {
    uiManager.UnloadAssets();
    gameMap.Unload();
    // Player and Hider textures are simple, handled by their destructors if they dynamically allocate.
    // If textures are loaded in Player/Hider Init and not globally, unload them here or in their destructors.
    // For simplicity, current Player/Hider use basic shapes or load textures in constructor/Init.
}

void GameManager::ResetGameValues() {
    player.Init({SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f}); // Seeker starts in the middle

    hiders.assign(NUM_HIDERS, Hider()); // Create/Reset hiders
    std::vector<Vector2> startingPositions; // Ensure hiders don't spawn on top of each other
    for (int i = 0; i < NUM_HIDERS; ++i) {
        Vector2 pos;
        bool positionOk;
        int attempts = 0;
        do {
            positionOk = true;
            pos = {(float)(rand() % (SCREEN_WIDTH - 100) + 50), (float)(rand() % (SCREEN_HEIGHT - 100) + 50)};
            // Check against player start
            if (Vector2DistanceSqr(pos, player.position) < (PLAYER_RADIUS + HIDER_RADIUS + 50) * (PLAYER_RADIUS + HIDER_RADIUS + 50)) {
                positionOk = false;
                continue;
            }
            // Check against other hiders already placed
            for(size_t j=0; j < startingPositions.size(); ++j) {
                if (Vector2DistanceSqr(pos, startingPositions[j]) < (HIDER_RADIUS * 4) * (HIDER_RADIUS * 4) ) {
                    positionOk = false;
                    break;
                }
            }
            attempts++;
        } while(!positionOk && attempts < 50);
        startingPositions.push_back(pos);
        hiders[i].Init(pos, gameMap);
    }

    hidersRemaining = NUM_HIDERS;
    playerWon = false;
    StartHidingPhase(); // Sets timer and phase
    restartGameFlag = false;
}


void GameManager::InitGame() {
    ResetGameValues();
    // If currentScreen is IN_GAME from a "Play Again" button, it's already set.
    // If coming from MAIN_MENU first time, this InitGame is usually called before loop.
    // currentScreen = GameScreen::IN_GAME; // Ensure game starts in IN_GAME if called fresh
}

void GameManager::StartHidingPhase() {
    currentPhase = GamePhase::HIDING;
    gameTimer = HIDING_PHASE_DURATION;
    hidingPhaseElapsed = 0.0f;
    for (auto& hider : hiders) {
        hider.hidingState = HiderHidingFSMState::SCOUTING;
    }
}

void GameManager::StartSeekingPhase() {
    currentPhase = GamePhase::SEEKING;
    gameTimer = SEEKING_PHASE_DURATION;
    // Hiders switch to their seeking phase FSM behaviors
    for (auto& hider : hiders) {
        hider.seekingState = HiderSeekingFSMState::IDLING;
    }
}

void GameManager::Update() {
    float deltaTime = GetFrameTime();

    if (restartGameFlag) {
        InitGame(); // Fully reset and re-initialize
        currentScreen = GameScreen::IN_GAME; // Ensure we go to game screen
        return; // Skip rest of update for this frame
    }
    
    GameScreen previousScreen = currentScreen;

    switch (currentScreen) {
        case GameScreen::MAIN_MENU:
            UpdateMainMenu();
            break;
        case GameScreen::HOW_TO_PLAY:
            UpdateHowToPlay();
            break;
        case GameScreen::IN_GAME:
            UpdateInGame();
            break;
        case GameScreen::PAUSE_MENU:
            UpdatePauseMenu();
            break;
        case GameScreen::GAME_OVER:
            UpdateGameOver();
            break;
    }

    // If "Play Again" from GameOver or "Start Over" from Pause was clicked
    if ((previousScreen == GameScreen::GAME_OVER && currentScreen == GameScreen::IN_GAME) ||
        (previousScreen == GameScreen::PAUSE_MENU && currentScreen == GameScreen::IN_GAME && restartGameFlag) ) {
        InitGame();
    }
    // If returning to Main Menu from Pause or Game Over, then clicking "Start Game"
    if ((previousScreen == GameScreen::PAUSE_MENU || previousScreen == GameScreen::GAME_OVER) &&
         currentScreen == GameScreen::MAIN_MENU) {
        // No immediate reset needed here, reset will happen if "Start Game" is clicked from Main Menu
    }
     if (previousScreen == GameScreen::MAIN_MENU && currentScreen == GameScreen::IN_GAME) {
        InitGame(); // Fresh start from main menu
    }


}

void GameManager::UpdateMainMenu() {
    // UI Manager handles button clicks and changes currentScreen
    // If quit is selected on main menu (hypothetical button):
    // Rectangle quitButtonBounds = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.5f + 160, 300, 60};
    // if (CheckCollisionPointRec(GetMousePosition(), quitButtonBounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    //     quitGame = true;
    // }
}

void GameManager::UpdateHowToPlay() {
    // UI Manager handles button clicks and changes currentScreen
}

void GameManager::UpdateInGame() {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
        currentScreen = GameScreen::PAUSE_MENU;
        return;
    }

    float deltaTime = GetFrameTime();
    gameTimer -= deltaTime;

    if (currentPhase == GamePhase::HIDING) {
        hidingPhaseElapsed += deltaTime;
        hidersRemaining = 0;
        bool playerTaggedByHider = false;

        for (auto& hider : hiders) {
            if (!hider.isTagged) {
                hider.Update(deltaTime, currentPhase, player, gameMap, hiders);
                hidersRemaining++;

                // Check if player is tagged by this hider (during seeking phase)
                if (currentPhase == GamePhase::SEEKING &&
                    hider.seekingState == HiderSeekingFSMState::ATTACKING && // Hider must be in attack mode
                    CheckCollisionCircles(player.position, PLAYER_RADIUS, hider.position, HIDER_RADIUS)) {
                    playerTaggedByHider = true;
                }
            }
        }

        // Player tagging hiders
        if (currentPhase == GamePhase::SEEKING && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            player.Update(deltaTime, gameMap, hiders);
            for (auto& hider : hiders) {
                if (player.CanTag(hider)) {
                    hider.isTagged = true;
                    // Potentially play a sound or visual effect
                }
            }
        }
        
        // Update hiders remaining after potential tags
        hidersRemaining = 0;
        for(const auto& hider : hiders) {
            if (!hider.isTagged) hidersRemaining++;
        }

        // Phase transitions
        if (currentPhase == GamePhase::HIDING && gameTimer <= 0) {
            StartSeekingPhase();
        }

        CheckWinLossConditions(playerTaggedByHider);
        return;
    }

    player.Update(deltaTime, gameMap, hiders);

    hidersRemaining = 0;
    bool playerTaggedByHider = false;

    for (auto& hider : hiders) {
        if (!hider.isTagged) {
            hider.Update(deltaTime, currentPhase, player, gameMap, hiders);
            hidersRemaining++;

            // Check if player is tagged by this hider (during seeking phase)
            if (currentPhase == GamePhase::SEEKING &&
                hider.seekingState == HiderSeekingFSMState::ATTACKING && // Hider must be in attack mode
                CheckCollisionCircles(player.position, PLAYER_RADIUS, hider.position, HIDER_RADIUS)) {
                playerTaggedByHider = true;
            }
        }
    }

    // Player tagging hiders
    if (currentPhase == GamePhase::SEEKING && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        for (auto& hider : hiders) {
            if (player.CanTag(hider)) {
                hider.isTagged = true;
                // Potentially play a sound or visual effect
            }
        }
    }
    
    // Update hiders remaining after potential tags
    hidersRemaining = 0;
    for(const auto& hider : hiders) {
        if (!hider.isTagged) hidersRemaining++;
    }


    // Phase transitions
    if (currentPhase == GamePhase::HIDING && gameTimer <= 0) {
        StartSeekingPhase();
    }

    CheckWinLossConditions(playerTaggedByHider);
}


void GameManager::CheckWinLossConditions(bool playerGotTagged) {
    if (currentPhase == GamePhase::SEEKING) { // Only check win/loss during seeking phase
        if (hidersRemaining == 0) {
            playerWon = true;
            lastGameTime = SEEKING_PHASE_DURATION - gameTimer;
            currentScreen = GameScreen::GAME_OVER;
        } else if (gameTimer <= 0) {
            playerWon = false;
            lastGameTime = 0; // Indicates timer ran out
            currentScreen = GameScreen::GAME_OVER;
        } else if (playerGotTagged) {
            playerWon = false;
            lastGameTime = SEEKING_PHASE_DURATION - gameTimer; // Time when player was tagged
            currentScreen = GameScreen::GAME_OVER;
        }
    }
}


void GameManager::UpdatePauseMenu() {
    // UI Manager handles button clicks and sets flags (quitGame, restartGame)
    // and currentScreen.
    // If restartGame is set by UI, it will be handled in the main Update loop.
}

void GameManager::UpdateGameOver() {
    // UI Manager handles button clicks and changes currentScreen.
    // If "Play Again" leads to IN_GAME, InitGame() will be called.
}


// src/game_manager.cpp

// ... (other functions) ...

void GameManager::Draw() {
    BeginDrawing();
    ClearBackground(RAYWHITE); // Default clear, specific screens might override

    switch (currentScreen) {
        case GameScreen::MAIN_MENU:
            uiManager.DrawMainMenu(currentScreen, restartGameFlag, quitGame);
            // The commented-out logic for quitButtonBounds was here.
            // Its removal must have led to a syntax error with braces.
            // The 'break;' for this case is essential.
            break; // <--- THIS BREAK IS IMPORTANT FOR THE MAIN_MENU CASE

        case GameScreen::HOW_TO_PLAY:
            uiManager.DrawHowToPlay(currentScreen);
            break;

        case GameScreen::IN_GAME:
            DrawInGame();
            break;

        case GameScreen::PAUSE_MENU:
            DrawInGame(); // Draw game state underneath pause menu
            uiManager.DrawPauseMenu(currentScreen, quitGame, restartGameFlag);
            break;

        case GameScreen::GAME_OVER:
            uiManager.DrawGameOverScreen(currentScreen, playerWon, lastGameTime, restartGameFlag);
            break;

        // Optional: Add a default case to handle unexpected screen states,
        // which can also help with the -Wswitch warning.
        default:
            // Handle unexpected screen state, or leave empty if not needed.
            // For example, draw some error text or go to main menu.
            // DrawText("Error: Unknown Game Screen!", 10, 10, 20, RED);
            break;
    } // <--- THIS IS THE CLOSING BRACE FOR THE switch STATEMENT

    DrawFPS(SCREEN_WIDTH - 90, 10); // Always draw FPS for debugging
    EndDrawing();
} // <--- THIS IS THE CLOSING BRACE FOR THE Draw() FUNCTION


void GameManager::DrawInGame() {
    gameMap.Draw();
    player.Draw();
    if (currentPhase == GamePhase::HIDING) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);

        // Calculate elapsed time
        float elapsed = hidingPhaseElapsed;
        float remaining = HIDING_PHASE_DURATION - elapsed;

        // Message and fade logic
        const char* msg = nullptr;
        float alpha = 1.0f;
        if (elapsed < 5.0f) {
            msg = "CLOSE YOUR EYES!";
            alpha = 1.0f - (elapsed / 5.0f); // Fades out over 5 seconds
        } else {
            msg = "Hiders are hiding...";
            alpha = 1.0f - ((elapsed - 5.0f) / 5.0f); // Fades out over next 5 seconds
        }
        if (alpha < 0.0f) alpha = 0.0f;

        int fontSize = 56;
        int textWidth = MeasureText(msg, fontSize);
        int x = (SCREEN_WIDTH - textWidth) / 2;
        int y = (SCREEN_HEIGHT - fontSize) / 2;

        // Draw fading text
        DrawText(msg, x, y, fontSize, Fade(WHITE, alpha));

        // Draw timer below
        char timerText[16];
        snprintf(timerText, sizeof(timerText), "%.1f", remaining > 0 ? remaining : 0.0f);
        int timerFontSize = 36;
        int timerWidth = MeasureText(timerText, timerFontSize);
        int timerX = (SCREEN_WIDTH - timerWidth) / 2;
        int timerY = y + fontSize + 20;
        DrawText(timerText, timerX, timerY, timerFontSize, WHITE);

        return;
    }
    player.Draw();
    for (auto& hider : hiders) {
        hider.Draw();
    }
    uiManager.DrawInGameHUD(gameTimer, hidersRemaining, player.sprintValue);
}

