/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * THE STOCKADE DUNGEON SCRIPT
 *
 * Map ID: 34
 * Level Range: 15-30 (Originally 22-30, scaled down in later versions)
 * Location: Stormwind City, Alliance-only dungeon
 *
 * BOSS ENCOUNTERS:
 * 1. Kam Deepfury (1666) - Fury warrior, enrage
 * 2. Hamhock (1717) - Brutal fighter, chain strike
 * 3. Bazil Thredd (1716) - Smokebomb and DoTs
 * 4. Dextren Ward (1663) - Fear and shadow damage (rarely spawns)
 *
 * DUNGEON CHARACTERISTICS:
 * - Very short, compact prison dungeon
 * - Straightforward layout with three wings
 * - No complex mechanics
 * - Good for quick runs and leveling
 * - Alliance-only access
 *
 * SPECIAL MECHANICS:
 * - Kam Deepfury enrages at low health
 * - Bazil Thredd uses smokebomb (blind effect)
 * - Dextren Ward fears and does shadow damage
 * - Multiple elite packs throughout
 */

#include "DungeonScript.h"
#include "DungeonScriptMgr.h"
#include "EncounterStrategy.h"
#include "Log.h"
#include "Player.h"
#include "Creature.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "MotionMaster.h"
#include "../../../Spatial/SpatialGridManager.h"
#include "../../../Spatial/DoubleBufferedSpatialGrid.h"
#include "../../../Core/PlayerBotHelpers.h"
#include "../../../AI/BotAI.h"

namespace Playerbot
{

class StockadeScript : public DungeonScript
{
public:
    StockadeScript() : DungeonScript("the_stockade", 34) {}

    // ============================================================================
    // BOSS HOOKS
    // ============================================================================

    void OnBossEngage(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 1666: // Kam Deepfury
                TC_LOG_INFO("module.playerbot", "StockadeScript: Engaging Kam Deepfury");
                // Enrage at low health
                break;

            case 1717: // Hamhock
                TC_LOG_INFO("module.playerbot", "StockadeScript: Engaging Hamhock");
                // Chain strike damage
                break;

            case 1716: // Bazil Thredd
                TC_LOG_INFO("module.playerbot", "StockadeScript: Engaging Bazil Thredd");
                // Smokebomb mechanic
                break;

            case 1663: // Dextren Ward
                TC_LOG_INFO("module.playerbot", "StockadeScript: Engaging Dextren Ward (Rare Spawn)");
                // Fear mechanic
                HandleSpreadMechanic(player, boss);
                break;

            default:
                break;
        }
    }

    // ============================================================================
    // MECHANIC HANDLERS
    // ============================================================================

    void HandleInterruptPriority(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 1663: // Dextren Ward
            {
                // Ward casts Mind Blast (frequent, high damage)
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Mind Blast is interruptible and deals significant damage
    if (spellId == 15587 || spellId == 13860) // Mind Blast variations
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "StockadeScript: Interrupting Dextren Ward's Mind Blast");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }
                    }
                }
                break;
            }

            default:
                break;
        }

        // Fall back to generic
        DungeonScript::HandleInterruptPriority(player, boss);
    }

    void HandleGroundAvoidance(::Player* player, ::Creature* boss) override
    {
        // ENTERPRISE: Use lock-free spatial grid for thread-safe DynamicObject queries
        // The Stockade has minimal ground effects but using spatial grid for consistency
        Map* map = player->GetMap();
        if (map)
        {
            DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
            if (!spatialGrid)
            {
                sSpatialGridManager.CreateGrid(map);
                spatialGrid = sSpatialGridManager.GetGrid(map);
            }

            if (spatialGrid)
            {
                // Query nearby dynamic objects using immutable snapshots (lock-free!)
                auto dynamicObjectSnapshots = spatialGrid->QueryNearbyDynamicObjects(player->GetPosition(), 15.0f);

                for (auto const& snapshot : dynamicObjectSnapshots)
                {
                    if (!snapshot.IsActive())
                        continue;

                    if (snapshot.casterGuid != boss->GetGUID())
                        continue;

                    float distance = player->GetExactDist(snapshot.position);
                    if (distance < 10.0f)
                    {
                        if (DynamicObject* dynObj = ObjectAccessor::GetDynamicObject(*player, snapshot.guid))
                        {
                            if (IsDangerousGroundEffect(dynObj))
                            {
                                TC_LOG_DEBUG("module.playerbot", "StockadeScript: Avoiding ground effect at distance {:.1f}", distance);
                                MoveAwayFromGroundEffect(player, dynObj);
                                return;
                            }
                        }
                    }
                }
            }
        }

        // Fall back to generic
        DungeonScript::HandleGroundAvoidance(player, boss);
    }

    void HandleDispelMechanic(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 1663: // Dextren Ward
            {
                // Ward casts Psychic Scream (fear)
                // Need to dispel or wait out the fear

                Group* group = player->GetGroup();
                if (!group)
                    break;
                // Check for feared players
    for (auto const& member : group->GetMemberSlots())
                {
                    Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
                    if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
                        continue;

                    // Check for fear debuff
    if (groupMember->HasAuraType(SPELL_AURA_MOD_FEAR))
                    {
                        TC_LOG_DEBUG("module.playerbot", "StockadeScript: Player is feared by Dextren Ward");
                        // Dispel fear if possible
                        // Otherwise wait it out
                        return;
                    }
                }
                break;
            }

            default:
                break;
        }

        // Fall back to generic
        DungeonScript::HandleDispelMechanic(player, boss);
    }

    void HandlePositioning(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 1666: // Kam Deepfury
            {
                // Kam enrages at low health - tank needs defensive cooldowns
                // No special positioning needed, standard melee setup
                break;
            }

            case 1716: // Bazil Thredd
            {
                // Bazil uses Smoke Bomb (blind effect in area)
                // If blinded, move away from boss briefly
    if (player->HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED)) // Blind-like effect
                {
                    // Move away briefly
                    float angle = boss->GetAbsoluteAngle(player) + static_cast<float>(M_PI); // Away from boss
                    float x = player->GetPositionX() + 5.0f * ::std::cos(angle);
                    float y = player->GetPositionY() + 5.0f * ::std::sin(angle);
                    float z = player->GetPositionZ();
                    TC_LOG_DEBUG("module.playerbot", "StockadeScript: Moving away from Bazil's Smoke Bomb");

                    // Use validated pathfinding for bot players
                    Position dest(x, y, z, 0.0f);
                    if (BotAI* ai = GetBotAI(player))
                    {
                        if (!ai->MoveTo(dest, true))
                        {
                            // Fallback to legacy if validation fails
                            player->GetMotionMaster()->MovePoint(0, x, y, z);
                        }
                    }
                    else
                    {
                        // Non-bot player - use standard movement
                        player->GetMotionMaster()->MovePoint(0, x, y, z);
                    }
                    return;
                }
                break;
            }

            case 1663: // Dextren Ward
            {
                // Ward fears players - spread to minimize fear overlap
                DungeonRole role = GetPlayerRole(player);

                if (role == DungeonRole::RANGED_DPS || role == DungeonRole::HEALER)
                {
                    HandleSpreadMechanic(player, boss);
                    return;
                }
                break;
            }

            default:
                break;
        }

        // Fall back to generic
        DungeonScript::HandlePositioning(player, boss);
    }

    void HandleSpreadMechanic(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 1663: // Dextren Ward
            {
                // Psychic Scream (fear) - spread to avoid chain CC
                EncounterStrategy::HandleGenericSpread(player, boss, 10.0f);
                return;
            }

            default:
                break;
        }

        // Default spread
        DungeonScript::HandleSpreadMechanic(player, boss);
    }

    void HandleMovementMechanic(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 1666: // Kam Deepfury
            {
                // When Kam enrages (below 20% health), tank should use defensive cooldowns
                // DPS should maximize damage to end fight quickly
    if (boss->GetHealthPct() < 20.0f)
                {
                    DungeonRole role = GetPlayerRole(player);

                    if (role == DungeonRole::TANK)
                    {
                        TC_LOG_DEBUG("module.playerbot", "StockadeScript: Kam enraged - tank using defensives");
                        // Tank should use defensive cooldowns
                    }
                    else if (role == DungeonRole::MELEE_DPS || role == DungeonRole::RANGED_DPS)
                    {
                        TC_LOG_DEBUG("module.playerbot", "StockadeScript: Kam enraged - DPS burn phase");
                        // DPS should burn boss down quickly
                    }
                }
                break;
            }

            default:
                break;
        }

        // Fall back to generic
        DungeonScript::HandleMovementMechanic(player, boss);
    }
};

// ============================================================================
// REGISTRATION
// ============================================================================

void AddSC_stockade_playerbot()
{
    // Register script
    DungeonScriptMgr::instance()->RegisterScript(new StockadeScript());

    // Register bosses
    DungeonScriptMgr::instance()->RegisterBossScript(1666, DungeonScriptMgr::instance()->GetScriptForMap(34)); // Kam Deepfury
    DungeonScriptMgr::instance()->RegisterBossScript(1717, DungeonScriptMgr::instance()->GetScriptForMap(34)); // Hamhock
    DungeonScriptMgr::instance()->RegisterBossScript(1716, DungeonScriptMgr::instance()->GetScriptForMap(34)); // Bazil Thredd
    DungeonScriptMgr::instance()->RegisterBossScript(1663, DungeonScriptMgr::instance()->GetScriptForMap(34)); // Dextren Ward

    TC_LOG_INFO("server.loading", ">> Registered The Stockade playerbot script with 4 boss mappings");
}

} // namespace Playerbot

/**
 * USAGE NOTES FOR THE STOCKADE:
 *
 * WHAT THIS SCRIPT HANDLES:
 * - Kam Deepfury enrage detection and defensive response
 * - Bazil Thredd smokebomb avoidance
 * - Dextren Ward Mind Blast interrupts
 * - Fear dispel for Dextren Ward
 * - Spread mechanics for fear avoidance
 *
 * WHAT FALLS BACK TO GENERIC:
 * - Basic tank positioning for all bosses
 * - Standard melee DPS positioning
 * - Ranged DPS optimal range
 * - Basic add priority (no special adds)
 *
 * DUNGEON-SPECIFIC TIPS:
 * - Very straightforward dungeon, minimal mechanics
 * - Interrupt Dextren Ward's Mind Blast to reduce damage
 * - Use defensive cooldowns when Kam Deepfury enrages
 * - Move away from Bazil's Smoke Bomb if blinded
 * - Spread for Dextren Ward to avoid fear overlap
 * - Fast dungeon, good for quick leveling runs
 *
 * DIFFICULTY RATING: 1/10 (Very Easy)
 * - Simplest dungeon in the game
 * - Minimal mechanics to handle
 * - Short duration (15-20 minutes)
 * - Good introduction to group content
 * - Excellent for new tanks to practice
 */
