#pragma once

#include "raylib.h"
#include "constants.h"
#include <vector> // For vision cone points

class Player {
public:
    Vector2 position;
    float rotation; // in degrees, 0 is right, 90 is down
    float speed;
    float sprintValue;
    bool isSprinting;
    Texture2D texture;
    Texture2D alertTexture;
    bool showAlert;
    std::vector<Vector2> visionConePoints;

    Player();
    void Init(Vector2 startPos);
    void HandleInput(const class Map& map);
    void Update(float deltaTime, const class Map& map, const std::vector<class Hider>& hiders);
    void Draw();
    bool CanTag(const class Hider& hider) const;
    Vector2 GetForwardVector() const;
    bool IsInVisionCone(Vector2 targetPos, float coneAngle, float visionRadius) const;

private:
    void UpdateVision();
     // For drawing
};

