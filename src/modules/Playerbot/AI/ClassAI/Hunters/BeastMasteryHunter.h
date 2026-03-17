/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Beast Mastery Hunter Specialization - REFACTORED
 *
 * This demonstrates the complete migration of Beast Mastery Hunter to the template-based
 * architecture, eliminating code duplication while maintaining full functionality.
 *
 * Beast Mastery focuses on pet synergy, providing powerful buffs to the pet while
 * maintaining consistent ranged damage through focus management.
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
#include "PetDefines.h"
#include "MotionMaster.h"
#include "CharmInfo.h"
#include "HunterAI.h"
#include "ObjectAccessor.h"
#include <unordered_map>
#include <queue>

// Phase 5 Integration: Decision Systems
#include "../../Decision/ActionPriorityQueue.h"
#include "../../Decision/BehaviorTree.h"
#include "../BotAI.h"
#include "GameTime.h"

namespace Playerbot
{

// Forward declaration for trap management
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
// BEAST MASTERY HUNTER SPELL IDs (WoW 12.0 - The War Within)
// Using centralized spell registry from SpellValidation_WoW120.h
// ============================================================================

enum BeastMasterySpells
{
    // Core Abilities - Using central registry: WoW120Spells::Hunter::BeastMastery
    SPELL_KILL_COMMAND          = WoW120Spells::Hunter::BeastMastery::KILL_COMMAND,
    SPELL_BARBED_SHOT           = WoW120Spells::Hunter::BeastMastery::BARBED_SHOT,
    SPELL_COBRA_SHOT            = WoW120Spells::Hunter::BeastMastery::COBRA_SHOT,
    SPELL_BESTIAL_WRATH         = WoW120Spells::Hunter::BeastMastery::BESTIAL_WRATH,
    SPELL_ASPECT_OF_THE_WILD    = WoW120Spells::Hunter::BeastMastery::ASPECT_OF_THE_WILD,
    SPELL_MULTISHOT             = WoW120Spells::Hunter::BeastMastery::MULTI_SHOT_BM,

    // Pet Management - Using central registry: WoW120Spells::Hunter
    SPELL_CALL_PET_1            = WoW120Spells::Hunter::CALL_PET_1,
    SPELL_MEND_PET              = WoW120Spells::Hunter::MEND_PET,
    SPELL_REVIVE_PET            = WoW120Spells::Hunter::REVIVE_PET,
    SPELL_PET_ATTACK            = WoW120Spells::Hunter::PET_ATTACK,
    SPELL_PET_FOLLOW            = WoW120Spells::Hunter::PET_FOLLOW,
    SPELL_PET_STAY              = WoW120Spells::Hunter::PET_STAY,

    // Talents/Special - Using central registry: WoW120Spells::Hunter::BeastMastery
    SPELL_DIRE_BEAST            = WoW120Spells::Hunter::BeastMastery::DIRE_BEAST,
    SPELL_BLOODSHED             = WoW120Spells::Hunter::BeastMastery::BLOODSHED,
    SPELL_WILD_CALL             = WoW120Spells::Hunter::BeastMastery::WILD_CALL,
    SPELL_ANIMAL_COMPANION      = WoW120Spells::Hunter::BeastMastery::ANIMAL_COMPANION,

    // Buffs/Debuffs - Using central registry: WoW120Spells::Hunter
    SPELL_HUNTERS_MARK          = WoW120Spells::Hunter::HUNTERS_MARK,
    SPELL_ASPECT_OF_CHEETAH     = WoW120Spells::Hunter::ASPECT_OF_THE_CHEETAH,
    SPELL_EXHILARATION          = WoW120Spells::Hunter::EXHILARATION,
    SPELL_PET_FRENZY            = WoW120Spells::Hunter::BeastMastery::PET_FRENZY,

    // Utility - Using central registry: WoW120Spells::Hunter
    SPELL_COUNTER_SHOT          = WoW120Spells::Hunter::COUNTER_SHOT,
    SPELL_TRANQUILIZING_SHOT    = WoW120Spells::Hunter::TRANQUILIZING_SHOT,
    SPELL_TAR_TRAP              = WoW120Spells::Hunter::TAR_TRAP,
    SPELL_FREEZING_TRAP         = WoW120Spells::Hunter::FREEZING_TRAP
};

/**
 * Pet Management System for Beast Mastery
 *
 * Handles all pet-related mechanics including summoning, commanding,
 * ability usage, and health management.
 */
class BeastMasteryPetManager
{
public:
    explicit BeastMasteryPetManager(Player* bot)        : _bot(bot)
        , _lastMendPet(0)
        , _lastPetCommand(0)
        , _petFrenzyStacks(0)
        , _petFrenzyExpireTime(0)
    {
    }

    void SummonPet()
    {
        if (!_bot || HasActivePet())
            return;

        // Check if we can summon a pet
        if (_bot->GetPetGUID().IsEmpty())
        {
            _bot->CastSpell(CastSpellTargetArg(_bot), SPELL_CALL_PET_1);
        }
    }

    // Thread-safe version using validated pointer
    void SummonPetSafe(Player* bot)
    {
        if (!bot || HasActivePetSafe(bot))
            return;

        // Check if we can summon a pet
        if (bot->GetPetGUID().IsEmpty())
        {
            bot->CastSpell(CastSpellTargetArg(bot), SPELL_CALL_PET_1);
        }
    }

    void CommandPetAttack(Unit* target)
    {
        if (!target || !HasActivePet())
            return;

        Pet* pet = _bot->GetPet();
        if (!pet || !pet->IsAlive())
            return;

        // Update pet target if different
        if (pet->GetVictim() != target)
        {
            pet->AttackStop();
            pet->GetMotionMaster()->Clear();

            if (pet->GetCharmInfo())
            {
                pet->GetCharmInfo()->SetCommandState(COMMAND_ATTACK);
                pet->GetCharmInfo()->SetIsCommandAttack(true);
                pet->GetCharmInfo()->SetIsReturning(false);
                pet->GetCharmInfo()->SetIsFollowing(false);
            }

            pet->Attack(target, true);
            pet->GetMotionMaster()->MoveChase(target);
        }
    }

    // Thread-safe version using validated pointer
    void CommandPetAttackSafe(Player* bot, Unit* target)
    {
        if (!target || !HasActivePetSafe(bot))
            return;

        Pet* pet = bot->GetPet();
        if (!pet || !pet->IsAlive())
            return;

        // Update pet target if different
        if (pet->GetVictim() != target)
        {
            pet->AttackStop();
            pet->GetMotionMaster()->Clear();

            if (pet->GetCharmInfo())
            {
                pet->GetCharmInfo()->SetCommandState(COMMAND_ATTACK);
                pet->GetCharmInfo()->SetIsCommandAttack(true);
                pet->GetCharmInfo()->SetIsReturning(false);
                pet->GetCharmInfo()->SetIsFollowing(false);
            }

            pet->Attack(target, true);
            pet->GetMotionMaster()->MoveChase(target);
        }
    }

    void CommandPetFollow()
    {
        Pet* pet = _bot->GetPet();
        if (!pet || !pet->IsAlive())

            return;

        pet->AttackStop();
        pet->GetMotionMaster()->Clear();

        if (pet->GetCharmInfo())
        {

            pet->GetCharmInfo()->SetCommandState(COMMAND_FOLLOW);

            pet->GetCharmInfo()->SetIsCommandAttack(false);

            pet->GetCharmInfo()->SetIsReturning(true);

            pet->GetCharmInfo()->SetIsFollowing(true);
        }

        pet->GetMotionMaster()->MoveFollow(_bot, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
    }

    bool HasActivePet() const
    {
        return _bot && !_bot->GetPetGUID().IsEmpty() && _bot->GetPet() && _bot->GetPet()->IsAlive();
    }

    // Thread-safe version using validated pointer
    bool HasActivePetSafe(Player* bot) const
    {
        return bot && !bot->GetPetGUID().IsEmpty() && bot->GetPet() && bot->GetPet()->IsAlive();
    }

    bool IsPetHealthLow() const
    {
        if (!HasActivePet())
            return false;

        Pet* pet = _bot->GetPet();
        return pet && pet->GetHealthPct() < 70.0f;
    }

    // Thread-safe version using validated pointer
    bool IsPetHealthLowSafe(Player* bot) const
    {
        if (!HasActivePetSafe(bot))
            return false;

        Pet* pet = bot->GetPet();
        return pet && pet->GetHealthPct() < 70.0f;
    }

    void MendPet()
    {
        uint32 currentTime = GameTime::GetGameTimeMS();
        if (currentTime - _lastMendPet < 10000) // 10 second cooldown
            return;

        if (!HasActivePet() || !IsPetHealthLow())
            return;

        Pet* pet = _bot->GetPet();
        if (pet && pet->IsAlive() && !_bot->HasAura(SPELL_MEND_PET))
        {
            _bot->CastSpell(CastSpellTargetArg(pet), SPELL_MEND_PET);
            _lastMendPet = currentTime;
        }
    }

    // Thread-safe version using validated pointer
    void MendPetSafe(Player* bot)
    {
        uint32 currentTime = GameTime::GetGameTimeMS();
        if (currentTime - _lastMendPet < 10000) // 10 second cooldown
            return;

        if (!HasActivePetSafe(bot) || !IsPetHealthLowSafe(bot))
            return;

        Pet* pet = bot->GetPet();
        if (pet && pet->IsAlive() && !bot->HasAura(SPELL_MEND_PET))
        {
            bot->CastSpell(CastSpellTargetArg(pet), SPELL_MEND_PET);
            _lastMendPet = currentTime;
        }
    }

    void UpdatePetFrenzy()
    {
        uint32 currentTime = GameTime::GetGameTimeMS();

        // Check if frenzy has expired
        if (_petFrenzyExpireTime > 0 && currentTime > _petFrenzyExpireTime)
        {

            _petFrenzyStacks = 0;

            _petFrenzyExpireTime = 0;
        }
    }

    void ApplyBarbedShot()
    {
        // Barbed Shot applies/refreshes Pet Frenzy
        _petFrenzyStacks = ::std::min<uint32>(_petFrenzyStacks + 1, 3);
        _petFrenzyExpireTime = GameTime::GetGameTimeMS() + 8000; // 8 second duration
    }

    uint32 GetPetFrenzyStacks() const { return _petFrenzyStacks; }

    void EnsurePetActive(Unit* target)
    {
        // THREAD-SAFETY FIX: Validate bot pointer before access
        if (!_bot)
            return;

        // Store GUID for thread-safe lookup
        ObjectGuid botGuid = _bot->GetGUID();
        Player* bot = ObjectAccessor::FindPlayer(botGuid);
        if (!bot)
            return;

        if (!HasActivePetSafe(bot))
        {
            SummonPetSafe(bot);
            return;
        }

        // CRITICAL FIX: Ensure bot pets are always in DEFENSIVE mode
        // Pet react state is loaded from database on spawn. If it was saved as PASSIVE,
        // the pet will not defend itself or its master when attacked.
        Pet* pet = bot->GetPet();
        if (pet && pet->HasReactState(REACT_PASSIVE))
        {
            TC_LOG_DEBUG("module.playerbot.ai", "BM Hunter {} pet {} was PASSIVE, setting to DEFENSIVE",
                         bot->GetName(), pet->GetName());
            pet->SetReactState(REACT_DEFENSIVE);
        }

        // Heal pet if needed
        if (IsPetHealthLowSafe(bot))
            MendPetSafe(bot);

        // Command pet to attack
        if (target && target->IsAlive())
            CommandPetAttackSafe(bot, target);
    }

    uint32 GetLastCommandTime() const { return _lastPetCommand; }

private:
    CooldownManager _cooldowns;
    Player* _bot;
    uint32 _lastMendPet;
    uint32 _lastPetCommand;
    uint32 _petFrenzyStacks;
    uint32 _petFrenzyExpireTime;
};

/**
 * Refactored Beast Mastery Hunter using template architecture
 *
 * Key features:
 * - Inherits from RangedDpsSpecialization<FocusResource> for role defaults
 * - Comprehensive pet management system
 * - Barbed Shot stack tracking for optimal DPS
 * - Kill Command priority system
 * - Focus management with 5/sec regeneration
 */
class BeastMasteryHunterRefactored : public RangedDpsSpecialization<FocusResource>
{
public:
    using Base = RangedDpsSpecialization<FocusResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::CanUseAbility;
    using Base::ConsumeResource;
    using Base::_resource;

    explicit BeastMasteryHunterRefactored(Player* bot)
        : RangedDpsSpecialization<FocusResource>(bot)
        , _petManager(bot)
        , _barbedShotCharges(2)
        , _lastBarbedShotRecharge(0)
        , _bestialWrathActive(false)
        , _bestialWrathEndTime(0)
        , _aspectOfTheWildActive(false)
        , _aspectEndTime(0)        , _wildCallProc(false)
        , _lastKillCommand(0)
        , _lastCobraShot(0)
    {
        // Focus regeneration is handled by template system
        // Setup BM-specific cooldown tracking

        // Phase 5 Integration: Initialize decision systems
        InitializeBeastMasteryMechanics();
    }

    // ========================================================================
    // CORE ROTATION - Beast Mastery specific logic
    // ========================================================================

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !target->IsAlive() || !target->IsHostileTo(this->GetBot()))

            return;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero talent rotation branches
        if (_heroTalents.IsTree(HeroTalentTree::PACK_LEADER))
        {
            // Pack Leader: Vicious Hunt for coordinated pet burst
            if (this->_resource >= 30 &&
                this->CanCastSpell(WoW120Spells::Hunter::BeastMastery::VICIOUS_HUNT, target))
            {
                this->CastSpell(WoW120Spells::Hunter::BeastMastery::VICIOUS_HUNT, target);
                this->ConsumeResource(30);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::DARK_RANGER))
        {
            // Dark Ranger: Black Arrow for shadow-empowered shots
            if (this->CanCastSpell(WoW120Spells::Hunter::BeastMastery::BLACK_ARROW, target))
            {
                this->CastSpell(WoW120Spells::Hunter::BeastMastery::BLACK_ARROW, target);
                return;
            }
        }

        // Ensure pet is active and attacking
        _petManager.EnsurePetActive(target);

        // Update BM-specific mechanics
        UpdateBeastMasteryState();

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
        Player* bot = this->GetBot();
        

        // Ensure pet is summoned
        if (!_petManager.HasActivePet())
        {

            _petManager.SummonPet();

            return;
        }

        // Maintain Aspect of the Cheetah when out of combat
        if (!bot->IsInCombat() && !bot->HasAura(SPELL_ASPECT_OF_CHEETAH))
        {

            this->CastSpell(SPELL_ASPECT_OF_CHEETAH, bot);
        }

        // Use Exhilaration for emergency healing
        if (bot->GetHealthPct() < 40.0f && this->CanUseAbility(SPELL_EXHILARATION))
        {

            this->CastSpell(SPELL_EXHILARATION, bot);
        }

        // Apply Hunter's Mark to current target
        if (Unit* target = bot->GetVictim())        {

            if (!target->HasAura(SPELL_HUNTERS_MARK) && this->CanUseAbility(SPELL_HUNTERS_MARK))

            {

                this->CastSpell(SPELL_HUNTERS_MARK, target);

            }
        }
    }

    void OnInterruptRequired(::Unit* target, uint32 /*spellId*/)
    {
        if (this->CanUseAbility(SPELL_COUNTER_SHOT))
        {

            this->CastSpell(SPELL_COUNTER_SHOT, target);
        }
    }

    void OnDispelRequired(::Unit* target)
    {
        if (this->CanUseAbility(SPELL_TRANQUILIZING_SHOT))
        {

            this->CastSpell(SPELL_TRANQUILIZING_SHOT, target);
        }
    }

protected:
    // ========================================================================
    // RESOURCE MANAGEMENT - Focus costs for BM Hunter abilities
    // ========================================================================

    uint32 GetResourceCost(uint32 spellId)
    {
        switch (spellId)
        {

            case SPELL_KILL_COMMAND:
            return 30;

            case SPELL_COBRA_SHOT:
            return 35;

            case SPELL_MULTISHOT:
            return 40;

            case SPELL_BARBED_SHOT:
            return 0;  // Generates 20 focus

            case SPELL_BESTIAL_WRATH:
            return 0;  // No cost

            case SPELL_ASPECT_OF_THE_WILD: return 0; // No cost

            case SPELL_DIRE_BEAST:
            return 25;

            case SPELL_COUNTER_SHOT:
            return 0;  // No cost

            case SPELL_TRANQUILIZING_SHOT: return 10;

            default:
            return 20;
        }
    }

    // ========================================================================
    // BEAST MASTERY SPECIFIC ROTATION LOGIC
    // ========================================================================

    void ExecuteSingleTargetRotation(::Unit* target)
    {
        uint32 currentFocus = this->_resource;

        // Priority 1: Bestial Wrath for burst
        if (ShouldUseBestialWrath(target) && this->CanUseAbility(SPELL_BESTIAL_WRATH))
        {

            this->CastSpell(SPELL_BESTIAL_WRATH, this->GetBot());

            _bestialWrathActive = true;

            _bestialWrathEndTime = GameTime::GetGameTimeMS() + 15000;

            return;
        }

        // Priority 2: Aspect of the Wild during Bestial Wrath
        if (_bestialWrathActive && this->CanUseAbility(SPELL_ASPECT_OF_THE_WILD))
        {

            this->CastSpell(SPELL_ASPECT_OF_THE_WILD, this->GetBot());

            _aspectOfTheWildActive = true;

            _aspectEndTime = GameTime::GetGameTimeMS() + 20000;

            return;
        }

        // Priority 3: Kill Command on cooldown (core ability)
        if (currentFocus >= 30 && this->CanUseAbility(SPELL_KILL_COMMAND))
        {

            this->CastSpell(SPELL_KILL_COMMAND, target);

            _lastKillCommand = GameTime::GetGameTimeMS();

            this->ConsumeResource(30);

            return;
        }

        // Priority 4: Barbed Shot to maintain Pet Frenzy
        if (ShouldUseBarbedShot() && HasBarbedShotCharge())
        {

            this->CastSpell(SPELL_BARBED_SHOT, target);

            _petManager.ApplyBarbedShot();

            _barbedShotCharges--;

            _resource = ::std::min<uint32>(_resource + 20, 100); // Barbed Shot generates 20 focus

            return;
        }

        // Priority 5: Dire Beast for additional damage
        if (currentFocus >= 25 && this->CanUseAbility(SPELL_DIRE_BEAST))
        {

            this->CastSpell(SPELL_DIRE_BEAST, target);

            this->ConsumeResource(25);

            return;
        }

        // Priority 6: Cobra Shot as filler
        if (currentFocus >= 35)
        {

            this->CastSpell(SPELL_COBRA_SHOT, target);

            _lastCobraShot = GameTime::GetGameTimeMS();

            this->ConsumeResource(35);

            _resource = ::std::min<uint32>(_resource + 5, 100); // Small focus return

            return;
        }

        // Wait for focus regeneration if needed
    }

    void ExecuteAoERotation(::Unit* target)
    {
        uint32 currentFocus = this->_resource;

        // Priority 1: Multi-Shot for Beast Cleave
        if (currentFocus >= 40)
        {

            this->CastSpell(SPELL_MULTISHOT, target);

            this->ConsumeResource(40);

            return;
        }

        // Priority 2: Barbed Shot for focus generation
        if (HasBarbedShotCharge())
        {

            this->CastSpell(SPELL_BARBED_SHOT, target);

            _barbedShotCharges--;

            _resource = ::std::min<uint32>(_resource + 20, 100);

            return;
        }

        // Priority 3: Kill Command if focus allows
        if (currentFocus >= 30 && this->CanUseAbility(SPELL_KILL_COMMAND))
        {

            this->CastSpell(SPELL_KILL_COMMAND, target);

            this->ConsumeResource(30);

            return;
        }
    }

private:
    // ========================================================================
    // BEAST MASTERY STATE MANAGEMENT
    // ========================================================================

    void UpdateBeastMasteryState()
    {
        uint32 currentTime = GameTime::GetGameTimeMS();

        // Update pet frenzy
        _petManager.UpdatePetFrenzy();

        // Check Bestial Wrath expiry
        if (_bestialWrathActive && currentTime > _bestialWrathEndTime)
        {

            _bestialWrathActive = false;

            _bestialWrathEndTime = 0;
        }

        // Check Aspect of the Wild expiry
        if (_aspectOfTheWildActive && currentTime > _aspectEndTime)
        {

            _aspectOfTheWildActive = false;

            _aspectEndTime = 0;
        }

        // Recharge Barbed Shot charges (12 second recharge)
        if (_barbedShotCharges < 2)
        {

            if (currentTime - _lastBarbedShotRecharge > 12000)
            {

                _barbedShotCharges++;

                _lastBarbedShotRecharge = currentTime;

            }
        }

        // Check for Wild Call proc (chance to reset Barbed Shot)
        CheckWildCallProc();
    }

    bool ShouldUseBarbedShot() const
    {
        // Use if we need to refresh Pet Frenzy or have 2 charges
        return _petManager.GetPetFrenzyStacks() < 3 ||

               _barbedShotCharges == 2 ||

               _wildCallProc;
    }

    bool HasBarbedShotCharge() const
    {
        return _barbedShotCharges > 0 || _wildCallProc;
    }

    bool ShouldUseBestialWrath(Unit* target) const
    {
        if (!target)

            return false;

        // Use on high priority targets or when we have good focus
        return (target->GetHealthPct() > 50.0f && this->_resource > 60) ||

               target->GetLevel() > this->GetBot()->GetLevel() + 2;
    }

    void CheckWildCallProc()
    {
        // Simulate 20% proc chance on auto-shot
        if (this->GetBot()->IsInCombat())
        {

            static uint32 lastCheck = 0;

            uint32 currentTime = GameTime::GetGameTimeMS();


            if (currentTime - lastCheck > 3000) // Check every 3 seconds

            {

                lastCheck = currentTime;

                _wildCallProc = (rand() % 100) < 20;

            }
        }
    }

    

private:
    // ========================================================================
    // HUNTER SPECIALIZATION ABSTRACT METHOD IMPLEMENTATIONS
    // ========================================================================

    // Pet management - implemented by BeastMasteryPetManager
    void UpdatePetManagement() { _petManager.EnsurePetActive(GetBot()->GetVictim()); }
    void SummonPet() { _petManager.SummonPet(); }
    void MendPetIfNeeded() { _petManager.MendPet(); }
    void FeedPetIfNeeded() { /* Feeding not implemented in WoW 12.0 */ }
    bool HasActivePet() const { return _petManager.HasActivePet(); }
    ::Playerbot::PetInfo GetPetInfo() const
    {
        ::Playerbot::PetInfo info;
        Pet* pet = GetBot()->GetPet();
        if (pet)
        {
            info.guid = pet->GetGUID();
            info.health = pet->GetHealth();
            info.maxHealth = pet->GetMaxHealth();
            info.isDead = !pet->IsAlive();
            info.type = pet->getPetType();
            // Note: happiness and feeding were removed in modern WoW (Cataclysm+)
            info.happiness = 0;
            info.lastFeed = 0;
            info.lastCommand = _petManager.GetLastCommandTime();
        }
        return info;
    }

    // Trap management - delegated to AI
    void UpdateTrapManagement() { /* Traps managed by AI */ }
    void PlaceTrap(uint32 /*trapSpell*/, Position /*position*/) { /* Traps managed by AI */ }
    bool ShouldPlaceTrap() const { return false; }
    uint32 GetOptimalTrapSpell() const { return 0; }
    ::std::vector<TrapInfo> GetActiveTraps() const { return ::std::vector<TrapInfo>(); }

    // Aspect management - delegated to UpdateBuffs
    void UpdateAspectManagement() { /* Aspects managed in UpdateBuffs */ }
    void SwitchToOptimalAspect() { /* Aspects managed in UpdateBuffs */ }
    uint32 GetOptimalAspect() const { return SPELL_ASPECT_OF_CHEETAH; }
    bool HasCorrectAspect() const { return true; }

    // Range and positioning - BM is ranged
    void UpdateRangeManagement() { /* Range handled by base class */ }
    bool IsInDeadZone(::Unit* /*target*/) const { return false; }
    bool ShouldKite(::Unit* /*target*/) const { return false; }
    Position GetKitePosition(::Unit* /*target*/) const { return Position(); }
    void HandleDeadZone(::Unit* /*target*/) { /* No dead zone management */ }

    // Tracking management - delegated to AI
    void UpdateTracking() { /* Tracking managed by AI */ }
    uint32 GetOptimalTracking() const { return 0; /* No specific tracking */ }
    void ApplyTracking(uint32 /*trackingSpell*/) { /* Applied by AI */ }

    // Pet command interface - delegated to pet manager
    void CommandPetAttack(::Unit* target) { _petManager.CommandPetAttack(target); }
    void CommandPetFollow() { _petManager.CommandPetFollow(); }
    void CommandPetStay() { /* Handled by pet AI */ }

    // Positioning interface - ranged DPS positioning
    // Note: GetOptimalRange is final in RangedDpsSpecialization (returns 25-40 yards)
    Position GetOptimalPosition(::Unit* /*target*/) override { return Position(); /* Handled by base class */ }

    // Phase 5 Integration: Decision Systems Initialization
    void InitializeBeastMasteryMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;

        // ========================================================================
        // ActionPriorityQueue: Register Beast Mastery Hunter spells with priorities
        // ========================================================================
        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Survival (HP < 40%)

            queue->RegisterSpell(SPELL_EXHILARATION, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);

            queue->AddCondition(SPELL_EXHILARATION, [this](Player* bot, Unit*) {

                return bot && bot->GetHealthPct() < 40.0f;

            }, "Bot HP < 40% (heal self + pet)");

            // CRITICAL: Major burst cooldowns

            queue->RegisterSpell(SPELL_BESTIAL_WRATH, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);

            queue->AddCondition(SPELL_BESTIAL_WRATH, [this](Player* bot, Unit* target) {

                return target && this->_resource > 60 && !this->_bestialWrathActive;

            }, "60+ Focus and BW not active (major burst)");


            queue->RegisterSpell(SPELL_ASPECT_OF_THE_WILD, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);

            queue->AddCondition(SPELL_ASPECT_OF_THE_WILD, [this](Player* bot, Unit* target) {

                return target && this->_bestialWrathActive && !this->_aspectOfTheWildActive;

            }, "During Bestial Wrath (stack cooldowns)");

            // HIGH: Core rotation abilities

            queue->RegisterSpell(SPELL_KILL_COMMAND, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(SPELL_KILL_COMMAND, [this](Player* bot, Unit* target) {

                return target && this->_resource >= 30 && this->_petManager.HasActivePet();

            }, "30+ Focus and pet alive (core pet ability)");


            queue->RegisterSpell(SPELL_BARBED_SHOT, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(SPELL_BARBED_SHOT, [this](Player* bot, Unit* target) {

                return target && this->HasBarbedShotCharge() &&

                       (this->_petManager.GetPetFrenzyStacks() < 3 || this->_barbedShotCharges == 2);

            }, "Has charge and (Pet Frenzy < 3 stacks or 2 charges)");

            // MEDIUM: Talent abilities and utility

            queue->RegisterSpell(SPELL_DIRE_BEAST, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(SPELL_DIRE_BEAST, [this](Player* bot, Unit* target) {

                return bot && bot->HasSpell(SPELL_DIRE_BEAST) &&

                       target && this->_resource >= 25;

            }, "Has talent, 25+ Focus (summon additional beast)");


            queue->RegisterSpell(SPELL_COUNTER_SHOT, SpellPriority::MEDIUM, SpellCategory::UTILITY);

            queue->AddCondition(SPELL_COUNTER_SHOT, [this](Player* bot, Unit* target) {

                return target && target->IsNonMeleeSpellCast(false);

            }, "Target casting (interrupt)");


            queue->RegisterSpell(SPELL_TRANQUILIZING_SHOT, SpellPriority::MEDIUM, SpellCategory::UTILITY);

            queue->AddCondition(SPELL_TRANQUILIZING_SHOT, [this](Player* bot, Unit* target) {

                return target && this->_resource >= 10 && target->HasAuraType(SPELL_AURA_MOD_INCREASE_SPEED);

            }, "Target has enrage/buff (dispel, 10 Focus)");

            // LOW: Filler abilities

            queue->RegisterSpell(SPELL_COBRA_SHOT, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(SPELL_COBRA_SHOT, [this](Player* bot, Unit* target) {

                return target && this->_resource >= 35 && this->GetEnemiesInRange(40.0f) < 3;

            }, "35+ Focus, < 3 enemies (single target filler)");


            queue->RegisterSpell(SPELL_MULTISHOT, SpellPriority::LOW, SpellCategory::DAMAGE_AOE);

            queue->AddCondition(SPELL_MULTISHOT, [this](Player* bot, Unit* target) {

                return target && this->_resource >= 40 && this->GetEnemiesInRange(40.0f) >= 3;

            }, "40+ Focus, 3+ enemies (AoE filler + Beast Cleave)");


            TC_LOG_INFO("module.playerbot", " BEAST MASTERY HUNTER: Registered {} spells in ActionPriorityQueue", queue->GetSpellCount());
        }

        // ========================================================================
        // BehaviorTree: Beast Mastery Hunter DPS rotation logic
        // ========================================================================
        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {

            auto root = Selector("Beast Mastery Hunter DPS", {
                // Tier 1: Burst Window (Bestial Wrath → Aspect of the Wild)

                Sequence("Burst Cooldowns", {

                    Condition("Target exists", [this](Player* bot, Unit* target) {

                        return target != nullptr;

                    }),

                    Selector("Use Burst Cooldowns", {
                        // Bestial Wrath (major cooldown)

                        Sequence("Cast Bestial Wrath", {

                            Condition("Should use BW", [this](Player* bot, Unit* target) {

                                return this->_resource > 60 && !this->_bestialWrathActive &&

                                       this->ShouldUseBestialWrath(target);

                            }),

                            bot::ai::Action("Cast Bestial Wrath", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->CanUseAbility(SPELL_BESTIAL_WRATH))

                                {

                                    this->CastSpell(SPELL_BESTIAL_WRATH, bot);

                                    this->_bestialWrathActive = true;

                                    this->_bestialWrathEndTime = GameTime::GetGameTimeMS() + 15000;

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),
                        // Aspect of the Wild (during Bestial Wrath)

                        Sequence("Cast Aspect of the Wild", {

                            Condition("During Bestial Wrath", [this](Player* bot, Unit*) {

                                return this->_bestialWrathActive && !this->_aspectOfTheWildActive;

                            }),

                            bot::ai::Action("Cast Aspect of the Wild", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->CanUseAbility(SPELL_ASPECT_OF_THE_WILD))

                                {

                                    this->CastSpell(SPELL_ASPECT_OF_THE_WILD, bot);

                                    this->_aspectOfTheWildActive = true;

                                    this->_aspectEndTime = GameTime::GetGameTimeMS() + 20000;

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 2: Core Rotation (Kill Command priority)

                Sequence("Core Rotation", {

                    Condition("Target exists and pet alive", [this](Player* bot, Unit* target) {

                        return target != nullptr && this->_petManager.HasActivePet();

                    }),

                    Selector("Cast Core Abilities", {
                        // Kill Command (highest priority DPS ability)

                        Sequence("Cast Kill Command", {

                            Condition("30+ Focus", [this](Player* bot, Unit*) {

                                return this->_resource >= 30;

                            }),

                            bot::ai::Action("Cast Kill Command", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->CanUseAbility(SPELL_KILL_COMMAND))

                                {

                                    this->CastSpell(SPELL_KILL_COMMAND, target);

                                    this->_lastKillCommand = GameTime::GetGameTimeMS();

                                    this->ConsumeResource(30);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),
                        // Dire Beast (talent - additional damage source)

                        Sequence("Cast Dire Beast", {

                            Condition("Has talent and 25+ Focus", [this](Player* bot, Unit*) {

                                return bot && bot->HasSpell(SPELL_DIRE_BEAST) &&

                                       this->_resource >= 25;

                            }),

                            bot::ai::Action("Cast Dire Beast", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->CanUseAbility(SPELL_DIRE_BEAST))

                                {

                                    this->CastSpell(SPELL_DIRE_BEAST, target);

                                    this->ConsumeResource(25);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 3: Focus Management (Barbed Shot for generation + Pet Frenzy)

                Sequence("Focus Management", {

                    Condition("Target exists", [this](Player* bot, Unit* target) {

                        return target != nullptr;

                    }),

                    Selector("Generate Focus", {
                        // Barbed Shot (generates 20 Focus, maintains Pet Frenzy)

                        Sequence("Cast Barbed Shot", {

                            Condition("Should use Barbed Shot", [this](Player* bot, Unit*) {

                                return this->HasBarbedShotCharge() &&

                                       (this->_petManager.GetPetFrenzyStacks() < 3 ||

                                        this->_barbedShotCharges == 2 ||

                                        this->_wildCallProc);

                            }),

                            bot::ai::Action("Cast Barbed Shot", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->HasBarbedShotCharge())

                                {

                                    this->CastSpell(SPELL_BARBED_SHOT, target);

                                    this->_petManager.ApplyBarbedShot();

                                    this->_barbedShotCharges--;

                                    this->_resource = ::std::min<uint32>(this->_resource + 20, 100);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 4: Filler Rotation (Cobra Shot single target, Multishot AoE)

                Sequence("Filler Rotation", {

                    Condition("Target exists", [this](Player* bot, Unit* target) {

                        return target != nullptr;

                    }),

                    Selector("Choose Filler", {
                        // AoE filler (3+ enemies)

                        Sequence("AoE Filler", {

                            Condition("3+ enemies and 40+ Focus", [this](Player* bot, Unit*) {

                                return this->GetEnemiesInRange(40.0f) >= 3 && this->_resource >= 40;

                            }),

                            bot::ai::Action("Cast Multishot", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->_resource >= 40)

                                {

                                    this->CastSpell(SPELL_MULTISHOT, target);

                                    this->ConsumeResource(40);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),
                        // Single target filler

                        Sequence("Single Target Filler", {

                            Condition("35+ Focus", [this](Player* bot, Unit*) {

                                return this->_resource >= 35;

                            }),

                            bot::ai::Action("Cast Cobra Shot", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->_resource >= 35)

                                {

                                    this->CastSpell(SPELL_COBRA_SHOT, target);

                                    this->_lastCobraShot = GameTime::GetGameTimeMS();

                                    this->ConsumeResource(35);

                                    this->_resource = ::std::min<uint32>(this->_resource + 5, 100);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                })

            });


            behaviorTree->SetRoot(root);

            TC_LOG_INFO("module.playerbot", " BEAST MASTERY HUNTER: BehaviorTree initialized with 4-tier DPS rotation");
        }
    }

private:
    BeastMasteryPetManager _petManager;

    // Barbed Shot management
    uint32 _barbedShotCharges;
    uint32 _lastBarbedShotRecharge;

    // Cooldown tracking
    bool _bestialWrathActive;
    uint32 _bestialWrathEndTime;
    bool _aspectOfTheWildActive;
    uint32 _aspectEndTime;

    // Proc tracking
    bool _wildCallProc;

    // Ability timing
    uint32 _lastKillCommand;
    uint32 _lastCobraShot;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
