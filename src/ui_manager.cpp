#include "ui_manager.h"
#include "raymath.h" // For Vector2 functions used in button logic if any complex one is needed

UIManager::UIManager() {
    // Load font (optional)
    if (FileExists("font.ttf")) {
        gameFont = LoadFont("font.ttf");
    } else {
        gameFont = GetFontDefault(); // Use Raylib's default font
    }
    // Initialize textures to 0
    titleBg = {0};
    howToPlayBg = {0};
    gameOverBg = {0};
}

void UIManager::LoadAssets() {
    // Load background textures
    if (FileExists("title_screen_bg.jpg")) titleBg = LoadTexture("title_screen_bg.jpg");
    if (FileExists("how_to_play_bg.jpg")) howToPlayBg = LoadTexture("how_to_play_bg.jpg");
    if (FileExists("game_over_bg.jpg")) gameOverBg = LoadTexture("game_over_bg.jpg");
}

void UIManager::UnloadAssets() {
    if (titleBg.id > 0) UnloadTexture(titleBg);
    if (howToPlayBg.id > 0) UnloadTexture(howToPlayBg);
    if (gameOverBg.id > 0) UnloadTexture(gameOverBg);
    if (gameFont.texture.id != GetFontDefault().texture.id) UnloadFont(gameFont); // Unload only if custom font was loaded
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
    float textWidth = MeasureTextEx(gameFont, text, (float)fontSize, 1).x;
    DrawTextEx(gameFont, text,
               {bounds.x + (bounds.width - textWidth) / 2, bounds.y + (bounds.height - fontSize) / 2},
               (float)fontSize, 1, textColor);
    return clicked;
}

void UIManager::DrawMainMenu(GameScreen& currentScreen) {
    if (titleBg.id > 0) DrawTexture(titleBg, 0, 0, WHITE);
    else ClearBackground(DARKGRAY);

    const char* title = GAME_TITLE;
    float titleWidth = MeasureTextEx(gameFont, title, (float)FONT_SIZE_LARGE, 1).x;
    DrawTextEx(gameFont, title, {(SCREEN_WIDTH - titleWidth) / 2, SCREEN_HEIGHT * 0.2f}, (float)FONT_SIZE_LARGE, 1, TEXT_COLOR);

    Rectangle playButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.5f, 300, 60};
    if (DrawButton(playButton, "Start Game", FONT_SIZE_MEDIUM, BUTTON_COLOR, BUTTON_HOVER_COLOR, TEXT_COLOR)) {
        currentScreen = GameScreen::IN_GAME;
    }

    Rectangle howToPlayButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.5f + 80, 300, 60};
    if (DrawButton(howToPlayButton, "How to Play", FONT_SIZE_MEDIUM, BUTTON_COLOR, BUTTON_HOVER_COLOR, TEXT_COLOR)) {
        currentScreen = GameScreen::HOW_TO_PLAY;
    }

    Rectangle quitButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.5f + 160, 300, 60};
     if (DrawButton(quitButton, "Quit", FONT_SIZE_MEDIUM, BUTTON_COLOR, BUTTON_HOVER_COLOR, TEXT_COLOR)) {
        // This should be handled by GameManager to set quitGame flag
        // For now, it's a visual button. GameManager will check currentScreen.
         // For simplicity, assume game manager handles this state.
    }
}

void UIManager::DrawHowToPlay(GameScreen& currentScreen) {
    if (howToPlayBg.id > 0) DrawTexture(howToPlayBg, 0, 0, WHITE);
    else ClearBackground(DARKBLUE); // Or make it an overlay

    // Draw as an overlay example
    // DrawRectangle(50, 50, SCREEN_WIDTH - 100, SCREEN_HEIGHT - 100, Fade(BLACK, 0.8f));
    // DrawRectangleLines(50, 50, SCREEN_WIDTH - 100, SCREEN_HEIGHT - 100, WHITE);


    const char* title = "How to Play";
    float titleWidth = MeasureTextEx(gameFont, title, (float)FONT_SIZE_MEDIUM,1).x;
    DrawTextEx(gameFont, title, {(SCREEN_WIDTH - titleWidth) / 2, 100}, (float)FONT_SIZE_MEDIUM, 1, TEXT_COLOR);

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
        DrawTextEx(gameFont, line, {100, yPos}, (float)FONT_SIZE_SMALL, 1, TEXT_COLOR);
        yPos += 30;
    }

    Rectangle backButton = {SCREEN_WIDTH / 2.0f - 100, SCREEN_HEIGHT - 120.0f, 200, 50};
    if (DrawButton(backButton, "Back", FONT_SIZE_SMALL, BUTTON_COLOR, BUTTON_HOVER_COLOR, TEXT_COLOR)) {
        currentScreen = GameScreen::MAIN_MENU;
    }
}

void UIManager::DrawInGameHUD(float timer, int hidersLeft, float sprintValue) {
    // Timer
    DrawTextEx(gameFont, TextFormat("Time: %02d:%02d", (int)timer / 60, (int)timer % 60),
               {20, 20}, (float)FONT_SIZE_MEDIUM, 1, TEXT_COLOR);

    // Hider Count
    DrawTextEx(gameFont, TextFormat("Hiders Left: %d", hidersLeft),
               {SCREEN_WIDTH - 250.0f, 20}, (float)FONT_SIZE_MEDIUM, 1, TEXT_COLOR);

    // Sprint Bar
    float sprintBarWidth = 200;
    float sprintBarHeight = 20;
    DrawRectangle(20, SCREEN_HEIGHT - 40.0f, (int)sprintBarWidth, (int)sprintBarHeight, DARKGRAY);
    DrawRectangle(20, SCREEN_HEIGHT - 40.0f, (int)(sprintBarWidth * (sprintValue / SPRINT_MAX)), (int)sprintBarHeight, SKYBLUE);
    DrawRectangleLines(20, (int)(SCREEN_HEIGHT - 40.0f), (int)sprintBarWidth, (int)sprintBarHeight, LIGHTGRAY);
    DrawTextEx(gameFont, "Sprint", {25 + sprintBarWidth, SCREEN_HEIGHT - 40.0f}, (float)FONT_SIZE_SMALL, 1, TEXT_COLOR);
}

void UIManager::DrawPauseMenu(GameScreen& currentScreen, bool& quitGame, bool& restartGame) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, OVERLAY_COLOR); // Transparent overlay

    const char* pauseText = "PAUSED";
    float textWidth = MeasureTextEx(gameFont, pauseText, FONT_SIZE_LARGE, 1).x;
    DrawTextEx(gameFont, pauseText, {(SCREEN_WIDTH - textWidth) / 2, SCREEN_HEIGHT * 0.25f}, FONT_SIZE_LARGE, 1, TEXT_COLOR);

    Rectangle resumeButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.4f, 300, 60};
    if (DrawButton(resumeButton, "Resume", FONT_SIZE_MEDIUM, BUTTON_COLOR, BUTTON_HOVER_COLOR, TEXT_COLOR)) {
        currentScreen = GameScreen::IN_GAME;
    }

    Rectangle restartButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.4f + 80, 300, 60};
    if (DrawButton(restartButton, "Start Over", FONT_SIZE_MEDIUM, BUTTON_COLOR, BUTTON_HOVER_COLOR, TEXT_COLOR)) {
        restartGame = true; // GameManager will handle this
        currentScreen = GameScreen::IN_GAME; // Will be reset by InitGame
    }

    Rectangle mainMenuButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.4f + 160, 300, 60};
    if (DrawButton(mainMenuButton, "Main Menu", FONT_SIZE_MEDIUM, BUTTON_COLOR, BUTTON_HOVER_COLOR, TEXT_COLOR)) {
        currentScreen = GameScreen::MAIN_MENU;
    }

    Rectangle quitPauseButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.4f + 240, 300, 60};
    if (DrawButton(quitPauseButton, "Quit Game", FONT_SIZE_MEDIUM, BUTTON_COLOR, BUTTON_HOVER_COLOR, TEXT_COLOR)) {
        quitGame = true; // GameManager will handle this
    }
}

void UIManager::DrawGameOverScreen(GameScreen& currentScreen, bool playerWon, float finalTime) {
    if (gameOverBg.id > 0) DrawTexture(gameOverBg, 0, 0, WHITE);
    else ClearBackground(playerWon ? DARKGREEN : MAROON);

    const char* gameOverText = playerWon ? "YOU WIN!" : "GAME OVER";
    float textWidth = MeasureTextEx(gameFont, gameOverText, FONT_SIZE_LARGE, 1).x;
    DrawTextEx(gameFont, gameOverText, {(SCREEN_WIDTH - textWidth) / 2, SCREEN_HEIGHT * 0.3f}, FONT_SIZE_LARGE, 1, TEXT_COLOR);

    const char* reasonText;
    if (playerWon) {
        reasonText = TextFormat("All hiders tagged in %02d:%02d!", (int)finalTime / 60, (int)finalTime % 60);
    } else {
        if (finalTime <= 0.1f) { // Timer expired if finalTime is close to 0 (or negative if set that way)
             reasonText = "Time's up!";
        } else { // otherwise, player was tagged
             reasonText = "You were tagged by a Hider!";
        }
    }
    float reasonWidth = MeasureTextEx(gameFont, reasonText, FONT_SIZE_MEDIUM, 1).x;
    DrawTextEx(gameFont, reasonText, {(SCREEN_WIDTH - reasonWidth) / 2, SCREEN_HEIGHT * 0.45f}, FONT_SIZE_MEDIUM, 1, TEXT_COLOR);


    Rectangle mainMenuButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.6f, 300, 60};
    if (DrawButton(mainMenuButton, "Main Menu", FONT_SIZE_MEDIUM, BUTTON_COLOR, BUTTON_HOVER_COLOR, TEXT_COLOR)) {
        currentScreen = GameScreen::MAIN_MENU;
    }

    Rectangle playAgainButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.6f + 80, 300, 60};
    if (DrawButton(playAgainButton, "Play Again", FONT_SIZE_MEDIUM, BUTTON_COLOR, BUTTON_HOVER_COLOR, TEXT_COLOR)) {
        // GameManager should set a flag to re-initialize the game
        // For now, this button will transition, GameManager will see currentScreen change
        currentScreen = GameScreen::IN_GAME; // This signals to GameManager to re-init
    }
}

