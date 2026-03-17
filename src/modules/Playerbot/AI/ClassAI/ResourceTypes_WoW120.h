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
#include "GameTime.h"
#include <array>
#include <algorithm>

namespace Playerbot
{

/**
 * WoW 12.0 (The War Within) Complete Resource Type System
 *
 * This file defines all resource types used by the 13 classes in WoW 12.0.
 * Each resource type is designed to work with the CombatSpecializationBase template.
 */

// Forward declarations
enum class RuneType : uint8;
struct RuneInfo;

//==============================================================================
// SIMPLE RESOURCE TYPES - Single value resources
//==============================================================================

/**
 * SimpleResource - Used for single-value resources like Mana, Rage, Energy, Focus
 * Used by: Hunter (Focus), Warrior (Rage), Demon Hunter (Fury/Pain)
 */
struct SimpleResource
{
    uint32 current;
    uint32 maximum;
    float regenRate;        // Per second
    float decayRate;        // Per second (out of combat)
    bool regenerates;       // Does this resource regenerate naturally?
    bool decays;            // Does this resource decay out of combat?

    SimpleResource()
        : current(0), maximum(100), regenRate(0.0f), decayRate(0.0f),
          regenerates(false), decays(false) {}

    SimpleResource(uint32 max, float regen = 0.0f, float decay = 0.0f)
        : current(max), maximum(max), regenRate(regen), decayRate(decay),
          regenerates(regen > 0.0f), decays(decay > 0.0f) {}

    // Helper methods
    float GetPercent() const { return maximum > 0 ? (float)current / maximum : 0.0f; }
    bool HasEnough(uint32 amount) const { return current >= amount; }

    uint32 Consume(uint32 amount)
    {
        uint32 consumed = ::std::min(amount, current);
        current -= consumed;
        return consumed;
    }

    uint32 Add(uint32 amount)
    {
        uint32 oldCurrent = current;
        current = ::std::min(current + amount, maximum);
        return current - oldCurrent;
    }

    void Regenerate(uint32 deltaMs, bool inCombat = true)
    {
        if (regenerates && regenRate > 0.0f)
        {
            float regen = (regenRate * deltaMs) / 1000.0f;
            current = ::std::min(current + (uint32)regen, maximum);
        }

        if (!inCombat && decays && decayRate > 0.0f)
        {
            float decay = (decayRate * deltaMs) / 1000.0f;
            current = current > (uint32)decay ? current - (uint32)decay : 0;
        }
    }
};

//==============================================================================
// DUAL RESOURCE TYPES - Primary + Secondary resources
//==============================================================================

/**
 * DualResource - Used for classes with two resource types
 * Used by: Rogue (Energy + Combo Points), Monk (Energy + Chi),
 *          Paladin (Mana + Holy Power), Druid Feral (Energy + Combo Points)
 */
struct DualResource
{
    SimpleResource primary;    // Energy, Mana, etc.
    SimpleResource secondary;  // Combo Points, Chi, Holy Power, etc.

    DualResource() = default;

    DualResource(uint32 primaryMax, float primaryRegen, uint32 secondaryMax)
        : primary(primaryMax, primaryRegen, 0.0f),
          secondary(secondaryMax, 0.0f, 0.0f) {}

    // Helper for checking both resources
    bool HasEnough(uint32 primaryCost, uint32 secondaryCost = 0) const
    {
        return primary.HasEnough(primaryCost) &&
               (secondaryCost == 0 || secondary.HasEnough(secondaryCost));
    }

    void Regenerate(uint32 deltaMs, bool inCombat = true)
    {
        primary.Regenerate(deltaMs, inCombat);
        // Secondary resources typically don't regenerate
    }
};

//==============================================================================
// COMBO POINT RESOURCE - Special handling for Rogue/Feral Druid
//==============================================================================

/**
 * ComboPointResource - Specialized for Combo Point mechanics
 * Combo Points are now on the player (since Legion), not the target
 */
struct ComboPointResource : public DualResource
{
    uint32 maxComboPoints;      // Can be 5-7 with talents
    uint32 anticipation;        // Overflow combo points (if talented)

    ComboPointResource() : DualResource(100, 10.0f, 5), maxComboPoints(5), anticipation(0)
    {
        secondary.maximum = maxComboPoints;
    }

    void AddComboPoints(uint32 points)
    {
        uint32 overflow = 0;
        if (secondary.current + points > secondary.maximum)
        {
            overflow = (secondary.current + points) - secondary.maximum;
            secondary.current = secondary.maximum;
        }
        else
        {
            secondary.current += points;
        }

        // Handle Anticipation talent (stores overflow combo points)
        if (overflow > 0 && anticipation < 10)
        {
            anticipation = ::std::min(anticipation + overflow, 10u);
        }
    }

    void ConsumeComboPoints()
    {
        secondary.current = 0;

        // Transfer Anticipation stacks if any
        if (anticipation > 0)
        {
            uint32 transfer = ::std::min(anticipation, secondary.maximum);
            secondary.current = transfer;
            anticipation -= transfer;
        }
    }
};

//==============================================================================
// RUNE RESOURCE - Death Knight specific
//==============================================================================

/**
 * RuneType - Death Knight rune types
 */
enum class RuneType : uint8
{
    BLOOD = 0,
    FROST = 1,
    UNHOLY = 2,
    DEATH = 3   // Can be used as any rune type
};

/**
 * RuneInfo - Individual rune state
 */
struct RuneInfo
{
    RuneType type;
    bool available;
    uint32 cooldownRemaining;   // Milliseconds
    uint32 lastUsed;            // Timestamp

    RuneInfo() : type(RuneType::BLOOD), available(true), cooldownRemaining(0), lastUsed(0) {}
    RuneInfo(RuneType t) : type(t), available(true), cooldownRemaining(0), lastUsed(0) {}

    bool IsReady() const { return available && cooldownRemaining == 0; }

    void Use()
    {
        available = false;
        cooldownRemaining = 10000;  // 10 seconds base
        lastUsed = GameTime::GetGameTimeMS();
    }

    void Update(uint32 diff, float hasteModifier = 1.0f)
    {
        if (!available && cooldownRemaining > 0)
        {
            uint32 reduction = (uint32)(diff * hasteModifier);
            if (cooldownRemaining > reduction)
                cooldownRemaining -= reduction;
            else
            {
                cooldownRemaining = 0;
                available = true;
            }
        }
    }

    bool CanBeUsedAs(RuneType requiredType) const
    {
        return IsReady() && (type == requiredType || type == RuneType::DEATH);
    }
};

/**
 * RuneResource - Complete Death Knight resource system
 */
struct RuneResource
{
    static constexpr uint32 MAX_RUNES = 6;
    static constexpr uint32 RUNIC_POWER_MAX = 130;
    static constexpr uint32 RUNE_COOLDOWN_MS = 10000;
    static constexpr float RUNIC_POWER_DECAY_RATE = 2.0f; // per second

    ::std::array<RuneInfo, MAX_RUNES> runes;
    uint32 runicPower;
    uint32 maxRunicPower;

    RuneResource() : runicPower(0), maxRunicPower(RUNIC_POWER_MAX)
    {
        // Initialize 2 Blood, 2 Frost, 2 Unholy runes
        runes[0] = RuneInfo(RuneType::BLOOD);
        runes[1] = RuneInfo(RuneType::BLOOD);
        runes[2] = RuneInfo(RuneType::FROST);
        runes[3] = RuneInfo(RuneType::FROST);
        runes[4] = RuneInfo(RuneType::UNHOLY);
        runes[5] = RuneInfo(RuneType::UNHOLY);
    }

    // Count available runes of a specific type
    uint32 GetAvailableRunes(RuneType type) const
    {
        uint32 count = 0;
        for (const auto& rune : runes)
        {
            if (rune.CanBeUsedAs(type))
                count++;
        }
        return count;
    }

    // Check if we have enough runes for a spell
    bool HasRunes(uint32 blood, uint32 frost, uint32 unholy) const
    {
        uint32 availableBlood = GetAvailableRunes(RuneType::BLOOD);
        uint32 availableFrost = GetAvailableRunes(RuneType::FROST);
        uint32 availableUnholy = GetAvailableRunes(RuneType::UNHOLY);
        uint32 availableDeath = GetAvailableRunes(RuneType::DEATH);

        // Death runes can substitute for any type
        return (availableBlood + availableDeath >= blood) &&
               (availableFrost + availableDeath >= frost) &&
               (availableUnholy + availableDeath >= unholy);
    }

    // Consume runes for a spell
    void ConsumeRunes(uint32 blood, uint32 frost, uint32 unholy)
    {
        auto consumeType = [this](RuneType type, uint32 count)
        {
            for (auto& rune : runes)
            {
                if (count == 0) break;
                if (rune.CanBeUsedAs(type))
                {
                    rune.Use();
                    GenerateRunicPower(10); // Each rune generates 10 RP base
                    count--;
                }
            }
        };

        // Prioritize using specific runes before Death runes
        consumeType(RuneType::BLOOD, blood);
        consumeType(RuneType::FROST, frost);
        consumeType(RuneType::UNHOLY, unholy);
    }

    // Generate Runic Power
    void GenerateRunicPower(uint32 amount)
    {
        runicPower = ::std::min(runicPower + amount, maxRunicPower);
    }

    // Spend Runic Power
    bool SpendRunicPower(uint32 amount)
    {
        if (runicPower >= amount)
        {
            runicPower -= amount;
            return true;
        }
        return false;
    }

    // Update rune cooldowns and runic power decay
    void Update(uint32 diff, bool inCombat, float hasteModifier = 1.0f)
    {
        // Update rune cooldowns
        for (auto& rune : runes)
        {
            rune.Update(diff, hasteModifier);
        }

        // Decay runic power out of combat
        if (!inCombat && runicPower > 0)
        {
            float decay = (RUNIC_POWER_DECAY_RATE * diff) / 1000.0f;
            runicPower = runicPower > (uint32)decay ? runicPower - (uint32)decay : 0;
        }
    }

    // Check combined resource availability
    bool HasEnough(uint32 bloodRunes, uint32 frostRunes, uint32 unholyRunes, uint32 runicPowerCost) const
    {
        return HasRunes(bloodRunes, frostRunes, unholyRunes) && runicPower >= runicPowerCost;
    }
};

//==============================================================================
// ESSENCE RESOURCE - Evoker specific
//==============================================================================

/**
 * EssenceResource - Evoker's unique charge-based system
 */
struct EssenceResource
{
    static constexpr uint32 MAX_ESSENCE_CHARGES = 5;
    static constexpr uint32 ESSENCE_RECHARGE_MS = 5000; // 5 seconds per charge

    SimpleResource mana;                                       // Primary resource
    uint8 essenceCharges;                                     // Current charges
    ::std::array<uint32, MAX_ESSENCE_CHARGES> rechargeTimers;   // Per-charge timers

    EssenceResource() : mana(100000, 0.0f, 0.0f), essenceCharges(MAX_ESSENCE_CHARGES)
    {
        for (auto& timer : rechargeTimers)
            timer = 0;
    }

    bool HasEssence(uint8 charges = 1) const
    {
        return essenceCharges >= charges;
    }

    void ConsumeEssence(uint8 charges = 1)
    {
        charges = ::std::min(charges, essenceCharges);
        essenceCharges -= charges;

        // Start recharge timers for consumed charges
        for (uint8 i = 0; i < charges; ++i)
        {
            for (auto& timer : rechargeTimers)
            {
                if (timer == 0)
                {
                    timer = ESSENCE_RECHARGE_MS;
                    break;
                }
            }
        }
    }

    void Update(uint32 diff)
    {
        // Update recharge timers
        for (auto& timer : rechargeTimers)
        {
            if (timer > 0)
            {
                if (timer > diff)
                    timer -= diff;
                else
                {
                    timer = 0;
                    if (essenceCharges < MAX_ESSENCE_CHARGES)
                        essenceCharges++;
                }
            }
        }

        // Mana regeneration is handled by base game
    }

    uint32 GetNextEssenceRecharge() const
    {
        uint32 shortest = UINT32_MAX;
        for (const auto& timer : rechargeTimers)
        {
            if (timer > 0 && timer < shortest)
                shortest = timer;
        }
        return shortest == UINT32_MAX ? 0 : shortest;
    }
};

//==============================================================================
// SOUL SHARD RESOURCE - Warlock specific
//==============================================================================

/**
 * SoulShardResource - Warlock's fractional shard system
 */
struct SoulShardResource
{
    static constexpr uint32 MAX_SOUL_SHARDS = 5;
    static constexpr uint32 SHARD_FRACTION_DIVISOR = 10; // Each shard = 10 fragments

    SimpleResource mana;      // Primary resource
    uint32 soulFragments;     // Stored as fragments (50 = 5.0 shards)

    SoulShardResource() : mana(100000, 0.0f, 0.0f), soulFragments(30) {} // Start with 3.0 shards

    float GetSoulShards() const
    {
        return (float)soulFragments / SHARD_FRACTION_DIVISOR;
    }

    uint32 GetWholeSoulShards() const
    {
        return soulFragments / SHARD_FRACTION_DIVISOR;
    }

    bool HasSoulShards(float shards) const
    {
        uint32 required = (uint32)(shards * SHARD_FRACTION_DIVISOR);
        return soulFragments >= required;
    }

    void GenerateSoulFragments(uint32 fragments)
    {
        uint32 maxFragments = MAX_SOUL_SHARDS * SHARD_FRACTION_DIVISOR;
        soulFragments = ::std::min(soulFragments + fragments, maxFragments);
    }

    void ConsumeSoulShards(float shards)
    {
        uint32 fragments = (uint32)(shards * SHARD_FRACTION_DIVISOR);
        soulFragments = fragments > soulFragments ? 0 : soulFragments - fragments;
    }

    // Helper for abilities that generate fractional shards
    void GenerateFromDamage(uint32 damage, uint32 targetMaxHealth)
    {
        // Generate 0.1-0.3 shards based on damage percentage
        float damagePercent = (float)damage / targetMaxHealth;
        uint32 fragments = (uint32)(damagePercent * 3); // 0-3 fragments
        GenerateSoulFragments(::std::min(fragments, 3u));
    }
};

//==============================================================================
// ASTRAL POWER RESOURCE - Balance Druid specific
//==============================================================================

/**
 * AstralPowerResource - Balance Druid's builder/spender system
 */
struct AstralPowerResource
{
    SimpleResource mana;         // Primary (for utility spells)
    SimpleResource astralPower;  // Secondary (for damage)

    AstralPowerResource()
        : mana(100000, 0.0f, 0.0f),
          astralPower(100, 0.0f, 0.0f) // Max 100, no natural regen/decay
    {
        astralPower.maximum = 100;
    }

    void GenerateAstralPower(uint32 amount)
    {
        astralPower.Add(amount);
    }

    bool SpendAstralPower(uint32 amount)
    {
        if (astralPower.HasEnough(amount))
        {
            astralPower.Consume(amount);
            return true;
        }
        return false;
    }

    // Celestial Alignment/Incarnation bonus generation
    void ApplyCelestialBonus(float multiplier = 1.5f)
    {
        // Increase generation rate during cooldowns
        // This would affect the actual spell calculations
    }
};

//==============================================================================
// MAELSTROM RESOURCE - Elemental/Enhancement Shaman
//==============================================================================

/**
 * MaelstromResource - Shaman's builder/spender system
 */
struct MaelstromResource
{
    SimpleResource mana;       // Primary (for utility)
    SimpleResource maelstrom;  // Secondary (for damage)
    uint32 maelstromWeaponStacks; // Enhancement only

    MaelstromResource()
        : mana(100000, 0.0f, 0.0f),
          maelstrom(100, 0.0f, 0.0f),
          maelstromWeaponStacks(0)
    {
        maelstrom.maximum = 100; // Can be increased to 150 with talents
    }

    void GenerateMaelstrom(uint32 amount)
    {
        maelstrom.Add(amount);
    }

    bool SpendMaelstrom(uint32 amount)
    {
        if (maelstrom.HasEnough(amount))
        {
            maelstrom.Consume(amount);
            return true;
        }
        return false;
    }

    // Enhancement-specific Maelstrom Weapon stacks
    void AddMaelstromWeaponStack()
    {
        maelstromWeaponStacks = ::std::min(maelstromWeaponStacks + 1, 10u);
    }

    bool ConsumeMaelstromWeapon(uint32 stacks = 5)
    {
        if (maelstromWeaponStacks >= stacks)
        {
            maelstromWeaponStacks -= stacks;
            return true;
        }
        return false;
    }
};

//==============================================================================
// INSANITY RESOURCE - Shadow Priest specific
//==============================================================================

/**
 * InsanityResource - Shadow Priest's unique voidform system
 */
struct InsanityResource
{
    SimpleResource mana;      // Primary
    SimpleResource insanity;  // Secondary (0-100)
    bool inVoidform;
    uint32 voidformStacks;
    float insanityDrainRate;  // Increases over time in Voidform

    InsanityResource()
        : mana(100000, 0.0f, 0.0f),
          insanity(100, 0.0f, 0.0f),
          inVoidform(false),
          voidformStacks(0),
          insanityDrainRate(6.0f) // Base 6 insanity/sec
    {
        insanity.maximum = 100;
        insanity.current = 0;
    }

    void GenerateInsanity(uint32 amount)
    {
        insanity.Add(amount);

        // Enter Voidform at 100 Insanity
        if (!inVoidform && insanity.current >= 100)
        {
            EnterVoidform();
        }
    }

    void EnterVoidform()
    {
        inVoidform = true;
        voidformStacks = 1;
        insanityDrainRate = 6.0f;
        insanity.current = 100; // Start with full in Voidform
    }

    void ExitVoidform()
    {
        inVoidform = false;
        voidformStacks = 0;
        insanityDrainRate = 6.0f;
        insanity.current = 0;
    }

    void UpdateVoidform(uint32 diff)
    {
        if (!inVoidform)
            return;

        // Drain Insanity
        float drain = (insanityDrainRate * diff) / 1000.0f;
        if (insanity.current > drain)
        {
            insanity.current -= (uint32)drain;

            // Increase drain rate every second (stack)
            static uint32 stackTimer = 0;
            stackTimer += diff;
            if (stackTimer >= 1000)
            {
                stackTimer = 0;
                voidformStacks++;
                insanityDrainRate += 0.68f; // Increases by 0.68/sec per stack
            }
        }
        else
        {
            ExitVoidform();
        }
    }
};

//==============================================================================
// CHI RESOURCE - Monk specific (Energy + Chi)
//==============================================================================

/**
 * ChiResource - Monk's Energy + Chi system
 */
struct ChiResource : public DualResource
{
    static constexpr uint32 MAX_CHI_BASE = 5;
    static constexpr uint32 MAX_CHI_TALENTED = 6;

    ChiResource() : DualResource(100, 10.0f, MAX_CHI_BASE)
    {
        primary.maximum = 100;     // Energy
        secondary.maximum = MAX_CHI_BASE; // Chi (can be 6 with talent)
    }

    void GenerateChi(uint32 amount = 1)
    {
        secondary.Add(amount);
    }

    bool SpendChi(uint32 amount)
    {
        if (secondary.HasEnough(amount))
        {
            secondary.Consume(amount);
            return true;
        }
        return false;
    }

    // Windwalker Mastery: Combo Strikes bonus
    bool hasComboStrike = false;
    uint32 lastAbilityUsed = 0;

    void OnAbilityUse(uint32 spellId)
    {
        hasComboStrike = (spellId != lastAbilityUsed);
        lastAbilityUsed = spellId;
    }

    float GetMasteryBonus() const
    {
        return hasComboStrike ? 1.12f : 1.0f; // 12% bonus with Combo Strikes
    }
};

//==============================================================================
// HOLY POWER RESOURCE - Paladin specific
//==============================================================================

/**
 * HolyPowerResource - Paladin's builder/spender system
 */
struct HolyPowerResource : public DualResource
{
    static constexpr uint32 MAX_HOLY_POWER = 5;

    HolyPowerResource() : DualResource(100000, 0.0f, MAX_HOLY_POWER)
    {
        primary.maximum = 100000;  // Mana
        secondary.maximum = MAX_HOLY_POWER; // Holy Power
    }

    void GenerateHolyPower(uint32 amount = 1)
    {
        secondary.Add(amount);
    }

    bool SpendHolyPower(uint32 amount)
    {
        if (secondary.HasEnough(amount))
        {
            secondary.Consume(amount);
            return true;
        }
        return false;
    }

    // Divine Purpose proc tracking
    bool hasDivinePurpose = false;
    uint32 divinePurposeExpires = 0;

    void CheckDivinePurpose(uint32 currentTime)
    {
        hasDivinePurpose = currentTime < divinePurposeExpires;
    }

    bool CanUseFreeSpender() const
    {
        return hasDivinePurpose;
    }
};

//==============================================================================
// ARCANE CHARGES - Mage Arcane specific
//==============================================================================

/**
 * ArcaneChargeResource - Arcane Mage's charge system
 */
struct ArcaneChargeResource
{
    static constexpr uint32 MAX_ARCANE_CHARGES = 4;

    SimpleResource mana;
    uint8 arcaneCharges;
    float manaCostModifier; // Increases with charges

    ArcaneChargeResource() : mana(100000, 0.0f, 0.0f), arcaneCharges(0), manaCostModifier(1.0f) {}

    void GenerateArcaneCharge()
    {
        if (arcaneCharges < MAX_ARCANE_CHARGES)
        {
            arcaneCharges++;
            UpdateManaCostModifier();
        }
    }

    void ConsumeAllCharges()
    {
        arcaneCharges = 0;
        UpdateManaCostModifier();
    }

    void UpdateManaCostModifier()
    {
        // Each charge increases mana cost by 100%
        manaCostModifier = 1.0f + (arcaneCharges * 1.0f);
    }

    uint32 GetModifiedManaCost(uint32 baseCost) const
    {
        return (uint32)(baseCost * manaCostModifier);
    }

    float GetDamageModifier() const
    {
        // Each charge increases damage by 60%
        return 1.0f + (arcaneCharges * 0.6f);
    }
};

//==============================================================================
// RESOURCE TYPE MAPPING - For template specialization
//==============================================================================

/**
 * Resource type traits for template metaprogramming
 */
template<typename T>
struct ResourceTraits
{
    static constexpr bool isSimple = false;
    static constexpr bool isDual = false;
    static constexpr bool isComplex = false;
};

// Specializations
template<> struct ResourceTraits<SimpleResource>
{
    static constexpr bool isSimple = true;
};

template<> struct ResourceTraits<DualResource>
{
    static constexpr bool isDual = true;
};

template<> struct ResourceTraits<ComboPointResource>
{
    static constexpr bool isDual = true;
};

template<> struct ResourceTraits<RuneResource>
{
    static constexpr bool isComplex = true;
};

template<> struct ResourceTraits<EssenceResource>
{
    static constexpr bool isComplex = true;
};

template<> struct ResourceTraits<SoulShardResource>
{
    static constexpr bool isComplex = true;
};

template<> struct ResourceTraits<AstralPowerResource>
{
    static constexpr bool isDual = true;
};

template<> struct ResourceTraits<MaelstromResource>
{
    static constexpr bool isDual = true;
};

template<> struct ResourceTraits<InsanityResource>
{
    static constexpr bool isComplex = true;
};

template<> struct ResourceTraits<ChiResource>
{
    static constexpr bool isDual = true;
};

template<> struct ResourceTraits<HolyPowerResource>
{
    static constexpr bool isDual = true;
};

template<> struct ResourceTraits<ArcaneChargeResource>
{
    static constexpr bool isComplex = true;
};

//==============================================================================
// UNIFIED RESOURCE INTERFACE - For polymorphic access
//==============================================================================

/**
 * IResourceSystem - Base interface for all resource types
 */
class IResourceSystem
{
public:
    virtual ~IResourceSystem() = default;

    // Core resource checks
    virtual bool HasEnoughPrimary(uint32 amount) const = 0;
    virtual bool HasEnoughSecondary(uint32 amount) const = 0;
    virtual float GetPrimaryPercent() const = 0;
    virtual float GetSecondaryPercent() const = 0;

    // Resource management
    virtual void ConsumePrimary(uint32 amount) = 0;
    virtual void ConsumeSecondary(uint32 amount) = 0;
    virtual void GeneratePrimary(uint32 amount) = 0;
    virtual void GenerateSecondary(uint32 amount) = 0;

    // Update
    virtual void Update(uint32 diff, bool inCombat) = 0;

    // Type identification
    virtual const char* GetResourceTypeName() const = 0;
};

} // namespace Playerbot