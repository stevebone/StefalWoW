/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Subtlety Rogue Refactored - Template-Based Implementation
 *
 * This file provides a complete, template-based implementation of Subtlety Rogue
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
// NOTE: Common Rogue spell IDs are defined in RogueSpecialization.h
// NOTE: Shared spells (SHADOW_DANCE, SYMBOLS_OF_DEATH, EVASION, SAP, etc.) are in RogueSpecialization.h
// Only Subtlety-unique spell IDs defined below to avoid duplicate definition errors
// NOTE: ComboPointsSubtlety is defined in RogueResourceTypes.h (spec-specific resource type)

// ============================================================================
// SUBTLETY ROGUE SPELL IDs (WoW 12.0 - The War Within)
// See central registry: WoW120Spells::Rogue and WoW120Spells::Rogue::Subtlety
// ============================================================================

enum SubtletySpells
{
    // NOTE: Shared spells (BACKSTAB, RUPTURE, STEALTH, VANISH, KICK, etc.) are in RogueSpecialization.h
    // Only Subtlety-UNIQUE spells defined here to avoid duplicate definitions
    // Using central registry: WoW120Spells::Rogue and WoW120Spells::Rogue::Subtlety

    // Combo Point Builders (Unique to Subtlety)
    SHADOWSTRIKE_SUB         = WoW120Spells::Rogue::Subtlety::SHADOWSTRIKE,
    SHURIKEN_STORM           = WoW120Spells::Rogue::Subtlety::SHURIKEN_STORM,

    // Combo Point Spenders (Unique to Subtlety)
    EVISCERATE_SUB           = WoW120Spells::Rogue::Subtlety::EVISCERATE,
    BLACK_POWDER             = WoW120Spells::Rogue::Subtlety::BLACK_POWDER,
    SECRET_TECHNIQUE         = WoW120Spells::Rogue::Subtlety::SECRET_TECHNIQUE,

    // Major Cooldowns
    SHADOW_BLADES            = WoW120Spells::Rogue::Subtlety::SHADOW_BLADES,
    SHURIKEN_TORNADO         = WoW120Spells::Rogue::Subtlety::SHURIKEN_TORNADO,

    // Shadow Techniques
    SHADOW_TECHNIQUES_PROC   = WoW120Spells::Rogue::Subtlety::SHADOW_TECHNIQUES,

    // Finisher Buffs
    SLICE_AND_DICE_SUB       = WoW120Spells::Rogue::Outlaw::SLICE_AND_DICE,

    // Procs and Buffs
    DANSE_MACABRE            = WoW120Spells::Rogue::Subtlety::DANSE_MACABRE,
    DEEPER_DAGGERS           = WoW120Spells::Rogue::Subtlety::DEEPER_DAGGERS,

    // Talents
    DARK_SHADOW              = WoW120Spells::Rogue::Subtlety::DARK_SHADOW,
    DEEPER_STRATAGEM_SUB     = WoW120Spells::Rogue::Subtlety::DEEPER_STRATAGEM,
    MARKED_FOR_DEATH_SUB     = WoW120Spells::Rogue::MARKED_FOR_DEATH,
    SHURIKEN_TORNADO_TALENT  = WoW120Spells::Rogue::Subtlety::SHURIKEN_TORNADO
};

// ============================================================================
// SHADOW DANCE TRACKER
// ============================================================================

class ShadowDanceTracker
{
public:
    ShadowDanceTracker()
        : _charges(3)
        , _maxCharges(3)
        , _active(false)
        , _endTime(0)
        , _lastUseTime(0)
        , _lastRechargeTime(0)
        , _chargeCooldown(60000) // 60 sec per charge
        , _duration(8000)        // 8 sec duration
    {
    }

    void Update()
    {
        uint32 now = GameTime::GetGameTimeMS();

        // Check if Shadow Dance expired
        if (_active && now >= _endTime)
        {
            _active = false;
            _endTime = 0;
        }

        // Recharge charges
        if (_charges < _maxCharges)
        {
            uint32 timeSinceRecharge = now - _lastRechargeTime;
            if (timeSinceRecharge >= _chargeCooldown)
            {
                _charges++;
                _lastRechargeTime = now;
            }
        }
    }

    bool CanUse() const
    {
        return _charges > 0 && !_active;
    }

    bool IsActive() const { return _active; }
    uint32 GetCharges() const { return _charges; }

    uint32 GetTimeRemaining() const
    {
        if (!_active)
            return 0;

        uint32 now = GameTime::GetGameTimeMS();
        return _endTime > now ? _endTime - now : 0;
    }

    void Use()
    {
        if (_charges > 0)
        {
            _charges--;
            _lastUseTime = GameTime::GetGameTimeMS();
            _active = true;
            _endTime = _lastUseTime + _duration;

            if (_charges == (_maxCharges - 1))
                _lastRechargeTime = _lastUseTime;
        }
    }

    bool ShouldUse(uint32 comboPoints) const
    {
        // Use Shadow Dance when:
        // 1. Have charges available
        // 2. Low combo points to build quickly
        // 3. Not already active

        if (!CanUse())
            return false;

        // Use to build combo points quickly
        if (comboPoints < 3)
            return true;

        // Use all charges eventually
        if (_charges == _maxCharges)
            return true;

        return false;
    }

private:
    CooldownManager _cooldowns;
    uint32 _charges;
    const uint32 _maxCharges;
    bool _active;
    uint32 _endTime;
    uint32 _lastUseTime;
    uint32 _lastRechargeTime;
    const uint32 _chargeCooldown;
    const uint32 _duration;
};

// ============================================================================
// SUBTLETY ROGUE REFACTORED
// ============================================================================

class SubtletyRogueRefactored : public MeleeDpsSpecialization<ComboPointsSubtlety>
{
public:
    // Use base class members with type alias for cleaner syntax
    using Base = MeleeDpsSpecialization<ComboPointsSubtlety>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::GetEnemiesInRange;
    using Base::IsBehindTarget;
    using Base::_resource;

    explicit SubtletyRogueRefactored(Player* bot)
        : MeleeDpsSpecialization<ComboPointsSubtlety>(bot)
        , _shadowDanceTracker()
        , _inStealth(false)
        , _symbolsOfDeathActive(false)
        , _symbolsOfDeathEndTime(0)
        , _shadowBladesActive(false)
        , _shadowBladesEndTime(0)
        , _lastBackstabTime(0)
        , _lastShadowstrikeTime(0)
        , _lastEviscerateTime(0)
        , _shadowTechniquesActive(false)
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
        InitializeSubtletyMechanics();

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
        if (_heroTalents.IsTree(HeroTalentTree::DEATHSTALKER))
        {
            // Deathstalker: Apply Deathstalker's Mark for amplified finishers
            if (this->CanCastSpell(WoW120Spells::Rogue::Subtlety::SUB_DEATHSTALKERS_MARK, target))
            {
                this->CastSpell(WoW120Spells::Rogue::Subtlety::SUB_DEATHSTALKERS_MARK, target);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::TRICKSTER))
        {
            // Trickster: Unseen Blade for burst from stealth/Shadow Dance
            if (this->CanCastSpell(WoW120Spells::Rogue::Subtlety::SUB_UNSEEN_BLADE, target))
            {
                this->CastSpell(WoW120Spells::Rogue::Subtlety::SUB_UNSEEN_BLADE, target);
                return;
            }
        }

        // CRITICAL: Deferred spell initialization - bot's spell data must be loaded
        if (!_spellsInitialized && this->GetBot() && this->GetBot()->IsInWorld())
        {
            this->_resource.maxComboPoints = this->GetBot()->HasSpell(DEEPER_STRATAGEM_SUB) ? 6 : 5;
            _spellsInitialized = true;
        }

        // Update tracking systems
        UpdateSubtletyState();

        // Check stealth status (stealth or Shadow Dance)
        _inStealth = this->GetBot()->HasAuraType(SPELL_AURA_MOD_STEALTH) || _shadowDanceTracker.IsActive();

        // Main rotation
        uint32 enemyCount = this->GetEnemiesInRange(10.0f);
        if (enemyCount >= 3)
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

        if (bot->GetHealthPct() < 50.0f && this->CanCastSpell(RogueAI::EVASION, bot))
        {
            this->CastSpell(RogueAI::EVASION, bot);
        }
    }

    // NOTE: GetOptimalRange is final in base class, cannot override
    // Melee range (5.0f) is already handled by MeleeDpsSpecialization

protected:
    void ExecuteSingleTargetRotation(::Unit* target)
    {
        uint32 energy = this->_resource.energy;
        uint32 cp = this->_resource.comboPoints;
        uint32 maxCp = this->_resource.maxComboPoints;

        // Priority 1: Symbols of Death on cooldown
        if (this->CanCastSpell(RogueAI::SYMBOLS_OF_DEATH, this->GetBot()))
        {
            this->CastSpell(RogueAI::SYMBOLS_OF_DEATH, this->GetBot());
            _symbolsOfDeathActive = true;
            _symbolsOfDeathEndTime = GameTime::GetGameTimeMS() + 10000;
            return;
        }

        // Priority 2: Shadow Blades on cooldown (talent)
        if (this->CanCastSpell(RogueAI::SHADOW_BLADES, this->GetBot()))
        {
            this->CastSpell(RogueAI::SHADOW_BLADES, this->GetBot());
            _shadowBladesActive = true;
            _shadowBladesEndTime = GameTime::GetGameTimeMS() + 20000;
            return;
        }

        // Priority 3: Shadow Dance to build combo points
        if (_shadowDanceTracker.ShouldUse(cp))
        {
            if (this->CanCastSpell(RogueAI::SHADOW_DANCE, this->GetBot()))
            {
                this->CastSpell(RogueAI::SHADOW_DANCE, this->GetBot());
                _shadowDanceTracker.Use();
                _inStealth = true; // Enables stealth abilities
                return;
            }
        }

        // Priority 4: Shadowstrike from stealth/Shadow Dance
        if (_inStealth && energy >= 40)
        {
            if (this->CanCastSpell(SHADOWSTRIKE_SUB, target))
            {
                this->CastSpell(SHADOWSTRIKE_SUB, target);
                _lastShadowstrikeTime = GameTime::GetGameTimeMS();
                ConsumeEnergy(40);
                GenerateComboPoints(2);
                // Shadow Blades makes all attacks give CP
                if (_shadowBladesActive)
                    GenerateComboPoints(1);
                return;
            }
        }

        // Priority 5: Secret Technique finisher at 5-6 CP (talent)
        if (cp >= maxCp && energy >= 30)
        {
            if (this->GetBot()->HasSpell(SECRET_TECHNIQUE) && this->CanCastSpell(SECRET_TECHNIQUE, target))
            {
                this->CastSpell(SECRET_TECHNIQUE, target);
                ConsumeEnergy(30);
                this->_resource.comboPoints = 0;
                return;
            }
        }

        // Priority 6: Eviscerate finisher at 5-6 CP
        if (cp >= (maxCp - 1) && energy >= 35)
        {
            if (this->CanCastSpell(EVISCERATE_SUB, target))
            {
                this->CastSpell(EVISCERATE_SUB, target);
                _lastEviscerateTime = GameTime::GetGameTimeMS();
                ConsumeEnergy(35);
                this->_resource.comboPoints = 0;
                return;
            }
        }

        // Priority 7: Rupture if not active
        if (!HasRupture(target) && cp >= 4 && energy >= 25)
        {
            if (this->CanCastSpell(RogueAI::RUPTURE, target))
            {
                this->CastSpell(RogueAI::RUPTURE, target);
                ConsumeEnergy(25);
                this->_resource.comboPoints = 0;
                return;
            }
        }

        // Priority 8: Backstab for combo points (from behind)
        if (energy >= 35 && cp < maxCp)
        {
            if (this->IsBehindTarget(target) && this->CanCastSpell(RogueAI::BACKSTAB, target))
            {
                this->CastSpell(RogueAI::BACKSTAB, target);
                _lastBackstabTime = GameTime::GetGameTimeMS();
                ConsumeEnergy(35);
                GenerateComboPoints(1);
                if (_shadowBladesActive)
                    GenerateComboPoints(1);
                return;
            }
        }

        // Priority 9: Shadowstrike if can't get behind (less efficient)
        if (energy >= 40 && cp < maxCp)
        {
            if (this->CanCastSpell(SHADOWSTRIKE_SUB, target))
            {
                this->CastSpell(SHADOWSTRIKE_SUB, target);
                ConsumeEnergy(40);
                GenerateComboPoints(2);
                return;
            }
        }
    }

    void ExecuteAoERotation(::Unit* target, uint32 enemyCount)
    {
        uint32 energy = this->_resource.energy;
        uint32 cp = this->_resource.comboPoints;
        uint32 maxCp = this->_resource.maxComboPoints;

        // Priority 1: Shuriken Tornado (talent, sustained AoE)
        if (this->CanCastSpell(SHURIKEN_TORNADO_TALENT, this->GetBot()))
        {
            this->CastSpell(SHURIKEN_TORNADO_TALENT, this->GetBot());
            return;
        }

        // Priority 2: Symbols of Death
        if (this->CanCastSpell(RogueAI::SYMBOLS_OF_DEATH, this->GetBot()))
        {
            this->CastSpell(RogueAI::SYMBOLS_OF_DEATH, this->GetBot());
            _symbolsOfDeathActive = true;
            _symbolsOfDeathEndTime = GameTime::GetGameTimeMS() + 10000;
            return;
        }

        // Priority 3: Shadow Dance
        if (_shadowDanceTracker.CanUse())
        {
            if (this->CanCastSpell(RogueAI::SHADOW_DANCE, this->GetBot()))
            {
                this->CastSpell(RogueAI::SHADOW_DANCE, this->GetBot());
                _shadowDanceTracker.Use();
                _inStealth = true;
                return;
            }
        }

        // Priority 4: Black Powder finisher at 5+ CP
        if (cp >= 5 && energy >= 35)
        {
            if (this->CanCastSpell(BLACK_POWDER, this->GetBot()))
            {
                this->CastSpell(BLACK_POWDER, this->GetBot());
                ConsumeEnergy(35);
                this->_resource.comboPoints = 0;
                return;
            }
        }

        // Priority 5: Shuriken Storm for AoE combo building
        if (energy >= 35 && cp < maxCp)
        {
            if (this->CanCastSpell(SHURIKEN_STORM, this->GetBot()))
            {
                this->CastSpell(SHURIKEN_STORM, this->GetBot());
                ConsumeEnergy(35);
                GenerateComboPoints(::std::min(enemyCount, 5u));
                return;
            }
        }

        // Fallback to single target if AoE abilities on CD
        ExecuteSingleTargetRotation(target);
    }

private:
    void UpdateSubtletyState()
    {
        uint32 now = GameTime::GetGameTimeMS();

        // Update Shadow Dance tracker
        _shadowDanceTracker.Update();

        // Check Symbols of Death expiry
        if (_symbolsOfDeathActive && now >= _symbolsOfDeathEndTime)
        {
            _symbolsOfDeathActive = false;
            _symbolsOfDeathEndTime = 0;
        }

        // Check Shadow Blades expiry
        if (_shadowBladesActive && now >= _shadowBladesEndTime)
        {
            _shadowBladesActive = false;
            _shadowBladesEndTime = 0;
        }

        // Shadow Techniques passive proc: auto-attacks have a chance to
        // generate an additional combo point and restore 8 energy.
        // In WoW 12.0, this occurs roughly every 4th auto-attack.
        // We simulate this by checking the aura and granting the benefit.
        Player* bot = this->GetBot();
        if (bot && bot->HasAura(SHADOW_TECHNIQUES_PROC))
        {
            if (!_shadowTechniquesActive)
            {
                _shadowTechniquesActive = true;
                // Shadow Techniques grants 1 combo point and 8 energy
                GenerateComboPoints(1);
                this->_resource.energy = ::std::min(this->_resource.energy + 8, this->_resource.maxEnergy);
            }
        }
        else
        {
            _shadowTechniquesActive = false;
        }

        // Regenerate energy (10 per second)
        static uint32 lastRegenTime = now;
        uint32 timeDiff = now - lastRegenTime;

        if (timeDiff >= 100) // Every 100ms
        {
            uint32 energyRegen = (timeDiff / 100);
            this->_resource.energy = ::std::min(this->_resource.energy + energyRegen, this->_resource.maxEnergy);
            lastRegenTime = now;
        }
    }

    void ConsumeEnergy(uint32 amount)
    {        this->_resource.energy = (this->_resource.energy > amount) ? this->_resource.energy - amount : 0;
    }

    void GenerateComboPoints(uint32 amount)
    {
        this->_resource.comboPoints = ::std::min(this->_resource.comboPoints + amount, this->_resource.maxComboPoints);
    }

    bool IsBehindTarget(::Unit* target) const
    {
        if (!target || !this->GetBot())
            return false;

        // Check if bot is behind target (simplified)
        return GetBot()->isInBack(target);
    }

    bool HasRupture(::Unit* target) const
    {
        // Simplified - check if target has Rupture aura
        return target && target->HasAura(RogueAI::RUPTURE, this->GetBot()->GetGUID());
    }

    // Phase 5 Integration: Decision Systems Initialization
    void InitializeSubtletyMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;

        // ========================================================================
        // ActionPriorityQueue: Register Subtlety Rogue spells with priorities
        // ========================================================================
        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Defensive cooldowns
            queue->RegisterSpell(RogueAI::CLOAK_OF_SHADOWS, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(RogueAI::CLOAK_OF_SHADOWS, [this](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 30.0f;
            }, "Bot HP < 30% (spell immunity)");

            queue->RegisterSpell(RogueAI::EVASION, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(RogueAI::EVASION, [this](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 50.0f;
            }, "Bot HP < 50% (dodge boost)");

            // CRITICAL: Burst cooldowns and Shadow Dance
            queue->RegisterSpell(RogueAI::SYMBOLS_OF_DEATH, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(RogueAI::SYMBOLS_OF_DEATH, [this](Player* bot, Unit* target) {
                return target && !this->_symbolsOfDeathActive;
            }, "Not active (10s burst, 15% damage increase)");

            queue->RegisterSpell(RogueAI::SHADOW_BLADES, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(RogueAI::SHADOW_BLADES, [this](Player* bot, Unit* target) {
                return bot && bot->HasSpell(RogueAI::SHADOW_BLADES) &&
                       target && !this->_shadowBladesActive;
            }, "Has talent, not active (20s burst, all attacks give CP)");

            queue->RegisterSpell(RogueAI::SHADOW_DANCE, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(RogueAI::SHADOW_DANCE, [this](Player* bot, Unit* target) {
                return target && this->_shadowDanceTracker.ShouldUse(this->_resource.comboPoints);
            }, "Should use (3 charges, 8s duration, enables Shadowstrike)");

            // HIGH: Stealth abilities and finishers
            queue->RegisterSpell(SHADOWSTRIKE_SUB, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SHADOWSTRIKE_SUB, [this](Player* bot, Unit* target) {
                return target && this->_resource.energy >= 40 && this->_inStealth;
            }, "40+ Energy, in stealth/Shadow Dance (generates 2 CP)");

            queue->RegisterSpell(SECRET_TECHNIQUE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SECRET_TECHNIQUE, [this](Player* bot, Unit* target) {
                return bot && bot->HasSpell(SECRET_TECHNIQUE) &&
                       target && this->_resource.energy >= 30 &&
                       this->_resource.comboPoints >= this->_resource.maxComboPoints;
            }, "Has talent, 30+ Energy, max CP (finisher, teleport attacks)");

            queue->RegisterSpell(EVISCERATE_SUB, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(EVISCERATE_SUB, [this](Player* bot, Unit* target) {
                return target && this->_resource.energy >= 35 &&
                       this->_resource.comboPoints >= (this->_resource.maxComboPoints - 1);
            }, "35+ Energy, 4-5+ CP (finisher damage)");

            queue->RegisterSpell(RogueAI::RUPTURE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(RogueAI::RUPTURE, [this](Player* bot, Unit* target) {
                return target && this->_resource.energy >= 25 &&
                       this->_resource.comboPoints >= 4 &&
                       !this->HasRupture(target);
            }, "25+ Energy, 4+ CP, DoT not active (finisher bleed)");

            // MEDIUM: Combo builders
            queue->RegisterSpell(RogueAI::BACKSTAB, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(RogueAI::BACKSTAB, [this](Player* bot, Unit* target) {
                return target && this->_resource.energy >= 35 &&
                       this->_resource.comboPoints < this->_resource.maxComboPoints &&
                       this->IsBehindTarget(target);
            }, "35+ Energy, not max CP, behind target (generates 1 CP)");

            queue->RegisterSpell(SHADOWSTRIKE_SUB, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SHADOWSTRIKE_SUB, [this](Player* bot, Unit* target) {
                return target && this->_resource.energy >= 40 &&
                       this->_resource.comboPoints < this->_resource.maxComboPoints &&
                       !this->_inStealth;
            }, "40+ Energy, not max CP, not in stealth (fallback builder)");

            queue->RegisterSpell(RogueAI::KICK, SpellPriority::MEDIUM, SpellCategory::UTILITY);
            queue->AddCondition(RogueAI::KICK, [this](Player* bot, Unit* target) {
                return target && target->IsNonMeleeSpellCast(false);
            }, "Target casting (interrupt)");

            // LOW: AoE abilities
            queue->RegisterSpell(SHURIKEN_STORM, SpellPriority::LOW, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(SHURIKEN_STORM, [this](Player* bot, Unit* target) {
                return target && this->_resource.energy >= 35 &&
                       this->GetEnemiesInRange(10.0f) >= 3 &&
                       this->_resource.comboPoints < this->_resource.maxComboPoints;
            }, "35+ Energy, 3+ enemies, not max CP (AoE combo builder)");

            queue->RegisterSpell(BLACK_POWDER, SpellPriority::LOW, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(BLACK_POWDER, [this](Player* bot, Unit* target) {
                return target && this->_resource.energy >= 35 &&
                       this->GetEnemiesInRange(10.0f) >= 3 &&
                       this->_resource.comboPoints >= 5;
            }, "35+ Energy, 3+ enemies, 5+ CP (AoE finisher)");

            TC_LOG_INFO("module.playerbot", " SUBTLETY ROGUE: Registered {} spells in ActionPriorityQueue", queue->GetSpellCount());
        }

        // ========================================================================
        // BehaviorTree: Subtlety Rogue DPS rotation logic
        // ========================================================================
        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Subtlety Rogue DPS", {
                // Tier 1: Burst Cooldowns (Symbols of Death, Shadow Blades)
                Sequence("Burst Cooldowns", {
                    Condition("Target exists", [this](Player* bot, Unit* target) {
                        return target != nullptr;
                    }),
                    Selector("Use Burst", {
                        Sequence("Cast Symbols of Death", {
                            Condition("Not active", [this](Player* bot, Unit*) {
                                return !this->_symbolsOfDeathActive;
                            }),
                            bot::ai::Action("Cast Symbols of Death", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(RogueAI::SYMBOLS_OF_DEATH, bot))
                                {
                                    this->CastSpell(RogueAI::SYMBOLS_OF_DEATH, bot);
                                    this->_symbolsOfDeathActive = true;
                                    this->_symbolsOfDeathEndTime = GameTime::GetGameTimeMS() + 10000;
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Cast Shadow Blades", {
                            Condition("Has talent and not active", [this](Player* bot, Unit*) {
                                return bot && bot->HasSpell(RogueAI::SHADOW_BLADES) &&
                                       !this->_shadowBladesActive;
                            }),
                            bot::ai::Action("Cast Shadow Blades", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(RogueAI::SHADOW_BLADES, bot))
                                {
                                    this->CastSpell(RogueAI::SHADOW_BLADES, bot);
                                    this->_shadowBladesActive = true;
                                    this->_shadowBladesEndTime = GameTime::GetGameTimeMS() + 20000;
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 2: Shadow Dance (enable stealth abilities)
                Sequence("Shadow Dance", {
                    Condition("Target exists and should use", [this](Player* bot, Unit* target) {
                        return target != nullptr &&
                               this->_shadowDanceTracker.ShouldUse(this->_resource.comboPoints);
                    }),
                    bot::ai::Action("Cast Shadow Dance", [this](Player* bot, Unit* target) -> NodeStatus {
                        if (this->CanCastSpell(RogueAI::SHADOW_DANCE, bot))
                        {
                            this->CastSpell(RogueAI::SHADOW_DANCE, bot);
                            this->_shadowDanceTracker.Use();
                            this->_inStealth = true;
                            return NodeStatus::SUCCESS;
                        }
                        return NodeStatus::FAILURE;
                    })
                }),

                // Tier 3: Finishers (Secret Technique, Eviscerate, Rupture at 4-5+ CP)
                Sequence("Finishers", {
                    Condition("Target exists and has CP", [this](Player* bot, Unit* target) {
                        return target != nullptr &&
                               this->_resource.comboPoints >= (this->_resource.maxComboPoints - 1);
                    }),
                    Selector("Choose Finisher", {
                        // Rupture if not active
                        Sequence("Cast Rupture", {
                            Condition("Rupture missing and 4+ CP", [this](Player* bot, Unit* target) {
                                return this->_resource.comboPoints >= 4 &&
                                       !this->HasRupture(target) &&
                                       this->_resource.energy >= 25;
                            }),
                            bot::ai::Action("Cast Rupture", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(RogueAI::RUPTURE, target))
                                {
                                    this->CastSpell(RogueAI::RUPTURE, target);
                                    this->ConsumeEnergy(25);
                                    this->_resource.comboPoints = 0;
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        // Secret Technique at max CP
                        Sequence("Cast Secret Technique", {
                            Condition("Has talent and max CP", [this](Player* bot, Unit*) {
                                return bot && bot->HasSpell(SECRET_TECHNIQUE) &&
                                       this->_resource.comboPoints >= this->_resource.maxComboPoints &&
                                       this->_resource.energy >= 30;
                            }),
                            bot::ai::Action("Cast Secret Technique", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(SECRET_TECHNIQUE, target))
                                {
                                    this->CastSpell(SECRET_TECHNIQUE, target);
                                    this->ConsumeEnergy(30);
                                    this->_resource.comboPoints = 0;
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        // Eviscerate at 4-5+ CP
                        Sequence("Cast Eviscerate", {
                            Condition("35+ Energy", [this](Player* bot, Unit*) {
                                return this->_resource.energy >= 35;
                            }),
                            bot::ai::Action("Cast Eviscerate", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(EVISCERATE_SUB, target))
                                {
                                    this->CastSpell(EVISCERATE_SUB, target);
                                    this->_lastEviscerateTime = GameTime::GetGameTimeMS();
                                    this->ConsumeEnergy(35);
                                    this->_resource.comboPoints = 0;
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 4: Combo Builders (Shadowstrike in stealth, Backstab from behind)
                Sequence("Combo Builders", {
                    Condition("Target exists", [this](Player* bot, Unit* target) {
                        return target != nullptr &&
                               this->_resource.comboPoints < this->_resource.maxComboPoints;
                    }),
                    Selector("Build Combo Points", {
                        // Shadowstrike from stealth/Shadow Dance
                        Sequence("Cast Shadowstrike in stealth", {
                            Condition("In stealth and 40+ Energy", [this](Player* bot, Unit*) {
                                return this->_inStealth && this->_resource.energy >= 40;
                            }),
                            bot::ai::Action("Cast Shadowstrike", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(SHADOWSTRIKE_SUB, target))
                                {
                                    this->CastSpell(SHADOWSTRIKE_SUB, target);
                                    this->_lastShadowstrikeTime = GameTime::GetGameTimeMS();
                                    this->ConsumeEnergy(40);
                                    this->GenerateComboPoints(2);
                                    if (this->_shadowBladesActive)
                                        this->GenerateComboPoints(1);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        // Backstab from behind
                        Sequence("Cast Backstab", {
                            Condition("Behind target and 35+ Energy", [this](Player* bot, Unit* target) {
                                return this->IsBehindTarget(target) && this->_resource.energy >= 35;
                            }),
                            bot::ai::Action("Cast Backstab", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(RogueAI::BACKSTAB, target))
                                {
                                    this->CastSpell(RogueAI::BACKSTAB, target);
                                    this->_lastBackstabTime = GameTime::GetGameTimeMS();
                                    this->ConsumeEnergy(35);
                                    this->GenerateComboPoints(1);
                                    if (this->_shadowBladesActive)
                                        this->GenerateComboPoints(1);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        // Shadowstrike fallback
                        Sequence("Cast Shadowstrike fallback", {
                            Condition("40+ Energy", [this](Player* bot, Unit*) {
                                return this->_resource.energy >= 40;
                            }),
                            bot::ai::Action("Cast Shadowstrike", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(SHADOWSTRIKE_SUB, target))
                                {
                                    this->CastSpell(SHADOWSTRIKE_SUB, target);
                                    this->ConsumeEnergy(40);
                                    this->GenerateComboPoints(2);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                })
            });

            behaviorTree->SetRoot(root);
            TC_LOG_INFO("module.playerbot", " SUBTLETY ROGUE: BehaviorTree initialized with 4-tier DPS rotation");
        }
    }

private:
    ShadowDanceTracker _shadowDanceTracker;
    bool _inStealth;
    bool _symbolsOfDeathActive;
    uint32 _symbolsOfDeathEndTime;
    bool _shadowBladesActive;
    uint32 _shadowBladesEndTime;
    uint32 _lastBackstabTime;
    uint32 _lastShadowstrikeTime;
    uint32 _lastEviscerateTime;
    bool _shadowTechniquesActive;  // Shadow Techniques passive proc tracking
    bool _spellsInitialized;  // Deferred initialization flag

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
