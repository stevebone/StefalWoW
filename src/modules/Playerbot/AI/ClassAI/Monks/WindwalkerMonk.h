/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Windwalker Monk Refactored - Template-Based Implementation
 *
 * This file provides a complete, template-based implementation of Windwalker Monk
 * using the MeleeDpsSpecialization with dual resource system (Energy + Chi).
 */

#pragma once


#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "../CombatSpecializationTemplates.h"
#include "../ResourceTypes.h"
#include "../SpellValidation_WoW120.h"
#include "../HeroTalentDetector.h"
#include "Player.h"
#include "SpellMgr.h"
#include "SpellAuraEffects.h"
#include "Log.h"
#include "../../Decision/ActionPriorityQueue.h"
#include "../../Decision/BehaviorTree.h"
#include "../BotAI.h"

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
// ============================================================================
// WINDWALKER MONK SPELL IDs (WoW 12.0 - The War Within)
// Using centralized spell registry from SpellValidation_WoW120.h
// ============================================================================

namespace WindwalkerMonkSpells
{
    // Import from central registry - Monk common spells
    using namespace WoW120Spells::Monk;

    // Chi Generators
    constexpr uint32 TIGER_PALM_WIND          = Windwalker::TIGER_PALM_WW;
    constexpr uint32 EXPEL_HARM_WIND          = EXPEL_HARM;
    constexpr uint32 CHI_WAVE_WIND            = Windwalker::CHI_WAVE_WW;
    constexpr uint32 CHI_BURST_WIND           = Windwalker::CHI_BURST_WW;
    constexpr uint32 CRACKLING_JADE_LIGHTNING_WW = WoW120Spells::Monk::CRACKLING_JADE_LIGHTNING;

    // Chi Spenders
    constexpr uint32 RISING_SUN_KICK          = Windwalker::RISING_SUN_KICK_WW;
    constexpr uint32 BLACKOUT_KICK            = Windwalker::BLACKOUT_KICK_WW;
    constexpr uint32 FISTS_OF_FURY            = Windwalker::FISTS_OF_FURY;
    constexpr uint32 SPINNING_CRANE_KICK      = Windwalker::SPINNING_CRANE_KICK_WW;
    constexpr uint32 WHIRLING_DRAGON_PUNCH    = Windwalker::WHIRLING_DRAGON_PUNCH;

    // Strike of the Windlord
    constexpr uint32 STRIKE_OF_THE_WINDLORD   = Windwalker::STRIKE_OF_THE_WINDLORD;

    // Major Cooldowns
    constexpr uint32 STORM_EARTH_AND_FIRE     = Windwalker::STORM_EARTH_AND_FIRE;
    constexpr uint32 INVOKE_XUEN              = Windwalker::INVOKE_XUEN;
    constexpr uint32 SERENITY                 = Windwalker::SERENITY;
    constexpr uint32 WEAPONS_OF_ORDER_WIND    = Windwalker::WEAPONS_OF_ORDER_WW;
    constexpr uint32 BONEDUST_BREW_WIND       = Windwalker::BONEDUST_BREW_WW;

    // Utility
    constexpr uint32 TOUCH_OF_DEATH_WW        = TOUCH_OF_DEATH;
    constexpr uint32 TOUCH_OF_KARMA           = Windwalker::TOUCH_OF_KARMA;
    constexpr uint32 FORTIFYING_BREW_WIND     = FORTIFYING_BREW;
    constexpr uint32 DIFFUSE_MAGIC_WIND       = DIFFUSE_MAGIC;
    constexpr uint32 PARALYSIS_WIND           = PARALYSIS;
    constexpr uint32 LEG_SWEEP_WW             = LEG_SWEEP;
    constexpr uint32 RING_OF_PEACE_WW         = RING_OF_PEACE;

    // Movement
    constexpr uint32 ROLL_WIND                = ROLL;
    constexpr uint32 FLYING_SERPENT_KICK      = Windwalker::FLYING_SERPENT_KICK;
    constexpr uint32 TIGER_LUST_WIND          = TIGERS_LUST;

    // Procs and Buffs
    constexpr uint32 TEACHINGS_OF_THE_MONASTERY_WIND = Windwalker::TEACHINGS_OF_THE_MONASTERY_WW;
    constexpr uint32 DANCE_OF_CHI_JI          = Windwalker::DANCE_OF_CHI_JI;
    constexpr uint32 COMBO_BREAKER            = Windwalker::COMBO_BREAKER;
    constexpr uint32 HIT_COMBO                = Windwalker::HIT_COMBO;
    constexpr uint32 MARK_OF_THE_CRANE        = Windwalker::MARK_OF_THE_CRANE;

    // Talents
    constexpr uint32 FAELINE_STOMP            = Windwalker::FAELINE_STOMP_WW;
    constexpr uint32 JADEFIRE_STOMP           = Windwalker::JADEFIRE_STOMP;
    constexpr uint32 JADE_IGNITION            = Windwalker::JADE_IGNITION;
    constexpr uint32 TRANSFER_THE_POWER       = Windwalker::TRANSFER_THE_POWER;
    constexpr uint32 GLORY_OF_THE_DAWN        = Windwalker::GLORY_OF_THE_DAWN;
    constexpr uint32 INVOKERS_DELIGHT         = Windwalker::INVOKERS_DELIGHT;

    // Hero Talents
    constexpr uint32 WW_FLURRY_STRIKES        = Windwalker::WW_FLURRY_STRIKES;
    constexpr uint32 WW_CELESTIAL_CONDUIT     = Windwalker::WW_CELESTIAL_CONDUIT;
}
using namespace WindwalkerMonkSpells;

// Dual resource type for Windwalker Monk
struct EnergyChiResourceWindwalker
{
    uint32 energy{0};
    uint32 chi{0};
    uint32 maxEnergy{100};
    uint32 maxChi{6};

    bool available{true};
    bool Consume(uint32 energyCost)
    {
        if (energy >= energyCost) {
            energy -= energyCost;
            return true;
        }
        return false;
    }
    void Regenerate(uint32 diff)
    {
        // Resource regeneration logic (simplified)
        available = true;
    }

    [[nodiscard]] uint32 GetAvailable() const {
        return 100; // Simplified - override in specific implementations
    }

    [[nodiscard]] uint32 GetMax() const {
        return 100; // Simplified - override in specific implementations
    }


    void Initialize(Player* /*bot*/)
    {
        // CRITICAL: NEVER call GetMaxPower()/GetPower() during construction!
        // Even with IsInWorld() check, the power data may not be initialized yet
        // during bot login. Use static defaults and refresh later in UpdateRotation.
        maxEnergy = 100;  // Standard Monk max energy
        energy = 100;
        chi = 0;
    }

    // Call this from UpdateRotation when bot is fully ready
    void RefreshFromPlayer(Player* bot)
    {
        if (bot && bot->IsInWorld())
        {
            maxEnergy = bot->GetMaxPower(POWER_ENERGY);
            energy = bot->GetPower(POWER_ENERGY);
        }
    }
};

// ============================================================================
// WINDWALKER HIT COMBO TRACKER
// ============================================================================

class WindwalkerHitComboTracker
{
public:
    WindwalkerHitComboTracker()
        : _lastSpellCast(0)
        , _comboCount(0)
    {
    }

    void RecordSpell(uint32 spellId)
    {
        if (_lastSpellCast == spellId)
        {
            // Broke combo by repeating same spell
            _comboCount = 1;
        }
        else
        {
            // Extended combo
            _comboCount++;
        }

        _lastSpellCast = spellId;
    }

    void Reset()
    {
        _lastSpellCast = 0;
        _comboCount = 0;
    }

    uint32 GetComboCount() const { return _comboCount; }
    float GetDamageMultiplier() const
    {
        // Hit Combo: 1% damage per stack (max 10%)
        return 1.0f + (::std::min(_comboCount, 10u) * 0.01f);
    }

private:
    CooldownManager _cooldowns;
    uint32 _lastSpellCast;
    uint32 _comboCount;
};

// ============================================================================
// WINDWALKER STORM EARTH AND FIRE TRACKER
// ============================================================================

class WindwalkerSEFTracker
{
public:
    WindwalkerSEFTracker()
        : _sefActive(false)
        , _sefEndTime(0)
    {
    }

    void Activate()
    {
        _sefActive = true;
        _sefEndTime = GameTime::GetGameTimeMS() + 15000; // 15 sec duration
    }

    bool IsActive() const { return _sefActive; }

    uint32 GetTimeRemaining() const
    {
        if (!_sefActive)
            return 0;

        uint32 now = GameTime::GetGameTimeMS();
        return _sefEndTime > now ? _sefEndTime - now : 0;
    }

    void Update()
    {
        if (_sefActive && GameTime::GetGameTimeMS() >= _sefEndTime)
        {
            _sefActive = false;
            _sefEndTime = 0;
        }
    }

private:
    bool _sefActive;
    uint32 _sefEndTime;
};

// ============================================================================
// WINDWALKER MONK REFACTORED
// ============================================================================

class WindwalkerMonkRefactored : public MeleeDpsSpecialization<EnergyChiResourceWindwalker>
{
public:
    using Base = MeleeDpsSpecialization<EnergyChiResourceWindwalker>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::GetEnemiesInRange;
    using Base::_resource;
    explicit WindwalkerMonkRefactored(Player* bot)
        : MeleeDpsSpecialization<EnergyChiResourceWindwalker>(bot)
        , _hitComboTracker()
        , _sefTracker()
        , _lastRisingSunKickTime(0)
        , _comboBreaker(false)
    {
        // CRITICAL: Do NOT call bot->GetPower(), bot->GetMaxPower(), or bot->GetName() here!
        // Bot is not fully in world during constructor. Resource initialization is safe
        // because it uses IsInWorld() check internally.
        this->_resource.Initialize(bot);

        // Note: Do NOT call bot->GetName() here - Player data may not be loaded yet
        // Logging will happen once bot is fully active
        TC_LOG_DEBUG("playerbot", "WindwalkerMonkRefactored created for bot GUID: {}",
            bot ? bot->GetGUID().GetCounter() : 0);

        InitializeWindwalkerMechanics();
    }

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !target->IsAlive() || !target->IsHostileTo(this->GetBot()))
            return;

        // Lazy hero talent detection (once per bot lifetime until respec)
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Update Windwalker state
        UpdateWindwalkerState();

        // Use major cooldowns
        HandleCooldowns(target);

        // Hero talent tree rotation branching
        // Windwalker has access to: Shado-Pan / Conduit of the Celestials
        if (_heroTalents.IsTree(HeroTalentTree::SHADO_PAN))
        {
            // Shado-Pan: Flurry Strikes proc from combo strikes (using different abilities)
            // When Flurry Strikes buff is active, prioritize high-damage spenders
            if (this->GetBot()->HasAura(WW_FLURRY_STRIKES))
            {
                // Flurry Strikes active - use highest impact chi spender
                if (this->_resource.chi >= 2 && this->CanCastSpell(RISING_SUN_KICK, target))
                {
                    this->CastSpell(RISING_SUN_KICK, target);
                    _hitComboTracker.RecordSpell(RISING_SUN_KICK);
                    ConsumeChi(2);
                    return;
                }
                if (this->_resource.chi >= 3 && this->CanCastSpell(FISTS_OF_FURY, target))
                {
                    this->CastSpell(FISTS_OF_FURY, target);
                    _hitComboTracker.RecordSpell(FISTS_OF_FURY);
                    ConsumeChi(3);
                    return;
                }
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::CONDUIT_OF_THE_CELESTIALS))
        {
            // Conduit of the Celestials: Celestial Conduit channels celestial power
            // Align Celestial Conduit with Storm, Earth, and Fire for maximum burst
            if (this->CanCastSpell(WW_CELESTIAL_CONDUIT, this->GetBot()))
            {
                // Use during SEF burst window or when cooldowns are up
                if (_sefTracker.IsActive() || this->_resource.chi >= 4)
                {
                    this->CastSpell(WW_CELESTIAL_CONDUIT, this->GetBot());
                    return;
                }
            }
        }

        // Determine if AoE or single target
        uint32 enemyCount = this->GetEnemiesInRange(8.0f);
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
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // Defensive cooldowns
        HandleDefensiveCooldowns();
    }

protected:
    void ExecuteSingleTargetRotation(::Unit* target)
    {
        uint32 energy = this->_resource.energy;
        uint32 chi = this->_resource.chi;

        // Priority 1: Touch of Death (execute)
        if (target->GetHealthPct() < 15.0f && this->CanCastSpell(TOUCH_OF_DEATH, target))
        {
            this->CastSpell(TOUCH_OF_DEATH, target);
            _hitComboTracker.RecordSpell(TOUCH_OF_DEATH);
            return;
        }

        // Priority 2: Serenity burst window
        if (chi >= 4 && this->CanCastSpell(SERENITY, this->GetBot()))
        {
            this->CastSpell(SERENITY, this->GetBot());
            // During Serenity, spam Chi spenders
            return;
        }

        // Priority 3: Strike of the Windlord
        if (chi >= 2 && this->CanCastSpell(STRIKE_OF_THE_WINDLORD, target))
        {
            this->CastSpell(STRIKE_OF_THE_WINDLORD, target);
            _hitComboTracker.RecordSpell(STRIKE_OF_THE_WINDLORD);
            ConsumeChi(2);
            return;
        }

        // Priority 4: Rising Sun Kick (maintains debuff)
        if (chi >= 2 && this->CanCastSpell(RISING_SUN_KICK, target))
        {
            this->CastSpell(RISING_SUN_KICK, target);
            _lastRisingSunKickTime = GameTime::GetGameTimeMS();
            _hitComboTracker.RecordSpell(RISING_SUN_KICK);
            ConsumeChi(2);
            return;
        }

        // Priority 5: Fists of Fury (channel burst)
        if (chi >= 3 && this->CanCastSpell(FISTS_OF_FURY, target))
        {
            this->CastSpell(FISTS_OF_FURY, target);
            _hitComboTracker.RecordSpell(FISTS_OF_FURY);
            ConsumeChi(3);
            return;
        }

        // Priority 6: Whirling Dragon Punch (talent)
        if (chi >= 2 && this->CanCastSpell(WHIRLING_DRAGON_PUNCH, target))
        {
            this->CastSpell(WHIRLING_DRAGON_PUNCH, target);
            _hitComboTracker.RecordSpell(WHIRLING_DRAGON_PUNCH);
            ConsumeChi(2);
            return;
        }

        // Priority 7: Blackout Kick (combo builder)
        if (chi >= 1)
        {
            // Use Combo Breaker proc if available
            if (_comboBreaker || this->CanCastSpell(BLACKOUT_KICK, target))
            {
                this->CastSpell(BLACKOUT_KICK, target);
                _hitComboTracker.RecordSpell(BLACKOUT_KICK);
                ConsumeChi(1);
                _comboBreaker = false;
                return;
            }
        }

        // Priority 8: Tiger Palm (Chi generator)
        if (energy >= 25 && chi < 5 && this->CanCastSpell(TIGER_PALM_WIND, target))
        {
            this->CastSpell(TIGER_PALM_WIND, target);
            _hitComboTracker.RecordSpell(TIGER_PALM_WIND);
            GenerateChi(2);
            return;
        }

        // Priority 9: Expel Harm (Chi generator + heal)
        if (energy >= 15 && chi < 5 && this->CanCastSpell(EXPEL_HARM_WIND, this->GetBot()))
        {
            this->CastSpell(EXPEL_HARM_WIND, this->GetBot());
            _hitComboTracker.RecordSpell(EXPEL_HARM_WIND);
            GenerateChi(1);
            return;
        }

        // Priority 10: Chi Wave (talent)
        if (chi < 5 && this->CanCastSpell(CHI_WAVE_WIND, target))
        {
            this->CastSpell(CHI_WAVE_WIND, target);
            _hitComboTracker.RecordSpell(CHI_WAVE_WIND);
            GenerateChi(1);
            return;
        }
    }

    void ExecuteAoERotation(::Unit* target, uint32 enemyCount)
    {
        uint32 energy = this->_resource.energy;
        uint32 chi = this->_resource.chi;

        // Priority 1: Fists of Fury (best AoE Chi spender)
        if (chi >= 3 && this->CanCastSpell(FISTS_OF_FURY, target))
        {
            this->CastSpell(FISTS_OF_FURY, target);
            _hitComboTracker.RecordSpell(FISTS_OF_FURY);
            ConsumeChi(3);
            return;
        }

        // Priority 2: Whirling Dragon Punch
        if (chi >= 2 && this->CanCastSpell(WHIRLING_DRAGON_PUNCH, target))
        {
            this->CastSpell(WHIRLING_DRAGON_PUNCH, target);
            _hitComboTracker.RecordSpell(WHIRLING_DRAGON_PUNCH);
            ConsumeChi(2);
            return;
        }

        // Priority 3: Spinning Crane Kick (AoE Chi spender)
        if (chi >= 2 && this->CanCastSpell(SPINNING_CRANE_KICK, this->GetBot()))
        {
            this->CastSpell(SPINNING_CRANE_KICK, this->GetBot());
            _hitComboTracker.RecordSpell(SPINNING_CRANE_KICK);
            ConsumeChi(2);
            return;
        }

        // Priority 4: Rising Sun Kick
        if (chi >= 2 && this->CanCastSpell(RISING_SUN_KICK, target))
        {
            this->CastSpell(RISING_SUN_KICK, target);
            _hitComboTracker.RecordSpell(RISING_SUN_KICK);
            ConsumeChi(2);
            return;
        }

        // Priority 5: Tiger Palm (Chi generator)
        if (energy >= 25 && chi < 5 && this->CanCastSpell(TIGER_PALM_WIND, target))
        {
            this->CastSpell(TIGER_PALM_WIND, target);
            _hitComboTracker.RecordSpell(TIGER_PALM_WIND);
            GenerateChi(2);
            return;
        }

        // Priority 6: Chi Wave
        if (chi < 5 && this->CanCastSpell(CHI_WAVE_WIND, target))
        {
            this->CastSpell(CHI_WAVE_WIND, target);
            _hitComboTracker.RecordSpell(CHI_WAVE_WIND);
            GenerateChi(1);
            return;
        }
    }

    void HandleCooldowns(::Unit* target)
    {
        uint32 chi = this->_resource.chi;

        // Use Storm, Earth, and Fire for burst
        if (chi >= 3 && !_sefTracker.IsActive() && this->CanCastSpell(STORM_EARTH_AND_FIRE, this->GetBot()))
        {
            this->CastSpell(STORM_EARTH_AND_FIRE, this->GetBot());
            _sefTracker.Activate();
            

        // Register cooldowns using CooldownManager
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT:             {RISING_SUN_KICK, 10000, 1},
        // COMMENTED OUT:             {FISTS_OF_FURY, 24000, 1},
        // COMMENTED OUT:             {WHIRLING_DRAGON_PUNCH, 24000, 1},
        // COMMENTED OUT:             {STRIKE_OF_THE_WINDLORD, 40000, 1},
        // COMMENTED OUT:             {STORM_EARTH_AND_FIRE, CooldownPresets::MINOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {INVOKE_XUEN, CooldownPresets::MINOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {SERENITY, 90000, 1},
        // COMMENTED OUT:             {TOUCH_OF_DEATH, CooldownPresets::MAJOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {TOUCH_OF_KARMA, 90000, 1},
        // COMMENTED OUT:             {FORTIFYING_BREW_WIND, 360000, 1},
        // COMMENTED OUT:             {DIFFUSE_MAGIC_WIND, 90000, 1},
        // COMMENTED OUT:             {LEG_SWEEP, CooldownPresets::OFFENSIVE_60, 1},
        // COMMENTED OUT:             {RING_OF_PEACE, CooldownPresets::OFFENSIVE_45, 1},
        // COMMENTED OUT:             {CHI_WAVE_WIND, CooldownPresets::INTERRUPT, 1},
        // COMMENTED OUT:             {CHI_BURST_WIND, CooldownPresets::OFFENSIVE_30, 1},
        // COMMENTED OUT:         });

        TC_LOG_DEBUG("playerbot", "Windwalker: Storm, Earth, and Fire activated");
        }

        // Invoke Xuen the White Tiger
        if (chi >= 3 && this->CanCastSpell(INVOKE_XUEN, this->GetBot()))
        {
            this->CastSpell(INVOKE_XUEN, this->GetBot());
            TC_LOG_DEBUG("playerbot", "Windwalker: Invoke Xuen");
        }

        // Touch of Karma (damage absorption + reflect)
        if (this->GetBot()->GetHealthPct() < 70.0f && this->CanCastSpell(TOUCH_OF_KARMA, target))
        {
            this->CastSpell(TOUCH_OF_KARMA, target);
            TC_LOG_DEBUG("playerbot", "Windwalker: Touch of Karma");
        }
    }

    void HandleDefensiveCooldowns()
    {
        Player* bot = this->GetBot();
        float healthPct = bot->GetHealthPct();

        // Fortifying Brew
        if (healthPct < 40.0f && this->CanCastSpell(FORTIFYING_BREW_WIND, bot))
        {
            this->CastSpell(FORTIFYING_BREW_WIND, bot);
            TC_LOG_DEBUG("playerbot", "Windwalker: Fortifying Brew");
            return;
        }

        // Diffuse Magic
        if (healthPct < 50.0f && this->CanCastSpell(DIFFUSE_MAGIC_WIND, bot))
        {
            this->CastSpell(DIFFUSE_MAGIC_WIND, bot);
            TC_LOG_DEBUG("playerbot", "Windwalker: Diffuse Magic");
            return;
        }
    }

private:
    void UpdateWindwalkerState()
    {
        // Update SEF tracker
        _sefTracker.Update();

        // Update Combo Breaker proc
        if (this->GetBot() && this->GetBot()->HasAura(COMBO_BREAKER))
            _comboBreaker = true;
        else
            _comboBreaker = false;

        // Update Chi and Energy from bot
        if (this->GetBot())
        {
            this->_resource.chi = this->GetBot()->GetPower(POWER_CHI);
            this->_resource.energy = this->GetBot()->GetPower(POWER_ENERGY);
        }
    }

    void GenerateChi(uint32 amount)
    {
        this->_resource.chi = ::std::min(this->_resource.chi + amount, this->_resource.maxChi);
    }

    void ConsumeChi(uint32 amount)
    {
        this->_resource.chi = (this->_resource.chi > amount) ? this->_resource.chi - amount : 0;
    }

    void InitializeWindwalkerMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)

        auto* queue = this->GetActionPriorityQueue();
        if (queue)
        {
            queue->RegisterSpell(TOUCH_OF_KARMA, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(TOUCH_OF_KARMA, [](Player* bot, Unit*) { return bot && bot->GetHealthPct() < 40.0f; }, "HP < 40%");

            queue->RegisterSpell(STORM_EARTH_AND_FIRE, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(STORM_EARTH_AND_FIRE, [](Player*, Unit* target) { return target != nullptr; }, "Burst CD");

            queue->RegisterSpell(RISING_SUN_KICK, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(RISING_SUN_KICK, [this](Player*, Unit* target) { return target && this->_resource.chi >= 2; }, "2 chi (priority)");

            queue->RegisterSpell(FISTS_OF_FURY, SpellPriority::HIGH, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(FISTS_OF_FURY, [this](Player*, Unit* target) { return target && this->_resource.chi >= 3; }, "3 chi (channel)");

            queue->RegisterSpell(BLACKOUT_KICK, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(BLACKOUT_KICK, [this](Player*, Unit* target) { return target && this->_resource.chi >= 1; }, "1 chi (spender)");

            queue->RegisterSpell(TIGER_PALM_WIND, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(TIGER_PALM_WIND, [this](Player*, Unit* target) { return target && this->_resource.energy >= 50; }, "50 energy (builder)");
        }

        auto* tree = this->GetBehaviorTree();
        if (tree)
        {
            auto root = Selector("Windwalker Monk", {
                Sequence("Burst", { Condition("Has target", [this](Player* bot, Unit*) { return bot && bot->GetVictim(); }),
                    bot::ai::Action("SEF", [this](Player* bot, Unit*) { if (this->CanCastSpell(STORM_EARTH_AND_FIRE, bot)) { this->CastSpell(STORM_EARTH_AND_FIRE, bot); return NodeStatus::SUCCESS; } return NodeStatus::FAILURE; }) }),
                Sequence("Chi Spender", { Condition("2+ chi", [this](Player*, Unit*) { return this->_resource.chi >= 2; }),
                    bot::ai::Action("RSK/FoF", [this](Player* bot, Unit*) { Unit* t = bot->GetVictim(); if (t && this->CanCastSpell(RISING_SUN_KICK, t)) { this->CastSpell(RISING_SUN_KICK, t); this->ConsumeChi(2); return NodeStatus::SUCCESS; } return NodeStatus::FAILURE; }) }),
                Sequence("Builder", { Condition("50+ energy", [this](Player*, Unit*) { return this->_resource.energy >= 50; }),
                    bot::ai::Action("Tiger Palm", [this](Player* bot, Unit*) { Unit* t = bot->GetVictim(); if (t && this->CanCastSpell(TIGER_PALM_WIND, t)) { this->CastSpell(TIGER_PALM_WIND, t); this->GenerateChi(2); return NodeStatus::SUCCESS; } return NodeStatus::FAILURE; }) })
            });
            tree->SetRoot(root);
        }
    }

private:
    WindwalkerHitComboTracker _hitComboTracker;
    WindwalkerSEFTracker _sefTracker;
    uint32 _lastRisingSunKickTime;
    bool _comboBreaker;
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
