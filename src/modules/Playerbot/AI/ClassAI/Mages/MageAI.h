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
#include "Threading/LockHierarchy.h"
#include "Position.h"
#include "../../Combat/BotThreatManager.h"
#include "../../Combat/TargetSelector.h"
#include "../../Combat/PositionManager.h"
#include "../../Combat/InterruptManager.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>
#include <chrono>

namespace Playerbot
{

// Forward declarations for specialization classes (QW-4 FIX)
class ArcaneMageRefactored;
class FireMageRefactored;
class FrostMageRefactored;

// Mage schools for spell priorities
enum class MageSchool : uint8
{
    ARCANE = 0,
    FIRE = 1,
    FROST = 2,
    GENERIC = 3
};

// Mage AI implementation with full spellcaster capabilities
class TC_GAME_API MageAI : public ClassAI
{
public:
    explicit MageAI(Player* bot);
    ~MageAI();

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

private:
    // Performance tracking
    ::std::atomic<uint32> _manaSpent{0};
    ::std::atomic<uint32> _damageDealt{0};
    ::std::atomic<uint32> _spellsCast{0};
    ::std::atomic<uint32> _interruptedCasts{0};
    ::std::atomic<uint32> _criticalHits{0};
    ::std::atomic<uint32> _successfulPolymorphs{0};
    ::std::atomic<uint32> _successfulCounterspells{0};
    uint32 _lastPolymorph;
    uint32 _lastCounterspell;
    uint32 _lastBlink;

    // Combat system integration
    ::std::unique_ptr<BotThreatManager> _threatManager;
    ::std::unique_ptr<TargetSelector> _targetSelector;
    ::std::unique_ptr<PositionManager> _positionManager;
    ::std::unique_ptr<InterruptManager> _interruptManager;

    // QW-4 FIX: Per-instance specialization objects (was static - caused cross-bot contamination)
    // Each bot now has its own specialization object initialized with correct bot pointer
    ::std::unique_ptr<ArcaneMageRefactored> _arcaneSpec;
    ::std::unique_ptr<FireMageRefactored> _fireSpec;
    ::std::unique_ptr<FrostMageRefactored> _frostSpec;

    // Shared utility tracking
    ::std::unordered_map<ObjectGuid, uint32> _polymorphTargets;
    ::std::unordered_map<ObjectGuid, uint32> _slowTargets;
    uint32 _lastManaShield;
    uint32 _lastIceBarrier;

    void UpdateSpecialization();
    // Mana management
    bool HasEnoughMana(uint32 amount);
    uint32 GetMana();
    uint32 GetMaxMana();
    float GetManaPercent();
    void OptimizeManaUsage();
    bool ShouldConserveMana();
    void UseManaRegeneration();

    // Buff management
    void UpdateMageBuffs();
    void CastMageArmor();
    void CastManaShield();
    void CastIceBarrier();
    void CastArcaneIntellect();
    void UpdateArmorSpells();

    // Defensive abilities
    void UseDefensiveAbilities();
    void UseBlink();
    void UseInvisibility();
    void UseIceBlock();
    void UseColdSnap();
    void UseBarrierSpells();

    // Offensive cooldowns
    void UseOffensiveCooldowns();
    void UseArcanePower();
    void UseCombustion();
    void UseIcyVeins();
    void UsePresenceOfMind();
    void UseMirrorImage();

    // Crowd control abilities
    void UseCrowdControl(::Unit* target);
    void UsePolymorph(::Unit* target);
    void UseFrostNova();
    void UseCounterspell(::Unit* target);
    void UseBanish(::Unit* target);

    // AoE abilities
    void UseAoEAbilities(const ::std::vector<::Unit*>& enemies);
    void UseBlizzard(const ::std::vector<::Unit*>& enemies);
    void UseFlamestrike(const ::std::vector<::Unit*>& enemies);
    void UseArcaneExplosion(const ::std::vector<::Unit*>& enemies);
    void UseConeOfCold(const ::std::vector<::Unit*>& enemies);

    // Positioning and movement
    void UpdateMagePositioning();
    bool IsAtOptimalRange(::Unit* target);
    bool NeedsToKite(::Unit* target);
    void PerformKiting(::Unit* target);
    bool IsInDanger();
    void FindSafeCastingPosition();
    Position GetSafeCastingPosition(); // Helper method that returns Position
    uint32 GetNearbyEnemyCount(float range) const;

    // Targeting and priorities
    ::Unit* GetBestPolymorphTarget();
    ::Unit* GetBestCounterspellTarget();
    ::Unit* GetBestAoETarget();
    bool ShouldInterrupt(::Unit* target);
    bool CanPolymorphSafely(::Unit* target);

    // Delegation to specialization
    void DelegateToSpecialization(::Unit* target);

    // Advanced spell effectiveness tracking
    void RecordSpellCast(uint32 spellId, ::Unit* target);
    void RecordSpellHit(uint32 spellId, ::Unit* target, uint32 damage);
    void RecordSpellCrit(uint32 spellId, ::Unit* target, uint32 damage);
    void RecordSpellResist(uint32 spellId, ::Unit* target);
    void RecordInterruptAttempt(uint32 spellId, ::Unit* target, bool success);
    void AnalyzeCastingEffectiveness();
    float CalculateSpellEfficiency(uint32 spellId);
    void OptimizeSpellPriorities();

    // Combat metrics and analytics
    struct CombatMetrics {
        ::std::atomic<uint32> totalDamage{0};
        ::std::atomic<uint32> totalHealing{0};
        ::std::atomic<uint32> totalManaSpent{0};
        ::std::atomic<float> averageCastTime{0.0f};
        ::std::atomic<float> criticalHitRate{0.0f};
        ::std::atomic<float> interruptSuccessRate{0.0f};
        ::std::chrono::steady_clock::time_point combatStartTime;
        ::std::chrono::steady_clock::time_point lastMetricsUpdate;
        void Reset()
        {
            totalDamage = 0; totalHealing = 0; totalManaSpent = 0;
            averageCastTime = 0.0f; criticalHitRate = 0.0f; interruptSuccessRate = 0.0f;
            combatStartTime = ::std::chrono::steady_clock::now();
            lastMetricsUpdate = combatStartTime;
        }
    } _combatMetrics;

    // Mage spell IDs - Using central registry (WoW 12.0)
    static constexpr uint32 ARCANE_MISSILES = WoW120Spells::Mage::Common::ARCANE_MISSILES;
    static constexpr uint32 ARCANE_BLAST = WoW120Spells::Mage::Common::ARCANE_BLAST;
    static constexpr uint32 ARCANE_BARRAGE = WoW120Spells::Mage::Common::ARCANE_BARRAGE;
    static constexpr uint32 ARCANE_ORB = WoW120Spells::Mage::Common::ARCANE_ORB;
    static constexpr uint32 ARCANE_POWER = WoW120Spells::Mage::Common::ARCANE_POWER;
    static constexpr uint32 ARCANE_INTELLECT = WoW120Spells::Mage::Common::ARCANE_INTELLECT;
    static constexpr uint32 ARCANE_EXPLOSION = WoW120Spells::Mage::Common::ARCANE_EXPLOSION;

    // Fire spells
    static constexpr uint32 FIREBALL = WoW120Spells::Mage::Common::FIREBALL;
    static constexpr uint32 FIRE_BLAST = WoW120Spells::Mage::Common::FIRE_BLAST;
    static constexpr uint32 PYROBLAST = WoW120Spells::Mage::Common::PYROBLAST;
    static constexpr uint32 FLAMESTRIKE = WoW120Spells::Mage::Common::FLAMESTRIKE;
    static constexpr uint32 SCORCH = WoW120Spells::Mage::Common::SCORCH;
    static constexpr uint32 COMBUSTION = WoW120Spells::Mage::Common::COMBUSTION;
    static constexpr uint32 LIVING_BOMB = WoW120Spells::Mage::Common::LIVING_BOMB;
    static constexpr uint32 DRAGON_BREATH = WoW120Spells::Mage::Common::DRAGONS_BREATH;

    // Frost spells
    static constexpr uint32 FROSTBOLT = WoW120Spells::Mage::Common::FROSTBOLT;
    static constexpr uint32 ICE_LANCE = WoW120Spells::Mage::Common::ICE_LANCE;
    static constexpr uint32 FROZEN_ORB = WoW120Spells::Mage::Common::FROZEN_ORB;
    static constexpr uint32 BLIZZARD = WoW120Spells::Mage::Common::BLIZZARD;
    static constexpr uint32 CONE_OF_COLD = WoW120Spells::Mage::Common::CONE_OF_COLD;
    static constexpr uint32 ICY_VEINS = WoW120Spells::Mage::Common::ICY_VEINS;
    static constexpr uint32 WATER_ELEMENTAL = WoW120Spells::Mage::Common::SUMMON_WATER_ELEMENTAL;
    static constexpr uint32 ICE_BARRIER = WoW120Spells::Mage::Common::ICE_BARRIER;
    static constexpr uint32 FROST_NOVA = WoW120Spells::Mage::Common::FROST_NOVA;

    // Crowd control
    static constexpr uint32 POLYMORPH = WoW120Spells::Mage::Common::POLYMORPH;
    static constexpr uint32 COUNTERSPELL = WoW120Spells::Mage::Common::COUNTERSPELL;

    // Defensive abilities
    static constexpr uint32 BLINK = WoW120Spells::Mage::Common::BLINK;
    static constexpr uint32 SHIMMER = WoW120Spells::Mage::Common::SHIMMER;
    static constexpr uint32 INVISIBILITY = WoW120Spells::Mage::Common::INVISIBILITY;
    static constexpr uint32 GREATER_INVISIBILITY = WoW120Spells::Mage::Common::GREATER_INVISIBILITY;
    static constexpr uint32 ICE_BLOCK = WoW120Spells::Mage::Common::ICE_BLOCK;
    static constexpr uint32 BLAZING_BARRIER = WoW120Spells::Mage::Common::BLAZING_BARRIER;

    // Utility
    static constexpr uint32 MIRROR_IMAGE = WoW120Spells::Mage::MIRROR_IMAGE;
    static constexpr uint32 PRESENCE_OF_MIND = WoW120Spells::Mage::Common::PRESENCE_OF_MIND;
    static constexpr uint32 TIME_WARP = WoW120Spells::Mage::Common::TIME_WARP;
    static constexpr uint32 TELEPORT_STORMWIND = WoW120Spells::Mage::Common::TELEPORT_STORMWIND;
    static constexpr uint32 TELEPORT_IRONFORGE = WoW120Spells::Mage::Common::TELEPORT_IRONFORGE;
    static constexpr uint32 PORTAL_STORMWIND = WoW120Spells::Mage::Common::PORTAL_STORMWIND;
    static constexpr uint32 PORTAL_IRONFORGE = WoW120Spells::Mage::Common::PORTAL_IRONFORGE;

    // Conjure spells
    static constexpr uint32 CONJURE_REFRESHMENT = WoW120Spells::Mage::CONJURE_REFRESHMENT;

    // Helper methods
    bool IsChanneling();
    bool IsCasting();

    // Spell school mappings
    static inline const ::std::unordered_map<uint32, MageSchool> _spellSchools = {
        // Arcane spells
        {ARCANE_MISSILES, MageSchool::ARCANE},
        {ARCANE_BLAST, MageSchool::ARCANE},
        {ARCANE_BARRAGE, MageSchool::ARCANE},
        {ARCANE_ORB, MageSchool::ARCANE},
        {ARCANE_POWER, MageSchool::ARCANE},
        {ARCANE_INTELLECT, MageSchool::ARCANE},
        {ARCANE_EXPLOSION, MageSchool::ARCANE},

        // Fire spells
        {FIREBALL, MageSchool::FIRE},
        {FIRE_BLAST, MageSchool::FIRE},
        {PYROBLAST, MageSchool::FIRE},
        {FLAMESTRIKE, MageSchool::FIRE},
        {SCORCH, MageSchool::FIRE},
        {COMBUSTION, MageSchool::FIRE},
        {LIVING_BOMB, MageSchool::FIRE},
        {DRAGON_BREATH, MageSchool::FIRE},
        {BLAZING_BARRIER, MageSchool::FIRE},

        // Frost spells
        {FROSTBOLT, MageSchool::FROST},
        {ICE_LANCE, MageSchool::FROST},
        {FROZEN_ORB, MageSchool::FROST},
        {BLIZZARD, MageSchool::FROST},
        {CONE_OF_COLD, MageSchool::FROST},
        {ICY_VEINS, MageSchool::FROST},
        {WATER_ELEMENTAL, MageSchool::FROST},
        {ICE_BARRIER, MageSchool::FROST},
        {FROST_NOVA, MageSchool::FROST},

        // Generic utility spells
        {POLYMORPH, MageSchool::GENERIC},
        {COUNTERSPELL, MageSchool::GENERIC},
        {BLINK, MageSchool::GENERIC},
        {SHIMMER, MageSchool::GENERIC},
        {INVISIBILITY, MageSchool::GENERIC},
        {GREATER_INVISIBILITY, MageSchool::GENERIC},
        {ICE_BLOCK, MageSchool::GENERIC},
        {MIRROR_IMAGE, MageSchool::GENERIC},
        {TIME_WARP, MageSchool::GENERIC}
    };
};

// Utility class for mage spell calculations
class TC_GAME_API MageSpellCalculator
{
public:
    // Damage calculations
    static uint32 CalculateFireballDamage(Player* caster, ::Unit* target);
    MageSchool GetSpellSchool(uint32 spellId);
    uint32 GetSpellCastTime(uint32 spellId);
    bool IsSpellInstant(uint32 spellId);

    // Optimization helpers
    void OptimizeForSpecialization();
    bool HasTalent(uint32 talentId);

    // Threat and aggro management
    void ManageThreat();
    bool HasTooMuchThreat();
    void ReduceThreat();

    // Advanced emergency responses
    void HandleEmergencySituation();
    bool IsInCriticalDanger();
    void UseEmergencyEscape();
    void HandleMultipleEnemies(const std::vector<::Unit*>& enemies);
    void HandleLowManaEmergency();
    void HandleHighThreatSituation();
    void ExecuteEmergencyTeleport();

    // Advanced combat AI
    void UpdateAdvancedCombatLogic(::Unit* target);
    void OptimizeCastingSequence(::Unit* target);
    void ManageResourceEfficiency();
    void HandleCombatPhaseTransitions();
    ::Unit* SelectOptimalTarget(const std::vector<::Unit*>& enemies);
    void ExecuteAdvancedRotation(::Unit* target);

    // Spell school mastery
    void UpdateSchoolMastery();
    float GetSchoolMasteryBonus(MageSchool school);
    void AdaptToTargetResistances(::Unit* target);
    MageSchool GetMostEffectiveSchool(::Unit* target);

    // Predictive casting
    void PredictEnemyMovement(::Unit* target);
    void PrecastSpells(::Unit* target);
    void HandleMovingTargets(::Unit* target);
    void OptimizeInstantCasts();

    // Performance optimization
    void UpdatePerformanceMetrics(uint32 diff);
    void OptimizeCastingSequence();

    // Constants
    static constexpr float OPTIMAL_CASTING_RANGE = 30.0f;
    static constexpr float MINIMUM_SAFE_RANGE = 15.0f;
    static constexpr float KITING_RANGE = 20.0f;
    static constexpr uint32 MAX_ARCANE_CHARGES = 4;
    static constexpr uint32 POLYMORPH_COOLDOWN = 30000; // 30 seconds
    static constexpr uint32 COUNTERSPELL_COOLDOWN = 24000; // 24 seconds
    static constexpr uint32 BLINK_COOLDOWN = 15000; // 15 seconds
    static constexpr float MANA_CONSERVATION_THRESHOLD = 0.3f; // 30%
    static constexpr float MANA_EMERGENCY_THRESHOLD = 0.15f; // 15%

public:
    static uint32 CalculateFrostboltDamage(Player* caster, ::Unit* target);
    static uint32 CalculateArcaneMissilesDamage(Player* caster, ::Unit* target);

    // Mana cost calculations
    static uint32 CalculateSpellManaCost(uint32 spellId, Player* caster);
    static uint32 ApplyArcanePowerBonus(uint32 damage, Player* caster);

    // Cast time calculations
    static uint32 CalculateCastTime(uint32 spellId, Player* caster);
    static float GetHasteModifier(Player* caster);

    // Critical hit calculations
    static float CalculateCritChance(uint32 spellId, Player* caster, ::Unit* target);
    static bool WillCriticalHit(uint32 spellId, Player* caster, ::Unit* target);

    // Resistance calculations
    static float CalculateResistance(uint32 spellId, Player* caster, ::Unit* target);
    static uint32 ApplyResistance(uint32 damage, float resistance);

    // Specialization optimization (moved to template system)

private:
    // Cache for spell data
    static inline std::unordered_map<uint32, uint32> _baseDamageCache;
    static inline std::unordered_map<uint32, uint32> _manaCostCache;
    static inline std::unordered_map<uint32, uint32> _castTimeCache;
    // DEADLOCK FIX: Changed to recursive_mutex to allow same thread to lock multiple times
    // Prevents "resource deadlock would occur" error during UpdateRotation
    static inline Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _cacheMutex;

    static void CacheSpellData(uint32 spellId);
};

} // namespace Playerbot
