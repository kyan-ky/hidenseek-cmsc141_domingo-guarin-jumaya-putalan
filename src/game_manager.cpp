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
    player.Init({PLAYER_RADIUS + 50.0f, PLAYER_RADIUS + 50.0f}); // Seeker starts at top left with some padding

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

// In src/game_manager.cpp

void GameManager::DrawInGame() {
    // --- HIDING PHASE VISUALS ("Countdown" screen) ---
    if (currentPhase == GamePhase::HIDING && hidingPhaseElapsed < HIDING_PHASE_DURATION) {
        ClearBackground(BLACK); // Start with a black screen

        float time = GetTime(); 
        float displayTimeRemaining = HIDING_PHASE_DURATION - hidingPhaseElapsed;

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
        } else if (hidingPhaseElapsed < (HIDING_PHASE_DURATION - stage3Duration)) {
            msg = "Hiders are hiding...";
            msgColor = GetColor(0xEDEAD0FF);
            msgFontSize = 60;
        } else if (hidingPhaseElapsed < HIDING_PHASE_DURATION) {
            msg = "GET READY!";
            msgColor = GetColor(0xFFCF56FF);
            msgFontSize = 90;
            float pulseSpeed = 6.0f;
            msgPulseScale = 1.0f + 0.1f * fabsf(sinf(time * pulseSpeed)); 

            if (hidingPhaseElapsed >= (HIDING_PHASE_DURATION - stage3Duration) && 
                hidingPhaseElapsed < (HIDING_PHASE_DURATION - stage3Duration + 0.1f)) { 
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

        return; // Don't draw the game world or vision overlay during this countdown
    }

    // --- SEEKING PHASE or if HIDING_PHASE_DURATION is over ---

    // 1. Draw game elements with camera
    BeginMode2D(camera); // Assuming 'camera' is a member of GameManager or accessible
        gameMap.Draw();
        // Draw hiders first so player is on top, or before foreground objects if you implement those
        for (auto& hider : hiders) {
            // Consider drawing tagged hiders differently or not at all
            if (!hider.isTagged) { // Example: only draw non-tagged
                 hider.Draw();
            }
        }
        player.Draw();
    EndMode2D();

    // 2. Update and Draw the Vision Overlay (only during seeking phase)
    if (currentPhase == GamePhase::SEEKING) { // Only apply vision overlay in seeking phase
        Vector2 screenPos = GetWorldToScreen2D(player.position, camera);
        float radius = PLAYER_VISION_RADIUS * camera.zoom; 
        // float coneAngle = PLAYER_VISION_CONE_ANGLE; // Use defined constant

        // Create the vision overlay
        BeginTextureMode(visionOverlay); // Assuming 'visionOverlay' is a RenderTexture2D member of GameManager
            ClearBackground(ColorAlpha(BLACK, 0.0f)); // Start with fully transparent for the overlay RT
            
            // Draw the dark overlay that covers everything
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, 0.85f)); // Adjust alpha for darkness
            
            // Cut out the vision circle/cone using BLEND_SUBTRACT_COLORS or BLEND_ADD_COLORS with transparency
            // For a cone, you'd draw a triangle fan. For a circle:
            BeginBlendMode(BLEND_SUBTRACT_COLORS); // This makes WHITE subtract, effectively creating transparency
                // To make the cutout area fully clear, you'd subtract BLACK from BLACK (which is black)
                // then add your clear shape. Or use BLEND_ADD_COLORS for light.
                // Let's try a simpler approach for a clear circle:
                // DrawCircleV(screenPos, radius, ColorAlpha(BLACK, 0.0f)); // Doesn't work like this
            EndBlendMode();

            // Alternative for clear circle:
            // 1. Fill overlay with dark semi-transparent
            // 2. DrawCircleV with BLEND_MASK (need a mask texture) OR draw everything *except* the circle
            // A common way for a "hole" is to use a shader, or:
            // Draw a circle with full transparency using a blend mode that overwrites alpha.
            // Or, if you are just drawing a circle of light:
            // BeginBlendMode(BLEND_ADDITIVE); // Or BLEND_ALPHA for smooth light circle
            // DrawCircleGradient(screenPos.x, screenPos.y, radius, ColorAlpha(WHITE,0.6f), ColorAlpha(WHITE,0.0f)); // Fading light circle
            // EndBlendMode();

            // Simpler "Hole Punch" using DrawCircle and specific BlendMode:
            // This is tricky without shaders. A common non-shader way is to draw the dark overlay,
            // then draw the "lit" area (game world through the hole) again on top, clipped to the circle/cone.
            // However, with RenderTexture, we can try to make a "mask":
            // Fill with solid black for the overlay effect
            // DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, 0.85f));
            // Make the circle area completely transparent in the render texture
            // This requires a blend mode that replaces alpha. BLEND_ALPHA with a ColorAlpha(anycolor, 0.0f) might work.
            BeginBlendMode(BLEND_ALPHA);
                 DrawCircleV(screenPos, radius, ColorAlpha(WHITE, 0.0f)); // Make circle area fully transparent
            EndBlendMode();


        EndTextureMode();

        // Draw the final overlay texture onto the screen
        DrawTextureRec(
            visionOverlay.texture,
            (Rectangle){ 0, 0, (float)visionOverlay.texture.width, -(float)visionOverlay.texture.height },  // Flip Y for RenderTexture
            (Vector2){ 0, 0 },
            WHITE // No additional tint needed for the overlay itself usually
        );
    }


    // 3. Draw UI elements (HUD) in screen space, on top of everything
    if (currentPhase == GamePhase::SEEKING) {
        uiManager.DrawInGameHUD(gameTimer, hidersRemaining, player.sprintValue);
    }
}
