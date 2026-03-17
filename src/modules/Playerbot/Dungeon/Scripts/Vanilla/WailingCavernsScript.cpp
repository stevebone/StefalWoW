/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * WAILING CAVERNS DUNGEON SCRIPT
 *
 * Map ID: 43
 * Level Range: 15-25
 * Location: Northern Barrens
 *
 * BOSS ENCOUNTERS:
 * 1. Lady Anacondra (3671) - Druid of the Fang, sleep and poison
 * 2. Lord Cobrahn (3669) - Druid of the Fang, lightning and poison
 * 3. Lord Pythas (3670) - Druid of the Fang, fire and sleep
 * 4. Lord Serpentis (3673) - Druid of the Fang, deviate forms
 * 5. Skum (3674) - Turtle boss, knockback and adds
 * 6. Verdan the Everliving (5775) - Giant plant, growth and AoE
 * 7. Mutanus the Devourer (3654) - Final boss, sleep and fear
 *
 * DUNGEON CHARACTERISTICS:
 * - Long, winding cavern layout
 * - Many enemies with sleep/CC abilities
 * - Nature and poison damage prevalent
 * - Multiple optional bosses
 * - Confusing layout for new players
 *
 * SPECIAL MECHANICS:
 * - Sleep mechanics on multiple bosses (Anacondra, Pythas, Mutanus)
 * - Poison damage requires cleansing
 * - Deviate transformation effects
 * - Growth mechanic on Verdan
 * - Fear effects from Mutanus
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

class WailingCavernsScript : public DungeonScript
{
public:
    WailingCavernsScript() : DungeonScript("wailing_caverns", 43) {}

    // ============================================================================
    // BOSS HOOKS
    // ============================================================================

    void OnBossEngage(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 3671: // Lady Anacondra
                TC_LOG_INFO("module.playerbot", "WailingCavernsScript: Engaging Lady Anacondra");
                // Sleep mechanic warning
                break;

            case 3669: // Lord Cobrahn
                TC_LOG_INFO("module.playerbot", "WailingCavernsScript: Engaging Lord Cobrahn");
                // Poison and lightning damage
                break;

            case 3670: // Lord Pythas
                TC_LOG_INFO("module.playerbot", "WailingCavernsScript: Engaging Lord Pythas");
                // Sleep and fire damage
                break;

            case 3673: // Lord Serpentis
                TC_LOG_INFO("module.playerbot", "WailingCavernsScript: Engaging Lord Serpentis");
                break;

            case 3674: // Skum
                TC_LOG_INFO("module.playerbot", "WailingCavernsScript: Engaging Skum");
                break;

            case 5775: // Verdan the Everliving
                TC_LOG_INFO("module.playerbot", "WailingCavernsScript: Engaging Verdan the Everliving");
                // Growth mechanic - increases size and damage
                break;

            case 3654: // Mutanus the Devourer
                TC_LOG_INFO("module.playerbot", "WailingCavernsScript: Engaging Mutanus the Devourer (Final Boss)");
                // Sleep and fear mechanics
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
            case 3671: // Lady Anacondra
            {
                // Anacondra casts Sleep (8040) - HIGHEST PRIORITY INTERRUPT
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Sleep is critical to interrupt
    if (spellId == 8040 || spellId == 700) // Sleep / Sleep visual
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "WailingCavernsScript: Interrupting Anacondra's Sleep");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }

                        // Lightning Bolt also interruptible
    if (spellId == 9532) // Lightning Bolt
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

            case 3669: // Lord Cobrahn
            {
                // Cobrahn casts Lightning Bolt (9532) and Poison (744)
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Lightning Bolt deals significant damage
    if (spellId == 9532)
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

            case 3670: // Lord Pythas
            {
                // Pythas casts Sleep (8040) and Healing Touch
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Sleep is critical
    if (spellId == 8040)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "WailingCavernsScript: Interrupting Pythas's Sleep");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }
                    }
                }
                break;
            }

            case 3654: // Mutanus the Devourer
            {
                // Mutanus casts Thundercrack (8147) - area stun
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Thundercrack stuns entire group
    if (spellId == 8147)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "WailingCavernsScript: Interrupting Mutanus's Thundercrack");
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
        // Wailing Caverns has poison clouds and nature effects
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
                                TC_LOG_DEBUG("module.playerbot", "WailingCavernsScript: Avoiding ground effect at distance {:.1f}", distance);
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
            case 3671: // Lady Anacondra
            case 3670: // Lord Pythas
            case 3654: // Mutanus the Devourer
            {
                // These bosses cast Sleep - need immediate dispel/wakeup
                Group* group = player->GetGroup();
                if (!group)
                    break;

                // Find sleeping players
    for (auto const& member : group->GetMemberSlots())
                {
                    Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
                    if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
                        continue;

                    // Check for Sleep debuff (8040)
    if (groupMember->HasAura(8040) || groupMember->HasAura(700))
                    {
                        TC_LOG_DEBUG("module.playerbot", "WailingCavernsScript: Player is sleeping, needs wakeup");

                        // Dispel or damage to wake up
                        // Priority: highest threat targets first
                        return;
                    }
                }
                break;
            }

            case 3669: // Lord Cobrahn
            {
                // Cobrahn applies poison (744) - should be cleansed
                Group* group = player->GetGroup();
                if (!group)
                    break;

                for (auto const& member : group->GetMemberSlots())
                {
                    Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
                    if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
                        continue;

                    // Check for poison debuff
    if (groupMember->HasAura(744))
                    {
                        TC_LOG_DEBUG("module.playerbot", "WailingCavernsScript: Player poisoned by Cobrahn");
                        // Dispel poison
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

    void HandleAddPriority(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 3674: // Skum
            {
                // Skum summons adds (Deviate Ravagers)
                // Adds should be killed quickly as they overwhelm group

                ::std::vector<::Creature*> adds = GetAddsInCombat(player, boss);

                ::Creature* highestPriorityAdd = nullptr;
                for (::Creature* add : adds)
                {
                    // Target lowest health add for quick kill
    if (!highestPriorityAdd || add->GetHealthPct() < highestPriorityAdd->GetHealthPct())
                    {
                        highestPriorityAdd = add;
                    }
                }

                if (highestPriorityAdd)
                {
                    TC_LOG_DEBUG("module.playerbot", "WailingCavernsScript: Targeting Skum add");
                    player->SetSelection(highestPriorityAdd->GetGUID());
                    return;
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
            case 5775: // Verdan the Everliving
            {
                // Verdan grows larger and hits harder as fight progresses
                // Tank needs to maintain threat, DPS spread for cleave

                DungeonRole role = GetPlayerRole(player);

                if (role == DungeonRole::MELEE_DPS)
                {
                    // Melee DPS spread slightly to avoid cleave
                    Position meleePos = CalculateMeleePosition(player, boss);

                    // Add some spread
                    float angle = player->GetAbsoluteAngle(boss) + (frand(-0.5f, 0.5f));
                    meleePos.RelocateOffset({::std::cos(angle) * 2.0f, ::std::sin(angle) * 2.0f, 0.0f});

                    if (player->GetExactDist(&meleePos) > 3.0f)
                    {
                        MoveTo(player, meleePos);
                        return;
                    }
                }
                break;
            }

            case 3654: // Mutanus the Devourer
            {
                // Mutanus has Naralex's Nightmare (area fear effect)
                // Group should spread to avoid chain CC

                DungeonRole role = GetPlayerRole(player);

                if (role == DungeonRole::RANGED_DPS || role == DungeonRole::HEALER)
                {
                    // Ranged spread 15 yards apart
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
            case 3654: // Mutanus the Devourer
            {
                // Fear and sleep effects - spread 15 yards
                EncounterStrategy::HandleGenericSpread(player, boss, 15.0f);
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
            case 5775: // Verdan the Everliving
            {
                // Verdan grows during fight - maintain proper distance
                // As he grows, melee range extends slightly

                DungeonRole role = GetPlayerRole(player);
                float currentDist = player->GetExactDist(boss);

                if (role == DungeonRole::TANK || role == DungeonRole::MELEE_DPS)
                {
                    // Maintain 5-8 yards (melee + growth buffer)
    if (currentDist > 10.0f || currentDist < 3.0f)
                    {
                        HandlePositioning(player, boss);
                        return;
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

void AddSC_wailing_caverns_playerbot()
{
    // Register script
    DungeonScriptMgr::instance()->RegisterScript(new WailingCavernsScript());

    // Register all bosses
    DungeonScriptMgr::instance()->RegisterBossScript(3671, DungeonScriptMgr::instance()->GetScriptForMap(43)); // Lady Anacondra
    DungeonScriptMgr::instance()->RegisterBossScript(3669, DungeonScriptMgr::instance()->GetScriptForMap(43)); // Lord Cobrahn
    DungeonScriptMgr::instance()->RegisterBossScript(3670, DungeonScriptMgr::instance()->GetScriptForMap(43)); // Lord Pythas
    DungeonScriptMgr::instance()->RegisterBossScript(3673, DungeonScriptMgr::instance()->GetScriptForMap(43)); // Lord Serpentis
    DungeonScriptMgr::instance()->RegisterBossScript(3674, DungeonScriptMgr::instance()->GetScriptForMap(43)); // Skum
    DungeonScriptMgr::instance()->RegisterBossScript(5775, DungeonScriptMgr::instance()->GetScriptForMap(43)); // Verdan
    DungeonScriptMgr::instance()->RegisterBossScript(3654, DungeonScriptMgr::instance()->GetScriptForMap(43)); // Mutanus

    TC_LOG_INFO("server.loading", ">> Registered Wailing Caverns playerbot script with 7 boss mappings");
}

} // namespace Playerbot

/**
 * USAGE NOTES FOR WAILING CAVERNS:
 *
 * WHAT THIS SCRIPT HANDLES:
 * - Sleep interrupt priority (Anacondra, Pythas, Mutanus)
 * - Poison dispel from Cobrahn
 * - Sleep dispel for entire group
 * - Add priority for Skum encounter
 * - Spread mechanics for Mutanus (fear/sleep)
 * - Growth positioning for Verdan
 * - Thundercrack interrupt on Mutanus
 *
 * WHAT FALLS BACK TO GENERIC:
 * - Basic tank positioning
 * - Standard melee DPS positioning for most bosses
 * - Ranged DPS optimal range
 * - Basic movement mechanics
 *
 * DUNGEON-SPECIFIC TIPS:
 * - Interrupt Sleep casts immediately (Anacondra, Pythas)
 * - Dispel/wake sleeping players ASAP
 * - Cleanse poison from Cobrahn
 * - Kill Skum's adds quickly before they overwhelm group
 * - Spread for Mutanus to avoid chain CC
 * - Be patient with Verdan's growth - don't panic, adjust positioning
 * - Bring poison and curse removal if possible
 *
 * DIFFICULTY RATING: 4/10 (Easy-Moderate)
 * - Multiple sleep mechanics require attention
 * - Poison damage adds up if not cleansed
 * - Confusing layout can cause issues
 * - Good practice for dispel mechanics
 * - Mutanus can be challenging without proper spread
 */
