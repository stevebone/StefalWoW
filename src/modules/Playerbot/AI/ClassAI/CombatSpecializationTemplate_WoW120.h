/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "CombatSpecializationBase.h"
#include "ResourceTypes_WoW120.h"
#include <type_traits>

namespace Playerbot
{

/**
 * Enhanced Combat Specialization Template for WoW 12.0
 *
 * This template provides specialized resource handling for all 13 classes.
 * Each class specialization inherits from this template with their specific resource type.
 */
template<typename ResourceType>
class CombatSpecializationTemplate : public CombatSpecializationBase
{
public:
    explicit CombatSpecializationTemplate(Player* bot, CombatRole role)
        : CombatSpecializationBase(bot, role, DetermineResourceEnum()),
          _resource()
    {
        InitializeResource();
    }

    virtual ~CombatSpecializationTemplate() = default;

    //==========================================================================
    // RESOURCE MANAGEMENT OVERRIDES
    //==========================================================================

    bool HasEnoughResource(uint32 spellId) override
    {
        uint32 cost = CalculateResourceCost(spellId);
        return HasEnoughResourceInternal(cost);
    }

    void ConsumeResource(uint32 spellId) override
    {
        uint32 cost = CalculateResourceCost(spellId);
        ConsumeResourceInternal(cost);
    }

    uint32 GetCurrentResource() const override
    {
        return GetCurrentResourceInternal();
    }

    uint32 GetMaxResource() const override
    {
        return GetMaxResourceInternal();
    }

    float GetResourcePercent() const override
    {
        return GetResourcePercentInternal();
    }

    void RegenerateResource(uint32 diff) override
    {
        RegenerateResourceInternal(diff);
    }

    //==========================================================================
    // TEMPLATE-SPECIFIC RESOURCE ACCESS
    //==========================================================================

    const ResourceType& GetTypedResource() const { return _resource; }
    ResourceType& GetTypedResource() { return _resource; }

protected:
    ResourceType _resource;

private:
    //==========================================================================
    // TEMPLATE SPECIALIZATION FOR SIMPLE RESOURCES
    //==========================================================================

    // SimpleResource specialization (Warrior, Hunter, DH)
    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, SimpleResource>::value, bool>::type
    HasEnoughResourceInternal(uint32 cost)
    {
        return _resource.HasEnough(cost);
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, SimpleResource>::value, void>::type
    ConsumeResourceInternal(uint32 cost)
    {
        _resource.Consume(cost);
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, SimpleResource>::value, uint32>::type
    GetCurrentResourceInternal() const
    {
        return _resource.current;
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, SimpleResource>::value, uint32>::type
    GetMaxResourceInternal() const
    {
        return _resource.maximum;
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, SimpleResource>::value, float>::type
    GetResourcePercentInternal() const
    {
        return _resource.GetPercent();
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, SimpleResource>::value, void>::type
    RegenerateResourceInternal(uint32 diff)
    {
        _resource.Regenerate(diff, _inCombat);
    }

    //==========================================================================
    // TEMPLATE SPECIALIZATION FOR DUAL RESOURCES
    //==========================================================================

    // DualResource specialization (Rogue, Monk, Paladin)
    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, DualResource>::value ||
                           ::std::is_same<T, ComboPointResource>::value ||
                           ::std::is_same<T, ChiResource>::value ||
                           ::std::is_same<T, HolyPowerResource>::value, bool>::type
    HasEnoughResourceInternal(uint32 cost)
    {
        // For dual resources, cost typically applies to primary
        return _resource.primary.HasEnough(cost);
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, DualResource>::value ||
                           ::std::is_same<T, ComboPointResource>::value ||
                           ::std::is_same<T, ChiResource>::value ||
                           ::std::is_same<T, HolyPowerResource>::value, void>::type
    ConsumeResourceInternal(uint32 cost)
    {
        _resource.primary.Consume(cost);
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, DualResource>::value ||
                           ::std::is_same<T, ComboPointResource>::value ||
                           ::std::is_same<T, ChiResource>::value ||
                           ::std::is_same<T, HolyPowerResource>::value, uint32>::type
    GetCurrentResourceInternal() const
    {
        return _resource.primary.current;
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, DualResource>::value ||
                           ::std::is_same<T, ComboPointResource>::value ||
                           ::std::is_same<T, ChiResource>::value ||
                           ::std::is_same<T, HolyPowerResource>::value, uint32>::type
    GetMaxResourceInternal() const
    {
        return _resource.primary.maximum;
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, DualResource>::value ||
                           ::std::is_same<T, ComboPointResource>::value ||
                           ::std::is_same<T, ChiResource>::value ||
                           ::std::is_same<T, HolyPowerResource>::value, float>::type
    GetResourcePercentInternal() const
    {
        return _resource.primary.GetPercent();
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, DualResource>::value ||
                           ::std::is_same<T, ComboPointResource>::value ||
                           ::std::is_same<T, ChiResource>::value ||
                           ::std::is_same<T, HolyPowerResource>::value, void>::type
    RegenerateResourceInternal(uint32 diff)
    {
        _resource.Regenerate(diff, _inCombat);
    }

    //==========================================================================
    // TEMPLATE SPECIALIZATION FOR RUNE RESOURCE
    //==========================================================================

    // RuneResource specialization (Death Knight)
    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, RuneResource>::value, bool>::type
    HasEnoughResourceInternal(uint32 cost)
    {
        // For DK, cost represents runic power
        return _resource.runicPower >= cost;
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, RuneResource>::value, void>::type
    ConsumeResourceInternal(uint32 cost)
    {
        _resource.SpendRunicPower(cost);
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, RuneResource>::value, uint32>::type
    GetCurrentResourceInternal() const
    {
        return _resource.runicPower;
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, RuneResource>::value, uint32>::type
    GetMaxResourceInternal() const
    {
        return _resource.maxRunicPower;
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, RuneResource>::value, float>::type
    GetResourcePercentInternal() const
    {
        return (float)_resource.runicPower / _resource.maxRunicPower;
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, RuneResource>::value, void>::type
    RegenerateResourceInternal(uint32 diff)
    {
        // Get haste modifier from player stats (affects rune regeneration)
        // Rune regeneration is affected by melee haste in WoW
        float hasteModifier = 1.0f;

        Player* player = dynamic_cast<Player*>(_bot);
        if (player)
        {
            // Get melee haste from gear rating
            float hastePct = player->GetRatingBonusValue(CR_HASTE_MELEE);

            // Add haste from auras (buffs, talents, etc.)
            hastePct += player->GetTotalAuraModifier(SPELL_AURA_MOD_MELEE_HASTE);
            hastePct += player->GetTotalAuraModifier(SPELL_AURA_MOD_MELEE_HASTE_2);
            hastePct += player->GetTotalAuraModifier(SPELL_AURA_MOD_MELEE_RANGED_HASTE);

            // Convert percentage to multiplier (e.g., 20% haste = 1.20 multiplier)
            hasteModifier = 1.0f + (hastePct / 100.0f);

            // Clamp to reasonable bounds (prevent negative or excessive values)
            hasteModifier = ::std::max(0.1f, ::std::min(hasteModifier, 3.0f));
        }

        _resource.Update(diff, _inCombat, hasteModifier);
    }

    //==========================================================================
    // TEMPLATE SPECIALIZATION FOR ESSENCE RESOURCE
    //==========================================================================

    // EssenceResource specialization (Evoker)
    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, EssenceResource>::value, bool>::type
    HasEnoughResourceInternal(uint32 cost)
    {
        return _resource.mana.HasEnough(cost);
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, EssenceResource>::value, void>::type
    ConsumeResourceInternal(uint32 cost)
    {
        _resource.mana.Consume(cost);
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, EssenceResource>::value, uint32>::type
    GetCurrentResourceInternal() const
    {
        return _resource.mana.current;
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, EssenceResource>::value, uint32>::type
    GetMaxResourceInternal() const
    {
        return _resource.mana.maximum;
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, EssenceResource>::value, float>::type
    GetResourcePercentInternal() const
    {
        return _resource.mana.GetPercent();
    }

    template<typename T = ResourceType>
    typename ::std::enable_if<::std::is_same<T, EssenceResource>::value, void>::type
    RegenerateResourceInternal(uint32 diff)
    {
        _resource.Update(diff);
    }

    //==========================================================================
    // RESOURCE TYPE DETERMINATION
    //==========================================================================

    static ResourceType DetermineResourceEnum()
    {
        // Map template type to enum
        if (::std::is_same<ResourceType, SimpleResource>::value)
        {
            // This would be determined by the specific class
            return ResourceType::RAGE; // Example
        }
        else if (::std::is_same<ResourceType, DualResource>::value ||
                 ::std::is_same<ResourceType, ComboPointResource>::value)
        {
            return ResourceType::ENERGY;
        }
        else if (::std::is_same<ResourceType, RuneResource>::value)
        {
            return ResourceType::RUNES;
        }
        else if (::std::is_same<ResourceType, EssenceResource>::value)
        {
            return ResourceType::ESSENCE;
        }
        // Add more mappings as needed
        return ResourceType::MANA;
    }

    void InitializeResource()
    {
        // Initialize resource based on type
        if constexpr (::std::is_same_v<ResourceType, SimpleResource>)
        {
            // Set up based on class
            // This would be done in the derived class
        }
        else if constexpr (::std::is_same_v<ResourceType, RuneResource>)
        {
            // Runes are already initialized in constructor
        }
        // Add more initialization as needed
    }

    uint32 CalculateResourceCost(uint32 spellId)
    {
        // This would query spell database for actual cost
        // For now, return placeholder
        return 20;
    }
};

//==============================================================================
// CLASS-SPECIFIC TEMPLATE SPECIALIZATIONS
//==============================================================================

// Death Knight specializations
class BloodDeathKnight : public CombatSpecializationTemplate<RuneResource>
{
public:
    explicit BloodDeathKnight(Player* bot)
        : CombatSpecializationTemplate<RuneResource>(bot, CombatRole::TANK) {}

    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);

    // Death Knight specific methods
    bool HasRunes(uint32 blood, uint32 frost, uint32 unholy) const
    {
        return _resource.HasRunes(blood, frost, unholy);
    }

    void ConsumeRunes(uint32 blood, uint32 frost, uint32 unholy)
    {
        _resource.ConsumeRunes(blood, frost, unholy);
    }
};

class FrostDeathKnight : public CombatSpecializationTemplate<RuneResource>
{
public:
    explicit FrostDeathKnight(Player* bot)
        : CombatSpecializationTemplate<RuneResource>(bot, CombatRole::MELEE_DPS) {}

    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);
};

class UnholyDeathKnight : public CombatSpecializationTemplate<RuneResource>
{
public:
    explicit UnholyDeathKnight(Player* bot)
        : CombatSpecializationTemplate<RuneResource>(bot, CombatRole::MELEE_DPS) {}

    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);
};

// Demon Hunter specializations
class HavocDemonHunter : public CombatSpecializationTemplate<SimpleResource>
{
public:
    explicit HavocDemonHunter(Player* bot)
        : CombatSpecializationTemplate<SimpleResource>(bot, CombatRole::MELEE_DPS)
    {
        _resource = SimpleResource(120, 0.0f, 0.0f); // Fury: max 120, no regen
    }

    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);
};

class VengeanceDemonHunter : public CombatSpecializationTemplate<SimpleResource>
{
public:
    explicit VengeanceDemonHunter(Player* bot)
        : CombatSpecializationTemplate<SimpleResource>(bot, CombatRole::TANK)
    {
        _resource = SimpleResource(100, 0.0f, 0.0f); // Pain: max 100, no regen
    }

    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);
};

// Druid specializations (complex due to forms)
class BalanceDruid : public CombatSpecializationTemplate<AstralPowerResource>
{
public:
    explicit BalanceDruid(Player* bot)
        : CombatSpecializationTemplate<AstralPowerResource>(bot, CombatRole::RANGED_DPS) {}

    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);
};

class FeralDruid : public CombatSpecializationTemplate<ComboPointResource>
{
public:
    explicit FeralDruid(Player* bot)
        : CombatSpecializationTemplate<ComboPointResource>(bot, CombatRole::MELEE_DPS) {}

    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);
};

class GuardianDruid : public CombatSpecializationTemplate<SimpleResource>
{
public:
    explicit GuardianDruid(Player* bot)
        : CombatSpecializationTemplate<SimpleResource>(bot, CombatRole::TANK)
    {
        _resource = SimpleResource(100, 0.0f, 1.0f); // Rage: max 100, decay 1/sec
    }

    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);
};

class RestorationDruid : public CombatSpecializationTemplate<SimpleResource>
{
public:
    explicit RestorationDruid(Player* bot)
        : CombatSpecializationTemplate<SimpleResource>(bot, CombatRole::HEALER)
    {
        _resource = SimpleResource(100000, 0.0f, 0.0f); // Mana only
    }

    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);
};

// Evoker specializations
class DevastationEvoker : public CombatSpecializationTemplate<EssenceResource>
{
public:
    explicit DevastationEvoker(Player* bot)
        : CombatSpecializationTemplate<EssenceResource>(bot, CombatRole::RANGED_DPS) {}

    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);

    bool HasEssence(uint8 charges = 1) const { return _resource.HasEssence(charges); }
    void ConsumeEssence(uint8 charges = 1) { _resource.ConsumeEssence(charges); }
};

class PreservationEvoker : public CombatSpecializationTemplate<EssenceResource>
{
public:
    explicit PreservationEvoker(Player* bot)
        : CombatSpecializationTemplate<EssenceResource>(bot, CombatRole::HEALER) {}

    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);
};

class AugmentationEvoker : public CombatSpecializationTemplate<EssenceResource>
{
public:
    explicit AugmentationEvoker(Player* bot)
        : CombatSpecializationTemplate<EssenceResource>(bot, CombatRole::HYBRID) {}

    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);
};

// Hunter specializations
class BeastMasteryHunter : public CombatSpecializationTemplate<SimpleResource>
{
public:
    explicit BeastMasteryHunter(Player* bot)
        : CombatSpecializationTemplate<SimpleResource>(bot, CombatRole::RANGED_DPS)
    {
        _resource = SimpleResource(100, 5.0f, 0.0f); // Focus: max 100, regen 5/sec
    }

    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);
};

class MarksmanshipHunter : public CombatSpecializationTemplate<SimpleResource>
{
public:
    explicit MarksmanshipHunter(Player* bot)
        : CombatSpecializationTemplate<SimpleResource>(bot, CombatRole::RANGED_DPS)
    {
        _resource = SimpleResource(100, 5.0f, 0.0f); // Focus
    }

    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);
};

class SurvivalHunter : public CombatSpecializationTemplate<SimpleResource>
{
public:
    explicit SurvivalHunter(Player* bot)
        : CombatSpecializationTemplate<SimpleResource>(bot, CombatRole::MELEE_DPS)
    {
        _resource = SimpleResource(100, 5.0f, 0.0f); // Focus
    }

    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);
};

// Additional class specializations would follow the same pattern...

} // namespace Playerbot