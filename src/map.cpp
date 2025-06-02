#include "map.h"
#include "constants.h"
#include "raymath.h" // For Vector2Distance
#include <cstdlib> // For rand()

Map::Map() {
    background = {0}; // Initialize texture struct
    // TODO: Add Texture2D wallTexture = {0}; to your Map class in map.h
    wallTexture = {0}; // Initialize the new texture struct
    objTexture = {0};
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

    if (FileExists("Object_hiding.png")) {
        objTexture = LoadTexture("Object_hiding.png");
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
    
    // Add all predefined hiding spots
    if (IsPositionValid(HIDING_SPOT_BUSH_G1, 0)) hidingSpots.push_back(HIDING_SPOT_BUSH_G1);
    if (IsPositionValid(HIDING_SPOT_BUSH_G2, 0)) hidingSpots.push_back(HIDING_SPOT_BUSH_G2);
    if (IsPositionValid(HIDING_SPOT_BUSH_G3, 0)) hidingSpots.push_back(HIDING_SPOT_BUSH_G3);
    if (IsPositionValid(HIDING_SPOT_BUSH_G4, 0)) hidingSpots.push_back(HIDING_SPOT_BUSH_G4);
    if (IsPositionValid(HIDING_SPOT_BUSH_B1, 0)) hidingSpots.push_back(HIDING_SPOT_BUSH_B1);
    if (IsPositionValid(HIDING_SPOT_BUSH_B2, 0)) hidingSpots.push_back(HIDING_SPOT_BUSH_B2);
    if (IsPositionValid(HIDING_SPOT_BUSH_B3, 0)) hidingSpots.push_back(HIDING_SPOT_BUSH_B3);
    if (IsPositionValid(HIDING_SPOT_TABLE_1, 0)) hidingSpots.push_back(HIDING_SPOT_TABLE_1);
    if (IsPositionValid(HIDING_SPOT_TABLE_2, 0)) hidingSpots.push_back(HIDING_SPOT_TABLE_2);
    if (IsPositionValid(HIDING_SPOT_WASHER, 0)) hidingSpots.push_back(HIDING_SPOT_WASHER);
    if (IsPositionValid(HIDING_SPOT_BOX, 0)) hidingSpots.push_back(HIDING_SPOT_BOX);
    if (IsPositionValid(HIDING_SPOT_COUCH_1, 0)) hidingSpots.push_back(HIDING_SPOT_COUCH_1);
    if (IsPositionValid(HIDING_SPOT_COUCH_2, 0)) hidingSpots.push_back(HIDING_SPOT_COUCH_2);

    // Warning if not enough hiding spots are defined for the number of hiders
    if (hidingSpots.size() < NUM_HIDERS) {
        TraceLog(LOG_WARNING, "MAP: Number of defined valid hiding spots (%d) is less than the number of hiders (%d). Hiders may spawn in sub-optimal or identical locations.", hidingSpots.size(), NUM_HIDERS);
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
    if (objTexture.id > 0) UnloadTexture(objTexture);
}

void Map::DrawBaseAndWalls() {
    // Draw the base map design first
    if (background.id > 0) {
        DrawTexture(background, 0, 0, WHITE);
    } else {
        ClearBackground(RAYWHITE); // Fallback if no texture
    }

    // Draw the wall texture on top
    if (wallTexture.id > 0) {
        DrawTexture(wallTexture, 0, 0, WHITE);
    }

    // Draw obstacles (for debugging or if they are simple visual elements)
    for (const auto& obs : obstacles) {
        DrawRectangleRec(obs, Fade(BLACK, 0.0f));
    }
}

void Map::DrawObjects(const Vector2& playerPos) {
    // Draw the object texture (hiding spots)
    if (objTexture.id > 0) {
        // Define the transparency range
        float maxDistance = 30.0f;  // Maximum distance for partial transparency
        float minDistance = 5.0f;  // Minimum distance for maximum transparency
        
        // Find the closest hiding spot to the player
        float closestDistance = maxDistance;
        for (const auto& spot : hidingSpots) {
            float distance = Vector2Distance(playerPos, spot);
            if (distance < closestDistance) {
                closestDistance = distance;
            }
        }
        
        // Calculate alpha based on closest distance
        float alpha = 1.0f;
        if (closestDistance < maxDistance) {
            if (closestDistance < minDistance) {
                alpha = 0.3f; // Partially transparent when very close
            } else {
                // Linear interpolation between 0.3 and 1.0
                alpha = 0.3f + (0.7f * (closestDistance - minDistance) / (maxDistance - minDistance));
            }
        }
        
        // Draw with transparency
        BeginBlendMode(BLEND_ALPHA);
            DrawTexture(objTexture, 0, 0, ColorAlpha(WHITE, alpha));
        EndBlendMode();
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

