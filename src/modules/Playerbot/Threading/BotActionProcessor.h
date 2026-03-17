/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_BOT_ACTION_PROCESSOR_H
#define PLAYERBOT_BOT_ACTION_PROCESSOR_H

#include "BotAction.h"
#include "BotActionQueue.h"
#include "Define.h"

class Player;
class Unit;
class Creature;
class GameObject;

namespace Playerbot
{

/**
 * @class BotActionProcessor
 * @brief Executes bot actions on main thread with full Map access
 *
 * CRITICAL DESIGN:
 * This class runs ONLY on the main thread during World::Update().
 * It has full access to Map, ObjectAccessor, and all game state.
 * Actions are pre-validated by worker threads using snapshot data.
 *
 * EXECUTION FLOW:
 * 1. ProcessActions() called from World::Update() (main thread)
 * 2. Pop actions from queue until empty
 * 3. Convert GUIDs → pointers using ObjectAccessor (thread-safe on main thread)
 * 4. Execute action on game state
 * 5. Log results
 *
 * PERFORMANCE:
 * - Target: <1ms per 100 actions
 * - Most actions are fast pointer lookups + method calls
 * - No lock contention (single consumer)
 */
class TC_GAME_API BotActionProcessor
{
public:
    explicit BotActionProcessor(BotActionQueue& queue) : _queue(queue), _actionsThisFrame(0) {}
    ~BotActionProcessor() = default;

    /**
     * @brief Process all pending actions (main thread only!)
     *
     * Called from World::Update() after bot worker threads complete.
     * Processes actions until queue is empty or frame budget exhausted.
     *
     * @param maxActionsPerFrame Max actions to process (prevents frame spikes)
     * @return Number of actions processed
     */
    uint32 ProcessActions(uint32 maxActionsPerFrame = 1000);

private:
    /**
     * @brief Execute single action on main thread
     *
     * Converts GUIDs → pointers and executes action on game state.
     * Returns result for logging/statistics.
     */
    BotActionResult ExecuteAction(BotAction const& action);

    // Action executors (one per action type)
    BotActionResult ExecuteAttackTarget(Player* bot, BotAction const& action);
    BotActionResult ExecuteCastSpell(Player* bot, BotAction const& action);
    BotActionResult ExecuteStopAttack(Player* bot, BotAction const& action);
    BotActionResult ExecuteMoveToPosition(Player* bot, BotAction const& action);
    BotActionResult ExecuteFollowTarget(Player* bot, BotAction const& action);
    BotActionResult ExecuteStopMovement(Player* bot, BotAction const& action);
    BotActionResult ExecuteInteractObject(Player* bot, BotAction const& action);
    BotActionResult ExecuteInteractNPC(Player* bot, BotAction const& action);
    BotActionResult ExecuteLootObject(Player* bot, BotAction const& action);
    BotActionResult ExecuteAcceptQuest(Player* bot, BotAction const& action);
    BotActionResult ExecuteTurnInQuest(Player* bot, BotAction const& action);
    BotActionResult ExecuteUseItem(Player* bot, BotAction const& action);
    BotActionResult ExecuteEquipItem(Player* bot, BotAction const& action);
    BotActionResult ExecuteSendChatMessage(Player* bot, BotAction const& action);
    BotActionResult ExecuteEnterVehicle(Player* bot, BotAction const& action);
    BotActionResult ExecuteExitVehicle(Player* bot, BotAction const& action);

    // Helper methods
    Player* GetBot(ObjectGuid guid);
    Unit* GetUnit(Player* bot, ObjectGuid guid);
    Creature* GetCreature(Player* bot, ObjectGuid guid);
    GameObject* GetGameObject(Player* bot, ObjectGuid guid);

private:
    BotActionQueue& _queue;
    uint32 _actionsThisFrame;
};

} // namespace Playerbot

#endif // PLAYERBOT_BOT_ACTION_PROCESSOR_H
