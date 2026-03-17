/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "DungeonTypes.h"  // Canonical enum definitions (DungeonRole, MechanicType, etc.)
#include "Player.h"
#include "Creature.h"
#include "InstanceScript.h"
#include "Position.h"
#include <string>
#include <vector>

namespace Playerbot
{

// DungeonRole and MechanicType are now defined in DungeonTypes.h

/**
 * @brief Base class for all dungeon scripts
 *
 * This class provides a plugin-style interface for dungeon-specific AI behavior.
 * Each dungeon can override specific mechanics while falling back to generic
 * implementations for common behaviors.
 *
 * ARCHITECTURE:
 * - Base class provides DEFAULT implementations that call generic mechanics
 * - Derived classes OVERRIDE only what they need
 * - If no script exists, system uses generic fallback directly
 *
 * USAGE EXAMPLE:
 * class DeadminesScript : public DungeonScript
 * {
 * public:
 *     DeadminesScript() : DungeonScript("deadmines", 36) {}
 *
 *     void OnBossEngage(::Player* player, ::Creature* boss) override
 *     {
 *         if (boss->GetEntry() == 647) // Captain Greenskin
 *             HandleGreenskin(player, boss);
 *     }
 * };
 *
 * REGISTRATION:
 * void AddSC_deadmines_playerbot()
 * {
 *     DungeonScriptMgr::instance()->RegisterScript(new DeadminesScript());
 * }
 */
class TC_GAME_API DungeonScript
{
public:
    /**
     * Constructor
     * @param name Script name (e.g., "deadmines")
     * @param mapId Map ID for this dungeon
     */
    explicit DungeonScript(char const* name, uint32 mapId);
    virtual ~DungeonScript() = default;

    // ============================================================================
    // LIFECYCLE HOOKS
    // ============================================================================

    /**
     * Called when player enters dungeon
     * DEFAULT: No action
     * OVERRIDE: Dungeon-specific initialization
     */
    virtual void OnDungeonEnter(::Player* player, ::InstanceScript* instance);

    /**
     * Called when player exits dungeon
     * DEFAULT: No action
     * OVERRIDE: Cleanup logic
     */
    virtual void OnDungeonExit(::Player* player);

    /**
     * Called periodically during dungeon (every 1-5 seconds)
     * DEFAULT: No action
     * OVERRIDE: Continuous dungeon mechanics (e.g., patrol checks)
     */
    virtual void OnUpdate(::Player* player, uint32 diff);

    // ============================================================================
    // BOSS HOOKS
    // ============================================================================

    /**
     * Called when boss is engaged
     * DEFAULT: No action
     * OVERRIDE: Boss-specific setup
     */
    virtual void OnBossEngage(::Player* player, ::Creature* boss);

    /**
     * Called when boss is killed
     * DEFAULT: No action
     * OVERRIDE: Post-boss cleanup
     */
    virtual void OnBossKill(::Player* player, ::Creature* boss);

    /**
     * Called when group wipes on boss
     * DEFAULT: No action
     * OVERRIDE: Wipe recovery logic
     */
    virtual void OnBossWipe(::Player* player, ::Creature* boss);

    // ============================================================================
    // MECHANIC HANDLERS (Virtual - can be overridden)
    // ============================================================================

    /**
     * Handle interrupt priority for boss spells
     * DEFAULT: Generic interrupt logic (heals > damage > CC)
     * OVERRIDE: Boss-specific interrupt priorities
     *
     * @param player Player performing interrupt
     * @param boss Boss casting spell
     */
    virtual void HandleInterruptPriority(::Player* player, ::Creature* boss);

    /**
     * Handle ground effect avoidance
     * DEFAULT: Generic ground effect detection and movement
     * OVERRIDE: Boss-specific ground patterns
     *
     * @param player Player avoiding ground effects
     * @param boss Boss creating ground effects
     */
    virtual void HandleGroundAvoidance(::Player* player, ::Creature* boss);

    /**
     * Handle add kill priority
     * DEFAULT: Generic add priority (healers > casters > low health)
     * OVERRIDE: Boss-specific add kill order
     *
     * @param player Player targeting adds
     * @param boss Boss with adds
     */
    virtual void HandleAddPriority(::Player* player, ::Creature* boss);

    /**
     * Handle player positioning
     * DEFAULT: Generic positioning (tank front, DPS behind, healer range)
     * OVERRIDE: Boss-specific positioning requirements
     *
     * @param player Player to position
     * @param boss Boss being fought
     */
    virtual void HandlePositioning(::Player* player, ::Creature* boss);

    /**
     * Handle dispel mechanics
     * DEFAULT: Generic dispel priority (harmful > helpful)
     * OVERRIDE: Boss-specific dispel requirements
     *
     * @param player Player with dispel
     * @param boss Boss fight
     */
    virtual void HandleDispelMechanic(::Player* player, ::Creature* boss);

    /**
     * Handle movement mechanics (e.g., kiting, running out)
     * DEFAULT: Generic movement (stay at optimal range)
     * OVERRIDE: Boss-specific movement requirements
     *
     * @param player Player moving
     * @param boss Boss fight
     */
    virtual void HandleMovementMechanic(::Player* player, ::Creature* boss);

    /**
     * Handle tank swap mechanics
     * DEFAULT: No tank swap
     * OVERRIDE: Bosses requiring tank swaps
     *
     * @param player Tank player
     * @param boss Boss requiring tank swap
     */
    virtual void HandleTankSwap(::Player* player, ::Creature* boss);

    /**
     * Handle spread mechanic
     * DEFAULT: Players spread 10 yards apart
     * OVERRIDE: Boss-specific spread distance
     *
     * @param player Player spreading
     * @param boss Boss fight
     */
    virtual void HandleSpreadMechanic(::Player* player, ::Creature* boss);

    /**
     * Handle stack mechanic
     * DEFAULT: Players stack on designated position
     * OVERRIDE: Boss-specific stack location
     *
     * @param player Player stacking
     * @param boss Boss fight
     */
    virtual void HandleStackMechanic(::Player* player, ::Creature* boss);

    // ============================================================================
    // UTILITY METHODS (Non-virtual - available to all scripts)
    // ============================================================================

    /**
     * Get player's role in dungeon
     */
    DungeonRole GetPlayerRole(::Player* player) const;

    /**
     * Get all adds in combat with boss
     */
    ::std::vector<::Creature*> GetAddsInCombat(::Player* player, ::Creature* boss) const;

    /**
     * Find creature by entry near player
     */
    ::Creature* FindCreatureNearby(::Player* player, uint32 entry, float range) const;

    /**
     * Check if player has interrupt available
     */
    bool HasInterruptAvailable(::Player* player) const;

    /**
     * Use interrupt spell on target
     */
    bool UseInterruptSpell(::Player* player, ::Creature* target) const;

    /**
     * Check if ground effect is dangerous
     */
    bool IsDangerousGroundEffect(::DynamicObject* obj) const;

    /**
     * Move away from ground effect
     */
    void MoveAwayFromGroundEffect(::Player* player, ::DynamicObject* obj) const;

    /**
     * Calculate add priority score
     */
    uint32 CalculateAddPriority(::Creature* add) const;

    /**
     * Get tank position for boss
     */
    Position CalculateTankPosition(::Player* player, ::Creature* boss) const;

    /**
     * Get melee DPS position for boss
     */
    Position CalculateMeleePosition(::Player* player, ::Creature* boss) const;

    /**
     * Get ranged position for boss
     */
    Position CalculateRangedPosition(::Player* player, ::Creature* boss) const;

    /**
     * Move player to position
     */
    void MoveTo(::Player* player, Position const& position) const;

    // ============================================================================
    // ACCESSORS
    // ============================================================================

    char const* GetName() const { return _scriptName; }
    uint32 GetMapId() const { return _mapId; }

protected:
    char const* _scriptName;
    uint32 _mapId;
};

} // namespace Playerbot
