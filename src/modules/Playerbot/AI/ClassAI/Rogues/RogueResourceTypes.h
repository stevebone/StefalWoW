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

// Forward declarations
class Player;

namespace Playerbot
{

/**
 * @brief Dual resource type for Rogue specializations (Energy + Combo Points)
 *
 * This struct manages the two primary resources used by all Rogue specializations:
 * - Energy: Regenerating resource used to perform abilities (max 100-120 with talents)
 * - Combo Points: Building resource used for finisher abilities (max 5-7 with talents)
 *
 * Implements ComplexResource concept requirements for template-based specialization system.
 *
 * @note Shared across Assassination, Outlaw, and Subtlety specializations
 * @since WoW 12.0 (The War Within)
 */
struct EnergyComboResource
{
    // Current resource values
    uint32 energy{0};
    uint32 comboPoints{0};

    // Resource caps (modified by talents)
    uint32 maxEnergy{100};
    uint32 maxComboPoints{5};

    // ComplexResource concept requirements
    bool available{true};

    /**
     * @brief Attempt to consume energy for an ability
     * @param energyCost Amount of energy to consume
     * @return true if energy was available and consumed, false otherwise
     */
    bool Consume(uint32 energyCost)
    {
        if (energy >= energyCost)
        {
            energy -= energyCost;
            return true;
        }
        return false;
    }

    /**
     * @brief Regenerate resources over time
     * @param diff Time difference in milliseconds since last update
     *
     * Energy regenerates at 10 per second baseline (modified by haste).
     * Combo points do not regenerate naturally.
     */
    void Regenerate(uint32 diff)
    {
        // Resource regeneration logic handled by specialization implementations
        available = true;
    }

    /**
     * @brief Get current available energy
     * @return Current energy amount (0-maxEnergy)
     */
    [[nodiscard]] uint32 GetAvailable() const
    {
        return energy;
    }

    /**
     * @brief Get maximum energy capacity
     * @return Maximum energy (100 baseline, 120 with Vigor talent)
     */
    [[nodiscard]] uint32 GetMax() const
    {
        return maxEnergy;
    }

    /**
     * @brief Initialize resources for a bot
     * @param bot Player bot to initialize resources for
     *
     * Sets starting values and checks for talents that modify resource caps.
     */
    void Initialize(Player* /*bot*/)
    {
        energy = 0;
        comboPoints = 0;
        // Resource caps adjusted by specialization based on talents
    }
};

// ============================================================================
// SPECIALIZATION-SPECIFIC RESOURCE TYPES
// ============================================================================
// Each spec gets its own distinct type to avoid template instantiation conflicts
// when all three specs are included in the same translation unit (RogueAI.cpp).
// All specs share the same underlying implementation via inheritance.

/**
 * @brief Assassination Rogue resource type (Energy + Combo Points)
 * @note Distinct type for template instantiation: MeleeDpsSpecialization<ComboPointsAssassination>
 */
struct ComboPointsAssassination : public EnergyComboResource
{
    // Inherits all functionality from EnergyComboResource
    // Assassination-specific customizations can be added here if needed
};

/**
 * @brief Outlaw Rogue resource type (Energy + Combo Points)
 * @note Distinct type for template instantiation: MeleeDpsSpecialization<ComboPointsOutlaw>
 */
struct ComboPointsOutlaw : public EnergyComboResource
{
    // Inherits all functionality from EnergyComboResource
    // Outlaw-specific customizations can be added here if needed
};

/**
 * @brief Subtlety Rogue resource type (Energy + Combo Points)
 * @note Distinct type for template instantiation: MeleeDpsSpecialization<ComboPointsSubtlety>
 */
struct ComboPointsSubtlety : public EnergyComboResource
{
    // Inherits all functionality from EnergyComboResource
    // Subtlety-specific customizations can be added here if needed
};

} // namespace Playerbot
