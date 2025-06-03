#pragma once

#include "raylib.h"
#include "constants.h"
#include "game_state.h" // For GamePhase
#include <vector>

// Forward declarations
class Player;
class Map;
class GameManager; // Forward declaration for GameManager

enum class HiderHidingFSMState {
    SCOUTING,
    MOVING_TO_HIDING_SPOT,
    HIDING
};

enum class HiderSeekingFSMState {
    IDLING,
    EVADING,
    ATTACKING
};

class Hider {
public:
    Vector2 position;
    float rotation; // in degrees
    float speed;
    bool isTagged;
    float timeSinceLastTag = 0.0f;
    float timeSinceLastPlayerMovement = 0.0f;
    Vector2 lastPlayerPosition = {0, 0};
    Texture2D texture;
    Texture2D attackTexture; // New texture for attacking state
    int hiderId; // ID to identify which hider this is (0-4)
    GameManager* gameManager; // Pointer to game manager for sound access

    HiderHidingFSMState hidingState;
    HiderSeekingFSMState seekingState;

    Hider();
    void Init(Vector2 startPos, const Map& gameMap, int id = 0);
    void Update(float deltaTime, GamePhase currentPhase, Player& player, const Map& gameMap, const std::vector<Hider>& otherHiders);
    void Draw();
    bool IsInVision(Vector2 targetPos) const;
    Vector2 GetForwardVector() const;
    bool CanAttack(const Player& player) const;
    void AttemptTag(const Map& gameMap, Player& player);


private:
    Vector2 targetHidingSpot;
    float attackCooldownTimer;

    // Hiding Phase FSM Logic
    void UpdateHidingPhase(float deltaTime, const Map& gameMap, const Player& player, const std::vector<Hider>& otherHiders);
    void Scout(const Map& gameMap, const Player& player, const std::vector<Hider>& otherHiders);
    void MoveToHidingSpot(float deltaTime, const Map& gameMap);

    // Seeking Phase FSM Logic
    void UpdateSeekingPhase(float deltaTime, Player& player, const Map& gameMap);
    void Idle(const Player& player, const Map& gameMap);
    void Evade(float deltaTime, const Player& player, const Map& gameMap);
    void Attack(float deltaTime, Player& player, const Map& gameMap);

    bool IsSpotTaken(Vector2 spot, const std::vector<Hider>& otherHiders, const Player& player);
};

