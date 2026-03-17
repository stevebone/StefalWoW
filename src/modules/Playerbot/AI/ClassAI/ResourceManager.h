/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Player.h"
#include <unordered_map>
#include <memory>
#include <atomic>
#include <mutex>
#include <vector>
#include "GameTime.h"

namespace Playerbot
{

// Types of resources that classes use
enum class ResourceType : uint8
{
    MANA = 0,
    RAGE = 1,
    FOCUS = 2,
    ENERGY = 3,
    COMBO_POINTS = 4,
    RUNES = 5,
    RUNIC_POWER = 6,
    SOUL_SHARDS = 7,
    LUNAR_POWER = 8,
    HOLY_POWER = 9,
    MAELSTROM = 10,
    CHI = 11,
    INSANITY = 12,
    BURNING_EMBERS = 13,
    DEMONIC_FURY = 14,
    ARCANE_CHARGES = 15,
    FURY = 16,
    PAIN = 17,
    ESSENCE = 18
};

// Resource information for tracking
struct ResourceInfo
{
    ResourceType type;
    uint32 current;
    uint32 maximum;
    float regenRate;        // Per second regeneration
    uint32 lastUpdate;      // Last update timestamp
    bool isRegenerated;     // Whether this resource regenerates over time

    ResourceInfo() : type(ResourceType::MANA), current(0), maximum(0),
                     regenRate(0.0f), lastUpdate(0), isRegenerated(true) {}

    ResourceInfo(ResourceType t, uint32 cur, uint32 max, float regen = 0.0f)
        : type(t), current(cur), maximum(max), regenRate(regen),
          lastUpdate(GameTime::GetGameTimeMS()), isRegenerated(regen > 0.0f) {}

    // Get resource as percentage (0.0 to 1.0)
    float GetPercent() const
    {
        return maximum > 0 ? static_cast<float>(current) / maximum : 0.0f;
    }

    // Check if we have enough of this resource
    bool HasEnough(uint32 amount) const
    {
        return current >= amount;
    }

    // Consume resource (returns actual amount consumed)
    uint32 Consume(uint32 amount)
    {
        uint32 consumed = ::std::min(amount, current);
        current -= consumed;
        return consumed;
    }

    // Add resource (returns actual amount added)
    uint32 Add(uint32 amount)
    {
        uint32 maxAddable = maximum - current;
        uint32 added = ::std::min(amount, maxAddable);
        current += added;
        return added;
    }
};

// Class for managing bot resources (mana, energy, rage, etc.)
class TC_GAME_API ResourceManager
{
public:
    explicit ResourceManager(Player* bot);
    ~ResourceManager() = default;

    // Core interface
    void Update(uint32 diff);
    void Initialize();

    // Resource queries
    bool HasEnoughResource(uint32 spellId);
    bool HasEnoughResource(ResourceType type, uint32 amount);
    uint32 GetResource(ResourceType type);
    uint32 GetMaxResource(ResourceType type);
    float GetResourcePercent(ResourceType type);

    // Resource management
    void ConsumeResource(uint32 spellId);
    void ConsumeResource(ResourceType type, uint32 amount);
    void AddResource(ResourceType type, uint32 amount);
    void SetResource(ResourceType type, uint32 amount);
    void SetMaxResource(ResourceType type, uint32 amount);

    // Resource prediction
    uint32 GetResourceIn(ResourceType type, uint32 timeMs);
    bool WillHaveEnoughIn(ResourceType type, uint32 amount, uint32 timeMs);
    uint32 GetTimeToResource(ResourceType type, uint32 amount);

    // Class-specific resource helpers
    uint32 GetComboPoints();
    void ConsumeComboPoints();
    void AddComboPoints(uint32 points);

    uint32 GetHolyPower();
    void ConsumeHolyPower();
    void AddHolyPower(uint32 power);

    uint32 GetChi();
    void ConsumeChi(uint32 amount = 1);
    void AddChi(uint32 amount = 1);

    // Rune system (Death Knight)
    struct RuneInfo
    {
        bool available;
        uint32 cooldownRemaining;
        uint8 type; // 0=Blood, 1=Frost, 2=Unholy, 3=Death
    };

    ::std::vector<RuneInfo> GetRunes();
    uint32 GetAvailableRunes(uint8 runeType = 255); // 255 = any type
    bool HasRunesAvailable(uint32 bloodRunes, uint32 frostRunes, uint32 unholyRunes);
    void ConsumeRunes(uint32 bloodRunes, uint32 frostRunes, uint32 unholyRunes);

    // Resource efficiency tracking
    void RecordResourceUsage(ResourceType type, uint32 amount, uint32 spellId);
    float GetResourceEfficiency(ResourceType type);
    float GetSpellResourceEfficiency(uint32 spellId);

    // Optimization and planning
    bool CanAffordSpellSequence(const ::std::vector<uint32>& spellIds);
    uint32 GetOptimalResourceThreshold(ResourceType type);
    bool ShouldConserveResource(ResourceType type);

    // Emergency resource management
    bool IsResourceCritical(ResourceType type);
    bool NeedsResourceEmergency();
    ::std::vector<uint32> GetResourceEmergencySpells();

    // Statistics and monitoring
    uint32 GetTotalResourceGenerated(ResourceType type);
    uint32 GetTotalResourceConsumed(ResourceType type);
    float GetAverageResourceUsage(ResourceType type);

    // Debug and monitoring
    void DumpResourceState();
    ResourceInfo GetResourceInfo(ResourceType type);

private:
    Player* _bot;
    ::std::unordered_map<ResourceType, ResourceInfo> _resources;

    // Tracking data
    ::std::unordered_map<ResourceType, uint32> _totalGenerated;
    ::std::unordered_map<ResourceType, uint32> _totalConsumed;
    ::std::unordered_map<uint32, uint32> _spellResourceCost; // spellId -> resource cost
    ::std::unordered_map<uint32, uint32> _spellUsageCount;   // spellId -> usage count

    // Rune tracking (Death Knight specific)
    // MAX_RUNES is defined in Player.h as 7 - do not redefine
    RuneInfo _runes[MAX_RUNES];
    uint32 _runicPower;

    // Performance tracking
    ::std::atomic<uint32> _updateCount{0};
    uint32 _lastPerformanceCheck{0};

    // Internal methods
    void UpdateResourceRegeneration(ResourceInfo& resource, uint32 diff);
    void UpdateRunes(uint32 diff);
    void SyncWithPlayer();
    void InitializeClassResources();
    void LoadSpellResourceCosts();

    // Resource type mapping
    ResourceType GetPrimaryResourceType();
    Powers GetPowerTypeForResource(ResourceType type);
    ResourceType GetResourceTypeForPower(Powers power);

    // Spell cost calculation
    uint32 CalculateSpellResourceCost(uint32 spellId, ResourceType type);
    bool IsResourceTypeUsedBySpell(uint32 spellId, ResourceType type);

    // Constants
    static constexpr uint32 RUNE_COOLDOWN_MS = 10000; // 10 seconds
    static constexpr uint32 PERFORMANCE_CHECK_INTERVAL = 30000; // 30 seconds
    static constexpr float CRITICAL_RESOURCE_THRESHOLD = 0.2f;  // 20%
    static constexpr float CONSERVATION_THRESHOLD = 0.5f;       // 50%
};

// Utility class for resource calculations
class TC_GAME_API ResourceCalculator
{
public:
    // Calculate resource costs for spells
    static uint32 CalculateManaCost(uint32 spellId, Player* caster);
    static uint32 CalculateRageCost(uint32 spellId, Player* caster);
    static uint32 CalculateEnergyCost(uint32 spellId, Player* caster);
    static uint32 CalculateFocusCost(uint32 spellId, Player* caster);

    // Calculate resource regeneration rates
    static float CalculateManaRegen(Player* player);
    static float CalculateEnergyRegen(Player* player);
    static float CalculateRageDecay(Player* player);

    // Resource optimization
    static bool IsResourceEfficientSpell(uint32 spellId, Player* caster);
    static float CalculateResourceEfficiency(uint32 spellId, Player* caster);
    static uint32 GetOptimalResourceLevel(ResourceType type, Player* player);

    // Class-specific calculations
    static uint32 GetComboPointsFromSpell(uint32 spellId);
    static uint32 GetHolyPowerFromSpell(uint32 spellId);
    static uint32 GetChiFromSpell(uint32 spellId);

private:
    // Meyer's singleton accessors for DLL-safe static data
    static ::std::unordered_map<uint32, uint32>& GetManaCostCache();
    static ::std::unordered_map<uint32, uint32>& GetRageCostCache();
    static ::std::unordered_map<uint32, uint32>& GetEnergyCostCache();
    static ::std::recursive_mutex& GetCacheMutex();

    static void CacheSpellResourceCost(uint32 spellId);
};

// Resource monitoring for performance analysis
class TC_GAME_API ResourceMonitor
{
public:
    static ResourceMonitor& Instance();

    void RecordResourceUsage(uint32 botGuid, ResourceType type, uint32 amount);
    void RecordResourceWaste(uint32 botGuid, ResourceType type, uint32 amount);
    void RecordResourceStarvation(uint32 botGuid, ResourceType type, uint32 duration);

    // Analytics
    float GetAverageResourceUsage(ResourceType type);
    float GetResourceWasteRate(ResourceType type);
    uint32 GetResourceStarvationTime(ResourceType type);

    // Optimization suggestions
    ::std::vector<::std::string> GetResourceOptimizationSuggestions(uint32 botGuid);

private:
    ResourceMonitor() = default;
    ~ResourceMonitor() = default;

    struct ResourceUsageData
    {
        uint32 totalUsed;
        uint32 totalWasted;
        uint32 starvationTime;
        uint32 sampleCount;
    };

    ::std::unordered_map<uint32, ::std::unordered_map<ResourceType, ResourceUsageData>> _botResourceData;
    Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _dataMutex;
};

} // namespace Playerbot