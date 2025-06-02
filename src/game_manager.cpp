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

    // UIManager needs a reference to GameManager to play sounds.
    uiManager.SetGameManager(this); 

    LoadAudioAssets(); // Load audio assets
    
    uiManager.LoadAssets(); // Your existing UI asset loading
    gameMap.Load();       // Your existing map loading
    
    // Your existing camera initialization
    camera = {0};
    camera.offset = {SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f}; 
    // Initialize camera.target after player is initialized, or to a default
    // For now, let's set it to player's typical start, will be updated in ResetGameValues/Init
    camera.target = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f}; 
    camera.rotation = 0.0f;
    camera.zoom = 1.5f; // Or 2.0f as you had

    // Your existing vision overlay texture initialization
    visionOverlay = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

    PlayMainMenuMusic(); // Start playing main menu music
}

GameManager::~GameManager() {
    StopMainMenuMusic();     // For audio
    UnloadAudioAssets();    // For audio

    uiManager.UnloadAssets(); // Your existing
    gameMap.Unload();       // Your existing
    UnloadRenderTexture(visionOverlay); // Your existing
}

void GameManager::LoadAudioAssets() {
    if (FileExists("main_menu.mp3")) { 
        mainMenuMusic = LoadMusicStream("main_menu.mp3");
        if (mainMenuMusic.ctxData != nullptr) {
            SetMusicVolume(mainMenuMusic, 0.5f); 
            mainMenuMusic.looping = true;        
        } else {
            TraceLog(LOG_WARNING, "AUDIO: Failed to load main menu music from 'resources/main_menu.mp3'");
        }
    } else {
        TraceLog(LOG_WARNING, "AUDIO: 'resources/main_menu.mp3' not found.");
    }

    if (FileExists("button_click.mp3")) { 
        sfxButtonClick = LoadSound("button_click.mp3");
        if (sfxButtonClick.frameCount == 0) {
            TraceLog(LOG_WARNING, "AUDIO: Failed to load button click sound from 'resources/button_click.mp3'");
        }
    } else {
        TraceLog(LOG_WARNING, "AUDIO: 'resources/button_click.mp3' not found.");
    }

    if (FileExists("countdown.mp3")) { 
        countdownMusic = LoadMusicStream("countdown.mp3");
        if (countdownMusic.ctxData != nullptr) {
            SetMusicVolume(countdownMusic, 0.6f); 
            countdownMusic.looping = false;     
        } else {
            TraceLog(LOG_WARNING, "AUDIO: Failed to load countdown music.");
        }
    } else {
        TraceLog(LOG_WARNING, "AUDIO: countdown.mp3 not found.");
    }

    if (FileExists("ingame.mp3")) { 
        inGameSeekingMusic = LoadMusicStream("ingame.mp3");
        if (inGameSeekingMusic.ctxData != nullptr) {
            SetMusicVolume(inGameSeekingMusic, 0.4f); 
            inGameSeekingMusic.looping = true;  
        } else {
            TraceLog(LOG_WARNING, "AUDIO: Failed to load in-game seeking music.");
        }
    } else {
        TraceLog(LOG_WARNING, "AUDIO: ingame.mp3 not found.");
    }

    if (FileExists("tag.mp3")) { 
        sfxTag = LoadSound("tag.mp3");
        if (sfxTag.frameCount == 0) TraceLog(LOG_WARNING, "AUDIO: Failed to load tag.mp3");
    } else { TraceLog(LOG_WARNING, "AUDIO: tag.mp3 not found."); }

    if (FileExists("game_over.mp3")) { 
        gameOverMusic = LoadMusicStream("game_over.mp3");
        if (gameOverMusic.ctxData != nullptr) {
            SetMusicVolume(gameOverMusic, 0.5f); 
            gameOverMusic.looping = false;
        } else {
            TraceLog(LOG_WARNING, "AUDIO: Failed to load game_over.mp3 as Music.");
        }
    } else {
        TraceLog(LOG_WARNING, "AUDIO: game_over.mp3 not found.");
    }

    if (FileExists("victory.mp3")) { 
        gameWinMusic = LoadMusicStream("victory.mp3");
        if (gameWinMusic.ctxData != nullptr) {
            SetMusicVolume(gameOverMusic, 0.5f); 
            gameWinMusic.looping = false;
        } else {
            TraceLog(LOG_WARNING, "AUDIO: Failed to load victory.mp3 as Music.");
        }
    } else {
        TraceLog(LOG_WARNING, "AUDIO: victory.mp3 not found.");
    }


}

void GameManager::UnloadAudioAssets() {
    if (mainMenuMusic.ctxData != nullptr) UnloadMusicStream(mainMenuMusic);
    if (sfxButtonClick.frameCount > 0) UnloadSound(sfxButtonClick);
    if (countdownMusic.ctxData != nullptr) UnloadMusicStream(countdownMusic);
    if (inGameSeekingMusic.ctxData != nullptr) UnloadMusicStream(inGameSeekingMusic);
    if (sfxTag.frameCount > 0) UnloadSound(sfxTag);
    if (gameOverMusic.ctxData != nullptr) UnloadMusicStream(gameOverMusic);
    if (gameWinMusic.ctxData != nullptr) UnloadMusicStream(gameWinMusic);
}

void GameManager::PlayMainMenuMusic() {
    if (mainMenuMusic.ctxData != nullptr) {
        if (!IsMusicStreamPlaying(mainMenuMusic)) {
            PlayMusicStream(mainMenuMusic);
        }
        SeekMusicStream(mainMenuMusic, 0); 
    }
}

void GameManager::StopMainMenuMusic() {
    if (mainMenuMusic.ctxData != nullptr && IsMusicStreamPlaying(mainMenuMusic)) {
        StopMusicStream(mainMenuMusic);
    }
}

void GameManager::PlayButtonSound() {
    if (sfxButtonClick.frameCount > 0) {
        PlaySound(sfxButtonClick);
    }
}

void GameManager::ResetGameValues() {
    float padding = PLAYER_RADIUS + 50.0f;
    std::vector<Vector2> cornerSpawnPoints = {
        {padding, padding}, {SCREEN_WIDTH - padding, padding},
        {padding, SCREEN_HEIGHT - padding}, {SCREEN_WIDTH - padding, SCREEN_HEIGHT - padding}
    };
    Vector2 playerSpawnPos = {padding, padding}; 
    bool spawnPosFound = false; int attempts = 0; const int maxAttempts = 10;
    while (!spawnPosFound && attempts < maxAttempts) {
        int cornerIndex = rand() % cornerSpawnPoints.size();
        Vector2 potentialPos = cornerSpawnPoints[cornerIndex];
        if (gameMap.IsPositionValid(potentialPos, PLAYER_RADIUS)) {
            playerSpawnPos = potentialPos; spawnPosFound = true; }
        attempts++;
    }
    if (!spawnPosFound) TraceLog(LOG_WARNING, "GAME: Could not find valid corner spawn. Defaulting.");
    player.Init(playerSpawnPos);
    player.rotation = 0.0f;
    player.showAlert = false;
    camera.target = player.position; 

    hiders.assign(NUM_HIDERS, Hider());
    std::vector<Vector2> startingPositions;
    for (int i = 0; i < NUM_HIDERS; ++i) {
        Vector2 pos; bool positionOk; int hiderAttempts = 0; const int maxHiderAttempts = 100;
        do {
            positionOk = true;
            pos = {(float)(rand() % (SCREEN_WIDTH - 200) + 100), (float)(rand() % (SCREEN_HEIGHT - 200) + 100)};
            hiderAttempts++;
            if (Vector2DistanceSqr(pos, player.position) < (PLAYER_RADIUS + HIDER_RADIUS + 50) * (PLAYER_RADIUS + HIDER_RADIUS + 50)) {
                positionOk = false; continue; }
            for(size_t j=0; j < startingPositions.size(); ++j) {
                if (Vector2DistanceSqr(pos, startingPositions[j]) < (HIDER_RADIUS * 4) * (HIDER_RADIUS * 4) ) {
                    positionOk = false; break; }
            }
            if (!positionOk) continue;
            if (!gameMap.IsPositionValid(pos, HIDER_RADIUS)) positionOk = false;
        } while(!positionOk && hiderAttempts < maxHiderAttempts);
        if (hiderAttempts >= maxHiderAttempts) TraceLog(LOG_WARNING, "GAME: Could not find valid spawn for hider %d.", i);
        startingPositions.push_back(pos);
        hiders[i].Init(pos, gameMap);
        hiders[i].isTagged = false; 
        hiders[i].hidingState = HiderHidingFSMState::SCOUTING;
        hiders[i].seekingState = HiderSeekingFSMState::IDLING;
    }

    hidersRemaining = NUM_HIDERS; 
    playerWon = false;
    hidingPhaseElapsed = 0.0f; 
    lastGameTime = 0.0f;
    StartHidingPhase(); 
    TraceLog(LOG_INFO, "RESET_VALUES: Finished. currentScreen: %d, currentPhase: %d, hidersRemaining: %d, hidingPhaseElapsed: %.2f", 
         (int)currentScreen, (int)currentPhase, hidersRemaining, hidingPhaseElapsed);
}

void GameManager::InitGame() {
    TraceLog(LOG_INFO, "INIT_GAME: Called.");
    // Stop all potentially active music streams BEFORE resetting values and starting new phase music
    if (IsMusicStreamPlaying(mainMenuMusic)) { TraceLog(LOG_INFO, "INIT_GAME: Stopping mainMenuMusic."); StopMusicStream(mainMenuMusic); }
    if (IsMusicStreamPlaying(inGameSeekingMusic)) { TraceLog(LOG_INFO, "INIT_GAME: Stopping inGameSeekingMusic."); StopMusicStream(inGameSeekingMusic); }
    if (IsMusicStreamPlaying(countdownMusic)) { TraceLog(LOG_INFO, "INIT_GAME: Stopping countdownMusic (precaution)."); StopMusicStream(countdownMusic); }

    ResetGameValues(); // This will call StartHidingPhase, which handles starting countdownMusic
    TraceLog(LOG_INFO, "INIT_GAME: Finished. currentScreen (should be IN_GAME due to UIManager action): %d, currentPhase: %d", (int)currentScreen, (int)currentPhase);
}

void GameManager::StartHidingPhase() {
    currentPhase = GamePhase::HIDING;
    gameTimer = HIDING_PHASE_DURATION; 
    hidingPhaseElapsed = 0.0f;   
    TraceLog(LOG_INFO, "START_HIDING_PHASE: Set phase to HIDING. Timer: %.2f", gameTimer);

    // Explicitly stop other music streams that should not play during countdown
    if (IsMusicStreamPlaying(mainMenuMusic)) { TraceLog(LOG_INFO, "START_HIDING_PHASE: Ensuring mainMenuMusic is stopped."); StopMusicStream(mainMenuMusic); }
    if (IsMusicStreamPlaying(inGameSeekingMusic)) { TraceLog(LOG_INFO, "START_HIDING_PHASE: Ensuring inGameSeekingMusic is stopped."); StopMusicStream(inGameSeekingMusic); }

    // Start countdown music
    if (countdownMusic.ctxData != nullptr) {
        PlayMusicStream(countdownMusic);
        SeekMusicStream(countdownMusic, 0); // Start from the beginning
        TraceLog(LOG_INFO, "START_HIDING_PHASE: Playing countdownMusic.");
    } else {
        TraceLog(LOG_WARNING, "START_HIDING_PHASE: countdownMusic not loaded, cannot play.");
    }
    
    for (auto& hider : hiders) { /* ... set hider.hidingState ... */ }
}

void GameManager::StartSeekingPhase() {
    currentPhase = GamePhase::SEEKING;
    gameTimer = SEEKING_PHASE_DURATION; 

    if (countdownMusic.ctxData != nullptr && IsMusicStreamPlaying(countdownMusic)) {
        StopMusicStream(countdownMusic); 
    }

    if (inGameSeekingMusic.ctxData != nullptr) { 
        PlayMusicStream(inGameSeekingMusic);
        SeekMusicStream(inGameSeekingMusic, 0); 
    }
    
    for (auto& hider : hiders) {
        if (!hider.isTagged) {
            hider.seekingState = HiderSeekingFSMState::IDLING;
        }
    }
}


// In src/game_manager.cpp

void GameManager::Update() {
    GameScreen screenAtFrameStart = this->currentScreen; 
    // GamePhase phaseAtFrameStart = this->currentPhase; // For more detailed debugging if needed

    // --- Update All Active Music Streams ---
    if (mainMenuMusic.ctxData != nullptr && IsMusicStreamPlaying(mainMenuMusic)) UpdateMusicStream(mainMenuMusic);
    if (countdownMusic.ctxData != nullptr && IsMusicStreamPlaying(countdownMusic)) UpdateMusicStream(countdownMusic);
    if (inGameSeekingMusic.ctxData != nullptr && IsMusicStreamPlaying(inGameSeekingMusic)) UpdateMusicStream(inGameSeekingMusic);
    if (gameOverMusic.ctxData != nullptr && IsMusicStreamPlaying(gameOverMusic)) UpdateMusicStream(gameOverMusic);
    if (gameWinMusic.ctxData != nullptr && IsMusicStreamPlaying(gameWinMusic)) UpdateMusicStream(gameWinMusic);

    // --- Screen-Specific Update Logic ---
    switch (this->currentScreen) {
        case GameScreen::MAIN_MENU: UpdateMainMenu(); break;
        case GameScreen::HOW_TO_PLAY: UpdateHowToPlay(); break;
        case GameScreen::IN_GAME: UpdateInGame(); break; 
        case GameScreen::PAUSE_MENU: UpdatePauseMenu(); break;
        case GameScreen::GAME_OVER: UpdateGameOver(); break;
        default: break;
    }

    // --- Handle Game Restart Logic ---
    bool gameWasJustRestartedThisFrame = false; // Changed name for clarity
    if (this->currentScreen == GameScreen::IN_GAME && this->restartGameFlag) {
        InitGame(); // InitGame stops all music streams & then starts countdownMusic via Reset/StartHidingPhase    
        this->restartGameFlag = false; 
        gameWasJustRestartedThisFrame = true; 
    } 

    // --- Handle Music Based on Screen Transitions ---
    if (screenAtFrameStart != this->currentScreen || gameWasJustRestartedThisFrame) {
        // --- Stop music from the PREVIOUS screen, or music that shouldn't play on the NEW screen ---
        if (screenAtFrameStart == GameScreen::GAME_OVER) {
            if (IsMusicStreamPlaying(gameOverMusic)) StopMusicStream(gameOverMusic);
            if (IsMusicStreamPlaying(gameWinMusic)) StopMusicStream(gameWinMusic);
        }
        if (screenAtFrameStart == GameScreen::IN_GAME && this->currentScreen != GameScreen::PAUSE_MENU && this->currentScreen != GameScreen::IN_GAME /* Resuming */) {
            // Stop game-related music if we are leaving IN_GAME for something other than PAUSE
            if (IsMusicStreamPlaying(countdownMusic)) StopMusicStream(countdownMusic);
            if (IsMusicStreamPlaying(inGameSeekingMusic)) StopMusicStream(inGameSeekingMusic);
        }
        if ((screenAtFrameStart == GameScreen::MAIN_MENU || screenAtFrameStart == GameScreen::HOW_TO_PLAY) && 
            !(this->currentScreen == GameScreen::MAIN_MENU || this->currentScreen == GameScreen::HOW_TO_PLAY || this->currentScreen == GameScreen::PAUSE_MENU)) {
            // Stop main menu music if leaving main menu/how_to_play for something other than pause
            if (IsMusicStreamPlaying(mainMenuMusic)) StopMusicStream(mainMenuMusic);
        }
        // If pausing, music is handled below

        // --- Start/Resume music for the NEW screen ---
        if (this->currentScreen == GameScreen::MAIN_MENU || this->currentScreen == GameScreen::HOW_TO_PLAY) {
            PlayMainMenuMusic(); // This also seeks to 0
        } else if (this->currentScreen == GameScreen::IN_GAME) {
            if (gameWasJustRestartedThisFrame) {
                // countdownMusic was already started by InitGame -> StartHidingPhase
            } else if (screenAtFrameStart == GameScreen::PAUSE_MENU) { // Resuming from pause
                if (currentPhase == GamePhase::SEEKING && inGameSeekingMusic.ctxData != nullptr) {
                    if(!IsMusicStreamPlaying(inGameSeekingMusic)) SeekMusicStream(inGameSeekingMusic, GetMusicTimePlayed(inGameSeekingMusic)); // Ensure correct position if stopped instead of paused
                    ResumeMusicStream(inGameSeekingMusic);
                } else if (currentPhase == GamePhase::HIDING && countdownMusic.ctxData != nullptr) {
                    if(!IsMusicStreamPlaying(countdownMusic)) SeekMusicStream(countdownMusic, GetMusicTimePlayed(countdownMusic));
                    ResumeMusicStream(countdownMusic); 
                }
            }
            // If transitioning from MAIN_MENU to IN_GAME, InitGame (triggered by restartGameFlag) handles starting countdownMusic.
        } else if (this->currentScreen == GameScreen::GAME_OVER) {
            // Game outcome music (win/loss streams) should have been started in CheckWinLossConditions.
            // This block ensures it plays if it wasn't already (e.g., if CheckWinLossConditions only set screen)
            if (playerWon && gameWinMusic.ctxData != nullptr && !IsMusicStreamPlaying(gameWinMusic)) {
                 PlayMusicStream(gameWinMusic); 
                 SeekMusicStream(gameWinMusic, 0);
            } else if (!playerWon && gameOverMusic.ctxData != nullptr && !IsMusicStreamPlaying(gameOverMusic)) {
                 PlayMusicStream(gameOverMusic);
                 SeekMusicStream(gameOverMusic,0);
            }
        } 
    }
    
    // Specific logic for entering/exiting PAUSE_MENU
    if (screenAtFrameStart != GameScreen::PAUSE_MENU && this->currentScreen == GameScreen::PAUSE_MENU) { // Just entered pause
        if (IsMusicStreamPlaying(countdownMusic)) PauseMusicStream(countdownMusic);
        if (IsMusicStreamPlaying(inGameSeekingMusic)) PauseMusicStream(inGameSeekingMusic);
        // If mainMenuMusic was playing (e.g., from How To Play), decide whether to pause it or let it continue.
        // if (IsMusicStreamPlaying(mainMenuMusic)) PauseMusicStream(mainMenuMusic);
    }
    // Note: Resuming from PAUSE_MENU is handled in the `screenActuallyChangedThisFrame` block above.
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
    camera.target = player.position; // Keep camera following player

    if (currentPhase == GamePhase::HIDING) {
        hidingPhaseElapsed += deltaTime;
        if (IsKeyPressed(KEY_SPACE)) { StartSeekingPhase(); return; } // Debug skip
        for (auto& hider : hiders) {
            if (!hider.isTagged) {
                hider.Update(deltaTime, currentPhase, player, gameMap, hiders);
            }
        }
        gameTimer -= deltaTime;
        if (gameTimer <= 0) { StartSeekingPhase(); }
        return; 
    }

    if (currentPhase == GamePhase::SEEKING) {
        gameTimer -= deltaTime;
        player.Update(deltaTime, gameMap, hiders); 
        hidersRemaining = 0;
        bool playerTaggedByHider = false;
        for (auto& hider : hiders) {
            if (!hider.isTagged) {
                hider.Update(deltaTime, currentPhase, player, gameMap, hiders); 
                hidersRemaining++; 
                if (hider.seekingState == HiderSeekingFSMState::ATTACKING &&
                    CheckCollisionCircles(player.position, PLAYER_RADIUS, hider.position, HIDER_RADIUS)) {
                    playerTaggedByHider = true;
                    if (sfxTag.frameCount > 0) PlaySound(sfxTag);
                }
            }
        }


        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            for (auto& hider : hiders) {
                if (player.CanTag(hider)) { 
                    if (!hider.isTagged) { 
                        hider.isTagged = true;
                        if (sfxTag.frameCount > 0) PlaySound(sfxTag);
                    }
                }
            }
        }

        hidersRemaining = 0; 
        for(const auto& hider : hiders) { if (!hider.isTagged) hidersRemaining++; }
        CheckWinLossConditions(playerTaggedByHider);
    }
}

void GameManager::CheckWinLossConditions(bool playerGotTagged) {
    if (currentPhase == GamePhase::SEEKING && currentScreen != GameScreen::GAME_OVER) { 
        bool gameOverTriggeredThisFrame = false;
        if (hidersRemaining == 0) {
            if (!playerWon && currentScreen != GameScreen::GAME_OVER) { 
                playerWon = true;
                lastGameTime = SEEKING_PHASE_DURATION - gameTimer;
                gameOverTriggeredThisFrame = true;
            }
        }
        
        else if (gameTimer <= 0 || playerGotTagged) {
            if (currentScreen != GameScreen::GAME_OVER) { 
                playerWon = false;
                if (gameTimer <= 0) lastGameTime = 0; 
                else lastGameTime = SEEKING_PHASE_DURATION - gameTimer;
                gameOverTriggeredThisFrame = true;
            }
        }

        if (gameOverTriggeredThisFrame) {
            currentScreen = GameScreen::GAME_OVER;
            if (IsMusicStreamPlaying(countdownMusic)) StopMusicStream(countdownMusic);
            if (IsMusicStreamPlaying(inGameSeekingMusic)) StopMusicStream(inGameSeekingMusic);
            if (IsMusicStreamPlaying(mainMenuMusic)) StopMusicStream(mainMenuMusic); // Stop main menu too
                currentScreen = GameScreen::GAME_OVER;
            
            if (playerWon && gameWinMusic.ctxData != nullptr) {
                PlayMusicStream(gameWinMusic);
                SeekMusicStream(gameWinMusic, 0);
            } else if (!playerWon && gameOverMusic.ctxData != nullptr) { // Assuming gameOverMusic for loss
                PlayMusicStream(gameOverMusic);
                SeekMusicStream(gameOverMusic, 0);
            }
        }
    }
}

void GameManager::Draw() {
    BeginDrawing();
    ClearBackground(BLACK); // Your chosen clear color

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
            DrawInGame(); 
            uiManager.DrawPauseMenu(currentScreen, this->quitGame, this->restartGameFlag);
            break;
        case GameScreen::GAME_OVER:
            uiManager.DrawGameOverScreen(currentScreen, playerWon, lastGameTime, this->restartGameFlag);
            break;
        default: break;
    } 
    // DrawFPS(SCREEN_WIDTH - 90, 10); // Keep commented if you don't want FPS
    EndDrawing();
}

void GameManager::DrawInGame() {
    // Hiding Phase Countdown (your existing code for this)
    if (currentPhase == GamePhase::HIDING && hidingPhaseElapsed < HIDING_PHASE_DURATION) { 
        ClearBackground(BLACK);
        // ... (all your countdown drawing logic: msg, timer, pulsing, colors) ...
        // Example for one message part:
        float time = GetTime(); float displayTimeRemaining = HIDING_PHASE_DURATION - hidingPhaseElapsed;
        float stage1Duration = 4.0f; float stage3Duration = 2.0f; 
        const char* msg = nullptr; int msgFontSize = 70; Color msgColor = WHITE; float msgPulseScale = 1.0f;
        Font messageFont = (uiManager.titleTextFont.texture.id != 0) ? uiManager.titleTextFont : GetFontDefault();
        Font timerDetailFont = (uiManager.bodyTextFont.texture.id != 0) ? uiManager.bodyTextFont : GetFontDefault();
        if (hidingPhaseElapsed < stage1Duration) {
            msg = "CLOSE YOUR EYES!"; msgColor = GetColor(0xAF3800FF); msgFontSize = 80;
            msgPulseScale = 1.0f + 0.05f * sinf(time * 3.0f); 
        } else if (hidingPhaseElapsed < (HIDING_PHASE_DURATION - stage3Duration)) {
            msg = "Hiders are hiding..."; msgColor = GetColor(0xEDEAD0FF); msgFontSize = 60;
        } else if (hidingPhaseElapsed < HIDING_PHASE_DURATION) {
            msg = "GET READY!"; msgColor = GetColor(0xFFCF56FF); msgFontSize = 90;
            msgPulseScale = 1.0f + 0.1f * fabsf(sinf(time * 6.0f)); 
            if (hidingPhaseElapsed >= (HIDING_PHASE_DURATION - stage3Duration) && hidingPhaseElapsed < (HIDING_PHASE_DURATION - stage3Duration + 0.1f)) { 
                DrawRectangle(0,0,SCREEN_WIDTH, SCREEN_HEIGHT, Fade(WHITE, 0.3f)); }
        }
        if (msg) { 
            float actualMsgFontSize = msgFontSize * msgPulseScale;
            Vector2 textSize = MeasureTextEx(messageFont, msg, actualMsgFontSize, 1);
            DrawTextEx(messageFont, msg, {(SCREEN_WIDTH - textSize.x) / 2, SCREEN_HEIGHT * 0.4f - textSize.y / 2}, actualMsgFontSize, 1, msgColor);
        }
        char timerText[16]; snprintf(timerText, sizeof(timerText), "%.1f", displayTimeRemaining > 0 ? displayTimeRemaining : 0.0f);
        int timerFontSize = 70; Color timerColor = GetColor(0xEDEAD0FF); float timerPulseScale = 1.0f;
        if (displayTimeRemaining <= 3.5f && displayTimeRemaining > 0) { 
            timerColor = GetColor(0xFFCF56FF);
            timerPulseScale = 1.0f + 0.08f * fabsf(sinf(time * 5.0f * 2.0f)); 
            if (displayTimeRemaining <= 1.5f) timerColor = GetColor(0xAF3800FF);
        }
        float actualTimerFontSize = timerFontSize * timerPulseScale;
        Vector2 timerTextSize = MeasureTextEx(timerDetailFont, timerText, actualTimerFontSize, 1); 
        DrawTextEx(timerDetailFont, timerText, {(SCREEN_WIDTH - timerTextSize.x) / 2, SCREEN_HEIGHT * 0.6f - timerTextSize.y / 2}, actualTimerFontSize, 1, timerColor);
        return; 
    }
  
    // Seeking Phase Drawing
    BeginMode2D(camera);
        gameMap.Draw();
        for (auto& hider : hiders) {
            if (!hider.isTagged) {
                 hider.Draw();
            }
        }
        player.Draw();
    EndMode2D();
  
    // Vision Overlay (Your existing logic)
    Vector2 screenPos = GetWorldToScreen2D(player.position, camera);
    float radius = PLAYER_VISION_RADIUS * camera.zoom;
    // float coneAngle = 60.0f; // Not used in current circle overlay
    BeginTextureMode(visionOverlay);
        ClearBackground(BLACK); // For the RT itself
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, 0.95f));
        BeginBlendMode(BLEND_SUBTRACT_COLORS); // This makes WHITE "cut"
            DrawCircleV(screenPos, radius - 140, WHITE); // Adjust radius - 140 as needed
        EndBlendMode();
    EndTextureMode();
    BeginBlendMode(BLEND_ALPHA); // Draw the overlay with its alpha
        DrawTextureRec( visionOverlay.texture,
            (Rectangle){ 0, 0, (float)visionOverlay.texture.width, -(float)visionOverlay.texture.height },
            (Vector2){ 0, 0 }, WHITE );
    EndBlendMode();

    // HUD
    if (currentPhase == GamePhase::SEEKING) { // Only draw HUD during seeking
        uiManager.DrawInGameHUD(gameTimer, hidersRemaining, player.sprintValue);
    }
}
