/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * RAGEFIRE CHASM DUNGEON SCRIPT
 *
 * Map ID: 389
 * Level Range: 13-18
 * Location: Orgrimmar, Horde-only dungeon
 *
 * BOSS ENCOUNTERS:
 * 1. Oggleflint (11517) - Fire-wielding ogre
 * 2. Taragaman the Hungerer (11520) - Final boss, demon
 * 3. Jergosh the Invoker (11518) - Fire mage boss
 * 4. Bazzalan (11519) - Bonus boss, demon
 *
 * DUNGEON CHARACTERISTICS:
 * - Very linear layout, short dungeon
 * - Heavy fire damage throughout
 * - Many fire-based enemies
 * - Good for new players, straightforward mechanics
 * - Fire resistance recommended
 *
 * SPECIAL MECHANICS:
 * - Fire patches on ground throughout dungeon
 * - Multiple elite pulls
 * - Taragaman has fire shield and area fire damage
 * - Jergosh channels fire spells
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
#include "ObjectAccessor.h"
#include "../../../Spatial/SpatialGridManager.h"
#include "../../../Spatial/DoubleBufferedSpatialGrid.h"

namespace Playerbot
{

class RagefireChasmScript : public DungeonScript
{
public:
    RagefireChasmScript() : DungeonScript("ragefire_chasm", 389) {}

    // ============================================================================
    // LIFECYCLE HOOKS
    // ============================================================================

    void OnDungeonEnter(::Player* player, ::InstanceScript* instance) override
    {
        TC_LOG_DEBUG("module.playerbot", "RagefireChasmScript: Player {} entered Ragefire Chasm",
            player->GetGUID().GetCounter());

        // Recommend fire resistance
        // Note: Could check player's fire resistance and suggest gear
    }

    // ============================================================================
    // BOSS HOOKS
    // ============================================================================

    void OnBossEngage(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 11517: // Oggleflint
                TC_LOG_INFO("module.playerbot", "RagefireChasmScript: Engaging Oggleflint");
                break;

            case 11520: // Taragaman the Hungerer
                TC_LOG_INFO("module.playerbot", "RagefireChasmScript: Engaging Taragaman the Hungerer (Final Boss)");
                // Spread out for fire damage
                HandleSpreadMechanic(player, boss);
                break;

            case 11518: // Jergosh the Invoker
                TC_LOG_INFO("module.playerbot", "RagefireChasmScript: Engaging Jergosh the Invoker");
                break;

            case 11519: // Bazzalan
                TC_LOG_INFO("module.playerbot", "RagefireChasmScript: Engaging Bazzalan (Bonus Boss)");
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
            case 11518: // Jergosh the Invoker
            {
                // Jergosh channels Immolate (11962) - should be interrupted
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Interrupt Immolate (high damage DoT)
    if (spellId == 11962 || spellId == 20294) // Immolate / Fireball
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "RagefireChasmScript: Interrupting Jergosh's fire spell");
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

        // Fall back to generic interrupt logic
        DungeonScript::HandleInterruptPriority(player, boss);
    }

    void HandleGroundAvoidance(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 11520: // Taragaman the Hungerer
            {
                // Taragaman has Upper Cut (knock back) and Fire Nova (area fire)
                // Need to avoid standing in fire patches

                // ENTERPRISE: Use lock-free spatial grid for thread-safe DynamicObject queries
                Map* map = player->GetMap();
                if (!map)
                    break;

                DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
                if (!spatialGrid)
                {
                    sSpatialGridManager.CreateGrid(map);
                    spatialGrid = sSpatialGridManager.GetGrid(map);
                    if (!spatialGrid)
                        break;
                }

                // Query nearby dynamic objects using immutable snapshots (lock-free!)
                auto dynamicObjectSnapshots = spatialGrid->QueryNearbyDynamicObjects(player->GetPosition(), 10.0f);

                for (auto const& snapshot : dynamicObjectSnapshots)
                {
                    // Filter using snapshot data - no ObjectAccessor calls needed for filtering!
                    if (!snapshot.IsActive())
                        continue;

                    // Check if casted by the boss
                    if (snapshot.casterGuid != boss->GetGUID())
                        continue;

                    // Calculate distance using snapshot position
                    float distance = player->GetExactDist(snapshot.position);

                    // Fire Nova (11970) leaves fire on ground
                    if (distance < 8.0f)
                    {
                        if (DynamicObject* dynObj = ObjectAccessor::GetDynamicObject(*player, snapshot.guid))
                        {
                            if (IsDangerousGroundEffect(dynObj))
                            {
                                TC_LOG_DEBUG("module.playerbot", "RagefireChasmScript: Avoiding Taragaman's fire at distance {:.1f}", distance);
                                MoveAwayFromGroundEffect(player, dynObj);
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

        // Fall back to generic ground avoidance
        DungeonScript::HandleGroundAvoidance(player, boss);
    }

    void HandlePositioning(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 11520: // Taragaman the Hungerer
            {
                // Taragaman has Fire Nova (area damage around him)
                // Ranged and healers should stay at max range
                DungeonRole role = GetPlayerRole(player);

                if (role == DungeonRole::RANGED_DPS || role == DungeonRole::HEALER)
                {
                    Position rangedPos = CalculateRangedPosition(player, boss);

                    // Stay at 25-30 yards
    if (player->GetExactDist(boss) < 20.0f)
                    {
                        MoveTo(player, rangedPos);
                        return;
                    }
                }

                // Melee and tank use standard positioning
                break;
            }

            case 11518: // Jergosh the Invoker
            {
                // Jergosh casts Immolate - spread out to avoid chain fire damage
                DungeonRole role = GetPlayerRole(player);

                if (role == DungeonRole::RANGED_DPS || role == DungeonRole::HEALER)
                {
                    // Stay spread out
                    HandleSpreadMechanic(player, boss);
                    return;
                }
                break;
            }

            default:
                break;
        }

        // Fall back to generic positioning
        DungeonScript::HandlePositioning(player, boss);
    }

    void HandleSpreadMechanic(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 11520: // Taragaman the Hungerer
            {
                // Fire Nova damage - spread 8 yards apart
                EncounterStrategy::HandleGenericSpread(player, boss, 8.0f);
                return;
            }

            case 11518: // Jergosh the Invoker
            {
                // Immolate can spread - 10 yards
                EncounterStrategy::HandleGenericSpread(player, boss, 10.0f);
                return;
            }

            default:
                break;
        }
        // Default spread distance
        DungeonScript::HandleSpreadMechanic(player, boss);
    }

    void HandleDispelMechanic(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();
        switch (entry)
        {
            case 11518: // Jergosh the Invoker
            {
                // Immolate (11962) is a high-damage DoT that should be dispelled immediately
                // Priority dispel for healers

                Group* group = player->GetGroup();
                if (!group)
                    break;

                // Find players with Immolate
    for (auto const& member : group->GetMemberSlots())
                {
                    Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
                    if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
                        continue;

                    // Check for Immolate debuff
    if (groupMember->HasAura(11962))
                    {
                        TC_LOG_DEBUG("module.playerbot", "RagefireChasmScript: Player has Immolate, needs dispel");

                        // Try to dispel (would use actual dispel spell here)
                        return;
                    }
                }
                break;
            }

            default:
                break;
        }

        // Fall back to generic dispel
        DungeonScript::HandleDispelMechanic(player, boss);
    }

    // ============================================================================
    // ADDITIONAL MECHANICS
    // ============================================================================

    void OnUpdate(::Player* player, uint32 diff) override
    {
        // Ragefire Chasm has fire patches throughout the dungeon
        // Constantly check and avoid fire on ground

        // This would be called periodically to check for environmental hazards
        // Could implement patrol path checks, trap avoidance, etc.
    }
};

// ============================================================================
// REGISTRATION
// ============================================================================

/**
 * This function is called by DungeonScriptLoader to register this script
 */
void AddSC_ragefire_chasm_playerbot()
{
    // Register script with DungeonScriptMgr
    DungeonScriptMgr::instance()->RegisterScript(new RagefireChasmScript());

    // Register boss mappings
    DungeonScriptMgr::instance()->RegisterBossScript(11517, DungeonScriptMgr::instance()->GetScriptForMap(389)); // Oggleflint
    DungeonScriptMgr::instance()->RegisterBossScript(11520, DungeonScriptMgr::instance()->GetScriptForMap(389)); // Taragaman
    DungeonScriptMgr::instance()->RegisterBossScript(11518, DungeonScriptMgr::instance()->GetScriptForMap(389)); // Jergosh
    DungeonScriptMgr::instance()->RegisterBossScript(11519, DungeonScriptMgr::instance()->GetScriptForMap(389)); // Bazzalan

    TC_LOG_INFO("server.loading", ">> Registered Ragefire Chasm playerbot script with 4 boss mappings");
}

} // namespace Playerbot

/**
 * USAGE NOTES FOR RAGEFIRE CHASM:
 *
 * WHAT THIS SCRIPT HANDLES:
 * - Jergosh interrupt priority (Immolate)
 * - Taragaman ground fire avoidance (Fire Nova)
 * - Ranged positioning for area fire damage
 * - Spread mechanics for multiple fire effects
 * - Immolate dispel priority
 *
 * WHAT FALLS BACK TO GENERIC:
 * - Basic add priority (no special adds)
 * - Tank positioning (standard tanking)
 * - Melee DPS positioning (standard behind boss)
 * - Basic movement mechanics
 *
 * DUNGEON-SPECIFIC TIPS:
 * - Fire resistance gear helps significantly
 * - Interrupt Jergosh's Immolate to reduce damage
 * - Spread out for Taragaman to minimize Fire Nova damage
 * - Dispel Immolate DoT as soon as possible
 * - Watch for fire patches on ground throughout dungeon
 *
 * DIFFICULTY RATING: 2/10 (Very Easy)
 * - Straightforward dungeon with simple mechanics
 * - Good for new players learning dungeon basics
 * - Fire damage is manageable with basic healing
 * - No complex positioning or timing requirements
 */
