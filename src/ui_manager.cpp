#include "ui_manager.h"
#include "raymath.h" // For Vector2 functions used in button logic if any complex one is needed


UIManager::UIManager() : currentInstructionPage(1) { // Initialize to page 1
    // Load Title Text Font 
    if (FileExists("kiwi_soda.ttf")) { // Your title font
        titleTextFont = LoadFont("kiwi_soda.ttf");
    } else {
        titleTextFont = GetFontDefault();
    }

    // Load Body Text Font
    if (FileExists("rainy_hearts.ttf")) { // Your body/button font
        bodyTextFont = LoadFont("rainy_hearts.ttf");
    } else {
        bodyTextFont = GetFontDefault();
    }
    
    // Initialize other textures to 0 (or load placeholders)
    titleBg = {0};
    howToPlayBg = {0};
    howToPlayInstructions1 = {0}; // Initialize
    howToPlayInstructions2 = {0}; // Initialize
    gameOverBg = {0};
}

void UIManager::LoadAssets() {
    // Load background textures
    if (FileExists("title_screen_bg.png")) titleBg = LoadTexture("title_screen_bg.png");
    if (FileExists("how_to_play_bg.png")) howToPlayBg = LoadTexture("how_to_play_bg.png");
    if (FileExists("instruction_1.png")) howToPlayInstructions1 = LoadTexture("instruction_1.png");
    if (FileExists("instruction_2.png")) howToPlayInstructions2 = LoadTexture("instruction_2.png");
    if (FileExists("game_over_bg.png")) gameOverBg = LoadTexture("game_over_bg.png"); 
}

void UIManager::UnloadAssets() {
    if (titleBg.id > 0) UnloadTexture(titleBg);
    if (howToPlayBg.id > 0) UnloadTexture(howToPlayBg);
    if (howToPlayInstructions1.id > 0) UnloadTexture(howToPlayInstructions1); 
    if (howToPlayInstructions2.id > 0) UnloadTexture(howToPlayInstructions2); 
    if (gameOverBg.id > 0) UnloadTexture(gameOverBg);
    if (titleTextFont.texture.id != GetFontDefault().texture.id) UnloadFont(titleTextFont); 
    if (bodyTextFont.texture.id != GetFontDefault().texture.id) UnloadFont(bodyTextFont); 

}

bool UIManager::DrawButton(Rectangle bounds, const char* text, int fontSize, Color baseColor, Color hoverColor, Color textColor) {
    bool clicked = false;
    Vector2 mousePoint = GetMousePosition();
    Color currentButtonFaceColor = baseColor; // The color of the button's top surface
    Color currentButtonTextColor = textColor; // The color of the text

    // --- Parameters for Appearance ---
    float roundness = 0.35f;         // 0.0 (sharp) to 0.5 (fully rounded ends if height allows)
    int segments = 12;              // Smoothness of curves
    float shadowOffset = 3.0f;      // How far the "3D" shadow extends (for pop-out effect)
    float pressDepth = 2.0f;        // How much the button face moves "in" when pressed

    Rectangle buttonFaceBounds = bounds; // This will be the rectangle for the top surface
                                         // It gets modified if the button is pressed.

    // --- Button State Logic (Hover & Press) ---
    if (CheckCollisionPointRec(mousePoint, bounds)) { // Collision check always on original bounds
        currentButtonFaceColor = hoverColor;
        // Optional: Slightly change text color on hover
        // currentButtonTextColor = ColorBrightness(textColor, 0.2f);

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            currentButtonFaceColor = ColorBrightness(hoverColor, -0.1f); // Face slightly darker when held
            
            // Offset the button face to simulate being pressed "into" the shadow
            buttonFaceBounds.x += pressDepth;
            buttonFaceBounds.y += pressDepth;
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePoint, bounds)) {
            // Click happens on release, still within the original bounds
            clicked = true;
        }
    }

    // --- Drawing the Button ---

    if (!(CheckCollisionPointRec(mousePoint, bounds) && IsMouseButtonDown(MOUSE_LEFT_BUTTON))) {
        // Only draw the "pop-out" shadow if the button is NOT currently being pressed.
        // When pressed, the button face moves to cover this area.
        Rectangle shadowBounds = {
            bounds.x + shadowOffset,
            bounds.y + shadowOffset,
            bounds.width,
            bounds.height
        };
        DrawRectangleRounded(shadowBounds, roundness, segments, ColorBrightness(baseColor, -0.5f)); // Significantly darker than base
    }

    // 2. Draw the Main Button Face (using buttonFaceBounds, which is offset if pressed)
    DrawRectangleRounded(buttonFaceBounds, roundness, segments, currentButtonFaceColor);

    // --- Text Drawing ---
    Font currentButtonFont = (this->bodyTextFont.texture.id != 0) ? this->bodyTextFont : GetFontDefault();
    float textWidth = MeasureTextEx(currentButtonFont, text, (float)fontSize, 1).x;
    
    // Text position is relative to buttonFaceBounds (which includes press offset)
    Vector2 textPosition = {
        buttonFaceBounds.x + (buttonFaceBounds.width - textWidth) / 2,
        buttonFaceBounds.y + (buttonFaceBounds.height - fontSize) / 2
    };

    // Text Enhancement: Simple 1px Outline (good for pixel fonts)
    Color textOutlineColor = Fade(BLACK, 0.5f); // Semi-transparent black for a softer outline
    int textOutlineThickness = 1; // For pixel fonts, 1 is usually good

    // Draw outline by drawing text multiple times, offset
    DrawTextEx(currentButtonFont, text, (Vector2){textPosition.x - textOutlineThickness, textPosition.y}, (float)fontSize, 1, textOutlineColor);
    DrawTextEx(currentButtonFont, text, (Vector2){textPosition.x + textOutlineThickness, textPosition.y}, (float)fontSize, 1, textOutlineColor);
    DrawTextEx(currentButtonFont, text, (Vector2){textPosition.x, textPosition.y - textOutlineThickness}, (float)fontSize, 1, textOutlineColor);
    DrawTextEx(currentButtonFont, text, (Vector2){textPosition.x, textPosition.y + textOutlineThickness}, (float)fontSize, 1, textOutlineColor);
    // Draw Main Text on top of the outline
    DrawTextEx(currentButtonFont, text, textPosition, (float)fontSize, 1, currentButtonTextColor);
    
    return clicked;
}

/* void UIManager::DrawMainMenu(GameScreen& currentScreen) {
=======
void UIManager::DrawMainMenu(GameScreen& currentScreen, bool& restartGame, bool& quitGame) {
    if (titleBg.id > 0) DrawTexture(titleBg, 0, 0, WHITE);
    else ClearBackground(DARKGRAY);

    const char* titleLine1 = "State of Fear:";
    const cahr* titleLine2 = "Ryan's Revenge";

    const char* title = GAME_TITLE;
    // Use titleTextFont, MAIN_TITLE_COLOR, MAIN_TITLE_FONT_SIZE
    float titleWidth = MeasureTextEx(titleTextFont, title, (float)MAIN_TITLE_FONT_SIZE, 1).x; // Get the width
    DrawTextEx(titleTextFont, title, 
           {(SCREEN_WIDTH - titleWidth) / 2, SCREEN_HEIGHT * 0.2f}, 
           (float)MAIN_TITLE_FONT_SIZE, 1, MAIN_TITLE_COLOR); // Use MAIN_TITLE_COLOR 
                                                            
    Rectangle playButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.5f, 300, 60};
    if (DrawButton(playButton, "Start Game", MENU_BUTTON_FONT_SIZE, 
                BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        restartGame = true;
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
        quitGame = true;
        // This should be handled by GameManager to set quitGame flag.
        // Example: Set a flag that GameManager checks, or call a GameManager method.
        // For instance, if GameManager had a public static bool& GetQuitFlag();
        // GameManager::GetQuitFlag() = true; 
        // Or more cleanly, DrawButton could return an enum, or the click could be checked in GameManager::UpdateMainMenu()
    }

} */


void UIManager::DrawMainMenu(GameScreen& currentScreen, bool& quitGameFlag, bool& wantsToStartNewGame) {
    if (titleBg.id > 0) DrawTexture(titleBg, 0, 0, WHITE);
    else ClearBackground(DARKGRAY); // Or your preferred fallback background color

    // ---- TITLE DRAWING ----
    // Define the two lines for your title
    const char* titleLine1 = "State of Fear:";
    const char* titleLine2 = "Ryan's Revenge";

    // Use the font and color you've set up for the title
    // (Assuming titleTextFont, MAIN_TITLE_FONT_SIZE, MAIN_TITLE_COLOR from constants.h)
    Font currentTitleFont = (titleTextFont.texture.id != 0) ? titleTextFont : GetFontDefault();
    float titleFontSize = (float)MAIN_TITLE_FONT_SIZE; 
    Color titleColor = MAIN_TITLE_COLOR;
    
    // Adjust this for the vertical space between the two lines of the title
    float lineSpacing = titleFontSize * 0.15f; // e.g., 15% of the font height
    float time = GetTime();
    float bobSpeed = 2.0f;    // How fast it bobs
    float bobAmount = 4.0f;   // How many pixels up/down
    float yAnimationOffset = sinf(time * bobSpeed) * bobAmount;

    // --- Draw Title Line 1 ---
    Vector2 titleLine1Size = MeasureTextEx(currentTitleFont, titleLine1, titleFontSize, 1); // Get width and height
    Vector2 titleLine1BasePos = {
        (SCREEN_WIDTH - titleLine1Size.x) / 2,  // Center horizontally
        SCREEN_HEIGHT * 0.18f                    // Adjust Y position for the first line (e.g., 18% from top)
    };
    Vector2 shadowOffset = {3, 3};
    Color shadowColor = Fade(BLACK, 0.6f);
    DrawTextEx(currentTitleFont, titleLine1, 
           {titleLine1BasePos.x + shadowOffset.x, titleLine1BasePos.y + shadowOffset.y + yAnimationOffset}, 
           titleFontSize, 1, shadowColor);
    DrawTextEx(currentTitleFont, titleLine1, {titleLine1BasePos.x, titleLine1BasePos.y + yAnimationOffset}, titleFontSize, 1, titleColor);

    // --- Draw Title Line 2 ---
    Vector2 titleLine2Size = MeasureTextEx(currentTitleFont, titleLine2, titleFontSize, 1); // Get width and height
    Vector2 titleLine2BasePos = {
        (SCREEN_WIDTH - titleLine2Size.x) / 2,   // Center horizontally
        titleLine1BasePos.y + titleLine1Size.y + lineSpacing // Position below line 1
    };
    DrawTextEx(currentTitleFont, titleLine2, 
           {titleLine2BasePos.x + shadowOffset.x, titleLine2BasePos.y + shadowOffset.y + yAnimationOffset}, 
           titleFontSize, 1, shadowColor);
    DrawTextEx(currentTitleFont, titleLine2, {titleLine2BasePos.x, titleLine2BasePos.y + yAnimationOffset}, titleFontSize, 1, titleColor);
    // ---- END OF TITLE DRAWING ----


    // --- BUTTONS ---
    // Adjust the starting Y position of the buttons if needed, based on the new title height
    float buttonsStartY = titleLine2BasePos.y + titleLine2Size.y + 70; // e.g., 70 pixels below the second title line

    Rectangle playButton = {SCREEN_WIDTH / 2.0f - 150, buttonsStartY, 300, 60};
    if (DrawButton(playButton, "Start Game", MENU_BUTTON_FONT_SIZE, 
                   BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::IN_GAME;
        wantsToStartNewGame = true;
    }

    Rectangle howToPlayButton = {SCREEN_WIDTH / 2.0f - 150, buttonsStartY + 70, 300, 60}; // 70px spacing
    if (DrawButton(howToPlayButton, "How to Play", MENU_BUTTON_FONT_SIZE, 
                   BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::HOW_TO_PLAY;
    }

    Rectangle quitButton = {SCREEN_WIDTH / 2.0f - 150, buttonsStartY + 140, 300, 60}; // 70px spacing
    if (DrawButton(quitButton, "Quit", MENU_BUTTON_FONT_SIZE, 
                   BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        // Quit logic (e.g., set a flag for GameManager to handle)
        // For example, if you have a quitGame flag in GameManager:
        quitGameFlag = true; // This would need to be accessible or handled by GameManager
    }
} 

// In src/ui_manager.cpp

void UIManager::DrawHowToPlay(GameScreen& currentScreen) {
    if (howToPlayBg.id > 0) DrawTexture(howToPlayBg, 0, 0, WHITE);
    else ClearBackground(DARKBLUE); 

    // --- "How to Play" Title Setup ---
    Font screenTitleFont = (this->titleTextFont.texture.id != 0) ? this->titleTextFont : GetFontDefault();
    const char* pageTitleText = "How to Play";
    float pageTitleFontSize = (float)HOW_TO_PLAY_SCREEN_TITLE_FONT_SIZE; // From constants.h
    Color pageTitleColor = MAIN_TITLE_COLOR;          // From constants.h

    // Calculate title size ONCE
    Vector2 pageTitleSize = MeasureTextEx(screenTitleFont, pageTitleText, pageTitleFontSize, 1);
    // float titleTextWidth = pageTitleSize.x; // Already available in pageTitleSize.x
    float titleTextHeight = pageTitleSize.y; // Use the y component for height

    // Define title Y position ONCE
    float titleYPosition = 50.0f; // Adjust this to position the "How to Play" title vertically

    // Base position for the title (used for drawing shadow and main text)
    Vector2 pageTitleBasePos = { 
        (SCREEN_WIDTH - pageTitleSize.x) / 2, 
        titleYPosition
    };

    // Drop Shadow Parameters
    Vector2 shadowOffset = {3, 3};      
    Color shadowColor = Fade(BLACK, 0.6f);

    // 1. Draw the Shadow
    DrawTextEx(screenTitleFont, pageTitleText, 
               {pageTitleBasePos.x + shadowOffset.x, pageTitleBasePos.y + shadowOffset.y}, 
               pageTitleFontSize, 1, shadowColor);

    // 2. Draw the Main Title Text
    DrawTextEx(screenTitleFont, pageTitleText, 
               pageTitleBasePos, 
               pageTitleFontSize, 1, pageTitleColor);

    // --- Determine which instruction image to show ---
    Texture2D currentInstructionImage = {0};
    if (currentInstructionPage == 1 && howToPlayInstructions1.id > 0) {
        currentInstructionImage = howToPlayInstructions1;
    } else if (currentInstructionPage == 2 && howToPlayInstructions2.id > 0) {
        currentInstructionImage = howToPlayInstructions2;
    }

    // --- Define available area and desired size for the instruction image ---
    // titleTextHeight is already defined and calculated above
    float paddingBelowTitle = -60.0f; // You had this, adjust as needed. Negative will move image upwards overlapping title if not careful.
                                     // Perhaps start with a small positive like 10.0f or 15.0f
    float imageStartY = titleYPosition + titleTextHeight + paddingBelowTitle; 
    
    float bottomMarginForButtons = 100.0f; 
    float availableHeightForImage = SCREEN_HEIGHT - imageStartY - bottomMarginForButtons; 
    float availableWidthForImage = SCREEN_WIDTH * 0.95f; 

    // ... (rest of your image drawing logic with DrawTexturePro) ...
    if (currentInstructionImage.id > 0 && currentInstructionImage.width > 0 && currentInstructionImage.height > 0) {
        float scale = 1.0f;
        if (currentInstructionImage.width * scale > availableWidthForImage) { 
            scale = availableWidthForImage / (float)currentInstructionImage.width;
        }
        if (currentInstructionImage.height * scale > availableHeightForImage) { 
            scale = availableHeightForImage / (float)currentInstructionImage.height;
        }
        if (currentInstructionImage.width * scale > availableWidthForImage) {
             scale = availableWidthForImage / (float)currentInstructionImage.width;
        }

        float destWidth = currentInstructionImage.width * scale;
        float destHeight = currentInstructionImage.height * scale;
        
        Rectangle sourceRec = {0.0f, 0.0f, (float)currentInstructionImage.width, (float)currentInstructionImage.height};
        Rectangle destRec = {
            (SCREEN_WIDTH - destWidth) / 2, 
            imageStartY,                    
            destWidth,
            destHeight
        };
        Vector2 origin = {0.0f, 0.0f}; 

        DrawTexturePro(currentInstructionImage, sourceRec, destRec, origin, 0.0f, WHITE);

    } else {
        float placeholderY = imageStartY + availableHeightForImage / 2 - MENU_BUTTON_FONT_SIZE / 2; 
        const char* placeholderMsg = TextFormat("Instruction Page %d Image Missing", currentInstructionPage);
        DrawTextEx(bodyTextFont, placeholderMsg, 
                   {(SCREEN_WIDTH - MeasureTextEx(bodyTextFont, placeholderMsg, MENU_BUTTON_FONT_SIZE, 1).x) / 2 , placeholderY}, 
                   MENU_BUTTON_FONT_SIZE, 1, RED);
    }

    // ... (Navigation Buttons logic) ...
    float buttonY = SCREEN_HEIGHT - 70.0f; 
    float buttonWidth = 200;
    float buttonHeight = 50;

    Rectangle backToMenuButton = {30, buttonY - buttonHeight/2, buttonWidth, buttonHeight}; 
    if (DrawButton(backToMenuButton, "Main Menu", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::MAIN_MENU;
        currentInstructionPage = 1; 
    }

    if (currentInstructionPage == 1) {
        Rectangle nextButton = {(SCREEN_WIDTH - buttonWidth) - 30, buttonY - buttonHeight/2, buttonWidth, buttonHeight};
        if (DrawButton(nextButton, "Next >>", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
            currentInstructionPage = 2;
        }
    } else if (currentInstructionPage == 2) {
        Rectangle prevButton = {(SCREEN_WIDTH - buttonWidth) - 30, buttonY - buttonHeight/2, buttonWidth, buttonHeight};
        if (DrawButton(prevButton, "<< Back", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
            currentInstructionPage = 1;
        }
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
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(OVERLAY_COLOR, 0.5f)); // Transparent overlay

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

void UIManager::DrawGameOverScreen(GameScreen& currentScreen, bool playerWon, float finalTime, bool& wantsToPlayAgain) {
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
        wantsToPlayAgain = true;
    }
}

