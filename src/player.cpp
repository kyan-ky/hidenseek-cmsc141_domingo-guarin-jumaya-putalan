#include "player.h"
#include "hider.h" // For CanTag, and alert check
#include "map.h"
#include "raymath.h" // For Vector2Normalize, Vector2Rotate, Vector2Angle
#include <cmath>    // For atan2f, cosf, sinf, fabsf

Player::Player() : position({0, 0}), rotation(0.0f), speed(PLAYER_SPEED),
                   sprintValue(SPRINT_MAX), isSprinting(false), showAlert(false),
                   isTagged(false) {
    // Attempt to load texture, use placeholder if fails
    if (FileExists("seeker_sprite.jpg")) {
        texture = LoadTexture("seeker_sprite.jpg");
    } else {
        Image img = GenImageColor( (int)PLAYER_RADIUS * 2, (int)PLAYER_RADIUS * 2, PLAYER_COLOR);
        texture = LoadTextureFromImage(img);
        UnloadImage(img);
    }
     if (FileExists("alert_icon.png")) {
        alertTexture = LoadTexture("alert_icon.png");
    } else {
        Image img = GenImageColor(20, 20, RED); // Simple red square for alert
        ImageDrawText(&img, "!", 5, 0, 20, WHITE);
        alertTexture = LoadTextureFromImage(img);
        UnloadImage(img);
    }
}

void Player::Init(Vector2 startPos) {
    position = startPos;
    rotation = 0.0f; // Facing right
    sprintValue = SPRINT_MAX;
    isSprinting = false;
    showAlert = false;
    isTagged = false;
    UpdateVision();
}

Vector2 Player::GetForwardVector() const {
    return Vector2Rotate({1, 0}, rotation * DEG2RAD);
}

void Player::HandleInput(const Map& map) {
    Vector2 moveDir = {0, 0};
    
    // Handle sprint key press and release
    if (IsKeyDown(KEY_LEFT_SHIFT)) {
        if (sprintValue >= (SPRINT_MAX * 0.50f)) {
            isSprinting = true;
        }
    } else {
        isSprinting = false;
    }
    
    float currentSpeed = isSprinting ? PLAYER_SPRINT_SPEED : PLAYER_SPEED;

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) moveDir.y -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) moveDir.y += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) moveDir.x -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) moveDir.x += 1;

    if (Vector2LengthSqr(moveDir) > 0) {
        moveDir = Vector2Normalize(moveDir);
        Vector2 newPos = Vector2Add(position, Vector2Scale(moveDir, currentSpeed * GetFrameTime()));

        // Update rotation based on movement direction
        rotation = atan2f(moveDir.y, moveDir.x) * RAD2DEG;

        // Basic boundary collision (can be improved with map.IsPositionValid)
        if (newPos.x - PLAYER_RADIUS < 0) newPos.x = PLAYER_RADIUS;
        if (newPos.x + PLAYER_RADIUS > SCREEN_WIDTH) newPos.x = SCREEN_WIDTH - PLAYER_RADIUS;
        if (newPos.y - PLAYER_RADIUS < 0) newPos.y = PLAYER_RADIUS;
        if (newPos.y + PLAYER_RADIUS > SCREEN_HEIGHT) newPos.y = SCREEN_HEIGHT - PLAYER_RADIUS;
        
        // More robust collision check with map obstacles
        if (map.IsPositionValid(newPos, PLAYER_RADIUS)) {
             position = newPos;
        } else {
            // Try moving only X
            Vector2 newPosX = {newPos.x, position.y};
            if (map.IsPositionValid(newPosX, PLAYER_RADIUS)) position = newPosX;
            else {
                // Try moving only Y
                Vector2 newPosY = {position.x, newPos.y};
                if (map.IsPositionValid(newPosY, PLAYER_RADIUS)) position = newPosY;
            }
        }
    }
}

void Player::Update(float deltaTime, const Map& map, const std::vector<Hider>& hiders) {
    HandleInput(map);

    if (isSprinting) {
        sprintValue -= SPRINT_DEPLETE_RATE * deltaTime;
        if (sprintValue < 0) sprintValue = 0;
    } else {
        sprintValue += SPRINT_REGEN_RATE * deltaTime;
        if (sprintValue > SPRINT_MAX) sprintValue = SPRINT_MAX;
    }
    UpdateVision();

    // Alert symbol logic
    showAlert = false;
    Vector2 backDir = Vector2Rotate({-1, 0}, rotation * DEG2RAD); // Opposite to forward
    for (const auto& hider : hiders) {
        if (!hider.isTagged) {
            Vector2 toHider = Vector2Subtract(hider.position, position);
            float distToHider = Vector2Length(toHider);
            if (distToHider < ALERT_BEHIND_DISTANCE && distToHider > PLAYER_RADIUS + HIDER_RADIUS) { // Not too close (colliding)
                if (!IsInVisionCone(hider.position, PLAYER_VISION_CONE_ANGLE, PLAYER_VISION_RADIUS)) { // Not in front vision
                    // Check if hider is roughly behind
                    float angleToHider = Vector2Angle(backDir, Vector2Normalize(toHider)) * RAD2DEG;
                    if (fabsf(angleToHider) < ALERT_BEHIND_ANGLE_RANGE / 2.0f) {
                        showAlert = true;
                        break;
                    }
                }
            }
        }
    }
}

void Player::UpdateVision() {
    visionConePoints.clear();
    visionConePoints.push_back(position); // Apex of the cone

    float startAngle = rotation - PLAYER_VISION_CONE_ANGLE / 2.0f;
    float endAngle = rotation + PLAYER_VISION_CONE_ANGLE / 2.0f;

    // Add points along the arc of the vision cone for drawing
    int segments = 32; // Increased number of segments for smoother cone
    float angleStep = PLAYER_VISION_CONE_ANGLE / segments;
    
    // Add points in a way that creates a more natural cone shape
    for (int i = 0; i <= segments; ++i) {
        float currentAngle = startAngle + angleStep * i;
        float radius = PLAYER_VISION_RADIUS;
        
        // Create a slight curve in the cone by adjusting the radius
        if (i > 0 && i < segments) {
            float t = (float)i / segments;
            float curve = sinf(t * PI) * 0.1f; // Subtle curve effect
            radius *= (1.0f - curve);
        }
        
        Vector2 pointOnRadius = {
            position.x + radius * cosf(currentAngle * DEG2RAD),
            position.y + radius * sinf(currentAngle * DEG2RAD)
        };
        visionConePoints.push_back(pointOnRadius);
    }
}


void Player::Draw() {
    // Draw vision cone first (underneath player)
    if (visionConePoints.size() >= 3) {
        // Draw multiple layers of the cone for a gradient effect
        for (int i = 0; i < 3; i++) {
            float alpha = 0.8f - (i * 0.2f); // Decrease opacity for each layer
            if (alpha < 0) alpha = 0;
            
            // Scale the points slightly for each layer
            std::vector<Vector2> scaledPoints = visionConePoints;
            Vector2 center = visionConePoints[0]; // Player position
            for (size_t j = 1; j < scaledPoints.size(); j++) {
                Vector2 dir = Vector2Subtract(scaledPoints[j], center);
                scaledPoints[j] = Vector2Add(center, Vector2Scale(dir, 1.0f - (i * 0.1f)));
            }
            
            DrawTriangleFan(scaledPoints.data(), scaledPoints.size(), Fade(WHITE, alpha));
        }
    }
    
    // Draw Player
    DrawTexturePro(texture,
                   {0, 0, (float)texture.width, (float)texture.height}, // source rec
                   {position.x, position.y, PLAYER_RADIUS * 2, PLAYER_RADIUS * 2}, // dest rec
                   {PLAYER_RADIUS, PLAYER_RADIUS}, // origin
                   rotation, WHITE);

    // Draw Alert Symbol if active
    if (showAlert) {
        // Position alert icon slightly above the player
        Vector2 alertPos = { position.x - alertTexture.width / 2.0f, position.y - PLAYER_RADIUS - alertTexture.height - 5.0f };
        DrawTexture(alertTexture, (int)alertPos.x, (int)alertPos.y, WHITE);
    }
}


bool Player::IsInVisionCone(Vector2 targetPos, float coneAngle, float visionRadius) const {
    Vector2 toTarget = Vector2Subtract(targetPos, position);
    float distanceToTarget = Vector2Length(toTarget);

    if (distanceToTarget > visionRadius || distanceToTarget < 0.1f) { // Target too far or is self
        return false;
    }

    Vector2 forward = GetForwardVector();
    if (Vector2LengthSqr(forward) == 0) return false; // Should not happen if rotation is well-defined

    Vector2 normalizedToTarget = Vector2Normalize(toTarget);
    
    float dotProduct = Vector2DotProduct(forward, normalizedToTarget);
    float angleToTargetRad = acosf(dotProduct); // Angle in radians
    float angleToTargetDeg = angleToTargetRad * RAD2DEG;

    return angleToTargetDeg <= coneAngle / 2.0f;
}

// src/player.cpp

bool Player::CanTag(const Hider& hider) const {
    if (hider.isTagged) return false;

    float distanceToHider = Vector2Distance(position, hider.position);

    // Check if the hider's center is within the player's TAG_RANGE
    if (distanceToHider <= TAG_RANGE) {
        // Then check if the hider is within the player's vision cone
        return IsInVisionCone(hider.position, PLAYER_VISION_CONE_ANGLE, PLAYER_VISION_RADIUS);
    }
    return false;
}

bool Player::IsLookingAt(Vector2 targetPos) const {
    return IsInVisionCone(targetPos, PLAYER_VISION_CONE_ANGLE, PLAYER_VISION_RADIUS);
}