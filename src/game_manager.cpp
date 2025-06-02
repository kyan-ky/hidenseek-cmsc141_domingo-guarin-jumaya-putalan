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
    
    // Initialize camera
    camera = {0};
    camera.offset = {SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f}; // Center of screen
    camera.target = {0, 0}; // Will be updated to follow player
    camera.rotation = 0.0f;
    camera.zoom = 2.0f;

    // Initialize vision overlay texture
    visionOverlay = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
}

GameManager::~GameManager() {
    uiManager.UnloadAssets();
    gameMap.Unload();
    UnloadRenderTexture(visionOverlay);
}

void GameManager::ResetGameValues() {
    player.rotation = 0.0f;
    player.showAlert = false;

    // Define possible corner spawn points with padding
    float padding = PLAYER_RADIUS + 50.0f; // Same padding as before
    std::vector<Vector2> cornerSpawnPoints = {
        {padding, padding},                                      // Top-left
        {SCREEN_WIDTH - padding, padding},                     // Top-right
        {padding, SCREEN_HEIGHT - padding},                    // Bottom-left
        {SCREEN_WIDTH - padding, SCREEN_HEIGHT - padding}      // Bottom-right
    };

    Vector2 playerSpawnPos = {0, 0};
    bool spawnPosFound = false;
    int attempts = 0;
    const int maxAttempts = 10; // Try up to 10 times to find a valid corner

    // Randomly select a corner and check if it's valid
    while (!spawnPosFound && attempts < maxAttempts) {
        int cornerIndex = rand() % cornerSpawnPoints.size();
        Vector2 potentialPos = cornerSpawnPoints[cornerIndex];

        if (gameMap.IsPositionValid(potentialPos, PLAYER_RADIUS)) {
            playerSpawnPos = potentialPos;
            spawnPosFound = true;
        }
        attempts++;
    }

    // Fallback if no valid corner found (shouldn't happen with reasonable maps)
    if (!spawnPosFound) {
         TraceLog(LOG_WARNING, "GAME: Could not find a valid corner spawn point after %d attempts. Spawning at default top-left.", maxAttempts);
         playerSpawnPos = {padding, padding}; // Default to top-left if no valid corner found
         // Note: This default position might also be invalid if the entire top-left is blocked
         // A more robust fallback might be needed for very complex maps.
    }

    player.Init(playerSpawnPos); // Initialize player at the selected valid position

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

    // Handle game restart from any screen that can trigger it
    if (this->restartGameFlag) {
        InitGame();
        this->restartGameFlag = false; // CRITICAL: Reset the flag after use
        this->currentScreen = GameScreen::IN_GAME; // Ensure we go to game screen
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

    // Update camera to follow player
    camera.target = player.position;

    if (currentPhase == GamePhase::HIDING) {
        hidingPhaseElapsed += deltaTime;

        // Option to skip hiding phase for debugging/testing
        if (IsKeyPressed(KEY_SPACE)) {
            TraceLog(LOG_INFO, "GAME: Skipping hiding phase.");
            StartSeekingPhase();
            return; // Exit update early to prevent further hiding phase logic this frame
        }

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
    ClearBackground(BLACK); 

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

    // Draw game elements with camera
    BeginMode2D(camera);
    gameMap.Draw();
    player.Draw();
    for (auto& hider : hiders) {
        hider.Draw();
    }
  
    // 1. Draw game elements with camera
    BeginMode2D(camera);
        // Draw base map and walls first
        gameMap.DrawBaseAndWalls();
        
        // Draw hiders before the object texture so they appear behind hiding spots
        for (auto& hider : hiders) {
            if (!hider.isTagged) {
                hider.Draw();
            }
        }
        
        // Draw object texture (hiding spots) on top of hiders, with transparency based on player position
        gameMap.DrawObjects(player.position);
        
        // Draw player last so it's always on top
        player.Draw();
    EndMode2D();


    // Draw the black overlay with vision cone
    Vector2 screenPos = GetWorldToScreen2D(player.position, camera);
    float radius = PLAYER_VISION_RADIUS * camera.zoom;
    float coneAngle = 60.0f; // Angle of the vision cone in degrees

    // Create the vision overlay
    BeginTextureMode(visionOverlay);
        ClearBackground(BLACK);  // Start with black background
        
        // Draw the dark overlay
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, 0.95f));
        
        // Cut out the vision circle using BLEND_SUBTRACT_COLORS
        BeginBlendMode(BLEND_SUBTRACT_COLORS);
            DrawCircleV(screenPos, radius - 200, WHITE);  // Use WHITE to cut out the circle
        EndBlendMode();
    EndTextureMode();

    // Draw the final overlay
    BeginBlendMode(BLEND_ALPHA);
        DrawTextureRec(
            visionOverlay.texture,
            (Rectangle){ 0, 0, (float)SCREEN_WIDTH, -(float)SCREEN_HEIGHT },  // Flip Y
            (Vector2){ 0, 0 },
            WHITE
        );
    EndBlendMode();

    // Draw UI elements in screen space
    uiManager.DrawInGameHUD(gameTimer, hidersRemaining, player.sprintValue);
}