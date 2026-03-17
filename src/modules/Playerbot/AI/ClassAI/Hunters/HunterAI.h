/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITY_HUNTERPLAYERAI_H
#define TRINITY_HUNTERPLAYERAI_H

#include "../ClassAI.h"
#include "../SpellValidation_WoW120.h"
#include "../../Combat/CombatBehaviorIntegration.h"
#include "Position.h"
#include "ObjectGuid.h"
#include "PetDefines.h"
#include <memory>
#include <atomic>
#include <unordered_set>
#include <vector>
#include "GameTime.h"

class Player;
class Pet;

namespace Playerbot
{

// Forward declarations for specialization classes (QW-4 FIX)
class BeastMasteryHunterRefactored;
class MarksmanshipHunterRefactored;
class SurvivalHunterRefactored;

// Type aliases for consistency with base naming
using BeastMasteryHunter = BeastMasteryHunterRefactored;
using MarksmanshipHunter = MarksmanshipHunterRefactored;
using SurvivalHunter = SurvivalHunterRefactored;

enum class HunterSpec : uint8
{
    BEAST_MASTERY = 1,
    MARKSMANSHIP = 2,
    SURVIVAL = 3
};

// Trap information tracking
struct TrapInfo
{
    uint32 spellId;
    uint32 lastUsed;
    Position position;
    uint32 duration;

    TrapInfo(uint32 spell = 0, uint32 last = 0, Position pos = Position(), uint32 dur = 30000)
        : spellId(spell), lastUsed(last), position(pos), duration(dur) {}

    bool IsReady() const { return (GameTime::GetGameTimeMS() - lastUsed) >= 30000; } // 30sec trap cooldown
    bool IsActive() const { return (GameTime::GetGameTimeMS() - lastUsed) < duration; }
};

// Pet information tracking
struct PetInfo
{
    ObjectGuid guid;
    uint32 health;
    uint32 maxHealth;
    uint32 happiness;
    PetType type;
    uint32 lastCommand;
    uint32 lastFeed;
    bool isDead;

    PetInfo() : guid(ObjectGuid::Empty), health(0), maxHealth(0), happiness(0),
                type(PetType::MAX_PET_TYPE), lastCommand(0), lastFeed(0), isDead(true) {}

    float GetHealthPct() const
    {
        return maxHealth > 0 ? (float(health) / maxHealth * 100.0f) : 0.0f;
    }
};

// Hunter spell constants defined in HunterSpecialization.h (HunterSpells enum)

// Combat metrics structure for performance tracking
struct HunterCombatMetrics
{
    ::std::atomic<uint32> shotsLanded{0};
    ::std::atomic<uint32> shotsMissed{0};
    ::std::atomic<uint32> criticalStrikes{0};
    ::std::atomic<uint32> interrupts{0};
    ::std::atomic<uint32> trapsTriggered{0};
    ::std::atomic<uint32> petCommands{0};
    ::std::atomic<uint32> focusSpent{0};
    ::std::atomic<uint32> damageDealt{0};
    float timeAtRange{0.0f};      // Not atomic - += not supported for atomic<float>
    float timeInDeadZone{0.0f};   // Not atomic - += not supported for atomic<float>

    void Reset()
    {
        shotsLanded = 0;
        shotsMissed = 0;
        criticalStrikes = 0;
        interrupts = 0;
        trapsTriggered = 0;
        petCommands = 0;
        focusSpent = 0;
        damageDealt = 0;
        timeAtRange = 0.0f;
        timeInDeadZone = 0.0f;
    }
};

class HunterAI : public ClassAI
{
public:
    explicit HunterAI(Player* bot);
    ~HunterAI();

    // Core AI interface
    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);
    void OnCombatStart(::Unit* target);
    void OnCombatEnd();
    bool HasEnoughResource(uint32 spellId);
    void ConsumeResource(uint32 spellId);
    Position GetOptimalPosition(::Unit* target);
    float GetOptimalRange(::Unit* target);

    // Hunter-specific methods
    Pet* GetPet() const;
    bool HasActivePet() const;
    HunterSpec GetCurrentSpecialization() const;

    // Combat behavior integration
    CombatBehaviorIntegration* GetCombatBehaviors() const { return _combatBehaviors.get(); }

private:
    // ========================================================================
    // QW-4 FIX: Per-instance specialization objects
    // Each bot has its own specialization object initialized with correct bot pointer
    // ========================================================================

    ::std::unique_ptr<BeastMasteryHunter> _beastMasterySpec;
    ::std::unique_ptr<MarksmanshipHunter> _marksmanshipSpec;
    ::std::unique_ptr<SurvivalHunter> _survivalSpec;

    // Delegation to specialization
    void DelegateToSpecialization(::Unit* target);

    // Initialization methods
    void InitializeCombatSystems();

    // Combat priority handlers (9 priorities from integration)
    bool HandleInterrupts(::Unit* target);
    bool HandleDefensives(::Unit* target);
    bool HandlePositioning(::Unit* target);
    bool HandlePetManagement(::Unit* target);
    bool HandleTargetSwitching(::Unit* target);
    bool HandleCrowdControl(::Unit* target);
    bool HandleAoEDecisions(::Unit* target);
    bool HandleOffensiveCooldowns(::Unit* target);
    void ExecuteNormalRotation(::Unit* target);

    // Pet management system
    void UpdatePetStatus();
    bool NeedsPetRevive() const;
    bool NeedsPetHeal() const;
    bool ShouldDismissPet() const;
    void CommandPetAttack(::Unit* target);
    void CommandPetFollow();
    void CommandPetStay();
    bool IsPetInCombat() const;
    float GetPetHealthPercent() const;
    void HealPet();
    void RevivePet();
    void CallPet();

    // Trap management
    bool CanPlaceTrap() const;
    bool ShouldPlaceFreezingTrap(::Unit* target) const;
    bool ShouldPlaceExplosiveTrap() const;
    bool ShouldPlaceTarTrap() const;
    void PlaceTrap(uint32 trapSpell, const Position& pos);
    uint32 GetBestTrapForSituation() const;

    // Range management
    bool IsInOptimalRange(::Unit* target) const;
    bool IsInDeadZone(::Unit* target) const;
    bool NeedsToKite(::Unit* target) const;
    void MaintainRange(::Unit* target);
    float GetDistanceToTarget(::Unit* target) const;

    // Hunter-specific mechanics
    void ManageAspects();
    void UpdateTracking();
    bool HasAnyAspect(); // Not const - calls non-const HasAura()
    uint32 GetCurrentAspect(); // Not const - calls non-const HasAura()
    void SwitchToCombatAspect();
    void SwitchToMovementAspect();
    bool ValidateAspectForAbility(uint32 spellId) const;
    uint32 GetOptimalAspect() const;

    // Utility methods
    bool ShouldFeignDeath() const;
    bool CanInterruptTarget(::Unit* target) const;
    ::Unit* GetBestCrowdControlTarget() const;
    uint32 GetNearbyEnemyCount(float range) const;
    bool HasFocus(uint32 amount) const;
    uint32 GetFocus() const;
    uint32 GetMaxFocus() const;
    float GetFocusPercent() const;
    void LogCombatMetrics();

    // Helper methods
    Player* GetMainTank(); // Not const - calls GetClass() on group members
    bool IsTargetDangerous(::Unit* target) const;
    bool ShouldSaveDefensives() const;
    void RecordShotResult(bool hit, bool crit);
    void RecordTrapPlacement(uint32 trapSpell);

    // Member variables
    ::std::unique_ptr<CombatBehaviorIntegration> _combatBehaviors;

    // Combat state tracking
    HunterCombatMetrics _combatMetrics;
    uint32 _lastCounterShot;
    uint32 _lastFeignDeath;
    uint32 _lastDeterrence;
    uint32 _lastDisengage;
    uint32 _lastTrapPlacement;
    uint32 _lastPetCommand;
    uint32 _lastAspectSwitch;
    uint32 _lastPetRevive;
    uint32 _lastPetHeal;

    // Pet state
    ObjectGuid _petGuid;
    bool _petNeedsHeal;
    bool _petIsAggressive;
    uint32 _petTargetSwitch;

    // Trap management
    ::std::unordered_set<ObjectGuid> _frozenTargets;
    Position _lastTrapPosition;
    uint32 _activeTrapType;

    // Performance tracking
    uint32 _updateCounter;
    uint32 _totalUpdateTime;
    uint32 _peakUpdateTime;

    // Constants
    static constexpr float OPTIMAL_RANGE_MIN = 8.0f;   // Outside dead zone
    static constexpr float OPTIMAL_RANGE_MAX = 35.0f;  // Max effective range
    static constexpr float OPTIMAL_RANGE_PREFERRED = 25.0f; // Preferred combat range
    static constexpr float DEAD_ZONE_MIN = 0.0f;       // Melee range
    static constexpr float DEAD_ZONE_MAX = 8.0f;       // Can't shoot in this range
    static constexpr float KITING_RANGE = 30.0f;       // Range when kiting
    static constexpr float TRAP_PLACEMENT_RANGE = 30.0f;
    static constexpr uint32 PET_HEAL_THRESHOLD = 50;   // Heal pet below 50% HP
    static constexpr uint32 DEFENSIVE_HEALTH_THRESHOLD = 30; // Use defensives below 30% HP
    static constexpr uint32 FEIGN_DEATH_THRESHOLD = 20; // Feign death below 20% HP

public:
    // Hunter Spell IDs
    // Hunter Spell IDs - Using central registry (WoW 12.0)
    enum HunterSpells
    {
        // Shots and Attacks
        STEADY_SHOT = WoW120Spells::Hunter::Common::STEADY_SHOT,
        ARCANE_SHOT = WoW120Spells::Hunter::Common::ARCANE_SHOT,
        MULTI_SHOT = WoW120Spells::Hunter::Common::MULTI_SHOT,
        AIMED_SHOT = WoW120Spells::Hunter::Common::AIMED_SHOT,
        KILL_SHOT = WoW120Spells::Hunter::Common::KILL_SHOT,
        EXPLOSIVE_SHOT = WoW120Spells::Hunter::Common::EXPLOSIVE_SHOT,
        SERPENT_STING = WoW120Spells::Hunter::Common::SERPENT_STING,
        CONCUSSIVE_SHOT = WoW120Spells::Hunter::Common::CONCUSSIVE_SHOT,

        // Pet Abilities
        KILL_COMMAND = WoW120Spells::Hunter::Common::KILL_COMMAND,
        MEND_PET = WoW120Spells::Hunter::Common::MEND_PET,
        REVIVE_PET = WoW120Spells::Hunter::Common::REVIVE_PET,
        CALL_PET = WoW120Spells::Hunter::Common::CALL_PET,
        MASTER_S_CALL = WoW120Spells::Hunter::Common::MASTERS_CALL,

        // Traps
        FREEZING_TRAP = WoW120Spells::Hunter::Common::FREEZING_TRAP,
        EXPLOSIVE_TRAP = WoW120Spells::Hunter::Common::EXPLOSIVE_TRAP,
        TAR_TRAP = WoW120Spells::Hunter::Common::TAR_TRAP,

        // Defensive/Utility
        HUNTER_DISENGAGE = WoW120Spells::Hunter::Common::DISENGAGE,
        FEIGN_DEATH = WoW120Spells::Hunter::Common::FEIGN_DEATH,
        DETERRENCE = WoW120Spells::Hunter::Common::ASPECT_OF_THE_TURTLE, // Renamed to Aspect of the Turtle in 12.0
        EXHILARATION = WoW120Spells::Hunter::Common::EXHILARATION,
        SCATTER_SHOT = WoW120Spells::Hunter::Common::SCATTER_SHOT,
        COUNTER_SHOT = WoW120Spells::Hunter::Common::COUNTER_SHOT,

        // Aspects (WoW 12.0 only)
        ASPECT_OF_THE_WILD = WoW120Spells::Hunter::Common::ASPECT_OF_THE_WILD,
        ASPECT_OF_THE_CHEETAH = WoW120Spells::Hunter::Common::ASPECT_OF_THE_CHEETAH,
        ASPECT_OF_THE_TURTLE = WoW120Spells::Hunter::Common::ASPECT_OF_THE_TURTLE,

        // Marks/Debuffs
        HUNTER_S_MARK = WoW120Spells::Hunter::Common::HUNTERS_MARK,

        // Cooldowns
        RAPID_FIRE = WoW120Spells::Hunter::Common::RAPID_FIRE,
        BESTIAL_WRATH = WoW120Spells::Hunter::Common::BESTIAL_WRATH,
        TRUESHOT = WoW120Spells::Hunter::Common::TRUESHOT,
        BARRAGE = WoW120Spells::Hunter::Common::BARRAGE,
        VOLLEY = WoW120Spells::Hunter::Common::VOLLEY,

        // Note: Tracking spells were removed in WoW 12.0 - no longer supported
    };
};

} // namespace Playerbot

#endif
