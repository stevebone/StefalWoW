/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * GNOMEREGAN DUNGEON SCRIPT
 *
 * Map ID: 90
 * Level Range: 24-34
 * Location: Dun Morogh (entrance near Ironforge)
 *
 * BOSS ENCOUNTERS:
 * 1. Grubbis (7361) - Radioactive slime boss, radiation damage
 * 2. Viscous Fallout (7079) - Ooze boss, poison and slowing
 * 3. Electrocutioner 6000 (6235) - Robot boss, chain lightning and static
 * 4. Crowd Pummeler 9-60 (6229) - Giant robot, knockback and crowd control
 * 5. Mekgineer Thermaplugg (7800) - Final boss, bomb adds and radiation
 *
 * DUNGEON CHARACTERISTICS:
 * - Complex multi-level layout with elevators and platforms
 * - Heavy radiation damage throughout
 * - Many mechanical enemies (robots)
 * - Troggs and leper gnomes
 * - Environmental hazards (bombs, radiation zones)
 *
 * SPECIAL MECHANICS:
 * - Radiation damage from environment and bosses
 * - Grubbis's radiation cloud
 * - Electrocutioner's chain lightning (must spread)
 * - Crowd Pummeler's knockback and fear
 * - Thermaplugg's bomb adds (walking bombs must be killed quickly)
 * - Static shock mechanics requiring positioning
 * - Slowing ooze effects
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
#include "../../../Spatial/SpatialGridManager.h"
#include "../../../Spatial/DoubleBufferedSpatialGrid.h"

namespace Playerbot
{

class GnomereganScript : public DungeonScript
{
public:
    GnomereganScript() : DungeonScript("gnomeregan", 90) {}

    // ============================================================================
    // LIFECYCLE HOOKS
    // ============================================================================

    void OnDungeonEnter(::Player* player, ::InstanceScript* instance) override
    {
        TC_LOG_DEBUG("module.playerbot", "GnomereganScript: Player {} entered Gnomeregan",
            player->GetGUID().GetCounter());

        // Note: Radiation damage is prevalent - healing will be constant
        // Nature resistance recommended
    }

    // ============================================================================
    // BOSS HOOKS
    // ============================================================================

    void OnBossEngage(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 7361: // Grubbis
                TC_LOG_INFO("module.playerbot", "GnomereganScript: Engaging Grubbis");
                // Radiation cloud - spread mechanic
                HandleSpreadMechanic(player, boss);
                break;

            case 7079: // Viscous Fallout
                TC_LOG_INFO("module.playerbot", "GnomereganScript: Engaging Viscous Fallout");
                // Ooze slowing effects
                break;

            case 6235: // Electrocutioner 6000
                TC_LOG_INFO("module.playerbot", "GnomereganScript: Engaging Electrocutioner 6000");
                // Chain lightning - must spread
                HandleSpreadMechanic(player, boss);
                break;

            case 6229: // Crowd Pummeler 9-60
                TC_LOG_INFO("module.playerbot", "GnomereganScript: Engaging Crowd Pummeler 9-60");
                // Knockback and fear mechanics
                break;

            case 7800: // Mekgineer Thermaplugg
                TC_LOG_INFO("module.playerbot", "GnomereganScript: Engaging Mekgineer Thermaplugg (Final Boss)");
                // Bomb adds - critical priority
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
            case 6235: // Electrocutioner 6000
            {
                // Electrocutioner casts Chain Bolt (11975) - high priority interrupt
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Chain Bolt (11975) - jumps to entire group
    if (spellId == 11975 || spellId == 12167)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "GnomereganScript: Interrupting Electrocutioner's Chain Bolt");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }

                        // Static (6535) - also worth interrupting
    if (spellId == 6535)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }
                    }
                }
                break;
            }

            case 7800: // Mekgineer Thermaplugg
            {
                // Thermaplugg doesn't have critical interrupts
                // His danger comes from bomb adds
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
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 7361: // Grubbis
            {
                // Grubbis spawns radiation clouds on ground
                // Must move out of radiation zones

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
                auto dynamicObjectSnapshots = spatialGrid->QueryNearbyDynamicObjects(player->GetPosition(), 15.0f);

                for (auto const& snapshot : dynamicObjectSnapshots)
                {
                    // Filter using snapshot data - no ObjectAccessor calls needed for filtering!
                    if (!snapshot.IsActive())
                        continue;

                    // Check if casted by the boss
                    if (snapshot.casterGuid != boss->GetGUID())
                        continue;

                    // Check for radiation cloud effects using spell info
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(snapshot.spellId, DIFFICULTY_NONE);
                    if (!spellInfo)
                        continue;

                    // Radiation (spell 6524 and similar)
                    if (!spellInfo->HasAura(SPELL_AURA_PERIODIC_DAMAGE) && !spellInfo->HasEffect(SPELL_EFFECT_PERSISTENT_AREA_AURA))
                        continue;

                    // Calculate distance using snapshot position
                    float distance = player->GetExactDist(snapshot.position);
                    if (distance < 8.0f)
                    {
                        TC_LOG_DEBUG("module.playerbot", "GnomereganScript: Avoiding Grubbis's radiation cloud at distance {:.1f}", distance);

                        // Only get actual DynamicObject* for movement calculation (needs object reference)
                        if (DynamicObject* dynObj = ObjectAccessor::GetDynamicObject(*player, snapshot.guid))
                        {
                            MoveAwayFromGroundEffect(player, dynObj);
                            return;
                        }
                    }
                }
                break;
            }

            case 7079: // Viscous Fallout
            {
                // Fallout leaves slowing ooze on ground

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
                auto dynamicObjectSnapshots = spatialGrid->QueryNearbyDynamicObjects(player->GetPosition(), 12.0f);

                for (auto const& snapshot : dynamicObjectSnapshots)
                {
                    // Filter using snapshot data
                    if (!snapshot.IsActive())
                        continue;

                    // Check if casted by the boss
                    if (snapshot.casterGuid != boss->GetGUID())
                        continue;

                    // Calculate distance using snapshot position
                    float distance = player->GetExactDist(snapshot.position);
                    if (distance < 6.0f)
                    {
                        TC_LOG_DEBUG("module.playerbot", "GnomereganScript: Avoiding Viscous Fallout's ooze at distance {:.1f}", distance);

                        // Only get actual DynamicObject* for movement calculation
                        if (DynamicObject* dynObj = ObjectAccessor::GetDynamicObject(*player, snapshot.guid))
                        {
                            MoveAwayFromGroundEffect(player, dynObj);
                            return;
                        }
                    }
                }
                break;
            }

            default:
                break;
        }

        // Fall back to generic
        DungeonScript::HandleGroundAvoidance(player, boss);
    }

    void HandleAddPriority(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 7800: // Mekgineer Thermaplugg
            {
                // Thermaplugg summons Bomb adds (Entry 7915)
                // These walking bombs MUST be killed immediately or they explode
                // CRITICAL: Top priority over boss

                ::std::vector<::Creature*> adds = GetAddsInCombat(player, boss);

                for (::Creature* add : adds)
                {
                    if (!add || !add->IsAlive())
                        continue;

                    // Bomb adds (entry 7915 - Walking Bomb)
    if (add->GetEntry() == 7915)
                    {
                        TC_LOG_DEBUG("module.playerbot", "GnomereganScript: PRIORITY - Targeting Thermaplugg's bomb add");
                        player->SetSelection(add->GetGUID());
                        return;
                    }
                }

                // If no bomb adds, focus boss
                break;
            }

            case 6229: // Crowd Pummeler 9-60
            {
                // Crowd Pummeler can summon alarm bots
                // Kill quickly to prevent reinforcements

                ::std::vector<::Creature*> adds = GetAddsInCombat(player, boss);

                if (!adds.empty())
                {
                    // Prioritize low health adds for quick elimination
                    ::Creature* lowestHealthAdd = nullptr;
                    float lowestPct = 100.0f;

                    for (::Creature* add : adds)
                    {
                        if (!add || !add->IsAlive())
                            continue;

                        float healthPct = add->GetHealthPct();
                        if (healthPct < lowestPct)
                        {
                            lowestPct = healthPct;
                            lowestHealthAdd = add;
                        }
                    }

                    if (lowestHealthAdd)
                    {
                        player->SetSelection(lowestHealthAdd->GetGUID());
                        return;
                    }
                }
                break;
            }

            default:
                break;
        }

        // Fall back to generic
        DungeonScript::HandleAddPriority(player, boss);
    }

    void HandlePositioning(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 6235: // Electrocutioner 6000
            {
                // Chain Bolt jumps between players - must spread
                // All players should maintain distance

                DungeonRole role = GetPlayerRole(player);

                if (role == DungeonRole::RANGED_DPS || role == DungeonRole::HEALER)
                {
                    // Ranged spread widely
                    HandleSpreadMechanic(player, boss);
                    return;
                }
                else if (role == DungeonRole::MELEE_DPS)
                {
                    // Melee also need some spread (8+ yards)
                    HandleSpreadMechanic(player, boss);
                    return;
                }
                break;
            }

            case 6229: // Crowd Pummeler 9-60
            {
                // Knockback and fear - tank should position against wall
                DungeonRole role = GetPlayerRole(player);

                if (role == DungeonRole::TANK)
                {
                    // Try to keep boss near wall to minimize knockback
                    // This is difficult without room geometry
                    // For now, standard tank positioning
                }
                else if (role == DungeonRole::RANGED_DPS || role == DungeonRole::HEALER)
                {
                    // Stay at range to avoid fear
                    Position rangedPos = CalculateRangedPosition(player, boss);
                    float distance = player->GetExactDist(boss);
                    if (distance < 20.0f)
                    {
                        MoveTo(player, rangedPos);
                        return;
                    }
                }
                break;
            }

            case 7800: // Mekgineer Thermaplugg
            {
                // Spread out for bomb explosions
                HandleSpreadMechanic(player, boss);
                return;
            }
            default:
                break;
        }

        // Fall back to generic
        DungeonScript::HandlePositioning(player, boss);
    }

    void HandleDispelMechanic(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 7079: // Viscous Fallout
            {
                // Fallout applies slowing disease debuffs
                Group* group = player->GetGroup();
                if (!group)
                    break;

                for (auto const& member : group->GetMemberSlots())
                {
                    Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
                    if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
                        continue;

                    // Check for slowing effects (disease type)
    if (groupMember->HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED))
                    {
                        TC_LOG_DEBUG("module.playerbot", "GnomereganScript: Dispelling slowing disease from Viscous Fallout");
                        // Dispel disease
                        return;
                    }
                }
                break;
            }

            case 6229: // Crowd Pummeler 9-60
            {
                // Fear effects need dispel or break
                Group* group = player->GetGroup();
                if (!group)
                    break;

                for (auto const& member : group->GetMemberSlots())
                {
                    Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
                    if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
                        continue;

                    // Check for fear
    if (groupMember->HasAuraType(SPELL_AURA_MOD_FEAR))
                    {
                        TC_LOG_DEBUG("module.playerbot", "GnomereganScript: Player feared by Crowd Pummeler");
                        // Break fear with damage or tremor totem
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

    void HandleSpreadMechanic(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 6235: // Electrocutioner 6000
            {
                // Chain Bolt requires 12+ yard spread to prevent jumps
                EncounterStrategy::HandleGenericSpread(player, boss, 12.0f);
                return;
            }

            case 7361: // Grubbis
            {
                // Radiation cloud - 8 yard spread
                EncounterStrategy::HandleGenericSpread(player, boss, 8.0f);
                return;
            }
            case 7800: // Mekgineer Thermaplugg
            {
                // Bomb explosions - 10 yard spread
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
            case 7361: // Grubbis
            {
                // Constantly spawns radiation - be ready to move
                HandleGroundAvoidance(player, boss);
                return;
            }

            case 7800: // Mekgineer Thermaplugg
            {
                // Walking bombs - must kite away from them while killing
                ::std::vector<::Creature*> adds = GetAddsInCombat(player, boss);

                for (::Creature* add : adds)
                {
                    if (!add || !add->IsAlive())
                        continue;

                    // Walking Bomb (7915)
    if (add->GetEntry() == 7915)
                    {
                        float distance = player->GetExactDist(add);

                        // If bomb is too close, move away while attacking
    if (distance < 5.0f)
                        {
                            TC_LOG_DEBUG("module.playerbot", "GnomereganScript: Kiting away from walking bomb");

                            // Calculate position away from bomb
                            Position bombPos = add->GetPosition();
                            Position safePos = player->GetPosition();

                            // Move directly away from bomb
                            float angle = bombPos.GetAbsoluteAngle(&safePos);
                            float newX = player->GetPositionX() + cos(angle) * 8.0f;
                            float newY = player->GetPositionY() + sin(angle) * 8.0f;
                            float newZ = player->GetPositionZ();
                            Position movePos(newX, newY, newZ);
                            MoveTo(player, movePos);
                            return;
                        }
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

void AddSC_gnomeregan_playerbot()
{
    // Register script
    DungeonScriptMgr::instance()->RegisterScript(new GnomereganScript());

    // Register bosses
    DungeonScriptMgr::instance()->RegisterBossScript(7361, DungeonScriptMgr::instance()->GetScriptForMap(90)); // Grubbis
    DungeonScriptMgr::instance()->RegisterBossScript(7079, DungeonScriptMgr::instance()->GetScriptForMap(90)); // Viscous Fallout
    DungeonScriptMgr::instance()->RegisterBossScript(6235, DungeonScriptMgr::instance()->GetScriptForMap(90)); // Electrocutioner 6000
    DungeonScriptMgr::instance()->RegisterBossScript(6229, DungeonScriptMgr::instance()->GetScriptForMap(90)); // Crowd Pummeler 9-60
    DungeonScriptMgr::instance()->RegisterBossScript(7800, DungeonScriptMgr::instance()->GetScriptForMap(90)); // Mekgineer Thermaplugg

    TC_LOG_INFO("server.loading", ">> Registered Gnomeregan playerbot script with 5 boss mappings");
}

} // namespace Playerbot

/**
 * USAGE NOTES FOR GNOMEREGAN:
 *
 * WHAT THIS SCRIPT HANDLES:
 * - Electrocutioner's Chain Bolt interrupt (critical)
 * - Grubbis's radiation cloud ground avoidance
 * - Viscous Fallout's ooze ground avoidance
 * - Thermaplugg's walking bomb add priority (CRITICAL)
 * - Electrocutioner spread mechanics (12+ yards)
 * - Grubbis radiation spread
 * - Thermaplugg bomb explosion spread
 * - Viscous Fallout disease dispel
 * - Crowd Pummeler fear management
 * - Walking bomb kiting mechanics
 * - Crowd Pummeler alarm bot priority
 *
 * WHAT FALLS BACK TO GENERIC:
 * - Basic tank positioning
 * - Standard melee DPS positioning
 * - Ranged DPS optimal range for most bosses
 * - Basic healing priority
 *
 * DUNGEON-SPECIFIC TIPS:
 * - Nature resistance gear helps with radiation
 * - ALWAYS interrupt Electrocutioner's Chain Bolt
 * - Spread 12+ yards apart for Electrocutioner fight
 * - IMMEDIATELY kill Thermaplugg's walking bombs (top priority)
 * - Kite away from walking bombs while DPS'ing them
 * - Move out of Grubbis's radiation clouds immediately
 * - Dispel disease debuffs from Viscous Fallout
 * - Fear Ward or tremor totem helpful for Crowd Pummeler
 * - Tank Crowd Pummeler near walls to minimize knockback
 * - Watch for environmental radiation damage
 * - Complex layout - use map awareness
 *
 * DIFFICULTY RATING: 6/10 (Moderate)
 * - Complex multi-level layout can confuse navigation
 * - Thermaplugg's bomb adds require fast reactions
 * - Electrocutioner requires precise spreading
 * - Environmental radiation adds constant pressure
 * - Multiple interrupt requirements
 * - Good practice for add priority and kiting
 * - Longer dungeon with multiple mini-bosses
 */
