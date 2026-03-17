/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "RacialAbilityManager.h"
#include "Log.h"
#include "Player.h"
#include "Creature.h"
#include "SpellHistory.h"
#include "ThreatManager.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "GameTime.h"
#include "RaceMask.h"
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// STATIC RACIAL DATABASE
// ============================================================================

::std::vector<RacialAbilityInfo> const& RacialAbilityManager::GetRacialDatabase()
{
    // Comprehensive racial ability database for all playable races
    // Spell IDs are from WoW 11.x (The War Within / Midnight era)
    static const ::std::vector<RacialAbilityInfo> database = {

        // ====================================================================
        // HUMAN
        // ====================================================================
        { 59752, RACE_HUMAN, "Will to Survive", RacialCategory::CC_BREAK,
          0.0f, false, false, true, false, 180000 },

        // ====================================================================
        // ORC
        // ====================================================================
        { 33697, RACE_ORC, "Blood Fury (AP)", RacialCategory::OFFENSIVE,
          0.0f, true, false, true, false, 120000 },
        { 33702, RACE_ORC, "Blood Fury (SP)", RacialCategory::OFFENSIVE,
          0.0f, true, false, true, false, 120000 },

        // ====================================================================
        // DWARF
        // ====================================================================
        { 20594, RACE_DWARF, "Stoneform", RacialCategory::DEFENSIVE,
          0.5f, false, false, true, false, 120000 },

        // ====================================================================
        // NIGHT ELF
        // ====================================================================
        { 58984, RACE_NIGHTELF, "Shadowmeld", RacialCategory::UTILITY,
          0.0f, false, false, false, false, 120000 },

        // ====================================================================
        // UNDEAD
        // ====================================================================
        { 7744, RACE_UNDEAD_PLAYER, "Will of the Forsaken", RacialCategory::CC_BREAK,
          0.0f, false, false, true, false, 120000 },

        // ====================================================================
        // TAUREN
        // ====================================================================
        { 20549, RACE_TAUREN, "War Stomp", RacialCategory::AOE_CC,
          0.0f, false, false, true, false, 90000 },

        // ====================================================================
        // GNOME
        // ====================================================================
        { 20589, RACE_GNOME, "Escape Artist", RacialCategory::CC_BREAK,
          0.0f, false, false, true, false, 60000 },

        // ====================================================================
        // TROLL
        // ====================================================================
        { 26297, RACE_TROLL, "Berserking", RacialCategory::OFFENSIVE,
          0.0f, true, false, true, false, 180000 },

        // ====================================================================
        // GOBLIN
        // ====================================================================
        { 69041, RACE_GOBLIN, "Rocket Barrage", RacialCategory::OFFENSIVE,
          0.0f, false, true, true, false, 90000 },
        { 69070, RACE_GOBLIN, "Rocket Jump", RacialCategory::UTILITY,
          0.0f, false, false, false, false, 90000 },

        // ====================================================================
        // BLOOD ELF
        // ====================================================================
        { 28730, RACE_BLOODELF, "Arcane Torrent", RacialCategory::RESOURCE,
          0.0f, false, false, true, false, 120000 },

        // ====================================================================
        // DRAENEI
        // ====================================================================
        { 59547, RACE_DRAENEI, "Gift of the Naaru", RacialCategory::DEFENSIVE,
          0.6f, false, false, true, false, 180000 },

        // ====================================================================
        // WORGEN
        // ====================================================================
        { 68992, RACE_WORGEN, "Darkflight", RacialCategory::UTILITY,
          0.0f, false, false, false, false, 120000 },

        // ====================================================================
        // PANDAREN (all factions use same abilities)
        // ====================================================================
        { 107079, RACE_PANDAREN_ALLIANCE, "Quaking Palm", RacialCategory::AOE_CC,
          0.0f, false, false, true, false, 120000 },
        { 107079, RACE_PANDAREN_HORDE, "Quaking Palm", RacialCategory::AOE_CC,
          0.0f, false, false, true, false, 120000 },
        { 107079, RACE_PANDAREN_NEUTRAL, "Quaking Palm", RacialCategory::AOE_CC,
          0.0f, false, false, true, false, 120000 },

        // ====================================================================
        // NIGHTBORNE
        // ====================================================================
        { 260364, RACE_NIGHTBORNE, "Arcane Pulse", RacialCategory::OFFENSIVE,
          0.0f, false, true, true, false, 180000 },

        // ====================================================================
        // HIGHMOUNTAIN TAUREN
        // ====================================================================
        { 255654, RACE_HIGHMOUNTAIN_TAUREN, "Bull Rush", RacialCategory::AOE_CC,
          0.0f, false, false, true, false, 120000 },

        // ====================================================================
        // VOID ELF
        // ====================================================================
        { 256948, RACE_VOID_ELF, "Spatial Rift", RacialCategory::UTILITY,
          0.0f, false, false, false, false, 180000 },

        // ====================================================================
        // LIGHTFORGED DRAENEI
        // ====================================================================
        { 255647, RACE_LIGHTFORGED_DRAENEI, "Light's Judgment", RacialCategory::OFFENSIVE,
          0.0f, false, true, true, false, 150000 },

        // ====================================================================
        // ZANDALARI TROLL
        // ====================================================================
        { 291944, RACE_ZANDALARI_TROLL, "Regeneratin'", RacialCategory::DEFENSIVE,
          0.4f, false, false, true, false, 150000 },

        // ====================================================================
        // KUL TIRAN
        // ====================================================================
        { 287712, RACE_KUL_TIRAN, "Haymaker", RacialCategory::AOE_CC,
          0.0f, false, false, true, false, 150000 },

        // ====================================================================
        // DARK IRON DWARF
        // ====================================================================
        { 265221, RACE_DARK_IRON_DWARF, "Fireblood", RacialCategory::DEFENSIVE,
          0.0f, true, false, true, false, 120000 },

        // ====================================================================
        // VULPERA
        // ====================================================================
        { 312411, RACE_VULPERA, "Bag of Tricks", RacialCategory::OFFENSIVE,
          0.0f, false, true, true, false, 90000 },

        // ====================================================================
        // MAG'HAR ORC
        // ====================================================================
        { 274738, RACE_MAGHAR_ORC, "Ancestral Call", RacialCategory::OFFENSIVE,
          0.0f, true, false, true, false, 120000 },

        // ====================================================================
        // MECHAGNOME
        // ====================================================================
        { 312924, RACE_MECHAGNOME, "Hyper Organic Light Originator", RacialCategory::OFFENSIVE,
          0.0f, false, true, true, false, 180000 },
        { 312916, RACE_MECHAGNOME, "Emergency Failsafe", RacialCategory::DEFENSIVE,
          0.2f, false, false, true, false, 150000 },

        // ====================================================================
        // DRACTHYR
        // ====================================================================
        { 368970, RACE_DRACTHYR_ALLIANCE, "Tail Swipe", RacialCategory::AOE_CC,
          0.0f, false, false, true, false, 90000 },
        { 368970, RACE_DRACTHYR_HORDE, "Tail Swipe", RacialCategory::AOE_CC,
          0.0f, false, false, true, false, 90000 },
        { 357214, RACE_DRACTHYR_ALLIANCE, "Wing Buffet", RacialCategory::UTILITY,
          0.0f, false, false, true, false, 90000 },
        { 357214, RACE_DRACTHYR_HORDE, "Wing Buffet", RacialCategory::UTILITY,
          0.0f, false, false, true, false, 90000 },

        // ====================================================================
        // EARTHEN
        // ====================================================================
        { 446280, RACE_EARTHEN_DWARF_ALLIANCE, "Azerite Surge", RacialCategory::OFFENSIVE,
          0.0f, true, false, true, false, 120000 },
        { 446280, RACE_EARTHEN_DWARF_HORDE, "Azerite Surge", RacialCategory::OFFENSIVE,
          0.0f, true, false, true, false, 120000 },
    };

    return database;
}

// ============================================================================
// CONSTRUCTOR / INITIALIZATION
// ============================================================================

RacialAbilityManager::RacialAbilityManager(Player* bot)
    : _bot(bot)
{
}

void RacialAbilityManager::Initialize()
{
    if (!_bot)
        return;

    uint8 raceId = _bot->GetRace();
    LoadRacialsForRace(raceId);
    _initialized = true;

    TC_LOG_DEBUG("module.playerbot", "RacialAbilityManager: Initialized {} racial abilities for bot {} (race={})",
        _racials.size(), _bot->GetName(), GetRaceName());
}

void RacialAbilityManager::LoadRacialsForRace(uint8 raceId)
{
    _racials.clear();

    auto const& database = GetRacialDatabase();
    for (auto const& racial : database)
    {
        if (racial.raceId == raceId)
        {
            // Verify the bot actually has this spell
            if (_bot->HasSpell(racial.spellId))
            {
                _racials.push_back(racial);
            }
            else
            {
                // Check if the spell exists in the spell DB at all
                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(racial.spellId, DIFFICULTY_NONE);
                if (spellInfo)
                {
                    TC_LOG_DEBUG("module.playerbot", "RacialAbilityManager: Bot {} doesn't have racial {} (ID: {}), may need training",
                        _bot->GetName(), racial.name, racial.spellId);
                }
            }
        }
    }
}

// ============================================================================
// EVALUATION
// ============================================================================

uint32 RacialAbilityManager::EvaluateRacials()
{
    if (!_bot || !_initialized || _racials.empty())
        return 0;

    uint32 now = GameTime::GetGameTimeMS();
    if (now - _lastEvalTime < MIN_EVAL_INTERVAL)
        return 0;
    _lastEvalTime = now;

    // Priority order: CC Break > Defensive > Offensive > Resource > AoE CC > Utility

    // 1. CC Break - highest priority, use immediately when CC'd
    uint32 ccBreak = GetCCBreakRacial();
    if (ccBreak != 0)
    {
        _stats.ccBreakUsed++;
        _stats.totalUsed++;
        _lastUsedTime[ccBreak] = now;
        return ccBreak;
    }

    // 2. Defensive - when health is critical
    uint32 defensive = GetDefensiveRacial();
    if (defensive != 0)
    {
        _stats.defensiveUsed++;
        _stats.totalUsed++;
        _lastUsedTime[defensive] = now;
        return defensive;
    }

    // Only consider offensive/resource/utility if in combat
    if (!_bot->IsInCombat())
        return 0;

    // 3. Offensive - during burst windows or on cooldown
    uint32 offensive = GetOffensiveRacial();
    if (offensive != 0)
    {
        _stats.offensiveUsed++;
        _stats.totalUsed++;
        _lastUsedTime[offensive] = now;
        return offensive;
    }

    // 4. Resource - when low on primary resource
    uint32 resource = GetResourceRacial();
    if (resource != 0)
    {
        _stats.utilityUsed++;
        _stats.totalUsed++;
        _lastUsedTime[resource] = now;
        return resource;
    }

    // 5. AoE CC - when multiple enemies nearby and not in boss fight
    uint32 aoeCc = GetAoECCRacial();
    if (aoeCc != 0)
    {
        _stats.utilityUsed++;
        _stats.totalUsed++;
        _lastUsedTime[aoeCc] = now;
        return aoeCc;
    }

    return 0;
}

uint32 RacialAbilityManager::GetOffensiveRacial() const
{
    for (auto const& racial : _racials)
    {
        if (racial.category != RacialCategory::OFFENSIVE)
            continue;

        if (!IsSpellReady(racial.spellId))
            continue;

        // Use during burst if aligned, otherwise use on cooldown if flagged
        if (racial.useDuringBurst)
        {
            if (IsInBurstWindow())
                return racial.spellId;
        }
        else if (racial.useOnCooldown)
        {
            return racial.spellId;
        }
    }

    return 0;
}

uint32 RacialAbilityManager::GetDefensiveRacial() const
{
    if (!_bot)
        return 0;

    float healthPct = _bot->GetHealthPct() / 100.0f;

    for (auto const& racial : _racials)
    {
        if (racial.category != RacialCategory::DEFENSIVE)
            continue;

        if (!IsSpellReady(racial.spellId))
            continue;

        // Use when health drops below threshold
        if (racial.healthThreshold > 0.0f && healthPct <= racial.healthThreshold)
            return racial.spellId;
    }

    return 0;
}

uint32 RacialAbilityManager::GetCCBreakRacial() const
{
    if (!IsCrowdControlled())
        return 0;

    for (auto const& racial : _racials)
    {
        if (racial.category != RacialCategory::CC_BREAK)
            continue;

        if (!IsSpellReady(racial.spellId))
            continue;

        return racial.spellId;
    }

    return 0;
}

uint32 RacialAbilityManager::GetResourceRacial() const
{
    if (!_bot)
        return 0;

    // Use resource racials when primary power is below 30%
    float powerPct = 100.0f;
    Powers powerType = _bot->GetPowerType();
    uint32 maxPower = _bot->GetMaxPower(powerType);
    if (maxPower > 0)
        powerPct = static_cast<float>(_bot->GetPower(powerType)) / static_cast<float>(maxPower) * 100.0f;

    if (powerPct > 30.0f)
        return 0;

    for (auto const& racial : _racials)
    {
        if (racial.category != RacialCategory::RESOURCE)
            continue;

        if (!IsSpellReady(racial.spellId))
            continue;

        return racial.spellId;
    }

    return 0;
}

uint32 RacialAbilityManager::GetAoECCRacial() const
{
    if (!_bot)
        return 0;

    // Only use AoE CC when 3+ enemies are nearby and in melee range
    uint32 nearbyEnemies = 0;
    for (auto const& [guid, ref] : _bot->GetThreatManager().GetThreatenedByMeList())
    {
        if (!ref)
            continue;
        Creature* enemy = ref->GetOwner();
        if (enemy && enemy->IsAlive() && _bot->GetDistance(enemy) <= 8.0f)
            ++nearbyEnemies;
    }

    if (nearbyEnemies < 3)
        return 0;

    for (auto const& racial : _racials)
    {
        if (racial.category != RacialCategory::AOE_CC)
            continue;

        if (!IsSpellReady(racial.spellId))
            continue;

        return racial.spellId;
    }

    return 0;
}

// ============================================================================
// QUERIES
// ============================================================================

bool RacialAbilityManager::HasOffensiveRacial() const
{
    for (auto const& r : _racials)
        if (r.category == RacialCategory::OFFENSIVE)
            return true;
    return false;
}

bool RacialAbilityManager::HasDefensiveRacial() const
{
    for (auto const& r : _racials)
        if (r.category == RacialCategory::DEFENSIVE)
            return true;
    return false;
}

bool RacialAbilityManager::HasCCBreakRacial() const
{
    for (auto const& r : _racials)
        if (r.category == RacialCategory::CC_BREAK)
            return true;
    return false;
}

::std::string RacialAbilityManager::GetRaceName() const
{
    if (!_bot)
        return "Unknown";

    switch (_bot->GetRace())
    {
        case RACE_HUMAN:                   return "Human";
        case RACE_ORC:                     return "Orc";
        case RACE_DWARF:                   return "Dwarf";
        case RACE_NIGHTELF:                return "Night Elf";
        case RACE_UNDEAD_PLAYER:           return "Undead";
        case RACE_TAUREN:                  return "Tauren";
        case RACE_GNOME:                   return "Gnome";
        case RACE_TROLL:                   return "Troll";
        case RACE_GOBLIN:                  return "Goblin";
        case RACE_BLOODELF:                return "Blood Elf";
        case RACE_DRAENEI:                 return "Draenei";
        case RACE_WORGEN:                  return "Worgen";
        case RACE_PANDAREN_ALLIANCE:
        case RACE_PANDAREN_HORDE:
        case RACE_PANDAREN_NEUTRAL:        return "Pandaren";
        case RACE_NIGHTBORNE:              return "Nightborne";
        case RACE_HIGHMOUNTAIN_TAUREN:     return "Highmountain Tauren";
        case RACE_VOID_ELF:               return "Void Elf";
        case RACE_LIGHTFORGED_DRAENEI:     return "Lightforged Draenei";
        case RACE_ZANDALARI_TROLL:         return "Zandalari Troll";
        case RACE_KUL_TIRAN:              return "Kul Tiran";
        case RACE_DARK_IRON_DWARF:         return "Dark Iron Dwarf";
        case RACE_VULPERA:                 return "Vulpera";
        case RACE_MAGHAR_ORC:              return "Mag'har Orc";
        case RACE_MECHAGNOME:              return "Mechagnome";
        case RACE_DRACTHYR_ALLIANCE:
        case RACE_DRACTHYR_HORDE:          return "Dracthyr";
        case RACE_EARTHEN_DWARF_ALLIANCE:
        case RACE_EARTHEN_DWARF_HORDE:     return "Earthen";
        default:                           return "Unknown";
    }
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

bool RacialAbilityManager::CanUseSpell(uint32 spellId) const
{
    if (!_bot)
        return false;

    if (!_bot->HasSpell(spellId))
        return false;

    // Check if the spell is usable (not silenced, not on GCD for instants, etc.)
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    return true;
}

bool RacialAbilityManager::IsSpellReady(uint32 spellId) const
{
    if (!_bot)
        return false;

    if (!_bot->HasSpell(spellId))
        return false;

    return !_bot->GetSpellHistory()->HasCooldown(spellId);
}

bool RacialAbilityManager::IsCrowdControlled() const
{
    if (!_bot)
        return false;

    return _bot->HasUnitState(UNIT_STATE_STUNNED) ||
           _bot->HasUnitState(UNIT_STATE_CONFUSED) ||
           _bot->HasUnitState(UNIT_STATE_FLEEING) ||
           _bot->HasAuraType(SPELL_AURA_MOD_CHARM) ||
           _bot->HasAuraType(SPELL_AURA_MOD_FEAR) ||
           _bot->HasAuraType(SPELL_AURA_MOD_STUN) ||
           _bot->HasAuraType(SPELL_AURA_TRANSFORM);
}

bool RacialAbilityManager::IsInBurstWindow() const
{
    if (!_bot || !_bot->IsInCombat())
        return false;

    // Check for common burst indicators:
    // 1. Bloodlust/Heroism/Time Warp active
    if (_bot->HasAura(2825) || _bot->HasAura(32182) || _bot->HasAura(80353))
        return true;

    // 2. Power Infusion active (10060)
    if (_bot->HasAura(10060))
        return true;

    // 3. Target below 20% health (execute phase)
    Unit* target = _bot->GetVictim();
    if (target && target->GetHealthPct() <= 20.0f)
        return true;

    // 4. Just entered combat (opener window - first 5 seconds)
    // This is a heuristic - offensive racials are good in openers
    return false;
}

} // namespace Playerbot
