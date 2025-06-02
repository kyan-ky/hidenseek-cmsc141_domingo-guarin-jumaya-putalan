#pragma once

#include "raylib.h"
#include <vector>

class Map {
public:
    Texture2D background;
    Texture2D wallTexture;
    Texture2D objTexture;
    std::vector<Rectangle> obstacles; // Simple rectangular obstacles
    std::vector<Vector2> hidingSpots;

    Map();
    void Load();
    void Unload();
    void DrawBaseAndWalls(); // Draw background and walls
    void DrawObjects(const Vector2& playerPos); // Draw object texture (hiding spots) with transparency based on player position
    bool IsPositionValid(Vector2 position, float radius) const; // Basic bounds check for now
    Vector2 GetRandomHidingSpot() const;
    void InitHidingSpots();
};

