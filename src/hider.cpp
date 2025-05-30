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
    float minDistance = HIDER_RADIUS * 4; // Minimum distance between hiders or player
    if (Vector2DistanceSqr(spot, player.position) < minDistance * minDistance) {
        return true;
    }
    for (const auto& other : otherHiders) {
        if (&other == this) continue; // Don't check against self
        if (Vector2DistanceSqr(spot, other.targetHidingSpot) < minDistance * minDistance || // if other is moving to it
            Vector2DistanceSqr(spot, other.position) < minDistance * minDistance) { // if other is at/near it
            return true;
        }
    }
    return false;
}

void Hider::Scout(const Map& gameMap, const Player& player, const std::vector<Hider>& otherHiders) {
    int attempts = 0;
    const int maxAttempts = 10; // Try a few times to find a unique spot
    do {
        targetHidingSpot = gameMap.GetRandomHidingSpot();
        attempts++;
    } while (IsSpotTaken(targetHidingSpot, otherHiders, player) && attempts < maxAttempts);
    
    // If after maxAttempts still no good spot, just pick one (could be improved)
    if (IsSpotTaken(targetHidingSpot, otherHiders, player)) {
         targetHidingSpot = gameMap.GetRandomHidingSpot();
    }

    hidingState = HiderHidingFSMState::MOVING_TO_HIDING_SPOT;
    // Update rotation to face target
    Vector2 direction = Vector2Normalize(Vector2Subtract(targetHidingSpot, position));
    if (Vector2LengthSqr(direction) > 0) {
        rotation = atan2f(direction.y, direction.x) * RAD2DEG;
    }
}

void Hider::MoveToHidingSpot(float deltaTime, const Map& gameMap) {
    if (Vector2Distance(position, targetHidingSpot) < HIDER_RADIUS / 2.0f) {
        position = targetHidingSpot; // Snap to spot
        hidingState = HiderHidingFSMState::HIDING;
    } else {
        Vector2 direction = Vector2Normalize(Vector2Subtract(targetHidingSpot, position));
        Vector2 newPos = Vector2Add(position, Vector2Scale(direction, speed * deltaTime));

        if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
             position = newPos;
        } else {
            // Basic obstacle avoidance: if stuck, maybe try to transition back to scouting or pick a new near spot
            // For now, just stop or get stuck. Could also try small random movements.
            hidingState = HiderHidingFSMState::HIDING; // Give up and hide where it is
        }
        // Update rotation to face target
        if (Vector2LengthSqr(direction) > 0) {
           rotation = atan2f(direction.y, direction.x) * RAD2DEG;
        }
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
        case HiderSeekingFSMState::ATTACKING:
            Attack(deltaTime, player, gameMap);
            break;
    }
}

void Hider::Idle(const Player& player, const Map& gameMap) {
    // Check for player
    if (IsInVision(player.position)) {
        float distToPlayer = Vector2Distance(position, player.position);
        if (distToPlayer < HIDER_ATTACK_RANGE && attackCooldownTimer <= 0) {
            seekingState = HiderSeekingFSMState::ATTACKING;
        } else if (distToPlayer < HIDER_VISION_RADIUS) { // Player is close enough to react
             seekingState = HiderSeekingFSMState::EVADING;
        }
    } else {
        // Optional: Slight random movement or rotation
        if (rand() % 100 < 2) { // Small chance to change rotation
            rotation += (float)(rand() % 90 - 45);
        }
    }
}

void Hider::Evade(float deltaTime, const Player& player, const Map& gameMap) {
    if (!IsInVision(player.position)) { // Player out of sight
        seekingState = HiderSeekingFSMState::IDLING;
        return;
    }

    // Try to attack if player gets too close during evasion
    if (Vector2Distance(position, player.position) < HIDER_ATTACK_RANGE && attackCooldownTimer <= 0) {
        seekingState = HiderSeekingFSMState::ATTACKING;
        return;
    }

    Vector2 directionAwayFromPlayer = Vector2Normalize(Vector2Subtract(position, player.position));
    Vector2 newPos = Vector2Add(position, Vector2Scale(directionAwayFromPlayer, speed * deltaTime));

    if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
        position = newPos;
    } else {
        // Basic obstacle handling: try to turn and move, or switch to attack if cornered
        rotation += 90.0f; // Turn 90 degrees
        // If still stuck, could transition to IDLING or even ATTACKING if desperate
    }

    // Update rotation to face away or direction of movement
    if (Vector2LengthSqr(directionAwayFromPlayer) > 0) {
        rotation = atan2f(directionAwayFromPlayer.y, directionAwayFromPlayer.x) * RAD2DEG;
    }
}

void Hider::Attack(float deltaTime, Player& player, const Map& gameMap) {
    if (attackCooldownTimer > 0) { // Should not happen if logic is correct, but as safeguard
        seekingState = HiderSeekingFSMState::IDLING; // Go back to idle if on cooldown
        return;
    }

    Vector2 directionToPlayer = Vector2Normalize(Vector2Subtract(player.position, position));
    Vector2 newPos = Vector2Add(position, Vector2Scale(directionToPlayer, speed * 1.2f * deltaTime)); // Slightly faster when attacking

    // Update rotation to face player
    if (Vector2LengthSqr(directionToPlayer) > 0) {
        rotation = atan2f(directionToPlayer.y, directionToPlayer.x) * RAD2DEG;
    }

    if (gameMap.IsPositionValid(newPos, HIDER_RADIUS)) {
        position = newPos;
    }
    // No complex obstacle avoidance during attack, assume direct charge

    // Check for tag (collision with player)
    if (CheckCollisionCircles(position, HIDER_RADIUS, player.position, PLAYER_RADIUS)) {
        // Game Manager will handle the "player tagged" logic
        // For now, hider sets its cooldown and goes to IDLE
        attackCooldownTimer = HIDER_ATTACK_COOLDOWN;
        seekingState = HiderSeekingFSMState::IDLING;
        // GameManager needs to check this collision and set player as "lost"
    }

    // If player moves out of effective attack range or vision
    if (!IsInVision(player.position) || Vector2Distance(position, player.position) > HIDER_ATTACK_RANGE * 1.5f) {
        seekingState = HiderSeekingFSMState::IDLING; // Give up attack
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

    // Optional: Draw Hider's vision cone for debugging
    // if (!isTagged) {
    //     std::vector<Vector2> visionPoints;
    //     visionPoints.push_back(position);
    //     float startAngle = rotation - HIDER_VISION_CONE_ANGLE / 2.0f;
    //     for (int i = 0; i <= 10; ++i) {
    //         float currentAngle = startAngle + (HIDER_VISION_CONE_ANGLE / 10) * i;
    //         visionPoints.push_back(Vector2Add(position, Vector2Scale(Vector2Rotate({1,0}, currentAngle*DEG2RAD), HIDER_VISION_RADIUS)));
    //     }
    //     DrawTriangleFan(visionPoints.data(), visionPoints.size(), Fade(PURPLE, 0.2f));
    // }
}

