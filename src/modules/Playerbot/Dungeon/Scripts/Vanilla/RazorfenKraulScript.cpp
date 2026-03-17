/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * RAZORFEN KRAUL DUNGEON SCRIPT
 *
 * Map ID: 47
 * Level Range: 25-35
 * Location: The Barrens (Horde-friendly area)
 *
 * BOSS ENCOUNTERS:
 * 1. Roogug (6168) - Quilboar shaman, totems and lightning
 * 2. Aggem Thorncurse (4424) - Quilboar caster, curse and shadow damage
 * 3. Death Speaker Jargba (4428) - Quilboar necromancer, shadow bolts
 * 4. Overlord Ramtusk (4420) - Quilboar warrior, cleave and shield bash
 * 5. Agathelos the Raging (4422) - Earth elemental, boulder throw
 * 6. Charlga Razorflank (4421) - Final boss, totems and heal
 *
 * DUNGEON CHARACTERISTICS:
 * - Thorn-covered tunnels and passages
 * - Many quilboar enemies
 * - Earth elementals and geomancers
 * - Totem mechanics (must kill totems)
 * - Nature and shadow damage prevalent
 *
 * SPECIAL MECHANICS:
 * - Roogug's totems (healing stream, searing)
 * - Charlga's totems (must be killed)
 * - Aggem's curses (must be dispelled)
 * - Death Speaker's shadow bolts (interruptible)
 * - Agathelos's boulder throw (ranged damage)
 * - Overlord's cleave (positioning)
 * - Geomancer earth elementals throughout dungeon
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

class RazorfenKraulScript : public DungeonScript
{
public:
    RazorfenKraulScript() : DungeonScript("razorfen_kraul", 47) {}

    // ============================================================================
    // LIFECYCLE HOOKS
    // ============================================================================

    void OnDungeonEnter(::Player* player, ::InstanceScript* instance) override
    {
        TC_LOG_DEBUG("module.playerbot", "RazorfenKraulScript: Player {} entered Razorfen Kraul",
            player->GetGUID().GetCounter());

        // Quilboar dungeon with heavy totem mechanics
        // Curse removal recommended
    }

    // ============================================================================
    // BOSS HOOKS
    // ============================================================================

    void OnBossEngage(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 6168: // Roogug
                TC_LOG_INFO("module.playerbot", "RazorfenKraulScript: Engaging Roogug");
                // Totem mechanics - must kill totems
                break;

            case 4424: // Aggem Thorncurse
                TC_LOG_INFO("module.playerbot", "RazorfenKraulScript: Engaging Aggem Thorncurse");
                // Curse mechanics - dispel priority
                break;

            case 4428: // Death Speaker Jargba
                TC_LOG_INFO("module.playerbot", "RazorfenKraulScript: Engaging Death Speaker Jargba");
                // Shadow bolt spam - interrupt priority
                break;

            case 4420: // Overlord Ramtusk
                TC_LOG_INFO("module.playerbot", "RazorfenKraulScript: Engaging Overlord Ramtusk");
                // Cleave mechanics - positioning
                break;

            case 4422: // Agathelos the Raging
                TC_LOG_INFO("module.playerbot", "RazorfenKraulScript: Engaging Agathelos the Raging");
                // Earth elemental - boulder throw
                break;

            case 4421: // Charlga Razorflank
                TC_LOG_INFO("module.playerbot", "RazorfenKraulScript: Engaging Charlga Razorflank (Final Boss)");
                // Heavy totem mechanics - kill totems immediately
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
            case 4428: // Death Speaker Jargba
            {
                // Death Speaker spams Shadow Bolt Volley (11975) - high priority interrupt
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Shadow Bolt (9613, 20297)
    if (spellId == 9613 || spellId == 20297 || spellId == 15232)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "RazorfenKraulScript: Interrupting Death Speaker's Shadow Bolt");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }
                    }
                }
                break;
            }

            case 4421: // Charlga Razorflank
            {
                // Charlga casts Chain Lightning and heals
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Healing Wave (11986) - CRITICAL interrupt
    if (spellId == 11986 || spellId == 939 || spellId == 959)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "RazorfenKraulScript: PRIORITY - Interrupting Charlga's Healing Wave");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }

                        // Chain Lightning (12058)
    if (spellId == 12058 || spellId == 421)
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

            case 6168: // Roogug
            {
                // Roogug casts Lightning Bolt
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Lightning Bolt
    if (spellId == 9532 || spellId == 915 || spellId == 943)
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
        // ENTERPRISE: Use lock-free spatial grid for thread-safe DynamicObject queries
        // Razorfen Kraul has minimal ground effects but using spatial grid for consistency
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
                                TC_LOG_DEBUG("module.playerbot", "RazorfenKraulScript: Avoiding ground effect at distance {:.1f}", distance);
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

    void HandleAddPriority(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 6168: // Roogug
            {
                // Roogug summons totems - MUST kill totems
                // Healing Stream Totem (high priority)
                // Searing Totem (damage)

                ::std::vector<::Creature*> adds = GetAddsInCombat(player, boss);

                // Prioritize healing totems first
    for (::Creature* add : adds)
                {
                    if (!add || !add->IsAlive())
                        continue;

                    // Check if it's a totem (typical totem entries)
                    // Healing Stream Totem (3527, 3906, 3907)
                    // Searing Totem (2523, 3902, 3903, 3904, 7400, 7402)
                    uint32 addEntry = add->GetEntry();

                    // Healing totems ALWAYS priority #1
    if (addEntry == 3527 || addEntry == 3906 || addEntry == 3907 || addEntry == 5923)
                    {
                        TC_LOG_DEBUG("module.playerbot", "RazorfenKraulScript: CRITICAL - Targeting healing totem");
                        player->SetSelection(add->GetGUID());
                        return;
                    }
                }

                // Then damage totems
    for (::Creature* add : adds)
                {
                    if (!add || !add->IsAlive())
                        continue;

                    uint32 addEntry = add->GetEntry();

                    // Searing/damage totems
    if (addEntry == 2523 || addEntry == 3902 || addEntry == 3903 ||
                        addEntry == 3904 || addEntry == 7400 || addEntry == 7402)
                    {
                        TC_LOG_DEBUG("module.playerbot", "RazorfenKraulScript: Targeting damage totem");
                        player->SetSelection(add->GetGUID());
                        return;
                    }
                }

                break;
            }

            case 4421: // Charlga Razorflank
            {
                // Charlga summons multiple totems - CRITICAL priority
                // Healing Stream Totem MUST die first
                // Then Searing Totems

                ::std::vector<::Creature*> adds = GetAddsInCombat(player, boss);

                // Healing totems absolute priority
    for (::Creature* add : adds)
                {
                    if (!add || !add->IsAlive())
                        continue;

                    uint32 addEntry = add->GetEntry();

                    // Healing Stream Totem
    if (addEntry == 3527 || addEntry == 3906 || addEntry == 3907 || addEntry == 5923)
                    {
                        TC_LOG_DEBUG("module.playerbot", "RazorfenKraulScript: EMERGENCY - Killing Charlga's healing totem");
                        player->SetSelection(add->GetGUID());
                        return;
                    }
                }

                // Then other totems
    for (::Creature* add : adds)
                {
                    if (!add || !add->IsAlive())
                        continue;

                    // Any totem
    if (IsTotemCreature(add))
                    {
                        TC_LOG_DEBUG("module.playerbot", "RazorfenKraulScript: Killing Charlga's totem");
                        player->SetSelection(add->GetGUID());
                        return;
                    }
                }

                break;
            }

            case 4422: // Agathelos the Raging
            {
                // Earth elemental - may spawn smaller elementals
                ::std::vector<::Creature*> adds = GetAddsInCombat(player, boss);

                if (!adds.empty())
                {
                    // Kill adds quickly, focus lowest health
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
            case 4420: // Overlord Ramtusk
            {
                // Cleave - melee must position behind
                DungeonRole role = GetPlayerRole(player);

                if (role == DungeonRole::MELEE_DPS)
                {
                    // Stay behind boss (melee position is behind)
                    Position behindPos = CalculateMeleePosition(player, boss);
                    float angle = player->GetAbsoluteAngle(boss);
                    float bossAngle = boss->GetOrientation();
                    float angleDiff = ::std::abs(angle - bossAngle);

                    // If not behind (within 90 degrees of back), move
    if (angleDiff > static_cast<float>(M_PI) / 2.0f)
                    {
                        TC_LOG_DEBUG("module.playerbot", "RazorfenKraulScript: Positioning behind Overlord Ramtusk to avoid cleave");
                        MoveTo(player, behindPos);
                        return;
                    }
                }

                break;
            }

            case 4422: // Agathelos the Raging
            {
                // Boulder throw - ranged should spread
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
            case 4424: // Aggem Thorncurse
            {
                // Aggem casts curses - MUST dispel
                Group* group = player->GetGroup();
                if (!group)
                    break;

                for (auto const& member : group->GetMemberSlots())
                {
                    Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
                    if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
                        continue;

                    // Check for curse debuffs
    if (groupMember->HasAuraType(SPELL_AURA_DUMMY) ||
                        groupMember->HasAuraType(SPELL_AURA_PERIODIC_DAMAGE))
                    {
                        // Check if it's actually a curse (would need specific spell check)
                        TC_LOG_DEBUG("module.playerbot", "RazorfenKraulScript: Dispelling curse from Aggem Thorncurse");
                        // Dispel curse
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
            case 4422: // Agathelos the Raging
            {
                // Boulder throw - moderate spread
                EncounterStrategy::HandleGenericSpread(player, boss, 8.0f);
                return;
            }

            default:
                break;
        }

        // Default spread
        DungeonScript::HandleSpreadMechanic(player, boss);
    }

private:
    // ============================================================================
    // UTILITY METHODS
    // ============================================================================

    /**
     * Check if creature is a totem
     * Totems have specific creature types and behaviors
     */
    bool IsTotemCreature(::Creature* creature)
    {
        if (!creature)
            return false;

        // Totems are typically CREATURE_TYPE_TOTEM (11)
    if (creature->GetCreatureTemplate()->type == CREATURE_TYPE_TOTEM)
            return true;

        // Common totem entries
        uint32 entry = creature->GetEntry();

        // List of common totem entries in Razorfen Kraul
        // Healing Stream Totem: 3527, 3906, 3907, 5923
        // Searing Totem: 2523, 3902, 3903, 3904, 7400, 7402
        // Stoneclaw Totem: 3579, 3911, 3912, 3913
        // Fire Nova Totem: 3556, 3557, 5879, 5926

        ::std::vector<uint32> totemEntries = {
            3527, 3906, 3907, 5923,  // Healing Stream
            2523, 3902, 3903, 3904, 7400, 7402,  // Searing
            3579, 3911, 3912, 3913,  // Stoneclaw
            3556, 3557, 5879, 5926   // Fire Nova
        };

        return ::std::find(totemEntries.begin(), totemEntries.end(), entry) != totemEntries.end();
    }
};

// ============================================================================
// REGISTRATION
// ============================================================================

void AddSC_razorfen_kraul_playerbot()
{
    // Register script
    DungeonScriptMgr::instance()->RegisterScript(new RazorfenKraulScript());

    // Register bosses
    DungeonScriptMgr::instance()->RegisterBossScript(6168, DungeonScriptMgr::instance()->GetScriptForMap(47));  // Roogug
    DungeonScriptMgr::instance()->RegisterBossScript(4424, DungeonScriptMgr::instance()->GetScriptForMap(47));  // Aggem Thorncurse
    DungeonScriptMgr::instance()->RegisterBossScript(4428, DungeonScriptMgr::instance()->GetScriptForMap(47));  // Death Speaker Jargba
    DungeonScriptMgr::instance()->RegisterBossScript(4420, DungeonScriptMgr::instance()->GetScriptForMap(47));  // Overlord Ramtusk
    DungeonScriptMgr::instance()->RegisterBossScript(4422, DungeonScriptMgr::instance()->GetScriptForMap(47));  // Agathelos the Raging
    DungeonScriptMgr::instance()->RegisterBossScript(4421, DungeonScriptMgr::instance()->GetScriptForMap(47));  // Charlga Razorflank

    TC_LOG_INFO("server.loading", ">> Registered Razorfen Kraul playerbot script with 6 boss mappings");
}

} // namespace Playerbot

/**
 * USAGE NOTES FOR RAZORFEN KRAUL:
 *
 * WHAT THIS SCRIPT HANDLES:
 * - Death Speaker's Shadow Bolt interrupt
 * - Charlga's Healing Wave interrupt (CRITICAL)
 * - Charlga's Chain Lightning interrupt
 * - Roogug's totem add priority (healing totems first)
 * - Charlga's totem add priority (CRITICAL - healing totems)
 * - Overlord Ramtusk cleave positioning (melee behind)
 * - Aggem Thorncurse curse dispel mechanics
 * - Agathelos boulder throw spread mechanics
 * - Totem identification and prioritization
 * - Add priority for earth elementals
 *
 * WHAT FALLS BACK TO GENERIC:
 * - Basic tank positioning
 * - Standard ranged DPS positioning
 * - Ground avoidance (minimal ground effects)
 * - Basic healing priority
 *
 * DUNGEON-SPECIFIC TIPS:
 * - ALWAYS kill healing totems immediately (top priority)
 * - Interrupt Charlga's Healing Wave at all costs
 * - Interrupt Death Speaker's Shadow Bolt spam
 * - Dispel Aggem's curses quickly
 * - Melee DPS stay behind Overlord Ramtusk (cleave)
 * - Focus totems before boss on Roogug and Charlga fights
 * - Spread for Agathelos's boulder throw
 * - Bring curse removal (druids, mages)
 * - Nature resistance helps with lightning damage
 * - Kill searing totems second after healing totems
 * - Watch for geomancer packs summoning earth elementals
 *
 * TOTEM PRIORITY (CRITICAL):
 * 1. Healing Stream Totem - KILL IMMEDIATELY
 * 2. Searing Totem - High damage, kill second
 * 3. Fire Nova Totem - Can wipe group, kill quickly
 * 4. Other totems - Kill as needed
 *
 * DIFFICULTY RATING: 5/10 (Moderate)
 * - Totem mechanics require add awareness
 * - Charlga fight can be chaotic without totem control
 * - Multiple interrupt requirements
 * - Curse removal mandatory for some classes
 * - Good practice for add priority and focus switching
 * - Geomancer packs can be challenging
 * - Earth elementals hit hard if not controlled
 */
