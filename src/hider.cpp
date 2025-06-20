#include "hider.h"
#include "player.h"
#include "map.h"
#include "game_manager.h"
#include "raymath.h"
#include <cstdlib> // For rand
#include <cmath>   // For atan2f, fabsf
#include <cstdio>  // For snprintf

Hider::Hider() : position({0, 0}), rotation(0.0f), speed(HIDER_SPEED), isTagged(false),
                 hidingState(HiderHidingFSMState::SCOUTING),
                 seekingState(HiderSeekingFSMState::IDLING),
                 attackCooldownTimer(0.0f), texture{0}, attackTexture{0}, hiderId(0),
                 gameManager(nullptr) { 
    // Textures will be loaded in Init
}

void Hider::Init(Vector2 startPos, const Map& gameMap, int id) {
    position = startPos;
    isTagged = false;
    hidingState = HiderHidingFSMState::SCOUTING;
    seekingState = HiderSeekingFSMState::IDLING;
    attackCooldownTimer = 0.0f;
    rotation = (float)(rand() % 360); // Random initial rotation
    hiderId = id;
    gameManager = nullptr; // Will be set by GameManager when needed

    // Load appropriate textures based on hider ID
    char standTextureName[32];
    char tagTextureName[32];
    
    if (hiderId == 0) {
        snprintf(standTextureName, sizeof(standTextureName), "hider_stand.png");
        snprintf(tagTextureName, sizeof(tagTextureName), "hider_tag.png");
    } else {
        snprintf(standTextureName, sizeof(standTextureName), "hider%d_stand.png", hiderId);
        snprintf(tagTextureName, sizeof(tagTextureName), "hider%d_tag.png", hiderId);
    }

    if (FileExists(standTextureName)) { 
        this->texture = LoadTexture(standTextureName);
    }

    if (FileExists(tagTextureName)) { 
        this->attackTexture = LoadTexture(tagTextureName);
    }
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
    // Increase minimum distance between hiders significantly
    float minDistance = HIDER_RADIUS * 15; // Increased from 10 to 15
    
    // Check distance from player with increased safety margin
    if (Vector2DistanceSqr(spot, player.position) < (PLAYER_RADIUS + HIDER_RADIUS + 100) * (PLAYER_RADIUS + HIDER_RADIUS + 100)) {
        return true;
    }
    
    // Check distance from other hiders with improved collision detection
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

        // Additional check for hiders that are moving
        if (other.hidingState == HiderHidingFSMState::MOVING_TO_HIDING_SPOT) {
            Vector2 otherDirection = Vector2Normalize(Vector2Subtract(other.targetHidingSpot, other.position));
            Vector2 projectedPosition = Vector2Add(other.position, Vector2Scale(otherDirection, HIDER_RADIUS * 10));
            if (Vector2DistanceSqr(spot, projectedPosition) < minDistance * minDistance) {
                return true;
            }
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
    // Check for nearby hiding spots
    const std::vector<Vector2>& availableSpots = gameMap.GetHidingSpots();
    float minDistanceToSpot = HIDER_VISION_RADIUS;
    Vector2 closestSpot = {0, 0};
    bool foundSpot = false;
    float bestScore = 0.0f;

    for (const auto& spot : availableSpots) {
        float distanceToSpot = Vector2Distance(position, spot);
        
        // Calculate a score for this spot based on distance to hider
        float score = 1.0f - (distanceToSpot / HIDER_VISION_RADIUS);
        
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
                hidingState = HiderHidingFSMState::HIDING;
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
                // If we can't find a path to the spot, go back to scouting
                hidingState = HiderHidingFSMState::SCOUTING;
                return;
            }
        }
    } else {
        // If no good spot found, go back to scouting
        hidingState = HiderHidingFSMState::SCOUTING;
        return;
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
    // Check for direct collision first
    float distanceToPlayer = Vector2Distance(position, player.position);
    float collisionDistance = HIDER_RADIUS + PLAYER_RADIUS;
    if (distanceToPlayer <= collisionDistance) {
        seekingState = HiderSeekingFSMState::EVADING;
        return;
    }

    // Check if we're at a hiding spot
    bool isAtHidingSpot = false;
    Vector2 currentSpot = {0, 0};
    for (const auto& spot : gameMap.GetHidingSpots()) {
        if (Vector2Distance(position, spot) < HIDER_RADIUS * 2) {
            isAtHidingSpot = true;
            currentSpot = spot;
            break;
        }
    }

    // Check for very close proximity to player (0.1f)
    if (distanceToPlayer <= 0.1f) {
        seekingState = HiderSeekingFSMState::EVADING;
        return;
    }

    // If we're at a hiding spot
    if (isAtHidingSpot) {
        // Check if player is inside our current hiding spot
        float distanceToSpot = Vector2Distance(player.position, currentSpot);
        if (distanceToSpot < HIDER_RADIUS * 2) {
            // Calculate direction away from player
            Vector2 directionAwayFromPlayer = Vector2Normalize(Vector2Subtract(position, player.position));
            
            // Try to move away from player at increased speed
            Vector2 newPos = Vector2Add(position, Vector2Scale(directionAwayFromPlayer, speed * 1.2f * GetFrameTime()));

            // If we can't move directly away, try multiple angles
            if (!gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
                // Try angles in both directions, starting with smaller angles
                float angles[] = {45.0f, -45.0f, 90.0f, -90.0f, 135.0f, -135.0f};
                bool foundValidMove = false;
                
                for (float angle : angles) {
                    Vector2 rotatedDir = Vector2Rotate(directionAwayFromPlayer, angle * DEG2RAD);
                    newPos = Vector2Add(position, Vector2Scale(rotatedDir, speed * 1.2f * GetFrameTime()));
                    
                    if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
                        foundValidMove = true;
                        directionAwayFromPlayer = rotatedDir; // Update direction for rotation
                        break;
                    }
                }
                
                // If no valid move found, try to move in the opposite direction of the player
                if (!foundValidMove) {
                    Vector2 oppositeDir = Vector2Scale(directionAwayFromPlayer, -1.0f);
                    newPos = Vector2Add(position, Vector2Scale(oppositeDir, speed * 0.8f * GetFrameTime()));
                    
                    if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
                        directionAwayFromPlayer = oppositeDir;
                    } else {
                        // If still stuck, switch to evading
                        seekingState = HiderSeekingFSMState::EVADING;
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
                // If we can't move, switch to evading
                seekingState = HiderSeekingFSMState::EVADING;
                return;
            }
            return;
        }
        // Stay still at hiding spot if player is not inside it
        return;
    }

    // If not at a hiding spot, use normal idle behavior
    bool playerInVision = IsInVision(player.position);
    
    // Check if player is in vision or too close
    if (playerInVision || distanceToPlayer < HIDER_VISION_RADIUS) {
        // Check if player is looking at us
        if (player.IsLookingAt(position)) {
            seekingState = HiderSeekingFSMState::EVADING;
            return;
        }

        static float alertTimer = 0.0f;
        // If player is in vision but not looking at us, check for alert status
        if (player.IsInAlertStatus()) {
            alertTimer += GetFrameTime();
            
            if (alertTimer >= 1.5f) {
                seekingState = HiderSeekingFSMState::ATTACKING;
                alertTimer = 0.0f;
                return;
            }
        } else {
            // Reset timer if player is not in alert status
            alertTimer = 0.0f;
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
    // Check if player is in alert status
    static float alertTimer = 0.0f;
    if (player.IsInAlertStatus()) {
        alertTimer += GetFrameTime();
        
        if (alertTimer >= 1.5f) {
            seekingState = HiderSeekingFSMState::ATTACKING;
            alertTimer = 0.0f;
            return;
        }
    } else {
        // Reset timer if player is not in alert status
        alertTimer = 0.0f;
    }

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

    // Each hider has a unique evasion pattern based on their ID
    float evasionAngle = 0.0f;
    float evasionSpeed = speed;
    
    // Use hiderId to create unique behavior patterns
    switch (hiderId % 4) {
        case 0: // Zigzag pattern
            evasionAngle = (float)((int)(GetTime() * 2) % 2) * 45.0f - 22.5f;
            evasionSpeed = speed * 1.2f;
            break;
        case 1: // Circular pattern
            evasionAngle = GetTime() * 90.0f;
            evasionSpeed = speed * 0.9f;
            break;
        case 2: // Sharp turns
            evasionAngle = (float)((int)(GetTime() * 3) % 2) * 90.0f - 45.0f;
            evasionSpeed = speed * 1.1f;
            break;
        case 3: // Erratic movement
            evasionAngle = (float)(rand() % 360);
            evasionSpeed = speed * (0.8f + (float)(rand() % 40) / 100.0f);
            break;
    }

    // Calculate base direction away from player
    Vector2 directionAwayFromPlayer = Vector2Normalize(Vector2Subtract(position, player.position));
    
    // Apply the unique evasion pattern
    Vector2 evasionDirection = Vector2Rotate(directionAwayFromPlayer, evasionAngle * DEG2RAD);
    
    // Add some randomness to prevent synchronized movement
    float randomVariation = (float)(rand() % 20 - 10) / 100.0f;
    evasionDirection = Vector2Rotate(evasionDirection, randomVariation * DEG2RAD);
    
    // Try to move in the calculated direction
    Vector2 newPos = Vector2Add(position, Vector2Scale(evasionDirection, evasionSpeed * deltaTime));
    
    // If the new position is invalid, try alternative directions
    if (!gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
        float alternativeAngles[] = {30.0f, -30.0f, 60.0f, -60.0f, 90.0f, -90.0f};
        bool foundValidMove = false;
        
        for (float angle : alternativeAngles) {
            Vector2 alternativeDir = Vector2Rotate(evasionDirection, angle * DEG2RAD);
            newPos = Vector2Add(position, Vector2Scale(alternativeDir, evasionSpeed * deltaTime));
            
            if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
                evasionDirection = alternativeDir;
                foundValidMove = true;
                break;
            }
        }
        
        if (!foundValidMove) {
            // If no valid move found, try moving in the opposite direction
            Vector2 oppositeDir = Vector2Scale(evasionDirection, -1.0f);
            newPos = Vector2Add(position, Vector2Scale(oppositeDir, evasionSpeed * 0.8f * deltaTime));
            
            if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
                evasionDirection = oppositeDir;
            } else {
                // If still stuck, switch to attacking
                seekingState = HiderSeekingFSMState::ATTACKING;
                return;
            }
        }
    }

    // Update position and rotation
    if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
        position = newPos;
        rotation = atan2f(evasionDirection.y, evasionDirection.x) * RAD2DEG;
    }

    // Check if we should return to idle state
    float distanceToPlayer = Vector2Distance(position, player.position);
    if (distanceToPlayer > HIDER_VISION_RADIUS * 1.5f) {
        seekingState = HiderSeekingFSMState::IDLING;
    }
}

void Hider::Draw() {
    if (this->isTagged) {
        if (texture.id > 0 && texture.width > 0 && texture.height > 0) { // Added width/height check
            Rectangle sourceRec = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
            Rectangle destRec = { position.x, position.y, HIDER_RADIUS * 2, HIDER_RADIUS * 2 };
            Vector2 origin = { HIDER_RADIUS, HIDER_RADIUS };
            DrawTexturePro(texture, sourceRec, destRec, origin, rotation, WHITE);
        } else {
            DrawCircleV(position, HIDER_RADIUS, RED);
        }
    } else {
        // Choose the appropriate texture based on state
        Texture2D currentTexture = texture;
        if (seekingState == HiderSeekingFSMState::ATTACKING && attackTexture.id > 0) {
            currentTexture = attackTexture;
        }

        if (currentTexture.id > 0 && currentTexture.width > 0 && currentTexture.height > 0) {
            Rectangle sourceRec = { 0.0f, 0.0f, (float)currentTexture.width, (float)currentTexture.height };
            Rectangle destRec = { position.x, position.y, HIDER_RADIUS * 2, HIDER_RADIUS * 2 };
            Vector2 origin = { HIDER_RADIUS, HIDER_RADIUS };
            DrawTexturePro(currentTexture, sourceRec, destRec, origin, rotation, WHITE);
        } else {
            DrawCircleV(position, HIDER_RADIUS, BLUE);
        }

        // Draw vision cone for debugging
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
        player.SetTagged(true);
        timeSinceLastTag = 0.0f;
        seekingState = HiderSeekingFSMState::IDLING;
        
        // Play tag sound through game manager
        if (player.gameManager->tagSound.frameCount > 0) {
            PlaySound(player.gameManager->tagSound);
        }
    }
}
