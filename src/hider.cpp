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
    timeSinceLastTag += deltaTime;

    if (Vector2Distance(player.position, lastPlayerPosition) < 1.0f) {
        timeSinceLastPlayerMovement += deltaTime;
    } else {
        timeSinceLastPlayerMovement = 0.0f;
        lastPlayerPosition = player.position;
    }

    switch (seekingState) {
        case HiderSeekingFSMState::IDLING:
            Idle(player, gameMap);
            if (CanAttack(player)) {
                seekingState = HiderSeekingFSMState::ATTACKING;
            }
            break;
        case HiderSeekingFSMState::EVADING:
            Evade(deltaTime, player, gameMap);
            break;
        case HiderSeekingFSMState::ATTACKING:
            AttemptTag(gameMap, player);
            break;
    }
}

void Hider::Idle(const Player& player, const Map& gameMap) {
    // Always check for player's presence and vision first
    bool playerInVision = IsInVision(player.position);
    float distanceToPlayer = Vector2Distance(position, player.position);
    float collisionDistance = HIDER_RADIUS + PLAYER_RADIUS;
    
    // Check if player is in vision or too close
    if (playerInVision || distanceToPlayer < HIDER_VISION_RADIUS) {
        // Immediately transition to evading if there's a collision
        if (distanceToPlayer <= collisionDistance) {
            seekingState = HiderSeekingFSMState::EVADING;
            return;
        }
        
        // Check if player is looking at us
        if (player.IsLookingAt(position)) {
            seekingState = HiderSeekingFSMState::EVADING;
            return;
        }

        // If player is in vision but not looking at us, move around
        if (distanceToPlayer < HIDER_VISION_RADIUS) {
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
        } else {
            // If player is in vision but too far, start evading
            seekingState = HiderSeekingFSMState::EVADING;
        }
    }
    // When player is not in vision and far away, stay still but keep checking
}

void Hider::Evade(float deltaTime, const Player& player, const Map& gameMap) {
    // Check if we're stuck (can't move in any direction)
    bool isStuck = true;
    float angles[] = {0.0f, 45.0f, -45.0f, 90.0f, -90.0f, 135.0f, -135.0f, 180.0f};
    for (float angle : angles) {
        Vector2 testDir = Vector2Rotate({1, 0}, angle * DEG2RAD);
        Vector2 testPos = Vector2Add(position, Vector2Scale(testDir, speed * deltaTime));
        if (gameMap.IsPositionValid(testPos, HIDER_RADIUS)) {
            isStuck = false;
            break;
        }
    }

    // If we're stuck, switch to attacking immediately
    if (isStuck) {
        seekingState = HiderSeekingFSMState::ATTACKING;
        return;
    }

    // Check for nearby hiding spots
    const std::vector<Vector2>& availableSpots = gameMap.GetHidingSpots();
    float minDistanceToSpot = HIDER_VISION_RADIUS;
    Vector2 closestSpot = {0, 0};
    bool foundSpot = false;
    float bestScore = 0.0f;

    for (const auto& spot : availableSpots) {
        float distanceToSpot = Vector2Distance(position, spot);
        float distanceToPlayer = Vector2Distance(spot, player.position);
        
        // Calculate a score for this spot based on distance to player and distance to hider
        float score = (distanceToPlayer / HIDER_VISION_RADIUS) - (distanceToSpot / HIDER_VISION_RADIUS);
        
        // Only consider spots that are valid and have a good score
        if (score > bestScore && gameMap.IsPositionValid(spot, HIDER_RADIUS)) {
            bestScore = score;
            closestSpot = spot;
            foundSpot = true;
        }
    }

    // If we found a good hiding spot, move towards it
    if (foundSpot) {
        Vector2 directionToSpot = Vector2Normalize(Vector2Subtract(closestSpot, position));
        Vector2 newPos = Vector2Add(position, Vector2Scale(directionToSpot, speed * 1.2f * deltaTime));
        
        if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
            position = newPos;
            rotation = atan2f(directionToSpot.y, directionToSpot.x) * RAD2DEG;
            
            // If we're close enough to the spot, start hiding
            if (Vector2Distance(position, closestSpot) < HIDER_RADIUS * 2) {
                position = closestSpot; // Snap to spot
                seekingState = HiderSeekingFSMState::IDLING;
                return;
            }
        } else {
            // If we can't move directly to the spot, try to find a path around obstacles
            float angles[] = {45.0f, -45.0f, 90.0f, -90.0f};
            bool foundPath = false;
            
            for (float angle : angles) {
                Vector2 rotatedDir = Vector2Rotate(directionToSpot, angle * DEG2RAD);
                newPos = Vector2Add(position, Vector2Scale(rotatedDir, speed * 1.2f * deltaTime));
                
                if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
                    position = newPos;
                    rotation = atan2f(rotatedDir.y, rotatedDir.x) * RAD2DEG;
                    foundPath = true;
                    break;
                }
            }
            
            if (!foundPath) {
                // If we can't find a path to the spot, continue with normal evasion
                foundSpot = false;
            }
        }
    }

    // If no good spot found or we can't reach it, continue normal evasion
    if (!foundSpot) {
        // Calculate direction away from player
        Vector2 directionAwayFromPlayer = Vector2Normalize(Vector2Subtract(position, player.position));
        
        // Try to move away from player at increased speed
        Vector2 newPos = Vector2Add(position, Vector2Scale(directionAwayFromPlayer, speed * 1.2f * deltaTime));

        // If we can't move directly away, try multiple angles
        if (!gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
            // Try angles in both directions, starting with smaller angles
            float angles[] = {45.0f, -45.0f, 90.0f, -90.0f, 135.0f, -135.0f};
            bool foundValidMove = false;
            
            for (float angle : angles) {
                Vector2 rotatedDir = Vector2Rotate(directionAwayFromPlayer, angle * DEG2RAD);
                newPos = Vector2Add(position, Vector2Scale(rotatedDir, speed * 1.2f * deltaTime));
                
                if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
                    foundValidMove = true;
                    directionAwayFromPlayer = rotatedDir; // Update direction for rotation
                    break;
                }
            }
            
            // If no valid move found, try to move in the opposite direction of the player
            if (!foundValidMove) {
                Vector2 oppositeDir = Vector2Scale(directionAwayFromPlayer, -1.0f);
                newPos = Vector2Add(position, Vector2Scale(oppositeDir, speed * 0.8f * deltaTime));
                
                if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
                    directionAwayFromPlayer = oppositeDir;
                } else {
                    // If still stuck, switch to attacking
                    seekingState = HiderSeekingFSMState::ATTACKING;
                    return;
                }
            }
        }

        // If we found a valid position, move there
        if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
            position = newPos;
            // Update rotation to face away from player
            rotation = atan2f(directionAwayFromPlayer.y, directionAwayFromPlayer.x) * RAD2DEG;
        } else {
            // If we can't move, switch to attacking
            seekingState = HiderSeekingFSMState::ATTACKING;
            return;
        }
    }

    // Only return to idle if we're far enough from the player and not moving to a hiding spot
    if (!foundSpot) {
        float distanceToPlayer = Vector2Distance(position, player.position);
        if (distanceToPlayer > HIDER_VISION_RADIUS * 1.5f) {
            seekingState = HiderSeekingFSMState::IDLING;
        }
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

bool Hider::CanAttack(const Player& player) const {
    return (!player.IsLookingAt(position) &&
            timeSinceLastPlayerMovement > 2.0f &&
            timeSinceLastTag > 5.0f &&
            Vector2Distance(position, player.position) < HIDER_VISION_RADIUS);
}

void Hider::AttemptTag(const Map& gameMap, Player& player) {
    float distanceToPlayer = Vector2Distance(position, player.position);
    float collisionDistance = HIDER_RADIUS + PLAYER_RADIUS;

    // Always try to move towards player when attacking
    Vector2 direction = Vector2Normalize(Vector2Subtract(player.position, position));
    Vector2 newPos = Vector2Add(position, Vector2Scale(direction, speed * 1.2f * GetFrameTime()));
    
    // Try to move towards player
    if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
        position = newPos;
    } else {
        // If blocked, try angles to get around obstacles
        float angles[] = {45.0f, -45.0f, 90.0f, -90.0f};
        bool foundPath = false;
        
        for (float angle : angles) {
            Vector2 rotatedDir = Vector2Rotate(direction, angle * DEG2RAD);
            newPos = Vector2Add(position, Vector2Scale(rotatedDir, speed * 1.2f * GetFrameTime()));
            
            if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
                position = newPos;
                direction = rotatedDir;
                foundPath = true;
                break;
            }
        }
    }
    
    // Update rotation to face player
    rotation = atan2f(direction.y, direction.x) * RAD2DEG;

    // Check for successful tag
    if (distanceToPlayer <= collisionDistance) {
        // Tag successful
        TraceLog(LOG_INFO, "HIDER: Tag successful! Distance: %.2f", distanceToPlayer);
        player.SetTagged(true);
        timeSinceLastTag = 0.0f;
        seekingState = HiderSeekingFSMState::IDLING;
    }
}
