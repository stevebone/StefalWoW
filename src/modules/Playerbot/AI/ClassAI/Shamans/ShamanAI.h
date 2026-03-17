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
#include "Position.h"
#include <unordered_map>
#include <memory>
#include <chrono>
#include <array>

// Forward declarations

namespace Playerbot
{

// Forward declarations for specialization classes (QW-4 FIX)
class ElementalShamanRefactored;
class EnhancementShamanRefactored;
class RestorationShamanRefactored;

// Type aliases for consistency with base naming
using ElementalShaman = ElementalShamanRefactored;
using EnhancementShaman = EnhancementShamanRefactored;
using RestorationShaman = RestorationShamanRefactored;

// TotemType and TotemInfo are defined in ShamanSpecialization.h

// Shaman AI implementation with specialization pattern and CombatBehaviorIntegration
class TC_GAME_API ShamanAI : public ClassAI
{
public:
    explicit ShamanAI(Player* bot);
    ~ShamanAI(); // Implemented in cpp file

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


    // Totem management types
    enum class TotemType : uint8
    {
        EARTH = 0,
        FIRE = 1,
        WATER = 2,
        AIR = 3,
        MAX_TOTEMS = 4
    };

    enum class TotemBehavior : uint8
    {
        AGGRESSIVE = 0,
        DEFENSIVE = 1,
        PASSIVE = 2
    };

    struct TotemInfo
    {
        uint32 spellId;
        TotemType type;
        TotemBehavior behavior;
        uint32 duration;
        uint32 cooldown;
        uint32 lastCast;
        ::Unit* totemUnit;
        uint32 deployTime;
        Position position;

        TotemInfo() : spellId(0), type(TotemType::EARTH), behavior(TotemBehavior::AGGRESSIVE),
                     duration(0), cooldown(0), lastCast(0), totemUnit(nullptr), deployTime(0), position() {}

        bool IsActive() const { return totemUnit != nullptr; }
        bool IsOnCooldown(uint32 currentTime) const { return (currentTime - lastCast) < cooldown; }
    };

    struct WeaponImbue
    {
        uint32 spellId;
        uint32 mainHandEnchant;
        uint32 offHandEnchant;
        uint32 duration;
        uint32 lastApplied;

        WeaponImbue() : spellId(0), mainHandEnchant(0), offHandEnchant(0), duration(0), lastApplied(0) {}

        bool IsActive(uint32 currentTime) const { return (currentTime - lastApplied) < duration; }
    };

private:
    // ========================================================================
    // QW-4 FIX: Per-instance specialization objects
    // Each bot has its own specialization object initialized with correct bot pointer
    // ========================================================================

    ::std::unique_ptr<ElementalShaman> _elementalSpec;
    ::std::unique_ptr<EnhancementShaman> _enhancementSpec;
    ::std::unique_ptr<RestorationShaman> _restorationSpec;

    // Performance tracking
    uint32 _manaSpent;
    uint32 _damageDealt;
    uint32 _healingDone;
    uint32 _totemsDeploy;
    uint32 _shocksUsed;
    ::std::unordered_map<uint32, uint32> _abilityUsage;

    // Totem management system (5 types: FIRE, EARTH, WATER, AIR, NONE)
    ::std::array<TotemInfo, 5> _activeTotems;
    uint32 _lastTotemUpdate;
    uint32 _lastTotemCheck;

    // Shock management
    uint32 _lastShockTime;
    uint32 _flameshockTarget;
    uint32 _flameshockExpiry;

    // Maelstrom/resource tracking
    uint32 _maelstromWeaponStacks;
    uint32 _lastMaelstromCheck;
    uint32 _elementalMaelstrom;

    // Cooldown tracking
    uint32 _lastWindShear;
    uint32 _lastBloodlust;
    uint32 _lastElementalMastery;
    uint32 _lastAscendance;
    uint32 _lastFireElemental;
    uint32 _lastEarthElemental;
    uint32 _lastSpiritWalk;
    uint32 _lastShamanisticRage;

    // Combat state
    bool _hasFlameShockUp;
    uint32 _lavaBurstCharges;
    bool _hasLavaSurgeProc;
    uint32 _healingStreamTotemTime;
    uint32 _chainHealBounceCount;

    // Delegation to specialization
    void DelegateToSpecialization(::Unit* target);

    // Shared shaman utilities
    void UpdateShamanBuffs();
    void UpdateTotemCheck();
    void UpdateShockRotation();

    // CombatBehaviorIntegration rotation priorities
    bool HandleInterrupts(::Unit* target);
    bool HandleDefensives();
    bool HandlePositioning(::Unit* target);
    bool HandleTotemManagement(::Unit* target);
    bool HandleTargetSwitching(::Unit* target);
    bool HandlePurgeDispel(::Unit* target);
    bool HandleAoEDecisions(::Unit* target);
    bool HandleOffensiveCooldowns(::Unit* target);
    bool HandleResourceManagement();
    bool HandleNormalRotation(::Unit* target);

    // Totem management helpers
    bool NeedsTotemRefresh(TotemType type) const;
    uint32 GetOptimalTotem(TotemType type, ::Unit* target) const;
    bool DeployTotem(uint32 spellId, TotemType type);
    void UpdateTotemPositions();
    bool IsTotemInRange(TotemType type, ::Unit* target) const;
    void RecallTotem(TotemType type);

    // Elemental-specific helpers
    bool UpdateElementalRotation(::Unit* target);
    bool HandleLavaBurst(::Unit* target);
    bool HandleFlameShock(::Unit* target);
    bool HandleChainLightning(::Unit* target);
    bool HandleEarthquake();
    bool HandleElementalBlast(::Unit* target);
    bool CheckLavaSurgeProc();
    uint32 GetElementalMaelstrom() const;

    // Enhancement-specific helpers
    bool UpdateEnhancementRotation(::Unit* target);
    bool HandleStormstrike(::Unit* target);
    bool HandleLavaLash(::Unit* target);
    bool HandleCrashLightning();
    bool HandleWindstrike(::Unit* target);
    bool HandleMaelstromWeapon();
    uint32 GetMaelstromWeaponStacks() const;
    bool ShouldUseInstantLightningBolt() const;

    // Restoration-specific helpers
    bool UpdateRestorationRotation(::Unit* target);
    bool HandleChainHeal();
    bool HandleRiptide(Player* target);
    bool HandleHealingWave(Player* target);
    bool HandleHealingRain();
    bool HandleHealingStreamTotem();
    bool HandleSpiritLink();
    Player* GetLowestHealthGroupMember() const;
    uint32 CountInjuredGroupMembers(float healthThreshold = 80.0f) const;

    // Private helper methods
    void UpdateWeaponImbues();
    void UpdateUtilityBuffs();
    void DeployInitialTotems(::Unit* target);
    void RecallCombatTotems();
    void ApplyCombatBuffs();
    void LogCombatMetrics();
    bool IsShockSpell(uint32 spellId) const;
    bool IsTotemSpell(uint32 spellId) const;
    uint32 GetOptimalWeaponImbue(bool mainHand) const;
    bool HasWeaponImbue(bool mainHand) const;
    bool NearWater() const;
    bool ShouldUseBloodlust() const;
    Player* FindGroupTank(Group* group) const;
    uint32 CalculateDamageDealt(::Unit* target) const;
    uint32 CalculateHealingDone() const;
    uint32 CalculateManaUsage() const;
    bool IsInMeleeRange(::Unit* target) const;
    bool HasFlameShockOnTarget(::Unit* target) const;
    void RefreshFlameShock(::Unit* target);
    bool ShouldUseAscendance() const;
    bool ShouldUseElementalMastery() const;
};

} // namespace Playerbot
