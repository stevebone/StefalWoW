/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Outlaw Rogue Refactored - Template-Based Implementation
 *
 * This file provides a complete, template-based implementation of Outlaw Rogue
 * using the MeleeDpsSpecialization with dual resource system (Energy + Combo Points).
 */

#pragma once


#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "../CombatSpecializationTemplates.h"
#include "RogueResourceTypes.h"  // Shared EnergyComboResource definition
#include "Player.h"
#include "SpellMgr.h"
#include "SpellAuraEffects.h"
#include "Log.h"

// Phase 5 Integration: Decision Systems
#include "../../Decision/ActionPriorityQueue.h"
#include "../../Decision/BehaviorTree.h"
#include "../BotAI.h"

// Central Spell Registry - See WoW120Spells::Rogue namespace
#include "../SpellValidation_WoW120.h"
#include "../SpellValidation_WoW120_Part2.h"
#include "../HeroTalentDetector.h"      // Hero talent tree detection

namespace Playerbot
{


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
// NOTE: Common Rogue spell IDs (BLADE_FLURRY, ADRENALINE_RUSH, KILLING_SPREE, etc.) are in RogueSpecialization.h
// Only Outlaw-UNIQUE spell IDs defined below to avoid duplicate definition errors
// NOTE: ComboPointsOutlaw is defined in RogueResourceTypes.h (spec-specific resource type)

// ============================================================================
// OUTLAW ROGUE SPELL IDs (WoW 12.0 - The War Within) - UNIQUE ONLY
// See central registry: WoW120Spells::Rogue and WoW120Spells::Rogue::Outlaw
// ============================================================================

enum OutlawSpells
{
    // WoW 12.0 Outlaw-specific spells
    // Using central registry: WoW120Spells::Rogue and WoW120Spells::Rogue::Outlaw
    PISTOL_SHOT              = WoW120Spells::Rogue::Outlaw::PISTOL_SHOT,
    BETWEEN_THE_EYES         = WoW120Spells::Rogue::Outlaw::BETWEEN_THE_EYES,
    DISPATCH_OUTLAW          = WoW120Spells::Rogue::Outlaw::DISPATCH,

    // Roll the Bones System (Outlaw unique)
    ROLL_THE_BONES           = WoW120Spells::Rogue::Outlaw::ROLL_THE_BONES,
    BUFF_RUTHLESS_PRECISION  = WoW120Spells::Rogue::Outlaw::RUTHLESS_PRECISION,
    BUFF_GRAND_MELEE         = WoW120Spells::Rogue::Outlaw::GRAND_MELEE,
    BUFF_BROADSIDE           = WoW120Spells::Rogue::Outlaw::BROADSIDE,
    BUFF_TRUE_BEARING        = WoW120Spells::Rogue::Outlaw::TRUE_BEARING,
    BUFF_SKULL_AND_CROSSBONES = WoW120Spells::Rogue::Outlaw::SKULL_AND_CROSSBONES,
    BUFF_BURIED_TREASURE     = WoW120Spells::Rogue::Outlaw::BURIED_TREASURE,

    // Talents (Outlaw specific)
    BLADE_RUSH               = WoW120Spells::Rogue::Outlaw::BLADE_RUSH,
    OPPORTUNITY_PROC         = WoW120Spells::Rogue::Outlaw::OPPORTUNITY_PROC,
    GHOSTLY_STRIKE           = WoW120Spells::Rogue::Outlaw::GHOSTLY_STRIKE,
    DREADBLADES              = WoW120Spells::Rogue::Outlaw::DREADBLADES,

    // Outlaw-specific utility
    FEINT_OUTLAW             = WoW120Spells::Rogue::FEINT,
    MARKED_FOR_DEATH         = WoW120Spells::Rogue::MARKED_FOR_DEATH
};

// ============================================================================
// ROLL THE BONES TRACKER
// ============================================================================

class RollTheBonesTracker
{
public:
    struct Buff
    {
        uint32 spellId;
        bool active;
        uint32 endTime;

        Buff(uint32 id) : spellId(id), active(false), endTime(0) {}

        bool IsActive() const { return active && GameTime::GetGameTimeMS() < endTime; }
        uint32 GetTimeRemaining() const
        {
            if (!active) return 0;
            uint32 now = GameTime::GetGameTimeMS();
            return endTime > now ? endTime - now : 0;
        }
    };

    RollTheBonesTracker()
    {
        this->_buffs.push_back(Buff(BUFF_RUTHLESS_PRECISION));
        this->_buffs.push_back(Buff(BUFF_GRAND_MELEE));
        this->_buffs.push_back(Buff(BUFF_BROADSIDE));
        this->_buffs.push_back(Buff(BUFF_TRUE_BEARING));
        this->_buffs.push_back(Buff(BUFF_SKULL_AND_CROSSBONES));
        this->_buffs.push_back(Buff(BUFF_BURIED_TREASURE));
    }

    void RollBuffs(Player* bot)
    {
        // Clear old buffs
        for (auto& buff : _buffs)
        {
            buff.active = false;
            buff.endTime = 0;
        }

        // Simulate rolling 1-2 random buffs
        uint32 buffCount = (rand() % 6 == 0) ? 2 : 1; // ~17% chance for 2 buffs

        for (uint32 i = 0; i < buffCount; ++i)
        {
            uint32 randomIndex = rand() % this->_buffs.size();
            _buffs[randomIndex].active = true;
            _buffs[randomIndex].endTime = GameTime::GetGameTimeMS() + 30000; // 30 sec duration
        }
    }

    uint32 GetActiveBuffCount() const
    {
        uint32 count = 0;
        for (const auto& buff : _buffs)
        {
            if (buff.IsActive())
                count++;
        }
        return count;
    }

    bool HasAnyBuff() const
    {
        return GetActiveBuffCount() > 0;
    }

    bool HasGoodBuffs() const
    {
        // Good buffs: 2+ buffs, or 1 True Bearing/Broadside
        uint32 count = GetActiveBuffCount();
        if (count >= 2)
            return true;

        for (const auto& buff : _buffs)
        {
            if (buff.IsActive() &&
                (buff.spellId == BUFF_TRUE_BEARING || buff.spellId == BUFF_BROADSIDE))
                return true;
        }

        return false;
    }

    uint32 GetLowestBuffDuration() const
    {
        uint32 lowest = 999999;
        for (const auto& buff : _buffs)
        {
            if (buff.IsActive())
            {
                uint32 remaining = buff.GetTimeRemaining();
                if (remaining < lowest)
                    lowest = remaining;
            }
        }
        return lowest == 999999 ? 0 : lowest;
    }

    bool NeedsReroll() const
    {
        // Reroll if no buffs or low duration
        if (!HasAnyBuff())
            return true;

        if (GetLowestBuffDuration() < 3000) // Less than 3 seconds
            return true;

        // Reroll if only 1 bad buff
        if (GetActiveBuffCount() == 1 && !HasGoodBuffs())
            return true;

        return false;
    }

    void Update()
    {
        uint32 now = GameTime::GetGameTimeMS();
        for (auto& buff : _buffs)
        {
            if (buff.active && now >= buff.endTime)
            {
                buff.active = false;
                buff.endTime = 0;
            }
        }
    }

private:
    CooldownManager _cooldowns;
    ::std::vector<Buff> _buffs;
};

// ============================================================================
// OUTLAW ROGUE REFACTORED
// ============================================================================

class OutlawRogueRefactored : public MeleeDpsSpecialization<ComboPointsOutlaw>
{
public:
    // Use base class members with type alias for cleaner syntax
    using Base = MeleeDpsSpecialization<ComboPointsOutlaw>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::GetEnemiesInRange;
    using Base::_resource;

    explicit OutlawRogueRefactored(Player* bot)
        : MeleeDpsSpecialization<ComboPointsOutlaw>(bot)
        , _rollTheBonesTracker()
        , _bladeFlurryActive(false)
        , _bladeFlurryEndTime(0)
        , _adrenalineRushActive(false)
        , _adrenalineRushEndTime(0)
        , _inStealth(false)
        , _lastSinisterStrikeTime(0)
        , _lastDispatchTime(0)
        , _spellsInitialized(false)
    {
        // CRITICAL: Do NOT call bot->HasSpell() or bot->GetName() in constructor!
        // Bot's spell data and internal fields are NOT initialized during constructor chain.
        // Use default values here, real values initialized in first UpdateRotation() when bot IsInWorld().
        this->_resource.maxEnergy = 100;
        this->_resource.maxComboPoints = 5;  // Default, updated when spells loaded
        this->_resource.energy = this->_resource.maxEnergy;
        this->_resource.comboPoints = 0;

        // Phase 5 Integration: Initialize decision systems
        InitializeOutlawMechanics();

        // Logging deferred to first Update when bot IsInWorld()
    }

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !target->IsAlive() || !target->IsHostileTo(this->GetBot()))
            return;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero talent rotation branches
        if (_heroTalents.IsTree(HeroTalentTree::TRICKSTER))
        {
            // Trickster: Unseen Blade for surprise burst damage
            if (this->CanCastSpell(WoW120Spells::Rogue::Outlaw::UNSEEN_BLADE, target))
            {
                this->CastSpell(WoW120Spells::Rogue::Outlaw::UNSEEN_BLADE, target);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::FATEBOUND))
        {
            // Fatebound: Fate Intertwined - amplify finishers at max combo points
            if (this->_resource.comboPoints >= 5 &&
                this->CanCastSpell(WoW120Spells::Rogue::Outlaw::OUTLAW_FATE_INTERTWINED, target))
            {
                this->CastSpell(WoW120Spells::Rogue::Outlaw::OUTLAW_FATE_INTERTWINED, target);
                return;
            }
        }

        // CRITICAL: Deferred spell initialization - bot's spell data must be loaded
        if (!_spellsInitialized && this->GetBot() && this->GetBot()->IsInWorld())
        {
            this->_resource.maxComboPoints = this->GetBot()->HasSpell(193531) ? 6 : 5; // Deeper Stratagem
            _spellsInitialized = true;
        }

        // Update tracking systems
        UpdateOutlawState();

        // Check stealth status
        _inStealth = this->GetBot()->HasAuraType(SPELL_AURA_MOD_STEALTH);

        // Stealth opener
        if (_inStealth)
        {
            ExecuteStealthOpener(target);
            return;
        }

        // Main rotation
        uint32 enemyCount = this->GetEnemiesInRange(8.0f);
        if (enemyCount >= 2)
        {
            ExecuteAoERotation(target, enemyCount);
        }
        else
        {
            ExecuteSingleTargetRotation(target);
        }
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();        // Enter stealth out of combat
        if (!bot->IsInCombat() && !_inStealth && this->CanCastSpell(RogueAI::STEALTH, bot))
        {
            this->CastSpell(RogueAI::STEALTH, bot);
        }

        // Defensive cooldowns
        if (bot->GetHealthPct() < 30.0f && this->CanCastSpell(RogueAI::CLOAK_OF_SHADOWS, bot))
        {
            this->CastSpell(RogueAI::CLOAK_OF_SHADOWS, bot);
        }

        if (bot->GetHealthPct() < 50.0f && this->CanCastSpell(FEINT_OUTLAW, bot))
        {
            this->CastSpell(FEINT_OUTLAW, bot);
        }
    }

    // GetOptimalRange is final in MeleeDpsSpecialization, cannot override
    // Returns 5.0f melee range by default

protected:
    void ExecuteSingleTargetRotation(::Unit* target)
    {
        uint32 energy = this->_resource.energy;
        uint32 cp = this->_resource.comboPoints;
        uint32 maxCp = this->_resource.maxComboPoints;

        // Priority 1: Adrenaline Rush on cooldown
        if (this->CanCastSpell(RogueAI::ADRENALINE_RUSH, this->GetBot()))
        {
            this->CastSpell(RogueAI::ADRENALINE_RUSH, this->GetBot());
            _adrenalineRushActive = true;
            _adrenalineRushEndTime = GameTime::GetGameTimeMS() + 20000;
            return;
        }

        // Priority 2: Roll the Bones if needed
        if (_rollTheBonesTracker.NeedsReroll() && cp >= 1 && energy >= 25)
        {
            if (this->CanCastSpell(ROLL_THE_BONES, this->GetBot()))
            {
                this->CastSpell(ROLL_THE_BONES, this->GetBot());
                _rollTheBonesTracker.RollBuffs(this->GetBot());
                ConsumeEnergy(25);
                this->_resource.comboPoints = 0;
                return;
            }
        }

        // Priority 3: Between the Eyes at 5-6 CP
        if (cp >= maxCp && energy >= 25)
        {
            if (this->CanCastSpell(BETWEEN_THE_EYES, target))
            {
                this->CastSpell(BETWEEN_THE_EYES, target);
                ConsumeEnergy(25);
                this->_resource.comboPoints = 0;
                return;
            }
        }

        // Priority 4: Dispatch at 5-6 CP
        if (cp >= (maxCp - 1) && energy >= 35)
        {
            if (this->CanCastSpell(DISPATCH_OUTLAW, target))
            {
                this->CastSpell(DISPATCH_OUTLAW, target);
                _lastDispatchTime = GameTime::GetGameTimeMS();
                ConsumeEnergy(35);
                this->_resource.comboPoints = 0;
                return;
            }
        }

        // Priority 5: Opportunity proc - free Pistol Shot
        if (this->GetBot()->HasAura(OPPORTUNITY_PROC))
        {
            if (this->CanCastSpell(PISTOL_SHOT, target))
            {
                this->CastSpell(PISTOL_SHOT, target);
                GenerateComboPoints(1);
                // No energy cost with proc
                return;
            }
        }

        // Priority 6: Blade Rush (talent)
        if (energy >= 25 && this->CanCastSpell(BLADE_RUSH, target))
        {
            this->CastSpell(BLADE_RUSH, target);
            ConsumeEnergy(25);
            GenerateComboPoints(1);
            return;
        }

        // Priority 7: Sinister Strike for combo points
        if (energy >= 45 && cp < maxCp)
        {
            if (this->CanCastSpell(RogueAI::SINISTER_STRIKE, target))
            {
                this->CastSpell(RogueAI::SINISTER_STRIKE, target);
                _lastSinisterStrikeTime = GameTime::GetGameTimeMS();
                ConsumeEnergy(45);
                GenerateComboPoints(1);
                // Broadside buff gives extra CP
                if (_rollTheBonesTracker.HasGoodBuffs())
                    GenerateComboPoints(1);
                return;
            }
        }

        // Priority 8: Pistol Shot if can't melee
        if (this->GetBot()->GetExactDist(target) > 10.0f && energy >= 40)
        {
            if (this->CanCastSpell(PISTOL_SHOT, target))
            {
                this->CastSpell(PISTOL_SHOT, target);
                ConsumeEnergy(40);
                GenerateComboPoints(1);
                return;
            }
        }
    }

    void ExecuteAoERotation(::Unit* target, uint32 enemyCount)
    {
        uint32 energy = this->_resource.energy;
        uint32 cp = this->_resource.comboPoints;
        uint32 maxCp = this->_resource.maxComboPoints;

        // Priority 1: Enable Blade Flurry for cleave
        if (!_bladeFlurryActive && energy >= 15)
        {
            if (this->CanCastSpell(RogueAI::BLADE_FLURRY, this->GetBot()))
            {
                this->CastSpell(RogueAI::BLADE_FLURRY, this->GetBot());
                _bladeFlurryActive = true;
                _bladeFlurryEndTime = GameTime::GetGameTimeMS() + 12000;
                ConsumeEnergy(15);
                return;
            }
        }

        // Priority 2: Adrenaline Rush
        if (this->CanCastSpell(RogueAI::ADRENALINE_RUSH, this->GetBot()))
        {
            this->CastSpell(RogueAI::ADRENALINE_RUSH, this->GetBot());
            _adrenalineRushActive = true;
            _adrenalineRushEndTime = GameTime::GetGameTimeMS() + 20000;
            return;
        }

        // Priority 3: Roll the Bones
        if (_rollTheBonesTracker.NeedsReroll() && cp >= 1 && energy >= 25)
        {
            if (this->CanCastSpell(ROLL_THE_BONES, this->GetBot()))
            {
                this->CastSpell(ROLL_THE_BONES, this->GetBot());
                _rollTheBonesTracker.RollBuffs(this->GetBot());
                ConsumeEnergy(25);
                this->_resource.comboPoints = 0;
                return;
            }
        }

        // Priority 4: Between the Eyes at 5+ CP
        if (cp >= 5 && energy >= 25)
        {
            if (this->CanCastSpell(BETWEEN_THE_EYES, target))
            {
                this->CastSpell(BETWEEN_THE_EYES, target);
                ConsumeEnergy(25);
                this->_resource.comboPoints = 0;
                return;
            }
        }

        // Use single target rotation with Blade Flurry active
        ExecuteSingleTargetRotation(target);
    }

    void ExecuteStealthOpener(::Unit* target)
    {
        // Ambush from stealth for high damage
        if (this->CanCastSpell(RogueAI::AMBUSH, target))
        {
            this->CastSpell(RogueAI::AMBUSH, target);
            GenerateComboPoints(2);
            _inStealth = false;
            return;
        }

        // Cheap Shot for control
        if (this->CanCastSpell(RogueAI::CHEAP_SHOT, target))
        {
            this->CastSpell(RogueAI::CHEAP_SHOT, target);
            GenerateComboPoints(2);
            _inStealth = false;
            return;
        }
    }

private:
    void UpdateOutlawState()
    {
        uint32 now = GameTime::GetGameTimeMS();

        // Update Roll the Bones buffs
        _rollTheBonesTracker.Update();

        // Check Blade Flurry expiry
        if (_bladeFlurryActive && now >= _bladeFlurryEndTime)
        {
            _bladeFlurryActive = false;
            _bladeFlurryEndTime = 0;
        }

        // Check Adrenaline Rush expiry
        if (_adrenalineRushActive && now >= _adrenalineRushEndTime)
        {
            _adrenalineRushActive = false;
            _adrenalineRushEndTime = 0;
        }

        // Regenerate energy (10 per second, 25 during Adrenaline Rush)
        static uint32 lastRegenTime = now;
        uint32 timeDiff = now - lastRegenTime;

        if (timeDiff >= 100) // Every 100ms
        {
            uint32 regenRate = _adrenalineRushActive ? 25 : 10;
            uint32 energyRegen = (timeDiff / 100) * (regenRate / 10);
            this->_resource.energy = ::std::min(this->_resource.energy + energyRegen, this->_resource.maxEnergy);
            lastRegenTime = now;
        }
    }

    void ConsumeEnergy(uint32 amount)
    {
        this->_resource.energy = (this->_resource.energy > amount) ? this->_resource.energy - amount : 0;
    }

    void GenerateComboPoints(uint32 amount)
    {
        this->_resource.comboPoints = ::std::min(this->_resource.comboPoints + amount, this->_resource.maxComboPoints);
    }

    // Phase 5 Integration: Decision Systems Initialization
    void InitializeOutlawMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;

        // ========================================================================
        // ActionPriorityQueue: Register Outlaw Rogue spells with priorities
        // ========================================================================
        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Defensive cooldowns
            queue->RegisterSpell(RogueAI::CLOAK_OF_SHADOWS, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(RogueAI::CLOAK_OF_SHADOWS,
                ::std::function<bool(Player*, Unit*)>{[this](Player* bot, Unit*)
                {
                return bot && bot->GetHealthPct() < 30.0f;
            }},
                "Bot HP < 30% (spell immunity)");

            queue->RegisterSpell(FEINT_OUTLAW, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(FEINT_OUTLAW,
                ::std::function<bool(Player*, Unit*)>{[this](Player* bot, Unit*)
                {
                return bot && bot->GetHealthPct() < 50.0f;
            }},
                "Bot HP < 50% (threat reduction + damage reduction)");

            // CRITICAL: Burst cooldowns and Roll the Bones
            queue->RegisterSpell(RogueAI::ADRENALINE_RUSH, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(RogueAI::ADRENALINE_RUSH,
                ::std::function<bool(Player*, Unit*)>{[this](Player* bot, Unit* target)
                {
                return target && !this->_adrenalineRushActive;
            }},
                "Not active (20s burst, 2.5x energy regen)");

            queue->RegisterSpell(ROLL_THE_BONES, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(ROLL_THE_BONES,
                ::std::function<bool(Player*, Unit*)>{[this](Player* bot, Unit* target)
                {
                return target && this->_resource.energy >= 25 &&
                       this->_resource.comboPoints >= 1 &&
                       this->_rollTheBonesTracker.NeedsReroll();
            }},
                "25+ Energy, 1+ CP, needs reroll (random buffs)");

            // HIGH: Finishers at 5-6 CP
            queue->RegisterSpell(BETWEEN_THE_EYES, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(BETWEEN_THE_EYES,
                ::std::function<bool(Player*, Unit*)>{[this](Player* bot, Unit* target)
                {
                return target && this->_resource.energy >= 25 &&
                       this->_resource.comboPoints >= this->_resource.maxComboPoints;
            }},
                "25+ Energy, max CP (finisher with stun)");

            queue->RegisterSpell(DISPATCH_OUTLAW, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(DISPATCH_OUTLAW,
                ::std::function<bool(Player*, Unit*)>{[this](Player* bot, Unit* target)
                {
                return target && this->_resource.energy >= 35 &&
                       this->_resource.comboPoints >= (this->_resource.maxComboPoints - 1);
            }},
                "35+ Energy, 4-5+ CP (finisher damage)");

            // MEDIUM: Combo builders and AoE
            queue->RegisterSpell(RogueAI::BLADE_FLURRY, SpellPriority::MEDIUM, SpellCategory::OFFENSIVE);
            queue->AddCondition(RogueAI::BLADE_FLURRY,
                ::std::function<bool(Player*, Unit*)>{[this](Player* bot, Unit* target)
                {
                return target && this->_resource.energy >= 15 &&
                       !this->_bladeFlurryActive &&
                       this->GetEnemiesInRange(8.0f) >= 2;
            }},
                "15+ Energy, not active, 2+ enemies (12s cleave)");

            queue->RegisterSpell(BLADE_RUSH, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(BLADE_RUSH,
                ::std::function<bool(Player*, Unit*)>{[this](Player* bot, Unit* target)
                {
                return bot && bot->HasSpell(BLADE_RUSH) &&
                       target && this->_resource.energy >= 25;
            }},
                "Has talent, 25+ Energy (charge + AoE + 1 CP)");

            queue->RegisterSpell(PISTOL_SHOT, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(PISTOL_SHOT,
                ::std::function<bool(Player*, Unit*)>{[this](Player* bot, Unit* target)
                {
                return target && bot->HasAura(OPPORTUNITY_PROC);
            }},
                "Opportunity proc (free Pistol Shot, 1 CP)");

            queue->RegisterSpell(RogueAI::SINISTER_STRIKE, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(RogueAI::SINISTER_STRIKE,
                ::std::function<bool(Player*, Unit*)>{[this](Player* bot, Unit* target)
                {
                return target && this->_resource.energy >= 45 &&
                       this->_resource.comboPoints < this->_resource.maxComboPoints;
            }},
                "45+ Energy, not max CP (generates 1-2 CP)");

            queue->RegisterSpell(RogueAI::KICK, SpellPriority::MEDIUM, SpellCategory::UTILITY);
            queue->AddCondition(RogueAI::KICK,
                ::std::function<bool(Player*, Unit*)>{[this](Player* bot, Unit* target)
                {
                return target && target->IsNonMeleeSpellCast(false);
            }},
                "Target casting (interrupt)");

            // LOW: Ranged filler
            queue->RegisterSpell(PISTOL_SHOT, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(PISTOL_SHOT,
                ::std::function<bool(Player*, Unit*)>{[this](Player* bot, Unit* target)
                {
                return target && this->_resource.energy >= 40 &&
                       !bot->HasAura(OPPORTUNITY_PROC) &&
                       bot->GetExactDist(target) > 10.0f;
            }},
                "40+ Energy, > 10 yards, no proc (ranged builder)");

            TC_LOG_INFO("module.playerbot", " OUTLAW ROGUE: Registered {} spells in ActionPriorityQueue", queue->GetSpellCount());
        }

        // ========================================================================
        // BehaviorTree: Outlaw Rogue DPS rotation logic
        // ========================================================================
        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Outlaw Rogue DPS", {
                // Tier 1: Burst Cooldowns (Adrenaline Rush)
                Sequence("Burst Cooldowns", {
                    Condition("Target exists", [this](Player* bot, Unit* target) {
                        return target != nullptr;
                    }),
                    Selector("Use Burst", {
                        Sequence("Cast Adrenaline Rush", {
                            Condition("Not active", [this](Player* bot, Unit*) {
                                return !this->_adrenalineRushActive;
                            }),
                            bot::ai::Action("Cast Adrenaline Rush", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(RogueAI::ADRENALINE_RUSH, bot))
                                {
                                    this->CastSpell(RogueAI::ADRENALINE_RUSH, bot);
                                    this->_adrenalineRushActive = true;
                                    this->_adrenalineRushEndTime = GameTime::GetGameTimeMS() + 20000;
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 2: Roll the Bones (maintain buffs)
                Sequence("Roll the Bones", {
                    Condition("Target exists and needs reroll", [this](Player* bot, Unit* target) {
                        return target != nullptr && this->_resource.energy >= 25 &&
                               this->_resource.comboPoints >= 1 &&
                               this->_rollTheBonesTracker.NeedsReroll();
                    }),
                    bot::ai::Action("Cast Roll the Bones", [this](Player* bot, Unit* target) -> NodeStatus {
                        if (this->CanCastSpell(ROLL_THE_BONES, bot))
                        {
                            this->CastSpell(ROLL_THE_BONES, bot);
                            this->_rollTheBonesTracker.RollBuffs(bot);
                            this->ConsumeEnergy(25);
                            this->_resource.comboPoints = 0;
                            return NodeStatus::SUCCESS;
                        }
                        return NodeStatus::FAILURE;
                    })
                }),

                // Tier 3: Finishers (Between the Eyes, Dispatch at 5-6 CP)
                Sequence("Finishers", {
                    Condition("Target exists and has CP", [this](Player* bot, Unit* target) {
                        return target != nullptr &&
                               this->_resource.comboPoints >= (this->_resource.maxComboPoints - 1);
                    }),
                    Selector("Choose Finisher", {
                        // Between the Eyes at max CP
                        Sequence("Cast Between the Eyes", {
                            Condition("Max CP and 25+ Energy", [this](Player* bot, Unit*) {
                                return this->_resource.comboPoints >= this->_resource.maxComboPoints &&
                                       this->_resource.energy >= 25;
                            }),
                            bot::ai::Action("Cast Between the Eyes", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(BETWEEN_THE_EYES, target))
                                {
                                    this->CastSpell(BETWEEN_THE_EYES, target);
                                    this->ConsumeEnergy(25);
                                    this->_resource.comboPoints = 0;
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        // Dispatch at 4-5+ CP
                        Sequence("Cast Dispatch", {
                            Condition("35+ Energy", [this](Player* bot, Unit*) {
                                return this->_resource.energy >= 35;
                            }),
                            bot::ai::Action("Cast Dispatch", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(DISPATCH_OUTLAW, target))
                                {
                                    this->CastSpell(DISPATCH_OUTLAW, target);
                                    this->_lastDispatchTime = GameTime::GetGameTimeMS();
                                    this->ConsumeEnergy(35);
                                    this->_resource.comboPoints = 0;
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 4: Combo Builders (Opportunity proc, Blade Rush, Sinister Strike)
                Sequence("Combo Builders", {
                    Condition("Target exists", [this](Player* bot, Unit* target) {
                        return target != nullptr &&
                               this->_resource.comboPoints < this->_resource.maxComboPoints;
                    }),
                    Selector("Build Combo Points", {
                        // Opportunity proc (free Pistol Shot)
                        Sequence("Cast Pistol Shot with proc", {
                            Condition("Has Opportunity proc", [this](Player* bot, Unit*) {
                                return bot && bot->HasAura(OPPORTUNITY_PROC);
                            }),
                            bot::ai::Action("Cast Pistol Shot", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(PISTOL_SHOT, target))
                                {
                                    this->CastSpell(PISTOL_SHOT, target);
                                    this->GenerateComboPoints(1);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        // Blade Rush (talent)
                        Sequence("Cast Blade Rush", {
                            Condition("Has talent and 25+ Energy", [this](Player* bot, Unit*) {
                                return bot && bot->HasSpell(BLADE_RUSH) &&
                                       this->_resource.energy >= 25;
                            }),
                            bot::ai::Action("Cast Blade Rush", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(BLADE_RUSH, target))
                                {
                                    this->CastSpell(BLADE_RUSH, target);
                                    this->ConsumeEnergy(25);
                                    this->GenerateComboPoints(1);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        // Sinister Strike
                        Sequence("Cast Sinister Strike", {
                            Condition("45+ Energy", [this](Player* bot, Unit*) {
                                return this->_resource.energy >= 45;
                            }),
                            bot::ai::Action("Cast Sinister Strike", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(RogueAI::SINISTER_STRIKE, target))
                                {
                                    this->CastSpell(RogueAI::SINISTER_STRIKE, target);
                                    this->_lastSinisterStrikeTime = GameTime::GetGameTimeMS();
                                    this->ConsumeEnergy(45);
                                    this->GenerateComboPoints(1);
                                    // Broadside buff gives extra CP
                                    if (this->_rollTheBonesTracker.HasGoodBuffs())
                                        this->GenerateComboPoints(1);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                })
            });

            behaviorTree->SetRoot(root);
            TC_LOG_INFO("module.playerbot", " OUTLAW ROGUE: BehaviorTree initialized with 4-tier DPS rotation");
        }
    }

private:
    RollTheBonesTracker _rollTheBonesTracker;
    bool _bladeFlurryActive;
    uint32 _bladeFlurryEndTime;
    bool _adrenalineRushActive;
    uint32 _adrenalineRushEndTime;
    bool _inStealth;
    uint32 _lastSinisterStrikeTime;
    uint32 _lastDispatchTime;
    bool _spellsInitialized;  // Deferred initialization flag

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
