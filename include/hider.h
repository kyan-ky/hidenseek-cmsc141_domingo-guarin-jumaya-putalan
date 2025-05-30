#pragma once

#include "raylib.h"
#include "constants.h"
#include "game_state.h" // For GamePhase
#include <vector>

// Forward declarations
class Player;
class Map;

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
    Texture2D texture;

    HiderHidingFSMState hidingState;
    HiderSeekingFSMState seekingState;

    Hider();
    void Init(Vector2 startPos, const Map& gameMap);
    void Update(float deltaTime, GamePhase currentPhase, Player& player, const Map& gameMap, const std::vector<Hider>& otherHiders);
    void Draw();
    bool IsInVision(Vector2 targetPos) const;
    Vector2 GetForwardVector() const;


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

