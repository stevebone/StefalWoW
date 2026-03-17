/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Brewmaster Monk Refactored - Template-Based Implementation
 *
 * This file provides a complete, template-based implementation of Brewmaster Monk
 * using the TankSpecialization with dual resource system (Energy + Chi).
 */

#pragma once


#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "../CombatSpecializationTemplates.h"
#include "../ResourceTypes.h"
#include "../../Services/ThreatAssistant.h"
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
// BREWMASTER MONK SPELL IDs (WoW 12.0 - The War Within)
// Using centralized spell registry from SpellValidation_WoW120.h
// ============================================================================

namespace BrewmasterMonkSpells
{
    // Import from central registry - Monk common spells
    using namespace WoW120Spells::Monk;

    // Chi Generators
    constexpr uint32 KEG_SMASH                = Brewmaster::KEG_SMASH;
    constexpr uint32 TIGER_PALM_BREW          = TIGER_PALM;
    constexpr uint32 EXPEL_HARM_BREW          = EXPEL_HARM;
    constexpr uint32 CHI_WAVE_BREW            = CHI_WAVE;
    constexpr uint32 CHI_BURST                = Mistweaver::CHI_BURST; // Shared spell ID

    // Chi Spenders
    constexpr uint32 BLACKOUT_KICK_BREW       = Brewmaster::BLACKOUT_KICK;
    constexpr uint32 BREATH_OF_FIRE           = Brewmaster::BREATH_OF_FIRE;
    constexpr uint32 SPINNING_CRANE_KICK_BREW = Brewmaster::SPINNING_CRANE_KICK;
    constexpr uint32 RISING_SUN_KICK_BREW     = Mistweaver::RISING_SUN_KICK; // Shared with MW

    // Active Mitigation (Brews)
    constexpr uint32 PURIFYING_BREW           = Brewmaster::PURIFYING_BREW;
    constexpr uint32 CELESTIAL_BREW           = Brewmaster::CELESTIAL_BREW;
    constexpr uint32 FORTIFYING_BREW_BREW     = FORTIFYING_BREW;

    // Stagger
    constexpr uint32 STAGGER                  = Brewmaster::STAGGER;
    constexpr uint32 LIGHT_STAGGER            = Brewmaster::LIGHT_STAGGER;
    constexpr uint32 MODERATE_STAGGER         = Brewmaster::MODERATE_STAGGER;
    constexpr uint32 HEAVY_STAGGER            = Brewmaster::HEAVY_STAGGER;
    constexpr uint32 SHUFFLE                  = Brewmaster::SHUFFLE;
    constexpr uint32 IRONSKIN_BREW            = 115308; // Legacy - replaced by Shuffle in TWW

    // Threat Generation
    constexpr uint32 PROVOKE_TAUNT            = PROVOKE;

    // Major Cooldowns
    constexpr uint32 INVOKE_NIUZAO            = Brewmaster::INVOKE_NIUZAO;
    constexpr uint32 WEAPONS_OF_ORDER         = Brewmaster::WEAPONS_OF_ORDER;
    constexpr uint32 BONEDUST_BREW            = Brewmaster::BONEDUST_BREW;
    constexpr uint32 EXPLODING_KEG            = Brewmaster::EXPLODING_KEG;

    // Utility
    constexpr uint32 TRANSCENDENCE_BREW       = TRANSCENDENCE;
    constexpr uint32 TRANSCENDENCE_TRANSFER_BREW = TRANSCENDENCE_TRANSFER;
    constexpr uint32 ROLL_BREW                = ROLL;
    constexpr uint32 TIGER_LUST               = TIGERS_LUST;
    constexpr uint32 DETOX_BREW               = DETOX;

    // Defensive Cooldowns
    constexpr uint32 DAMPEN_HARM_BREW         = DAMPEN_HARM;
    constexpr uint32 ZEN_MEDITATION_BREW      = ZEN_MEDITATION;
    constexpr uint32 DIFFUSE_MAGIC_BREW       = DIFFUSE_MAGIC;

    // Passive/Procs
    constexpr uint32 ELUSIVE_BRAWLER          = Brewmaster::ELUSIVE_BRAWLER;
    constexpr uint32 GIFT_OF_THE_OX           = Brewmaster::GIFT_OF_THE_OX;
    constexpr uint32 COUNTERSTRIKE            = Brewmaster::COUNTERSTRIKE;
    constexpr uint32 BLACK_OX_BREW            = Brewmaster::BLACK_OX_BREW;
    constexpr uint32 CHARRED_PASSIONS         = Brewmaster::CHARRED_PASSIONS;

    // Hero Talents
    constexpr uint32 ASPECT_OF_HARMONY        = Brewmaster::ASPECT_OF_HARMONY;
    constexpr uint32 FLURRY_STRIKES           = Brewmaster::FLURRY_STRIKES;
}
using namespace BrewmasterMonkSpells;

// Dual resource type for Monk
struct EnergyChiResource
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
        // CRITICAL: NEVER call GetPower/GetMaxPower during construction!
        // Even with IsInWorld() check, power data may not be initialized during bot login.
        // Use static defaults here; refresh from player data in UpdateRotation later.
        maxEnergy = 100;  // Standard Monk max energy - will be refreshed when player data is ready
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
// BREWMASTER STAGGER TRACKER
// ============================================================================

class BrewmasterStaggerTracker
{
public:
    BrewmasterStaggerTracker()
        : _staggerAmount(0)
        , _staggerPercent(0.0f)
        , _lastStaggerCheck(0)
    {
    }

    void UpdateStagger(Player* bot)
    {
        if (!bot)
            return;

        uint32 now = GameTime::GetGameTimeMS();
        if (now - _lastStaggerCheck < 100) // Throttle checks
            return;

        _lastStaggerCheck = now;

        // Check for Stagger aura
        if (Aura* stagger = bot->GetAura(124255)) // Light Stagger
        {
            _staggerAmount = stagger->GetEffect(0)->GetAmount();
            _staggerPercent = (_staggerAmount * 100.0f) / bot->GetMaxHealth();
        }
        else if (Aura* stagger = bot->GetAura(124274)) // Moderate Stagger
        {
            _staggerAmount = stagger->GetEffect(0)->GetAmount();
            _staggerPercent = (_staggerAmount * 100.0f) / bot->GetMaxHealth();
        }
        else if (Aura* stagger = bot->GetAura(124273)) // Heavy Stagger
        {
            _staggerAmount = stagger->GetEffect(0)->GetAmount();
            _staggerPercent = (_staggerAmount * 100.0f) / bot->GetMaxHealth();
        }
        else
        {
            _staggerAmount = 0;
            _staggerPercent = 0.0f;
        }
    }

    uint32 GetStaggerAmount() const { return _staggerAmount; }
    float GetStaggerPercent() const { return _staggerPercent; }

    bool IsHeavyStagger() const { return _staggerPercent > 6.0f; }
    bool IsModerateStagger() const { return _staggerPercent > 3.0f; }
    bool ShouldPurify() const { return _staggerPercent > 4.0f; } // Purify at 4%+ of max HP

private:
    CooldownManager _cooldowns;
    uint32 _staggerAmount;
    float _staggerPercent;
    uint32 _lastStaggerCheck;
};

// ============================================================================
// BREWMASTER SHUFFLE TRACKER
// ============================================================================

class BrewmasterShuffleTracker
{
public:
    BrewmasterShuffleTracker()
        : _shuffleActive(false)
        , _shuffleEndTime(0)
    {
    }

    void ApplyShuffle()
    {
        _shuffleActive = true;
        _shuffleEndTime = GameTime::GetGameTimeMS() + 5000; // 5 sec base duration
    }

    void ExtendShuffle(uint32 durationMs)
    {
        if (_shuffleActive)
            _shuffleEndTime += durationMs;
        else
        {
            _shuffleActive = true;
            _shuffleEndTime = GameTime::GetGameTimeMS() + durationMs;
        }
    }

    bool IsActive() const { return _shuffleActive; }

    uint32 GetTimeRemaining() const
    {
        if (!_shuffleActive)
            return 0;

        uint32 now = GameTime::GetGameTimeMS();
        return _shuffleEndTime > now ? _shuffleEndTime - now : 0;
    }

    bool NeedsRefresh() const
    {
        // Refresh if less than 2 seconds remaining
        return !_shuffleActive || GetTimeRemaining() < 2000;
    }

    void Update()
    {
        if (_shuffleActive && GameTime::GetGameTimeMS() >= _shuffleEndTime)
        {
            _shuffleActive = false;
            _shuffleEndTime = 0;
        }
    }

private:
    bool _shuffleActive;
    uint32 _shuffleEndTime;
};

// ============================================================================
// BREWMASTER MONK REFACTORED
// ============================================================================

class BrewmasterMonkRefactored : public TankSpecialization<EnergyChiResource>
{
public:
    using Base = TankSpecialization<EnergyChiResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    explicit BrewmasterMonkRefactored(Player* bot)
        : TankSpecialization<EnergyChiResource>(bot)
        , _staggerTracker()
        , _shuffleTracker()
        , _ironskinBrewActive(false)
        , _ironskinEndTime(0)
        , _lastKegSmashTime(0)
    {
        // Initialize energy/chi resources
        this->_resource.Initialize(bot);

        // Phase 5: Initialize decision systems
        InitializeBrewmasterMechanics();

        // Note: Do NOT call bot->GetName() here - Player data may not be loaded yet
        TC_LOG_DEBUG("playerbot", "BrewmasterMonkRefactored created for bot GUID: {}",
            bot ? bot->GetGUID().GetCounter() : 0);
    }

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !target->IsAlive() || !target->IsHostileTo(this->GetBot()))
            return;

        // Lazy hero talent detection (once per bot lifetime until respec)
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Update Brewmaster state
        UpdateBrewmasterState();

        // Handle active mitigation first
        HandleActiveMitigation();

        // Hero talent tree rotation branching
        // Brewmaster has access to: Master of Harmony / Shado-Pan
        if (_heroTalents.IsTree(HeroTalentTree::MASTER_OF_HARMONY))
        {
            // Master of Harmony: Aspect of Harmony accumulates vitality from heals/damage
            // Cast Aspect of Harmony to release accumulated vitality as burst healing/damage
            if (this->CanCastSpell(ASPECT_OF_HARMONY, this->GetBot()))
            {
                // Use when stagger is moderate+ for defensive value, or during Weapons of Order burst
                if (_staggerTracker.IsModerateStagger() || this->GetBot()->HasAura(WEAPONS_OF_ORDER))
                {
                    this->CastSpell(ASPECT_OF_HARMONY, this->GetBot());
                    return;
                }
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::SHADO_PAN))
        {
            // Shado-Pan: Flurry Strikes proc from using different abilities (combo strikes)
            // Prioritize Flurry Strikes when the buff is active for bonus damage/threat
            if (this->GetBot()->HasAura(FLURRY_STRIKES))
            {
                // Flurry Strikes active - maximize different ability usage for more procs
                // Keg Smash is highest priority during Flurry Strikes for AoE threat
                if (this->_resource.energy >= 40 && this->CanCastSpell(KEG_SMASH, target))
                {
                    this->CastSpell(KEG_SMASH, target);
                    _lastKegSmashTime = GameTime::GetGameTimeMS();
                    GenerateChi(2);
                    return;
                }
            }
        }

        // Determine if AoE or single target
        uint32 enemyCount = this->GetEnemiesInRange(8.0f);
        if (enemyCount >= 3)
        {
            ExecuteAoEThreatRotation(target, enemyCount);
        }
        else
        {
            ExecuteSingleTargetThreatRotation(target);
        }
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // Emergency defensives
        HandleEmergencyDefensives();
    }

protected:
    void ExecuteSingleTargetThreatRotation(::Unit* target)
    {
        uint32 energy = this->_resource.energy;
        uint32 chi = this->_resource.chi;

        // Priority 1: Maintain Shuffle
        if (_shuffleTracker.NeedsRefresh() && chi >= 1)
        {
            if (this->CanCastSpell(BLACKOUT_KICK_BREW, target))
            {
                this->CastSpell(BLACKOUT_KICK_BREW, target);
                _shuffleTracker.ApplyShuffle();
                ConsumeChi(1);
                return;
            }
        }

        // Priority 2: Keg Smash on cooldown (best Chi generator + threat)
        if (energy >= 40 && chi < 5 && this->CanCastSpell(KEG_SMASH, target))
        {
            this->CastSpell(KEG_SMASH, target);
            _lastKegSmashTime = GameTime::GetGameTimeMS();
            GenerateChi(2);
            return;
        }

        // Priority 3: Breath of Fire (after Keg Smash for ignite)
        if (chi >= 2 && GameTime::GetGameTimeMS() - _lastKegSmashTime < 2000)
        {
            if (this->CanCastSpell(BREATH_OF_FIRE, target))
            {
                this->CastSpell(BREATH_OF_FIRE, target);
                ConsumeChi(2);
                return;
            }
        }

        // Priority 4: Rising Sun Kick for threat
        if (chi >= 2 && this->CanCastSpell(RISING_SUN_KICK_BREW, target))
        {
            this->CastSpell(RISING_SUN_KICK_BREW, target);
            ConsumeChi(2);
            return;
        }

        // Priority 5: Blackout Kick to spend Chi
        if (chi >= 3 && this->CanCastSpell(BLACKOUT_KICK_BREW, target))
        {
            this->CastSpell(BLACKOUT_KICK_BREW, target);
            _shuffleTracker.ExtendShuffle(5000);
            ConsumeChi(1);
            return;
        }

        // Priority 6: Tiger Palm for Chi generation
        if (energy >= 25 && chi < 5 && this->CanCastSpell(TIGER_PALM_BREW, target))
        {
            this->CastSpell(TIGER_PALM_BREW, target);
            GenerateChi(2);
            return;
        }

        // Priority 7: Expel Harm (heal + Chi)
        if (energy >= 15 && chi < 5 && this->GetBot()->GetHealthPct() < 90.0f)
        {
            if (this->CanCastSpell(EXPEL_HARM_BREW, this->GetBot()))
            {
                this->CastSpell(EXPEL_HARM_BREW, this->GetBot());
                GenerateChi(1);
                return;
            }
        }
    }

    void ExecuteAoEThreatRotation(::Unit* target, uint32 enemyCount)
    {
        uint32 energy = this->_resource.energy;
        uint32 chi = this->_resource.chi;

        // Priority 1: Maintain Shuffle
        if (_shuffleTracker.NeedsRefresh() && chi >= 1)
        {
            if (this->CanCastSpell(BLACKOUT_KICK_BREW, target))
            {
                this->CastSpell(BLACKOUT_KICK_BREW, target);
                _shuffleTracker.ApplyShuffle();
                ConsumeChi(1);
                return;
            }
        }

        // Priority 2: Keg Smash (AoE Chi generator)
        if (energy >= 40 && chi < 5 && this->CanCastSpell(KEG_SMASH, target))
        {
            this->CastSpell(KEG_SMASH, target);
            _lastKegSmashTime = GameTime::GetGameTimeMS();
            GenerateChi(2);
            return;
        }

        // Priority 3: Breath of Fire (AoE + DoT)
        if (chi >= 2 && this->CanCastSpell(BREATH_OF_FIRE, target))
        {
            this->CastSpell(BREATH_OF_FIRE, target);
            ConsumeChi(2);
            return;
        }

        // Priority 4: Spinning Crane Kick (AoE Chi spender)
        if (chi >= 2 && enemyCount >= 4 && this->CanCastSpell(SPINNING_CRANE_KICK_BREW, this->GetBot()))
        {
            this->CastSpell(SPINNING_CRANE_KICK_BREW, this->GetBot());
            ConsumeChi(2);
            return;
        }

        // Priority 5: Tiger Palm for Chi generation
        if (energy >= 25 && chi < 5 && this->CanCastSpell(TIGER_PALM_BREW, target))
        {
            this->CastSpell(TIGER_PALM_BREW, target);
            GenerateChi(2);
            return;
        }
    }

    void HandleActiveMitigation()
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // Update Ironskin Brew status
        UpdateIronskinBrew();

        // Priority 1: Purify heavy Stagger
        if (_staggerTracker.ShouldPurify() && this->CanCastSpell(PURIFYING_BREW, bot))
        {
            this->CastSpell(PURIFYING_BREW, bot);
            TC_LOG_DEBUG("playerbot", "Brewmaster: Purifying Brew used - Stagger at {:.1f}%", _staggerTracker.GetStaggerPercent());
            return;
        }

        // Priority 2: Maintain Ironskin Brew
        if (!_ironskinBrewActive || GetIronskinTimeRemaining() < 3000)
        {
            if (this->CanCastSpell(IRONSKIN_BREW, bot))
            {
                this->CastSpell(IRONSKIN_BREW, bot);
                _ironskinBrewActive = true;
                _ironskinEndTime = GameTime::GetGameTimeMS() + 7000; // 7 sec duration
                

        // Register cooldowns using CooldownManager
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT:             {KEG_SMASH, CooldownPresets::DISPEL, 1},
        // COMMENTED OUT:             {PROVOKE, CooldownPresets::DISPEL, 1},
        // COMMENTED OUT:             {PURIFYING_BREW, 20000, 1},
        // COMMENTED OUT:             {CELESTIAL_BREW, CooldownPresets::OFFENSIVE_60, 1},
        // COMMENTED OUT:             {FORTIFYING_BREW_BREW, 360000, 1},
        // COMMENTED OUT:             {DAMPEN_HARM, CooldownPresets::MINOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {ZEN_MEDITATION, 300000, 1},
        // COMMENTED OUT:             {INVOKE_NIUZAO, CooldownPresets::MAJOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {WEAPONS_OF_ORDER, CooldownPresets::MINOR_OFFENSIVE, 1},
        // COMMENTED OUT:             {BONEDUST_BREW, CooldownPresets::OFFENSIVE_60, 1},
        // COMMENTED OUT:         });

        TC_LOG_DEBUG("playerbot", "Brewmaster: Ironskin Brew applied");
                return;
            }
        }
    }

    void HandleEmergencyDefensives()
    {
        Player* bot = this->GetBot();
        float healthPct = bot->GetHealthPct();

        // Critical: Fortifying Brew
        if (healthPct < 25.0f && this->CanCastSpell(FORTIFYING_BREW_BREW, bot))
        {
            this->CastSpell(FORTIFYING_BREW_BREW, bot);
            TC_LOG_DEBUG("playerbot", "Brewmaster: Fortifying Brew emergency");
            return;
        }

        // Very low: Celestial Brew (absorb shield)
        if (healthPct < 40.0f && this->CanCastSpell(CELESTIAL_BREW, bot))
        {
            this->CastSpell(CELESTIAL_BREW, bot);
            TC_LOG_DEBUG("playerbot", "Brewmaster: Celestial Brew shield");
            return;
        }

        // Low: Zen Meditation (channeled DR)
        if (healthPct < 30.0f && this->CanCastSpell(ZEN_MEDITATION, bot))
        {
            this->CastSpell(ZEN_MEDITATION, bot);
            TC_LOG_DEBUG("playerbot", "Brewmaster: Zen Meditation");
            return;
        }

        // Moderate: Dampen Harm
        if (healthPct < 50.0f && this->CanCastSpell(DAMPEN_HARM, bot))
        {
            this->CastSpell(DAMPEN_HARM, bot);
            TC_LOG_DEBUG("playerbot", "Brewmaster: Dampen Harm");
            return;
        }
    }

private:
    void UpdateBrewmasterState()
    {
        // Update Stagger tracker
        _staggerTracker.UpdateStagger(this->GetBot());

        // Update Shuffle tracker
        _shuffleTracker.Update();

        // Update Ironskin Brew
        if (_ironskinBrewActive && GameTime::GetGameTimeMS() >= _ironskinEndTime)
        {
            _ironskinBrewActive = false;
            _ironskinEndTime = 0;
        }

        // Update Chi from bot
        if (this->GetBot())
        {
            this->_resource.chi = this->GetBot()->GetPower(POWER_CHI);
            this->_resource.energy = this->GetBot()->GetPower(POWER_ENERGY);
        }
    }

    void UpdateIronskinBrew()
    {
        if (this->GetBot() && this->GetBot()->HasAura(IRONSKIN_BREW))
        {
            _ironskinBrewActive = true;
        }
        else
        {
            _ironskinBrewActive = false;
            _ironskinEndTime = 0;
        }
    }

    uint32 GetIronskinTimeRemaining() const
    {
        if (!_ironskinBrewActive)
            return 0;

        uint32 now = GameTime::GetGameTimeMS();
        return _ironskinEndTime > now ? _ironskinEndTime - now : 0;
    }

    void GenerateChi(uint32 amount)
    {
        this->_resource.chi = ::std::min(this->_resource.chi + amount, this->_resource.maxChi);
    }

    void ConsumeChi(uint32 amount)
    {
        this->_resource.chi = (this->_resource.chi > amount) ? this->_resource.chi - amount : 0;
    }

    // ========================================================================
    // PHASE 5: DECISION SYSTEM INTEGRATION
    // ========================================================================

    void InitializeBrewmasterMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;
        if (!ai) return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Major defensive cooldowns
            queue->RegisterSpell(ZEN_MEDITATION, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(ZEN_MEDITATION, [](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 20.0f;
            }, "HP < 20% (channel 60% DR)");

            queue->RegisterSpell(FORTIFYING_BREW_BREW, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(FORTIFYING_BREW_BREW, [](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 35.0f;
            }, "HP < 35% (20% DR + 20% HP, 6min CD)");

            // CRITICAL: Active mitigation
            queue->RegisterSpell(PURIFYING_BREW, SpellPriority::CRITICAL, SpellCategory::DEFENSIVE);
            queue->AddCondition(PURIFYING_BREW, [this](Player*, Unit*) {
                return this->_staggerTracker.ShouldPurify();
            }, "Stagger > 4% max HP (clear stagger)");

            queue->RegisterSpell(CELESTIAL_BREW, SpellPriority::CRITICAL, SpellCategory::DEFENSIVE);
            queue->AddCondition(CELESTIAL_BREW, [](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 60.0f;
            }, "HP < 60% (absorb shield, 1min CD)");

            queue->RegisterSpell(IRONSKIN_BREW, SpellPriority::CRITICAL, SpellCategory::DEFENSIVE);
            queue->AddCondition(IRONSKIN_BREW, [this](Player*, Unit*) {
                return !this->_ironskinBrewActive || this->GetIronskinTimeRemaining() < 3000;
            }, "Ironskin down or < 3s (increases stagger)");

            // HIGH: Shuffle maintenance + threat
            queue->RegisterSpell(BLACKOUT_KICK_BREW, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(BLACKOUT_KICK_BREW, [this](Player*, Unit* target) {
                return target && this->_resource.chi >= 1 && this->_shuffleTracker.NeedsRefresh();
            }, "1 chi, Shuffle < 2s (maintain buff)");

            queue->RegisterSpell(KEG_SMASH, SpellPriority::HIGH, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(KEG_SMASH, [this](Player*, Unit* target) {
                return target && this->_resource.energy >= 40 && this->_resource.chi < 5;
            }, "40 energy, chi < 5 (generates 2 chi + threat)");

            queue->RegisterSpell(PROVOKE, SpellPriority::HIGH, SpellCategory::UTILITY);
            queue->AddCondition(PROVOKE, [](Player*, Unit* target) {
                return target != nullptr;
            }, "Taunt (ThreatAssistant determines need)");

            // MEDIUM: Chi spenders and threat
            queue->RegisterSpell(BREATH_OF_FIRE, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(BREATH_OF_FIRE, [this](Player*, Unit* target) {
                return target && this->_resource.chi >= 2 && (GameTime::GetGameTimeMS() - this->_lastKegSmashTime) < 2000;
            }, "2 chi, after Keg Smash (cone + DoT)");

            queue->RegisterSpell(SPINNING_CRANE_KICK_BREW, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(SPINNING_CRANE_KICK_BREW, [this](Player*, Unit*) {
                return this->_resource.chi >= 2 && this->GetEnemiesInRange(8.0f) >= 3;
            }, "2 chi, 3+ enemies (AoE channel)");

            queue->RegisterSpell(RISING_SUN_KICK_BREW, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(RISING_SUN_KICK_BREW, [this](Player*, Unit* target) {
                return target && this->_resource.chi >= 2;
            }, "2 chi (high threat)");

            // LOW: Chi generators
            queue->RegisterSpell(TIGER_PALM_BREW, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(TIGER_PALM_BREW, [this](Player*, Unit* target) {
                return target && this->_resource.energy >= 25 && this->_resource.chi < 5;
            }, "25 energy, chi < 5 (generates 2 chi)");

            queue->RegisterSpell(EXPEL_HARM_BREW, SpellPriority::LOW, SpellCategory::HEALING);
            queue->AddCondition(EXPEL_HARM_BREW, [this](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 90.0f && this->_resource.energy >= 15 && this->_resource.chi < 5;
            }, "HP < 90%, 15 energy (heal + 1 chi)");

            // UTILITY: Major cooldowns
            queue->RegisterSpell(INVOKE_NIUZAO, SpellPriority::HIGH, SpellCategory::OFFENSIVE);
            queue->AddCondition(INVOKE_NIUZAO, [this](Player* bot, Unit* target) {
                return bot && target && bot->HasSpell(INVOKE_NIUZAO) && this->GetEnemiesInRange(10.0f) >= 2;
            }, "2+ enemies (summon statue, 3min CD)");

            queue->RegisterSpell(WEAPONS_OF_ORDER, SpellPriority::HIGH, SpellCategory::OFFENSIVE);
            queue->AddCondition(WEAPONS_OF_ORDER, [this](Player* bot, Unit* target) {
                return bot && target && bot->HasSpell(WEAPONS_OF_ORDER);
            }, "Burst window (damage/defense, 2min CD)");

            queue->RegisterSpell(DAMPEN_HARM, SpellPriority::HIGH, SpellCategory::DEFENSIVE);
            queue->AddCondition(DAMPEN_HARM, [](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 50.0f;
            }, "HP < 50% (damage reduction, 2min CD)");
        }

        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Brewmaster Tank", {
                // Tier 1: Emergency Defensives
                Sequence("Emergency Defense", {
                    Condition("Critical HP", [](Player* bot, Unit* target) {
                        return bot && bot->GetHealthPct() < 35.0f;
                    }),
                    Selector("Use emergency", {
                        Sequence("Zen Meditation", {
                            Condition("HP < 20%", [](Player* bot, Unit* target) {
                                return bot->GetHealthPct() < 20.0f;
                            }),
                            bot::ai::Action("Cast Zen Meditation", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(ZEN_MEDITATION, bot))
                                {
                                    this->CastSpell(ZEN_MEDITATION, bot);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Fortifying Brew", {
                            bot::ai::Action("Cast Fortifying Brew", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(FORTIFYING_BREW_BREW, bot))
                                {
                                    this->CastSpell(FORTIFYING_BREW_BREW, bot);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 2: Active Mitigation (Stagger Management)
                Sequence("Stagger Management", {
                    Selector("Manage stagger", {
                        Sequence("Purifying Brew", {
                            Condition("Should purify", [this](Player*, Unit*) {
                                return this->_staggerTracker.ShouldPurify();
                            }),
                            bot::ai::Action("Cast Purifying Brew", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(PURIFYING_BREW, bot))
                                {
                                    this->CastSpell(PURIFYING_BREW, bot);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Celestial Brew", {
                            Condition("HP < 60%", [](Player* bot, Unit* target) {
                                return bot && bot->GetHealthPct() < 60.0f;
                            }),
                            bot::ai::Action("Cast Celestial Brew", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(CELESTIAL_BREW, bot))
                                {
                                    this->CastSpell(CELESTIAL_BREW, bot);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Ironskin Brew", {
                            Condition("Needs refresh", [this](Player*, Unit*) {
                                return !this->_ironskinBrewActive || this->GetIronskinTimeRemaining() < 3000;
                            }),
                            bot::ai::Action("Cast Ironskin Brew", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(IRONSKIN_BREW, bot))
                                {
                                    this->CastSpell(IRONSKIN_BREW, bot);
                                    this->_ironskinBrewActive = true;
                                    this->_ironskinEndTime = GameTime::GetGameTimeMS() + 7000;
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 3: Shuffle Maintenance
                Sequence("Maintain Shuffle", {
                    Condition("Has target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim();
                    }),
                    Condition("Shuffle needs refresh", [this](Player*, Unit*) {
                        return this->_shuffleTracker.NeedsRefresh();
                    }),
                    Condition("Has chi", [this](Player*, Unit*) {
                        return this->_resource.chi >= 1;
                    }),
                    bot::ai::Action("Cast Blackout Kick", [this](Player* bot, Unit*) {
                        Unit* target = bot->GetVictim();
                        if (target && this->CanCastSpell(BLACKOUT_KICK_BREW, target))
                        {
                            this->CastSpell(BLACKOUT_KICK_BREW, target);
                            this->_shuffleTracker.ApplyShuffle();
                            this->ConsumeChi(1);
                            return NodeStatus::SUCCESS;
                        }
                        return NodeStatus::FAILURE;
                    })
                }),

                // Tier 4: Chi Generation
                Sequence("Generate Chi", {
                    Condition("Has target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim();
                    }),
                    Condition("Chi < 5", [this](Player*, Unit*) {
                        return this->_resource.chi < 5;
                    }),
                    Selector("Generate", {
                        Sequence("Keg Smash", {
                            Condition("40 energy", [this](Player*, Unit*) {
                                return this->_resource.energy >= 40;
                            }),
                            bot::ai::Action("Cast Keg Smash", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(KEG_SMASH, target))
                                {
                                    this->CastSpell(KEG_SMASH, target);
                                    this->_lastKegSmashTime = GameTime::GetGameTimeMS();
                                    this->GenerateChi(2);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Tiger Palm", {
                            Condition("25 energy", [this](Player*, Unit*) {
                                return this->_resource.energy >= 25;
                            }),
                            bot::ai::Action("Cast Tiger Palm", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(TIGER_PALM_BREW, target))
                                {
                                    this->CastSpell(TIGER_PALM_BREW, target);
                                    this->GenerateChi(2);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Expel Harm", {
                            Condition("15 energy + low HP", [this](Player* bot, Unit*) {
                                return bot && this->_resource.energy >= 15 && bot->GetHealthPct() < 90.0f;
                            }),
                            bot::ai::Action("Cast Expel Harm", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(EXPEL_HARM_BREW, bot))
                                {
                                    this->CastSpell(EXPEL_HARM_BREW, bot);
                                    this->GenerateChi(1);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 5: Chi Spenders (Threat + Damage)
                Sequence("Spend Chi", {
                    Condition("Has target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim();
                    }),
                    Condition("Has chi", [this](Player*, Unit*) {
                        return this->_resource.chi >= 2;
                    }),
                    Selector("Spend", {
                        Sequence("Breath of Fire", {
                            Condition("After Keg Smash", [this](Player*, Unit*) {
                                return (GameTime::GetGameTimeMS() - this->_lastKegSmashTime) < 2000;
                            }),
                            bot::ai::Action("Cast BoF", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(BREATH_OF_FIRE, target))
                                {
                                    this->CastSpell(BREATH_OF_FIRE, target);
                                    this->ConsumeChi(2);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Spinning Crane Kick", {
                            Condition("3+ enemies", [this](Player*, Unit*) {
                                return this->GetEnemiesInRange(8.0f) >= 3;
                            }),
                            bot::ai::Action("Cast SCK", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(SPINNING_CRANE_KICK_BREW, bot))
                                {
                                    this->CastSpell(SPINNING_CRANE_KICK_BREW, bot);
                                    this->ConsumeChi(2);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Rising Sun Kick", {
                            bot::ai::Action("Cast RSK", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(RISING_SUN_KICK_BREW, target))
                                {
                                    this->CastSpell(RISING_SUN_KICK_BREW, target);
                                    this->ConsumeChi(2);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                })
            });

            behaviorTree->SetRoot(root);
        }
    }

private:
    BrewmasterStaggerTracker _staggerTracker;
    BrewmasterShuffleTracker _shuffleTracker;
    bool _ironskinBrewActive;
    uint32 _ironskinEndTime;
    uint32 _lastKegSmashTime;
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
