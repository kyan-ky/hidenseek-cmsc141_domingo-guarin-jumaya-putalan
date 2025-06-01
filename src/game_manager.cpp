#include "game_manager.h"
#include "constants.h"
#include "raymath.h"
#include <cstdlib>   // For srand, rand
#include <ctime>     // For time for srand
#include <algorithm> // For std::all_of
#include <cstdio>    // For snprintf

GameManager::GameManager() : currentScreen(GameScreen::MAIN_MENU), currentPhase(GamePhase::HIDING),
                             gameTimer(0.0f), hidersRemaining(0), playerWon(false),
                             quitGame(false), restartGameFlag(false), lastGameTime(0.0f), hidingPhaseElapsed(0.0f) {
    srand((unsigned int)time(NULL));
    uiManager.LoadAssets();
    gameMap.Load();
}

GameManager::~GameManager() {
    uiManager.UnloadAssets();
    gameMap.Unload();
}

void GameManager::ResetGameValues() {
    player.Init({SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f});
    player.rotation = 0.0f;
    player.showAlert = false;

    hiders.assign(NUM_HIDERS, Hider());
    std::vector<Vector2> startingPositions;
    for (int i = 0; i < NUM_HIDERS; ++i) {
        Vector2 pos;
        bool positionOk;
        int attempts = 0;
        do {
            positionOk = true;
            pos = {(float)(rand() % (SCREEN_WIDTH - 100) + 50), (float)(rand() % (SCREEN_HEIGHT - 100) + 50)};
            if (Vector2DistanceSqr(pos, player.position) < (PLAYER_RADIUS + HIDER_RADIUS + 50) * (PLAYER_RADIUS + HIDER_RADIUS + 50)) {
                positionOk = false;
                continue;
            }
            for(size_t j=0; j < startingPositions.size(); ++j) {
                if (Vector2DistanceSqr(pos, startingPositions[j]) < (HIDER_RADIUS * 4) * (HIDER_RADIUS * 4) ) {
                    positionOk = false;
                    break;
                }
            }
            attempts++;
        } while(!positionOk && attempts < 50);
        startingPositions.push_back(pos);
        hiders[i].Init(pos, gameMap); // Init should set FSM states and isTagged
        // Explicitly ensure these are reset if Init doesn't cover them fully for a *new game* scenario
        hiders[i].isTagged = false; 
        hiders[i].hidingState = HiderHidingFSMState::SCOUTING; 
        hiders[i].seekingState = HiderSeekingFSMState::IDLING;
    }

    hidersRemaining = NUM_HIDERS;
    playerWon = false;
    StartHidingPhase();         // Sets gameTimer, currentPhase, and hider FSM states
    hidingPhaseElapsed = 0.0f;  // << CRITICAL: Reset this for the "Close Your Eyes" screen
    lastGameTime = 0.0f;
    // restartGameFlag is reset after use in Update()
}

void GameManager::InitGame() {
    // TraceLog(LOG_INFO, "GAME: InitGame() Called."); // DEBUG
    ResetGameValues();
}

void GameManager::StartHidingPhase() {
    currentPhase = GamePhase::HIDING;
    gameTimer = HIDING_PHASE_DURATION; // This is the total duration for hiding AFTER "close eyes"
    hidingPhaseElapsed = 0.0f;         // Reset for the "close eyes" part specifically
    // TraceLog(LOG_INFO, "GAME: StartHidingPhase - Timer: %.2f, Elapsed: %.2f", gameTimer, hidingPhaseElapsed); // DEBUG
    for (auto& hider : hiders) {
        if (!hider.isTagged) { // Should always be true at the start of a new game
            hider.hidingState = HiderHidingFSMState::SCOUTING;
        }
    }
}

void GameManager::StartSeekingPhase() {
    currentPhase = GamePhase::SEEKING;
    gameTimer = SEEKING_PHASE_DURATION; // Timer for the actual seeking
    // TraceLog(LOG_INFO, "GAME: StartSeekingPhase - Timer: %.2f", gameTimer); // DEBUG
    for (auto& hider : hiders) {
        if (!hider.isTagged) {
            hider.seekingState = HiderSeekingFSMState::IDLING;
        }
    }
}

void GameManager::Update() {
    //float deltaTime = GetFrameTime();
    GameScreen screenAtFrameStart = this->currentScreen; // Capture screen state BEFORE UI might change it in Draw()

    // The Update... methods below are for ongoing logic for a screen,
    // NOT for handling button clicks that change screens. Those clicks happen
    // in UIManager::Draw... methods, which are called in GameManager::Draw().
    // The screen change is then detected in the *next* frame of this Update() function.
    switch (this->currentScreen) {
        case GameScreen::MAIN_MENU:
            UpdateMainMenu(); // Currently empty
            break;
        case GameScreen::HOW_TO_PLAY:
            UpdateHowToPlay(); // Currently empty
            break;
        case GameScreen::IN_GAME:
            UpdateInGame(); // Contains core game logic
            break;
        case GameScreen::PAUSE_MENU:
            UpdatePauseMenu(); // Currently empty
            break;
        case GameScreen::GAME_OVER:
            UpdateGameOver(); // Currently empty
            break;
        default:
            break;
    }

    if (this->currentScreen == GameScreen::IN_GAME && this->restartGameFlag) {
        InitGame();
        this->restartGameFlag = false; // CRITICAL: Reset the flag after use
    } 
    else if (screenAtFrameStart == GameScreen::PAUSE_MENU && this->currentScreen == GameScreen::IN_GAME && !this->restartGameFlag) {
    }
}

void GameManager::UpdateMainMenu() { /* Stub */ }
void GameManager::UpdateHowToPlay() { /* Stub */ }
void GameManager::UpdatePauseMenu() { /* Stub */ }
void GameManager::UpdateGameOver() { /* Stub */ }

void GameManager::UpdateInGame() {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
        currentScreen = GameScreen::PAUSE_MENU;
        // TraceLog(LOG_INFO, "GAME: Paused. CurrentScreen: %d", (int)currentScreen); // DEBUG
        return;
    }

    float deltaTime = GetFrameTime();

    if (currentPhase == GamePhase::HIDING) {
        hidingPhaseElapsed += deltaTime;

        // Hiders find spots during the entire HIDING_PHASE_DURATION
        for (auto& hider : hiders) {
            if (!hider.isTagged) {
                hider.Update(deltaTime, currentPhase, player, gameMap, hiders);
            }
        }

        // The "Close your eyes" visual part uses hidingPhaseElapsed (drawn in DrawInGame)
        // The actual gameTimer for hiding starts counting down *after* the visual part conceptually,
        // or rather, it represents the time hiders have *left* to hide.
        // Let's make the gameTimer decrement during this phase too.
        gameTimer -= deltaTime;

        if (gameTimer <= 0) { // Hiding time is up (after "close eyes")
            StartSeekingPhase();
        }
        // No win/loss checks during HIDING phase typically
        return; // Player does not update/move during HIDING phase
    }

    // --- SEEKING PHASE ---
    if (currentPhase == GamePhase::SEEKING) {
        gameTimer -= deltaTime;
        player.Update(deltaTime, gameMap, hiders); // Player can move and act

        hidersRemaining = 0;
        bool playerTaggedByHider = false;

        for (auto& hider : hiders) {
            if (!hider.isTagged) {
                hider.Update(deltaTime, currentPhase, player, gameMap, hiders); // Hiders evade/attack
                hidersRemaining++;

                if (hider.seekingState == HiderSeekingFSMState::ATTACKING &&
                    CheckCollisionCircles(player.position, PLAYER_RADIUS, hider.position, HIDER_RADIUS)) {
                    playerTaggedByHider = true;
                }
            }
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            for (auto& hider : hiders) {
                if (player.CanTag(hider)) {
                    hider.isTagged = true;
                }
            }
        }
        
        hidersRemaining = 0; // Recalculate after potential tags
        for(const auto& hider : hiders) {
            if (!hider.isTagged) hidersRemaining++;
        }

        CheckWinLossConditions(playerTaggedByHider);
    }
}

void GameManager::CheckWinLossConditions(bool playerGotTagged) {
    // Only check during SEEKING, and only if not already Game Over
    if (currentPhase == GamePhase::SEEKING && currentScreen != GameScreen::GAME_OVER) { 
        if (hidersRemaining == 0) {
            playerWon = true;
            lastGameTime = SEEKING_PHASE_DURATION - gameTimer;
            currentScreen = GameScreen::GAME_OVER;
            // TraceLog(LOG_INFO, "GAME: Player WON. All hiders tagged."); // DEBUG
        } else if (gameTimer <= 0) {
            playerWon = false;
            lastGameTime = 0; 
            currentScreen = GameScreen::GAME_OVER;
            // TraceLog(LOG_INFO, "GAME: Player LOST. Timer expired."); // DEBUG
        } else if (playerGotTagged) {
            playerWon = false;
            lastGameTime = SEEKING_PHASE_DURATION - gameTimer;
            currentScreen = GameScreen::GAME_OVER;
            // TraceLog(LOG_INFO, "GAME: Player LOST. Tagged by hider."); // DEBUG
        }
    }
}

void GameManager::Draw() {
    BeginDrawing();
    ClearBackground(RAYWHITE); 

    switch (currentScreen) {
        case GameScreen::MAIN_MENU:
            uiManager.DrawMainMenu(currentScreen, this->quitGame, this->restartGameFlag);
            break;
        case GameScreen::HOW_TO_PLAY:
            uiManager.DrawHowToPlay(currentScreen);
            break;
        case GameScreen::IN_GAME:
            DrawInGame();
            break;
        case GameScreen::PAUSE_MENU:
            DrawInGame(); // Draw game state underneath
            uiManager.DrawPauseMenu(currentScreen, this->quitGame, this->restartGameFlag);
            break;
        case GameScreen::GAME_OVER:
            uiManager.DrawGameOverScreen(currentScreen, playerWon, lastGameTime, this->restartGameFlag);
            break;
        default:
            break;
    } 
    DrawFPS(SCREEN_WIDTH - 90, 10);
    EndDrawing();
}

void GameManager::DrawInGame() {
    // --- HIDING PHASE VISUALS ("Close Your Eyes" screen) ---
    if (currentPhase == GamePhase::HIDING && hidingPhaseElapsed < HIDING_PHASE_DURATION) { // Show during entire hiding phase duration
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);

        float visualCountdownDuration = 5.0f; // How long "Close your eyes" shows prominently
        float hidingMessageDuration = HIDING_PHASE_DURATION - visualCountdownDuration; // How long "Hiders are hiding" shows

        const char* msg = nullptr;
        float alpha = 1.0f;
        float displayTimeRemaining = HIDING_PHASE_DURATION - hidingPhaseElapsed;

        if (hidingPhaseElapsed < visualCountdownDuration) {
            msg = "CLOSE YOUR EYES!";
            // Optional: Could make alpha fade in/out for this message too
        } else if (hidingPhaseElapsed < HIDING_PHASE_DURATION) {
            msg = "Hiders are hiding...";
            // Optional: Fade this message in/out
        } else {
            msg = "GET READY!"; // Brief message before seeking starts
        }
        
        if (msg) { // Only draw if msg is set
            int fontSize = 56;
            int textWidth = MeasureText(msg, fontSize);
            DrawText(msg, (SCREEN_WIDTH - textWidth) / 2, SCREEN_HEIGHT / 2 - fontSize, fontSize, Fade(WHITE, alpha));
        }

        char timerText[16];
        snprintf(timerText, sizeof(timerText), "%.1f", displayTimeRemaining > 0 ? displayTimeRemaining : 0.0f);
        int timerFontSize = 36;
        int timerWidth = MeasureText(timerText, timerFontSize);
        DrawText(timerText, (SCREEN_WIDTH - timerWidth) / 2, SCREEN_HEIGHT / 2 + 20, timerFontSize, WHITE);

        return; // Don't draw the game world during this "Close your eyes" visual
    }

    // --- SEEKING PHASE or if HIDING_PHASE_DURATION is over but phase hasn't switched ---
    gameMap.Draw();
    // Player should be drawn on top of map, hiders potentially between map and foreground objects later
    // For now, simple draw order:
    for (auto& hider : hiders) { // Draw hiders first so player can be on top
        if (!hider.isTagged) { // Only draw non-tagged hiders, or draw tagged ones differently
            hider.Draw();
        } else {
            // Optionally draw tagged hiders differently (e.g., faded)
            // For now, let's just not draw them or draw them faded
            // hider.DrawFaded(); // You'd need to implement this
        }
    }
    player.Draw(); // Player drawn on top of hiders and map
    
    // HUD is only for seeking phase
    if (currentPhase == GamePhase::SEEKING) {
        uiManager.DrawInGameHUD(gameTimer, hidersRemaining, player.sprintValue);
    }
}
