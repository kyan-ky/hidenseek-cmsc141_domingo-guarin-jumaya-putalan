#include "map.h"
#include "constants.h"
#include <cstdlib> // For rand()

Map::Map() {
    background = {0}; // Initialize texture struct
}

void Map::Load() {
    if (FileExists("map_design.jpg")) {
        background = LoadTexture("map_design.jpg");
    } else {
        // Create a placeholder background if map_design.jpg is missing
        Image img = GenImageChecked(SCREEN_WIDTH, SCREEN_HEIGHT, 64, 64, DARKGRAY, GRAY);
        background = LoadTextureFromImage(img);
        UnloadImage(img);
    }
    InitHidingSpots();

    // Example obstacles (can be loaded from a file or defined procedurally)
    // obstacles.push_back({300, 300, 100, 50}); // x, y, width, height
    // obstacles.push_back({800, 400, 50, 150});
}

void Map::InitHidingSpots() {
    hidingSpots.clear();
    // These should be valid positions on your map, away from immediate start areas or obstacles
    hidingSpots.push_back(HIDING_SPOT_1);
    hidingSpots.push_back(HIDING_SPOT_2);
    hidingSpots.push_back(HIDING_SPOT_3);
    hidingSpots.push_back(HIDING_SPOT_4);
    hidingSpots.push_back(HIDING_SPOT_5);
    // Add more if NUM_HIDERS is greater
    for(int i = hidingSpots.size(); i < NUM_HIDERS + 5; ++i) { // Add some buffer spots
        hidingSpots.push_back({(float)(rand() % (SCREEN_WIDTH-100) + 50), (float)(rand() % (SCREEN_HEIGHT-100) + 50)});
    }
}


Vector2 Map::GetRandomHidingSpot() const {
    if (hidingSpots.empty()) {
        // Fallback if no spots defined, though InitHidingSpots should prevent this
        return {(float)(rand() % SCREEN_WIDTH), (float)(rand() % SCREEN_HEIGHT)};
    }
    return hidingSpots[rand() % hidingSpots.size()];
}

void Map::Unload() {
    if (background.id > 0) UnloadTexture(background);
}

void Map::Draw() {
    if (background.id > 0) {
        DrawTexture(background, 0, 0, WHITE);
    } else {
        ClearBackground(RAYWHITE); // Fallback if no texture
    }

    // Draw obstacles (for debugging or if they are simple visual elements)
    for (const auto& obs : obstacles) {
        DrawRectangleRec(obs, Fade(BLACK, 0.5f));
    }
}

bool Map::IsPositionValid(Vector2 position, float radius) const {
    // Check screen boundaries
    if (position.x - radius < 0 || position.x + radius > SCREEN_WIDTH ||
        position.y - radius < 0 || position.y + radius > SCREEN_HEIGHT) {
        return false;
    }

    // Check against defined obstacles
    for (const auto& obs : obstacles) {
        if (CheckCollisionCircleRec(position, radius, obs)) {
            return false;
        }
    }
    return true;
}

