/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ResourceManager.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "DB2Structure.h"
#include "Log.h"
#include <algorithm>
#include "../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries
#include "GameTime.h"
#include "SpellHistory.h"
#include "SpellAuraDefines.h"
#include "SpellAuraEffects.h"

namespace Playerbot
{

ResourceManager::ResourceManager(Player* bot) : _bot(bot), _runicPower(0)
{
    // Initialize runes to available state
    for (uint32 i = 0; i < MAX_RUNES; ++i)
    {
        _runes[i].available = true;
        _runes[i].cooldownRemaining = 0;
        _runes[i].type = (i < 2) ? 0 : ((i < 4) ? 1 : 2); // 2 Blood, 2 Frost, 2 Unholy
    }

    // CRITICAL: Do NOT access _bot->GetName() or _bot->GetGUID() in constructor!
    // Bot may not be fully in world yet during ClassAI/BotAI construction,
    // and Player::m_name/m_guid are not initialized, causing ACCESS_VIOLATION.
    // Logging with bot name deferred to first Update() when bot IsInWorld().
}


void ResourceManager::Update(uint32 diff)
{
    _updateCount.fetch_add(1);

    // Sync with player state
    SyncWithPlayer();

    // Update resource regeneration
    for (auto& pair : _resources)
    {
        UpdateResourceRegeneration(pair.second, diff);
    }

    // Update runes for Death Knights
    if (_bot && _bot->GetClass() == CLASS_DEATH_KNIGHT)    {
        UpdateRunes(diff);
    }

    // Performance monitoring
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastPerformanceCheck > PERFORMANCE_CHECK_INTERVAL)
    {
        _lastPerformanceCheck = currentTime;
        // Could add performance metrics here
    }
}

void ResourceManager::Initialize()
{
    if (!_bot)
        return;

    InitializeClassResources();
    LoadSpellResourceCosts();
    SyncWithPlayer();


    TC_LOG_DEBUG("playerbot.resource", "ResourceManager initialized for class {}",
    static_cast<uint32>(_bot->GetClass()));
}

bool ResourceManager::HasEnoughResource(uint32 spellId)
{
    if (!spellId || !_bot)
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Check spell power costs using modern API
    ::std::vector<SpellPowerCost> costs = spellInfo->CalcPowerCost(_bot, spellInfo->GetSchoolMask());
    if (costs.empty())
        return true; // No resource cost

    // Check if we have enough of each required resource
    for (const SpellPowerCost& powerCost : costs)
    {
        if (powerCost.Amount > 0)
        {
            ResourceType resourceType = GetResourceTypeForPower(powerCost.Power);
            if (!HasEnoughResource(resourceType, powerCost.Amount))
                return false;
        }
    }

    return true;
}

bool ResourceManager::HasEnoughResource(ResourceType type, uint32 amount)
{
    auto it = _resources.find(type);
    if (it != _resources.end())
    {
        return it->second.HasEnough(amount);
    }    // Fallback to player power if not tracked
    Powers powerType = GetPowerTypeForResource(type);
    return _bot && _bot->GetPower(powerType) >= amount;
}

uint32 ResourceManager::GetResource(ResourceType type)
{
    auto it = _resources.find(type);
    if (it != _resources.end())
    {
        return it->second.current;
    }

    // Fallback to player power
    Powers powerType = GetPowerTypeForResource(type);
    return _bot ? _bot->GetPower(powerType) : 0;
}

uint32 ResourceManager::GetMaxResource(ResourceType type)
{
    auto it = _resources.find(type);
    if (it != _resources.end())
    {
        return it->second.maximum;
    }

    // Fallback to player power
    Powers powerType = GetPowerTypeForResource(type);
    return _bot ? _bot->GetMaxPower(powerType) : 0;
}

float ResourceManager::GetResourcePercent(ResourceType type)
{
    auto it = _resources.find(type);
    if (it != _resources.end())
    {
        return it->second.GetPercent();
    }

    // Fallback to player power
    Powers powerType = GetPowerTypeForResource(type);
    if (_bot)
    {
        uint32 max = _bot->GetMaxPower(powerType);
        return max > 0 ? static_cast<float>(_bot->GetPower(powerType)) / max : 0.0f;
    }

    return 0.0f;
}

void ResourceManager::ConsumeResource(uint32 spellId)
{
    if (!spellId || !_bot)
        return;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return;

    // Get power costs using modern API
    ::std::vector<SpellPowerCost> costs = spellInfo->CalcPowerCost(_bot, spellInfo->GetSchoolMask());

    for (const SpellPowerCost& powerCost : costs)
    {
        if (powerCost.Amount > 0)
        {

            ResourceType resourceType = GetResourceTypeForPower(powerCost.Power);

            ConsumeResource(resourceType, powerCost.Amount);

            // Record usage for tracking

            RecordResourceUsage(resourceType, powerCost.Amount, spellId);
        }
    }
}

void ResourceManager::ConsumeResource(ResourceType type, uint32 amount)
{
    auto it = _resources.find(type);
    if (it != _resources.end())
    {
        uint32 consumed = it->second.Consume(amount);
        _totalConsumed[type] += consumed;

        TC_LOG_DEBUG("playerbot.resource", "Consumed {} {} ({}%)",

                     consumed, static_cast<uint32>(type), it->second.GetPercent() * 100.0f);
    }
}

void ResourceManager::AddResource(ResourceType type, uint32 amount)
{
    auto it = _resources.find(type);
    if (it != _resources.end())
    {
        uint32 added = it->second.Add(amount);
        _totalGenerated[type] += added;

        TC_LOG_DEBUG("playerbot.resource", "Added {} {} ({}%)",

                     added, static_cast<uint32>(type), it->second.GetPercent() * 100.0f);
    }
}

void ResourceManager::SetResource(ResourceType type, uint32 amount)
{
    auto it = _resources.find(type);
    if (it != _resources.end())
    {
        it->second.current = ::std::min(amount, it->second.maximum);
    }
}

void ResourceManager::SetMaxResource(ResourceType type, uint32 amount)
{
    auto it = _resources.find(type);
    if (it != _resources.end())
    {
        it->second.maximum = amount;
        it->second.current = ::std::min(it->second.current, amount);
    }
}

uint32 ResourceManager::GetResourceIn(ResourceType type, uint32 timeMs)
{
    auto it = _resources.find(type);
    if (it != _resources.end() && it->second.isRegenerated)
    {
        float regenAmount = it->second.regenRate * (timeMs / 1000.0f);
        uint32 futureAmount = it->second.current + static_cast<uint32>(regenAmount);
        return ::std::min(futureAmount, it->second.maximum);
    }

    return GetResource(type);
}

bool ResourceManager::WillHaveEnoughIn(ResourceType type, uint32 amount, uint32 timeMs)
{
    return GetResourceIn(type, timeMs) >= amount;
}

uint32 ResourceManager::GetTimeToResource(ResourceType type, uint32 amount)
{
    auto it = _resources.find(type);
    if (it != _resources.end() && it->second.isRegenerated && it->second.regenRate > 0.0f)
    {
        if (it->second.current >= amount)

            return 0;

        uint32 needed = amount - it->second.current;
        return static_cast<uint32>((needed / it->second.regenRate) * 1000.0f);
    }

    return UINT32_MAX; // Never
}

uint32 ResourceManager::GetComboPoints()
{
    return GetResource(ResourceType::COMBO_POINTS);
}

void ResourceManager::ConsumeComboPoints()
{
    ConsumeResource(ResourceType::COMBO_POINTS, GetComboPoints());
}

void ResourceManager::AddComboPoints(uint32 points)
{
    AddResource(ResourceType::COMBO_POINTS, points);
}

uint32 ResourceManager::GetHolyPower()
{
    return GetResource(ResourceType::HOLY_POWER);
}

void ResourceManager::ConsumeHolyPower()
{
    ConsumeResource(ResourceType::HOLY_POWER, GetHolyPower());
}

void ResourceManager::AddHolyPower(uint32 power)
{
    AddResource(ResourceType::HOLY_POWER, power);
}

uint32 ResourceManager::GetChi()
{
    return GetResource(ResourceType::CHI);
}

void ResourceManager::ConsumeChi(uint32 amount)
{
    ConsumeResource(ResourceType::CHI, amount);
}

void ResourceManager::AddChi(uint32 amount)
{
    AddResource(ResourceType::CHI, amount);
}

::std::vector<ResourceManager::RuneInfo> ResourceManager::GetRunes()
{
    ::std::vector<RuneInfo> result;
    for (uint32 i = 0; i < MAX_RUNES; ++i)
    {
        result.push_back(_runes[i]);
    }
    return result;
}

uint32 ResourceManager::GetAvailableRunes(uint8 runeType)
{
    uint32 count = 0;
    for (uint32 i = 0; i < MAX_RUNES; ++i)
    {
        if (_runes[i].available && (runeType == 255 || _runes[i].type == runeType))
        {

            count++;
        }
    }
    return count;
}

bool ResourceManager::HasRunesAvailable(uint32 bloodRunes, uint32 frostRunes, uint32 unholyRunes)
{
    return GetAvailableRunes(0) >= bloodRunes &&
           GetAvailableRunes(1) >= frostRunes &&
           GetAvailableRunes(2) >= unholyRunes;
}

void ResourceManager::ConsumeRunes(uint32 bloodRunes, uint32 frostRunes, uint32 unholyRunes)
{
    auto consumeRuneType = [this](uint8 runeType, uint32 count) {
        uint32 consumed = 0;
        for (uint32 i = 0; i < MAX_RUNES && consumed < count; ++i)
        {

            if (_runes[i].available && _runes[i].type == runeType)

            {

                _runes[i].available = false;

                _runes[i].cooldownRemaining = RUNE_COOLDOWN_MS;

                consumed++;

            }
        }
    };

    consumeRuneType(0, bloodRunes);
    consumeRuneType(1, frostRunes);
    consumeRuneType(2, unholyRunes);

    TC_LOG_DEBUG("playerbot.resource", "Consumed runes: {} blood, {} frost, {} unholy",

                 bloodRunes, frostRunes, unholyRunes);
}

void ResourceManager::RecordResourceUsage(ResourceType type, uint32 amount, uint32 spellId)
{
    _spellResourceCost[spellId] = amount;
    _spellUsageCount[spellId]++;

    // Report to monitor
    if (_bot)
    {        ResourceMonitor::Instance().RecordResourceUsage(_bot->GetGUID().GetCounter(), type, amount);
    }
}

float ResourceManager::GetResourceEfficiency(ResourceType type)
{
    auto totalGenerated = _totalGenerated.find(type);
    auto totalConsumed = _totalConsumed.find(type);

    if (totalGenerated != _totalGenerated.end() && totalConsumed != _totalConsumed.end())
    {
        uint32 generated = totalGenerated->second;
        uint32 consumed = totalConsumed->second;

        if (generated > 0)
        {

            return static_cast<float>(consumed) / generated;
        }
    }

    return 1.0f; // Perfect efficiency if no data
}

float ResourceManager::GetSpellResourceEfficiency(uint32 spellId)
{
    auto usageIt = _spellUsageCount.find(spellId);
    auto costIt = _spellResourceCost.find(spellId);

    if (usageIt != _spellUsageCount.end() && costIt != _spellResourceCost.end())
    {
        // Simple efficiency metric: usage frequency vs cost
        return static_cast<float>(usageIt->second) / costIt->second;
    }

    return 1.0f;
}

bool ResourceManager::CanAffordSpellSequence(const ::std::vector<uint32>& spellIds)
{
    ::std::unordered_map<ResourceType, uint32> totalCosts;

    for (uint32 spellId : spellIds)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (spellInfo)
        {
            // Get power costs for this spell

            ::std::vector<SpellPowerCost> costs = spellInfo->CalcPowerCost(_bot, spellInfo->GetSchoolMask());

            for (const SpellPowerCost& powerCost : costs)

            {

                ResourceType type = GetResourceTypeForPower(powerCost.Power);

                uint32 cost = powerCost.Amount;

                totalCosts[type] += cost;

            }
        }
    }

    for (const auto& pair : totalCosts)
    {
        if (!HasEnoughResource(pair.first, pair.second))

            return false;
    }

    return true;
}

uint32 ResourceManager::GetOptimalResourceThreshold(ResourceType type)
{
    uint32 maxResource = GetMaxResource(type);

    // Different thresholds for different resource types
    switch (type)
    {
        case ResourceType::MANA:

            return maxResource * 30 / 100; // 30% for mana
            case ResourceType::ENERGY:

            return maxResource * 40 / 100; // 40% for energy
        case ResourceType::RAGE:

            return maxResource * 20 / 100; // 20% for rage
        default:

            return maxResource * 25 / 100; // 25% default
    }
}

bool ResourceManager::ShouldConserveResource(ResourceType type)
{
    float currentPercent = GetResourcePercent(type);
    return currentPercent < CONSERVATION_THRESHOLD;
}

bool ResourceManager::IsResourceCritical(ResourceType type)
{
    float currentPercent = GetResourcePercent(type);
    return currentPercent < CRITICAL_RESOURCE_THRESHOLD;
}

bool ResourceManager::NeedsResourceEmergency()
{
    ResourceType primaryResource = GetPrimaryResourceType();
    return IsResourceCritical(primaryResource);
}

::std::vector<uint32> ResourceManager::GetResourceEmergencySpells()
{
    ::std::vector<uint32> emergencySpells;

    if (!_bot)
        return emergencySpells;

    ChrSpecializationEntry const* spec = _bot->GetPrimarySpecializationEntry();
    uint32 specId = spec ? spec->ID : 0;

    // Class-specific emergency resource recovery spells
    switch (_bot->GetClass())
    {
        case CLASS_WARRIOR:
        {
            // Berserker Rage - breaks fear and generates rage
            emergencySpells.push_back(18499);

            // Avatar - burst window, generates rage on use
            if (specId == 71 || specId == 72) // Arms/Fury
                emergencySpells.push_back(107574);

            // Recklessness (Fury) - increases crit, synergizes with rage gen
            if (specId == 72) // Fury
                emergencySpells.push_back(1719);

            // Shield Wall / Last Stand for Protection (defensive, not resource)
            if (specId == 73) // Protection
            {
                emergencySpells.push_back(871);   // Shield Wall
                emergencySpells.push_back(12975); // Last Stand
            }
            break;
        }

        case CLASS_ROGUE:
        {
            // Adrenaline Rush (Outlaw) - doubles energy regen
            if (specId == 260) // Outlaw
                emergencySpells.push_back(13750);

            // Shadow Dance (Subtlety) - enables Shadowstrike spam
            if (specId == 261) // Subtlety
                emergencySpells.push_back(185313);

            // Vendetta (Assassination) - increases damage, synergy
            if (specId == 259) // Assassination
                emergencySpells.push_back(79140);

            // Thistle Tea - restores 100 energy
            emergencySpells.push_back(381623);

            // Vanish - resets combat state, enables openers
            emergencySpells.push_back(1856);

            break;
        }

        case CLASS_MAGE:
        {
            // Evocation - restores mana over channel
            emergencySpells.push_back(12051);

            // Arcane Surge (Arcane) - burst + mana management
            if (specId == 62) // Arcane
            {
                emergencySpells.push_back(365350);
            }

            // Ice Block - emergency defensive (not mana)
            emergencySpells.push_back(45438);

            break;
        }

        case CLASS_PALADIN:
        {
            // Divine Shield - emergency defensive
            emergencySpells.push_back(642);

            // Lay on Hands - emergency heal
            emergencySpells.push_back(633);

            // Avenging Wrath - burst window
            emergencySpells.push_back(31884);

            // Divine Toll - generates Holy Power
            emergencySpells.push_back(375576);

            break;
        }

        case CLASS_HUNTER:
        {
            // Exhilaration - heals and focus restore
            emergencySpells.push_back(109304);

            // Trueshot (Marksmanship) - rapid fire
            if (specId == 254)
                emergencySpells.push_back(288613);

            // Bestial Wrath (Beast Mastery) - damage + focus
            if (specId == 253)
                emergencySpells.push_back(19574);

            // Aspect of the Wild (Beast Mastery)
            if (specId == 253)
                emergencySpells.push_back(193530);

            break;
        }

        case CLASS_PRIEST:
        {
            // Shadowfiend/Mindbender - mana recovery
            if (specId == 256 || specId == 257) // Holy/Discipline
            {
                emergencySpells.push_back(34433);  // Shadowfiend
                emergencySpells.push_back(123040); // Mindbender
            }

            // Dispersion (Shadow) - emergency defensive + insanity
            if (specId == 258)
                emergencySpells.push_back(47585);

            // Symbol of Hope - party mana restore
            if (specId == 257) // Discipline
                emergencySpells.push_back(64901);

            break;
        }

        case CLASS_DEATH_KNIGHT:
        {
            // Empower Rune Weapon - restores runes and runic power
            emergencySpells.push_back(47568);

            // Pillar of Frost (Frost) - burst
            if (specId == 251)
                emergencySpells.push_back(51271);

            // Dancing Rune Weapon (Blood) - defensive
            if (specId == 250)
                emergencySpells.push_back(49028);

            // Unholy Frenzy (Unholy)
            if (specId == 252)
                emergencySpells.push_back(207289);

            break;
        }

        case CLASS_SHAMAN:
        {
            // Mana Tide Totem (Restoration) - party mana restore
            if (specId == 264)
                emergencySpells.push_back(16191);

            // Feral Spirit (Enhancement) - wolves
            if (specId == 263)
                emergencySpells.push_back(51533);

            // Stormkeeper (Elemental) - instant Lightning Bolts
            if (specId == 262)
                emergencySpells.push_back(191634);

            // Astral Shift - emergency defensive
            emergencySpells.push_back(108271);

            break;
        }

        case CLASS_WARLOCK:
        {
            // Life Tap (if still exists) - mana from health
            emergencySpells.push_back(1454);

            // Dark Soul: Misery/Instability - damage burst
            emergencySpells.push_back(113860);
            emergencySpells.push_back(113858);

            // Unending Resolve - emergency defensive
            emergencySpells.push_back(104773);

            // Summon Darkglare (Affliction)
            if (specId == 265)
                emergencySpells.push_back(205180);

            break;
        }

        case CLASS_DRUID:
        {
            // Innervate - mana restore (for self or ally)
            if (specId == 105) // Restoration
                emergencySpells.push_back(29166);

            // Tiger's Fury (Feral) - energy restore + damage
            if (specId == 103)
                emergencySpells.push_back(5217);

            // Berserk/Incarnation - burst windows
            emergencySpells.push_back(106951); // Berserk (Feral)
            emergencySpells.push_back(102558); // Incarnation (Guardian)

            // Barkskin - emergency defensive
            emergencySpells.push_back(22812);

            break;
        }

        case CLASS_MONK:
        {
            // Energizing Elixir - energy + chi restore
            emergencySpells.push_back(115288);

            // Touch of Karma (Windwalker) - damage redirect
            if (specId == 269)
                emergencySpells.push_back(122470);

            // Fortifying Brew (Brewmaster) - defensive
            if (specId == 268)
                emergencySpells.push_back(115203);

            // Thunder Focus Tea (Mistweaver) - empowers next spell
            if (specId == 270)
                emergencySpells.push_back(116680);

            break;
        }

        case CLASS_DEMON_HUNTER:
        {
            // Metamorphosis - burst + resource generation
            emergencySpells.push_back(191427); // Havoc
            emergencySpells.push_back(187827); // Vengeance

            // Eye Beam (Havoc) - AoE + fury generation
            if (specId == 577)
                emergencySpells.push_back(198013);

            // Fiery Brand (Vengeance) - defensive
            if (specId == 581)
                emergencySpells.push_back(204021);

            break;
        }

        case CLASS_EVOKER:
        {
            // Tip the Scales - instant empowered cast
            emergencySpells.push_back(370553);

            // Dragonrage (Devastation) - burst
            if (specId == 1467)
                emergencySpells.push_back(375087);

            // Rewind (Preservation) - mass heal
            if (specId == 1468)
                emergencySpells.push_back(363534);

            // Ebon Might (Augmentation) - buff
            if (specId == 1473)
                emergencySpells.push_back(395152);

            // Obsidian Scales - defensive
            emergencySpells.push_back(363916);

            break;
        }
    }

    // Filter out spells the bot doesn't know
    ::std::vector<uint32> knownSpells;
    for (uint32 spellId : emergencySpells)
    {
        if (_bot->HasSpell(spellId))
        {
            // Also check if it's not on cooldown
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
            if (spellInfo && _bot->GetSpellHistory()->IsReady(spellInfo))
            {
                knownSpells.push_back(spellId);
            }
        }
    }

    return knownSpells;
}

uint32 ResourceManager::GetTotalResourceGenerated(ResourceType type)
{
    auto it = _totalGenerated.find(type);
    return (it != _totalGenerated.end()) ? it->second : 0;
}

uint32 ResourceManager::GetTotalResourceConsumed(ResourceType type)
{
    auto it = _totalConsumed.find(type);
    return (it != _totalConsumed.end()) ? it->second : 0;
}

float ResourceManager::GetAverageResourceUsage(ResourceType type)
{
    uint32 consumed = GetTotalResourceConsumed(type);
    uint32 updates = _updateCount.load();

    return updates > 0 ? static_cast<float>(consumed) / updates : 0.0f;
}

void ResourceManager::DumpResourceState()
{
    if (!_bot || !_bot->IsInWorld())
        return;

    TC_LOG_DEBUG("playerbot.resource", "=== Resource Manager Dump for {} ===", _bot->GetName());
    for (const auto& pair : _resources)
    {
        const ResourceInfo& info = pair.second;
        TC_LOG_DEBUG("playerbot.resource", "{}: {}/{} ({}%) - Regen: {}/sec",

                     static_cast<uint32>(pair.first), info.current, info.maximum,

                     static_cast<uint32>(info.GetPercent() * 100), info.regenRate);
    }

    if (_bot->GetClass() == CLASS_DEATH_KNIGHT)    {
        TC_LOG_DEBUG("playerbot.resource", "Runic Power: {}", _runicPower);
        for (uint32 i = 0; i < MAX_RUNES; ++i)
        {

            TC_LOG_DEBUG("playerbot.resource", "Rune {}: {} (cooldown: {}ms)",

                         i, _runes[i].available ? "Available" : "On cooldown", _runes[i].cooldownRemaining);
        }
    }
}

ResourceInfo ResourceManager::GetResourceInfo(ResourceType type)
{
    auto it = _resources.find(type);
    if (it != _resources.end())
    {
        return it->second;
    }
    return ResourceInfo();
}
void ResourceManager::UpdateResourceRegeneration(ResourceInfo& resource, uint32 diff)
{
    if (!resource.isRegenerated || resource.regenRate <= 0.0f)
        return;

    if (resource.current >= resource.maximum)        return;

    float regenAmount = resource.regenRate * (diff / 1000.0f);
    uint32 regenInt = static_cast<uint32>(regenAmount);
    if (regenInt > 0)
    {
        resource.Add(regenInt);
        resource.lastUpdate = GameTime::GetGameTimeMS();
    }}void ResourceManager::UpdateRunes(uint32 diff)
{
    for (uint32 i = 0; i < MAX_RUNES; ++i)
    {
        if (!_runes[i].available && _runes[i].cooldownRemaining > 0)

        {
        if (_runes[i].cooldownRemaining > diff)

            {
            _runes[i].cooldownRemaining -= diff;

            }

            else

            {

                _runes[i].cooldownRemaining = 0;

                _runes[i].available = true;
                }
        }    }
}

void ResourceManager::SyncWithPlayer()
{
    if (!_bot)        return;

    // Sync primary resource
    ResourceType primaryType = GetPrimaryResourceType();
    Powers primaryPower = GetPowerTypeForResource(primaryType);

    auto it = _resources.find(primaryType);
    if (it != _resources.end())
    {
        it->second.current = _bot->GetPower(primaryPower);
        it->second.maximum = _bot->GetMaxPower(primaryPower);    }
    else
    {
        // Create resource info if it doesn't exist
        _resources[primaryType] = ResourceInfo(primaryType,

                                               _bot->GetPower(primaryPower),

                                               _bot->GetMaxPower(primaryPower));
    }

    // Sync class-specific resources
    switch (_bot->GetClass())
    {
        case CLASS_ROGUE:
        case CLASS_DRUID: // In cat form
            // Combo points
    if (_bot->GetPower(POWER_COMBO_POINTS) != GetComboPoints())
            {

                SetResource(ResourceType::COMBO_POINTS, _bot->GetPower(POWER_COMBO_POINTS));

            }

            break;

        case CLASS_PALADIN:
            // Holy Power
    if (_bot->GetPower(POWER_HOLY_POWER) != GetHolyPower())
            {

                SetResource(ResourceType::HOLY_POWER, _bot->GetPower(POWER_HOLY_POWER));

            }

            break;

        case CLASS_MONK:
            // Chi
    if (_bot->GetPower(POWER_CHI) != GetChi())
            {

                SetResource(ResourceType::CHI, _bot->GetPower(POWER_CHI));

            }

            break;

        case CLASS_DEATH_KNIGHT:
            // Runic Power

            _runicPower = _bot->GetPower(POWER_RUNIC_POWER);
            break;
    }
}

void ResourceManager::InitializeClassResources()
{
    if (!_bot)
        return;

    ResourceType primaryType = GetPrimaryResourceType();
    Powers primaryPower = GetPowerTypeForResource(primaryType);

    // Initialize primary resource
    float regenRate = 0.0f;
    switch (primaryType)
    {
        case ResourceType::MANA:

            regenRate = ResourceCalculator::CalculateManaRegen(_bot);

            break;
        case ResourceType::ENERGY:

            regenRate = ResourceCalculator::CalculateEnergyRegen(_bot);

            break;
        case ResourceType::RAGE:

            regenRate = -ResourceCalculator::CalculateRageDecay(_bot); // Decay is negative regen
            break;
    }

    _resources[primaryType] = ResourceInfo(primaryType,

                                           _bot->GetPower(primaryPower),

                                           _bot->GetMaxPower(primaryPower),

                                           regenRate);

    // Initialize class-specific secondary resources
    switch (_bot->GetClass())
    {
        case CLASS_ROGUE:
        case CLASS_DRUID:

            _resources[ResourceType::COMBO_POINTS] = ResourceInfo(ResourceType::COMBO_POINTS, 0, 5);

            break;

        case CLASS_PALADIN:

            _resources[ResourceType::HOLY_POWER] = ResourceInfo(ResourceType::HOLY_POWER, 0, 3);

            break;

        case CLASS_MONK:

            _resources[ResourceType::CHI] = ResourceInfo(ResourceType::CHI, 0, 4);

            break;

        case CLASS_WARLOCK:

            _resources[ResourceType::SOUL_SHARDS] = ResourceInfo(ResourceType::SOUL_SHARDS, 0, 3);

            break;
    }
}

void ResourceManager::LoadSpellResourceCosts()
{
    // This could be enhanced to load spell costs from a database or cache
    // For now, we'll calculate them on-demand
}

ResourceType ResourceManager::GetPrimaryResourceType()
{
    if (!_bot)
        return ResourceType::MANA;
        switch (_bot->GetClass())
    {
        case CLASS_WARRIOR:

            return ResourceType::RAGE;
        case CLASS_PALADIN:
        case CLASS_PRIEST:
        case CLASS_MAGE:
        case CLASS_WARLOCK:

            return ResourceType::MANA;
        case CLASS_HUNTER:

            return ResourceType::FOCUS;
        case CLASS_ROGUE:
        case CLASS_MONK:

            return ResourceType::ENERGY;
        case CLASS_DEATH_KNIGHT:

            return ResourceType::RUNIC_POWER;
        case CLASS_SHAMAN:
        case CLASS_DRUID:

            return ResourceType::MANA;
        case CLASS_DEMON_HUNTER:

            return ResourceType::FURY;
        case CLASS_EVOKER:

            return ResourceType::ESSENCE;
        default:

            return ResourceType::MANA;
    }
}

Powers ResourceManager::GetPowerTypeForResource(ResourceType type)
{
    switch (type)
    {
        case ResourceType::MANA: return POWER_MANA;
        case ResourceType::RAGE: return POWER_RAGE;
        case ResourceType::FOCUS: return POWER_FOCUS;
        case ResourceType::ENERGY: return POWER_ENERGY;
        case ResourceType::COMBO_POINTS: return POWER_COMBO_POINTS;
        case ResourceType::RUNES: return POWER_RUNES;
        case ResourceType::RUNIC_POWER: return POWER_RUNIC_POWER;
        case ResourceType::HOLY_POWER: return POWER_HOLY_POWER;
        case ResourceType::CHI: return POWER_CHI;
        default: return POWER_MANA;
    }
}

ResourceType ResourceManager::GetResourceTypeForPower(Powers power)
{
    switch (power)
    {
        case POWER_MANA: return ResourceType::MANA;
        case POWER_RAGE: return ResourceType::RAGE;
        case POWER_FOCUS: return ResourceType::FOCUS;
        case POWER_ENERGY: return ResourceType::ENERGY;
        case POWER_COMBO_POINTS: return ResourceType::COMBO_POINTS;
        case POWER_RUNES: return ResourceType::RUNES;
        case POWER_RUNIC_POWER: return ResourceType::RUNIC_POWER;
        case POWER_HOLY_POWER: return ResourceType::HOLY_POWER;
        case POWER_CHI: return ResourceType::CHI;
        default: return ResourceType::MANA;
    }
}

uint32 ResourceManager::CalculateSpellResourceCost(uint32 spellId, ResourceType type)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo || !_bot)
        return 0;

    Powers requiredPower = GetPowerTypeForResource(type);

    // Check if spell uses the required power type and calculate cost
    for (SpellPowerEntry const* powerEntry : spellInfo->PowerCosts)
    {
        if (!powerEntry)

            continue;

        if (powerEntry->PowerType == requiredPower)
        {

            uint32 baseCost = powerEntry->ManaCost;

            uint32 percentCost = 0;

            // Calculate percentage-based cost
    if (powerEntry->PowerCostPct > 0.0f)

            {

                percentCost = uint32(powerEntry->PowerCostPct * _bot->GetMaxPower(requiredPower) / 100.0f);

            }


            return baseCost + percentCost;
        }
    }

    return 0;
}

bool ResourceManager::IsResourceTypeUsedBySpell(uint32 spellId, ResourceType type)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    Powers requiredPower = GetPowerTypeForResource(type);

    // Check if any of the spell's power costs match the required type
    for (SpellPowerEntry const* powerEntry : spellInfo->PowerCosts)
    {
        if (powerEntry && powerEntry->PowerType == requiredPower)

            return true;
    }

    return false;
}

// ResourceCalculator implementation

// Meyer's singleton accessors for DLL-safe static data
::std::unordered_map<uint32, uint32>& ResourceCalculator::GetManaCostCache()
{
    static ::std::unordered_map<uint32, uint32> manaCostCache;
    return manaCostCache;
}

::std::unordered_map<uint32, uint32>& ResourceCalculator::GetRageCostCache()
{
    static ::std::unordered_map<uint32, uint32> rageCostCache;
    return rageCostCache;
}

::std::unordered_map<uint32, uint32>& ResourceCalculator::GetEnergyCostCache()
{
    static ::std::unordered_map<uint32, uint32> energyCostCache;
    return energyCostCache;
}

::std::recursive_mutex& ResourceCalculator::GetCacheMutex()
{
    static ::std::recursive_mutex cacheMutex;
    return cacheMutex;
}

uint32 ResourceCalculator::CalculateManaCost(uint32 spellId, Player* caster)
{
    if (!spellId || !caster)
        return 0;

    ::std::lock_guard lock(GetCacheMutex());
    auto it = GetManaCostCache().find(spellId);
    if (it != GetManaCostCache().end())
        return it->second;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return 0;

    // Calculate mana cost using modern API
    uint32 cost = 0;
    for (SpellPowerEntry const* powerEntry : spellInfo->PowerCosts)
    {
        if (!powerEntry || powerEntry->PowerType != POWER_MANA)
            continue;

        cost = powerEntry->ManaCost;
        if (powerEntry->PowerCostPct > 0.0f)
        {
            cost += uint32(powerEntry->PowerCostPct * caster->GetMaxPower(POWER_MANA) / 100.0f);
        }
        break; // Use first mana cost found
    }

    if (cost == 0)
        return 0;
    GetManaCostCache()[spellId] = cost;
    return cost;
}

uint32 ResourceCalculator::CalculateRageCost(uint32 spellId, Player* caster)
{
    if (!spellId || !caster)
        return 0;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return 0;

    // Find rage cost in power costs array
    for (SpellPowerEntry const* powerEntry : spellInfo->PowerCosts)
    {
        if (powerEntry && powerEntry->PowerType == POWER_RAGE)
        {

            return powerEntry->ManaCost;
        }
    }

    return 0;
}

uint32 ResourceCalculator::CalculateEnergyCost(uint32 spellId, Player* caster)
{
    if (!spellId || !caster)
        return 0;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return 0;

    // Find energy cost in power costs array
    for (SpellPowerEntry const* powerEntry : spellInfo->PowerCosts)
    {
        if (powerEntry && powerEntry->PowerType == POWER_ENERGY)
        {

            return powerEntry->ManaCost;
        }
    }

    return 0;
}

uint32 ResourceCalculator::CalculateFocusCost(uint32 spellId, Player* caster)
{
    if (!spellId || !caster)
        return 0;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return 0;

    // Find focus cost in power costs array
    for (SpellPowerEntry const* powerEntry : spellInfo->PowerCosts)
    {
        if (powerEntry && powerEntry->PowerType == POWER_FOCUS)
        {

            return powerEntry->ManaCost;
        }
    }

    return 0;
}

float ResourceCalculator::CalculateManaRegen(Player* player)
{
    if (!player)
        return 0.0f;

    // WoW 12.0 mana regeneration formula based on intellect and haste
    // Base mana regen = intellect * coefficient * haste modifier
    float intellect = player->GetStat(STAT_INTELLECT);
    // Base regeneration scales with max mana pool size
    float maxManaScale = player->GetMaxPower(POWER_MANA) > 0 ?
        std::min(1.5f, static_cast<float>(player->GetMaxPower(POWER_MANA)) / 100000.0f) : 1.0f;

    // Base regeneration coefficient (varies by expansion, 12.0 uses ~0.02 per int)
    constexpr float INTELLECT_REGEN_COEFFICIENT = 0.02f;
    constexpr float BASE_MANA_REGEN_PER_SECOND = 5.0f;

    // Calculate base regen from intellect, scaled by mana pool
    float baseRegen = (BASE_MANA_REGEN_PER_SECOND + (intellect * INTELLECT_REGEN_COEFFICIENT)) * maxManaScale;

    // Apply haste bonus (increases mana regen tick rate)
    float hastePct = player->GetRatingBonusValue(CR_HASTE_SPELL);
    float hasteMultiplier = 1.0f + (hastePct / 100.0f);
    baseRegen *= hasteMultiplier;

    // Apply combat penalty - 12.0 has roughly 50% regen in combat for most specs
    bool inCombat = player->IsInCombat();
    float combatMultiplier = inCombat ? 0.5f : 1.0f;

    // Class-specific adjustments
    uint8 playerClass = player->GetClass();
    ChrSpecializationEntry const* spec = player->GetPrimarySpecializationEntry();
    uint32 specId = spec ? spec->ID : 0;

    switch (playerClass)
    {
        case CLASS_PRIEST:
            // Shadow priests have reduced mana concerns, Holy/Disc have regen mechanics
            if (specId == 256) // Holy
                combatMultiplier = inCombat ? 0.65f : 1.0f; // Holy Word mechanics
            else if (specId == 257) // Discipline
                combatMultiplier = inCombat ? 0.55f : 1.0f; // Atonement sustain
            break;

        case CLASS_PALADIN:
            // Holy paladins have excellent mana efficiency
            if (specId == 65) // Holy
                combatMultiplier = inCombat ? 0.60f : 1.0f;
            break;

        case CLASS_DRUID:
            // Restoration druids have HoT efficiency
            if (specId == 105) // Restoration
                combatMultiplier = inCombat ? 0.55f : 1.0f;
            break;

        case CLASS_SHAMAN:
            // Restoration shamans have Mana Tide/efficient healing
            if (specId == 264) // Restoration
                combatMultiplier = inCombat ? 0.60f : 1.0f;
            break;

        case CLASS_MAGE:
            // Mages have Arcane Intellect and Evocation for mana
            combatMultiplier = inCombat ? 0.45f : 1.0f;
            break;

        case CLASS_WARLOCK:
            // Warlocks use Life Tap/Drain Life for mana
            combatMultiplier = inCombat ? 0.40f : 1.0f;
            break;

        case CLASS_EVOKER:
            // Preservation evokers have good mana tools
            if (specId == 1468) // Preservation
                combatMultiplier = inCombat ? 0.55f : 1.0f;
            break;
    }

    baseRegen *= combatMultiplier;

    // Check for mana regeneration auras (like Innervate, Mana Tide Totem)
    if (player->HasAuraType(SPELL_AURA_MOD_POWER_REGEN_PERCENT))
    {
        int32 regenPctBonus = player->GetTotalAuraModifier(SPELL_AURA_MOD_POWER_REGEN_PERCENT);
        baseRegen *= (1.0f + regenPctBonus / 100.0f);
    }

    // Check for flat mana regeneration auras
    if (player->HasAuraType(SPELL_AURA_MOD_POWER_REGEN))
    {
        int32 flatRegen = player->GetTotalAuraModifier(SPELL_AURA_MOD_POWER_REGEN);
        baseRegen += static_cast<float>(flatRegen) / 5.0f; // Aura is per 5 seconds
    }

    return std::max(0.0f, baseRegen);
}

float ResourceCalculator::CalculateEnergyRegen(Player* player)
{
    if (!player)
        return 0.0f;

    // WoW 12.0 energy regeneration: base 10/second + haste scaling
    constexpr float BASE_ENERGY_REGEN = 10.0f;

    float baseRegen = BASE_ENERGY_REGEN;

    // Haste directly increases energy regeneration rate
    float hastePct = player->GetRatingBonusValue(CR_HASTE_MELEE);
    float hasteMultiplier = 1.0f + (hastePct / 100.0f);
    baseRegen *= hasteMultiplier;

    // Class-specific energy regeneration modifications
    uint8 playerClass = player->GetClass();
    ChrSpecializationEntry const* spec = player->GetPrimarySpecializationEntry();
    uint32 specId = spec ? spec->ID : 0;

    switch (playerClass)
    {
        case CLASS_ROGUE:
        {
            // Rogues have various energy regeneration talents and passives
            // Combat Potency (Outlaw) - chance to gain energy on off-hand hits
            // Relentless Strikes - finishing moves restore energy
            // Vigor talent increases max energy and regen

            // Check for Vigor or similar max energy increase
            uint32 maxEnergy = player->GetMaxPower(POWER_ENERGY);
            if (maxEnergy > 100)
            {
                // Vigor-like talent detected, slight regen boost
                baseRegen *= 1.1f;
            }

            // Spec adjustments
            if (specId == 259) // Assassination
            {
                // Venomous Wounds passive - poison ticks can restore energy
                baseRegen *= 1.15f;
            }
            else if (specId == 260) // Outlaw
            {
                // Combat Potency and Blade Flurry synergy
                baseRegen *= 1.20f;
            }
            else if (specId == 261) // Subtlety
            {
                // Shadow Dance and Symbols of Death synergy
                baseRegen *= 1.10f;
            }
            break;
        }

        case CLASS_DRUID:
        {
            // Feral druids in cat form
            if (specId == 103) // Feral
            {
                // Omen of Clarity procs, Tiger's Fury
                baseRegen *= 1.15f;

                // Check if Tiger's Fury is active (grants 50 energy instantly + regen bonus)
                if (player->HasAura(5217)) // Tiger's Fury
                    baseRegen *= 1.15f;
            }
            break;
        }

        case CLASS_MONK:
        {
            // Windwalker/Brewmaster energy usage
            if (specId == 269) // Windwalker
            {
                // Ascension talent increases max chi and energy regen
                // Energizing Elixir can restore energy
                baseRegen *= 1.10f;
            }
            else if (specId == 268) // Brewmaster
            {
                // Brewmasters use energy for Keg Smash etc
                baseRegen *= 1.05f;
            }
            break;
        }
    }

    // Check for energy regeneration auras
    if (player->HasAuraType(SPELL_AURA_MOD_POWER_REGEN_PERCENT))
    {
        int32 regenPctBonus = player->GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_POWER_REGEN_PERCENT, POWER_ENERGY);
        if (regenPctBonus > 0)
            baseRegen *= (1.0f + regenPctBonus / 100.0f);
    }

    return std::max(0.0f, baseRegen);
}

float ResourceCalculator::CalculateRageDecay(Player* player)
{
    if (!player)
        return 0.0f;

    // WoW 12.0 rage decay mechanics
    // Rage decays at ~1 rage per second out of combat after a delay
    // No decay while in combat

    // Check combat status - no decay in combat
    if (player->IsInCombat())
        return 0.0f;

    // Base decay rate (approximately 1 rage per second out of combat)
    constexpr float BASE_RAGE_DECAY = 1.0f;
    float decayRate = BASE_RAGE_DECAY;

    uint8 playerClass = player->GetClass();
    ChrSpecializationEntry const* spec = player->GetPrimarySpecializationEntry();
    uint32 specId = spec ? spec->ID : 0;

    switch (playerClass)
    {
        case CLASS_WARRIOR:
        {
            // Warrior rage decay modifications
            // Some talents/abilities can reduce rage decay

            if (specId == 71) // Arms
            {
                // Arms has some rage generation passives
                // War Machine talent reduces decay
                decayRate *= 0.8f;
            }
            else if (specId == 72) // Fury
            {
                // Fury generates rage from auto-attacks faster
                // Faster decay to compensate
                decayRate *= 1.0f;
            }
            else if (specId == 73) // Protection
            {
                // Protection warriors need rage for active mitigation
                // Slower decay to maintain defensive capability
                decayRate *= 0.7f;
            }

            // Check for Berserker Rage or similar rage retention
            if (player->HasAura(18499)) // Berserker Rage
                decayRate = 0.0f;

            // Check for Battle Shout buff (can affect rage generation)
            if (player->HasAura(6673)) // Battle Shout
                decayRate *= 0.9f;

            break;
        }

        case CLASS_DRUID:
        {
            // Druid bear form rage
            if (specId == 104) // Guardian
            {
                // Guardian druids use rage for Ironfur/Frenzied Regen
                // Slower decay to maintain defensive options
                decayRate *= 0.6f;

                // Rage of the Sleeper and similar abilities
                if (player->HasAuraType(SPELL_AURA_MOD_SHAPESHIFT))
                {
                    // In bear form - check for specific rage retention effects
                    uint32 currentRage = player->GetPower(POWER_RAGE);
                    if (currentRage < 20)
                        decayRate *= 0.5f; // Minimal decay at low rage
                }
            }
            else if (specId == 103) // Feral
            {
                // Feral cat form uses energy, not rage
                // But can shift to bear for rage
                decayRate *= 0.9f;
            }
            break;
        }

        case CLASS_DEMON_HUNTER:
        {
            // Vengeance Demon Hunters use Fury (similar to rage)
            if (specId == 581) // Vengeance
            {
                decayRate *= 0.8f; // Slower decay for tank spec
            }
            break;
        }
    }

    // Check for rage decay reduction auras
    if (player->HasAuraType(SPELL_AURA_MOD_POWER_REGEN_PERCENT))
    {
        // Some auras reduce rage decay
        int32 decayMod = player->GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_POWER_REGEN_PERCENT, POWER_RAGE);
        if (decayMod < 0) // Negative means reduced decay
            decayRate *= (1.0f + decayMod / 100.0f);
    }

    // Clamp to reasonable values
    return std::max(0.0f, std::min(decayRate, 3.0f));
}

bool ResourceCalculator::IsResourceEfficientSpell(uint32 spellId, Player* caster)
{
    float efficiency = CalculateResourceEfficiency(spellId, caster);
    return efficiency > 1.0f; // Arbitrary threshold
}

float ResourceCalculator::CalculateResourceEfficiency(uint32 spellId, Player* caster)
{
    if (!spellId || !caster)
        return 0.0f;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return 0.0f;

    // Calculate the resource cost of this spell
    ::std::vector<SpellPowerCost> costs = spellInfo->CalcPowerCost(caster, spellInfo->GetSchoolMask());

    uint32 totalResourceCost = 0;
    for (const SpellPowerCost& cost : costs)
    {
        if (cost.Amount > 0)
            totalResourceCost += cost.Amount;
    }

    if (totalResourceCost == 0)
        return 100.0f; // Free spells have infinite efficiency (capped at 100)

    // Calculate the expected damage/healing output
    float expectedOutput = 0.0f;
    bool isDamageSpell = false;
    bool isHealingSpell = false;

    for (SpellEffectInfo const& effect : spellInfo->GetEffects())
    {
        if (!effect.IsEffect())
            continue;

        switch (effect.Effect)
        {
            case SPELL_EFFECT_SCHOOL_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
            case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
            case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
            {
                isDamageSpell = true;
                int32 baseDamage = effect.CalcValue(caster, nullptr, nullptr);
                float spellPower = static_cast<float>(caster->SpellBaseDamageBonusDone(spellInfo->GetSchoolMask()));
                float bonusCoeff = effect.BonusCoefficient > 0.0f ? effect.BonusCoefficient : 0.5f;
                expectedOutput += baseDamage + (spellPower * bonusCoeff);
                break;
            }

            case SPELL_EFFECT_HEAL:
            case SPELL_EFFECT_HEAL_MECHANICAL:
            {
                isHealingSpell = true;
                int32 baseHealing = effect.CalcValue(caster, nullptr, nullptr);
                float healPower = static_cast<float>(caster->SpellBaseHealingBonusDone(spellInfo->GetSchoolMask()));
                float bonusCoeff = effect.BonusCoefficient > 0.0f ? effect.BonusCoefficient : 0.5f;
                expectedOutput += baseHealing + (healPower * bonusCoeff);
                break;
            }

            case SPELL_EFFECT_APPLY_AURA:
            {
                // Check for DoT/HoT effects
                switch (effect.ApplyAuraName)
                {
                    case SPELL_AURA_PERIODIC_DAMAGE:
                    {
                        isDamageSpell = true;
                        int32 tickDamage = effect.CalcValue(caster, nullptr, nullptr);
                        uint32 duration = spellInfo->GetMaxDuration();
                        uint32 amplitude = effect.ApplyAuraPeriod > 0 ? effect.ApplyAuraPeriod : 3000;
                        uint32 ticks = duration / amplitude;
                        float spellPower = static_cast<float>(caster->SpellBaseDamageBonusDone(spellInfo->GetSchoolMask()));
                        float bonusCoeff = effect.BonusCoefficient > 0.0f ? effect.BonusCoefficient : 0.1f;
                        expectedOutput += (tickDamage + (spellPower * bonusCoeff)) * ticks;
                        break;
                    }

                    case SPELL_AURA_PERIODIC_HEAL:
                    {
                        isHealingSpell = true;
                        int32 tickHealing = effect.CalcValue(caster, nullptr, nullptr);
                        uint32 duration = spellInfo->GetMaxDuration();
                        uint32 amplitude = effect.ApplyAuraPeriod > 0 ? effect.ApplyAuraPeriod : 3000;
                        uint32 ticks = duration / amplitude;
                        float healPower = static_cast<float>(caster->SpellBaseHealingBonusDone(spellInfo->GetSchoolMask()));
                        float bonusCoeff = effect.BonusCoefficient > 0.0f ? effect.BonusCoefficient : 0.1f;
                        expectedOutput += (tickHealing + (healPower * bonusCoeff)) * ticks;
                        break;
                    }

                    default:
                        break;
                }
                break;
            }

            default:
                break;
        }
    }

    // If no damage/healing effect found, check if it's a utility spell
    if (expectedOutput <= 0.0f)
    {
        // Utility spells (buffs, CC, etc.) have moderate efficiency
        return 1.0f;
    }

    // Calculate efficiency as output per resource point
    float efficiency = expectedOutput / static_cast<float>(totalResourceCost);

    // Apply versatility bonus if applicable
    if (isDamageSpell)
    {
        float versatility = caster->GetRatingBonusValue(CR_VERSATILITY_DAMAGE_DONE);
        efficiency *= (1.0f + versatility / 100.0f);
    }
    else if (isHealingSpell)
    {
        float versatility = caster->GetRatingBonusValue(CR_VERSATILITY_DAMAGE_DONE); // Same stat for healing
        efficiency *= (1.0f + versatility / 100.0f);
    }

    // Normalize to a reasonable scale (baseline of 1.0 for average spells)
    // Most spells do 100-500 damage/heal per resource point at max level
    constexpr float NORMALIZATION_FACTOR = 200.0f;
    efficiency /= NORMALIZATION_FACTOR;

    return std::max(0.0f, std::min(efficiency, 100.0f));
}

uint32 ResourceCalculator::GetOptimalResourceLevel(ResourceType type, Player* player)
{
    if (!player)
        return 0;

    // Get the max resource for percentage calculations
    Powers powerType = POWER_MANA;
    switch (type)
    {
        case ResourceType::MANA: powerType = POWER_MANA; break;
        case ResourceType::RAGE: powerType = POWER_RAGE; break;
        case ResourceType::ENERGY: powerType = POWER_ENERGY; break;
        case ResourceType::FOCUS: powerType = POWER_FOCUS; break;
        case ResourceType::RUNIC_POWER: powerType = POWER_RUNIC_POWER; break;
        case ResourceType::HOLY_POWER: powerType = POWER_HOLY_POWER; break;
        case ResourceType::CHI: powerType = POWER_CHI; break;
        case ResourceType::COMBO_POINTS: powerType = POWER_COMBO_POINTS; break;
        default: break;
    }

    uint32 maxResource = player->GetMaxPower(powerType);
    if (maxResource == 0)
        return 0;

    uint8 playerClass = player->GetClass();
    ChrSpecializationEntry const* spec = player->GetPrimarySpecializationEntry();
    uint32 specId = spec ? spec->ID : 0;

    // Calculate optimal percentage based on resource type and class/spec
    float optimalPercent = 0.5f; // Default 50%

    switch (type)
    {
        case ResourceType::MANA:
        {
            // Healers need higher mana reserves, DPS casters can go lower
            switch (playerClass)
            {
                case CLASS_PRIEST:
                    optimalPercent = (specId == 258) ? 0.25f : 0.40f; // Shadow vs Holy/Disc
                    break;
                case CLASS_PALADIN:
                    optimalPercent = (specId == 65) ? 0.35f : 0.20f; // Holy vs Ret/Prot
                    break;
                case CLASS_DRUID:
                    optimalPercent = (specId == 105) ? 0.40f : 0.25f; // Resto vs others
                    break;
                case CLASS_SHAMAN:
                    optimalPercent = (specId == 264) ? 0.40f : 0.25f; // Resto vs Ele/Enh
                    break;
                case CLASS_MAGE:
                    optimalPercent = (specId == 62) ? 0.45f : 0.30f; // Arcane vs Frost/Fire
                    break;
                case CLASS_WARLOCK:
                    optimalPercent = 0.25f; // Life Tap can restore mana
                    break;
                case CLASS_EVOKER:
                    optimalPercent = (specId == 1468) ? 0.35f : 0.25f; // Preservation vs others
                    break;
                default:
                    optimalPercent = 0.30f;
                    break;
            }
            break;
        }

        case ResourceType::RAGE:
        {
            // Tanks need rage for active mitigation, DPS can spend freely
            switch (playerClass)
            {
                case CLASS_WARRIOR:
                    if (specId == 73) // Protection
                        optimalPercent = 0.40f; // Need rage for Shield Block/Ignore Pain
                    else if (specId == 72) // Fury
                        optimalPercent = 0.20f; // Spend freely, generates fast
                    else // Arms
                        optimalPercent = 0.30f;
                    break;
                case CLASS_DRUID:
                    if (specId == 104) // Guardian
                        optimalPercent = 0.45f; // Need for Ironfur/Frenzied Regen
                    else
                        optimalPercent = 0.25f;
                    break;
                default:
                    optimalPercent = 0.30f;
                    break;
            }
            break;
        }

        case ResourceType::ENERGY:
        {
            // Energy users want to pool for burst windows
            switch (playerClass)
            {
                case CLASS_ROGUE:
                    if (specId == 259) // Assassination
                        optimalPercent = 0.50f; // Pool for Envenom windows
                    else if (specId == 260) // Outlaw
                        optimalPercent = 0.35f; // Blade Flurry cleave
                    else // Subtlety
                        optimalPercent = 0.55f; // Pool for Shadow Dance
                    break;
                case CLASS_DRUID:
                    if (specId == 103) // Feral
                        optimalPercent = 0.50f; // Pool for Tiger's Fury
                    else
                        optimalPercent = 0.40f;
                    break;
                case CLASS_MONK:
                    if (specId == 269) // Windwalker
                        optimalPercent = 0.45f; // Chi generation
                    else if (specId == 268) // Brewmaster
                        optimalPercent = 0.35f;
                    else
                        optimalPercent = 0.40f;
                    break;
                default:
                    optimalPercent = 0.40f;
                    break;
            }
            break;
        }

        case ResourceType::FOCUS:
        {
            // Hunters pool focus for burst
            optimalPercent = 0.45f;
            if (specId == 253) // Beast Mastery
                optimalPercent = 0.40f; // Faster generation
            else if (specId == 254) // Marksmanship
                optimalPercent = 0.50f; // Pool for Aimed Shot
            else if (specId == 255) // Survival
                optimalPercent = 0.35f;
            break;
        }

        case ResourceType::RUNIC_POWER:
        {
            // Death Knights use RP for Death Strike (tank) or damage spenders
            if (specId == 250) // Blood
                optimalPercent = 0.50f; // Need RP for Death Strike healing
            else if (specId == 251) // Frost
                optimalPercent = 0.35f; // Frost Strike spam
            else if (specId == 252) // Unholy
                optimalPercent = 0.40f; // Death Coil/Epidemic
            else
                optimalPercent = 0.40f;
            break;
        }

        case ResourceType::HOLY_POWER:
        {
            // Paladins want 3+ for spenders
            optimalPercent = 0.66f; // 2/3 holy power minimum
            break;
        }

        case ResourceType::CHI:
        {
            // Monks want chi for spenders
            optimalPercent = 0.50f; // 2/4 chi
            break;
        }

        case ResourceType::COMBO_POINTS:
        {
            // Rogues/Ferals want 5+ for finishers (or 4+ with Deeper Stratagem)
            optimalPercent = 0.80f; // 4/5 combo points
            break;
        }

        default:
            optimalPercent = 0.50f;
            break;
    }

    // Adjust based on combat status
    if (player->IsInCombat())
    {
        // In combat, be more aggressive with resource usage for damage/healing
        optimalPercent *= 0.8f;
    }

    return static_cast<uint32>(maxResource * optimalPercent);
}

uint32 ResourceCalculator::GetComboPointsFromSpell(uint32 spellId)
{
    if (!spellId)
        return 0;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return 0;

    uint32 comboPoints = 0;

    // Check spell effects for combo point generation
    for (SpellEffectInfo const& effect : spellInfo->GetEffects())
    {
        if (!effect.IsEffect())
            continue;

        // SPELL_EFFECT_ENERGIZE can grant combo points
        if (effect.Effect == SPELL_EFFECT_ENERGIZE || effect.Effect == SPELL_EFFECT_ENERGIZE_PCT)
        {
            if (effect.MiscValue == POWER_COMBO_POINTS)
            {
                comboPoints += effect.CalcValue(nullptr, nullptr, nullptr);
            }
        }

        // Note: In WoW 12.0, combo point generation is typically handled
        // via SPELL_EFFECT_ENERGIZE rather than aura effects
    }

    // If no energize effect found, check for known combo point builders
    if (comboPoints == 0)
    {
        // Rogue abilities
        switch (spellId)
        {
            // Rogue combo point builders (typical 1 CP)
            case 1752:   // Sinister Strike
            case 8676:   // Ambush
            case 703:    // Garrote
            case 1784:   // Stealth opener abilities
            case 185763: // Pistol Shot (Outlaw)
            case 196819: // Eviscerate (consumes, but checked for reference)
            case 51723:  // Fan of Knives
            case 5938:   // Shiv
            case 114014: // Shuriken Toss
            case 315496: // Slice and Dice refresh
            case 200758: // Gloomblade (Subtlety)
            case 185438: // Shadowstrike (Subtlety)
            case 121411: // Crimson Tempest (AoE - consumes)
                comboPoints = 1;
                break;

            // 2 CP builders
            case 5374:   // Mutilate (Assassination) - 2 CP
            case 245388: // Mutilate with Blindside
                comboPoints = 2;
                break;

            // Variable CP - Marked for Death grants 5
            case 137619: // Marked for Death
                comboPoints = 5;
                break;

            // Shadow Dance doesn't generate but enables Shadowstrike
            case 185313: // Shadow Dance
                comboPoints = 0;
                break;

            default:
                // Check if it's a finishing move (consumes CP, not generates)
                // Eviscerate, Envenom, Kidney Shot, etc.
                for (SpellPowerEntry const* powerEntry : spellInfo->PowerCosts)
                {
                    if (powerEntry && powerEntry->PowerType == POWER_COMBO_POINTS)
                    {
                        // This spell CONSUMES combo points
                        return 0;
                    }
                }
                // Unknown spell - assume 1 CP if it's a Rogue ability with damage
                for (SpellEffectInfo const& effect : spellInfo->GetEffects())
                {
                    if (effect.IsEffect(SPELL_EFFECT_SCHOOL_DAMAGE) || effect.IsEffect(SPELL_EFFECT_WEAPON_DAMAGE))
                    {
                        comboPoints = 1;
                        break;
                    }
                }
                break;
        }
    }

    // Feral Druid combo point builders
    if (comboPoints == 0)
    {
        switch (spellId)
        {
            case 5221:   // Shred
            case 1822:   // Rake
            case 106830: // Thrash (Feral)
            case 202028: // Brutal Slash
            case 106785: // Swipe (Cat)
            case 155625: // Moonfire (Feral talent)
                comboPoints = 1;
                break;
        }
    }

    return std::min(comboPoints, 5u); // Cap at 5 (or 6 with Deeper Stratagem)
}

uint32 ResourceCalculator::GetHolyPowerFromSpell(uint32 spellId)
{
    if (!spellId)
        return 0;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return 0;

    uint32 holyPower = 0;

    // Check spell effects for holy power generation
    for (SpellEffectInfo const& effect : spellInfo->GetEffects())
    {
        if (!effect.IsEffect())
            continue;

        // SPELL_EFFECT_ENERGIZE can grant holy power
        if (effect.Effect == SPELL_EFFECT_ENERGIZE || effect.Effect == SPELL_EFFECT_ENERGIZE_PCT)
        {
            if (effect.MiscValue == POWER_HOLY_POWER)
            {
                holyPower += effect.CalcValue(nullptr, nullptr, nullptr);
            }
        }
    }

    // If no energize effect found, check known Paladin Holy Power builders
    if (holyPower == 0)
    {
        switch (spellId)
        {
            // ===== Retribution Holy Power Generators =====
            case 35395:  // Crusader Strike - 1 HP
            case 53600:  // Shield of the Righteous (Protection) - consumes
            case 184575: // Blade of Justice - 2 HP
            case 255937: // Wake of Ashes - 3 HP
            case 267798: // Execution Sentence - 0 (spender)
            case 343721: // Final Reckoning - 0 (spender)
            {
                if (spellId == 35395) holyPower = 1;      // Crusader Strike
                else if (spellId == 184575) holyPower = 2; // Blade of Justice
                else if (spellId == 255937) holyPower = 3; // Wake of Ashes
                break;
            }

            // Hammer of Wrath - 1 HP (execute phase)
            case 24275:
                holyPower = 1;
                break;

            // Judgment can generate HP with talent
            case 20271:  // Judgment
                holyPower = 1; // With Highlord's Judgment talent
                break;

            // Templar's Verdict / Divine Storm (spenders - consume 3 HP)
            case 85256:  // Templar's Verdict
            case 53385:  // Divine Storm
            case 383328: // Final Verdict
            case 231832: // Blade of Wrath
                holyPower = 0; // Spenders
                break;

            // ===== Protection Holy Power Generators =====
            case 31935:  // Avenger's Shield - 0 but can grant with talent
                holyPower = 1; // First Avenger talent
                break;

            case 275779: // Judgment (Protection)
                holyPower = 1;
                break;

            case 62124:  // Hand of Reckoning (taunt)
                holyPower = 0;
                break;

            // ===== Holy Holy Power Generators =====
            case 20473:  // Holy Shock - 1 HP
                holyPower = 1;
                break;

            case 85222:  // Light of Dawn (spender)
            case 53652:  // Beacon of Light procs
            case 82326:  // Holy Light
            case 19750:  // Flash of Light
                holyPower = 0;
                break;

            case 275773: // Hammer of Wrath (Holy)
                holyPower = 1;
                break;

            // Crusader Strike (all specs) - 1 HP
            default:
            {
                // Check if it's a spender (costs holy power)
                for (SpellPowerEntry const* powerEntry : spellInfo->PowerCosts)
                {
                    if (powerEntry && powerEntry->PowerType == POWER_HOLY_POWER)
                    {
                        return 0; // This is a spender
                    }
                }
                break;
            }
        }
    }

    return std::min(holyPower, 5u); // Cap at 5
}

uint32 ResourceCalculator::GetChiFromSpell(uint32 spellId)
{
    if (!spellId)
        return 0;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return 0;

    uint32 chi = 0;

    // Check spell effects for chi generation
    for (SpellEffectInfo const& effect : spellInfo->GetEffects())
    {
        if (!effect.IsEffect())
            continue;

        // SPELL_EFFECT_ENERGIZE can grant chi
        if (effect.Effect == SPELL_EFFECT_ENERGIZE || effect.Effect == SPELL_EFFECT_ENERGIZE_PCT)
        {
            if (effect.MiscValue == POWER_CHI)
            {
                chi += effect.CalcValue(nullptr, nullptr, nullptr);
            }
        }
    }

    // If no energize effect found, check known Monk chi builders
    if (chi == 0)
    {
        switch (spellId)
        {
            // ===== Windwalker Chi Generators =====
            case 100784: // Blackout Kick - SPENDER (costs 1 chi)
                chi = 0;
                break;

            case 107428: // Rising Sun Kick - SPENDER (costs 2 chi)
                chi = 0;
                break;

            case 113656: // Fists of Fury - SPENDER (costs 3 chi)
                chi = 0;
                break;

            case 100780: // Tiger Palm - 2 chi (Windwalker)
                chi = 2;
                break;

            case 115098: // Chi Wave - 0 chi (talent, no resource)
                chi = 0;
                break;

            case 117952: // Crackling Jade Lightning - 0 chi (channel)
                chi = 0;
                break;

            case 101546: // Spinning Crane Kick - SPENDER (costs 2 chi)
                chi = 0;
                break;

            case 137639: // Storm, Earth, and Fire - 0 (cooldown)
                chi = 0;
                break;

            case 152175: // Whirling Dragon Punch - SPENDER
                chi = 0;
                break;

            case 115151: // Renewing Mist (Mistweaver) - 0 chi
            case 116670: // Uplift (Mistweaver) - 0 chi
            case 191837: // Essence Font (Mistweaver) - 0 chi
                chi = 0;
                break;

            // Expel Harm - generates chi for Windwalker
            case 322101: // Expel Harm
                chi = 1;
                break;

            // Chi Burst talent - generates 1 chi
            case 123986: // Chi Burst
                chi = 1;
                break;

            // ===== Brewmaster Chi Generators =====
            case 121253: // Keg Smash - 2 chi
                chi = 2;
                break;

            case 115181: // Breath of Fire - 0 chi (spender)
                chi = 0;
                break;

            case 205523: // Blackout Strike (Brewmaster) - 1 chi
                chi = 1;
                break;

            case 322507: // Celestial Brew - SPENDER
            case 115203: // Fortifying Brew - 0 (defensive CD)
            case 115176: // Zen Meditation - 0 (defensive CD)
                chi = 0;
                break;

            // Tiger Palm for Brewmaster generates less chi
            case 100780 + 1: // Placeholder for Brewmaster Tiger Palm variant
                chi = 1;
                break;

            // ===== Mistweaver =====
            // Mistweavers don't use chi in current WoW

            // Energizing Elixir (talent) - restores chi
            case 115288:
                chi = 2;
                break;

            default:
            {
                // Check if it's a spender (costs chi)
                for (SpellPowerEntry const* powerEntry : spellInfo->PowerCosts)
                {
                    if (powerEntry && powerEntry->PowerType == POWER_CHI)
                    {
                        return 0; // This is a spender
                    }
                }
                break;
            }
        }
    }

    return std::min(chi, 6u); // Cap at 6 (with Ascension talent max is 6)
}

void ResourceCalculator::CacheSpellResourceCost(uint32 spellId)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return;

    ::std::lock_guard lock(GetCacheMutex());

    // Cache costs for each power type the spell uses
    for (SpellPowerEntry const* powerEntry : spellInfo->PowerCosts)
    {
        if (!powerEntry)

            continue;

        switch (powerEntry->PowerType)
        {

            case POWER_MANA:

                GetManaCostCache()[spellId] = powerEntry->ManaCost;

                break;

            case POWER_RAGE:

                GetRageCostCache()[spellId] = powerEntry->ManaCost;

                break;

            case POWER_ENERGY:

                GetEnergyCostCache()[spellId] = powerEntry->ManaCost;

                break;

            default:

                break;
        }
    }
}

// ResourceMonitor implementation

ResourceMonitor& ResourceMonitor::Instance()
{
    static ResourceMonitor instance;
    return instance;
}

void ResourceMonitor::RecordResourceUsage(uint32 botGuid, ResourceType type, uint32 amount)
{
    ::std::lock_guard lock(_dataMutex);
    _botResourceData[botGuid][type].totalUsed += amount;
    _botResourceData[botGuid][type].sampleCount++;
}

void ResourceMonitor::RecordResourceWaste(uint32 botGuid, ResourceType type, uint32 amount)
{
    ::std::lock_guard lock(_dataMutex);
    _botResourceData[botGuid][type].totalWasted += amount;
}

void ResourceMonitor::RecordResourceStarvation(uint32 botGuid, ResourceType type, uint32 duration)
{
    ::std::lock_guard lock(_dataMutex);
    _botResourceData[botGuid][type].starvationTime += duration;
}

float ResourceMonitor::GetAverageResourceUsage(ResourceType type)
{
    ::std::lock_guard lock(_dataMutex);

    uint32 totalUsed = 0;
    uint32 totalSamples = 0;

    for (const auto& botPair : _botResourceData)
    {
        auto typeIt = botPair.second.find(type);
        if (typeIt != botPair.second.end())
        {

            totalUsed += typeIt->second.totalUsed;

            totalSamples += typeIt->second.sampleCount;
        }
    }

    return totalSamples > 0 ? static_cast<float>(totalUsed) / totalSamples : 0.0f;
}

float ResourceMonitor::GetResourceWasteRate(ResourceType type)
{
    ::std::lock_guard lock(_dataMutex);

    uint32 totalUsed = 0;
    uint32 totalWasted = 0;

    for (const auto& botPair : _botResourceData)
    {
        auto typeIt = botPair.second.find(type);
        if (typeIt != botPair.second.end())
        {

            totalUsed += typeIt->second.totalUsed;

            totalWasted += typeIt->second.totalWasted;
        }
    }

    uint32 total = totalUsed + totalWasted;
    return total > 0 ? static_cast<float>(totalWasted) / total : 0.0f;
}

uint32 ResourceMonitor::GetResourceStarvationTime(ResourceType type)
{
    ::std::lock_guard lock(_dataMutex);

    uint32 totalStarvation = 0;
    for (const auto& botPair : _botResourceData)
    {
        auto typeIt = botPair.second.find(type);
        if (typeIt != botPair.second.end())
        {

            totalStarvation += typeIt->second.starvationTime;
        }
    }

    return totalStarvation;
}

::std::vector<::std::string> ResourceMonitor::GetResourceOptimizationSuggestions(uint32 botGuid)
{
    ::std::vector<::std::string> suggestions;

    ::std::lock_guard lock(_dataMutex);
    auto botIt = _botResourceData.find(botGuid);
    if (botIt == _botResourceData.end())
        return suggestions;

    for (const auto& resourcePair : botIt->second)
    {
        const ResourceUsageData& data = resourcePair.second;

        if (data.totalWasted > data.totalUsed * 0.2f) // 20% waste threshold
        {

            suggestions.push_back("Reduce resource waste for " + ::std::to_string(static_cast<uint32>(resourcePair.first)));
        }

        if (data.starvationTime > 10000) // 10 seconds of starvation
        {

            suggestions.push_back("Improve resource management for " + ::std::to_string(static_cast<uint32>(resourcePair.first)));
        }
    }

    return suggestions;
}

} // namespace Playerbot
