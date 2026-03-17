/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#include "DefensiveManager.h"
#include "Player.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "ThreatManager.h"
#include "Log.h"
#include "Creature.h"
#include "GameTime.h"
#include "../../Group/RoleDefinitions.h"
#include <algorithm>

namespace Playerbot
{

DefensiveManager::DefensiveManager(Player* bot)
    : _bot(bot)
    , _recentDamage(0.0f)
    , _lastUpdate(0)
{
}

void DefensiveManager::Update(uint32 diff, const CombatMetrics& metrics)
{
    if (!_bot)
        return;

    _lastUpdate += diff;

    if (_lastUpdate < UPDATE_INTERVAL)
        return;

    _lastUpdate = 0;

    // Update damage tracking
    UpdateDamageTracking(metrics);
}

void DefensiveManager::Reset()
{
    _cooldownTracker.clear();
    _recentDamage = 0.0f;
    _lastUpdate = 0;
}

bool DefensiveManager::NeedsDefensive()
{
    if (!_bot)
        return false;

    float healthPct = _bot->GetHealthPct();
    float incomingDamage = EstimateIncomingDamage();

    return ShouldUseDefensive(healthPct, incomingDamage);
}

bool DefensiveManager::NeedsEmergencyDefensive()
{
    if (!_bot)
        return false;

    float healthPct = _bot->GetHealthPct();

    // Emergency threshold: < 20% HP
    return healthPct < 20.0f;
}

uint32 DefensiveManager::GetRecommendedDefensive()
{
    if (!_bot)
        return 0;

    float healthPct = _bot->GetHealthPct();
    float incomingDamage = EstimateIncomingDamage();

    return GetBestDefensive(healthPct, incomingDamage);
}

uint32 DefensiveManager::UseEmergencyDefensive()
{
    if (!_bot)
        return 0;

    // Get emergency defensives only
    ::std::vector<DefensiveCooldown*> emergencies;

    for (auto& defensive : _availableDefensives)
    {
        if (defensive.isEmergency && defensive.IsAvailable() && !IsOnCooldown(defensive.spellId))
            emergencies.push_back(&defensive);
    }

    if (emergencies.empty())
        return 0;

    // Sort by damage reduction (highest first)
    ::std::sort(emergencies.begin(), emergencies.end(),
        [](const DefensiveCooldown* a, const DefensiveCooldown* b) {
            return a->damageReduction > b->damageReduction;
        });

    // Use best emergency defensive
    DefensiveCooldown* best = emergencies.front();
    best->MarkUsed();
    _cooldownTracker[best->spellId] = GameTime::GetGameTimeMS();

    TC_LOG_DEBUG("playerbot", "DefensiveManager: {} using EMERGENCY defensive {}",
        _bot->GetName(), best->spellId);

    return best->spellId;
}

void DefensiveManager::RegisterDefensive(const DefensiveCooldown& cooldown)
{
    _availableDefensives.push_back(cooldown);
}

void DefensiveManager::UseDefensiveCooldown(uint32 spellId)
{
    DefensiveCooldown* defensive = FindDefensive(spellId);
    if (!defensive)
        return;

    defensive->MarkUsed();
    _cooldownTracker[spellId] = GameTime::GetGameTimeMS();

    TC_LOG_DEBUG("playerbot", "DefensiveManager: {} used defensive {}",
        _bot ? _bot->GetName() : "unknown", spellId);
}

bool DefensiveManager::ShouldUseDefensive(float healthPercent, float incomingDamage)
{
    if (!_bot)
        return false;

    // Check if any defensive is available
    if (GetAvailableDefensives(DefensivePriority::EMERGENCY).empty())
        return false;

    // Already has active defensive? Don't stack
    if (HasActiveDefensive())
        return false;

    // Determine threshold based on role using RoleDefinitions
    float threshold = 50.0f;  // Default for DPS

    uint8 classId = _bot->GetClass();
    uint8 specId = static_cast<uint8>(_bot->GetPrimarySpecialization());
    GroupRole primaryRole = RoleDefinitions::GetPrimaryRole(classId, specId);

    switch (primaryRole)
    {
        case GroupRole::TANK:
            // Tanks should use defensives more liberally to smooth damage
            threshold = 80.0f;
            break;
        case GroupRole::HEALER:
            // Healers can partially self-heal, use defensives at lower threshold
            threshold = 70.0f;
            break;
        case GroupRole::MELEE_DPS:
            // Melee DPS take more damage, moderate threshold
            threshold = 60.0f;
            break;
        case GroupRole::RANGED_DPS:
        default:
            // Ranged DPS should avoid damage, emergency threshold
            threshold = 50.0f;
            break;
    }

    // Need defensive if below threshold or high incoming damage
    return (healthPercent < threshold) || (incomingDamage > (_bot->GetMaxHealth() * 0.3f));
}

uint32 DefensiveManager::GetBestDefensive(float healthPercent, float incomingDamage)
{
    if (!_bot)
        return 0;

    // Determine priority threshold based on health
    DefensivePriority minPriority = DefensivePriority::OPTIONAL;

    if (healthPercent < 20.0f)
        minPriority = DefensivePriority::EMERGENCY;
    else if (healthPercent < 40.0f)
        minPriority = DefensivePriority::HIGH;
    else if (healthPercent < 60.0f)
        minPriority = DefensivePriority::MEDIUM;
    else if (healthPercent < 80.0f)
        minPriority = DefensivePriority::LOW;

    // Get available defensives
    ::std::vector<DefensiveCooldown*> available = GetAvailableDefensives(minPriority);

    if (available.empty())
        return 0;

    // Sort by priority (emergency first, then damage reduction)
    ::std::sort(available.begin(), available.end(),
        [](const DefensiveCooldown* a, const DefensiveCooldown* b) {
            if (a->priority != b->priority)
                return a->priority < b->priority;  // Lower enum value = higher priority
            return a->damageReduction > b->damageReduction;
        });

    return available.front()->spellId;
}

bool DefensiveManager::IsOnCooldown(uint32 spellId) const
{
    auto it = _cooldownTracker.find(spellId);
    if (it == _cooldownTracker.end())
        return false;

    DefensiveCooldown* defensive = const_cast<DefensiveManager*>(this)->FindDefensive(spellId);
    if (!defensive)
        return false;

    uint32 now = GameTime::GetGameTimeMS();
    return (now - it->second) < defensive->cooldown;
}

uint32 DefensiveManager::GetRemainingCooldown(uint32 spellId) const
{
    auto it = _cooldownTracker.find(spellId);
    if (it == _cooldownTracker.end())
        return 0;

    DefensiveCooldown* defensive = const_cast<DefensiveManager*>(this)->FindDefensive(spellId);
    if (!defensive)
        return 0;

    uint32 now = GameTime::GetGameTimeMS();
    uint32 elapsed = now - it->second;

    if (elapsed >= defensive->cooldown)
        return 0;

    return defensive->cooldown - elapsed;
}

float DefensiveManager::EstimateIncomingDamage() const
{
    if (!_bot)
        return 0.0f;

    // Base estimate on recent damage
    float estimate = _recentDamage;

    // Multiply by enemy count
    ThreatManager& threatMgr = _bot->GetThreatManager();

    uint32 enemyCount = 0;
    for (ThreatReference const* ref : threatMgr.GetUnsortedThreatList())
    {
        if (ref && ref->GetVictim() && !ref->GetVictim()->isDead())
            ++enemyCount;
    }

    if (enemyCount > 0)
        estimate *= ::std::min(enemyCount, 5u);  // Cap at 5× multiplier

    return estimate;
}

// Private helper functions

DefensiveCooldown* DefensiveManager::FindDefensive(uint32 spellId)
{
    for (auto& defensive : _availableDefensives)
    {
        if (defensive.spellId == spellId)
            return &defensive;
    }
    return nullptr;
}

::std::vector<DefensiveCooldown*> DefensiveManager::GetAvailableDefensives(DefensivePriority minPriority)
{
    ::std::vector<DefensiveCooldown*> available;

    for (auto& defensive : _availableDefensives)
    {
        // Check priority
    if (defensive.priority > minPriority)
            continue;

        // Check availability
    if (!defensive.IsAvailable())
            continue;

        // Check cooldown
    if (IsOnCooldown(defensive.spellId))
            continue;

        available.push_back(&defensive);
    }

    return available;
}

bool DefensiveManager::HasActiveDefensive() const
{
    if (!_bot)
        return false;

    // Comprehensive defensive aura detection
    // Check for damage reduction auras
    if (_bot->HasAuraType(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN))
        return true;

    // Check for absorb shields
    if (_bot->HasAuraType(SPELL_AURA_SCHOOL_ABSORB))
        return true;

    // Check for immunity effects
    if (_bot->HasAuraType(SPELL_AURA_SCHOOL_IMMUNITY))
        return true;

    // Check for deflection/parry buffs
    if (_bot->HasAuraType(SPELL_AURA_MOD_PARRY_PERCENT) &&
        _bot->GetAuraEffectsByType(SPELL_AURA_MOD_PARRY_PERCENT).front()->GetAmount() > 10)
        return true;

    // Check for dodge buffs
    if (_bot->HasAuraType(SPELL_AURA_MOD_DODGE_PERCENT) &&
        _bot->GetAuraEffectsByType(SPELL_AURA_MOD_DODGE_PERCENT).front()->GetAmount() > 10)
        return true;

    // Check for block amount increases (shield wall type effects)
    if (_bot->HasAuraType(SPELL_AURA_MOD_BLOCK_PERCENT) &&
        _bot->GetAuraEffectsByType(SPELL_AURA_MOD_BLOCK_PERCENT).front()->GetAmount() > 20)
        return true;

    // Check known major defensive cooldown spell IDs
    static const std::vector<uint32> majorDefensiveSpells = {
        48707,  // Anti-Magic Shell (DK)
        48792,  // Icebound Fortitude (DK)
        871,    // Shield Wall (Warrior)
        12975,  // Last Stand (Warrior)
        498,    // Divine Protection (Paladin)
        642,    // Divine Shield (Paladin)
        31850,  // Ardent Defender (Paladin)
        22812,  // Barkskin (Druid)
        61336,  // Survival Instincts (Druid)
        47585,  // Dispersion (Priest)
        108271, // Astral Shift (Shaman)
        115203, // Fortifying Brew (Monk)
        122278, // Dampen Harm (Monk)
        122783, // Diffuse Magic (Monk)
        198589, // Blur (Demon Hunter)
        187827, // Metamorphosis (Demon Hunter)
        186265, // Aspect of the Turtle (Hunter)
        1966,   // Feint (Rogue)
        31224,  // Cloak of Shadows (Rogue)
        108238, // Spear Hand Strike (Monk)
    };

    for (uint32 spellId : majorDefensiveSpells)
    {
        if (_bot->HasAura(spellId))
            return true;
    }

    return false;
}

float DefensiveManager::GetHealthDeficit() const
{
    if (!_bot)
        return 0.0f;

    return 100.0f - _bot->GetHealthPct();
}

void DefensiveManager::UpdateDamageTracking(const CombatMetrics& metrics)
{
    if (!_bot)
        return;

    // Track damage from combat metrics
    // The CombatMetrics structure provides damage taken over time windows

    // Calculate recent damage using health delta tracking
    static float lastHealthPct = 100.0f;
    float currentHealthPct = _bot->GetHealthPct();

    // If health decreased, record damage
    if (currentHealthPct < lastHealthPct)
    {
        float healthLost = lastHealthPct - currentHealthPct;
        float maxHealth = static_cast<float>(_bot->GetMaxHealth());
        _recentDamage = (healthLost / 100.0f) * maxHealth;
    }
    else
    {
        // Health stable or increased (healed), decay recent damage
        _recentDamage *= 0.8f;
    }

    lastHealthPct = currentHealthPct;

    // Clamp to reasonable bounds
    _recentDamage = std::clamp(_recentDamage, 0.0f, static_cast<float>(_bot->GetMaxHealth()));
}

} // namespace Playerbot
