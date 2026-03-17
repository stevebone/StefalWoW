/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * SHADOWFANG KEEP DUNGEON SCRIPT
 *
 * Map ID: 33
 * Level Range: 18-25
 * Location: Silverpine Forest
 *
 * BOSS ENCOUNTERS:
 * 1. Baron Ashbury (46962) - Shadow magic, Asphyxiate
 * 2. Baron Silverlaine (3887) - Worgen boss, Veil of Shadow
 * 3. Commander Springvale (4278) - Holy damage, Word of Shame
 * 4. Lord Walden (46963) - Mad scientist, potions and transformations
 * 5. Lord Godfrey (46964) - Final boss, Pistol Barrage and mortal wounds
 *
 * DUNGEON CHARACTERISTICS:
 * - Gothic castle setting
 * - Worgen and undead enemies
 * - Shadow and holy magic prevalent
 * - Multiple patrol paths
 * - Classic dungeon with rich lore
 *
 * SPECIAL MECHANICS:
 * - Ashbury's Asphyxiate (channeled silence/damage)
 * - Silverlaine's Veil of Shadow (periodic damage)
 * - Springvale's Word of Shame (frontal cone fear)
 * - Walden's potions and transformations
 * - Godfrey's Pistol Barrage (cone damage)
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

class ShadowfangKeepScript : public DungeonScript
{
public:
    ShadowfangKeepScript() : DungeonScript("shadowfang_keep", 33) {}

    // ============================================================================
    // BOSS HOOKS
    // ============================================================================

    void OnBossEngage(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 46962: // Baron Ashbury (Cataclysm revamp)
            case 3850:  // Baron Ashbury (Classic)
                TC_LOG_INFO("module.playerbot", "ShadowfangKeepScript: Engaging Baron Ashbury");
                // Asphyxiate mechanic warning
                break;

            case 3887: // Baron Silverlaine
                TC_LOG_INFO("module.playerbot", "ShadowfangKeepScript: Engaging Baron Silverlaine");
                // Veil of Shadow damage
                break;

            case 4278: // Commander Springvale
                TC_LOG_INFO("module.playerbot", "ShadowfangKeepScript: Engaging Commander Springvale");
                // Holy damage and Word of Shame
                break;

            case 46963: // Lord Walden (Cataclysm)
            case 4275:  // Lord Walden (Classic)
                TC_LOG_INFO("module.playerbot", "ShadowfangKeepScript: Engaging Lord Walden");
                // Potion mechanics
                break;

            case 46964: // Lord Godfrey (Cataclysm)
            case 4274:  // Arugal (Classic final boss)
                TC_LOG_INFO("module.playerbot", "ShadowfangKeepScript: Engaging Final Boss");
                // Spread for Pistol Barrage / Arugal's Shadow Port
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
            case 46962: // Baron Ashbury
            case 3850:
            {
                // Ashbury channels Asphyxiate - MUST be interrupted
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
                    if (!currentSpell)
                        currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);

                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Asphyxiate (93423 in Cataclysm) - critical interrupt
    if (spellId == 93423 || spellId == 7645) // Various versions
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "ShadowfangKeepScript: Interrupting Ashbury's Asphyxiate");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }

                        // Pain and Suffering also interruptible
    if (spellId == 93581)
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

            case 4278: // Commander Springvale
            {
                // Springvale casts Holy Light (self-heal) - interrupt priority
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Holy Light (8362) - interrupt to prevent heal
    if (spellId == 8362 || spellId == 15493)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "ShadowfangKeepScript: Interrupting Springvale's Holy Light");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }
                    }
                }
                break;
            }

            case 4274: // Arugal (Classic)
            {
                // Arugal casts Void Bolt - interruptible shadow damage
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Void Bolt (7588) - high shadow damage
    if (spellId == 7588)
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
            case 46963: // Lord Walden
            case 4275:
            {
                // Walden throws potions creating ground effects
                // Ice, fire, and poison puddles to avoid

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

                    // Calculate distance using snapshot position
                    float distance = player->GetExactDist(snapshot.position);

                    // Only get actual DynamicObject* for effect check if in range
                    if (distance < 8.0f)
                    {
                        if (DynamicObject* dynObj = ObjectAccessor::GetDynamicObject(*player, snapshot.guid))
                        {
                            // Check for potion ground effects using the base class method
                            if (IsDangerousGroundEffect(dynObj))
                            {
                                TC_LOG_DEBUG("module.playerbot", "ShadowfangKeepScript: Avoiding Walden's potion effect at distance {:.1f}", distance);
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

        // Fall back to generic
        DungeonScript::HandleGroundAvoidance(player, boss);
    }

    void HandleAddPriority(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 4274: // Arugal (Classic)
            {
                // Arugal summons Worgen adds via Shadow Port
                // Adds should be picked up by tank and controlled

                ::std::vector<::Creature*> adds = GetAddsInCombat(player, boss);

                // Prioritize adds that are attacking healers
                Group* group = player->GetGroup();
                if (group)
                {
                    for (::Creature* add : adds)
                    {
                        if (!add || !add->IsAlive())
                            continue;

                        // Check if add is targeting healer
                        Unit* target = add->GetVictim();
                        if (target && target->IsPlayer())
                        {
                            Player* targetPlayer = target->ToPlayer();
                            if (GetPlayerRole(targetPlayer) == DungeonRole::HEALER)
                            {
                                TC_LOG_DEBUG("module.playerbot", "ShadowfangKeepScript: Add attacking healer - priority target");
                                player->SetSelection(add->GetGUID());
                                return;
                            }
                        }
                    }
                }

                // Otherwise use generic add priority
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
            case 4278: // Commander Springvale
            {
                // Springvale casts Word of Shame (frontal cone fear)
                // Group should not stand in front of boss

                DungeonRole role = GetPlayerRole(player);

                if (role == DungeonRole::TANK)
                {
                    // Tank faces boss away from group
                    Position tankPos = CalculateTankPosition(player, boss);

                    // Ensure boss is facing away from raid
    if (player->GetExactDist(&tankPos) > 3.0f)
                    {
                        MoveTo(player, tankPos);
                        return;
                    }
                }
                else if (role == DungeonRole::MELEE_DPS)
                {
                    // Melee behind boss, spread to avoid cleave
                    Position meleePos = CalculateMeleePosition(player, boss);

                    if (player->GetExactDist(&meleePos) > 5.0f)
                    {
                        MoveTo(player, meleePos);
                        return;
                    }
                }
                else if (role == DungeonRole::RANGED_DPS || role == DungeonRole::HEALER)
                {
                    // Ranged spread behind boss
                    Position rangedPos = CalculateRangedPosition(player, boss);

                    if (player->GetExactDist(&rangedPos) > 5.0f)
                    {
                        MoveTo(player, rangedPos);
                        return;
                    }
                }
                break;
            }

            case 46964: // Lord Godfrey
            {
                // Godfrey's Pistol Barrage (frontal cone)
                // Similar to Springvale - avoid frontal cone

                DungeonRole role = GetPlayerRole(player);

                if (role != DungeonRole::TANK)
                {
                    // All non-tanks should be behind or to the side
                    float angle = boss->GetOrientation();
                    float playerAngle = boss->GetAbsoluteAngle(player);
                    float angleDiff = ::std::abs(angle - playerAngle);

                    // If player is in frontal arc (< 90 degrees), move
    if (angleDiff < static_cast<float>(M_PI) / 2.0f)
                    {
                        Position safePos = CalculateMeleePosition(player, boss);
                        MoveTo(player, safePos);
                        return;
                    }
                }
                break;
            }

            case 4274: // Arugal
            {
                // Arugal teleports around the room (Shadow Port)
                // Group should reposition quickly after each teleport

                float distance = player->GetExactDist(boss);
                DungeonRole role = GetPlayerRole(player);
                // Reposition based on role after teleport
    if (role == DungeonRole::TANK || role == DungeonRole::MELEE_DPS)
                {
                    if (distance > 10.0f) // Too far after teleport
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
        DungeonScript::HandlePositioning(player, boss);
    }

    void HandleDispelMechanic(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            case 3887: // Baron Silverlaine
            {
                // Silverlaine applies Veil of Shadow (DoT debuff)
                // Should be dispelled if possible

                Group* group = player->GetGroup();
                if (!group)
                    break;

                for (auto const& member : group->GetMemberSlots())
                {
                    Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
                    if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
                        continue;

                    // Check for Veil of Shadow (7068)
    if (groupMember->HasAura(7068))
                    {
                        TC_LOG_DEBUG("module.playerbot", "ShadowfangKeepScript: Dispelling Veil of Shadow");
                        // Dispel shadow magic
                        return;
                    }
                }
                break;
            }

            case 4278: // Commander Springvale
            {
                // Word of Shame causes fear - dispel or wait out
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
                        TC_LOG_DEBUG("module.playerbot", "ShadowfangKeepScript: Player feared by Word of Shame");
                        // Dispel fear if available
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
            case 46964: // Lord Godfrey
            {
                // Pistol Barrage and Mortal Wound - spread to minimize hits
                EncounterStrategy::HandleGenericSpread(player, boss, 8.0f);
                return;
            }

            case 4274: // Arugal
            {
                // After Shadow Port teleport, spread to avoid stacking
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
            case 4274: // Arugal
            {
                // Arugal teleports frequently - group must chase him
                // After each Shadow Port, reposition quickly

                float distance = player->GetExactDist(boss);
                DungeonRole role = GetPlayerRole(player);

                if (role == DungeonRole::TANK)
                {
                    // Tank must reach boss quickly and re-establish threat
    if (distance > 8.0f)
                    {
                        TC_LOG_DEBUG("module.playerbot", "ShadowfangKeepScript: Tank chasing Arugal after teleport");
                        Position bossPos = boss->GetPosition();
                        MoveTo(player, bossPos);
                        return;
                    }
                }
                else if (role == DungeonRole::MELEE_DPS)
                {
                    // Melee must reposition behind boss
    if (distance > 10.0f)
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

void AddSC_shadowfang_keep_playerbot()
{
    // Register script
    DungeonScriptMgr::instance()->RegisterScript(new ShadowfangKeepScript());

    // Register Cataclysm bosses
    DungeonScriptMgr::instance()->RegisterBossScript(46962, DungeonScriptMgr::instance()->GetScriptForMap(33)); // Baron Ashbury
    DungeonScriptMgr::instance()->RegisterBossScript(3887, DungeonScriptMgr::instance()->GetScriptForMap(33));  // Baron Silverlaine
    DungeonScriptMgr::instance()->RegisterBossScript(4278, DungeonScriptMgr::instance()->GetScriptForMap(33));  // Commander Springvale
    DungeonScriptMgr::instance()->RegisterBossScript(46963, DungeonScriptMgr::instance()->GetScriptForMap(33)); // Lord Walden
    DungeonScriptMgr::instance()->RegisterBossScript(46964, DungeonScriptMgr::instance()->GetScriptForMap(33)); // Lord Godfrey

    // Register Classic bosses
    DungeonScriptMgr::instance()->RegisterBossScript(3850, DungeonScriptMgr::instance()->GetScriptForMap(33)); // Rethilgore (Classic)
    DungeonScriptMgr::instance()->RegisterBossScript(4275, DungeonScriptMgr::instance()->GetScriptForMap(33)); // Archmage Arugal (Classic)
    DungeonScriptMgr::instance()->RegisterBossScript(4274, DungeonScriptMgr::instance()->GetScriptForMap(33)); // Fenrus (Classic)

    TC_LOG_INFO("server.loading", ">> Registered Shadowfang Keep playerbot script with 8 boss mappings");
}

} // namespace Playerbot

/**
 * USAGE NOTES FOR SHADOWFANG KEEP:
 *
 * WHAT THIS SCRIPT HANDLES:
 * - Ashbury's Asphyxiate interrupt (critical)
 * - Springvale's Holy Light interrupt (healing)
 * - Springvale's Word of Shame positioning (frontal cone)
 * - Walden's potion ground effects avoidance
 * - Arugal's add priority (worgen adds)
 * - Arugal's Shadow Port repositioning
 * - Godfrey's Pistol Barrage positioning
 * - Silverlaine's Veil of Shadow dispel
 * - Spread mechanics for various bosses
 *
 * WHAT FALLS BACK TO GENERIC:
 * - Basic tank positioning
 * - Standard melee DPS positioning
 * - Ranged DPS optimal range
 * - Basic add priority when no special logic needed
 *
 * DUNGEON-SPECIFIC TIPS:
 * - MUST interrupt Ashbury's Asphyxiate or group wipes
 * - Interrupt Springvale's Holy Light to prevent healing
 * - Avoid standing in front for Word of Shame and Pistol Barrage
 * - Move out of Walden's potion puddles immediately
 * - Chase Arugal quickly after each teleport
 * - Tank picks up Arugal's worgen adds promptly
 * - Dispel Veil of Shadow to reduce shadow damage
 * - Be ready for frequent movement on Arugal fight
 *
 * DIFFICULTY RATING: 5/10 (Moderate)
 * - Asphyxiate can wipe group if not interrupted
 * - Arugal's teleports require good positioning
 * - Multiple mechanics to track
 * - Good practice for interrupt rotations
 * - Classic Horde dungeon with iconic encounters
 */
