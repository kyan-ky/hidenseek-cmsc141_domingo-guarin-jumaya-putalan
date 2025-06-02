#include "ui_manager.h"
#include "raymath.h"
#include <string> 
#include "game_manager.h" // Include for GameManager pointer type

UIManager::UIManager() : currentInstructionPage(1), gameManagerPtr(nullptr) { 
    if (FileExists("kiwi_soda.ttf")) { 
        titleTextFont = LoadFont("kiwi_soda.ttf");
    } else {
        TraceLog(LOG_WARNING, "FONT: kiwi_soda.ttf not found, using default.");
        titleTextFont = GetFontDefault();
    }

    if (FileExists("rainy_hearts.ttf")) { 
        bodyTextFont = LoadFont("rainy_hearts.ttf");
    } else {
        TraceLog(LOG_WARNING, "FONT: rainy_hearts.ttf not found, using default.");
        bodyTextFont = GetFontDefault();
    }
    
    titleBg = {0};
    howToPlayBg = {0};
    howToPlayInstructions1 = {0}; 
    howToPlayInstructions2 = {0}; 
    gameOverBg = {0};
}

void UIManager::SetGameManager(GameManager* gm) {
    gameManagerPtr = gm;
}

void UIManager::LoadAssets() {
    if (FileExists("title_screen_bg.png")) titleBg = LoadTexture("title_screen_bg.png");
    if (FileExists("how_to_play_bg.png")) howToPlayBg = LoadTexture("how_to_play_bg.png");
    if (FileExists("instruction_1.png")) howToPlayInstructions1 = LoadTexture("instruction_1.png");
    if (FileExists("instruction_2.png")) howToPlayInstructions2 = LoadTexture("instruction_2.png");
    
    if (FileExists("game_over_bg.png")) {
        gameOverBg = LoadTexture("game_over_bg.png"); 
    } else if (FileExists("game_over_bg.jpg")) { // Fallback to jpg if png not found
        gameOverBg = LoadTexture("game_over_bg.jpg");
    }
}

void UIManager::UnloadAssets() {
    if (titleBg.id > 0) UnloadTexture(titleBg);
    if (howToPlayBg.id > 0) UnloadTexture(howToPlayBg);
    if (howToPlayInstructions1.id > 0) UnloadTexture(howToPlayInstructions1); 
    if (howToPlayInstructions2.id > 0) UnloadTexture(howToPlayInstructions2); 
    if (gameOverBg.id > 0) UnloadTexture(gameOverBg);

    // Only unload fonts if they are not the default one
    if (titleTextFont.texture.id != GetFontDefault().texture.id) UnloadFont(titleTextFont); 
    if (bodyTextFont.texture.id != GetFontDefault().texture.id) {
         // Ensure not trying to unload the same font twice if titleTextFont was also default or same as bodyTextFont
         if(titleTextFont.texture.id == 0 || titleTextFont.texture.id != GetFontDefault().texture.id || bodyTextFont.texture.id != titleTextFont.texture.id) {
            UnloadFont(bodyTextFont);
         }
    }
}

bool UIManager::DrawButton(Rectangle bounds, const char* text, int fontSize, Color baseColor, Color hoverColor, Color textColor) {
    bool clicked = false;
    Vector2 mousePoint = GetMousePosition();
    Color currentButtonFaceColor = baseColor; 
    // Color currentButtonTextColor = textColor; // Not currently changed by hover/press

    float roundness = 0.35f;        
    int segments = 12;             
    float shadowOffsetXY = 3.0f;    
    float pressDepth = 2.0f;       

    Rectangle buttonFaceBounds = bounds; 
    Rectangle shadowBounds = { bounds.x + shadowOffsetXY, bounds.y + shadowOffsetXY, bounds.width, bounds.height };

    if (CheckCollisionPointRec(mousePoint, bounds)) { 
        currentButtonFaceColor = hoverColor;
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            currentButtonFaceColor = ColorBrightness(hoverColor, -0.15f); 
            buttonFaceBounds.x += pressDepth;
            buttonFaceBounds.y += pressDepth;
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePoint, bounds)) {
            clicked = true;
            if (gameManagerPtr && gameManagerPtr->sfxButtonClick.frameCount > 0) { // Check if sound is loaded
                gameManagerPtr->PlayButtonSound(); 
            }
        }
    }

    if (!(CheckCollisionPointRec(mousePoint, bounds) && IsMouseButtonDown(MOUSE_LEFT_BUTTON))) {
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
    DrawTextEx(currentButtonFont, text, textPosition, (float)fontSize, 1, textColor); // Use original textColor
    
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
    float bobAmount = 5.0f;  
    float yAnimationOffset = sinf(time * bobSpeed) * bobAmount; 
    Vector2 shadowOffset = {3, 3};
    Color shadowColor = Fade(BLACK, 0.6f);

    Vector2 titleLine1Size = MeasureTextEx(currentTitleFont, titleLine1, titleFontSize, 1);
    Vector2 titleLine1BasePos = {(SCREEN_WIDTH - titleLine1Size.x) / 2, SCREEN_HEIGHT * 0.18f };
    DrawTextEx(currentTitleFont, titleLine1, {titleLine1BasePos.x + shadowOffset.x, titleLine1BasePos.y + shadowOffset.y + yAnimationOffset}, titleFontSize, 1, shadowColor);
    DrawTextEx(currentTitleFont, titleLine1, {titleLine1BasePos.x, titleLine1BasePos.y + yAnimationOffset}, titleFontSize, 1, titleColor);

    Vector2 titleLine2Size = MeasureTextEx(currentTitleFont, titleLine2, titleFontSize, 1);
    Vector2 titleLine2BasePos = {(SCREEN_WIDTH - titleLine2Size.x) / 2, titleLine1BasePos.y + titleLine1Size.y + lineSpacing };
    DrawTextEx(currentTitleFont, titleLine2, {titleLine2BasePos.x + shadowOffset.x, titleLine2BasePos.y + shadowOffset.y + yAnimationOffset}, titleFontSize, 1, shadowColor);
    DrawTextEx(currentTitleFont, titleLine2, {titleLine2BasePos.x, titleLine2BasePos.y + yAnimationOffset}, titleFontSize, 1, titleColor);

    float buttonsStartY = titleLine2BasePos.y + titleLine2Size.y + 70; 
    Rectangle playButton = {SCREEN_WIDTH / 2.0f - 150, buttonsStartY, 300, 60};
    if (DrawButton(playButton, "Start Game", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::IN_GAME;
        if (gameManagerPtr) wantsToStartNewGame = true; 
    }
    Rectangle howToPlayButton = {SCREEN_WIDTH / 2.0f - 150, buttonsStartY + 70, 300, 60};
    if (DrawButton(howToPlayButton, "How to Play", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::HOW_TO_PLAY;
        if(gameManagerPtr) this->currentInstructionPage = 1;
    }
    Rectangle quitButton = {SCREEN_WIDTH / 2.0f - 150, buttonsStartY + 140, 300, 60};
    if (DrawButton(quitButton, "Quit", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        if (gameManagerPtr) quitGameFlag = true;
    }
} 

void UIManager::DrawHowToPlay(GameScreen& currentScreen) {
    // Your existing, working DrawHowToPlay function
    if (howToPlayBg.id > 0) DrawTexture(howToPlayBg, 0, 0, WHITE);
    else ClearBackground(DARKBLUE); 
    Font screenTitleFont = (this->titleTextFont.texture.id != 0) ? this->titleTextFont : GetFontDefault();
    const char* pageTitleText = "How to Play";
    float pageTitleFontSize = (float)HOW_TO_PLAY_SCREEN_TITLE_FONT_SIZE; 
    Color pageTitleColor = MAIN_TITLE_COLOR;          
    Vector2 pageTitleSize = MeasureTextEx(screenTitleFont, pageTitleText, pageTitleFontSize, 1);
    float titleTextHeight = pageTitleSize.y; 
    float titleYPosition = 50.0f; 
    Vector2 pageTitleBasePos = { (SCREEN_WIDTH - pageTitleSize.x) / 2, titleYPosition };
    Vector2 shadowOffset = {3, 3};      
    Color shadowColor = Fade(BLACK, 0.6f);
    DrawTextEx(screenTitleFont, pageTitleText, {pageTitleBasePos.x + shadowOffset.x, pageTitleBasePos.y + shadowOffset.y}, pageTitleFontSize, 1, shadowColor);
    DrawTextEx(screenTitleFont, pageTitleText, pageTitleBasePos, pageTitleFontSize, 1, pageTitleColor);
    Texture2D currentInstructionImage = {0};
    int instPage = this->currentInstructionPage; // Use GM's if available
    if (instPage == 1 && howToPlayInstructions1.id > 0) currentInstructionImage = howToPlayInstructions1;
    else if (instPage == 2 && howToPlayInstructions2.id > 0) currentInstructionImage = howToPlayInstructions2;
    float paddingBelowTitle = -60.0f; 
    float imageStartY = titleYPosition + titleTextHeight + paddingBelowTitle; 
    float bottomMarginForButtons = 100.0f; 
    float availableHeightForImage = SCREEN_HEIGHT - imageStartY - bottomMarginForButtons; 
    float availableWidthForImage = SCREEN_WIDTH * 0.99f; 
    if (currentInstructionImage.id > 0 && currentInstructionImage.width > 0 && currentInstructionImage.height > 0) {
        float scale = 1.0f;
        if (currentInstructionImage.width * scale > availableWidthForImage) scale = availableWidthForImage / (float)currentInstructionImage.width;
        if (currentInstructionImage.height * scale > availableHeightForImage) scale = availableHeightForImage / (float)currentInstructionImage.height;
        if (currentInstructionImage.width * scale > availableWidthForImage) scale = availableWidthForImage / (float)currentInstructionImage.width;
        float destWidth = currentInstructionImage.width * scale;
        float destHeight = currentInstructionImage.height * scale;
        Rectangle sourceRec = {0.0f, 0.0f, (float)currentInstructionImage.width, (float)currentInstructionImage.height};
        Rectangle destRec = {(SCREEN_WIDTH - destWidth) / 2, imageStartY, destWidth, destHeight };
        DrawTexturePro(currentInstructionImage, sourceRec, destRec, {0,0}, 0.0f, WHITE);
    } else { /* Placeholder */ 
        float placeholderY = imageStartY + availableHeightForImage / 2 - MENU_BUTTON_FONT_SIZE / 2; 
        const char* placeholderMsg = TextFormat("Instruction Page %d Image Missing", instPage);
        DrawTextEx(bodyTextFont, placeholderMsg, {(SCREEN_WIDTH - MeasureTextEx(bodyTextFont, placeholderMsg, MENU_BUTTON_FONT_SIZE, 1).x) / 2 , placeholderY}, MENU_BUTTON_FONT_SIZE, 1, RED);
    }
    float buttonY = SCREEN_HEIGHT - 70.0f; 
    float buttonWidth = 200;
    float buttonHeight = 50;
    Rectangle backToMenuButton = {30, buttonY - buttonHeight/2, buttonWidth, buttonHeight}; 
    if (DrawButton(backToMenuButton, "Main Menu", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::MAIN_MENU;
        if(gameManagerPtr) this->currentInstructionPage = 1; 
    }
    if (instPage == 1) { 
        Rectangle nextButton = {(SCREEN_WIDTH - buttonWidth) - 30, buttonY - buttonHeight/2, buttonWidth, buttonHeight};
        if (DrawButton(nextButton, "Next >>", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
            if(gameManagerPtr) this->currentInstructionPage = 2;
        }
    } else if (instPage == 2) { 
        Rectangle prevButton = {(SCREEN_WIDTH - buttonWidth) - 30, buttonY - buttonHeight/2, buttonWidth, buttonHeight};
        if (DrawButton(prevButton, "<< Back", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
            if(gameManagerPtr) this->currentInstructionPage = 1;
        }
    }
}

void UIManager::DrawInGameHUD(float timer, int hidersLeft, float sprintValue) {
    // Your existing DrawInGameHUD
    Font currentHudFont = (this->bodyTextFont.texture.id != 0) ? this->bodyTextFont : GetFontDefault(); // Assuming bodyTextFont for HUD
    // Define HUD_TEXT_FONT_SIZE and HUD_TEXT_COLOR in constants.h
    DrawTextEx(currentHudFont, TextFormat("Time: %02d:%02d", (int)timer / 60, (int)timer % 60), {20, 20}, (float)HUD_TEXT_FONT_SIZE, 1, HUD_TEXT_COLOR);
    const char* hidersCountText = TextFormat("Hiders Left: %d", hidersLeft);
    DrawTextEx(currentHudFont, hidersCountText, {SCREEN_WIDTH - MeasureTextEx(currentHudFont, hidersCountText, (float)HUD_TEXT_FONT_SIZE, 1).x - 20, 20}, (float)HUD_TEXT_FONT_SIZE, 1, HUD_TEXT_COLOR);
    float sprintBarWidth = 200; float sprintBarHeight = 20; float sprintBarY = SCREEN_HEIGHT - sprintBarHeight - 20;
    DrawRectangle(20, (int)sprintBarY, (int)sprintBarWidth, (int)sprintBarHeight, DARKGRAY);
    DrawRectangle(20, (int)sprintBarY, (int)(sprintBarWidth * (sprintValue / SPRINT_MAX)), (int)sprintBarHeight, SKYBLUE);
    DrawRectangleLines(20, (int)sprintBarY, (int)sprintBarWidth, (int)sprintBarHeight, LIGHTGRAY);
    DrawTextEx(currentHudFont, "Sprint", {20 + sprintBarWidth + 10, sprintBarY + (sprintBarHeight - (float)HUD_TEXT_FONT_SIZE * 0.8f)/2}, (float)HUD_TEXT_FONT_SIZE * 0.8f, 1, HUD_TEXT_COLOR);
}

void UIManager::DrawPauseMenu(GameScreen& currentScreen, bool& quitGame, bool& restartGame) {
    // Your existing DrawPauseMenu
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(OVERLAY_COLOR, 0.7f)); 
    const char* pauseText = "PAUSED";
    Font currentPauseTitleFont = (titleTextFont.texture.id != 0) ? titleTextFont : bodyTextFont;
    float textWidth = MeasureTextEx(currentPauseTitleFont, pauseText, (float)PAUSE_MENU_TITLE_FONT_SIZE, 1).x;
    DrawTextEx(currentPauseTitleFont, pauseText, {(SCREEN_WIDTH - textWidth) / 2, SCREEN_HEIGHT * 0.25f}, (float)PAUSE_MENU_TITLE_FONT_SIZE, 1, PAUSE_MENU_TEXT_COLOR);
    Rectangle resumeButton = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.4f, 300, 60};
    if (DrawButton(resumeButton, "Resume", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) currentScreen = GameScreen::IN_GAME;
    Rectangle restartBtn = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.4f + 80, 300, 60}; // Renamed to avoid conflict
    if (DrawButton(restartBtn, "Start Over", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::IN_GAME; 
        if(gameManagerPtr) restartGame = true; 
    }
    Rectangle mainMenuBtn = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.4f + 160, 300, 60}; // Renamed
    if (DrawButton(mainMenuBtn, "Main Menu", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::MAIN_MENU;
        if(gameManagerPtr) this->currentInstructionPage = 1;
    }
    Rectangle quitPauseBtn = {SCREEN_WIDTH / 2.0f - 150, SCREEN_HEIGHT * 0.4f + 240, 300, 60}; // Renamed
    if (DrawButton(quitPauseBtn, "Quit Game", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        if(gameManagerPtr) quitGame = true; 
    }
}

void UIManager::DrawGameOverScreen(GameScreen& currentScreen, bool playerWon, float finalTime, bool& wantsToPlayAgain) {
    // Your existing DrawGameOverScreen
    if (gameOverBg.id > 0) DrawTexture(gameOverBg, 0, 0, WHITE);
    else ClearBackground(playerWon ? DARKGREEN : MAROON);
    Font currentGOTitleFont = (this->titleTextFont.texture.id != 0) ? this->titleTextFont : GetFontDefault(); // Renamed local var
    Font currentGOBodyFont = (this->bodyTextFont.texture.id != 0) ? this->bodyTextFont : GetFontDefault(); // Renamed local var
    const char* primaryGOText = playerWon ? "YOU WIN!" : "GAME OVER"; // Renamed local var
    Color primaryGOColor = playerWon ? GAME_OVER_WIN_COLOR : GAME_OVER_LOSS_COLOR; 
    float primaryGOFontSize = (float)GAME_OVER_TITLE_FONT_SIZE;
    Vector2 primaryGOSize = MeasureTextEx(currentGOTitleFont, primaryGOText, primaryGOFontSize, 1);
    float primaryGOTextY = SCREEN_HEIGHT * 0.30f - primaryGOSize.y / 2; 
    DrawTextEx(currentGOTitleFont, primaryGOText, {(SCREEN_WIDTH - primaryGOSize.x) / 2, primaryGOTextY}, primaryGOFontSize, 1, primaryGOColor);
    const char* reasonTxt = ""; 
    float paddingBelowPrimary = 60.0f; 
    float reasonTxtY = primaryGOTextY + primaryGOSize.y + paddingBelowPrimary;
    if (playerWon) reasonTxt = TextFormat("All students accounted for in %02d:%02d!\nYou can run, but you can't hide from knowledge!", (int)finalTime / 60, (int)finalTime % 60);
    else { if (finalTime == 0) reasonTxt = "Lecture missed, attendance dismissed!"; else reasonTxt = "Your final state is... tagged!\nPerhaps a review session is in order?"; }
    float reasonTxtFontSize = (float)GAME_OVER_REASON_FONT_SIZE;
    Color reasonTxtColor = GAME_OVER_REASON_TEXT_COLOR;      
    std::string reasonOutputStr = reasonTxt; 
    size_t nlPos = reasonOutputStr.find('\n');
    float actualReasonTxtHeight; 
    if (nlPos != std::string::npos) { 
        std::string ln1 = reasonOutputStr.substr(0, nlPos);
        std::string ln2 = reasonOutputStr.substr(nlPos + 1); 
        Vector2 ln1Size = MeasureTextEx(currentGOBodyFont, ln1.c_str(), reasonTxtFontSize, 1);
        Vector2 ln2Size = MeasureTextEx(currentGOBodyFont, ln2.c_str(), reasonTxtFontSize, 1); 
        float lineSpaceBetween = reasonTxtFontSize * 0.1f;
        actualReasonTxtHeight = ln1Size.y + lineSpaceBetween + ln2Size.y;
        DrawTextEx(currentGOBodyFont, ln1.c_str(), {(SCREEN_WIDTH - ln1Size.x) / 2, reasonTxtY}, reasonTxtFontSize, 1, reasonTxtColor);
        DrawTextEx(currentGOBodyFont, ln2.c_str(), {(SCREEN_WIDTH - ln2Size.x) / 2, reasonTxtY + ln1Size.y + lineSpaceBetween}, reasonTxtFontSize, 1, reasonTxtColor);
    } else { 
        Vector2 singleLnSize = MeasureTextEx(currentGOBodyFont, reasonTxt, reasonTxtFontSize, 1);
        actualReasonTxtHeight = singleLnSize.y;
        DrawTextEx(currentGOBodyFont, reasonTxt, {(SCREEN_WIDTH - singleLnSize.x) / 2, reasonTxtY}, reasonTxtFontSize, 1, reasonTxtColor);
    }
    float buttonsY = reasonTxtY + actualReasonTxtHeight + 50; 
    if (buttonsY < SCREEN_HEIGHT * 0.65f) buttonsY = SCREEN_HEIGHT * 0.65f; 
    if (buttonsY + 140 > SCREEN_HEIGHT - 30) buttonsY = SCREEN_HEIGHT - 170; 
    Rectangle gameOverMmButton = {SCREEN_WIDTH / 2.0f - 150, buttonsY, 300, 60}; 
    if (DrawButton(gameOverMmButton, "Main Menu", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::MAIN_MENU;
        if(gameManagerPtr) this->currentInstructionPage = 1; 
    }
    Rectangle playAgainBtn = {SCREEN_WIDTH / 2.0f - 150, buttonsY + 70, 300, 60};
    if (DrawButton(playAgainBtn, "Play Again", MENU_BUTTON_FONT_SIZE, BUTTON_COLOR, BUTTON_HOVER_COLOR, MENU_BUTTON_TEXT_COLOR)) {
        currentScreen = GameScreen::IN_GAME; 
        if(gameManagerPtr) wantsToPlayAgain = true; 
    }
}
