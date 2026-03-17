/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Marksmanship Hunter Specialization - REFACTORED
 *
 * This demonstrates the complete migration of Marksmanship Hunter to the template-based
 * architecture, eliminating code duplication while maintaining full functionality.
 *
 * Marksmanship focuses on precise, powerful shots with careful aim and timing,
 * maximizing damage through proc management and burst windows.
 */

#pragma once


#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "../CombatSpecializationTemplates.h"
#include "../ResourceTypes.h"
#include "../SpellValidation_WoW120.h"
#include "../HeroTalentDetector.h"      // Hero talent tree detection
#include "Pet.h"
#include "MotionMaster.h"
#include "CharmInfo.h"
#include "HunterAI.h"
#include <unordered_map>
#include <deque>

// Phase 5 Integration: Decision Systems
#include "../../Decision/ActionPriorityQueue.h"
#include "../../Decision/BehaviorTree.h"
#include "../BotAI.h"

namespace Playerbot
{

// Forward declarations
struct TrapInfo;


// Import BehaviorTree helper functions (avoid conflict with Playerbot::Action)
using bot::ai::Sequence;
using bot::ai::Selector;
using bot::ai::Condition;
using bot::ai::Inverter;
using bot::ai::Repeater;
using bot::ai::NodeStatus;
using bot::ai::SpellPriority;
using bot::ai::SpellCategory;

// Note: bot::ai::Action() conflicts with Playerbot::Action, use bot::ai::Action() explicitly
// ============================================================================
// MARKSMANSHIP HUNTER SPELL IDs (WoW 12.0 - The War Within)
// Using centralized spell registry from SpellValidation_WoW120.h
// ============================================================================

enum MarksmanshipSpells
{
    // Core Abilities - Using central registry: WoW120Spells::Hunter::Marksmanship
    SPELL_AIMED_SHOT           = WoW120Spells::Hunter::Marksmanship::AIMED_SHOT_MM,
    SPELL_RAPID_FIRE           = WoW120Spells::Hunter::Marksmanship::RAPID_FIRE_MM,
    SPELL_STEADY_SHOT          = WoW120Spells::Hunter::Marksmanship::STEADY_SHOT_MM,
    SPELL_ARCANE_SHOT          = WoW120Spells::Hunter::Marksmanship::ARCANE_SHOT_MM,
    SPELL_TRUESHOT             = WoW120Spells::Hunter::Marksmanship::TRUESHOT,
    SPELL_DOUBLE_TAP           = WoW120Spells::Hunter::Marksmanship::DOUBLE_TAP,

    // AoE Abilities - Using central registry: WoW120Spells::Hunter
    SPELL_MULTISHOT_MM         = WoW120Spells::Hunter::MULTI_SHOT,
    SPELL_EXPLOSIVE_SHOT       = WoW120Spells::Hunter::Marksmanship::EXPLOSIVE_SHOT,
    SPELL_VOLLEY               = WoW120Spells::Hunter::Marksmanship::VOLLEY,

    // Procs and Buffs - Using central registry: WoW120Spells::Hunter::Marksmanship
    SPELL_PRECISE_SHOTS        = WoW120Spells::Hunter::Marksmanship::PRECISE_SHOTS,
    SPELL_TRICK_SHOTS          = WoW120Spells::Hunter::Marksmanship::TRICK_SHOTS,
    SPELL_LETHAL_SHOTS         = WoW120Spells::Hunter::Marksmanship::LETHAL_SHOTS,
    SPELL_CAREFUL_AIM          = WoW120Spells::Hunter::Marksmanship::CAREFUL_AIM,

    // Utility - Using central registry: WoW120Spells::Hunter
    SPELL_HUNTERS_MARK_MM      = WoW120Spells::Hunter::HUNTERS_MARK,
    SPELL_BINDING_SHOT         = WoW120Spells::Hunter::Marksmanship::BINDING_SHOT,
    SPELL_SCATTER_SHOT         = WoW120Spells::Hunter::Marksmanship::SCATTER_SHOT,
    SPELL_BURSTING_SHOT        = WoW120Spells::Hunter::Marksmanship::BURSTING_SHOT,
    SPELL_COUNTER_SHOT_MM      = WoW120Spells::Hunter::COUNTER_SHOT,

    // Defensives - Using central registry: WoW120Spells::Hunter
    SPELL_ASPECT_TURTLE        = WoW120Spells::Hunter::ASPECT_OF_THE_TURTLE,
    SPELL_EXHILARATION_MM      = WoW120Spells::Hunter::EXHILARATION,
    SPELL_SURVIVAL_TACTICS     = WoW120Spells::Hunter::Marksmanship::SURVIVAL_TACTICS,

    // Pet (minimal for MM) - Using central registry: WoW120Spells::Hunter
    SPELL_CALL_PET_MM          = WoW120Spells::Hunter::CALL_PET_1,
    SPELL_DISMISS_PET          = WoW120Spells::Hunter::DISMISS_PET,
    SPELL_LONE_WOLF            = WoW120Spells::Hunter::Marksmanship::LONE_WOLF
};

/**
 * Precise Shots Tracking System
 *
 * Manages the Precise Shots mechanic which grants instant Arcane Shots
 * after casting Aimed Shot.
 */
class PreciseShotsTracker
{
public:
    PreciseShotsTracker()
        : _charges(0)
        , _expireTime(0)
        , _aimedShotWindup(0)
    {
    }

    void OnAimedShotCast()
    {
        _charges = 2; // Grants 2 charges of Precise Shots
        _expireTime = GameTime::GetGameTimeMS() + 15000; // 15 second duration
    }

    bool HasCharges() const
    {
        if (GameTime::GetGameTimeMS() > _expireTime)

            return false;
        return _charges > 0;
    }

    void ConsumeCharge()
    {
        if (_charges > 0)

            _charges--;
    }

    uint32 GetCharges() const { return _charges; }

    void Reset()
    {
        _charges = 0;
        _expireTime = 0;
    }

private:
    CooldownManager _cooldowns;
    uint32 _charges;
    uint32 _expireTime;
    uint32 _aimedShotWindup;
};

/**
 * Marksmanship Cast Time Management
 *
 * Handles abilities with cast times and channels specific to MM
 */
class MarksmanshipCastManager
{
public:
    explicit MarksmanshipCastManager(Player* bot)
        : _bot(bot)
        , _isCasting(false)
        , _castEndTime(0)
        , _currentCastSpell(0)
        , _isChanneling(false)
        , _channelEndTime(0)
    {
    }

    bool CanStartCast() const
    {
        // Check if bot can cast (not already casting, channeling, or moving)
        return !_isCasting && !_isChanneling && !_bot->HasUnitMovementFlag(MOVEMENTFLAG_FORWARD);
    }

    void StartAimedShot(Unit* target)
    {
        if (!CanStartCast())

            return;

        _isCasting = true;
        _castEndTime = GameTime::GetGameTimeMS() + 2500; // 2.5 second base cast
        _currentCastSpell = SPELL_AIMED_SHOT;

        // Apply Careful Aim bonus if target is above 70% or below 20%
        if (target->GetHealthPct() > 70.0f || target->GetHealthPct() < 20.0f)
        {

            _castEndTime -= 500; // Faster cast with Careful Aim
        }
    }

    void StartRapidFire()
    {
        if (!CanStartCast())

            return;

        _isChanneling = true;
        _channelEndTime = GameTime::GetGameTimeMS() + 3000; // 3 second channel
        _currentCastSpell = SPELL_RAPID_FIRE;
    }

    void Update()
    {
        uint32 currentTime = GameTime::GetGameTimeMS();

        if (_isCasting && currentTime >= _castEndTime)
        {

            _isCasting = false;

            _castEndTime = 0;

            _currentCastSpell = 0;
        }

        if (_isChanneling && currentTime >= _channelEndTime)
        {

            _isChanneling = false;

            _channelEndTime = 0;

            _currentCastSpell = 0;
        }
    }

    bool IsBusy() const { return _isCasting || _isChanneling; }
    bool IsCasting() const { return _isCasting; }
    bool IsChanneling() const { return _isChanneling; }
    uint32 GetCurrentCast() const { return _currentCastSpell; }

    void InterruptCast()
    {
        _isCasting = false;
        _isChanneling = false;
        _castEndTime = 0;
        _channelEndTime = 0;
        _currentCastSpell = 0;
    }

private:
    Player* _bot;
    bool _isCasting;
    uint32 _castEndTime;
    uint32 _currentCastSpell;
    bool _isChanneling;
    uint32 _channelEndTime;
};

/**
 * Refactored Marksmanship Hunter using template architecture
 *
 * Key features:
 * - Inherits from RangedDpsSpecialization<FocusResource> for role defaults
 * - Cast time management for Aimed Shot
 * - Rapid Fire channel optimization
 * - Precise Shots proc tracking
 * - Double Tap burst window management
 * - Minimal pet usage (Lone Wolf preference)
 */
class MarksmanshipHunterRefactored : public RangedDpsSpecialization<FocusResource>
{
public:
    using Base = RangedDpsSpecialization<FocusResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::CanUseAbility;
    using Base::ConsumeResource;
    using Base::_resource;
    explicit MarksmanshipHunterRefactored(Player* bot)
        : RangedDpsSpecialization<FocusResource>(bot)
        , _preciseShotsTracker()
        , _castManager(bot)
        , _trueshotActive(false)
        , _trueshotEndTime(0)
        , _doubleTapActive(false)
        , _doubleTapEndTime(0)
        , _trickShotsActive(false)
        , _lastAimedShot(0)
        , _lastRapidFire(0)
        , _lastSteadyShot(0)
        , _loneWolfActive(false)
    {
        // Focus regeneration handled by base template (MM has base 15 focus/sec)
        // Resource regeneration is managed by the FocusResource type

        // Setup MM-specific cooldown tracking
        // Check for Lone Wolf preference
        CheckLoneWolfStatus();

        // Phase 5 Integration: Initialize decision systems
        InitializeMarksmanshipMechanics();
    }

    // ========================================================================
    // CORE ROTATION - Marksmanship specific logic
    // ========================================================================

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !target->IsAlive() || !target->IsHostileTo(this->GetBot()))

            return;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero talent rotation branches
        if (_heroTalents.IsTree(HeroTalentTree::SENTINEL))
        {
            // Sentinel: Sentinel Owl for enhanced precision strikes
            if (this->CanCastSpell(WoW120Spells::Hunter::Marksmanship::SENTINEL_OWL, target))
            {
                this->CastSpell(WoW120Spells::Hunter::Marksmanship::SENTINEL_OWL, target);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::DARK_RANGER))
        {
            // Dark Ranger: Black Arrow for shadow-infused damage
            if (this->CanCastSpell(WoW120Spells::Hunter::Marksmanship::MM_BLACK_ARROW, target))
            {
                this->CastSpell(WoW120Spells::Hunter::Marksmanship::MM_BLACK_ARROW, target);
                return;
            }
        }

        // Update cast manager
        _castManager.Update();

        // Don't start new casts while busy
        if (_castManager.IsBusy())

            return;

        // Update MM-specific mechanics
        UpdateMarksmanshipState();

        // Check for AoE situation
        uint32 enemyCount = this->GetEnemiesInRange(40.0f);
        if (enemyCount >= 3)
        {

            ExecuteAoERotation(target);

            return;
        }

        // Single target rotation
        ExecuteSingleTargetRotation(target);
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();        // Maintain Lone Wolf (dismiss pet if we have one)
        if (_loneWolfActive && !bot->GetPetGUID().IsEmpty())
        {

            bot->CastSpell(CastSpellTargetArg(bot), SPELL_DISMISS_PET);
        }

        // Use Aspect of the Turtle for survival
        if (bot->GetHealthPct() < 30.0f && this->CanUseAbility(SPELL_ASPECT_TURTLE))
        {

            this->CastSpell(SPELL_ASPECT_TURTLE, bot);
        }        // Use Exhilaration for healing
        if (bot->GetHealthPct() < 50.0f && this->CanUseAbility(SPELL_EXHILARATION_MM))
        {

            this->CastSpell(SPELL_EXHILARATION_MM, bot);
        }

        // Apply Hunter's Mark to current target
        if (Unit* target = bot->GetVictim())        {

            if (!target->HasAura(SPELL_HUNTERS_MARK_MM) && this->CanUseAbility(SPELL_HUNTERS_MARK_MM))

            {

                this->CastSpell(SPELL_HUNTERS_MARK_MM, target);

            }
        }
    }

    void OnInterruptRequired(::Unit* target, uint32 /*spellId*/)
    {
        // Interrupt current cast if needed
        if (_castManager.IsBusy())

            _castManager.InterruptCast();

        if (this->CanUseAbility(SPELL_COUNTER_SHOT_MM))
        {

            this->CastSpell(SPELL_COUNTER_SHOT_MM, target);
        }
    }

    void OnMovementRequired()
    {
        // Interrupt any casts when we need to move
        if (_castManager.IsBusy())

            _castManager.InterruptCast();
    }

protected:
    // ========================================================================
    // RESOURCE MANAGEMENT OVERRIDE
    // ========================================================================

    uint32 GetResourceCost(uint32 spellId)
    {
        switch (spellId)
        {

            case SPELL_AIMED_SHOT:
            return 35;

            case SPELL_RAPID_FIRE:
            return 30;

            case SPELL_STEADY_SHOT:
            return 0;   // Generates 10 focus

            case SPELL_ARCANE_SHOT:
            return 20;

            case SPELL_MULTISHOT_MM:
            return 20;

            case SPELL_EXPLOSIVE_SHOT:   return 20;

            case SPELL_VOLLEY:
            return 45;

            case SPELL_BINDING_SHOT:
            return 0;

            case SPELL_BURSTING_SHOT:
            return 10;

            case SPELL_SCATTER_SHOT:
            return 0;

            default:
            return 15;
        }
    }

    // ========================================================================
    // MARKSMANSHIP SPECIFIC ROTATION LOGIC
    // ========================================================================

    void ExecuteSingleTargetRotation(::Unit* target)
    {
        uint32 currentFocus = this->_resource;

        // Priority 1: Trueshot for major burst
        if (ShouldUseTrueshot(target) && this->CanUseAbility(SPELL_TRUESHOT))
        {

            this->CastSpell(SPELL_TRUESHOT, this->GetBot());

            _trueshotActive = true;

            _trueshotEndTime = GameTime::GetGameTimeMS() + 15000;

            return;
        }

        // Priority 2: Double Tap during Trueshot
        if (_trueshotActive && this->CanUseAbility(SPELL_DOUBLE_TAP))
        {

            this->CastSpell(SPELL_DOUBLE_TAP, this->GetBot());

            _doubleTapActive = true;

            _doubleTapEndTime = GameTime::GetGameTimeMS() + 3000;

            return;
        }

        // Priority 3: Rapid Fire during burst windows
        if (ShouldUseRapidFire() && currentFocus >= 30 && this->CanUseAbility(SPELL_RAPID_FIRE))
        {

            _castManager.StartRapidFire();

            this->CastSpell(SPELL_RAPID_FIRE, target);

            _lastRapidFire = GameTime::GetGameTimeMS();

            this->ConsumeResource(30);

            return;
        }

        // Priority 4: Aimed Shot (primary nuke)
        if (currentFocus >= 35 && this->CanUseAbility(SPELL_AIMED_SHOT))
        {

            if (_castManager.CanStartCast())

            {

                _castManager.StartAimedShot(target);

                this->CastSpell(SPELL_AIMED_SHOT, target);

                _lastAimedShot = GameTime::GetGameTimeMS();

                _preciseShotsTracker.OnAimedShotCast();

                this->ConsumeResource(35);

                return;

            }
        }

        // Priority 5: Arcane Shot with Precise Shots
        if (_preciseShotsTracker.HasCharges() && currentFocus >= 20)
        {

            this->CastSpell(SPELL_ARCANE_SHOT, target);

            _preciseShotsTracker.ConsumeCharge();

            this->ConsumeResource(20);

            return;
        }

        // Priority 6: Explosive Shot if available
        if (currentFocus >= 20 && this->CanUseAbility(SPELL_EXPLOSIVE_SHOT))
        {

            this->CastSpell(SPELL_EXPLOSIVE_SHOT, target);

            this->ConsumeResource(20);

            return;
        }

        // Priority 7: Steady Shot to generate focus
        if (currentFocus < 70)
        {

            this->CastSpell(SPELL_STEADY_SHOT, target);

            _lastSteadyShot = GameTime::GetGameTimeMS();

            this->_resource = ::std::min<uint32>(this->_resource + 10, 100);

            return;
        }

        // Priority 8: Arcane Shot as filler at high focus
        if (currentFocus >= 80)
        {

            this->CastSpell(SPELL_ARCANE_SHOT, target);

            this->ConsumeResource(20);

            return;
        }
    }

    void ExecuteAoERotation(::Unit* target)
    {
        uint32 currentFocus = this->_resource;

        // Enable Trick Shots for AoE
        if (!_trickShotsActive)
        {

            _trickShotsActive = true;
        }

        // Priority 1: Volley for sustained AoE
        if (currentFocus >= 45 && this->CanUseAbility(SPELL_VOLLEY))
        {

            this->CastSpell(SPELL_VOLLEY, target);

            this->ConsumeResource(45);

            return;
        }

        // Priority 2: Explosive Shot for burst AoE
        if (currentFocus >= 20 && this->CanUseAbility(SPELL_EXPLOSIVE_SHOT))
        {

            this->CastSpell(SPELL_EXPLOSIVE_SHOT, target);

            this->ConsumeResource(20);

            return;
        }

        // Priority 3: Multi-Shot to spread damage
        if (currentFocus >= 20)
        {

            this->CastSpell(SPELL_MULTISHOT_MM, target);

            this->ConsumeResource(20);

            return;
        }

        // Priority 4: Rapid Fire for cleave
        if (currentFocus >= 30 && this->CanUseAbility(SPELL_RAPID_FIRE))
        {

            _castManager.StartRapidFire();

            this->CastSpell(SPELL_RAPID_FIRE, target);

            this->ConsumeResource(30);

            return;
        }

        // Priority 5: Steady Shot for focus
        if (currentFocus < 40)
        {

            this->CastSpell(SPELL_STEADY_SHOT, target);

            this->_resource = ::std::min<uint32>(this->_resource + 10, 100);

            return;
        }
    }

private:
    // ========================================================================
    // MARKSMANSHIP STATE MANAGEMENT
    // ========================================================================

    void UpdateMarksmanshipState()
    {
        uint32 currentTime = GameTime::GetGameTimeMS();

        // Check Trueshot expiry
        if (_trueshotActive && currentTime > _trueshotEndTime)
        {

            _trueshotActive = false;

            _trueshotEndTime = 0;
        }

        // Check Double Tap expiry
        if (_doubleTapActive && currentTime > _doubleTapEndTime)
        {

            _doubleTapActive = false;

            _doubleTapEndTime = 0;
        }

        // Disable Trick Shots if not in AoE
        if (_trickShotsActive && this->GetEnemiesInRange(40.0f) < 3)
        {

            _trickShotsActive = false;
        }

        // Update Precise Shots if expired
        if (!_preciseShotsTracker.HasCharges())
        {

            _preciseShotsTracker.Reset();
        }
    }

    bool ShouldUseTrueshot(Unit* target) const
    {
        if (!target)

            return false;

        // Use on high priority targets or during burn phases
        return (target->GetHealthPct() > 50.0f && this->_resource > 60) ||

               target->GetLevel() > this->GetBot()->GetLevel() + 2 ||

               (target->GetMaxHealth() > this->GetBot()->GetMaxHealth() * 5);
    }

    bool ShouldUseRapidFire() const
    {
        // Use during burst windows or when we have high focus
        return _trueshotActive || _doubleTapActive || this->_resource > 80;
    }

    void CheckLoneWolfStatus()
    {
        // MM hunters prefer Lone Wolf for the damage bonus
        _loneWolfActive = true;

        // Dismiss pet if we have one
        if (!this->GetBot()->GetPetGUID().IsEmpty())
        {
            this->GetBot()->CastSpell(this->GetBot(), SPELL_DISMISS_PET);
        }
    }

    

private:
    // ========================================================================
    // HUNTER SPECIALIZATION ABSTRACT METHOD IMPLEMENTATIONS
    // ========================================================================

    // Pet management - MM uses Lone Wolf (no pet)
    void UpdatePetManagement() { /* Lone Wolf - no pet management */ }
    void SummonPet() { /* Lone Wolf - no pet */ }
    void MendPetIfNeeded() { /* Lone Wolf - no pet */ }
    void FeedPetIfNeeded() { /* Lone Wolf - no pet */ }
    bool HasActivePet() const { return false; /* Lone Wolf */ }
    ::Playerbot::PetInfo GetPetInfo() const { return ::Playerbot::PetInfo(); /* No pet */ }

    // Trap management - delegated to AI
    void UpdateTrapManagement() { /* Traps managed by AI */ }
    void PlaceTrap(uint32 /*trapSpell*/, Position /*position*/) { /* Traps managed by AI */ }
    bool ShouldPlaceTrap() const { return false; }
    uint32 GetOptimalTrapSpell() const { return 0; }
    ::std::vector<TrapInfo> GetActiveTraps() const { return ::std::vector<TrapInfo>(); }

    // Aspect management - delegated to UpdateBuffs
    void UpdateAspectManagement() { /* Aspects managed in UpdateBuffs */ }
    void SwitchToOptimalAspect() { /* Aspects managed in UpdateBuffs */ }
    uint32 GetOptimalAspect() const { return SPELL_ASPECT_TURTLE; }
    bool HasCorrectAspect() const { return true; }

    // Range and positioning - MM is ranged with cast times
    void UpdateRangeManagement() { /* Range handled by base class */ }
    bool IsInDeadZone(::Unit* /*target*/) const { return false; }
    bool ShouldKite(::Unit* /*target*/) const { return false; }
    Position GetKitePosition(::Unit* /*target*/) const { return Position(); }
    void HandleDeadZone(::Unit* /*target*/) { /* No dead zone management */ }

    // Tracking management - delegated to AI
    void UpdateTracking() { /* Tracking managed by AI */ }
    uint32 GetOptimalTracking() const { return 0; /* No specific tracking */ }
    void ApplyTracking(uint32 /*trackingSpell*/) { /* Applied by AI */ }

    // Pet command interface - MM uses Lone Wolf (no pet commands)
    void CommandPetAttack(::Unit* /*target*/) { /* Lone Wolf - no pet */ }
    void CommandPetFollow() { /* Lone Wolf - no pet */ }
    void CommandPetStay() { /* Lone Wolf - no pet */ }

    // Positioning interface - ranged DPS with cast times
    // Note: GetOptimalRange is final in RangedDpsSpecialization (returns 30-40 yards for casters)
    Position GetOptimalPosition(::Unit* /*target*/) override { return Position(); /* Handled by base class */ }

    // Phase 5 Integration: Decision Systems Initialization
    void InitializeMarksmanshipMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;

        // ========================================================================
        // ActionPriorityQueue: Register Marksmanship Hunter spells with priorities
        // ========================================================================
        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Survival cooldowns

            queue->RegisterSpell(SPELL_ASPECT_TURTLE, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);

            queue->AddCondition(SPELL_ASPECT_TURTLE, [this](Player* bot, Unit*) {

                return bot && bot->GetHealthPct() < 30.0f;

            }, "Bot HP < 30% (damage reduction + immune)");


            queue->RegisterSpell(SPELL_EXHILARATION_MM, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);

            queue->AddCondition(SPELL_EXHILARATION_MM, [this](Player* bot, Unit*) {

                return bot && bot->GetHealthPct() < 50.0f;

            }, "Bot HP < 50% (self heal)");

            // CRITICAL: Major burst cooldowns

            queue->RegisterSpell(SPELL_TRUESHOT, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);

            queue->AddCondition(SPELL_TRUESHOT, [this](Player* bot, Unit* target) {

                return target && this->_resource > 60 && !this->_trueshotActive &&

                       this->ShouldUseTrueshot(target);

            }, "60+ Focus, not active (major 15s burst)");


            queue->RegisterSpell(SPELL_DOUBLE_TAP, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);

            queue->AddCondition(SPELL_DOUBLE_TAP, [this](Player* bot, Unit* target) {

                return bot && bot->HasSpell(SPELL_DOUBLE_TAP) &&

                       target && this->_trueshotActive && !this->_doubleTapActive;

            }, "During Trueshot (3s burst enhancement)");


            queue->RegisterSpell(SPELL_RAPID_FIRE, SpellPriority::CRITICAL, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(SPELL_RAPID_FIRE, [this](Player* bot, Unit* target) {

                return target && this->_resource >= 30 &&

                       this->ShouldUseRapidFire() && !this->_castManager.IsBusy();

            }, "30+ Focus during burst or 80+ Focus (3s channel)");

            // HIGH: Primary nuke and procs

            queue->RegisterSpell(SPELL_AIMED_SHOT, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(SPELL_AIMED_SHOT, [this](Player* bot, Unit* target) {

                return target && this->_resource >= 35 &&

                       this->_castManager.CanStartCast() && this->GetEnemiesInRange(40.0f) < 3;

            }, "35+ Focus, can cast (primary nuke, 2.5s cast)");


            queue->RegisterSpell(SPELL_ARCANE_SHOT, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(SPELL_ARCANE_SHOT, [this](Player* bot, Unit* target) {

                return target && this->_resource >= 20 &&

                       this->_preciseShotsTracker.HasCharges();

            }, "20+ Focus, has Precise Shots charges (instant damage)");

            // MEDIUM: Utility and talents

            queue->RegisterSpell(SPELL_EXPLOSIVE_SHOT, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);

            queue->AddCondition(SPELL_EXPLOSIVE_SHOT, [this](Player* bot, Unit* target) {

                return bot && bot->HasSpell(SPELL_EXPLOSIVE_SHOT) &&

                       target && this->_resource >= 20;

            }, "Has talent, 20+ Focus (AoE burst)");


            queue->RegisterSpell(SPELL_COUNTER_SHOT_MM, SpellPriority::MEDIUM, SpellCategory::UTILITY);

            queue->AddCondition(SPELL_COUNTER_SHOT_MM, [this](Player* bot, Unit* target) {

                return target && target->IsNonMeleeSpellCast(false);

            }, "Target casting (interrupt)");

            // LOW: Focus generation and fillers

            queue->RegisterSpell(SPELL_STEADY_SHOT, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(SPELL_STEADY_SHOT, [this](Player* bot, Unit* target) {

                return target && this->_resource < 70;

            }, "< 70 Focus (focus generator, +10 Focus)");


            queue->RegisterSpell(SPELL_ARCANE_SHOT, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(SPELL_ARCANE_SHOT, [this](Player* bot, Unit* target) {

                return target && this->_resource >= 80 &&

                       !this->_preciseShotsTracker.HasCharges();

            }, "80+ Focus, no Precise Shots (filler dump)");


            queue->RegisterSpell(SPELL_MULTISHOT_MM, SpellPriority::LOW, SpellCategory::DAMAGE_AOE);

            queue->AddCondition(SPELL_MULTISHOT_MM, [this](Player* bot, Unit* target) {

                return target && this->_resource >= 20 &&

                       this->GetEnemiesInRange(40.0f) >= 3;

            }, "20+ Focus, 3+ enemies (AoE)");


            TC_LOG_INFO("module.playerbot", " MARKSMANSHIP HUNTER: Registered {} spells in ActionPriorityQueue", queue->GetSpellCount());
        }

        // ========================================================================
        // BehaviorTree: Marksmanship Hunter DPS rotation logic
        // ========================================================================
        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {

            auto root = Selector("Marksmanship Hunter DPS", {
                // Tier 1: Burst Window (Trueshot → Double Tap → Rapid Fire)

                Sequence("Burst Cooldowns", {

                    Condition("Target exists", [this](Player* bot, Unit* target) {

                        return target != nullptr;

                    }),

                    Selector("Use Burst Cooldowns", {
                        // Trueshot (major cooldown)

                        Sequence("Cast Trueshot", {

                            Condition("Should use Trueshot", [this](Player* bot, Unit* target) {

                                return this->_resource > 60 && !this->_trueshotActive &&

                                       this->ShouldUseTrueshot(target);

                            }),

                            bot::ai::Action("Cast Trueshot", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->CanUseAbility(SPELL_TRUESHOT))

                                {

                                    this->CastSpell(SPELL_TRUESHOT, bot);

                                    this->_trueshotActive = true;

                                    this->_trueshotEndTime = GameTime::GetGameTimeMS() + 15000;

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),
                        // Double Tap (during Trueshot)

                        Sequence("Cast Double Tap", {

                            Condition("During Trueshot", [this](Player* bot, Unit*) {

                                return bot && bot->HasSpell(SPELL_DOUBLE_TAP) &&

                                       this->_trueshotActive && !this->_doubleTapActive;

                            }),

                            bot::ai::Action("Cast Double Tap", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->CanUseAbility(SPELL_DOUBLE_TAP))

                                {

                                    this->CastSpell(SPELL_DOUBLE_TAP, bot);

                                    this->_doubleTapActive = true;

                                    this->_doubleTapEndTime = GameTime::GetGameTimeMS() + 3000;

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),
                        // Rapid Fire (during burst windows or high focus)

                        Sequence("Cast Rapid Fire", {

                            Condition("Should use Rapid Fire", [this](Player* bot, Unit*) {

                                return this->_resource >= 30 && this->ShouldUseRapidFire() &&

                                       !this->_castManager.IsBusy();

                            }),

                            bot::ai::Action("Cast Rapid Fire", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->CanUseAbility(SPELL_RAPID_FIRE))

                                {

                                    this->_castManager.StartRapidFire();

                                    this->CastSpell(SPELL_RAPID_FIRE, target);

                                    this->_lastRapidFire = GameTime::GetGameTimeMS();

                                    this->ConsumeResource(30);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 2: Primary Damage (Aimed Shot with Careful Aim)

                Sequence("Primary Nuke", {

                    Condition("Target exists and can cast", [this](Player* bot, Unit* target) {

                        return target != nullptr && this->_castManager.CanStartCast() &&

                               this->GetEnemiesInRange(40.0f) < 3;

                    }),

                    Selector("Cast Aimed Shot", {

                        Sequence("Cast Aimed Shot", {

                            Condition("35+ Focus", [this](Player* bot, Unit*) {

                                return this->_resource >= 35;

                            }),

                            bot::ai::Action("Cast Aimed Shot", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->CanUseAbility(SPELL_AIMED_SHOT))

                                {

                                    this->_castManager.StartAimedShot(target);

                                    this->CastSpell(SPELL_AIMED_SHOT, target);

                                    this->_lastAimedShot = GameTime::GetGameTimeMS();

                                    this->_preciseShotsTracker.OnAimedShotCast();

                                    this->ConsumeResource(35);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 3: Proc Management (Precise Shots → Arcane Shot)

                Sequence("Proc Management", {

                    Condition("Target exists", [this](Player* bot, Unit* target) {

                        return target != nullptr;

                    }),

                    Selector("Use Procs", {
                        // Precise Shots: Arcane Shot charges

                        Sequence("Cast Arcane Shot with Precise Shots", {

                            Condition("Has Precise Shots", [this](Player* bot, Unit*) {

                                return this->_preciseShotsTracker.HasCharges() && this->_resource >= 20;

                            }),

                            bot::ai::Action("Cast Arcane Shot", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->_resource >= 20)

                                {

                                    this->CastSpell(SPELL_ARCANE_SHOT, target);

                                    this->_preciseShotsTracker.ConsumeCharge();

                                    this->ConsumeResource(20);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),
                        // Explosive Shot (talent)

                        Sequence("Cast Explosive Shot", {

                            Condition("Has talent and 20+ Focus", [this](Player* bot, Unit*) {

                                return bot && bot->HasSpell(SPELL_EXPLOSIVE_SHOT) &&

                                       this->_resource >= 20;

                            }),

                            bot::ai::Action("Cast Explosive Shot", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->CanUseAbility(SPELL_EXPLOSIVE_SHOT))

                                {

                                    this->CastSpell(SPELL_EXPLOSIVE_SHOT, target);

                                    this->ConsumeResource(20);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 4: Focus Generation (Steady Shot < 70 Focus, Arcane Shot 80+ Focus)

                Sequence("Focus Management", {

                    Condition("Target exists", [this](Player* bot, Unit* target) {

                        return target != nullptr;

                    }),

                    Selector("Generate or Dump Focus", {
                        // Generate focus when low

                        Sequence("Generate Focus", {

                            Condition("< 70 Focus", [this](Player* bot, Unit*) {

                                return this->_resource < 70;

                            }),

                            bot::ai::Action("Cast Steady Shot", [this](Player* bot, Unit* target) -> NodeStatus {

                                this->CastSpell(SPELL_STEADY_SHOT, target);

                                this->_lastSteadyShot = GameTime::GetGameTimeMS();

                                this->_resource = ::std::min<uint32>(this->_resource + 10, 100);

                                return NodeStatus::SUCCESS;

                            })

                        }),
                        // Dump focus when high

                        Sequence("Dump Focus", {

                            Condition("80+ Focus", [this](Player* bot, Unit*) {

                                return this->_resource >= 80;

                            }),

                            bot::ai::Action("Cast Arcane Shot", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->_resource >= 20)

                                {

                                    this->CastSpell(SPELL_ARCANE_SHOT, target);

                                    this->ConsumeResource(20);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                })

            });


            behaviorTree->SetRoot(root);

            TC_LOG_INFO("module.playerbot", " MARKSMANSHIP HUNTER: BehaviorTree initialized with 4-tier DPS rotation");
        }
    }

private:
    PreciseShotsTracker _preciseShotsTracker;
    MarksmanshipCastManager _castManager;

    // Burst window tracking
    bool _trueshotActive;
    uint32 _trueshotEndTime;
    bool _doubleTapActive;
    uint32 _doubleTapEndTime;

    // AoE tracking
    bool _trickShotsActive;

    // Ability timing
    uint32 _lastAimedShot;
    uint32 _lastRapidFire;
    uint32 _lastSteadyShot;

    // Lone Wolf preference
    bool _loneWolfActive;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot