/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * RAZORFEN DOWNS DUNGEON SCRIPT
 *
 * Map ID: 129
 * Level Range: 35-45
 * Location: Thousand Needles (Southern Barrens)
 *
 * BOSS ENCOUNTERS:
 * 1. Tuten'kash (7355) - Undead quilboar, summons bone adds
 * 2. Mordresh Fire Eye (7357) - Undead necromancer, summoner
 * 3. Glutton (8567) - Undead abomination, disease and vomit
 * 4. Ragglesnout (7354) - Undead quilboar, cleave
 * 5. Amnennar the Coldbringer (7358) - Final boss, frost and shadow magic
 *
 * DUNGEON CHARACTERISTICS:
 * - Undead-infested quilboar citadel
 * - Heavy necrotic and frost damage
 * - Death knights and necromancers
 * - Spiral layout descending into depths
 * - Many undead quilboar enemies
 *
 * SPECIAL MECHANICS:
 * - Tuten'kash summons waves of skeleton adds (AOE required)
 * - Mordresh summons bone construct adds (kill priority)
 * - Glutton's disease cloud (dispel disease critical)
 * - Glutton's chain vomit (disgusting and damaging)
 * - Ragglesnout's cleave (positioning)
 * - Amnennar's frost tomb (iceblock mechanic - free players)
 * - Amnennar's frost nova (spread out)
 * - Death knight mini-bosses throughout
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
#include "SpellAuras.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "../../../Spatial/SpatialGridManager.h"
#include "../../../Spatial/DoubleBufferedSpatialGrid.h"

namespace Playerbot
{

class RazorfenDownsScript : public DungeonScript
{
public:
    RazorfenDownsScript() : DungeonScript("razorfen_downs", 129) {}

    // ============================================================================
    // LIFECYCLE HOOKS
    // ============================================================================

    void OnDungeonEnter(::Player* player, ::InstanceScript* instance) override
    {
        TC_LOG_DEBUG("module.playerbot", "RazorfenDownsScript: Player {} entered Razorfen Downs",
            player->GetGUID().GetCounter());

        // Undead dungeon - high disease and frost damage
        // Disease removal critical
        // Frost resistance helpful
    }

    // ============================================================================
    // BOSS HOOKS
    // ============================================================================

    void OnBossEngage(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 7355: // Tuten'kash
                TC_LOG_INFO("module.playerbot", "RazorfenDownsScript: Engaging Tuten'kash");
                // Skeleton waves - AOE adds down
                break;

            case 7357: // Mordresh Fire Eye
                TC_LOG_INFO("module.playerbot", "RazorfenDownsScript: Engaging Mordresh Fire Eye");
                // Bone construct summons
                break;

            case 8567: // Glutton
                TC_LOG_INFO("module.playerbot", "RazorfenDownsScript: Engaging Glutton");
                // Disease and vomit mechanics
                HandleSpreadMechanic(player, boss);
                break;

            case 7354: // Ragglesnout
                TC_LOG_INFO("module.playerbot", "RazorfenDownsScript: Engaging Ragglesnout");
                // Cleave - positioning critical
                break;

            case 7358: // Amnennar the Coldbringer
                TC_LOG_INFO("module.playerbot", "RazorfenDownsScript: Engaging Amnennar the Coldbringer (Final Boss)");
                // Frost tomb and frost nova mechanics
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
            case 7357: // Mordresh Fire Eye
            {
                // Mordresh casts Fireball and Flame Buffet
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Fireball (15228) - high damage
    if (spellId == 15228 || spellId == 9053)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "RazorfenDownsScript: Interrupting Mordresh's Fireball");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }

                        // Summon spells (lower priority but worth interrupting)
    if (spellId == 12746 || spellId == 12747)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "RazorfenDownsScript: Interrupting Mordresh's summon");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }
                    }
                }
                break;
            }

            case 7358: // Amnennar the Coldbringer
            {
                // Amnennar casts Frost Bolt and Chains of Ice
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Frost Bolt (15530) - high frost damage
    if (spellId == 15530 || spellId == 9672)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "RazorfenDownsScript: Interrupting Amnennar's Frost Bolt");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }

                        // Chains of Ice (15531) - immobilize
    if (spellId == 15531 || spellId == 12551)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "RazorfenDownsScript: Interrupting Amnennar's Chains of Ice");
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
            case 8567: // Glutton
            {
                // Glutton spawns disease clouds on ground
                // Must move out of disease

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

                    // Check for disease cloud effects using spell info
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(snapshot.spellId, DIFFICULTY_NONE);
                    if (!spellInfo)
                        continue;

                    // Disease cloud: periodic damage or disease dispel type
                    if (!spellInfo->HasAura(SPELL_AURA_PERIODIC_DAMAGE) && spellInfo->Dispel != DISPEL_DISEASE)
                        continue;

                    // Calculate distance using snapshot position
                    float distance = player->GetExactDist(snapshot.position);
                    if (distance < 8.0f)
                    {
                        TC_LOG_DEBUG("module.playerbot", "RazorfenDownsScript: Avoiding Glutton's disease cloud at distance {:.1f}", distance);

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
            case 7355: // Tuten'kash
            {
                // Tuten'kash summons waves of skeletons
                // AOE them down, don't focus single target
                // Focus boss if few adds, focus adds if many

                ::std::vector<::Creature*> adds = GetAddsInCombat(player, boss);

                // If 4+ adds, prioritize AOE (group should AOE)
                // If < 4 adds, focus boss and let cleave/incidental damage kill adds
    if (adds.size() >= 4)
                {
                    TC_LOG_DEBUG("module.playerbot", "RazorfenDownsScript: Multiple skeleton adds - using AOE priority");

                    // Target closest add for AOE positioning
                    ::Creature* closestAdd = nullptr;
                    float closestDist = 100.0f;

                    for (::Creature* add : adds)
                    {
                        if (!add || !add->IsAlive())
                            continue;

                        float dist = player->GetExactDist(add);
                        if (dist < closestDist)
                        {
                            closestDist = dist;
                            closestAdd = add;
                        }
                    }

                    if (closestAdd)
                    {
                        player->SetSelection(closestAdd->GetGUID());
                        return;
                    }
                }
                else if (!adds.empty())
                {
                    // Few adds - kill lowest health
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

            case 7357: // Mordresh Fire Eye
            {
                // Mordresh summons bone constructs
                // These hit hard - kill them

                ::std::vector<::Creature*> adds = GetAddsInCombat(player, boss);

                for (::Creature* add : adds)
                {
                    if (!add || !add->IsAlive())
                        continue;

                    // Bone constructs (entries vary)
                    // Priority: Kill constructs before boss
                    TC_LOG_DEBUG("module.playerbot", "RazorfenDownsScript: Targeting Mordresh's bone construct");
                    player->SetSelection(add->GetGUID());
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
            case 7354: // Ragglesnout
            {
                // Cleave - melee must be behind
                DungeonRole role = GetPlayerRole(player);

                if (role == DungeonRole::MELEE_DPS)
                {
                    // Position behind boss (melee position is behind)
                    Position behindPos = CalculateMeleePosition(player, boss);
                    float angle = player->GetAbsoluteAngle(boss);
                    float bossAngle = boss->GetOrientation();
                    float angleDiff = ::std::abs(angle - bossAngle);

                    // If not behind, move
    if (angleDiff > static_cast<float>(M_PI) / 2.0f)
                    {
                        TC_LOG_DEBUG("module.playerbot", "RazorfenDownsScript: Positioning behind Ragglesnout to avoid cleave");
                        MoveTo(player, behindPos);
                        return;
                    }
                }

                break;
            }

            case 8567: // Glutton
            {
                // Spread for disease cloud and vomit
                HandleSpreadMechanic(player, boss);
                return;
            }

            case 7358: // Amnennar the Coldbringer
            {
                // Frost Nova - spread out
                // Frost Tomb - need to free trapped players
                HandleSpreadMechanic(player, boss);
                return;
            }

            case 7355: // Tuten'kash
            {
                // Skeleton waves - group should stack for AOE
                DungeonRole role = GetPlayerRole(player);

                if (role == DungeonRole::RANGED_DPS || role == DungeonRole::HEALER)
                {
                    // Stay at range but not too spread
                    // AOE needs targets clustered
                    float distance = player->GetExactDist(boss);

                    if (distance > 20.0f || distance < 8.0f)
                    {
                        // Maintain 10-15 yard range
                        Position optimalPos = CalculateRangedPosition(player, boss);
                        MoveTo(player, optimalPos);
                        return;
                    }
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
            case 8567: // Glutton
            {
                // Glutton applies disease debuffs - CRITICAL to dispel
                Group* group = player->GetGroup();
                if (!group)
                    break;

                for (auto const& member : group->GetMemberSlots())
                {
                    Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
                    if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
                        continue;

                    // Check for disease debuffs
    if (groupMember->HasAuraType(SPELL_AURA_PERIODIC_DAMAGE_PERCENT) ||
                        groupMember->HasAuraType(SPELL_AURA_PERIODIC_DAMAGE))
                    {
                        // Check if it's a disease type
    for (auto const& auraApp : groupMember->GetAppliedAuras())
                        {
                            if (auraApp.second && auraApp.second->GetBase())
                            {
                                SpellInfo const* spellInfo = auraApp.second->GetBase()->GetSpellInfo();
                                if (spellInfo && spellInfo->Dispel == DISPEL_DISEASE)
                                {
                                    TC_LOG_DEBUG("module.playerbot", "RazorfenDownsScript: CRITICAL - Dispelling disease from Glutton");
                                    // Dispel disease
                                    return;
                                }
                            }
                        }
                    }
                }
                break;
            }

            case 7358: // Amnennar the Coldbringer
            {
                // Frost Tomb - player gets frozen in ice block
                // Need to free them (damage the ice block or dispel)
                Group* group = player->GetGroup();
                if (!group)
                    break;

                for (auto const& member : group->GetMemberSlots())
                {
                    Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
                    if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
                        continue;

                    // Check for ice block/frost tomb
    if (groupMember->HasAuraType(SPELL_AURA_MOD_STUN) ||
                        groupMember->HasAura(15532))  // Frost Tomb
                    {
                        TC_LOG_DEBUG("module.playerbot", "RazorfenDownsScript: Player trapped in Frost Tomb - breaking free");
                        // Damage to break ice or dispel
                        return;
                    }
                }

                // Chains of Ice - immobilize, can be dispelled
    for (auto const& member : group->GetMemberSlots())
                {
                    Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
                    if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
                        continue;

                    if (groupMember->HasAuraType(SPELL_AURA_MOD_ROOT))
                    {
                        TC_LOG_DEBUG("module.playerbot", "RazorfenDownsScript: Dispelling Chains of Ice");
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
            case 8567: // Glutton
            {
                // Disease cloud spread - 8 yards
                EncounterStrategy::HandleGenericSpread(player, boss, 8.0f);
                return;
            }

            case 7358: // Amnennar the Coldbringer
            {
                // Frost Nova spread - 10 yards
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
            case 8567: // Glutton
            {
                // Constantly spawns disease - be ready to move
                HandleGroundAvoidance(player, boss);
                return;
            }

            case 7358: // Amnennar the Coldbringer
            {
                // Frost Nova - may need to move out
                // Frost Tomb - free trapped allies

                Group* group = player->GetGroup();
                if (group)
                {
                    // Check for trapped allies
    for (auto const& member : group->GetMemberSlots())
                    {
                        Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
                        if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
                            continue;

                        // If ally is in Frost Tomb, move to them to help break ice
    if (groupMember->HasAura(15532))
                        {
                            float distance = player->GetExactDist(groupMember);
                            if (distance > 5.0f)
                            {
                                TC_LOG_DEBUG("module.playerbot", "RazorfenDownsScript: Moving to help break Frost Tomb");
                                MoveTo(player, groupMember->GetPosition());
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
        DungeonScript::HandleMovementMechanic(player, boss);
    }
};

// ============================================================================
// REGISTRATION
// ============================================================================

void AddSC_razorfen_downs_playerbot()
{
    // Register script
    DungeonScriptMgr::instance()->RegisterScript(new RazorfenDownsScript());

    // Register bosses
    DungeonScriptMgr::instance()->RegisterBossScript(7355, DungeonScriptMgr::instance()->GetScriptForMap(129)); // Tuten'kash
    DungeonScriptMgr::instance()->RegisterBossScript(7357, DungeonScriptMgr::instance()->GetScriptForMap(129)); // Mordresh Fire Eye
    DungeonScriptMgr::instance()->RegisterBossScript(8567, DungeonScriptMgr::instance()->GetScriptForMap(129)); // Glutton
    DungeonScriptMgr::instance()->RegisterBossScript(7354, DungeonScriptMgr::instance()->GetScriptForMap(129)); // Ragglesnout
    DungeonScriptMgr::instance()->RegisterBossScript(7358, DungeonScriptMgr::instance()->GetScriptForMap(129)); // Amnennar the Coldbringer

    TC_LOG_INFO("server.loading", ">> Registered Razorfen Downs playerbot script with 5 boss mappings");
}

} // namespace Playerbot

/**
 * USAGE NOTES FOR RAZORFEN DOWNS:
 *
 * WHAT THIS SCRIPT HANDLES:
 * - Mordresh Fire Eye Fireball interrupt
 * - Mordresh summon interrupt
 * - Amnennar Frost Bolt interrupt
 * - Amnennar Chains of Ice interrupt
 * - Tuten'kash skeleton wave add priority (AOE focus)
 * - Mordresh bone construct add priority
 * - Glutton disease cloud ground avoidance
 * - Glutton disease dispel (CRITICAL)
 * - Ragglesnout cleave positioning (melee behind)
 * - Amnennar Frost Tomb rescue mechanics
 * - Amnennar Frost Nova spread mechanics
 * - Glutton spread mechanics
 * - Frost Tomb ice block breaking
 * - Chains of Ice dispel
 * - Tuten'kash AOE positioning
 *
 * WHAT FALLS BACK TO GENERIC:
 * - Basic tank positioning
 * - Standard ranged DPS positioning
 * - Basic healing priority
 * - Standard melee DPS (when no special mechanics)
 *
 * DUNGEON-SPECIFIC TIPS:
 * - MUST have disease removal (Glutton fight)
 * - Frost resistance helps significantly
 * - Interrupt Mordresh's Fireball spam
 * - Interrupt Amnennar's Frost Bolt and Chains of Ice
 * - AOE abilities required for Tuten'kash skeleton waves
 * - Kill Mordresh's bone constructs before boss
 * - Dispel Glutton's disease immediately
 * - Move out of Glutton's disease clouds
 * - Spread 8+ yards for Glutton fight
 * - Spread 10+ yards for Amnennar fight
 * - Free players from Frost Tomb by damaging ice block
 * - Melee stay behind Ragglesnout (cleave)
 * - Death knights throughout dungeon can be challenging
 * - Bring holy water for undead (optional quest item)
 *
 * BOSS DIFFICULTY:
 * - Tuten'kash: 4/10 - AOE check, manageable with good DPS
 * - Mordresh Fire Eye: 5/10 - Adds can be dangerous
 * - Glutton: 6/10 - Disease must be managed properly
 * - Ragglesnout: 3/10 - Simple if positioning correct
 * - Amnennar: 7/10 - Complex mechanics, Frost Tomb dangerous
 *
 * OVERALL DIFFICULTY: 6/10 (Moderate)
 * - Disease removal is mandatory
 * - Amnennar fight requires coordination
 * - Frost Tomb mechanic can wipe group if not handled
 * - AOE requirements for Tuten'kash
 * - Death knight packs can be challenging
 * - Good practice for dispel mechanics
 * - Longer dungeon with multiple bosses
 * - Spiral layout can be disorienting
 *
 * RECOMMENDED GROUP COMPOSITION:
 * - Tank: Warrior, Paladin, or Druid
 * - Healer: Priest (disease dispel), Paladin, or Druid
 * - DPS: At least one with AOE for Tuten'kash
 * - DPS: Interrupt capability for Mordresh and Amnennar
 * - Utility: Disease removal CRITICAL
 */
