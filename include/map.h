#pragma once

#include "raylib.h"
#include <vector>

class Map {
public:
    Texture2D background;
    std::vector<Rectangle> obstacles; // Simple rectangular obstacles
    std::vector<Vector2> hidingSpots;

    Map();
    void Load();
    void Unload();
    void Draw();
    bool IsPositionValid(Vector2 position, float radius) const; // Basic bounds check for now
    Vector2 GetRandomHidingSpot() const;
    void InitHidingSpots();
};

