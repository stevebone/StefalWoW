/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Survival Hunter Specialization - REFACTORED
 *
 * This demonstrates the complete migration of Survival Hunter to the template-based
 * architecture, eliminating code duplication while maintaining full functionality.
 *
 * Survival is unique as a melee-focused hunter spec that uses bombs, traps,
 * and coordinated attacks with their pet for sustained damage.
 */

#pragma once


#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "../SpellValidation_WoW120.h"
#include "../HeroTalentDetector.h"      // Hero talent tree detection
#include "ObjectGuid.h"
#include "../../../Spatial/SpatialGridManager.h"
#include "ObjectAccessor.h"
#include "Creature.h"
#include "Map.h"
#include "../CombatSpecializationTemplates.h"
#include "../ResourceTypes.h"
#include "Pet.h"
#include "PetDefines.h"
#include "MotionMaster.h"
#include "CharmInfo.h"
#include "HunterAI.h"
#include <unordered_map>
#include <array>

// Phase 5 Integration: Decision Systems
#include "../../Decision/ActionPriorityQueue.h"
#include "../../Decision/BehaviorTree.h"
#include "../BotAI.h"

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
// SURVIVAL HUNTER SPELL IDs (WoW 12.0 - The War Within)
// Using centralized spell registry from SpellValidation_WoW120.h
// ============================================================================

enum SurvivalSpells
{
    // Core Melee Abilities - Using central registry: WoW120Spells::Hunter::Survival
    SPELL_RAPTOR_STRIKE        = WoW120Spells::Hunter::Survival::RAPTOR_STRIKE,
    SPELL_MONGOOSE_BITE        = WoW120Spells::Hunter::Survival::MONGOOSE_BITE,
    SPELL_CARVE                = WoW120Spells::Hunter::Survival::CARVE,
    SPELL_BUTCHERY             = WoW120Spells::Hunter::Survival::BUTCHERY,
    SPELL_COORDINATED_ASSAULT  = WoW120Spells::Hunter::Survival::COORDINATED_ASSAULT,
    SPELL_FLANKING_STRIKE      = WoW120Spells::Hunter::Survival::FLANKING_STRIKE,

    // Wildfire Bombs - Using central registry: WoW120Spells::Hunter::Survival
    SPELL_WILDFIRE_BOMB        = WoW120Spells::Hunter::Survival::WILDFIRE_BOMB,
    SPELL_SHRAPNEL_BOMB        = WoW120Spells::Hunter::Survival::SHRAPNEL_BOMB,
    SPELL_PHEROMONE_BOMB       = WoW120Spells::Hunter::Survival::PHEROMONE_BOMB,
    SPELL_VOLATILE_BOMB        = WoW120Spells::Hunter::Survival::VOLATILE_BOMB,
    SPELL_WILDFIRE_INFUSION    = WoW120Spells::Hunter::Survival::WILDFIRE_INFUSION,

    // DoTs and Debuffs - Using central registry: WoW120Spells::Hunter::Survival
    SPELL_SERPENT_STING        = WoW120Spells::Hunter::Survival::SERPENT_STING,
    SPELL_INTERNAL_BLEEDING    = WoW120Spells::Hunter::Survival::INTERNAL_BLEEDING,
    SPELL_BLOODSEEKER          = WoW120Spells::Hunter::Survival::BLOODSEEKER,

    // Focus Management - Using central registry: WoW120Spells::Hunter::Survival
    SPELL_KILL_COMMAND_SURV    = WoW120Spells::Hunter::Survival::KILL_COMMAND_SURVIVAL,
    SPELL_TERMS_OF_ENGAGEMENT  = WoW120Spells::Hunter::Survival::TERMS_OF_ENGAGEMENT,
    SPELL_HARPOON              = WoW120Spells::Hunter::Survival::HARPOON,

    // Utility - Using central registry: WoW120Spells::Hunter::Survival
    SPELL_ASPECT_OF_EAGLE      = WoW120Spells::Hunter::Survival::ASPECT_OF_THE_EAGLE,
    SPELL_MUZZLE               = WoW120Spells::Hunter::Survival::MUZZLE,
    SPELL_STEEL_TRAP           = WoW120Spells::Hunter::Survival::STEEL_TRAP,
    SPELL_GUERRILLA_TACTICS    = WoW120Spells::Hunter::Survival::GUERRILLA_TACTICS,

    // Pet - Using central registry: WoW120Spells::Hunter
    SPELL_CALL_PET_SURV        = WoW120Spells::Hunter::CALL_PET_1,
    SPELL_MEND_PET_SURV        = WoW120Spells::Hunter::MEND_PET,

    // Defensives - Using central registry: WoW120Spells::Hunter
    SPELL_ASPECT_TURTLE_SURV   = WoW120Spells::Hunter::ASPECT_OF_THE_TURTLE,
    SPELL_EXHILARATION_SURV    = WoW120Spells::Hunter::EXHILARATION,
    SPELL_SURVIVAL_OF_FITTEST  = WoW120Spells::Hunter::SURVIVAL_OF_THE_FITTEST
};

/**
 * Wildfire Bomb Management System
 *
 * Handles the unique bomb mechanic of Survival, including different bomb types
 * and their recharge system.
 */
class WildfireBombManager
{
public:
    enum BombType
    {
        BOMB_WILDFIRE = 0,
        BOMB_SHRAPNEL = 1,
        BOMB_PHEROMONE = 2,
        BOMB_VOLATILE = 3
    };

    WildfireBombManager()
        : _charges(2)
        , _maxCharges(2)
        , _lastRecharge(0)
        , _rechargeTime(18000) // 18 second recharge
        , _nextBombType(BOMB_WILDFIRE)
        , _hasWildfireInfusion(false)
    {
    }

    bool HasCharge() const { return _charges > 0; }

    uint32 GetCharges() const { return _charges; }

    void UseCharge()
    {
        if (_charges > 0)
        {

            _charges--;

            // Roll for next bomb type if we have Wildfire Infusion

            if (_hasWildfireInfusion)

            {

                _nextBombType = static_cast<BombType>(rand() % 4);

            }
        }
    }

    void UpdateRecharge()
    {
        if (_charges >= _maxCharges)

            return;

        uint32 currentTime = GameTime::GetGameTimeMS();
        if (currentTime - _lastRecharge > _rechargeTime)
        {

            _charges++;

            _lastRecharge = currentTime;
        }
    }

    uint32 GetBombSpell() const
    {
        if (!_hasWildfireInfusion)

            return SPELL_WILDFIRE_BOMB;

        switch (_nextBombType)
        {

            case BOMB_SHRAPNEL:  return SPELL_SHRAPNEL_BOMB;

            case BOMB_PHEROMONE: return SPELL_PHEROMONE_BOMB;

            case BOMB_VOLATILE:  return SPELL_VOLATILE_BOMB;

            default:
            return SPELL_WILDFIRE_BOMB;
        }
    }

    BombType GetNextBombType() const { return _nextBombType; }

    void EnableWildfireInfusion() { _hasWildfireInfusion = true; }

private:
    CooldownManager _cooldowns;
    uint32 _charges;
    uint32 _maxCharges;
    uint32 _lastRecharge;
    uint32 _rechargeTime;
    BombType _nextBombType;
    bool _hasWildfireInfusion;
};

/**
 * Mongoose Bite Stack Tracking
 *
 * Manages the stacking mechanic of Mongoose Bite for optimal damage
 */
class MongooseBiteTracker
{
public:
    MongooseBiteTracker()
        : _stacks(0)
        , _maxStacks(5)
        , _windowEndTime(0)
        , _charges(3)
        , _maxCharges(3)
        , _lastRecharge(0)
    {
    }

    void OnMongooseBiteCast()
    {
        uint32 currentTime = GameTime::GetGameTimeMS();

        // Start or extend window
        if (_stacks == 0 || currentTime > _windowEndTime)
        {

            _stacks = 1;

            _windowEndTime = currentTime + 14000; // 14 second window
        }
        else
        {

            _stacks = ::std::min<uint32>(_stacks + 1, _maxStacks);

            _windowEndTime = currentTime + 14000; // Refresh window
        }

        // Consume charge
        if (_charges > 0)

            _charges--;
    }

    bool HasCharges() const { return _charges > 0; }

    uint32 GetStacks() const
    {
        if (GameTime::GetGameTimeMS() > _windowEndTime)

            return 0;
        return _stacks;
    }

    void UpdateCharges()
    {
        if (_charges >= _maxCharges)

            return;

        uint32 currentTime = GameTime::GetGameTimeMS();
        if (currentTime - _lastRecharge > 12000) // 12 second recharge
        {

            _charges++;

            _lastRecharge = currentTime;
        }
    }

    bool IsWindowActive() const
    {
        return GameTime::GetGameTimeMS() < _windowEndTime;
    }

    void Reset()
    {
        _stacks = 0;
        _windowEndTime = 0;
    }

private:
    uint32 _stacks;
    uint32 _maxStacks;
    uint32 _windowEndTime;
    uint32 _charges;
    uint32 _maxCharges;
    uint32 _lastRecharge;
};

/**
 * Survival Pet Manager
 *
 * Simplified pet management for Survival (less critical than BM)
 */
class SurvivalPetManager
{
public:
    explicit SurvivalPetManager(Player* bot)
        : _bot(bot)
        , _lastMendPet(0)
    {
    }

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
            TC_LOG_DEBUG("module.playerbot.ai", "Survival Hunter {} pet {} was PASSIVE, setting to DEFENSIVE",
                         bot->GetName(), pet->GetName());
            pet->SetReactState(REACT_DEFENSIVE);
        }

        // Command pet to attack if not already
        if (pet && pet->IsAlive() && pet->GetVictim() != target)
        {
            pet->Attack(target, true);
        }

        // Heal pet if needed
        if (IsPetHealthLowSafe(bot))
        {
            MendPetSafe(bot);
        }
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

private:
    void SummonPet()
    {
        if (!_bot || HasActivePet())
            return;
        _bot->CastSpell(CastSpellTargetArg(_bot), SPELL_CALL_PET_SURV);
    }

    // Thread-safe version using validated pointer
    void SummonPetSafe(Player* bot)
    {
        if (!bot || HasActivePetSafe(bot))
            return;
        bot->CastSpell(CastSpellTargetArg(bot), SPELL_CALL_PET_SURV);
    }

    bool IsPetHealthLow() const
    {
        if (!HasActivePet())
            return false;

        Pet* pet = _bot->GetPet();
        return pet && pet->GetHealthPct() < 60.0f;
    }

    // Thread-safe version using validated pointer
    bool IsPetHealthLowSafe(Player* bot) const
    {
        if (!HasActivePetSafe(bot))
            return false;

        Pet* pet = bot->GetPet();
        return pet && pet->GetHealthPct() < 60.0f;
    }

    void MendPet()
    {
        uint32 currentTime = GameTime::GetGameTimeMS();
        if (currentTime - _lastMendPet < 10000)
            return;

        Pet* pet = _bot->GetPet();
        if (pet && pet->IsAlive() && !_bot->HasAura(SPELL_MEND_PET_SURV))
        {
            _bot->CastSpell(CastSpellTargetArg(pet), SPELL_MEND_PET_SURV);
            _lastMendPet = currentTime;
        }
    }

    // Thread-safe version using validated pointer
    void MendPetSafe(Player* bot)
    {
        uint32 currentTime = GameTime::GetGameTimeMS();
        if (currentTime - _lastMendPet < 10000)
            return;

        Pet* pet = bot->GetPet();
        if (pet && pet->IsAlive() && !bot->HasAura(SPELL_MEND_PET_SURV))
        {
            bot->CastSpell(CastSpellTargetArg(pet), SPELL_MEND_PET_SURV);
            _lastMendPet = currentTime;
        }
    }

    Player* _bot;
    uint32 _lastMendPet;
};

/**
 * Refactored Survival Hunter using template architecture
 *
 * IMPORTANT: Survival inherits from RangedDpsSpecialization but overrides
 * positioning to be melee-focused. This is unique among hunter specs.
 *
 * Key features:
 * - Melee positioning override (5.0f range instead of 40.0f)
 * - Wildfire Bomb management with different bomb types
 * - Mongoose Bite stacking mechanics
 * - Coordinated Assault burst windows
 * - DoT maintenance with Serpent Sting
 */
class SurvivalHunterRefactored : public RangedDpsSpecialization<FocusResource>
{
public:
    using Base = RangedDpsSpecialization<FocusResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::CanUseAbility;
    using Base::ConsumeResource;
    using Base::_resource;
    explicit SurvivalHunterRefactored(Player* bot)
        : RangedDpsSpecialization<FocusResource>(bot)
        , _bombManager()
        , _mongooseTracker()
        , _petManager(bot)
        , _coordinatedAssaultActive(false)
        , _coordinatedAssaultEndTime(0)
        , _aspectOfEagleActive(false)
        , _lastRaptorStrike(0)
        , _lastKillCommand(0)
        , _lastSerpentSting(0)
        , _guerillaTacticsActive(true)
    {
        // Focus regeneration handled by base template class
        // Survival has standard hunter focus regeneration (10 focus/sec)

        // Enable Wildfire Infusion if talented
        _bombManager.EnableWildfireInfusion();

        // Setup Survival-specific cooldown tracking

        // Phase 5 Integration: Initialize decision systems
        InitializeSurvivalMechanics();
    }

    // ========================================================================
    // POSITIONING OVERRIDE - Survival is MELEE
    // Note: GetOptimalRange is final in base class, positioning handled via other methods
    // ========================================================================

    bool ShouldMaintainRange() const
    {        // Survival wants to be in melee range
        return false;
    }

    // ========================================================================
    // CORE ROTATION - Survival specific logic
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
            // Pack Leader: Vicious Hunt for coordinated melee pet burst
            if (this->_resource >= 30 &&
                this->CanCastSpell(WoW120Spells::Hunter::Survival::SV_VICIOUS_HUNT, target))
            {
                this->CastSpell(WoW120Spells::Hunter::Survival::SV_VICIOUS_HUNT, target);
                this->ConsumeResource(30);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::SENTINEL))
        {
            // Sentinel: Enhanced sentinel tracking for melee precision
            if (this->CanCastSpell(WoW120Spells::Hunter::Survival::SV_SENTINEL, target))
            {
                this->CastSpell(WoW120Spells::Hunter::Survival::SV_SENTINEL, target);
                return;
            }
        }

        // Ensure pet is helping
        _petManager.EnsurePetActive(target);

        // Update Survival-specific mechanics
        UpdateSurvivalState();

        // Check if we need to gap close
        float distance = this->GetBot()->GetDistance(target);
        if (distance > 5.0f && distance < 30.0f)
        {

            UseHarpoon(target);

            return;
        }

        // Check for AoE situation
        uint32 enemyCount = this->GetEnemiesInRange(8.0f);
        if (enemyCount >= 3)

        {
        ExecuteAoERotation(target);

            return;
        }

        // Single target rotation        ExecuteSingleTargetRotation(target);
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();        // Ensure pet is summoned
        if (!_petManager.HasActivePet())
        {

            bot->CastSpell(CastSpellTargetArg(bot), SPELL_CALL_PET_SURV);

            return;
        }

        // Use Aspect of the Eagle for extended range when needed
        if (bot->IsInCombat() && !_aspectOfEagleActive && this->CanUseAbility(SPELL_ASPECT_OF_EAGLE))
        {

            if (this->GetEnemiesInRange(8.0f) > 0)

            {

                this->CastSpell(SPELL_ASPECT_OF_EAGLE, bot);

                _aspectOfEagleActive = true;

            }
        }

        // Use Survival of the Fittest for defense
        if (bot->GetHealthPct() < 50.0f && this->CanUseAbility(SPELL_SURVIVAL_OF_FITTEST))
        {

            this->CastSpell(SPELL_SURVIVAL_OF_FITTEST, bot);
        }

        // Use Exhilaration for healing
        if (bot->GetHealthPct() < 40.0f && this->CanUseAbility(SPELL_EXHILARATION_SURV))
        {

            this->CastSpell(SPELL_EXHILARATION_SURV, bot);
        }
    }

    void OnInterruptRequired(::Unit* target, uint32 /*spellId*/)
    {
        if (this->CanUseAbility(SPELL_MUZZLE))
        {

            this->CastSpell(SPELL_MUZZLE, target);
        }
    }

protected:
    // ========================================================================
    // RESOURCE MANAGEMENT OVERRIDE
    // ========================================================================

    uint32 GetResourceCost(uint32 spellId)
    {
        switch (spellId)
        {

            case SPELL_RAPTOR_STRIKE:
            return 30;

            case SPELL_MONGOOSE_BITE:
            return 30;

            case SPELL_CARVE:
            return 35;

            case SPELL_BUTCHERY:
            return 30;

            case SPELL_KILL_COMMAND_SURV:   return 0;   // Generates 15 focus

            case SPELL_WILDFIRE_BOMB:
            return 0;   // No cost

            case SPELL_SERPENT_STING:
            return 20;
            case SPELL_FLANKING_STRIKE:
            return 30;

            case SPELL_HARPOON:
            return 0;   // No cost

            case SPELL_COORDINATED_ASSAULT: return 0;   // No cost

            default:
            return 20;
        }
    }

    // ========================================================================
    // SURVIVAL SPECIFIC ROTATION LOGIC
    // ========================================================================

    void ExecuteSingleTargetRotation(::Unit* target)
    {
        uint32 currentFocus = this->_resource;

        // Priority 1: Coordinated Assault for burst
        if (ShouldUseCoordinatedAssault(target) && this->CanUseAbility(SPELL_COORDINATED_ASSAULT))
        {

            this->CastSpell(SPELL_COORDINATED_ASSAULT, this->GetBot());

            _coordinatedAssaultActive = true;

            _coordinatedAssaultEndTime = GameTime::GetGameTimeMS() + 20000;

            return;
        }

        // Priority 2: Maintain Serpent Sting
        if (!target->HasAura(SPELL_SERPENT_STING) && currentFocus >= 20)        {

            this->CastSpell(SPELL_SERPENT_STING, target);

            _lastSerpentSting = GameTime::GetGameTimeMS();

            this->ConsumeResource(20);

            return;
        }

        // Priority 3: Wildfire Bomb on cooldown
        if (_bombManager.HasCharge())
        {

            uint32 bombSpell = _bombManager.GetBombSpell();

            this->CastSpell(bombSpell, target);

            _bombManager.UseCharge();

            // Guerrilla Tactics makes first bomb stronger

            if (_guerillaTacticsActive)

            {

                _guerillaTacticsActive = false;

            }

            return;
        }

        // Priority 4: Kill Command for focus generation
        if (currentFocus < 50 && this->CanUseAbility(SPELL_KILL_COMMAND_SURV))
        {

            this->CastSpell(SPELL_KILL_COMMAND_SURV, target);

            _lastKillCommand = GameTime::GetGameTimeMS();

            this->_resource = ::std::min<uint32>(this->_resource + 15, 100);

            return;
        }

        // Priority 5: Mongoose Bite during window or with charges
        if (_mongooseTracker.IsWindowActive() || _mongooseTracker.HasCharges())
        {

            if (currentFocus >= 30)

            {

                this->CastSpell(SPELL_MONGOOSE_BITE, target);

                _mongooseTracker.OnMongooseBiteCast();

                this->ConsumeResource(30);

                return;

            }
        }

        // Priority 6: Flanking Strike for coordination
        if (currentFocus >= 30 && this->CanUseAbility(SPELL_FLANKING_STRIKE))
        {

            this->CastSpell(SPELL_FLANKING_STRIKE, target);

            this->ConsumeResource(30);

            this->_resource = ::std::min<uint32>(this->_resource + 15, 100); // Returns some focus

            return;
        }

        // Priority 7: Raptor Strike as filler
        if (currentFocus >= 30)
        {

            this->CastSpell(SPELL_RAPTOR_STRIKE, target);

            _lastRaptorStrike = GameTime::GetGameTimeMS();

            this->ConsumeResource(30);

            return;
        }

        // Priority 8: Kill Command if nothing else
        if (this->CanUseAbility(SPELL_KILL_COMMAND_SURV))
        {

            this->CastSpell(SPELL_KILL_COMMAND_SURV, target);

            this->_resource = ::std::min<uint32>(this->_resource + 15, 100);

            return;
        }
    }

    void ExecuteAoERotation(::Unit* target)
    {
        uint32 currentFocus = this->_resource;

        // Priority 1: Wildfire Bomb for AoE
        if (_bombManager.HasCharge())
        {

            uint32 bombSpell = _bombManager.GetBombSpell();

            this->CastSpell(bombSpell, target);

            _bombManager.UseCharge();

            return;
        }

        // Priority 2: Butchery for burst AoE
        if (currentFocus >= 30 && this->CanUseAbility(SPELL_BUTCHERY))
        {

            this->CastSpell(SPELL_BUTCHERY, this->GetBot());

            this->ConsumeResource(30);

            return;
        }

        // Priority 3: Carve for cleave
        if (currentFocus >= 35)
        {

            this->CastSpell(SPELL_CARVE, this->GetBot());

            this->ConsumeResource(35);

            return;
        }

        // Priority 4: Kill Command for focus
        if (currentFocus < 50 && this->CanUseAbility(SPELL_KILL_COMMAND_SURV))
        {

            this->CastSpell(SPELL_KILL_COMMAND_SURV, target);

            this->_resource = ::std::min<uint32>(this->_resource + 15, 100);

            return;
        }

        // Priority 5: Serpent Sting on multiple targets
        ApplySerpentStingToMultiple();
    }

private:
    // ========================================================================
    // SURVIVAL STATE MANAGEMENT
    // ========================================================================

    void UpdateSurvivalState()
    {
        uint32 currentTime = GameTime::GetGameTimeMS();

        // Update bomb recharge
        _bombManager.UpdateRecharge();

        // Update Mongoose Bite charges
        _mongooseTracker.UpdateCharges();

        // Check Coordinated Assault expiry
        if (_coordinatedAssaultActive && currentTime > _coordinatedAssaultEndTime)
        {

            _coordinatedAssaultActive = false;

            _coordinatedAssaultEndTime = 0;
        }

        // Reset Mongoose window if expired
        if (!_mongooseTracker.IsWindowActive())
        {

            _mongooseTracker.Reset();
        }

        // Check Aspect of Eagle (90 second duration)
        static uint32 aspectStartTime = 0;
        if (_aspectOfEagleActive)
        {

            if (aspectStartTime == 0)

                aspectStartTime = currentTime;
                if (currentTime - aspectStartTime > 90000)

            {

                _aspectOfEagleActive = false;

                aspectStartTime = 0;

            }
        }
    }

    void UseHarpoon(Unit* target)
    {
        if (this->CanUseAbility(SPELL_HARPOON))
        {

            this->CastSpell(SPELL_HARPOON, target);

            // Terms of Engagement generates focus

            if (this->GetBot()->HasAura(SPELL_TERMS_OF_ENGAGEMENT))

            {

                this->_resource = ::std::min<uint32>(this->_resource + 20, 100);

            }
        }
    }

    bool ShouldUseCoordinatedAssault(Unit* target) const
    {
        if (!target)

            return false;

        // Use on high priority targets or when we have mongoose stacks
        return (target->GetHealthPct() > 50.0f) ||

               (_mongooseTracker.GetStacks() >= 3) ||

               (target->GetLevel() > this->GetBot()->GetLevel() + 2);
    }

    void ApplySerpentStingToMultiple()
    {
        uint32 currentFocus = this->_resource;
        if (currentFocus < 20)

            return;

        // Get enemies in range
        ::std::list<Unit*> enemies;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck checker(this->GetBot(), this->GetBot(), 8.0f);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(this->GetBot(), enemies, checker);
        // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitAllObjects
        Map* map = this->GetBot()->GetMap();
        if (map)
        {

            auto* spatialGrid = Playerbot::SpatialGridManager::Instance().GetGrid(map);

            if (spatialGrid)

            {

                auto guids = spatialGrid->QueryNearbyCreatureGuids(*this->GetBot(), 8.0f);

                for (ObjectGuid guid : guids)

                {

                    if (Creature* creature = ObjectAccessor::GetCreature(*this->GetBot(), guid))

                    {

                        if (checker(creature))

                            enemies.push_back(creature);

                    }

                }

            }
        }

        uint32 stingsApplied = 0;
        for (Unit* enemy : enemies)
        {

            if (stingsApplied >= 3) // Limit to 3 targets

                break;


            if (!enemy->HasAura(SPELL_SERPENT_STING) && currentFocus >= 20)

            {

                this->CastSpell(SPELL_SERPENT_STING, enemy);

                this->ConsumeResource(20);

                currentFocus -= 20;

                stingsApplied++;

            }
        }
    }

    private:    // ========================================================================
    // HUNTER SPECIALIZATION ABSTRACT METHOD IMPLEMENTATIONS
    // ========================================================================

    // Pet management - implemented by SurvivalPetManager
    void UpdatePetManagement() { _petManager.EnsurePetActive(GetBot()->GetVictim()); }
    void SummonPet() { GetBot()->CastSpell(CastSpellTargetArg(GetBot()), SPELL_CALL_PET_SURV); }
    void MendPetIfNeeded() { if (_petManager.HasActivePet()) _petManager.EnsurePetActive(GetBot()->GetVictim()); }
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
            info.lastCommand = 0; // SurvivalPetManager doesn't track pet commands
        }
        return info;
    }

    // Trap management - delegated to AI
    void UpdateTrapManagement() { /* Traps managed by AI */ }
    void PlaceTrap(uint32 /*trapSpell*/, Position /*position*/) { /* Traps managed by AI */ }
    bool ShouldPlaceTrap() const { return false; }
    uint32 GetOptimalTrapSpell() const { return SPELL_STEEL_TRAP; }
    ::std::vector<TrapInfo> GetActiveTraps() const { return ::std::vector<TrapInfo>(); }

    // Aspect management - delegated to UpdateBuffs
    void UpdateAspectManagement() { /* Aspects managed in UpdateBuffs */ }
    void SwitchToOptimalAspect() { /* Aspects managed in UpdateBuffs */ }
    uint32 GetOptimalAspect() const { return SPELL_ASPECT_OF_EAGLE; }
    bool HasCorrectAspect() const { return true; }

    // Range and positioning - Survival is MELEE (unique)
    void UpdateRangeManagement() { /* Survival fights in melee */ }
    bool IsInDeadZone(::Unit* /*target*/) const { return false; /* No dead zone for melee */ }
    bool ShouldKite(::Unit* target) const { return target && GetBot()->GetHealthPct() < 30.0f; }
    Position GetKitePosition(::Unit* target) const
    {
        if (!target) return Position();
        // Get position 15 yards away from target
        float angle = target->GetRelativeAngle(GetBot());
        float x = target->GetPositionX() + 15.0f * ::std::cos(angle);
        float y = target->GetPositionY() + 15.0f * ::std::sin(angle);        return Position(x, y, target->GetPositionZ());
    }
    void HandleDeadZone(::Unit* /*target*/) { /* No dead zone for melee spec */ }

    // Tracking management - delegated to AI
    void UpdateTracking() { /* Tracking managed by AI */ }
    uint32 GetOptimalTracking() const { return 0; /* No specific tracking */ }
    void ApplyTracking(uint32 /*trackingSpell*/) { /* Applied by AI */ }

    // Pet command interface - delegated to pet manager
    void CommandPetAttack(::Unit* target)
    {
        if (target) _petManager.EnsurePetActive(target);
    }
    void CommandPetFollow() { /* Handled by pet AI */ }
    void CommandPetStay() { /* Handled by pet AI */ }

    // Positioning interface - MELEE positioning (unique for Survival)
    // Note: GetOptimalRange is final in base class - Survival should base melee range behavior
    // The base class RangedDpsSpecialization returns 30-40 yards, but Survival needs melee (see line 364)
    Position GetOptimalPosition(::Unit* /*target*/) { return Position(); /* Handled by base class */ }

    // Phase 5 Integration: Decision Systems Initialization
    void InitializeSurvivalMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;

        // ========================================================================
        // ActionPriorityQueue: Register Survival Hunter spells with priorities
        // ========================================================================
        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Survival cooldowns

            queue->RegisterSpell(SPELL_SURVIVAL_OF_FITTEST, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);

            queue->AddCondition(SPELL_SURVIVAL_OF_FITTEST, [this](Player* bot, Unit*) {

                return bot && bot->GetHealthPct() < 50.0f;

            }, "Bot HP < 50% (damage reduction)");


            queue->RegisterSpell(SPELL_EXHILARATION_SURV, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);

            queue->AddCondition(SPELL_EXHILARATION_SURV, [this](Player* bot, Unit*) {

                return bot && bot->GetHealthPct() < 40.0f;

            }, "Bot HP < 40% (self heal + pet heal)");

            // CRITICAL: Burst cooldowns and gap closer

            queue->RegisterSpell(SPELL_COORDINATED_ASSAULT, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);

            queue->AddCondition(SPELL_COORDINATED_ASSAULT, [this](Player* bot, Unit* target) {

                return target && !this->_coordinatedAssaultActive &&

                       this->ShouldUseCoordinatedAssault(target);

            }, "Not active, suitable target (20s burst)");


            queue->RegisterSpell(SPELL_HARPOON, SpellPriority::CRITICAL, SpellCategory::UTILITY);

            queue->AddCondition(SPELL_HARPOON, [this](Player* bot, Unit* target) {

                return target && bot->GetDistance(target) > 5.0f &&

                       bot->GetDistance(target) < 30.0f;

            }, "5-30 yards from target (gap closer)");

            // HIGH: Core rotation abilities

            queue->RegisterSpell(SPELL_WILDFIRE_BOMB, SpellPriority::HIGH, SpellCategory::DAMAGE_AOE);

            queue->AddCondition(SPELL_WILDFIRE_BOMB, [this](Player* bot, Unit* target) {

                return target && this->_bombManager.HasCharge();

            }, "Has bomb charge (2 charges, 18s recharge)");


            queue->RegisterSpell(SPELL_KILL_COMMAND_SURV, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(SPELL_KILL_COMMAND_SURV, [this](Player* bot, Unit* target) {

                return target && this->_resource < 50 &&

                       this->_petManager.HasActivePet();

            }, "< 50 Focus, pet alive (generates 15 Focus)");


            queue->RegisterSpell(SPELL_SERPENT_STING, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(SPELL_SERPENT_STING, [this](Player* bot, Unit* target) {

                return target && this->_resource >= 20 &&

                       !target->HasAura(SPELL_SERPENT_STING);

            }, "20+ Focus, DoT missing (primary DoT)");

            // MEDIUM: Stacking and coordination abilities

            queue->RegisterSpell(SPELL_MONGOOSE_BITE, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(SPELL_MONGOOSE_BITE, [this](Player* bot, Unit* target) {

                return target && this->_resource >= 30 &&

                       (this->_mongooseTracker.IsWindowActive() || this->_mongooseTracker.HasCharges());

            }, "30+ Focus, window active or has charges (stacks to 5)");


            queue->RegisterSpell(SPELL_FLANKING_STRIKE, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(SPELL_FLANKING_STRIKE, [this](Player* bot, Unit* target) {

                return bot && bot->HasSpell(SPELL_FLANKING_STRIKE) &&

                       target && this->_resource >= 30 &&

                       this->_petManager.HasActivePet();

            }, "Has talent, 30+ Focus, pet alive (pet coordination)");


            queue->RegisterSpell(SPELL_MUZZLE, SpellPriority::MEDIUM, SpellCategory::UTILITY);

            queue->AddCondition(SPELL_MUZZLE, [this](Player* bot, Unit* target) {

                return target && target->IsNonMeleeSpellCast(false);

            }, "Target casting (interrupt)");

            // LOW: Filler abilities

            queue->RegisterSpell(SPELL_RAPTOR_STRIKE, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);

            queue->AddCondition(SPELL_RAPTOR_STRIKE, [this](Player* bot, Unit* target) {

                return target && this->_resource >= 30 &&

                       this->GetEnemiesInRange(8.0f) < 3;

            }, "30+ Focus, < 3 enemies (single target filler)");


            queue->RegisterSpell(SPELL_CARVE, SpellPriority::LOW, SpellCategory::DAMAGE_AOE);

            queue->AddCondition(SPELL_CARVE, [this](Player* bot, Unit* target) {

                return target && this->_resource >= 35 &&

                       this->GetEnemiesInRange(8.0f) >= 3;

            }, "35+ Focus, 3+ enemies (AoE cleave)");


            queue->RegisterSpell(SPELL_BUTCHERY, SpellPriority::LOW, SpellCategory::DAMAGE_AOE);

            queue->AddCondition(SPELL_BUTCHERY, [this](Player* bot, Unit* target) {

                return bot && bot->HasSpell(SPELL_BUTCHERY) &&

                       target && this->_resource >= 30 &&

                       this->GetEnemiesInRange(8.0f) >= 3;

            }, "Has talent, 30+ Focus, 3+ enemies (AoE burst)");


            TC_LOG_INFO("module.playerbot", " SURVIVAL HUNTER: Registered {} spells in ActionPriorityQueue", queue->GetSpellCount());
        }

        // ========================================================================
        // BehaviorTree: Survival Hunter melee DPS rotation logic
        // ========================================================================
        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {

            auto root = Selector("Survival Hunter Melee DPS", {
                // Tier 1: Burst Window (Coordinated Assault)

                Sequence("Burst Cooldowns", {

                    Condition("Target exists", [this](Player* bot, Unit* target) {

                        return target != nullptr;

                    }),

                    Selector("Use Burst", {

                        Sequence("Cast Coordinated Assault", {

                            Condition("Should use CA", [this](Player* bot, Unit* target) {

                                return !this->_coordinatedAssaultActive &&

                                       this->ShouldUseCoordinatedAssault(target);

                            }),

                            bot::ai::Action("Cast Coordinated Assault", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->CanUseAbility(SPELL_COORDINATED_ASSAULT))

                                {

                                    this->CastSpell(SPELL_COORDINATED_ASSAULT, bot);

                                    this->_coordinatedAssaultActive = true;

                                    this->_coordinatedAssaultEndTime = GameTime::GetGameTimeMS() + 20000;

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 2: Resource Management (Wildfire Bomb, Kill Command, Serpent Sting)

                Sequence("Resource Management", {

                    Condition("Target exists", [this](Player* bot, Unit* target) {

                        return target != nullptr;

                    }),

                    Selector("Manage Resources", {
                        // Wildfire Bomb (has charge)

                        Sequence("Cast Wildfire Bomb", {

                            Condition("Has bomb charge", [this](Player* bot, Unit*) {

                                return this->_bombManager.HasCharge();

                            }),

                            bot::ai::Action("Cast Wildfire Bomb", [this](Player* bot, Unit* target) -> NodeStatus {

                                uint32 bombSpell = this->_bombManager.GetBombSpell();

                                this->CastSpell(bombSpell, target);

                                this->_bombManager.UseCharge();

                                if (this->_guerillaTacticsActive)

                                    this->_guerillaTacticsActive = false;

                                return NodeStatus::SUCCESS;

                            })

                        }),
                        // Kill Command (focus generation)

                        Sequence("Cast Kill Command", {

                            Condition("< 50 Focus and pet alive", [this](Player* bot, Unit*) {

                                return this->_resource < 50 && this->_petManager.HasActivePet();

                            }),

                            bot::ai::Action("Cast Kill Command", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->CanUseAbility(SPELL_KILL_COMMAND_SURV))

                                {

                                    this->CastSpell(SPELL_KILL_COMMAND_SURV, target);

                                    this->_lastKillCommand = GameTime::GetGameTimeMS();

                                    this->_resource = ::std::min<uint32>(this->_resource + 15, 100);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),
                        // Serpent Sting (DoT maintenance)

                        Sequence("Maintain Serpent Sting", {

                            Condition("DoT missing and 20+ Focus", [this](Player* bot, Unit* target) {

                                return !target->HasAura(SPELL_SERPENT_STING) && this->_resource >= 20;

                            }),

                            bot::ai::Action("Cast Serpent Sting", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->_resource >= 20)

                                {

                                    this->CastSpell(SPELL_SERPENT_STING, target);

                                    this->_lastSerpentSting = GameTime::GetGameTimeMS();

                                    this->ConsumeResource(20);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 3: Melee Abilities (Mongoose Bite, Flanking Strike)

                Sequence("Melee Abilities", {

                    Condition("Target exists and 30+ Focus", [this](Player* bot, Unit* target) {

                        return target != nullptr && this->_resource >= 30;

                    }),

                    Selector("Cast Melee Abilities", {
                        // Mongoose Bite (stacking)

                        Sequence("Cast Mongoose Bite", {

                            Condition("Window active or has charges", [this](Player* bot, Unit*) {

                                return this->_mongooseTracker.IsWindowActive() ||

                                       this->_mongooseTracker.HasCharges();

                            }),

                            bot::ai::Action("Cast Mongoose Bite", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->_resource >= 30)

                                {

                                    this->CastSpell(SPELL_MONGOOSE_BITE, target);

                                    this->_mongooseTracker.OnMongooseBiteCast();

                                    this->ConsumeResource(30);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        }),
                        // Flanking Strike (pet coordination)

                        Sequence("Cast Flanking Strike", {

                            Condition("Has talent and pet alive", [this](Player* bot, Unit*) {

                                return bot && bot->HasSpell(SPELL_FLANKING_STRIKE) &&

                                       this->_petManager.HasActivePet();

                            }),

                            bot::ai::Action("Cast Flanking Strike", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->CanUseAbility(SPELL_FLANKING_STRIKE))

                                {

                                    this->CastSpell(SPELL_FLANKING_STRIKE, target);

                                    this->ConsumeResource(30);

                                    this->_resource = ::std::min<uint32>(this->_resource + 15, 100);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                }),

                // Tier 4: Filler Rotation (Raptor Strike ST, Carve/Butchery AoE)

                Sequence("Filler Rotation", {

                    Condition("Target exists and 30+ Focus", [this](Player* bot, Unit* target) {

                        return target != nullptr && this->_resource >= 30;

                    }),

                    Selector("Choose Filler", {
                        // AoE filler (3+ enemies)

                        Sequence("AoE Filler", {

                            Condition("3+ enemies", [this](Player* bot, Unit*) {

                                return this->GetEnemiesInRange(8.0f) >= 3;

                            }),

                            Selector("Cast AoE Ability", {
                                // Butchery (talent)

                                Sequence("Cast Butchery", {

                                    Condition("Has Butchery", [this](Player* bot, Unit*) {

                                        return bot && bot->HasSpell(SPELL_BUTCHERY);

                                    }),

                                    bot::ai::Action("Cast Butchery", [this](Player* bot, Unit* target) -> NodeStatus {

                                        if (this->_resource >= 30)

                                        {

                                            this->CastSpell(SPELL_BUTCHERY, bot);

                                            this->ConsumeResource(30);

                                            return NodeStatus::SUCCESS;

                                        }

                                        return NodeStatus::FAILURE;

                                    })

                                }),
                                // Carve (baseline)

                                Sequence("Cast Carve", {

                                    Condition("35+ Focus", [this](Player* bot, Unit*) {

                                        return this->_resource >= 35;

                                    }),

                                    bot::ai::Action("Cast Carve", [this](Player* bot, Unit* target) -> NodeStatus {

                                        if (this->_resource >= 35)

                                        {

                                            this->CastSpell(SPELL_CARVE, bot);

                                            this->ConsumeResource(35);

                                            return NodeStatus::SUCCESS;

                                        }

                                        return NodeStatus::FAILURE;

                                    })

                                })

                            })

                        }),
                        // Single target filler

                        Sequence("Single Target Filler", {

                            bot::ai::Action("Cast Raptor Strike", [this](Player* bot, Unit* target) -> NodeStatus {

                                if (this->_resource >= 30)

                                {

                                    this->CastSpell(SPELL_RAPTOR_STRIKE, target);

                                    this->_lastRaptorStrike = GameTime::GetGameTimeMS();

                                    this->ConsumeResource(30);

                                    return NodeStatus::SUCCESS;

                                }

                                return NodeStatus::FAILURE;

                            })

                        })

                    })

                })

            });


            behaviorTree->SetRoot(root);

            TC_LOG_INFO("module.playerbot", " SURVIVAL HUNTER: BehaviorTree initialized with 4-tier melee DPS rotation");
        }
    }

private:
    WildfireBombManager _bombManager;
    MongooseBiteTracker _mongooseTracker;
    SurvivalPetManager _petManager;

    // Burst window tracking
    bool _coordinatedAssaultActive;
    uint32 _coordinatedAssaultEndTime;
    bool _aspectOfEagleActive;

    // Ability timing
    uint32 _lastRaptorStrike;
    uint32 _lastKillCommand;
    uint32 _lastSerpentSting;

    // Talent tracking
    bool _guerillaTacticsActive;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot