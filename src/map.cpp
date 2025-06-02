#include "map.h"
#include "constants.h"
#include <cstdlib> // For rand()

Map::Map() {
    background = {0}; // Initialize texture struct
    // TODO: Add Texture2D wallTexture = {0}; to your Map class in map.h
    wallTexture = {0}; // Initialize the new texture struct
}

void Map::Load() {
    // Load the base map design
    if (FileExists("map_design.jpg")) {
        background = LoadTexture("map_design.jpg");
    } else {
        // Create a placeholder background if map_design.jpg is missing
        Image img = GenImageChecked(SCREEN_WIDTH, SCREEN_HEIGHT, 64, 64, DARKGRAY, GRAY);
        background = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // Load the wall texture
    if (FileExists("layer_1_bg.png")) {
        // TODO: Load into the new wallTexture member you add to map.h
        wallTexture = LoadTexture("layer_1_bg.png");
    }

    // Clear previous obstacles
    obstacles.clear();

    // Horizontal wall above kitchen
    obstacles.push_back({236, 242, 394, 146});
    obstacles.push_back({551, 169, 80, 74});
    obstacles.push_back({630, 316, 78, 73});
    // Top wall
    obstacles.push_back({552, 21, 393, 74});
    obstacles.push_back({867, 95, 77, 74});
    // Reverse L Wall Top
    obstacles.push_back({787, 317, 158, 73});
    obstacles.push_back({866, 244, 79, 73});
    // Hallway Boxes
    obstacles.push_back({866, 462, 80, 73});
    obstacles.push_back({563, 472, 49, 48});
    // Bottom wall
    obstacles.push_back({236, 533, 80, 75});
    obstacles.push_back({236, 608, 708, 74});
    InitHidingSpots();
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
    // TODO: Unload wallTexture if it was loaded
    if (wallTexture.id > 0) UnloadTexture(wallTexture);
}

void Map::Draw() {
    // Draw the base map design first
    if (background.id > 0) {
        DrawTexture(background, 0, 0, WHITE);
    } else {
        ClearBackground(RAYWHITE); // Fallback if no texture
    }

    // Draw the wall texture on top
    // TODO: Draw wallTexture if it was loaded
    if (wallTexture.id > 0) {
        DrawTexture(wallTexture, 0, 0, WHITE);
    }

    // Draw obstacles (for debugging or if they are simple visual elements)
    for (const auto& obs : obstacles) {
        DrawRectangleRec(obs, Fade(BLACK, 0.0f));
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

