/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Resource Type Definitions for Template Specializations
 *
 * This file defines all resource types used by the template-based
 * combat specialization system, including complex types like runes.
 */

#pragma once

#include "Define.h"
#include "SharedDefines.h"
#include <array>
#include <atomic>
#include <algorithm>
#include "GameTime.h"

namespace Playerbot
{

// ============================================================================
// RUNE SYSTEM - Complex resource type for Death Knights
// ============================================================================

/**
 * Complex resource type for Death Knight rune management
 * Satisfies the ComplexResource concept
 */
class RuneSystem
{
public:
    enum class RuneType : uint8
    {
        BLOOD = 0,
        FROST = 1,
        UNHOLY = 2,
        DEATH = 3
    };

    struct Rune
    {
        RuneType type;
        ::std::atomic<bool> available{true};
        ::std::atomic<uint32> cooldownRemaining{0};
        uint32 lastUsedTime{0};

        bool IsReady() const { return available.load() && cooldownRemaining.load() == 0; }
    };

    RuneSystem()
    {
        // Initialize 6 runes: 2 Blood, 2 Frost, 2 Unholy
        _runes[0].type = RuneType::BLOOD;
        _runes[1].type = RuneType::BLOOD;
        _runes[2].type = RuneType::FROST;
        _runes[3].type = RuneType::FROST;
        _runes[4].type = RuneType::UNHOLY;
        _runes[5].type = RuneType::UNHOLY;

        _runicPower = 0;
        _maxRunicPower = 100;
    }

    /**
     * Initialize with bot-specific data
     */
    void Initialize(Player* bot)
    {
        _bot = bot;
        // Could sync with actual player rune state here
        ResetAllRunes();
    }

    /**
     * Check if resource is available (ComplexResource requirement)
     */
    bool available() const
    {
        return GetAvailable() > 0 || _runicPower > 0;
    }

    /**
     * Consume resources (ComplexResource requirement)
     * @param amount Number of runes or runic power to consume
     * @return true if consumption was successful
     */
    bool Consume(uint32 amount)
    {
        // For simplicity, consume any available runes
        uint32 consumed = 0;
        for (auto& rune : _runes)
        {
            if (consumed >= amount)
                break;

            if (rune.IsReady())
            {
                rune.available = false;
                rune.cooldownRemaining = RUNE_COOLDOWN_MS;
                rune.lastUsedTime = GameTime::GetGameTimeMS();
                consumed++;
            }
        }

        // If not enough runes, try to use runic power
        if (consumed < amount)
        {
            uint32 powerNeeded = (amount - consumed) * 10; // 10 RP per missing rune
            if (_runicPower >= powerNeeded)
            {
                _runicPower -= powerNeeded;
                return true;
            }
            return false;
        }

        // Generate runic power when consuming runes
        _runicPower = ::std::min<uint32>(_runicPower + consumed * 10, _maxRunicPower);
        return true;
    }

    /**
     * Regenerate resources over time (ComplexResource requirement)
     */
    void Regenerate(uint32 diff)
    {
        for (auto& rune : _runes)
        {
            if (!rune.available && rune.cooldownRemaining > 0)
            {
                if (rune.cooldownRemaining > diff)
                    rune.cooldownRemaining -= diff;
                else
                {
                    rune.cooldownRemaining = 0;
                    rune.available = true;
                }
            }
        }
    }

    /**
     * Get number of available resources (ComplexResource requirement)
     */
    uint32 GetAvailable() const
    {
        uint32 count = 0;
        for (const auto& rune : _runes)
        {
            if (rune.IsReady())
                count++;
        }
        return count;
    }

    /**
     * Get maximum resources (ComplexResource requirement)
     */
    uint32 GetMax() const
    {
        return 6; // 6 runes maximum
    }

    /**
     * Advanced rune queries for Death Knight specializations
     */
    uint32 GetAvailableRunesOfType(RuneType type) const
    {
        uint32 count = 0;
        for (const auto& rune : _runes)
        {
            if (rune.IsReady() && (rune.type == type || rune.type == RuneType::DEATH))
                count++;
        }
        return count;
    }

    bool HasRunes(uint32 blood, uint32 frost, uint32 unholy) const
    {
        return GetAvailableRunesOfType(RuneType::BLOOD) >= blood &&
               GetAvailableRunesOfType(RuneType::FROST) >= frost &&
               GetAvailableRunesOfType(RuneType::UNHOLY) >= unholy;
    }

    void ConsumeSpecificRunes(uint32 blood, uint32 frost, uint32 unholy)
    {
        auto consumeType = [this](RuneType type, uint32 count) {
            uint32 consumed = 0;
            for (auto& rune : _runes)
            {
                if (consumed >= count)
                    break;

                if (rune.IsReady() && (rune.type == type || rune.type == RuneType::DEATH))
                {
                    rune.available = false;
                    rune.cooldownRemaining = RUNE_COOLDOWN_MS;
                    rune.lastUsedTime = GameTime::GetGameTimeMS();
                    consumed++;
                }
            }
        };

        consumeType(RuneType::BLOOD, blood);
        consumeType(RuneType::FROST, frost);
        consumeType(RuneType::UNHOLY, unholy);

        // Generate runic power
        uint32 totalConsumed = blood + frost + unholy;
        _runicPower = ::std::min<uint32>(_runicPower + totalConsumed * 10, _maxRunicPower);
    }

    uint32 GetRunicPower() const { return _runicPower; }
    void SetRunicPower(uint32 power) { _runicPower = ::std::min(power, _maxRunicPower); }
    void ConsumeRunicPower(uint32 amount) { _runicPower = (amount > _runicPower) ? 0 : _runicPower - amount; }

    void ResetAllRunes()
    {
        for (auto& rune : _runes)
        {
            rune.available = true;
            rune.cooldownRemaining = 0;
        }
        _runicPower = 0;
    }

private:
    Player* _bot = nullptr;
    ::std::array<Rune, 6> _runes;
    ::std::atomic<uint32> _runicPower{0};
    uint32 _maxRunicPower{100};

    static constexpr uint32 RUNE_COOLDOWN_MS = 10000; // 10 seconds
};

// ============================================================================
// COMBO POINT SYSTEM - Secondary resource for Rogues/Feral Druids
// ============================================================================

/**
 * Combo point tracking system
 * This is a secondary resource, primary resource is Energy
 */
class ComboPointSystem
{
public:
    ComboPointSystem() : _comboPoints(0), _maxComboPoints(5) {}

    void Initialize(Player* bot)
    {
        _bot = bot;
        _comboPoints = 0;
    }

    bool available() const { return _comboPoints > 0; }

    bool Consume(uint32 amount)
    {
        if (_comboPoints >= amount)
        {
            _comboPoints -= amount;
            return true;
        }
        return false;
    }

    void Regenerate(uint32 /*diff*/)
    {
        // Combo points don't regenerate over time
    }

    uint32 GetAvailable() const { return _comboPoints; }
    uint32 GetMax() const { return _maxComboPoints; }

    void AddComboPoints(uint32 points)
    {
        _comboPoints = ::std::min<uint32>(_comboPoints + points, _maxComboPoints);
    }

    void ConsumeAll()
    {
        _lastConsumedPoints = _comboPoints;
        _comboPoints = 0;
    }

    uint32 GetLastConsumedPoints() const { return _lastConsumedPoints; }

private:
    Player* _bot = nullptr;
    ::std::atomic<uint32> _comboPoints{0};
    uint32 _maxComboPoints{5};
    uint32 _lastConsumedPoints{0};
};

// ============================================================================
// HOLY POWER SYSTEM - Secondary resource for Paladins
// ============================================================================

class HolyPowerSystem
{
public:
    HolyPowerSystem() : _holyPower(0), _maxHolyPower(3) {} // 5 with talents

    void Initialize(Player* bot)
    {
        _bot = bot;
        _holyPower = 0;
        // Check for talents that increase max holy power
        // _maxHolyPower = HasTalent(TALENT_ID) ? 5 : 3;
    }

    bool available() const { return _holyPower > 0; }

    bool Consume(uint32 amount)
    {
        if (_holyPower >= amount)
        {
            _holyPower -= amount;
            return true;
        }
        return false;
    }

    void Regenerate(uint32 /*diff*/)
    {
        // Holy Power doesn't regenerate over time
    }

    uint32 GetAvailable() const { return _holyPower; }
    uint32 GetMax() const { return _maxHolyPower; }

    void Generate(uint32 amount = 1)
    {
        _holyPower = ::std::min<uint32>(_holyPower + amount, _maxHolyPower);
    }

private:
    Player* _bot = nullptr;
    ::std::atomic<uint32> _holyPower{0};
    uint32 _maxHolyPower{3};
};

// ============================================================================
// CHI SYSTEM - Secondary resource for Monks
// ============================================================================

class ChiSystem
{
public:
    ChiSystem() : _chi(0), _maxChi(4) {} // 5 with Ascension talent

    void Initialize(Player* bot)
    {
        _bot = bot;
        _chi = 0;
    }

    bool available() const { return _chi > 0; }

    bool Consume(uint32 amount)
    {
        if (_chi >= amount)
        {
            _chi -= amount;
            return true;
        }
        return false;
    }

    void Regenerate(uint32 /*diff*/)
    {
        // Chi doesn't regenerate over time
    }

    uint32 GetAvailable() const { return _chi; }
    uint32 GetMax() const { return _maxChi; }

    void Generate(uint32 amount = 1)
    {
        _chi = ::std::min<uint32>(_chi + amount, _maxChi);
    }

private:
    Player* _bot = nullptr;
    ::std::atomic<uint32> _chi{0};
    uint32 _maxChi{4};
};

// ============================================================================
// SOUL SHARD SYSTEM - Resource for Warlocks
// ============================================================================

class SoulShardSystem
{
public:
    SoulShardSystem() : _shards(3.0f), _maxShards(5.0f) {}

    void Initialize(Player* bot)
    {
        _bot = bot;
        _shards = 3.0f; // Start with 3 shards
    }

    bool available() const { return _shards >= 1.0f; }

    bool Consume(uint32 amount)
    {
        float cost = static_cast<float>(amount);
        if (_shards >= cost)
        {
            _shards -= cost;
            return true;
        }
        return false;
    }

    void Regenerate(uint32 diff)
    {
        // Very slow regeneration out of combat        if (!_bot || !_bot->IsInCombat())
        {
            float regenRate = 0.1f; // 0.1 shards per second out of combat
            _shards = ::std::min(_shards + (regenRate * diff / 1000.0f), _maxShards);
        }
    }

    uint32 GetAvailable() const { return static_cast<uint32>(_shards); }
    uint32 GetMax() const { return static_cast<uint32>(_maxShards); }

    void Generate(float amount = 1.0f)
    {
        _shards = ::std::min(_shards + amount, _maxShards);
    }

    float GetExactShards() const { return _shards; }

private:
    Player* _bot = nullptr;
    float _shards{3.0f};  // Not atomic - per-bot instance, single-threaded access
    float _maxShards{5.0f};
};

// ============================================================================
// ESSENCE SYSTEM - Resource for Evokers
// ============================================================================

class EssenceResource
{
public:
    EssenceResource() : available(false), _essence(0), _maxEssence(5) {}

    void Initialize(Player* bot)
    {
        _bot = bot;
        _essence = 0;
        available = false;
    }

    bool available;

    bool Consume(uint32 amount)
    {
        if (_essence >= amount)
        {
            _essence -= amount;
            available = (_essence > 0);
            return true;
        }
        return false;
    }

    void Regenerate(uint32 diff)
    {
        // Essence regenerates at 0.2 per second (1 per 5 seconds)
        float regenRate = 0.2f;
        _essence = ::std::min(_essence + static_cast<uint32>(regenRate * diff / 1000.0f), _maxEssence);
        available = (_essence > 0);
    }

    uint32 GetAvailable() const { return _essence; }
    uint32 GetMax() const { return _maxEssence; }

    void Generate(uint32 amount = 1)
    {
        _essence = ::std::min<uint32>(_essence + amount, _maxEssence);
        available = (_essence > 0);
    }

private:
    Player* _bot = nullptr;
    ::std::atomic<uint32> _essence{0};
    uint32 _maxEssence{5};
};

// ============================================================================
// RESOURCE MANAGEMENT UTILITIES
// ============================================================================

/**
 * Helper to determine resource type from class
 */
inline Powers GetPrimaryPowerType(Classes playerClass)
{
    switch (playerClass)
    {
        case CLASS_WARRIOR:
            return POWER_RAGE;
        case CLASS_PALADIN:
            return POWER_MANA;
        case CLASS_HUNTER:
            return POWER_FOCUS;
        case CLASS_ROGUE:
            return POWER_ENERGY;
        case CLASS_PRIEST:
            return POWER_MANA;
        case CLASS_DEATH_KNIGHT:
            return POWER_RUNIC_POWER;
        case CLASS_SHAMAN:
            return POWER_MANA;
        case CLASS_MAGE:
            return POWER_MANA;
        case CLASS_WARLOCK:
            return POWER_MANA;
        case CLASS_MONK:
            return POWER_ENERGY;
        case CLASS_DRUID:
            return POWER_MANA; // Can change with form
        case CLASS_DEMON_HUNTER:
            return POWER_FURY;
        case CLASS_EVOKER:
            return POWER_ESSENCE;
        default:
            return POWER_MANA;
    }
}

/**
 * Helper to get resource regeneration rate
 */
inline float GetResourceRegenRate(Powers powerType)
{
    switch (powerType)
    {
        case POWER_MANA:
            return 5.0f; // 5% per 5 seconds
        case POWER_RAGE:
            return -2.0f; // Decays over time
        case POWER_FOCUS:
            return 5.0f; // 5 per second
        case POWER_ENERGY:
            return 10.0f; // 10 per second
        case POWER_RUNIC_POWER:
            return 0.0f; // Generated by rune usage
        case POWER_FURY:
            return -5.0f; // Decays over time
        case POWER_ESSENCE:
            return 0.2f; // 0.2 per second (1 per 5 seconds)
        default:
            return 0.0f;
    }
}

} // namespace Playerbot