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

    // Initialize music
    hidingPhaseMusic = {0};
    seekingPhaseMusic = {0};
    victorySound = {0};
    gameOverSound = {0};

    // Load phase music
    if (FileExists("countdown.mp3")) {
        hidingPhaseMusic = LoadMusicStream("countdown.mp3");
        SetMusicVolume(hidingPhaseMusic, 0.5f);
    }
    if (FileExists("ingame.mp3")) {
        seekingPhaseMusic = LoadMusicStream("ingame.mp3");
        SetMusicVolume(seekingPhaseMusic, 0.5f);
    }
    // Load sound effects
    if (FileExists("victory.mp3")) {
        victorySound = LoadSound("victory.mp3");
        SetSoundVolume(victorySound, 0.7f);
    }
    if (FileExists("game_over.mp3")) {
        gameOverSound = LoadSound("game_over.mp3");
        SetSoundVolume(gameOverSound, 0.7f);
    }
}

GameManager::~GameManager() {
    uiManager.UnloadAssets();
    gameMap.Unload();
    UnloadRenderTexture(visionOverlay);
    if (hidingPhaseMusic.stream.buffer != NULL) UnloadMusicStream(hidingPhaseMusic);
    if (seekingPhaseMusic.stream.buffer != NULL) UnloadMusicStream(seekingPhaseMusic);
    if (victorySound.frameCount > 0) UnloadSound(victorySound);
    if (gameOverSound.frameCount > 0) UnloadSound(gameOverSound);
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
        const int maxSpawnAttempts = 100; // Limit attempts to prevent infinite loops
        do {
            positionOk = true;
            pos = {(float)(rand() % (SCREEN_WIDTH - 200) + 100), (float)(rand() % (SCREEN_HEIGHT - 200) + 100)}; // Generate position away from edges
            attempts++;

            // Check distance from player
            if (Vector2DistanceSqr(pos, player.position) < (PLAYER_RADIUS + HIDER_RADIUS + 50) * (PLAYER_RADIUS + HIDER_RADIUS + 50)) {
                positionOk = false;
                continue;
            }

            // Check distance from already assigned hider starting positions
            for(const auto& existingPos : startingPositions) {
                if (Vector2DistanceSqr(pos, existingPos) < (HIDER_RADIUS * 4) * (HIDER_RADIUS * 4)) {
                    positionOk = false;
                    break;
                }
            }
            if (!positionOk) continue;

            // Check validity against map obstacles
            if (!gameMap.IsPositionValid(pos, HIDER_RADIUS)) {
                positionOk = false;
            }

        } while(!positionOk && attempts < maxSpawnAttempts);

        if (attempts >= maxSpawnAttempts) {
            TraceLog(LOG_WARNING, "GAME: Could not find a valid spawn position for hider %d after %d attempts.", i, maxSpawnAttempts);
            // Fallback: place at a default spot or allow potentially invalid spot (depending on desired game behavior)
            // For now, we'll just use the last attempted position, which might be invalid.
            // A better approach might be to place them near the player's start or a known valid spot.
        }

        startingPositions.push_back(pos);
        hiders[i].Init(pos, gameMap, i); // Pass the hider ID (0-4) to Init
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
    gameTimer = HIDING_PHASE_DURATION;
    hidingPhaseElapsed = 0.0f;

    // Stop seeking phase music if playing
    if (seekingPhaseMusic.stream.buffer != NULL) {
        StopMusicStream(seekingPhaseMusic);
    }

    // Start hiding phase music
    if (hidingPhaseMusic.stream.buffer != NULL) {
        PlayMusicStream(hidingPhaseMusic);
    }

    for (auto& hider : hiders) {
        if (!hider.isTagged) {
            hider.hidingState = HiderHidingFSMState::SCOUTING;
        }
    }
}

void GameManager::StartSeekingPhase() {
    currentPhase = GamePhase::SEEKING;
    gameTimer = SEEKING_PHASE_DURATION;

    // Stop hiding phase music if playing
    if (hidingPhaseMusic.stream.buffer != NULL) {
        StopMusicStream(hidingPhaseMusic);
    }

    // Start seeking phase music
    if (seekingPhaseMusic.stream.buffer != NULL) {
        PlayMusicStream(seekingPhaseMusic);
    }

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
        // Stop any playing game over or victory sounds
        if (gameOverSound.frameCount > 0) {
            StopSound(gameOverSound);
        }
        if (victorySound.frameCount > 0) {
            StopSound(victorySound);
        }
        InitGame();
        this->restartGameFlag = false; // CRITICAL: Reset the flag after use
        this->currentScreen = GameScreen::IN_GAME; // Ensure we go to game screen
    }

    // Stop all sounds when transitioning to main menu
    if (this->currentScreen == GameScreen::MAIN_MENU) {
        // Stop phase music
        if (hidingPhaseMusic.stream.buffer != NULL) {
            StopMusicStream(hidingPhaseMusic);
        }
        if (seekingPhaseMusic.stream.buffer != NULL) {
            StopMusicStream(seekingPhaseMusic);
        }
        // Stop game over and victory sounds
        if (gameOverSound.frameCount > 0) {
            StopSound(gameOverSound);
        }
        if (victorySound.frameCount > 0) {
            StopSound(victorySound);
        }
    }
}

void GameManager::UpdateMainMenu() { /* Stub */ }
void GameManager::UpdateHowToPlay() { /* Stub */ }
void GameManager::UpdatePauseMenu() { /* Stub */ }
void GameManager::UpdateGameOver() { /* Stub */ }

void GameManager::UpdateInGame() {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
        currentScreen = GameScreen::PAUSE_MENU;
        return;
    }

    float deltaTime = GetFrameTime();

    // Update camera to follow player
    camera.target = player.position;

    // Update current phase music
    if (currentPhase == GamePhase::HIDING && hidingPhaseMusic.stream.buffer != NULL) {
        UpdateMusicStream(hidingPhaseMusic);
    } else if (currentPhase == GamePhase::SEEKING && seekingPhaseMusic.stream.buffer != NULL) {
        UpdateMusicStream(seekingPhaseMusic);
    }

    if (currentPhase == GamePhase::HIDING) {
        hidingPhaseElapsed += deltaTime;

        // Option to skip hiding phase for debugging/testing
        if (IsKeyPressed(KEY_SPACE)) {
            StartSeekingPhase();
            return;
        }

        // Hiders find spots during the entire HIDING_PHASE_DURATION
        for (auto& hider : hiders) {
            if (!hider.isTagged) {
                hider.Update(deltaTime, currentPhase, player, gameMap, hiders);
            }
        }

        gameTimer -= deltaTime;

        if (gameTimer <= 0) {
            StartSeekingPhase();
        }
        return;
    }

    // --- SEEKING PHASE ---
    if (currentPhase == GamePhase::SEEKING) {
        gameTimer -= deltaTime;
        player.Update(deltaTime, gameMap, hiders);

        hidersRemaining = 0;
        bool playerTaggedByHider = false;

        for (auto& hider : hiders) {
            if (!hider.isTagged) {
                hider.Update(deltaTime, currentPhase, player, gameMap, hiders);
                hidersRemaining++;

                if (hider.seekingState == HiderSeekingFSMState::ATTACKING) {
                    float distanceToPlayer = Vector2Distance(player.position, hider.position);
                    float collisionDistance = HIDER_RADIUS + PLAYER_RADIUS;
                    if (distanceToPlayer <= collisionDistance) {
                        playerTaggedByHider = true;
                    }
                }
            }
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_ENTER)) {
            bool taggedAnyHider = false;
            for (auto& hider : hiders) {
                if (player.CanTag(hider)) {
                    hider.isTagged = true;
                    taggedAnyHider = true;
                }
            }
            // Play tag sound if we successfully tagged at least one hider
            if (taggedAnyHider && player.tagSound.frameCount > 0) {
                PlaySound(player.tagSound);
            }
        }
        
        hidersRemaining = 0;
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
            // Stop current music
            if (seekingPhaseMusic.stream.buffer != NULL) {
                StopMusicStream(seekingPhaseMusic);
            }
            // Play victory sound
            if (victorySound.frameCount > 0) {
                PlaySound(victorySound);
            }
            // TraceLog(LOG_INFO, "GAME: Player WON. All hiders tagged."); // DEBUG
        } else if (gameTimer <= 0) {
            playerWon = false;
            lastGameTime = 0; 
            currentScreen = GameScreen::GAME_OVER;
            // Stop current music
            if (seekingPhaseMusic.stream.buffer != NULL) {
                StopMusicStream(seekingPhaseMusic);
            }
            // Play game over sound
            if (gameOverSound.frameCount > 0) {
                PlaySound(gameOverSound);
            }
            // TraceLog(LOG_INFO, "GAME: Player LOST. Timer expired."); // DEBUG
        } else if (playerGotTagged) {
            playerWon = false;
            lastGameTime = SEEKING_PHASE_DURATION - gameTimer;
            currentScreen = GameScreen::GAME_OVER;
            // Stop current music
            if (seekingPhaseMusic.stream.buffer != NULL) {
                StopMusicStream(seekingPhaseMusic);
            }
            // Play game over sound
            if (gameOverSound.frameCount > 0) {
                PlaySound(gameOverSound);
            }
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
    //DrawFPS(SCREEN_WIDTH - 90, 10);
    EndDrawing();
}

// In src/game_manager.cpp

void GameManager::DrawInGame() {
    // --- HIDING PHASE VISUALS ("Countdown" screen) ---
    if (currentPhase == GamePhase::HIDING && hidingPhaseElapsed < 10.0f) {
        ClearBackground(BLACK); // Start with a black screen

        float time = GetTime(); 
        float displayTimeRemaining = 10.0f - hidingPhaseElapsed;

        // Define Stages for Messages
        float stage1Duration = 4.0f; 
        float stage3Duration = 2.0f; 

        const char* msg = nullptr;
        int msgFontSize = 70; 
        Color msgColor = WHITE;
        float msgPulseScale = 1.0f;

        Font messageFont = (uiManager.titleTextFont.texture.id != 0) ? uiManager.titleTextFont : GetFontDefault();
        Font timerDetailFont = (uiManager.bodyTextFont.texture.id != 0) ? uiManager.bodyTextFont : GetFontDefault();

        if (hidingPhaseElapsed < stage1Duration) {
            msg = "CLOSE YOUR EYES!";
            msgColor = GetColor(0xAF3800FF);
            msgFontSize = 80;
            float pulseSpeed = 3.0f;
            msgPulseScale = 1.0f + 0.05f * sinf(time * pulseSpeed); 
        } else if (hidingPhaseElapsed < (10.0f - stage3Duration)) {
            msg = "Hiders are hiding...";
            msgColor = GetColor(0xEDEAD0FF);
            msgFontSize = 60;
        } else if (hidingPhaseElapsed < 10.0f) {
            msg = "GET READY!";
            msgColor = GetColor(0xFFCF56FF);
            msgFontSize = 90;
            float pulseSpeed = 6.0f;
            msgPulseScale = 1.0f + 0.1f * fabsf(sinf(time * pulseSpeed)); 

            if (hidingPhaseElapsed >= (10.0f - stage3Duration) && 
                hidingPhaseElapsed < (10.0f - stage3Duration + 0.1f)) { 
                DrawRectangle(0,0,SCREEN_WIDTH, SCREEN_HEIGHT, Fade(WHITE, 0.3f));
            }
        }

        // Draw the main message
        if (msg) { 
            float actualMsgFontSize = msgFontSize * msgPulseScale;
            Vector2 textSize = MeasureTextEx(messageFont, msg, actualMsgFontSize, 1);
            DrawTextEx(messageFont, msg, 
                       {(SCREEN_WIDTH - textSize.x) / 2, SCREEN_HEIGHT * 0.4f - textSize.y / 2}, 
                       actualMsgFontSize, 1, msgColor);
        }

        // Draw Timer
        char timerText[16];
        snprintf(timerText, sizeof(timerText), "%.1f", displayTimeRemaining > 0 ? displayTimeRemaining : 0.0f);
        
        int timerFontSize = 70;
        Color timerColor = GetColor(0xEDEAD0FF);
        float timerPulseScale = 1.0f;

        if (displayTimeRemaining <= 3.5f && displayTimeRemaining > 0) { 
            timerColor = GetColor(0xFFCF56FF);
            float pulseSpeed = 5.0f;
            timerPulseScale = 1.0f + 0.08f * fabsf(sinf(time * pulseSpeed * 2.0f)); 
             if (displayTimeRemaining <= 1.5f) timerColor = GetColor(0xAF3800FF);
        }
        
        float actualTimerFontSize = timerFontSize * timerPulseScale;
        Vector2 timerTextSize = MeasureTextEx(timerDetailFont, timerText, actualTimerFontSize, 1); 
        DrawTextEx(timerDetailFont, timerText, 
                   {(SCREEN_WIDTH - timerTextSize.x) / 2, SCREEN_HEIGHT * 0.6f - timerTextSize.y / 2}, 
                   actualTimerFontSize, 1, timerColor);
    } else {
        // Draw game elements with camera
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
}