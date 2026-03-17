/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * BLACKFATHOM DEEPS DUNGEON SCRIPT
 *
 * Map ID: 48
 * Level Range: 18-24
 * Location: Ashenvale (underwater cave system)
 *
 * BOSS ENCOUNTERS:
 * 1. Ghamoo-ra (4887) - Giant turtle, water spout knockback
 * 2. Lady Sarevess (4831) - Naga caster, frost damage and slow
 * 3. Gelihast (6243) - Murloc boss, net and adds
 * 4. Lorgus Jett (12902) - Quest boss, lightning shield
 * 5. Baron Aquanis (12876) - Water elemental, frost damage
 * 6. Twilight Lord Kelris (4832) - Final boss, sleep and mind blast
 * 7. Aku'mai (4829) - Old God servant, poison cloud
 *
 * DUNGEON CHARACTERISTICS:
 * - Underwater cave environment
 * - Many water-based enemies
 * - Frost and nature damage prevalent
 * - Multiple quest objectives
 * - Winding, maze-like layout
 *
 * SPECIAL MECHANICS:
 * - Ghamoo-ra's water spout knockback
 * - Sarevess's frost slow effects
 * - Gelihast's net immobilize and murloc adds
 * - Kelris's mind control and sleep
 * - Aku'mai's poison cloud ground effect
 * - Underwater breathing considerations
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

class BlackfathomDeepsScript : public DungeonScript
{
public:
    BlackfathomDeepsScript() : DungeonScript("blackfathom_deeps", 48) {}

    // ============================================================================
    // LIFECYCLE HOOKS
    // ============================================================================

    void OnDungeonEnter(::Player* player, ::InstanceScript* instance) override
    {
        TC_LOG_DEBUG("module.playerbot", "BlackfathomDeepsScript: Player {} entered Blackfathom Deeps",
            player->GetGUID().GetCounter());

        // Note: Could check for water breathing buffs
        // Underwater sections require water breathing
    }

    // ============================================================================
    // BOSS HOOKS
    // ============================================================================

    void OnBossEngage(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 4887: // Ghamoo-ra
                TC_LOG_INFO("module.playerbot", "BlackfathomDeepsScript: Engaging Ghamoo-ra");
                // Knockback mechanic
                break;

            case 4831: // Lady Sarevess
                TC_LOG_INFO("module.playerbot", "BlackfathomDeepsScript: Engaging Lady Sarevess");
                // Frost damage and slow
                break;

            case 6243: // Gelihast
                TC_LOG_INFO("module.playerbot", "BlackfathomDeepsScript: Engaging Gelihast");
                // Net and murloc adds
                break;

            case 12902: // Lorgus Jett
                TC_LOG_INFO("module.playerbot", "BlackfathomDeepsScript: Engaging Lorgus Jett");
                // Lightning shield
                break;

            case 12876: // Baron Aquanis
                TC_LOG_INFO("module.playerbot", "BlackfathomDeepsScript: Engaging Baron Aquanis");
                // Frost damage
                break;

            case 4832: // Twilight Lord Kelris
                TC_LOG_INFO("module.playerbot", "BlackfathomDeepsScript: Engaging Twilight Lord Kelris (Final Boss)");
                // Sleep and mind blast
                HandleSpreadMechanic(player, boss);
                break;

            case 4829: // Aku'mai
                TC_LOG_INFO("module.playerbot", "BlackfathomDeepsScript: Engaging Aku'mai");
                // Poison cloud
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
            case 4831: // Lady Sarevess
            {
                // Sarevess casts Forked Lightning (chain lightning)
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Forked Lightning (8147) - high damage, jumps between targets
    if (spellId == 8147 || spellId == 8285)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "BlackfathomDeepsScript: Interrupting Sarevess's Forked Lightning");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }

                        // Frost Nova also interruptible
    if (spellId == 865 || spellId == 6131)
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

            case 4832: // Twilight Lord Kelris
            {
                // Kelris casts Mind Blast (high shadow damage) and Sleep
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Mind Blast (15587) - priority interrupt
    if (spellId == 15587 || spellId == 8105)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "BlackfathomDeepsScript: Interrupting Kelris's Mind Blast");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }

                        // Sleep (8399) - also high priority
    if (spellId == 8399 || spellId == 8040)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "BlackfathomDeepsScript: Interrupting Kelris's Sleep");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }
                    }
                }
                break;
            }

            case 12902: // Lorgus Jett
            {
                // Lorgus casts Lightning Bolt frequently
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Lightning Bolt (9532)
    if (spellId == 9532 || spellId == 915)
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
            case 4829: // Aku'mai
            {
                // Aku'mai spawns poison clouds on ground
                // Must move out immediately

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

                    // Check for poison cloud effects using spell info
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(snapshot.spellId, DIFFICULTY_NONE);
                    if (!spellInfo || !spellInfo->HasAura(SPELL_AURA_PERIODIC_DAMAGE))
                        continue;

                    // Calculate distance using snapshot position
                    float distance = player->GetExactDist(snapshot.position);
                    if (distance < 8.0f)
                    {
                        TC_LOG_DEBUG("module.playerbot", "BlackfathomDeepsScript: Avoiding Aku'mai's poison cloud at distance {:.1f}", distance);

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
            case 6243: // Gelihast
            {
                // Gelihast summons murloc adds
                // Adds should be killed quickly before they overwhelm group

                ::std::vector<::Creature*> adds = GetAddsInCombat(player, boss);

                if (!adds.empty())
                {
                    // Kill lowest health add first for quick elimination
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
                        TC_LOG_DEBUG("module.playerbot", "BlackfathomDeepsScript: Targeting Gelihast's murloc add");
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
            case 4887: // Ghamoo-ra
            {
                // Ghamoo-ra uses Booming Voice (knockback)
                // Tank should position boss against wall to minimize knockback distance

                DungeonRole role = GetPlayerRole(player);

                if (role == DungeonRole::TANK)
                {
                    // Tank should try to keep boss near wall
                    // This is difficult to implement without room geometry
                    // For now, use standard tank positioning
                }
                else if (role == DungeonRole::RANGED_DPS || role == DungeonRole::HEALER)
                {
                    // Ranged spread to minimize knockback impact
                    HandleSpreadMechanic(player, boss);
                    return;
                }
                break;
            }

            case 4831: // Lady Sarevess
            {
                // Sarevess slows with frost - maintain optimal range

                DungeonRole role = GetPlayerRole(player);
                float distance = player->GetExactDist(boss);

                if (role == DungeonRole::RANGED_DPS || role == DungeonRole::HEALER)
                {
                    // Stay at max range to reduce frost nova impact
    if (distance < 25.0f)
                    {
                        Position rangedPos = CalculateRangedPosition(player, boss);
                        MoveTo(player, rangedPos);
                        return;
                    }
                }
                break;
            }

            case 4832: // Twilight Lord Kelris
            {
                // Kelris uses Mind Blast and Sleep - spread to avoid chain CC
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

    void HandleDispelMechanic(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 4831: // Lady Sarevess
            {
                // Sarevess applies slow debuffs - dispel curse/magic
                Group* group = player->GetGroup();
                if (!group)
                    break;

                for (auto const& member : group->GetMemberSlots())
                {
                    Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
                    if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
                        continue;

                    // Check for slow debuffs
    if (groupMember->HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED))
                    {
                        TC_LOG_DEBUG("module.playerbot", "BlackfathomDeepsScript: Dispelling frost slow from Sarevess");
                        // Dispel magic/curse
                        return;
                    }
                }
                break;
            }

            case 4832: // Twilight Lord Kelris
            {
                // Kelris puts players to sleep - need wakeup/dispel
                Group* group = player->GetGroup();
                if (!group)
                    break;

                for (auto const& member : group->GetMemberSlots())
                {
                    Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
                    if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
                        continue;

                    // Check for sleep
    if (groupMember->HasAura(8399) || groupMember->HasAura(8040))
                    {
                        TC_LOG_DEBUG("module.playerbot", "BlackfathomDeepsScript: Player sleeping from Kelris");
                        // Dispel or damage to wake up
                        return;
                    }
                }
                break;
            }

            case 6243: // Gelihast
            {
                // Gelihast uses Net (immobilize) - dispel movement impairment
                Group* group = player->GetGroup();
                if (!group)
                    break;

                for (auto const& member : group->GetMemberSlots())
                {
                    Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
                    if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
                        continue;

                    // Check for net (root)
    if (groupMember->HasAuraType(SPELL_AURA_MOD_ROOT))
                    {
                        TC_LOG_DEBUG("module.playerbot", "BlackfathomDeepsScript: Player netted by Gelihast");
                        // Can't dispel net, must wait or break with damage
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
            case 4887: // Ghamoo-ra
            {
                // Knockback affects entire group - spread to minimize impact
                EncounterStrategy::HandleGenericSpread(player, boss, 8.0f);
                return;
            }

            case 4832: // Twilight Lord Kelris
            {
                // Sleep and Mind Blast - spread to avoid chain CC
                EncounterStrategy::HandleGenericSpread(player, boss, 12.0f);
                return;
            }

            case 4829: // Aku'mai
            {
                // Poison clouds - spread to give room for movement
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
            case 4829: // Aku'mai
            {
                // Constantly spawns poison clouds - be ready to move frequently
                // Check for nearby poison clouds and maintain safe distance

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
                auto dynamicObjectSnapshots = spatialGrid->QueryNearbyDynamicObjects(player->GetPosition(), 20.0f);

                bool nearPoison = false;
                for (auto const& snapshot : dynamicObjectSnapshots)
                {
                    // Filter using snapshot data - all filtering uses snapshot, no ObjectAccessor needed!
                    if (!snapshot.IsActive())
                        continue;

                    // Check if casted by the boss
                    if (snapshot.casterGuid != boss->GetGUID())
                        continue;

                    // Calculate distance using snapshot position
                    float distance = player->GetExactDist(snapshot.position);
                    if (distance < 10.0f)
                    {
                        nearPoison = true;
                        break;
                    }
                }

                if (nearPoison)
                {
                    // Move to clearer area
                    HandleGroundAvoidance(player, boss);
                    return;
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

void AddSC_blackfathom_deeps_playerbot()
{
    // Register script
    DungeonScriptMgr::instance()->RegisterScript(new BlackfathomDeepsScript());

    // Register bosses
    DungeonScriptMgr::instance()->RegisterBossScript(4887, DungeonScriptMgr::instance()->GetScriptForMap(48));  // Ghamoo-ra
    DungeonScriptMgr::instance()->RegisterBossScript(4831, DungeonScriptMgr::instance()->GetScriptForMap(48));  // Lady Sarevess
    DungeonScriptMgr::instance()->RegisterBossScript(6243, DungeonScriptMgr::instance()->GetScriptForMap(48));  // Gelihast
    DungeonScriptMgr::instance()->RegisterBossScript(12902, DungeonScriptMgr::instance()->GetScriptForMap(48)); // Lorgus Jett
    DungeonScriptMgr::instance()->RegisterBossScript(12876, DungeonScriptMgr::instance()->GetScriptForMap(48)); // Baron Aquanis
    DungeonScriptMgr::instance()->RegisterBossScript(4832, DungeonScriptMgr::instance()->GetScriptForMap(48));  // Twilight Lord Kelris
    DungeonScriptMgr::instance()->RegisterBossScript(4829, DungeonScriptMgr::instance()->GetScriptForMap(48));  // Aku'mai

    TC_LOG_INFO("server.loading", ">> Registered Blackfathom Deeps playerbot script with 7 boss mappings");
}

} // namespace Playerbot

/**
 * USAGE NOTES FOR BLACKFATHOM DEEPS:
 *
 * WHAT THIS SCRIPT HANDLES:
 * - Sarevess's Forked Lightning interrupt
 * - Kelris's Mind Blast and Sleep interrupts
 * - Aku'mai's poison cloud ground avoidance
 * - Gelihast's murloc add priority
 * - Ghamoo-ra knockback spread mechanics
 * - Sarevess frost slow dispel
 * - Kelris sleep dispel/wakeup
 * - Spread mechanics for multiple bosses
 * - Constant movement for Aku'mai poison clouds
 *
 * WHAT FALLS BACK TO GENERIC:
 * - Basic tank positioning
 * - Standard melee DPS positioning
 * - Ranged DPS optimal range for most bosses
 * - Basic add priority when no special logic needed
 *
 * DUNGEON-SPECIFIC TIPS:
 * - Bring water breathing potions/abilities
 * - Interrupt Sarevess's Forked Lightning to reduce chain damage
 * - Interrupt Kelris's Mind Blast and Sleep
 * - Kill Gelihast's murloc adds quickly
 * - Stay at max range against Sarevess to avoid Frost Nova
 * - Constantly move to avoid Aku'mai's poison clouds
 * - Spread out for Ghamoo-ra to minimize knockback impact
 * - Dispel frost slows and sleep effects
 * - Tank Ghamoo-ra near walls to reduce knockback distance
 *
 * DIFFICULTY RATING: 4/10 (Easy-Moderate)
 * - Underwater environment adds complexity
 * - Multiple interrupt requirements
 * - Poison cloud movement can be chaotic
 * - Good practice for dispel mechanics
 * - Maze-like layout can confuse new players
 */
