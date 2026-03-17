/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

/**
 * @file PvPSpellUtils.h
 * @brief WoW 12.0 PvP Spell Utilities for Playerbot
 *
 * This file provides enterprise-grade utilities for PvP spell calculations
 * that account for WoW 12.0 API changes:
 *
 * 1. SpellPvpModifier Support:
 *    - Access PvpMultiplier from SpellEffectEntry
 *    - Apply PvP-specific damage/healing multipliers
 *    - Support for all SpellPvpModifier types (HealingAndDamage, Periodic, etc.)
 *
 * 2. SpellAttr16 Infrastructure:
 *    - Checks for new SpellAttr16 attribute flags
 *    - All 32 flags are currently UNK (undocumented) as of WoW 12.0
 *    - Infrastructure ready for future flag documentation
 *
 * Usage:
 *   float pvpDamage = PvPSpellUtils::ApplyPvPModifier(baseDamage, spellId, effectIndex);
 *   bool hasAttr16 = PvPSpellUtils::HasSpellAttr16(spellInfo, SPELL_ATTR16_UNK0);
 */

#include "Define.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellDefines.h"
#include "SharedDefines.h"
#include "DB2Stores.h"
#include "DB2Structure.h"
#include "Unit.h"
#include <cstdint>

namespace Playerbot
{

/**
 * @class PvPSpellUtils
 * @brief Static utility class for PvP spell calculations
 *
 * Provides WoW 12.0-compatible spell damage/healing calculations
 * that account for PvP modifiers and new spell attributes.
 */
class TC_GAME_API PvPSpellUtils
{
public:
    // ========================================================================
    // PvP MULTIPLIER ACCESS (SpellPvpModifier Support)
    // ========================================================================

    /**
     * @brief Get the PvP multiplier for a specific spell effect
     * @param spellId The spell ID to query
     * @param effectIndex The effect index (0-based)
     * @return PvP multiplier (1.0 if no modifier, or the actual multiplier)
     *
     * The PvpMultiplier is stored in SpellEffectEntry (DB2 data) and represents
     * the damage/healing reduction applied in PvP combat. Common values:
     * - 1.0 = No PvP reduction
     * - 0.8 = 20% reduction in PvP
     * - 0.5 = 50% reduction in PvP
     */
    static float GetPvPMultiplier(uint32 spellId, uint8 effectIndex)
    {
        // First try to get SpellInfo for the spell
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            return 1.0f;

        // Validate effect index
        if (effectIndex >= spellInfo->GetEffects().size())
            return 1.0f;

        // Access SpellEffectEntry from DB2 store
        // SpellEffectEntry stores the PvpMultiplier field
        for (SpellEffectEntry const* effectEntry : sSpellEffectStore)
        {
            if (effectEntry && effectEntry->SpellID == static_cast<int32>(spellId) &&
                effectEntry->EffectIndex == effectIndex)
            {
                return effectEntry->PvpMultiplier > 0.0f ? effectEntry->PvpMultiplier : 1.0f;
            }
        }

        return 1.0f;
    }

    /**
     * @brief Apply PvP modifier to a damage/healing value
     * @param baseValue The base damage or healing value
     * @param spellId The spell ID being cast
     * @param effectIndex The effect index
     * @param isPvPCombat Whether combat is in PvP context
     * @return Modified value with PvP multiplier applied
     */
    static float ApplyPvPModifier(float baseValue, uint32 spellId, uint8 effectIndex, bool isPvPCombat = true)
    {
        if (!isPvPCombat)
            return baseValue;

        float pvpMultiplier = GetPvPMultiplier(spellId, effectIndex);
        return baseValue * pvpMultiplier;
    }

    /**
     * @brief Check if a spell has PvP modifiers on any effect
     * @param spellId The spell ID to check
     * @return True if any effect has a PvP modifier != 1.0
     */
    static bool HasPvPModifier(uint32 spellId)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            return false;

        for (uint8 i = 0; i < spellInfo->GetEffects().size(); ++i)
        {
            float mult = GetPvPMultiplier(spellId, i);
            if (mult != 1.0f && mult > 0.0f)
                return true;
        }

        return false;
    }

    /**
     * @brief Get all PvP multipliers for a spell
     * @param spellId The spell ID to query
     * @return Vector of PvP multipliers for each effect
     */
    static std::vector<float> GetAllPvPMultipliers(uint32 spellId)
    {
        std::vector<float> multipliers;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            return multipliers;

        for (uint8 i = 0; i < spellInfo->GetEffects().size(); ++i)
        {
            multipliers.push_back(GetPvPMultiplier(spellId, i));
        }

        return multipliers;
    }

    // ========================================================================
    // SPELL ATTR16 SUPPORT (WoW 12.0 New Attributes)
    // ========================================================================

    /**
     * @brief Check if a spell has a specific SpellAttr16 flag
     * @param spellInfo The SpellInfo to check
     * @param attribute The SpellAttr16 flag to check
     * @return True if the spell has the attribute
     *
     * Note: As of WoW 12.0, all SpellAttr16 flags are UNK (undocumented).
     * This infrastructure is ready for when flags are documented.
     *
     * Known flags (all undocumented):
     * - SPELL_ATTR16_UNK0 through SPELL_ATTR16_UNK31
     */
    static bool HasSpellAttr16(SpellInfo const* spellInfo, SpellAttr16 attribute)
    {
        if (!spellInfo)
            return false;

        return spellInfo->HasAttribute(attribute);
    }

    /**
     * @brief Check if a spell has any SpellAttr16 flags set
     * @param spellInfo The SpellInfo to check
     * @return True if any Attr16 flags are set
     */
    static bool HasAnySpellAttr16(SpellInfo const* spellInfo)
    {
        if (!spellInfo)
            return false;

        // Check if AttributesEx16 has any bits set
        return spellInfo->HasAttribute(static_cast<SpellAttr16>(0xFFFFFFFF));
    }

    /**
     * @brief Get the raw SpellAttr16 bitmask for a spell
     * @param spellId The spell ID to query
     * @return The raw bitmask value (0 if not found or no flags)
     */
    static uint32 GetSpellAttr16Mask(uint32 spellId)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            return 0;

        // Access the underlying AttributesEx16 value
        // Check individual known flags to reconstruct the mask
        uint32 mask = 0;
        for (uint32 bit = 0; bit < 32; ++bit)
        {
            SpellAttr16 attr = static_cast<SpellAttr16>(1u << bit);
            if (spellInfo->HasAttribute(attr))
                mask |= (1u << bit);
        }

        return mask;
    }

    // ========================================================================
    // PVP COMBAT DETECTION
    // ========================================================================

    /**
     * @brief Check if a unit is in PvP combat
     * @param unit The unit to check
     * @return True if the unit is engaged in PvP combat
     */
    static bool IsInPvPCombat(Unit const* unit)
    {
        if (!unit)
            return false;

        // Check if unit has PvP flag
        if (!unit->IsPvP() && !unit->IsFFAPvP())
            return false;

        // Check if in arena or battleground
        if (unit->IsPlayer())
        {
            Player const* player = unit->ToPlayer();
            if (player->InArena() || player->InBattleground())
                return true;
        }

        // Check if combat target is a player
        Unit const* victim = unit->GetVictim();
        if (victim && victim->IsPlayer())
            return true;

        // Check attackers
        if (unit->IsInCombat())
        {
            // Unit is in combat, check if any attacker is a player
            for (auto const& ref : unit->GetThreatManager().GetSortedThreatList())
            {
                if (ref && ref->GetVictim() && ref->GetVictim()->IsPlayer())
                    return true;
            }
        }

        return false;
    }

    // ========================================================================
    // PVP DAMAGE/HEALING ESTIMATION
    // ========================================================================

    /**
     * @brief Estimate PvP-adjusted spell damage
     * @param spellId The spell ID
     * @param caster The caster unit
     * @param target The target unit (optional, for context)
     * @return Estimated PvP damage value
     *
     * This accounts for:
     * - Base spell damage calculation
     * - PvP multiplier from SpellEffectEntry
     * - Caster stats and modifiers
     */
    static float EstimatePvPSpellDamage(uint32 spellId, Unit const* caster, Unit const* target = nullptr)
    {
        if (!caster)
            return 0.0f;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            return 0.0f;

        float totalDamage = 0.0f;

        for (uint8 i = 0; i < spellInfo->GetEffects().size(); ++i)
        {
            SpellEffectInfo const& effect = spellInfo->GetEffect(static_cast<SpellEffIndex>(i));

            // Only consider damage effects
            if (!IsDamageEffect(effect.Effect))
                continue;

            // Calculate base damage
            int32 baseDamage = effect.CalcValue(const_cast<Unit*>(caster), nullptr, const_cast<Unit*>(target));

            // Apply PvP multiplier
            float pvpMultiplier = GetPvPMultiplier(spellId, i);
            float adjustedDamage = static_cast<float>(baseDamage) * pvpMultiplier;

            totalDamage += adjustedDamage;
        }

        return totalDamage;
    }

    /**
     * @brief Estimate PvP-adjusted spell healing
     * @param spellId The spell ID
     * @param caster The caster unit
     * @param target The target unit (optional, for context)
     * @return Estimated PvP healing value
     */
    static float EstimatePvPSpellHealing(uint32 spellId, Unit const* caster, Unit const* target = nullptr)
    {
        if (!caster)
            return 0.0f;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            return 0.0f;

        float totalHealing = 0.0f;

        for (uint8 i = 0; i < spellInfo->GetEffects().size(); ++i)
        {
            SpellEffectInfo const& effect = spellInfo->GetEffect(static_cast<SpellEffIndex>(i));

            // Only consider healing effects
            if (!IsHealingEffect(effect.Effect))
                continue;

            // Calculate base healing
            int32 baseHealing = effect.CalcValue(const_cast<Unit*>(caster), nullptr, const_cast<Unit*>(target));

            // Apply PvP multiplier
            float pvpMultiplier = GetPvPMultiplier(spellId, i);
            float adjustedHealing = static_cast<float>(baseHealing) * pvpMultiplier;

            totalHealing += adjustedHealing;
        }

        return totalHealing;
    }

    // ========================================================================
    // SPELL EFFECT TYPE HELPERS
    // ========================================================================

    /**
     * @brief Check if a spell effect is a damage effect
     * @param effect The effect type to check
     * @return True if this is a damage-dealing effect
     */
    static bool IsDamageEffect(SpellEffectName effect)
    {
        switch (effect)
        {
            case SPELL_EFFECT_SCHOOL_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
            case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
            case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
            case SPELL_EFFECT_POWER_BURN:
            case SPELL_EFFECT_ENVIRONMENTAL_DAMAGE:
            case SPELL_EFFECT_HEALTH_LEECH:
            case SPELL_EFFECT_DAMAGE_FROM_MAX_HEALTH_PCT:
                return true;
            default:
                return false;
        }
    }

    /**
     * @brief Check if a spell effect is a healing effect
     * @param effect The effect type to check
     * @return True if this is a healing effect
     */
    static bool IsHealingEffect(SpellEffectName effect)
    {
        switch (effect)
        {
            case SPELL_EFFECT_HEAL:
            case SPELL_EFFECT_HEAL_PCT:
            case SPELL_EFFECT_HEAL_MAX_HEALTH:
            case SPELL_EFFECT_HEAL_MECHANICAL:
            case SPELL_EFFECT_HEALTH_LEECH: // Also returns health to caster
                return true;
            default:
                return false;
        }
    }

    /**
     * @brief Check if a spell effect is periodic (DoT/HoT)
     * @param effect The effect info to check
     * @return True if this is a periodic effect
     */
    static bool IsPeriodicEffect(SpellEffectInfo const& effect)
    {
        return effect.ApplyAuraPeriod > 0 && effect.IsAura();
    }

    // ========================================================================
    // PVP MODIFIER TYPE CLASSIFICATION
    // ========================================================================

    /**
     * @brief Get the SpellPvpModifier type for a spell effect
     * @param spellId The spell ID
     * @param effectIndex The effect index
     * @return The appropriate SpellPvpModifier type
     *
     * SpellPvpModifier types (from SpellDefines.h):
     * - HealingAndDamage = 0: Direct damage/healing
     * - PeriodicHealingAndDamage = 1: DoTs/HoTs
     * - BonusCoefficient = 2: Coefficient adjustments
     * - Points = 4: Base point adjustments
     * - PointsIndex0-4 = 5-9: Per-effect point adjustments
     */
    static SpellPvpModifier GetPvPModifierType(uint32 spellId, uint8 effectIndex)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo || effectIndex >= spellInfo->GetEffects().size())
            return SpellPvpModifier::HealingAndDamage;

        SpellEffectInfo const& effect = spellInfo->GetEffect(static_cast<SpellEffIndex>(effectIndex));

        // Check if periodic
        if (IsPeriodicEffect(effect))
            return SpellPvpModifier::PeriodicHealingAndDamage;

        // Default to direct damage/healing modifier
        return SpellPvpModifier::HealingAndDamage;
    }

private:
    // Private constructor - static class only
    PvPSpellUtils() = delete;
    ~PvPSpellUtils() = delete;
    PvPSpellUtils(PvPSpellUtils const&) = delete;
    PvPSpellUtils& operator=(PvPSpellUtils const&) = delete;
};

} // namespace Playerbot
