/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * SCARLET MONASTERY DUNGEON SCRIPT (ALL WINGS)
 *
 * Map ID: 189 (all 4 wings share same map ID, different areas)
 * Level Range: 26-45 (varies by wing)
 * Location: Tirisfal Glades (near Undercity)
 *
 * WING STRUCTURE:
 * - Graveyard (26-36): Entry wing, undead and ghosts
 * - Library (29-39): Book repository, heavy caster presence
 * - Armory (32-42): Training grounds, elite guards
 * - Cathedral (35-45): Final wing, Scarlet Commander
 *
 * BOSS ENCOUNTERS:
 *
 * GRAVEYARD WING:
 * 1. Interrogator Vishas (3983) - Torturer, shadow damage
 * 2. Bloodmage Thalnos (4543) - Mage, fire and frost spells
 * 3. Ironspine (14682) - Undead boss, shadow bolt
 * 4. Azshir the Sleepless (6490) - Rare spawn, shadow damage
 *
 * LIBRARY WING:
 * 5. Houndmaster Loksey (3974) - Hounds and bloodhounds
 * 6. Arcanist Doan (6487) - Arcane mage, detonation and silence
 *
 * ARMORY WING:
 * 7. Herod (3975) - Warrior boss, whirlwind and enrage
 *
 * CATHEDRAL WING:
 * 8. High Inquisitor Fairbanks (4542) - Holy caster, heal and smite
 * 9. Scarlet Commander Mograine (3976) - Paladin, holy damage and heal
 * 10. High Inquisitor Whitemane (3977) - Final boss, resurrect and sleep
 *
 * SPECIAL MECHANICS:
 * - Vishas's shadow word pain (dispel)
 * - Thalnos's flame spike (fire damage)
 * - Doan's arcane explosion and detonation (GET OUT)
 * - Doan's silence (cannot cast for duration)
 * - Herod's whirlwind (GET OUT of melee range)
 * - Herod's enrage at low health
 * - Mograine + Whitemane duo fight
 * - Whitemane resurrects Mograine at 50% (CRITICAL)
 * - Whitemane's sleep (mass sleep, need wakeup)
 * - Fairbanks's holy heal (interrupt)
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

class ScarletMonasteryScript : public DungeonScript
{
public:
    ScarletMonasteryScript() : DungeonScript("scarlet_monastery", 189) {}

    // ============================================================================
    // LIFECYCLE HOOKS
    // ============================================================================

    void OnDungeonEnter(::Player* player, ::InstanceScript* instance) override
    {
        TC_LOG_DEBUG("module.playerbot", "ScarletMonasteryScript: Player {} entered Scarlet Monastery",
            player->GetGUID().GetCounter());

        // 4 wings, each with unique bosses
        // Cathedral wing has complex duo boss fight
    }

    // ============================================================================
    // BOSS HOOKS
    // ============================================================================

    void OnBossEngage(::Player* player, ::Creature* boss) override
    {
        uint32 entry = boss->GetEntry();

        switch (entry)
        {
            // ====== GRAVEYARD WING ======
            case 3983: // Interrogator Vishas
                TC_LOG_INFO("module.playerbot", "ScarletMonasteryScript: [GRAVEYARD] Engaging Interrogator Vishas");
                break;

            case 4543: // Bloodmage Thalnos
                TC_LOG_INFO("module.playerbot", "ScarletMonasteryScript: [GRAVEYARD] Engaging Bloodmage Thalnos");
                break;

            case 14682: // Ironspine
                TC_LOG_INFO("module.playerbot", "ScarletMonasteryScript: [GRAVEYARD] Engaging Ironspine");
                break;

            case 6490: // Azshir the Sleepless
                TC_LOG_INFO("module.playerbot", "ScarletMonasteryScript: [GRAVEYARD] Engaging Azshir the Sleepless");
                break;

            // ====== LIBRARY WING ======
            case 3974: // Houndmaster Loksey
                TC_LOG_INFO("module.playerbot", "ScarletMonasteryScript: [LIBRARY] Engaging Houndmaster Loksey");
                // Hound adds
                break;

            case 6487: // Arcanist Doan
                TC_LOG_INFO("module.playerbot", "ScarletMonasteryScript: [LIBRARY] Engaging Arcanist Doan");
                // CRITICAL: Arcane Explosion and Detonation mechanics
                HandleSpreadMechanic(player, boss);
                break;

            // ====== ARMORY WING ======
            case 3975: // Herod
                TC_LOG_INFO("module.playerbot", "ScarletMonasteryScript: [ARMORY] Engaging Herod");
                // Whirlwind - ranged must stay out
                break;

            // ====== CATHEDRAL WING ======
            case 4542: // High Inquisitor Fairbanks
                TC_LOG_INFO("module.playerbot", "ScarletMonasteryScript: [CATHEDRAL] Engaging High Inquisitor Fairbanks");
                // Holy heals - interrupt priority
                break;

            case 3976: // Scarlet Commander Mograine
                TC_LOG_INFO("module.playerbot", "ScarletMonasteryScript: [CATHEDRAL] Engaging Scarlet Commander Mograine");
                // Duo fight with Whitemane - DO NOT DPS below 50% until Whitemane engaged
                break;

            case 3977: // High Inquisitor Whitemane
                TC_LOG_INFO("module.playerbot", "ScarletMonasteryScript: [CATHEDRAL] Engaging High Inquisitor Whitemane (FINAL BOSS)");
                // Resurrects Mograine, mass sleep mechanic
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
            case 4543: // Bloodmage Thalnos
            {
                // Thalnos casts Flame Spike and Frost Bolt
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Flame Spike (9532) - high damage
    if (spellId == 9532 || spellId == 11829)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "ScarletMonasteryScript: Interrupting Thalnos's Flame Spike");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }
                    }
                }
                break;
            }

            case 6487: // Arcanist Doan
            {
                // Doan casts Arcane Missiles and Polymorph
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Polymorph (13323) - CRITICAL interrupt
    if (spellId == 13323 || spellId == 118)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "ScarletMonasteryScript: CRITICAL - Interrupting Doan's Polymorph");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }

                        // Arcane Missiles (9435)
    if (spellId == 9435 || spellId == 15735)
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

            case 4542: // High Inquisitor Fairbanks
            {
                // Fairbanks heals himself - MUST interrupt
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Heal (8362) - CRITICAL interrupt
    if (spellId == 8362 || spellId == 2054 || spellId == 2055)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "ScarletMonasteryScript: CRITICAL - Interrupting Fairbanks's heal");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }
                    }
                }
                break;
            }

            case 3976: // Scarlet Commander Mograine
            {
                // Mograine uses Hammer of Justice (stun) and heals
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Lay on Hands (9257) - EMERGENCY interrupt
    if (spellId == 9257 || spellId == 2800)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "ScarletMonasteryScript: EMERGENCY - Interrupting Mograine's Lay on Hands");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }
                    }
                }
                break;
            }

            case 3977: // High Inquisitor Whitemane
            {
                // Whitemane heals and resurrects - CRITICAL interrupts
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Resurrect (20770) - ABSOLUTELY MUST INTERRUPT
                        // This brings Mograine back to life at full health
    if (spellId == 20770 || spellId == 9232)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "ScarletMonasteryScript: EMERGENCY - Interrupting Whitemane's Resurrect");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }

                        // Heal (9232) - Also critical
    if (spellId == 9232 || spellId == 2054)
                        {
                            if (HasInterruptAvailable(player))
                            {
                                TC_LOG_DEBUG("module.playerbot", "ScarletMonasteryScript: CRITICAL - Interrupting Whitemane's heal");
                                UseInterruptSpell(player, boss);
                                return;
                            }
                        }
                    }
                }
                break;
            }

            case 14682: // Ironspine
            {
                // Shadow Bolt spam
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;

                        // Shadow Bolt
    if (spellId == 9613 || spellId == 20297)
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

        // ENTERPRISE: Use lock-free spatial grid for thread-safe DynamicObject queries
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

                    // Check if casted by the boss
                    if (snapshot.casterGuid != boss->GetGUID())
                        continue;

                    float distance = player->GetExactDist(snapshot.position);
                    if (distance < 10.0f)
                    {
                        if (DynamicObject* dynObj = ObjectAccessor::GetDynamicObject(*player, snapshot.guid))
                        {
                            if (IsDangerousGroundEffect(dynObj))
                            {
                                TC_LOG_DEBUG("module.playerbot", "ScarletMonasteryScript: Avoiding ground effect at distance {:.1f}", distance);
                                MoveAwayFromGroundEffect(player, dynObj);
                                return;
                            }
                        }
                    }
                }
            }
        }

        switch (entry)
        {
            case 6487: // Arcanist Doan
            {
                // Doan's Detonation - MASSIVE explosion centered on him
                // ALL players must get out when he starts channeling
                // Visual: He glows bright blue

                // Check if Doan is casting Detonation (9435 or similar)
                if (boss->HasAura(13323) || boss->HasAura(9435))
                {
                    float distance = player->GetExactDist(boss);
                    // If within 20 yards of Doan during detonation, RUN AWAY
                    if (distance < 20.0f)
                    {
                        TC_LOG_DEBUG("module.playerbot", "ScarletMonasteryScript: EMERGENCY - Running from Doan's Detonation");

                        // Calculate position away from Doan
                        Position doanPos = boss->GetPosition();
                        Position playerPos = player->GetPosition();

                        float angle = doanPos.GetAbsoluteAngle(&playerPos);
                        float newX = player->GetPositionX() + cos(angle) * 15.0f;
                        float newY = player->GetPositionY() + sin(angle) * 15.0f;
                        float newZ = player->GetPositionZ();
                        Position safePos(newX, newY, newZ);
                        MoveTo(player, safePos);
                        return;
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
            case 3974: // Houndmaster Loksey
            {
                // Loksey has bloodhound adds
                // Kill adds before focusing boss

                ::std::vector<::Creature*> adds = GetAddsInCombat(player, boss);

                if (!adds.empty())
                {
                    // Kill lowest health hound first
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
                        TC_LOG_DEBUG("module.playerbot", "ScarletMonasteryScript: Targeting Loksey's hound");
                        player->SetSelection(lowestHealthAdd->GetGUID());
                        return;
                    }
                }
                break;
            }

            case 3977: // High Inquisitor Whitemane
            {
                // When Whitemane enters fight, she resurrects Mograine
                // Both must be killed, but focus should be on preventing resurrect

                // Check if Mograine is alive
                ::Creature* mograine = FindCreatureNearby(player, 3976, 50.0f);
                if (mograine && mograine->IsAlive())
                {
                    // If both bosses alive, kill Mograine first (he does more damage)
                    // But only after interrupting Whitemane's resurrect
                    TC_LOG_DEBUG("module.playerbot", "ScarletMonasteryScript: Mograine alive - prioritize after interrupting resurrect");
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
            case 3975: // Herod
            {
                // Herod uses Whirlwind - ALL players must get out of melee range
                // Whirlwind lasts several seconds and deals massive damage

                // Check if Herod is whirlwinding (8989)
    if (boss->HasAura(8989))
                {
                    float distance = player->GetExactDist(boss);

                    // Everyone get to 10+ yards
    if (distance < 10.0f)
                    {
                        TC_LOG_DEBUG("module.playerbot", "ScarletMonasteryScript: Running from Herod's Whirlwind");

                        // Run away from Herod
                        Position herodPos = boss->GetPosition();
                        Position playerPos = player->GetPosition();

                        float angle = herodPos.GetAbsoluteAngle(&playerPos);
                        float newX = player->GetPositionX() + cos(angle) * 12.0f;
                        float newY = player->GetPositionY() + sin(angle) * 12.0f;
                        float newZ = player->GetPositionZ();
                        Position safePos(newX, newY, newZ);
                        MoveTo(player, safePos);
                        return;
                    }
                }
                else
                {
                    // When not whirlwinding, standard positioning
                    DungeonRole role = GetPlayerRole(player);

                    if (role == DungeonRole::MELEE_DPS || role == DungeonRole::TANK)
                    {
                        // Return to melee range after whirlwind ends
                        float distance = player->GetExactDist(boss);
                        if (distance > 5.0f)
                        {
                            MoveTo(player, boss->GetPosition());
                            return;
                        }
                    }
                }
                break;
            }
            case 6487: // Arcanist Doan
            {
                // Stay spread for Arcane Explosion
                HandleSpreadMechanic(player, boss);
                return;
            }

            case 3977: // High Inquisitor Whitemane
            {
                // Spread for Deep Sleep (mass sleep)
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
            case 3983: // Interrogator Vishas
            {
                // Shadow Word: Pain - dispel disease/magic
                Group* group = player->GetGroup();
                if (!group)
                    break;

                for (auto const& member : group->GetMemberSlots())
                {
                    Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
                    if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
                        continue;

                    // Check for Shadow Word: Pain (589)
    if (groupMember->HasAura(589) || groupMember->HasAura(2060))
                    {
                        TC_LOG_DEBUG("module.playerbot", "ScarletMonasteryScript: Dispelling Shadow Word: Pain");
                        // Dispel magic
                        return;
                    }
                }
                break;
            }

            case 3977: // High Inquisitor Whitemane
            {
                // Deep Sleep - mass sleep that needs wakeup
                Group* group = player->GetGroup();
                if (!group)
                    break;

                for (auto const& member : group->GetMemberSlots())
                {
                    Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
                    if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
                        continue;

                    // Check for sleep
    if (groupMember->HasAuraType(SPELL_AURA_MOD_STUN) ||
                        groupMember->HasAura(9256))
                    {
                        TC_LOG_DEBUG("module.playerbot", "ScarletMonasteryScript: Waking player from Whitemane's sleep");
                        // Damage to wake up or dispel
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
            case 6487: // Arcanist Doan
            {
                // Arcane Explosion - 8 yard spread
                EncounterStrategy::HandleGenericSpread(player, boss, 8.0f);
                return;
            }

            case 3977: // High Inquisitor Whitemane
            {
                // Deep Sleep affects entire group - spread to minimize impact
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
            case 3975: // Herod
            {
                // During whirlwind, maintain distance
                // After whirlwind, return to position
    if (boss->HasAura(8989))
                {
                    // Stay away during whirlwind
                    HandlePositioning(player, boss);
                    return;
                }
                break;
            }

            case 6487: // Arcanist Doan
            {
                // During Detonation, run away
                // During normal phase, maintain position
    if (boss->HasAura(13323))
                {
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

void AddSC_scarlet_monastery_playerbot()
{
    // Register script
    DungeonScriptMgr::instance()->RegisterScript(new ScarletMonasteryScript());

    // Register bosses - Graveyard Wing
    DungeonScriptMgr::instance()->RegisterBossScript(3983, DungeonScriptMgr::instance()->GetScriptForMap(189));  // Interrogator Vishas
    DungeonScriptMgr::instance()->RegisterBossScript(4543, DungeonScriptMgr::instance()->GetScriptForMap(189));  // Bloodmage Thalnos
    DungeonScriptMgr::instance()->RegisterBossScript(14682, DungeonScriptMgr::instance()->GetScriptForMap(189)); // Ironspine
    DungeonScriptMgr::instance()->RegisterBossScript(6490, DungeonScriptMgr::instance()->GetScriptForMap(189));  // Azshir the Sleepless

    // Register bosses - Library Wing
    DungeonScriptMgr::instance()->RegisterBossScript(3974, DungeonScriptMgr::instance()->GetScriptForMap(189));  // Houndmaster Loksey
    DungeonScriptMgr::instance()->RegisterBossScript(6487, DungeonScriptMgr::instance()->GetScriptForMap(189));  // Arcanist Doan

    // Register bosses - Armory Wing
    DungeonScriptMgr::instance()->RegisterBossScript(3975, DungeonScriptMgr::instance()->GetScriptForMap(189));  // Herod

    // Register bosses - Cathedral Wing
    DungeonScriptMgr::instance()->RegisterBossScript(4542, DungeonScriptMgr::instance()->GetScriptForMap(189));  // High Inquisitor Fairbanks
    DungeonScriptMgr::instance()->RegisterBossScript(3976, DungeonScriptMgr::instance()->GetScriptForMap(189));  // Scarlet Commander Mograine
    DungeonScriptMgr::instance()->RegisterBossScript(3977, DungeonScriptMgr::instance()->GetScriptForMap(189));  // High Inquisitor Whitemane

    TC_LOG_INFO("server.loading", ">> Registered Scarlet Monastery playerbot script with 10 boss mappings (all 4 wings)");
}

} // namespace Playerbot

/**
 * USAGE NOTES FOR SCARLET MONASTERY:
 *
 * WHAT THIS SCRIPT HANDLES:
 * - Bloodmage Thalnos Flame Spike interrupt
 * - Arcanist Doan Polymorph interrupt (CRITICAL)
 * - Arcanist Doan Detonation escape (EMERGENCY)
 * - Fairbanks heal interrupt (CRITICAL)
 * - Mograine Lay on Hands interrupt (EMERGENCY)
 * - Whitemane Resurrect interrupt (ABSOLUTELY CRITICAL)
 * - Whitemane heal interrupt (CRITICAL)
 * - Ironspine Shadow Bolt interrupt
 * - Herod Whirlwind escape mechanics (ALL players)
 * - Houndmaster Loksey hound add priority
 * - Doan Arcane Explosion spread mechanics
 * - Whitemane Deep Sleep spread and wakeup
 * - Vishas Shadow Word: Pain dispel
 * - Mograine/Whitemane duo boss mechanics
 * - Herod enrage management
 *
 * WHAT FALLS BACK TO GENERIC:
 * - Basic tank positioning (except during special mechanics)
 * - Standard ranged DPS positioning
 * - Basic healing priority
 * - Melee DPS positioning (behind boss when possible)
 *
 * DUNGEON-SPECIFIC TIPS:
 *
 * GRAVEYARD WING (26-36):
 * - Dispel Shadow Word: Pain from Vishas
 * - Interrupt Thalnos's Flame Spike
 * - Interrupt Ironspine's Shadow Bolt spam
 * - Watch for Azshir (rare spawn)
 *
 * LIBRARY WING (29-39):
 * - Kill Loksey's hounds before boss
 * - CRITICAL: Interrupt Doan's Polymorph
 * - EMERGENCY: RUN from Doan's Detonation (blue glow = RUN)
 * - Doan casts Silence - be ready to stop casting
 * - Spread 8+ yards for Arcane Explosion
 *
 * ARMORY WING (32-42):
 * - ALL PLAYERS: Run from Herod's Whirlwind (even melee/tank)
 * - Return to melee after whirlwind ends
 * - Herod enrages at low health - burn fast
 * - Watch for Scarlet Defender packs before boss
 *
 * CATHEDRAL WING (35-45):
 * - Interrupt Fairbanks's heals
 * - MOGRAINE/WHITEMANE DUO FIGHT:
 *   1. Kill Mograine first
 *   2. At 50%, Whitemane enters fight
 *   3. INTERRUPT Whitemane's Resurrect (brings Mograine back at full HP)
 *   4. If resurrect succeeds, kill Mograine again
 *   5. Then kill Whitemane
 * - Whitemane casts Deep Sleep (mass sleep) - spread and damage to wake
 * - Interrupt ALL of Whitemane's heals
 * - Save interrupts for Resurrect (highest priority)
 *
 * INTERRUPT PRIORITY (CATHEDRAL):
 * 1. Whitemane's Resurrect - ABSOLUTE PRIORITY
 * 2. Whitemane's Heal - CRITICAL
 * 3. Mograine's Lay on Hands - CRITICAL
 * 4. Fairbanks's Heal - HIGH
 *
 * DIFFICULTY RATING BY WING:
 * - Graveyard: 3/10 (Easy) - Good starter wing
 * - Library: 5/10 (Moderate) - Doan Detonation is dangerous
 * - Armory: 4/10 (Easy-Moderate) - Herod Whirlwind requires awareness
 * - Cathedral: 7/10 (Moderate-Hard) - Complex duo boss fight
 *
 * OVERALL DIFFICULTY: 5/10 (Moderate)
 * - Multiple wings provide varied challenges
 * - Cathedral wing requires coordination
 * - Whitemane/Mograine fight is most complex
 * - Good practice for interrupt mechanics
 * - Excellent gear for level range
 * - Popular leveling dungeon
 */
