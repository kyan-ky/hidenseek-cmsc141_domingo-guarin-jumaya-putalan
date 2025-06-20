#include "map.h"
#include "constants.h"
#include "raymath.h" // For Vector2Distance
#include <cstdlib> // For rand()

Map::Map() {
    background = {0}; // Initialize texture struct
    // TODO: Add Texture2D wallTexture = {0}; to your Map class in map.h
    wallTexture = {0}; // Initialize the new texture struct
    objTexture = {0};
    interior = {0};
}

void Map::Load() {
    // Load the base map design
    if (FileExists("map_design.jpg")) {
        background = LoadTexture("map_design.jpg");
    } 
    if (FileExists("map_interior.png")) {
        interior = LoadTexture("map_interior.png");
    }

    // Load the wall texture
    if (FileExists("wall_bg.png")) {
        wallTexture = LoadTexture("wall_bg.png");
    }

    if (FileExists("Object_hiding.png")) {
        objTexture = LoadTexture("Object_hiding.png");
    }
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
    
    if (IsPositionValid(HIDING_SPOT_BUSH_G1, 0)) hidingSpots.push_back(HIDING_SPOT_BUSH_G1);
    if (IsPositionValid(HIDING_SPOT_BUSH_G2, 0)) hidingSpots.push_back(HIDING_SPOT_BUSH_G2);
    if (IsPositionValid(HIDING_SPOT_BUSH_G3, 0)) hidingSpots.push_back(HIDING_SPOT_BUSH_G3);
    if (IsPositionValid(HIDING_SPOT_BUSH_G4, 0)) hidingSpots.push_back(HIDING_SPOT_BUSH_G4);
    if (IsPositionValid(HIDING_SPOT_BUSH_G5, 0)) hidingSpots.push_back(HIDING_SPOT_BUSH_G5);
    if (IsPositionValid(HIDING_SPOT_BUSH_B1, 0)) hidingSpots.push_back(HIDING_SPOT_BUSH_B1);
    if (IsPositionValid(HIDING_SPOT_BUSH_B2, 0)) hidingSpots.push_back(HIDING_SPOT_BUSH_B2);
    if (IsPositionValid(HIDING_SPOT_BUSH_B3, 0)) hidingSpots.push_back(HIDING_SPOT_BUSH_B3);
    if (IsPositionValid(HIDING_SPOT_BUSH_B4, 0)) hidingSpots.push_back(HIDING_SPOT_BUSH_B4);
    if (IsPositionValid(HIDING_SPOT_TABLE_1, 0)) hidingSpots.push_back(HIDING_SPOT_TABLE_1);
    if (IsPositionValid(HIDING_SPOT_TABLE_2, 0)) hidingSpots.push_back(HIDING_SPOT_TABLE_2);
    if (IsPositionValid(HIDING_SPOT_WASHER, 0)) hidingSpots.push_back(HIDING_SPOT_WASHER);
    if (IsPositionValid(HIDING_SPOT_BOX, 0)) hidingSpots.push_back(HIDING_SPOT_BOX);
    if (IsPositionValid(HIDING_SPOT_COUCH_1, 0)) hidingSpots.push_back(HIDING_SPOT_COUCH_1);
    if (IsPositionValid(HIDING_SPOT_COUCH_2, 0)) hidingSpots.push_back(HIDING_SPOT_COUCH_2);
    if (IsPositionValid(HIDING_SPOT_COUCH_3, 0)) hidingSpots.push_back(HIDING_SPOT_COUCH_3);
    if (IsPositionValid(HIDING_SPOT_PLANT, 0)) hidingSpots.push_back(HIDING_SPOT_PLANT);
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

void Map::Draw() {
    // Draw the base map design first
    if (background.id > 0) {
        DrawTexture(background, 0, 0, WHITE);
    } else {
        ClearBackground(RAYWHITE); // Fallback if no texture
    }
    if (interior.id > 0) {
        DrawTexture(interior, 0, 0, WHITE);
    }
    if (wallTexture.id > 0) {
        DrawTexture(wallTexture, 0, 0, WHITE);
    }

    // Draw obstacles (for debugging or if they are simple visual elements)
    for (const auto& obs : obstacles) {
        DrawRectangleRec(obs, Fade(BLACK, 0.0f));
    }
}

void Map::DrawBaseAndWalls() {
    // Draw the base map design first
    if (background.id > 0) {
        DrawTexture(background, 0, 0, WHITE);
    } else {
        ClearBackground(RAYWHITE); // Fallback if no texture
    }
    if (interior.id > 0) {
        DrawTexture(interior, 0, 0, WHITE);
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
        float maxDistance = 50.0f;  // Maximum distance for partial transparency
        float minDistance = 10.0f;  // Minimum distance for maximum transparency
        
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
    // Check screen boundaries without margin
    float minX = 0;
    float maxX = SCREEN_WIDTH;
    float minY = 0;
    float maxY = SCREEN_HEIGHT;

    // Check if position is within bounds, accounting for the entity's radius
    if (position.x - radius < minX || position.x + radius > maxX ||
        position.y - radius < minY || position.y + radius > maxY) {
        return false;
    }

    // Check against all obstacles with a safety margin
    for (const auto& obs : obstacles) {
        // Create a slightly larger rectangle to account for the radius and safety margin
        float safetyMargin = radius + 5.0f; // Add 5 pixels of safety margin
        Rectangle expandedObs = {
            obs.x - safetyMargin,
            obs.y - safetyMargin,
            obs.width + (safetyMargin * 2),
            obs.height + (safetyMargin * 2)
        };
        
        // Check if the position is inside the expanded obstacle
        if (CheckCollisionPointRec(position, expandedObs)) {
            return false;
        }
    }
    return true;
}

