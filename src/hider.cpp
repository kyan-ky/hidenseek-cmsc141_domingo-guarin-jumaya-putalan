#include "hider.h"
#include "player.h"
#include "map.h"
#include "raymath.h"
#include <cstdlib> // For rand
#include <cmath>   // For atan2f, fabsf

Hider::Hider() : position({0, 0}), rotation(0.0f), speed(HIDER_SPEED), isTagged(false),
                 hidingState(HiderHidingFSMState::SCOUTING),
                 seekingState(HiderSeekingFSMState::IDLING),
                 attackCooldownTimer(0.0f) {
    // Attempt to load texture, use placeholder if fails
    if (FileExists("hider_sprite.jpg")) {
        texture = LoadTexture("hider_sprite.jpg");
    } else {
        Image img = GenImageColor((int)HIDER_RADIUS * 2, (int)HIDER_RADIUS * 2, HIDER_COLOR);
        texture = LoadTextureFromImage(img);
        UnloadImage(img);
    }
}

void Hider::Init(Vector2 startPos, const Map& gameMap) {
    position = startPos;
    isTagged = false;
    hidingState = HiderHidingFSMState::SCOUTING;
    seekingState = HiderSeekingFSMState::IDLING;
    attackCooldownTimer = 0.0f;
    rotation = (float)(rand() % 360); // Random initial rotation
    // Potentially scout for an initial hiding spot right away if needed for Hiding Phase
    // Scout(gameMap); // Let game manager call scout
}

Vector2 Hider::GetForwardVector() const {
    return Vector2Rotate({1, 0}, rotation * DEG2RAD);
}

bool Hider::IsInVision(Vector2 targetPos) const {
    Vector2 toTarget = Vector2Subtract(targetPos, position);
    float distanceToTarget = Vector2Length(toTarget);

    if (distanceToTarget > HIDER_VISION_RADIUS || distanceToTarget < 0.1f) {
        return false;
    }

    Vector2 forward = GetForwardVector();
    if (Vector2LengthSqr(forward) == 0) return false;

    Vector2 normalizedToTarget = Vector2Normalize(toTarget);
    float dotProduct = Vector2DotProduct(forward, normalizedToTarget);
    float angleToTargetRad = acosf(dotProduct);
    float angleToTargetDeg = angleToTargetRad * RAD2DEG;

    return angleToTargetDeg <= HIDER_VISION_CONE_ANGLE / 2.0f;
}


void Hider::Update(float deltaTime, GamePhase currentPhase, Player& player, const Map& gameMap, const std::vector<Hider>& otherHiders) {
    if (isTagged) return;

    if (currentPhase == GamePhase::HIDING) {
        UpdateHidingPhase(deltaTime, gameMap, player, otherHiders);
    } else if (currentPhase == GamePhase::SEEKING) {
        UpdateSeekingPhase(deltaTime, player, gameMap);
    }

    if (attackCooldownTimer > 0) {
        attackCooldownTimer -= deltaTime;
    }
}

// --- HIDING PHASE FSM ---
void Hider::UpdateHidingPhase(float deltaTime, const Map& gameMap, const Player& player, const std::vector<Hider>& otherHiders) {
    switch (hidingState) {
        case HiderHidingFSMState::SCOUTING:
            Scout(gameMap, player, otherHiders);
            break;
        case HiderHidingFSMState::MOVING_TO_HIDING_SPOT:
            MoveToHidingSpot(deltaTime, gameMap);
            break;
        case HiderHidingFSMState::HIDING:
            // Stay still, maybe slight animation if you add one
            break;
    }
}

bool Hider::IsSpotTaken(Vector2 spot, const std::vector<Hider>& otherHiders, const Player& player) {
    // Minimum distance between hiders (increased to prevent them from hiding too close to each other)
    float minDistance = HIDER_RADIUS * 10; // Minimum distance between hiders
    
    // Check distance from player
    if (Vector2DistanceSqr(spot, player.position) < (PLAYER_RADIUS + HIDER_RADIUS + 50) * (PLAYER_RADIUS + HIDER_RADIUS + 50)) {
        return true;
    }
    
    // Check distance from other hiders
    for (const auto& other : otherHiders) {
        if (&other == this) continue; // Don't check against self
        
        // Check if other hider is moving to this spot
        if (Vector2DistanceSqr(spot, other.targetHidingSpot) < minDistance * minDistance) {
            return true;
        }
        
        // Check if other hider is already at/near this spot
        if (Vector2DistanceSqr(spot, other.position) < minDistance * minDistance) {
            return true;
        }
    }
    return false;
}

void Hider::Scout(const Map& gameMap, const Player& player, const std::vector<Hider>& otherHiders) {
    // Get all available hiding spots from the map
    const std::vector<Vector2>& availableSpots = gameMap.GetHidingSpots();
    
    // Try to find an unoccupied hiding spot
    for (const auto& spot : availableSpots) {
        // First check if the spot is valid (not in an obstacle)
        if (!gameMap.IsPositionValid(spot, HIDER_RADIUS)) {
            continue; // Skip invalid spots
        }

        // Check if this spot is already taken by another hider
        bool spotIsTaken = IsSpotTaken(spot, otherHiders, player);
        
        // If this spot is free and valid, take it
        if (!spotIsTaken) {
            targetHidingSpot = spot;
            hidingState = HiderHidingFSMState::MOVING_TO_HIDING_SPOT;
            
            // Update rotation to face target
            Vector2 direction = Vector2Normalize(Vector2Subtract(targetHidingSpot, position));
            if (Vector2LengthSqr(direction) > 0) {
                rotation = atan2f(direction.y, direction.x) * RAD2DEG;
            }
            return;
        }
    }
    
    // If no valid spots are found, move randomly in open space
    static float randomMovementTimer = 0.0f;
    static float randomMovementInterval = 1.0f; // Change direction every second
    static Vector2 currentRandomDirection = {0, 0};
    
    // Update random movement timer
    randomMovementTimer += GetFrameTime();
    
    // Change direction periodically or if we hit an obstacle
    if (randomMovementTimer >= randomMovementInterval || Vector2LengthSqr(currentRandomDirection) == 0) {
        randomMovementTimer = 0.0f;
        float randomAngle = (float)(rand() % 360) * DEG2RAD;
        currentRandomDirection = Vector2Rotate({1, 0}, randomAngle);
        rotation = randomAngle * RAD2DEG;
    }
    
    // Move in the current random direction
    Vector2 newPos = Vector2Add(position, Vector2Scale(currentRandomDirection, speed * 0.5f * GetFrameTime()));
    
    if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
        position = newPos;
    } else {
        // If we hit an obstacle, immediately change direction
        randomMovementTimer = randomMovementInterval; // Force direction change on next frame
        currentRandomDirection = {0, 0}; // Force new direction calculation
    }
}

void Hider::MoveToHidingSpot(float deltaTime, const Map& gameMap) {
    // Verify the target spot is still valid
    if (!gameMap.IsPositionValid(targetHidingSpot, HIDER_RADIUS)) {
        hidingState = HiderHidingFSMState::SCOUTING;
        return;
    }

    // Calculate distance to target
    float distanceToTarget = Vector2Distance(position, targetHidingSpot);

    // Snap to exact position when very close
    if (distanceToTarget < 2.0f) {
        // Double check if the final position is valid before snapping
        if (gameMap.IsPositionValid(targetHidingSpot, HIDER_RADIUS)) {
            position = targetHidingSpot; // Snap exactly to spot
            hidingState = HiderHidingFSMState::HIDING;
        } else {
            // If the final position is invalid, go back to scouting
            hidingState = HiderHidingFSMState::SCOUTING;
            return;
        }
    }

    // Calculate direction to target
    Vector2 direction = Vector2Normalize(Vector2Subtract(targetHidingSpot, position));
    
    // Calculate movement for this frame
    float moveDistance = speed * deltaTime;
    
    // Try smaller steps if we're close to obstacles
    const int numSteps = 4; // Number of steps to check
    bool validMove = false;
    Vector2 finalPos = position;
    
    for (int i = 1; i <= numSteps; i++) {
        float stepDistance = moveDistance * (i / (float)numSteps);
        Vector2 testPos = Vector2Add(position, Vector2Scale(direction, stepDistance));
        
        if (gameMap.IsPositionValid(testPos, HIDER_RADIUS)) {
            finalPos = testPos;
            validMove = true;
        } else {
            break; // Stop if we hit an obstacle
        }
    }
    
    if (validMove) {
        position = finalPos;
    } else {
        // If we hit an obstacle, try to find a path around it
        // For now, just go back to scouting
        hidingState = HiderHidingFSMState::SCOUTING;
        return;
    }
    
    // Update rotation to face target
    if (Vector2LengthSqr(direction) > 0) {
        rotation = atan2f(direction.y, direction.x) * RAD2DEG;
    }
}


// --- SEEKING PHASE FSM ---
void Hider::UpdateSeekingPhase(float deltaTime, Player& player, const Map& gameMap) {
    switch (seekingState) {
        case HiderSeekingFSMState::IDLING:
            Idle(player, gameMap);
            break;
        case HiderSeekingFSMState::EVADING:
            Evade(deltaTime, player, gameMap);
            break;
    }
}

void Hider::Idle(const Player& player, const Map& gameMap) {
    // Check for player
    if (IsInVision(player.position)) {
        float distToPlayer = Vector2Distance(position, player.position);
        if (distToPlayer < HIDER_VISION_RADIUS) { // Player is close enough to react
            // Move around the player in a circular pattern
            Vector2 toPlayer = Vector2Subtract(player.position, position);
            float angleToPlayer = atan2f(toPlayer.y, toPlayer.x) * RAD2DEG;
            
            // Calculate a perpendicular direction to circle around the player
            float circleAngle = angleToPlayer + 90.0f; // 90 degrees perpendicular
            Vector2 circleDirection = Vector2Rotate({1, 0}, circleAngle * DEG2RAD);
            
            // Move in the circular pattern
            Vector2 newPos = Vector2Add(position, Vector2Scale(circleDirection, speed * 0.7f * GetFrameTime()));
            
            if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
                position = newPos;
                rotation = circleAngle; // Face the direction of movement
            } else {
                // If we hit an obstacle, try the opposite direction
                circleAngle = angleToPlayer - 90.0f;
                circleDirection = Vector2Rotate({1, 0}, circleAngle * DEG2RAD);
                newPos = Vector2Add(position, Vector2Scale(circleDirection, speed * 0.7f * GetFrameTime()));
                
                if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
                    position = newPos;
                    rotation = circleAngle;
                } else {
                    // If both directions are blocked, go back to evading
                    seekingState = HiderSeekingFSMState::EVADING;
                }
            }
        }
    }
    // When player is not in vision, stay still
}

void Hider::Evade(float deltaTime, const Player& player, const Map& gameMap) {
    if (!IsInVision(player.position)) { // Player out of sight
        seekingState = HiderSeekingFSMState::IDLING;
        return;
    }

    Vector2 directionAwayFromPlayer = Vector2Normalize(Vector2Subtract(position, player.position));
    Vector2 newPos = Vector2Add(position, Vector2Scale(directionAwayFromPlayer, speed * deltaTime));

    if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
        position = newPos;
    } else {
        // Basic obstacle handling: try to turn and move
        rotation += 90.0f; // Turn 90 degrees
    }

    // Update rotation to face away or direction of movement
    if (Vector2LengthSqr(directionAwayFromPlayer) > 0) {
        rotation = atan2f(directionAwayFromPlayer.y, directionAwayFromPlayer.x) * RAD2DEG;
    }
}

void Hider::Draw() {
    Color hiderDrawColor = isTagged ? HIDER_TAGGED_COLOR : HIDER_COLOR;
    if (texture.id > 0) {
         DrawTexturePro(texture,
                   {0, 0, (float)texture.width, (float)texture.height},
                   {position.x, position.y, HIDER_RADIUS * 2, HIDER_RADIUS * 2},
                   {HIDER_RADIUS, HIDER_RADIUS}, // origin
                   rotation, isTagged ? Fade(WHITE, 0.5f) : WHITE);
    } else {
        DrawCircleV(position, HIDER_RADIUS, hiderDrawColor);
        // Draw a line for direction
        Vector2 forward = GetForwardVector();
        DrawLineV(position, Vector2Add(position, Vector2Scale(forward, HIDER_RADIUS)), BLACK);
    }
}