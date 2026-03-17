/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "../ClassAI.h"
#include "../SpellValidation_WoW120.h"
#include <memory>
#include <chrono>

namespace Playerbot
{

// Forward declarations for specialization classes (QW-4 FIX)
class BalanceDruidRefactored;
class FeralDruidRefactored;
class GuardianDruidRefactored;
class RestorationDruidRefactored;

// Type aliases for consistency with base naming
using BalanceDruid = BalanceDruidRefactored;
using FeralDruid = FeralDruidRefactored;
using GuardianDruid = GuardianDruidRefactored;
using RestorationDruid = RestorationDruidRefactored;

class TC_GAME_API DruidAI : public ClassAI
{
public:
    explicit DruidAI(Player* bot);
    ~DruidAI();

    // ClassAI interface implementation
    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);

    // Combat state callbacks
    void OnCombatStart(::Unit* target);
    void OnCombatEnd();

protected:
    // Resource management
    bool HasEnoughResource(uint32 spellId);
    void ConsumeResource(uint32 spellId);

    // Positioning
    Position GetOptimalPosition(::Unit* target);
    float GetOptimalRange(::Unit* target);


    // Druid forms for shapeshift management
    enum class DruidForm : uint8
    {
        CASTER = 0,      // Humanoid/Caster form
        HUMANOID = 0,    // Alias for CASTER
        BEAR = 1,
        CAT = 2,
        AQUATIC = 3,
        TRAVEL = 4,
        MOONKIN = 5,
        TREE_OF_LIFE = 6,
        FLIGHT = 7
    };

private:
    // ========================================================================
    // QW-4 FIX: Per-instance specialization objects
    // Each bot has its own specialization object initialized with correct bot pointer
    // ========================================================================

    ::std::unique_ptr<BalanceDruid> _balanceSpec;
    ::std::unique_ptr<FeralDruid> _feralSpec;
    ::std::unique_ptr<GuardianDruid> _guardianSpec;
    ::std::unique_ptr<RestorationDruid> _restorationSpec;

    // Form management
    DruidForm _currentForm;
    uint32 _lastFormShift;

    // Combat state tracking
    uint32 _comboPoints;
    uint32 _energy;
    uint32 _rage;
    uint32 _lastSwipe;
    uint32 _lastThrash;
    bool _hasNaturesSwiftness;

    // Defensive cooldown tracking
    uint32 _lastBarkskin;
    uint32 _lastSurvivalInstincts;
    uint32 _lastFrenziedRegen;

    // Offensive cooldown tracking
    uint32 _lastTigersFury;
    uint32 _lastBerserk;
    uint32 _lastIncarnation;
    uint32 _lastCelestialAlignment;

    void DelegateToSpecialization(::Unit* target);

    // Combat behavior integration helpers
    bool HandleInterrupts(::Unit* target);
    bool HandleDefensives();
    bool HandleTargetSwitching(::Unit*& target);
    bool HandleAoERotation(::Unit* target);
    bool HandleOffensiveCooldowns(::Unit* target);
    void HandleComboPointManagement(::Unit* target);
    void ExecuteSpecializationRotation(::Unit* target);

    // Form management helpers
    bool IsInForm(DruidForm form) const;
    bool ShiftToForm(DruidForm form);
    DruidForm GetCurrentForm() const;
    bool CanShiftToForm(DruidForm form) const;

    // Resource helpers
    void UpdateResources();
    bool HasEnoughEnergy(uint32 amount) const;
    bool HasEnoughRage(uint32 amount) const;
    bool HasEnoughMana(uint32 amount) const;
    uint32 GetComboPoints() const;

    // Spell ID constants
    // Druid Spell IDs - Using central registry (WoW 12.0)
    enum DruidSpells
    {
        // Forms
        BEAR_FORM = WoW120Spells::Druid::Common::BEAR_FORM,
        CAT_FORM = WoW120Spells::Druid::Common::CAT_FORM,
        MOONKIN_FORM = WoW120Spells::Druid::Common::MOONKIN_FORM,
        TREE_OF_LIFE = WoW120Spells::Druid::Common::TREE_OF_LIFE,
        TRAVEL_FORM = WoW120Spells::Druid::Common::TRAVEL_FORM,

        // Interrupts
        SKULL_BASH_BEAR = WoW120Spells::Druid::Common::SKULL_BASH_BEAR,
        SKULL_BASH_CAT = WoW120Spells::Druid::Common::SKULL_BASH_CAT,
        SOLAR_BEAM = WoW120Spells::Druid::Common::SOLAR_BEAM,
        TYPHOON = WoW120Spells::Druid::Common::TYPHOON,
        MIGHTY_BASH = WoW120Spells::Druid::Common::MIGHTY_BASH,

        // Defensive abilities
        BARKSKIN = WoW120Spells::Druid::Common::BARKSKIN,
        SURVIVAL_INSTINCTS = WoW120Spells::Druid::Common::SURVIVAL_INSTINCTS,
        FRENZIED_REGENERATION = WoW120Spells::Druid::Common::FRENZIED_REGENERATION,
        IRONBARK = WoW120Spells::Druid::Common::IRONBARK,
        CENARION_WARD = WoW120Spells::Druid::Common::CENARION_WARD,

        // Feral offensive
        TIGERS_FURY = WoW120Spells::Druid::Common::TIGERS_FURY,
        BERSERK_CAT = WoW120Spells::Druid::Common::BERSERK_CAT,
        INCARNATION_KING = WoW120Spells::Druid::Common::INCARNATION_KING,

        // Feral abilities
        SHRED = WoW120Spells::Druid::Common::SHRED,
        RAKE = WoW120Spells::Druid::Common::RAKE,
        RIP = WoW120Spells::Druid::Common::RIP,
        FEROCIOUS_BITE = WoW120Spells::Druid::Common::FEROCIOUS_BITE,
        SAVAGE_ROAR = WoW120Spells::Druid::Common::SAVAGE_ROAR,
        SWIPE_CAT = WoW120Spells::Druid::Common::SWIPE_CAT,
        THRASH_CAT = WoW120Spells::Druid::Common::THRASH_CAT,
        PRIMAL_WRATH = WoW120Spells::Druid::Common::PRIMAL_WRATH,

        // Guardian abilities
        MANGLE_BEAR = WoW120Spells::Druid::Common::MANGLE_BEAR,
        MAUL = WoW120Spells::Druid::Common::MAUL,
        IRONFUR = WoW120Spells::Druid::Common::IRONFUR,
        THRASH_BEAR = WoW120Spells::Druid::Common::THRASH_BEAR,
        SWIPE_BEAR = WoW120Spells::Druid::Common::SWIPE_BEAR,
        BERSERK_BEAR = WoW120Spells::Druid::Common::BERSERK_BEAR,
        INCARNATION_GUARDIAN = WoW120Spells::Druid::Common::INCARNATION_GUARDIAN,
        PULVERIZE = WoW120Spells::Druid::Common::PULVERIZE,

        // Balance abilities
        WRATH = WoW120Spells::Druid::Common::WRATH,
        STARFIRE = WoW120Spells::Druid::Common::STARFIRE,
        MOONFIRE = WoW120Spells::Druid::Common::MOONFIRE,
        SUNFIRE = WoW120Spells::Druid::Common::SUNFIRE,
        STARSURGE = WoW120Spells::Druid::Common::STARSURGE,
        STARFALL = WoW120Spells::Druid::Common::STARFALL,
        CELESTIAL_ALIGNMENT = WoW120Spells::Druid::Common::CELESTIAL_ALIGNMENT,
        INCARNATION_BALANCE = WoW120Spells::Druid::Common::INCARNATION_BALANCE,
        // NOTE: LUNAR_STRIKE and SOLAR_WRATH were removed in WoW 11.x
        // They were merged with STARFIRE and WRATH respectively

        // Restoration abilities
        REJUVENATION = WoW120Spells::Druid::Common::REJUVENATION,
        REGROWTH = WoW120Spells::Druid::Common::REGROWTH,
        LIFEBLOOM = WoW120Spells::Druid::Common::LIFEBLOOM,
        HEALING_TOUCH = WoW120Spells::Druid::Common::HEALING_TOUCH,
        WILD_GROWTH = WoW120Spells::Druid::Common::WILD_GROWTH,
        SWIFTMEND = WoW120Spells::Druid::Common::SWIFTMEND,
        TRANQUILITY = WoW120Spells::Druid::Common::TRANQUILITY,
        INCARNATION_TREE = WoW120Spells::Druid::Common::INCARNATION_TREE,
        NATURES_SWIFTNESS = WoW120Spells::Druid::Common::NATURES_SWIFTNESS,
        EFFLORESCENCE = WoW120Spells::Druid::Common::EFFLORESCENCE,

        // Utility
        REMOVE_CORRUPTION = WoW120Spells::Druid::Common::REMOVE_CORRUPTION,
        NATURES_CURE = WoW120Spells::Druid::Common::NATURES_CURE,
        REBIRTH = WoW120Spells::Druid::Common::REBIRTH,
        INNERVATE = WoW120Spells::Druid::Common::INNERVATE,
        STAMPEDING_ROAR = WoW120Spells::Druid::Common::STAMPEDING_ROAR,
        MARK_OF_THE_WILD = WoW120Spells::Druid::MARK_OF_THE_WILD
    };
};

} // namespace Playerbot
