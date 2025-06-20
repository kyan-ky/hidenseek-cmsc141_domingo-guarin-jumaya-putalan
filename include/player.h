#pragma once

#include "raylib.h"
#include "constants.h"
#include <vector> // For vision cone points

class GameManager; // Forward declaration

class Player {
public:
    Vector2 position;
    float rotation; // in degrees, 0 is right, 90 is down
    float speed;
    float sprintValue;
    bool isSprinting;
    Texture2D texture;
    Texture2D alertTexture;
    Texture2D tagTexture; // New texture for tagging state
    Sound tagSound; // New sound for tagging
    bool showAlert;
    std::vector<Vector2> visionConePoints;
    bool isTagged;
    GameManager* gameManager; // Reference to game manager

    Player();
    void Init(Vector2 startPos);
    void HandleInput(const class Map& map);
    void Update(float deltaTime, const class Map& map, const std::vector<class Hider>& hiders);
    void Draw();
    bool CanTag(const class Hider& hider) const;
    Vector2 GetForwardVector() const;
    bool IsInVisionCone(Vector2 targetPos, float coneAngle, float visionRadius) const;
    bool IsLookingAt(Vector2 targetPos) const;
    void SetTagged(bool tagged) { isTagged = tagged; }
    bool IsInAlertStatus() const { return showAlert; }

private:
    void UpdateVision();
     // For drawing
};

