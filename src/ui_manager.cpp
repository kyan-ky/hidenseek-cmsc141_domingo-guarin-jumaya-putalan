#include "ui_manager.h"
#include "raymath.h"
#include <string>


UIManager::UIManager() : currentInstructionPage(1) {
    if (FileExists("kiwi_soda.ttf")) { 
        titleTextFont = LoadFont("kiwi_soda.ttf");
    } else {
        titleTextFont = GetFontDefault();
    }

    if (FileExists("rainy_hearts.ttf")) { 
        bodyTextFont = LoadFont("rainy_hearts.ttf");
    } else {
        bodyTextFont = GetFontDefault();
    }
    
    titleBg = {0};
    howToPlayBg = {0};
    howToPlayInstructions1 = {0}; 
    howToPlayInstructions2 = {0}; 
    gameOverBg = {0};
}

void UIManager::LoadAssets() {
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
    Color currentButtonFaceColor = baseColor; 
    Color currentButtonTextColor = textColor;

    float roundness = 0.35f;        
    int segments = 12;              
    float shadowOffset = 3.0f;  
    float pressDepth = 2.0f;    

    Rectangle buttonFaceBounds = bounds; 

    if (CheckCollisionPointRec(mousePoint, bounds)) { 
        currentButtonFaceColor = hoverColor;
        // currentButtonTextColor = ColorBrightness(textColor, 0.2f);

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            currentButtonFaceColor = ColorBrightness(hoverColor, -0.1f); 
            
            buttonFaceBounds.x += pressDepth;
            buttonFaceBounds.y += pressDepth;
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePoint, bounds)) {
            clicked = true;
        }
    }


    if (!(CheckCollisionPointRec(mousePoint, bounds) && IsMouseButtonDown(MOUSE_LEFT_BUTTON))) {
        Rectangle shadowBounds = {
            bounds.x + shadowOffset,
            bounds.y + shadowOffset,
            bounds.width,
            bounds.height
        };
        DrawRectangleRounded(shadowBounds, roundness, segments, ColorBrightness(baseColor, -0.5f)); 
    }

    DrawRectangleRounded(buttonFaceBounds, roundness, segments, currentButtonFaceColor);

    Font currentButtonFont = (this->bodyTextFont.texture.id != 0) ? this->bodyTextFont : GetFontDefault();
    float textWidth = MeasureTextEx(currentButtonFont, text, (float)fontSize, 1).x;
    
    Vector2 textPosition = {
        buttonFaceBounds.x + (buttonFaceBounds.width - textWidth) / 2,
        buttonFaceBounds.y + (buttonFaceBounds.height - fontSize) / 2
    };

    Color textOutlineColor = Fade(BLACK, 0.5f); 
    int textOutlineThickness = 1; 

    DrawTextEx(currentButtonFont, text, (Vector2){textPosition.x - textOutlineThickness, textPosition.y}, (float)fontSize, 1, textOutlineColor);
    DrawTextEx(currentButtonFont, text, (Vector2){textPosition.x + textOutlineThickness, textPosition.y}, (float)fontSize, 1, textOutlineColor);
    DrawTextEx(currentButtonFont, text, (Vector2){textPosition.x, textPosition.y - textOutlineThickness}, (float)fontSize, 1, textOutlineColor);
    DrawTextEx(currentButtonFont, text, (Vector2){textPosition.x, textPosition.y + textOutlineThickness}, (float)fontSize, 1, textOutlineColor);
    DrawTextEx(currentButtonFont, text, textPosition, (float)fontSize, 1, currentButtonTextColor);
    
    return clicked;
}

void UIManager::DrawMainMenu(GameScreen& currentScreen, bool& quitGameFlag, bool& wantsToStartNewGame) {
    if (titleBg.id > 0) DrawTexture(titleBg, 0, 0, WHITE);
    else ClearBackground(DARKGRAY); 

    const char* titleLine1 = "State of Fear:";
    const char* titleLine2 = "Ryan's Revenge";

    Font currentTitleFont = (titleTextFont.texture.id != 0) ? titleTextFont : GetFontDefault();
    float titleFontSize = (float)MAIN_TITLE_FONT_SIZE; 
    Color titleColor = MAIN_TITLE_COLOR;
    
    float lineSpacing = titleFontSize * 0.15f; 
    float time = GetTime();
    float bobSpeed = 2.0f;    
    float bobAmount = 4.0f;   
    float yAnimationOffset = sinf(time * bobSpeed) * bobAmount;

    Vector2 titleLine1Size = MeasureTextEx(currentTitleFont, titleLine1, titleFontSize, 1); 
    Vector2 titleLine1BasePos = {
        (SCREEN_WIDTH - titleLine1Size.x) / 2,  
        SCREEN_HEIGHT * 0.18f                   
    };
    Vector2 shadowOffset = {3, 3};
    Color shadowColor = Fade(BLACK, 0.6f);
    DrawTextEx(currentTitleFont, titleLine1, 
           {titleLine1BasePos.x + shadowOffset.x, titleLine1BasePos.y + shadowOffset.y + yAnimationOffset}, 
           titleFontSize, 1, shadowColor);
    DrawTextEx(currentTitleFont, titleLine1, {titleLine1BasePos.x, titleLine1BasePos.y + yAnimationOffset}, titleFontSize, 1, titleColor);

    Vector2 titleLine2Size = MeasureTextEx(currentTitleFont, titleLine2, titleFontSize, 1); 
    Vector2 titleLine2BasePos = {
        (SCREEN_WIDTH - titleLine2Size.x) / 2,   
        titleLine1BasePos.y + titleLine1Size.y + lineSpacing
    };
    DrawTextEx(currentTitleFont, titleLine2, 
           {titleLine2BasePos.x + shadowOffset.x, titleLine2BasePos.y + shadowOffset.y + yAnimationOffset}, 
           titleFontSize, 1, shadowColor);
    DrawTextEx(currentTitleFont, titleLine2, {titleLine2BasePos.x, titleLine2BasePos.y + yAnimationOffset}, titleFontSize, 1, titleColor);

    float buttonsStartY = titleLine2BasePos.y + titleLine2Size.y + 70; 

    Rectangle playButton = {SCREEN_WIDTH / 2.0f - 150, buttonsStartY, 300, 60};
    if (DrawButton(playButton, "Start Game", MENU_BUTTON_FONT_SIZE, 
                   BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::IN_GAME;
        wantsToStartNewGame = true;
    }

    Rectangle howToPlayButton = {SCREEN_WIDTH / 2.0f - 150, buttonsStartY + 70, 300, 60}; 
    if (DrawButton(howToPlayButton, "How to Play", MENU_BUTTON_FONT_SIZE, 
                   BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::HOW_TO_PLAY;
    }

    Rectangle quitButton = {SCREEN_WIDTH / 2.0f - 150, buttonsStartY + 140, 300, 60}; 
    if (DrawButton(quitButton, "Quit", MENU_BUTTON_FONT_SIZE, 
                   BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        quitGameFlag = true;
    }
} 


void UIManager::DrawHowToPlay(GameScreen& currentScreen) {
    if (howToPlayBg.id > 0) DrawTexture(howToPlayBg, 0, 0, WHITE);
    else ClearBackground(DARKBLUE); 

    Font screenTitleFont = (this->titleTextFont.texture.id != 0) ? this->titleTextFont : GetFontDefault();
    const char* pageTitleText = "How to Play";
    float pageTitleFontSize = (float)HOW_TO_PLAY_SCREEN_TITLE_FONT_SIZE; 
    Color pageTitleColor = MAIN_TITLE_COLOR;

    Vector2 pageTitleSize = MeasureTextEx(screenTitleFont, pageTitleText, pageTitleFontSize, 1);
    float titleTextHeight = pageTitleSize.y; 

    float titleYPosition = 50.0f; 

    Vector2 pageTitleBasePos = { 
        (SCREEN_WIDTH - pageTitleSize.x) / 2, 
        titleYPosition
    };


    Vector2 shadowOffset = {3, 3};      
    Color shadowColor = Fade(BLACK, 0.6f);

    DrawTextEx(screenTitleFont, pageTitleText, 
               {pageTitleBasePos.x + shadowOffset.x, pageTitleBasePos.y + shadowOffset.y}, 
               pageTitleFontSize, 1, shadowColor);

    DrawTextEx(screenTitleFont, pageTitleText, 
               pageTitleBasePos, 
               pageTitleFontSize, 1, pageTitleColor);

    Texture2D currentInstructionImage = {0};
    if (currentInstructionPage == 1 && howToPlayInstructions1.id > 0) {
        currentInstructionImage = howToPlayInstructions1;
    } else if (currentInstructionPage == 2 && howToPlayInstructions2.id > 0) {
        currentInstructionImage = howToPlayInstructions2;
    }

    float paddingBelowTitle = -60.0f; 
    float imageStartY = titleYPosition + titleTextHeight + paddingBelowTitle; 
    
    float bottomMarginForButtons = 100.0f; 
    float availableHeightForImage = SCREEN_HEIGHT - imageStartY - bottomMarginForButtons; 
    float availableWidthForImage = SCREEN_WIDTH * 0.95f; 

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

    DrawTextEx(currentHudFont, TextFormat("Time: %02d:%02d", (int)timer / 60, (int)timer % 60),
               {20, 20}, (float)HUD_TEXT_FONT_SIZE, 1, HUD_TEXT_COLOR);

    DrawTextEx(currentHudFont, TextFormat("Hiders Left: %d", hidersLeft),
                {SCREEN_WIDTH - MeasureTextEx(currentHudFont, TextFormat("Hiders Left: %d", NUM_HIDERS), (float)HUD_TEXT_FONT_SIZE, 1).x - 20, 20}, 
               (float)HUD_TEXT_FONT_SIZE, 1, HUD_TEXT_COLOR);
    float sprintBarWidth = 200;
    float sprintBarHeight = 20;
    DrawRectangle(20, SCREEN_HEIGHT - 40.0f, (int)sprintBarWidth, (int)sprintBarHeight, DARKGRAY);
    DrawRectangle(20, SCREEN_HEIGHT - 40.0f, (int)(sprintBarWidth * (sprintValue / SPRINT_MAX)), (int)sprintBarHeight, SKYBLUE);
    DrawRectangleLines(20, (int)(SCREEN_HEIGHT - 40.0f), (int)sprintBarWidth, (int)sprintBarHeight, LIGHTGRAY);
    DrawTextEx(currentHudFont, "Sprint", {25 + 200.0f, SCREEN_HEIGHT - 40.0f}, (float)HUD_TEXT_FONT_SIZE * 0.7f, 1, HUD_TEXT_COLOR);
}


void UIManager::DrawPauseMenu(GameScreen& currentScreen, bool& quitGame, bool& restartGame) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(OVERLAY_COLOR, 0.5f)); 

    const char* pauseText = "PAUSED";
    Font currentPauseTitleFont = titleTextFont.texture.id != 0 ? titleTextFont : bodyTextFont;
    float textWidth = MeasureTextEx(currentPauseTitleFont, pauseText, (float)PAUSE_MENU_TITLE_FONT_SIZE, 1).x;
    DrawTextEx(currentPauseTitleFont, pauseText, 
               {(SCREEN_WIDTH - textWidth) / 2, SCREEN_HEIGHT * 0.25f}, 
               (float)PAUSE_MENU_TITLE_FONT_SIZE, 1, PAUSE_MENU_TEXT_COLOR);

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


void UIManager::DrawGameOverScreen(GameScreen& currentScreen, bool playerWon, float finalTime, bool& wantsToPlayAgain) {
    if (gameOverBg.id > 0) DrawTexture(gameOverBg, 0, 0, WHITE);
    else ClearBackground(playerWon ? DARKGREEN : MAROON);

    Font currentTitleFont = (this->titleTextFont.texture.id != 0) ? this->titleTextFont : GetFontDefault();
    Font currentBodyFont = (this->bodyTextFont.texture.id != 0) ? this->bodyTextFont : GetFontDefault();

    const char* primaryGameOverText = playerWon ? "YOU WIN!" : "GAME OVER";
    Color primaryGameOverColor = playerWon ? GAME_OVER_WIN_COLOR : GAME_OVER_LOSS_COLOR; 
    float primaryTextFontSize = (float)GAME_OVER_TITLE_FONT_SIZE; 

    Vector2 primaryTextSize = MeasureTextEx(currentTitleFont, primaryGameOverText, primaryTextFontSize, 1);
    float primaryTextY = SCREEN_HEIGHT * 0.28f - primaryTextSize.y / 2; 
    DrawTextEx(currentTitleFont, primaryGameOverText, 
               {(SCREEN_WIDTH - primaryTextSize.x) / 2, primaryTextY}, 
               primaryTextFontSize, 1, primaryGameOverColor);

    const char* reasonText;
    if (playerWon) {
        reasonText = TextFormat("All students accounted for in %02d:%02d!\nYou can run, but you can't hide from knowledge!", (int)finalTime / 60, (int)finalTime % 60);
    } else {
        if (finalTime == 0) { 
             reasonText = "Lecture missed, attendance dismissed!";
        } else {
             reasonText = "Your final state is... tagged!\nPerhaps a review session is in order?";
        }
    }
    
    float reasonTextFontSize = (float)GAME_OVER_REASON_FONT_SIZE;
    Color reasonTextColor = GAME_OVER_REASON_TEXT_COLOR;      

    float paddingBelowPrimaryText = 50.0f; 
    float reasonTextCalculatedY = primaryTextY + primaryTextSize.y + paddingBelowPrimaryText;

    std::string reasonStr = reasonText;
    size_t newlinePos = reasonStr.find('\n');

    if (newlinePos != std::string::npos) { 
        std::string line1 = reasonStr.substr(0, newlinePos);
        std::string line2 = reasonStr.substr(newlinePos + 1);
        Vector2 line1Size = MeasureTextEx(currentBodyFont, line1.c_str(), reasonTextFontSize, 1);
        Vector2 line2Size = MeasureTextEx(currentBodyFont, line2.c_str(), reasonTextFontSize, 1);
        
        DrawTextEx(currentBodyFont, line1.c_str(), 
                   {(SCREEN_WIDTH - line1Size.x) / 2, reasonTextCalculatedY}, 
                   reasonTextFontSize, 1, reasonTextColor);
        DrawTextEx(currentBodyFont, line2.c_str(), 
                   {(SCREEN_WIDTH - line2Size.x) / 2, reasonTextCalculatedY + line1Size.y + (reasonTextFontSize * 0.1f)},
                   reasonTextFontSize, 1, reasonTextColor);
    } else {
        Vector2 singleLineSize = MeasureTextEx(currentBodyFont, reasonText, reasonTextFontSize, 1);
        DrawTextEx(currentBodyFont, reasonText, 
                   {(SCREEN_WIDTH - singleLineSize.x) / 2, reasonTextCalculatedY}, 
                   reasonTextFontSize, 1, reasonTextColor);
    }

    float actualReasonTextHeight;
    if (newlinePos != std::string::npos) {
        std::string line1 = reasonStr.substr(0, newlinePos);
        std::string line2 = reasonStr.substr(newlinePos + 1);
        actualReasonTextHeight = MeasureTextEx(currentBodyFont, line1.c_str(), reasonTextFontSize, 1).y + 
                                 MeasureTextEx(currentBodyFont, line2.c_str(), reasonTextFontSize, 1).y +
                                 (reasonTextFontSize * 0.1f); 
    } else {
        actualReasonTextHeight = MeasureTextEx(currentBodyFont, reasonText, reasonTextFontSize, 1).y;
    }
    float buttonsStartY = reasonTextCalculatedY + actualReasonTextHeight + 60; 

    if (buttonsStartY < SCREEN_HEIGHT * 0.65f) buttonsStartY = SCREEN_HEIGHT * 0.65f; 
    if (buttonsStartY + 140 > SCREEN_HEIGHT - 30) buttonsStartY = SCREEN_HEIGHT - 170; 

    Rectangle mainMenuButton = {SCREEN_WIDTH / 2.0f - 150, buttonsStartY, 300, 60};
    if (DrawButton(mainMenuButton, "Main Menu", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::MAIN_MENU;
        this->currentInstructionPage = 1; 
    }

    Rectangle playAgainButton = {SCREEN_WIDTH / 2.0f - 150, buttonsStartY + 70, 300, 60};
    if (DrawButton(playAgainButton, "Play Again", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::IN_GAME; 
        wantsToPlayAgain = true;
    }
}
