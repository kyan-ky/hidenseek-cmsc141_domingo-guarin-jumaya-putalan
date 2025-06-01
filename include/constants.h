#pragma once

#include "raylib.h"

// Screen Dimensions
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

// Game Title
inline const char* GAME_TITLE = "State of Fear: Ryan's Revenge";

// Player Constants
const float PLAYER_SPEED = 200.0f;
const float PLAYER_SPRINT_SPEED = 350.0f;
const float PLAYER_RADIUS = 15.0f;
const float PLAYER_VISION_CONE_ANGLE = 60.0f; // degrees
const float PLAYER_VISION_RADIUS = 200.0f;
const float SPRINT_MAX = 100.0f;
const float SPRINT_DEPLETE_RATE = 30.0f; // per second
const float SPRINT_REGEN_RATE = 15.0f;   // per second
const float TAG_RANGE = 50.0f; // How close player needs to be within cone to tag

// Hider Constants
const float HIDER_SPEED = 180.0f;
const float HIDER_RADIUS = 12.0f;
const float HIDER_VISION_CONE_ANGLE = 90.0f; // degrees
const float HIDER_VISION_RADIUS = 150.0f;
const float HIDER_ATTACK_COOLDOWN = 3.0f; // seconds
const float HIDER_ATTACK_RANGE = 30.0f;
const int NUM_HIDERS = 5;

// Game Timer
const float HIDING_PHASE_DURATION = 10.0f; // seconds for hiders to hide
const float SEEKING_PHASE_DURATION = 120.0f; // 2 minutes for seeker

// Colors
const Color PLAYER_COLOR = BLUE;
const Color HIDER_COLOR = GREEN;
const Color HIDER_TAGGED_COLOR = LIGHTGRAY;
const Color VISION_CONE_COLOR = Fade(YELLOW, 0.3f);
const Color ALERT_COLOR = RED;

// UI
/*const int FONT_SIZE_LARGE = 100;
const int FONT_SIZE_MEDIUM = 40;
const int FONT_SIZE_SMALL = 20; */ 
const int MAIN_TITLE_FONT_SIZE = 120;
const int MENU_BUTTON_FONT_SIZE = 30;
const int HOW_TO_PLAY_TITLE_FONT_SIZE = 48;
const int HOW_TO_PLAY_BODY_FONT_SIZE = 22;
const int HUD_TEXT_FONT_SIZE = 32;
const int PAUSE_MENU_TITLE_FONT_SIZE = 50;
const int GAME_OVER_TITLE_FONT_SIZE = 60;
const int GAME_OVER_REASON_FONT_SIZE = 30;

const Color TEXT_COLOR = WHITE;
const Color BUTTON_COLOR = GetColor(0xAF3800FF);
const Color BUTTON_HOVER_COLOR = GetColor(0xE86A17FF);
const Color OVERLAY_COLOR = Fade(BLACK, 0.7f); 

const Color MAIN_TITLE_COLOR = GetColor(0xFFCF56FF);
const Color MENU_BUTTON_TEXT_COLOR = GetColor(0xEDEAD0FF);
const Color HOW_TO_PLAY_TITLE_COLOR = SKYBLUE;
const Color HOW_TO_PLAY_BODY_COLOR = LIGHTGRAY;
const Color HUD_TEXT_COLOR = WHITE;
const Color PAUSE_MENU_TEXT_COLOR = WHITE;
const Color GAME_OVER_WIN_COLOR = GREEN;
const Color GAME_OVER_LOSS_COLOR = RED;
const Color GAME_OVER_REASON_TEXT_COLOR = WHITE;


// Map (Example values, adjust to your map_design.jpg)
inline const Vector2 HIDING_SPOT_1 = {200, 200};
inline const Vector2 HIDING_SPOT_2 = {1000, 150};
inline const Vector2 HIDING_SPOT_3 = {1100, 600};
inline const Vector2 HIDING_SPOT_4 = {150, 550};
inline const Vector2 HIDING_SPOT_5 = {700, 400};
// Add more if NUM_HIDERS is larger

// Alert symbol
const float ALERT_BEHIND_DISTANCE = 100.0f;
const float ALERT_BEHIND_ANGLE_RANGE = 120.0f; // degrees, centered at player's back

