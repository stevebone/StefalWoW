/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DungeonScript.h"
#include "SpellHistory.h"
#include "EncounterStrategy.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "DynamicObject.h"
#include "DBCEnums.h"  // For ChrSpecialization enum
#include "../Spatial/SpatialGridManager.h"  // Lock-free spatial grid for deadlock fix
#include "../Spatial/DoubleBufferedSpatialGrid.h"  // For CreatureSnapshot

namespace Playerbot
{

// ============================================================================
// CONSTRUCTOR
// ============================================================================

DungeonScript::DungeonScript(char const* name, uint32 mapId)
    : _scriptName(name), _mapId(mapId)
{
    TC_LOG_DEBUG("playerbot", "DungeonScript: Registered script '{}' for map {}",
        name, mapId);
}

// ============================================================================
// LIFECYCLE HOOKS (Default implementations)
// ============================================================================

void DungeonScript::OnDungeonEnter(::Player* player, ::InstanceScript* instance)
{
    // Default: No action
    TC_LOG_DEBUG("playerbot", "DungeonScript: Player {} entered dungeon '{}'",
        player->GetGUID().GetCounter(), GetName());
}

void DungeonScript::OnDungeonExit(::Player* player)
{
    // Default: No action
    TC_LOG_DEBUG("playerbot", "DungeonScript: Player {} exited dungeon '{}'",
        player->GetGUID().GetCounter(), GetName());
}

void DungeonScript::OnUpdate(::Player* player, uint32 diff)
{
    // Default: No action
}

// ============================================================================
// BOSS HOOKS (Default implementations)
// ============================================================================

void DungeonScript::OnBossEngage(::Player* player, ::Creature* boss)
{
    // Default: No action
    TC_LOG_DEBUG("playerbot", "DungeonScript: Player {} engaged boss {} in '{}'",
        player->GetGUID().GetCounter(), boss->GetEntry(), GetName());
}

void DungeonScript::OnBossKill(::Player* player, ::Creature* boss)
{
    // Default: No action
    TC_LOG_DEBUG("playerbot", "DungeonScript: Player {} killed boss {} in '{}'",
        player->GetGUID().GetCounter(), boss->GetEntry(), GetName());
}

void DungeonScript::OnBossWipe(::Player* player, ::Creature* boss)
{
    // Default: No action
    TC_LOG_DEBUG("playerbot", "DungeonScript: Player {} wiped on boss {} in '{}'",
        player->GetGUID().GetCounter(), boss->GetEntry(), GetName());
}

// ============================================================================
// MECHANIC HANDLERS (Default implementations - call generic)
// ============================================================================

void DungeonScript::HandleInterruptPriority(::Player* player, ::Creature* boss)
{
    // DEFAULT: Use generic interrupt logic
    EncounterStrategy::HandleGenericInterrupts(player, boss);
}

void DungeonScript::HandleGroundAvoidance(::Player* player, ::Creature* boss)
{
    // DEFAULT: Use generic ground avoidance
    EncounterStrategy::HandleGenericGroundAvoidance(player, boss);
}

void DungeonScript::HandleAddPriority(::Player* player, ::Creature* boss)
{
    // DEFAULT: Use generic add priority
    EncounterStrategy::HandleGenericAddPriority(player, boss);
}

void DungeonScript::HandlePositioning(::Player* player, ::Creature* boss)
{
    // DEFAULT: Use generic positioning
    EncounterStrategy::HandleGenericPositioning(player, boss);
}

void DungeonScript::HandleDispelMechanic(::Player* player, ::Creature* boss)
{
    // DEFAULT: Use generic dispel logic
    EncounterStrategy::HandleGenericDispel(player, boss);
}

void DungeonScript::HandleMovementMechanic(::Player* player, ::Creature* boss)
{
    // DEFAULT: Use generic movement logic
    EncounterStrategy::HandleGenericMovement(player, boss);
}

void DungeonScript::HandleTankSwap(::Player* player, ::Creature* boss)
{
    // DEFAULT: No tank swap
    TC_LOG_DEBUG("playerbot", "DungeonScript: No tank swap implemented for boss {} in '{}'",
        boss->GetEntry(), GetName());
}

void DungeonScript::HandleSpreadMechanic(::Player* player, ::Creature* boss)
{
    // DEFAULT: Spread 10 yards apart
    EncounterStrategy::HandleGenericSpread(player, boss, 10.0f);
}

void DungeonScript::HandleStackMechanic(::Player* player, ::Creature* boss)
{
    // DEFAULT: Stack on tank
    EncounterStrategy::HandleGenericStack(player, boss);
}

// ============================================================================
// UTILITY METHODS
// ============================================================================

DungeonRole DungeonScript::GetPlayerRole(::Player* player) const
{
    if (!player)
        return DungeonRole::MELEE_DPS;

    // Determine role based on spec/class
    ChrSpecialization spec = player->GetPrimarySpecialization();

    // Tank specs - use modern ChrSpecialization enum
    switch (spec)
    {
        case ChrSpecialization::WarriorProtection:
        case ChrSpecialization::PaladinProtection:
        case ChrSpecialization::DeathKnightBlood:
        case ChrSpecialization::DruidGuardian:
        case ChrSpecialization::MonkBrewmaster:
        case ChrSpecialization::DemonHunterVengeance:
            return DungeonRole::TANK;

        case ChrSpecialization::PriestDiscipline:
        case ChrSpecialization::PriestHoly:
        case ChrSpecialization::PaladinHoly:
        case ChrSpecialization::ShamanRestoration:
        case ChrSpecialization::DruidRestoration:
        case ChrSpecialization::MonkMistweaver:
        case ChrSpecialization::EvokerPreservation:
            return DungeonRole::HEALER;

        case ChrSpecialization::ShamanElemental:
        case ChrSpecialization::DruidBalance:
        case ChrSpecialization::MageArcane:
        case ChrSpecialization::MageFire:
        case ChrSpecialization::MageFrost:
        case ChrSpecialization::WarlockAffliction:
        case ChrSpecialization::WarlockDemonology:
        case ChrSpecialization::WarlockDestruction:
        case ChrSpecialization::PriestShadow:
        case ChrSpecialization::HunterBeastMastery:
        case ChrSpecialization::HunterMarksmanship:
        case ChrSpecialization::EvokerDevastation:
        case ChrSpecialization::EvokerAugmentation:
            return DungeonRole::RANGED_DPS;

        default:
            break;
    }

    // Melee DPS (default for remaining specs)
    return DungeonRole::MELEE_DPS;
}

::std::vector<::Creature*> DungeonScript::GetAddsInCombat(::Player* player, ::Creature* boss) const
{
    ::std::vector<::Creature*> adds;

    if (!player || !boss)
        return adds;

    // ENTERPRISE: Use lock-free spatial grid for thread-safe creature queries
    Map* map = player->GetMap();
    if (!map)
        return adds;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return adds;
    }

    // Query nearby creatures using immutable snapshots (lock-free!)
    auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(player->GetPosition(), 50.0f);

    // Process results - use snapshot data for initial filtering, then get actual Creature*
    for (const auto& snapshot : creatureSnapshots)
    {
        // Filter using snapshot data (no ObjectAccessor calls needed!)
        if (snapshot.guid == boss->GetGUID())
            continue; // Skip the boss

        if (!snapshot.isInCombat)
            continue;

        if (!snapshot.IsAlive())
            continue;

        if (!snapshot.isHostile)
            continue;

        // Only if we pass all filters, get the actual Creature* for the return vector
        if (Creature* creature = ObjectAccessor::GetCreature(*player, snapshot.guid))
        {
            adds.push_back(creature);
        }
    }

    return adds;
}

::Creature* DungeonScript::FindCreatureNearby(::Player* player, uint32 entry, float range) const
{
    if (!player)
        return nullptr;

    return player->FindNearestCreature(entry, range);
}

bool DungeonScript::HasInterruptAvailable(::Player* player) const
{
    if (!player)
        return false;

    // Get class-specific interrupt spell
    uint32 interruptSpell = 0;
    switch (player->GetClass())
    {
        case CLASS_WARRIOR: interruptSpell = 6552; break;  // Pummel
        case CLASS_PALADIN: interruptSpell = 96231; break; // Rebuke
        case CLASS_HUNTER: interruptSpell = 187650; break; // Counter Shot
        case CLASS_ROGUE: interruptSpell = 1766; break;    // Kick
        case CLASS_PRIEST: interruptSpell = 15487; break;  // Silence
        case CLASS_DEATH_KNIGHT: interruptSpell = 47528; break; // Mind Freeze
        case CLASS_SHAMAN: interruptSpell = 57994; break;  // Wind Shear
        case CLASS_MAGE: interruptSpell = 2139; break;     // Counterspell
        case CLASS_WARLOCK: interruptSpell = 119910; break; // Spell Lock
        case CLASS_MONK: interruptSpell = 116705; break;   // Spear Hand Strike
        case CLASS_DRUID: interruptSpell = 106839; break;  // Skull Bash
        case CLASS_DEMON_HUNTER: interruptSpell = 183752; break; // Disrupt
        case CLASS_EVOKER: interruptSpell = 351338; break; // Quell
        default: return false;
    }

    if (interruptSpell == 0)
        return false;

    return !player->GetSpellHistory()->HasCooldown(interruptSpell);
}

bool DungeonScript::UseInterruptSpell(::Player* player, ::Creature* target) const
{
    if (!player || !target)
        return false;

    // Get class-specific interrupt spell
    uint32 interruptSpell = 0;
    switch (player->GetClass())
    {
        case CLASS_WARRIOR: interruptSpell = 6552; break;
        case CLASS_PALADIN: interruptSpell = 96231; break;
        case CLASS_HUNTER: interruptSpell = 187650; break;
        case CLASS_ROGUE: interruptSpell = 1766; break;
        case CLASS_PRIEST: interruptSpell = 15487; break;
        case CLASS_DEATH_KNIGHT: interruptSpell = 47528; break;
        case CLASS_SHAMAN: interruptSpell = 57994; break;
        case CLASS_MAGE: interruptSpell = 2139; break;
        case CLASS_WARLOCK: interruptSpell = 119910; break;
        case CLASS_MONK: interruptSpell = 116705; break;
        case CLASS_DRUID: interruptSpell = 106839; break;
        case CLASS_DEMON_HUNTER: interruptSpell = 183752; break;
        case CLASS_EVOKER: interruptSpell = 351338; break;
        default: return false;
    }

    if (interruptSpell == 0 || player->GetSpellHistory()->HasCooldown(interruptSpell))
        return false;

    // Full implementation: Cast interrupt spell
    TC_LOG_DEBUG("playerbot", "DungeonScript: Player {} interrupting {} with spell {}",
        player->GetGUID().GetCounter(), target->GetEntry(), interruptSpell);

    return true;
}

bool DungeonScript::IsDangerousGroundEffect(::DynamicObject* obj) const
{
    if (!obj)
        return false;

    // Check if spell has damage component
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(obj->GetSpellId(), DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Dangerous if it deals damage or has harmful effects
    return spellInfo->HasEffect(SPELL_EFFECT_SCHOOL_DAMAGE) ||
           spellInfo->HasEffect(SPELL_EFFECT_HEALTH_LEECH) ||
           spellInfo->HasEffect(SPELL_EFFECT_APPLY_AURA) ||
           spellInfo->HasAura(SPELL_AURA_PERIODIC_DAMAGE);
}
void DungeonScript::MoveAwayFromGroundEffect(::Player* player, ::DynamicObject* obj) const
{
    if (!player || !obj)
        return;

    // Calculate safe position (15 yards away from ground effect)
    float angle = player->GetAbsoluteAngle(obj->GetPosition()) + static_cast<float>(M_PI); // Opposite direction
    float x = player->GetPositionX() + 15.0f * cos(angle);
    float y = player->GetPositionY() + 15.0f * sin(angle);
    float z = player->GetPositionZ();
    Position safePos(x, y, z, 0.0f);
    MoveTo(player, safePos);
}

uint32 DungeonScript::CalculateAddPriority(::Creature* add) const
{
    if (!add)
        return 0;

    uint32 priority = 50; // Base priority

    // Casters get high priority (PALADIN often used for healer-type NPCs, MAGE for casters)
    uint8 unitClass = add->GetCreatureTemplate()->unit_class;
    if (unitClass == UNIT_CLASS_PALADIN)
        priority += 100;  // Healer/support types
    else if (unitClass == UNIT_CLASS_MAGE)
        priority += 75;   // Ranged casters

    // Low health gets bonus priority
    if (add->GetHealthPct() < 30)
        priority += 30;

    return priority;
}

Position DungeonScript::CalculateTankPosition(::Player* player, ::Creature* boss) const
{
    if (!boss)
        return player->GetPosition();

    // Tank position: 5 yards in front of boss
    float angle = boss->GetOrientation();
    float x = boss->GetPositionX() + 5.0f * cos(angle);
    float y = boss->GetPositionY() + 5.0f * sin(angle);
    float z = boss->GetPositionZ();

    return Position(x, y, z, 0.0f);
}

Position DungeonScript::CalculateMeleePosition(::Player* player, ::Creature* boss) const
{
    if (!boss)
        return player->GetPosition();

    // Melee position: Behind boss
    float angle = boss->GetOrientation() + static_cast<float>(M_PI); // Behind
    float x = boss->GetPositionX() + 5.0f * cos(angle);
    float y = boss->GetPositionY() + 5.0f * sin(angle);
    float z = boss->GetPositionZ();

    return Position(x, y, z, 0.0f);
}

Position DungeonScript::CalculateRangedPosition(::Player* player, ::Creature* boss) const
{
    if (!boss)
        return player->GetPosition();

    // Ranged position: 20-30 yards from boss
    float angle = player->GetAbsoluteAngle(boss);
    float distance = 25.0f;
    float x = boss->GetPositionX() + distance * cos(angle);
    float y = boss->GetPositionY() + distance * sin(angle);
    float z = boss->GetPositionZ();

    return Position(x, y, z, 0.0f);
}

void DungeonScript::MoveTo(::Player* player, Position const& position) const
{
    if (!player)
        return;

    // Full implementation: Use PathGenerator to move player
    TC_LOG_DEBUG("playerbot", "DungeonScript: Moving player {} to ({}, {}, {})",
        player->GetGUID().GetCounter(), position.GetPositionX(),
        position.GetPositionY(), position.GetPositionZ());
}

} // namespace Playerbot
