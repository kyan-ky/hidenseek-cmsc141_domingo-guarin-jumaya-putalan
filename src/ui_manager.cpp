#include "ui_manager.h"
#include "raymath.h" // For Vector2 functions used in button logic if any complex one is needed

UIManager::UIManager() {
    // Load font (optional)
    if (FileExists("kiwi_soda.ttf")) {
        titleTextFont = LoadFont("kiwi_soda.ttf");
    } else {
        titleTextFont = GetFontDefault(); // Use Raylib's default font
    }

    if (FileExists("rainy_hearts.ttf")) {
        bodyTextFont = LoadFont("rainy_hearts.ttf");
    } else {
        bodyTextFont = GetFontDefault(); // Use Raylib's default font
    }

    // Initialize textures to 0
    titleBg = {0};
    howToPlayBg = {0};
    gameOverBg = {0};
}

void UIManager::LoadAssets() {
    // Load background textures
    if (FileExists("title_screen_bg.png")) titleBg = LoadTexture("title_screen_bg.png");
    if (FileExists("how_to_play_bg.jpg")) howToPlayBg = LoadTexture("how_to_play_bg.jpg");
    if (FileExists("game_over_bg.jpg")) gameOverBg = LoadTexture("game_over_bg.jpg");
}

void UIManager::UnloadAssets() {
    if (titleBg.id > 0) UnloadTexture(titleBg);
    if (howToPlayBg.id > 0) UnloadTexture(howToPlayBg);
    if (gameOverBg.id > 0) UnloadTexture(gameOverBg);
    if (titleTextFont.texture.id != GetFontDefault().texture.id) UnloadFont(titleTextFont); 
    if (bodyTextFont.texture.id != GetFontDefault().texture.id) UnloadFont(bodyTextFont); 

}

bool UIManager::DrawButton(Rectangle bounds, const char* text, int fontSize, Color baseColor, Color hoverColor, Color textColor) {
    bool clicked = false;
    Vector2 mousePoint = GetMousePosition();
    Color currentColor = baseColor;

    if (CheckCollisionPointRec(mousePoint, bounds)) {
        currentColor = hoverColor;
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            clicked = true;
        }
    }

    DrawRectangleRec(bounds, currentColor);
    float textWidth = MeasureTextEx(this->bodyTextFont, text, (float)fontSize, 1).x;
    DrawTextEx(this->bodyTextFont, text,
               {bounds.x + (bounds.width - textWidth) / 2, bounds.y + (bounds.height - fontSize) / 2},
               (float)fontSize, 1, textColor);
    return clicked;
}

void UIManager::DrawMainMenu(GameScreen& currentScreen) {
    if (titleBg.id > 0) DrawTexture(titleBg, 0, 0, WHITE);
    else ClearBackground(DARKGRAY);

    const char* title = GAME_TITLE;
    // Use titleTextFont, MAIN_TITLE_COLOR, MAIN_TITLE_FONT_SIZE
    float titleWidth = MeasureTextEx(titleTextFont, title, (float)MAIN_TITLE_FONT_SIZE, 1).x; // Get the width
    DrawTextEx(titleTextFont, title, 
           {(SCREEN_WIDTH - titleWidth) / 2, SCREEN_HEIGHT * 0.2f}, 
           (float)MAIN_TITLE_FONT_SIZE, 1, MAIN_TITLE_COLOR); // Use MAIN_TITLE_COLOR 
                                                            
    Rectangle playButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.5f, 300, 60};
    if (DrawButton(playButton, "Start Game", MENU_BUTTON_FONT_SIZE, 
                BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::IN_GAME;
    }

    Rectangle howToPlayButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.5f + 80, 300, 60};
    if (DrawButton(howToPlayButton, "How to Play", MENU_BUTTON_FONT_SIZE, 
                BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::HOW_TO_PLAY;
    }

    Rectangle quitButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.5f + 160, 300, 60};
    if (DrawButton(quitButton, "Quit", MENU_BUTTON_FONT_SIZE, 
                BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        // This should be handled by GameManager to set quitGame flag.
        // Example: Set a flag that GameManager checks, or call a GameManager method.
        // For instance, if GameManager had a public static bool& GetQuitFlag();
        // GameManager::GetQuitFlag() = true; 
        // Or more cleanly, DrawButton could return an enum, or the click could be checked in GameManager::UpdateMainMenu()
    }

}

void UIManager::DrawHowToPlay(GameScreen& currentScreen) {
    if (howToPlayBg.id > 0) DrawTexture(howToPlayBg, 0, 0, WHITE);
    else ClearBackground(DARKBLUE); // Or make it an overlay

    // Draw as an overlay example
    // DrawRectangle(50, 50, SCREEN_WIDTH - 100, SCREEN_HEIGHT - 100, Fade(BLACK, 0.8f));
    // DrawRectangleLines(50, 50, SCREEN_WIDTH - 100, SCREEN_HEIGHT - 100, WHITE);


    const char* title = "How to Play";
    float titleWidth = MeasureTextEx(bodyTextFont, title, (float)HOW_TO_PLAY_TITLE_FONT_SIZE,1).x;
    DrawTextEx(bodyTextFont, title, {(SCREEN_WIDTH - titleWidth) / 2, 100}, (float)HOW_TO_PLAY_TITLE_FONT_SIZE, 1, HOW_TO_PLAY_TITLE_COLOR);

    const char* instructions[] = {
        "You are the Seeker (Blue)!",
        "Goal: Tag all Hiders (Green) before the timer runs out.",
        "Controls:",
        "  W, A, S, D or Arrow Keys: Move",
        "  Left Shift: Sprint (uses sprint bar)",
        "  Mouse: Aim your vision cone",
        "  Left Mouse Button: Tag hiders in your cone",
        "",
        "Hiders are AI controlled.",
        "They will hide, and if they see you, they might evade or even try to tag YOU!",
        "If a Hider tags you, or time runs out, you lose.",
        "An alert icon (!) appears if a Hider is behind you.",
        "",
        "Good luck, Seeker!"
    };
    float yPos = 180;
    for (const char* line : instructions) {
        DrawTextEx(bodyTextFont, line, {100, yPos}, (float)HOW_TO_PLAY_BODY_FONT_SIZE, 1, HOW_TO_PLAY_BODY_COLOR);
        yPos += (float)HOW_TO_PLAY_BODY_FONT_SIZE + 8;
    }

    Rectangle backButton = {SCREEN_WIDTH / 2.0f - 100, SCREEN_HEIGHT - 120.0f, 200, 50};
    if (DrawButton(backButton, "Back", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::MAIN_MENU;
    }
}

void UIManager::DrawInGameHUD(float timer, int hidersLeft, float sprintValue) {
    Font currentHudFont = bodyTextFont; // Use hudTextFont if loaded, else bodyTextFont

    // Timer
    DrawTextEx(currentHudFont, TextFormat("Time: %02d:%02d", (int)timer / 60, (int)timer % 60),
               {20, 20}, (float)HUD_TEXT_FONT_SIZE, 1, HUD_TEXT_COLOR);

    // Hider Count
    DrawTextEx(currentHudFont, TextFormat("Hiders Left: %d", hidersLeft),
                {SCREEN_WIDTH - MeasureTextEx(currentHudFont, TextFormat("Hiders Left: %d", NUM_HIDERS), (float)HUD_TEXT_FONT_SIZE, 1).x - 20, 20}, 
               (float)HUD_TEXT_FONT_SIZE, 1, HUD_TEXT_COLOR);
    // Sprint Bar
    float sprintBarWidth = 200;
    float sprintBarHeight = 20;
    DrawRectangle(20, SCREEN_HEIGHT - 40.0f, (int)sprintBarWidth, (int)sprintBarHeight, DARKGRAY);
    DrawRectangle(20, SCREEN_HEIGHT - 40.0f, (int)(sprintBarWidth * (sprintValue / SPRINT_MAX)), (int)sprintBarHeight, SKYBLUE);
    DrawRectangleLines(20, (int)(SCREEN_HEIGHT - 40.0f), (int)sprintBarWidth, (int)sprintBarHeight, LIGHTGRAY);
    DrawTextEx(currentHudFont, "Sprint", {25 + 200.0f, SCREEN_HEIGHT - 40.0f}, (float)HUD_TEXT_FONT_SIZE * 0.7f, 1, HUD_TEXT_COLOR);
}

// In src/ui_manager.cpp

// ... (other UIManager methods) ...

void UIManager::DrawPauseMenu(GameScreen& currentScreen, bool& quitGame, bool& restartGame) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, OVERLAY_COLOR); // Transparent overlay

    const char* pauseText = "PAUSED";
    // Using titleTextFont for the main "PAUSED" message, or bodyTextFont if you prefer it to be less prominent
    Font currentPauseTitleFont = titleTextFont.texture.id != 0 ? titleTextFont : bodyTextFont;
    float textWidth = MeasureTextEx(currentPauseTitleFont, pauseText, (float)PAUSE_MENU_TITLE_FONT_SIZE, 1).x;
    DrawTextEx(currentPauseTitleFont, pauseText, 
               {(SCREEN_WIDTH - textWidth) / 2, SCREEN_HEIGHT * 0.25f}, 
               (float)PAUSE_MENU_TITLE_FONT_SIZE, 1, PAUSE_MENU_TEXT_COLOR);

    // Buttons will use bodyTextFont via DrawButton, with MENU_BUTTON_FONT_SIZE and MENU_BUTTON_TEXT_COLOR
    Rectangle resumeButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.4f, 300, 60};
    if (DrawButton(resumeButton, "Resume", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::IN_GAME;
    }

    Rectangle restartButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.4f + 80, 300, 60};
    if (DrawButton(restartButton, "Start Over", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        restartGame = true; 
        currentScreen = GameScreen::IN_GAME; 
    }

    Rectangle mainMenuButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.4f + 160, 300, 60};
    if (DrawButton(mainMenuButton, "Main Menu", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::MAIN_MENU;
    }

    Rectangle quitPauseButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.4f + 240, 300, 60};
    if (DrawButton(quitPauseButton, "Quit Game", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        quitGame = true;
    }
}

void UIManager::DrawGameOverScreen(GameScreen& currentScreen, bool playerWon, float finalTime) {
    if (gameOverBg.id > 0) DrawTexture(gameOverBg, 0, 0, WHITE);
    else ClearBackground(playerWon ? DARKGREEN : MAROON);

    const char* gameOverText = playerWon ? "YOU WIN!" : "GAME OVER";
    // Using titleTextFont for the main "YOU WIN!" / "GAME OVER" message
    Font currentGameOverTitleFont = titleTextFont.texture.id != 0 ? titleTextFont : bodyTextFont;
    Color currentGameOverTitleColor = playerWon ? GAME_OVER_WIN_COLOR : GAME_OVER_LOSS_COLOR;
    
    float textWidth = MeasureTextEx(currentGameOverTitleFont, gameOverText, (float)GAME_OVER_TITLE_FONT_SIZE, 1).x;
    DrawTextEx(currentGameOverTitleFont, gameOverText, 
               {(SCREEN_WIDTH - textWidth) / 2, SCREEN_HEIGHT * 0.3f}, 
               (float)GAME_OVER_TITLE_FONT_SIZE, 1, currentGameOverTitleColor);

    const char* reasonText;
    if (playerWon) {
        reasonText = TextFormat("All hiders tagged in %02d:%02d!", (int)finalTime / 60, (int)finalTime % 60);
    } else {
        if (finalTime <= 0.1f) { 
             reasonText = "Time's up!";
        } else { 
             reasonText = "You were tagged by a Hider!";
        }
    }
    // Using bodyTextFont for the reason text
    float reasonWidth = MeasureTextEx(bodyTextFont, reasonText, (float)GAME_OVER_REASON_FONT_SIZE, 1).x;
    DrawTextEx(bodyTextFont, reasonText, 
               {(SCREEN_WIDTH - reasonWidth) / 2, SCREEN_HEIGHT * 0.45f}, 
               (float)GAME_OVER_REASON_FONT_SIZE, 1, GAME_OVER_REASON_TEXT_COLOR);


    // Buttons will use bodyTextFont via DrawButton, with MENU_BUTTON_FONT_SIZE and MENU_BUTTON_TEXT_COLOR
    Rectangle mainMenuButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.6f, 300, 60};
    if (DrawButton(mainMenuButton, "Main Menu", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::MAIN_MENU;
    }

    Rectangle playAgainButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.6f + 80, 300, 60};
    if (DrawButton(playAgainButton, "Play Again", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::IN_GAME; 
    }
}

