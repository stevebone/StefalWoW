/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Devastation Evoker Refactored - Enterprise-Grade Header-Only Implementation
 *
 * This file provides a complete, template-based implementation of Devastation Evoker
 * using the DPSSpecialization with Essence resource system and Empowerment mechanics.
 */

#pragma once

#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "../CombatSpecializationTemplates.h"
#include "../ResourceTypes.h"
#include "../SpellValidation_WoW120.h"
#include "../HeroTalentDetector.h"      // Hero talent tree detection
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
// DEVASTATION EVOKER SPELL IDs (WoW 12.0 - The War Within)
// See central registry: WoW120Spells::Evoker and WoW120Spells::Evoker::Devastation
// ============================================================================

enum DevastationEvokerSpells
{
    // Essence Generators
    AZURE_STRIKE         = WoW120Spells::Evoker::AZURE_STRIKE,
    LIVING_FLAME         = WoW120Spells::Evoker::LIVING_FLAME,

    // Essence Spenders (Empowered)
    FIRE_BREATH          = WoW120Spells::Evoker::FIRE_BREATH,
    ETERNITY_SURGE       = WoW120Spells::Evoker::Devastation::ETERNITY_SURGE,

    // Direct Damage
    DISINTEGRATE         = WoW120Spells::Evoker::DISINTEGRATE,
    PYRE                 = WoW120Spells::Evoker::Devastation::PYRE,
    SHATTERING_STAR      = WoW120Spells::Evoker::Devastation::SHATTERING_STAR,

    // Major Cooldowns
    DRAGONRAGE           = WoW120Spells::Evoker::Devastation::DRAGONRAGE,
    DEEP_BREATH          = WoW120Spells::Evoker::DEEP_BREATH,
    TIP_THE_SCALES       = WoW120Spells::Evoker::Devastation::TIP_THE_SCALES,

    // Procs and Buffs
    ESSENCE_BURST        = WoW120Spells::Evoker::Devastation::ESSENSE_BURST,
    BURNOUT              = WoW120Spells::Evoker::Devastation::BURNOUT,
    IRIDESCENCE_BLUE     = WoW120Spells::Evoker::Devastation::IRIDESCENCE_BLUE,
    IRIDESCENCE_RED      = WoW120Spells::Evoker::Devastation::IRIDESCENCE_RED,

    // Utility
    HOVER                = WoW120Spells::Evoker::HOVER,
    OBSIDIAN_SCALES      = WoW120Spells::Evoker::OBSIDIAN_SCALES,
    RENEWING_BLAZE       = WoW120Spells::Evoker::RENEWING_BLAZE,
    QUELL                = WoW120Spells::Evoker::QUELL,
    TAIL_SWIPE           = WoW120Spells::Evoker::TAIL_SWIPE,
    WING_BUFFET          = WoW120Spells::Evoker::WING_BUFFET,

    // Talents
    ANIMOSITY            = WoW120Spells::Evoker::Devastation::ANIMOSITY,
    CATALYZE             = WoW120Spells::Evoker::Devastation::CATALYZE,
    FEED_THE_FLAMES      = WoW120Spells::Evoker::Devastation::FEED_THE_FLAMES,
    ONYX_LEGACY          = WoW120Spells::Evoker::Devastation::ONYX_LEGACY
};

// Essence resource type for Devastation Evoker
// Distinct type to avoid template instantiation conflicts with Preservation/Augmentation
struct DevastationEssence
{
    uint32 essence{0};
    uint32 maxEssence{5};
    bool available{true};

    bool Consume(uint32 cost)
    {
        if (essence >= cost) {
            essence -= cost;
            return true;
        }
        return false;
    }

    void Regenerate(uint32 diff)
    {
        // Passive essence regeneration every 5 seconds
        available = true;
    }

    [[nodiscard]] uint32 GetAvailable() const { return essence; }
    [[nodiscard]] uint32 GetMax() const { return maxEssence; }

    void Initialize(Player* bot)
    {
        if (bot)
        {
            essence = 0;
            maxEssence = 5; // Devastation has 5 max essence
        }
    }
};

// ============================================================================
// EMPOWERMENT TRACKING
// ============================================================================

enum class EmpowerLevel : uint8
{
    NONE = 0,
    RANK_1 = 1,
    RANK_2 = 2,
    RANK_3 = 3,
    RANK_4 = 4
};

class DevastationEmpowermentTracker
{
public:
    DevastationEmpowermentTracker()
        : _isChanneling(false)
        , _currentSpellId(0)
        , _targetLevel(EmpowerLevel::NONE)
        , _channelStartTime(0)
    {}

    void StartEmpower(uint32 spellId, EmpowerLevel targetLevel)
    {
        _isChanneling = true;
        _currentSpellId = spellId;
        _targetLevel = targetLevel;
        _channelStartTime = GameTime::GetGameTimeMS();
    }

    void StopEmpower()
    {
        _isChanneling = false;
        _currentSpellId = 0;
        _targetLevel = EmpowerLevel::NONE;
        _channelStartTime = 0;
    }

    bool IsChanneling() const { return _isChanneling; }
    uint32 GetSpellId() const { return _currentSpellId; }

    uint32 GetChannelTime() const
    {
        if (!_isChanneling) return 0;
        return GameTime::GetGameTimeMS() - _channelStartTime;
    }

    bool ShouldRelease() const
    {
        if (!_isChanneling) return false;
        uint32 requiredTime = static_cast<uint32>(_targetLevel) * 750; // 0.75s per rank
        return GetChannelTime() >= requiredTime;
    }

    EmpowerLevel GetAchievedLevel() const
    {
        if (!_isChanneling) return EmpowerLevel::NONE;
        uint32 channelTime = GetChannelTime();

        if (channelTime >= 3000) return EmpowerLevel::RANK_4; // 3.0s+
        if (channelTime >= 2250) return EmpowerLevel::RANK_3; // 2.25s
        if (channelTime >= 1500) return EmpowerLevel::RANK_2; // 1.5s
        if (channelTime >= 750) return EmpowerLevel::RANK_1;  // 0.75s

        return EmpowerLevel::NONE;
    }

private:
    bool _isChanneling;
    uint32 _currentSpellId;
    EmpowerLevel _targetLevel;
    uint32 _channelStartTime;
};

// ============================================================================
// DRAGONRAGE TRACKER
// ============================================================================

class DragonrageTracker
{
public:
    DragonrageTracker()
        : _isActive(false)
        , _endTime(0)
    {}

    void Activate()
    {
        _isActive = true;
        _endTime = GameTime::GetGameTimeMS() + 18000; // 18 second duration
    }

    void Update()
    {
        if (_isActive && GameTime::GetGameTimeMS() >= _endTime)
        {
            _isActive = false;
            _endTime = 0;
        }
    }

    bool IsActive() const { return _isActive; }
    uint32 GetTimeRemaining() const
    {
        if (!_isActive) return 0;
        uint32 now = GameTime::GetGameTimeMS();
        return _endTime > now ? _endTime - now : 0;
    }

private:
    bool _isActive;
    uint32 _endTime;
};

// ============================================================================
// DEVASTATION EVOKER REFACTORED
// ============================================================================

class DevastationEvokerRefactored : public RangedDpsSpecialization<DevastationEssence>
{
public:
    using Base = RangedDpsSpecialization<DevastationEssence>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::GetEnemiesInRange;
    using Base::_resource;

    explicit DevastationEvokerRefactored(Player* bot)
        : RangedDpsSpecialization<DevastationEssence>(bot)
        , _empowermentTracker()
        , _dragonrageTracker()
        , _essenceBurstStacks(0)
        , _lastEternityTime(0)
        , _lastFireBreathTime(0)
    {
        // DevastationEssence::Initialize() is safe - only sets default values
        this->_resource.Initialize(bot);

        // Phase 5: Initialize decision systems
        InitializeDevastationMechanics();

        // Note: Do NOT call bot->GetName() here - Player data may not be loaded yet
        TC_LOG_DEBUG("playerbot", "DevastationEvokerRefactored created for bot GUID: {}",
            bot ? bot->GetGUID().GetCounter() : 0);
    }

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !target->IsAlive() || !target->IsHostileTo(this->GetBot()))
            return;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero talent rotation branches
        if (_heroTalents.IsTree(HeroTalentTree::FLAMESHAPER))
        {
            // Flameshaper: Engulf for empowered fire damage
            if (this->CanCastSpell(WoW120Spells::Evoker::Devastation::ENGULF, target))
            {
                this->CastSpell(WoW120Spells::Evoker::Devastation::ENGULF, target);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::SCALECOMMANDER))
        {
            // Scalecommander: Mass Disintegrate when 3+ enemies for cleave
            if (this->GetEnemiesInRange(25.0f) >= 3 &&
                this->CanCastSpell(WoW120Spells::Evoker::Devastation::MASS_DISINTEGRATE, target))
            {
                this->CastSpell(WoW120Spells::Evoker::Devastation::MASS_DISINTEGRATE, target);
                return;
            }
        }

        // Update Devastation state
        UpdateDevastationState();

        // Update empowerment channeling
        if (_empowermentTracker.IsChanneling())
        {
            if (_empowermentTracker.ShouldRelease())
            {
                // Release empowered spell at target rank
                ReleaseEmpoweredSpell();
            }
            return; // Don't cast other spells while channeling empower
        }

        // Determine rotation based on enemy count
        uint32 enemyCount = this->GetEnemiesInRange(25.0f);
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

        // Emergency defensives
        HandleEmergencyDefensives();
    }

    /// Check if Essence Burst proc is active (makes next essence spender free)
    [[nodiscard]] bool HasEssenceBurstProc() const { return _essenceBurstStacks > 0; }

    /// Consume one stack of Essence Burst (called after casting a free spender)
    void ConsumeEssenceBurst()
    {
        if (_essenceBurstStacks > 0)
            _essenceBurstStacks--;
    }

    /// Returns the effective essence cost considering Essence Burst proc
    [[nodiscard]] uint32 GetEffectiveEssenceCost(uint32 baseCost) const
    {
        return HasEssenceBurstProc() ? 0 : baseCost;
    }

protected:
    void ExecuteSingleTargetRotation(::Unit* target)
    {
        uint32 essence = this->_resource.essence;

        // Priority 1: Dragonrage burst window
        if (_dragonrageTracker.IsActive())
        {
            ExecuteDragonrageBurst(target);
            return;
        }

        // Priority 2: Shattering Star debuff
        if (this->CanCastSpell(SHATTERING_STAR, target))
        {
            this->CastSpell(SHATTERING_STAR, target);
            return;
        }

        // Priority 3: Consume Essence Burst proc on Disintegrate (highest priority spender when proc active)
        if (HasEssenceBurstProc() && this->CanCastSpell(DISINTEGRATE, target))
        {
            this->CastSpell(DISINTEGRATE, target);
            ConsumeEssenceBurst(); // Free cast - don't consume essence
            return;
        }

        // Priority 4: Eternity's Surge (empowered)
        if (essence >= 3 && this->CanCastSpell(ETERNITY_SURGE, target))
        {
            StartEmpoweredSpell(ETERNITY_SURGE, EmpowerLevel::RANK_3, target);
            return;
        }

        // Priority 5: Disintegrate channel (normal cost)
        if (essence >= 3 && this->CanCastSpell(DISINTEGRATE, target))
        {
            this->CastSpell(DISINTEGRATE, target);
            ConsumeEssence(3);
            return;
        }

        // Priority 6: Fire Breath (empowered)
        if (essence >= 3 && this->CanCastSpell(FIRE_BREATH, target))
        {
            StartEmpoweredSpell(FIRE_BREATH, EmpowerLevel::RANK_2, target);
            return;
        }

        // Priority 7: Azure Strike for essence
        if (essence < 4 && this->CanCastSpell(AZURE_STRIKE, target))
        {
            this->CastSpell(AZURE_STRIKE, target);
            GenerateEssence(2);
            return;
        }

        // Priority 8: Living Flame filler
        if (this->CanCastSpell(LIVING_FLAME, target))
        {
            this->CastSpell(LIVING_FLAME, target);
            GenerateEssence(1);
            return;
        }
    }

    void ExecuteAoERotation(::Unit* target, uint32 enemyCount)
    {
        uint32 essence = this->_resource.essence;

        // Priority 1: Consume Essence Burst on Pyre in AoE (free AoE spender)
        if (HasEssenceBurstProc() && this->CanCastSpell(PYRE, target))
        {
            this->CastSpell(PYRE, target);
            ConsumeEssenceBurst(); // Free cast
            return;
        }

        // Priority 2: Fire Breath AoE (empowered rank 4)
        if (essence >= 3 && this->CanCastSpell(FIRE_BREATH, target))
        {
            StartEmpoweredSpell(FIRE_BREATH, EmpowerLevel::RANK_4, target);
            return;
        }

        // Priority 3: Pyre AoE (normal cost)
        if (essence >= 2 && this->CanCastSpell(PYRE, target))
        {
            this->CastSpell(PYRE, target);
            ConsumeEssence(2);
            return;
        }

        // Priority 3: Shattering Star
        if (this->CanCastSpell(SHATTERING_STAR, target))
        {
            this->CastSpell(SHATTERING_STAR, target);
            return;
        }

        // Priority 4: Azure Strike for essence
        if (essence < 4 && this->CanCastSpell(AZURE_STRIKE, target))
        {
            this->CastSpell(AZURE_STRIKE, target);
            GenerateEssence(2);
            return;
        }

        // Priority 5: Living Flame filler
        if (this->CanCastSpell(LIVING_FLAME, target))
        {
            this->CastSpell(LIVING_FLAME, target);
            GenerateEssence(1);
            return;
        }
    }

    void ExecuteDragonrageBurst(::Unit* target)
    {
        uint32 essence = this->_resource.essence;

        // Priority 0: Consume Essence Burst during Dragonrage (extremely high value)
        if (HasEssenceBurstProc())
        {
            if (this->CanCastSpell(DISINTEGRATE, target))
            {
                this->CastSpell(DISINTEGRATE, target);
                ConsumeEssenceBurst();
                return;
            }
        }

        // Spam empowered spells during Dragonrage
        if (essence >= 3)
        {
            if (this->CanCastSpell(ETERNITY_SURGE, target))
            {
                StartEmpoweredSpell(ETERNITY_SURGE, EmpowerLevel::RANK_1, target); // Quick rank 1
                return;
            }

            if (this->CanCastSpell(FIRE_BREATH, target))
            {
                StartEmpoweredSpell(FIRE_BREATH, EmpowerLevel::RANK_1, target); // Quick rank 1
                return;
            }

            if (this->CanCastSpell(DISINTEGRATE, target))
            {
                this->CastSpell(DISINTEGRATE, target);
                ConsumeEssence(3);
                return;
            }
        }

        // Generate essence quickly
        if (essence < 3 && this->CanCastSpell(AZURE_STRIKE, target))
        {
            this->CastSpell(AZURE_STRIKE, target);
            GenerateEssence(2);
            return;
        }
    }

    void UpdateDevastationState()
    {
        _dragonrageTracker.Update();

        // Update Essence Burst stacks from aura
        Player* bot = this->GetBot();
        if (bot)
        {
            if (Aura* aura = bot->GetAura(ESSENCE_BURST))
                _essenceBurstStacks = aura->GetStackAmount();
            else
                _essenceBurstStacks = 0;

            // Sync essence with actual resource
            this->_resource.essence = bot->GetPower(POWER_ALTERNATE_POWER);
        }
    }

    void HandleEmergencyDefensives()
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        float healthPct = bot->GetHealthPct();

        // Obsidian Scales at 40% HP
        if (healthPct < 40.0f && this->CanCastSpell(OBSIDIAN_SCALES, bot))
        {
            this->CastSpell(OBSIDIAN_SCALES, bot);
            return;
        }

        // Renewing Blaze at 50% HP
        if (healthPct < 50.0f && this->CanCastSpell(RENEWING_BLAZE, bot))
        {
            this->CastSpell(RENEWING_BLAZE, bot);
            return;
        }
    }

    void StartEmpoweredSpell(uint32 spellId, EmpowerLevel targetLevel, ::Unit* target)
    {
        _empowermentTracker.StartEmpower(spellId, targetLevel);
        this->CastSpell(spellId, target); // Start the channel
    }

    void ReleaseEmpoweredSpell()
    {
        uint32 spellId = _empowermentTracker.GetSpellId();
        EmpowerLevel achievedLevel = _empowermentTracker.GetAchievedLevel();

        // Stop the channel (release at achieved rank)
        Player* bot = this->GetBot();
        if (bot && bot->IsNonMeleeSpellCast(false))
        {
            bot->InterruptNonMeleeSpells(false);
        }

        ConsumeEssence(3); // All empowered spells cost 3 essence

        _empowermentTracker.StopEmpower();

        TC_LOG_DEBUG("playerbot", "DevastationEvoker {} released {} at rank {}",
                     bot->GetName(), spellId, static_cast<uint32>(achievedLevel));
    }

    void GenerateEssence(uint32 amount)
    {
        this->_resource.essence = ::std::min<uint32>(this->_resource.essence + amount, this->_resource.maxEssence);
    }

    void ConsumeEssence(uint32 amount)
    {
        this->_resource.essence = (this->_resource.essence > amount) ? this->_resource.essence - amount : 0;
    }

    // ========================================================================
    // PHASE 5: DECISION SYSTEM INTEGRATION
    // ========================================================================

    void InitializeDevastationMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)

        auto* queue = this->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Defensive cooldowns
            queue->RegisterSpell(OBSIDIAN_SCALES, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(OBSIDIAN_SCALES, [](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 40.0f;
            }, "HP < 40% (30% dmg reduction, 90s CD)");

            queue->RegisterSpell(RENEWING_BLAZE, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(RENEWING_BLAZE, [](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 50.0f;
            }, "HP < 50% (self-heal, 90s CD)");

            // CRITICAL: Major burst cooldowns
            queue->RegisterSpell(DRAGONRAGE, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(DRAGONRAGE, [this](Player*, Unit* target) {
                return target && this->_resource.essence >= 3 && !this->_dragonrageTracker.IsActive();
            }, "3+ essence, not active (18s burst, 2min CD)");

            queue->RegisterSpell(DEEP_BREATH, SpellPriority::CRITICAL, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(DEEP_BREATH, [this](Player*, Unit* target) {
                return target && this->GetEnemiesInRange(25.0f) >= 3;
            }, "3+ enemies (flying breath, 2min CD)");

            // HIGH: Core rotation spells
            queue->RegisterSpell(SHATTERING_STAR, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SHATTERING_STAR, [](Player*, Unit* target) {
                return target != nullptr;
            }, "Debuff target (20s CD)");

            queue->RegisterSpell(ETERNITY_SURGE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(ETERNITY_SURGE, [this](Player*, Unit* target) {
                return target && this->_resource.essence >= 3 && !this->_empowermentTracker.IsChanneling();
            }, "3 essence (empowered, high ST damage)");

            queue->RegisterSpell(DISINTEGRATE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(DISINTEGRATE, [this](Player*, Unit* target) {
                return target && this->_resource.essence >= 3;
            }, "3 essence (channel, high damage)");

            // MEDIUM: AoE and secondary spenders
            queue->RegisterSpell(FIRE_BREATH, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(FIRE_BREATH, [this](Player*, Unit* target) {
                return target && this->_resource.essence >= 3 && !this->_empowermentTracker.IsChanneling();
            }, "3 essence (empowered, AoE DoT)");

            queue->RegisterSpell(PYRE, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(PYRE, [this](Player*, Unit* target) {
                return target && this->_resource.essence >= 2 && this->GetEnemiesInRange(10.0f) >= 3;
            }, "2 essence, 3+ enemies (cone AoE)");

            // LOW: Essence generators
            queue->RegisterSpell(AZURE_STRIKE, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(AZURE_STRIKE, [this](Player*, Unit* target) {
                return target && this->_resource.essence < 4;
            }, "Essence < 4 (generates 2 essence)");

            queue->RegisterSpell(LIVING_FLAME, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(LIVING_FLAME, [this](Player*, Unit* target) {
                return target && this->_resource.essence < 5;
            }, "Essence < 5 (generates 1 essence)");

            // UTILITY: Interrupt and movement
            queue->RegisterSpell(QUELL, SpellPriority::HIGH, SpellCategory::CROWD_CONTROL);
            queue->AddCondition(QUELL, [](Player*, Unit* target) {
                return target && target->IsNonMeleeSpellCast(false);
            }, "Target casting (interrupt, 40s CD)");

            queue->RegisterSpell(HOVER, SpellPriority::MEDIUM, SpellCategory::UTILITY);
            queue->AddCondition(HOVER, [](Player* bot, Unit* target) {
                return bot && target && bot->GetDistance(target) < 15.0f;
            }, "< 15yd range (hover mode, reposition)");
        }

        auto* behaviorTree = this->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Devastation Evoker DPS", {
                // Tier 1: Emergency Defense
                Sequence("Emergency Defense", {
                    Condition("Low HP", [](Player* bot, Unit* target) {
                        return bot && bot->GetHealthPct() < 50.0f;
                    }),
                    Selector("Use defensive", {
                        Sequence("Obsidian Scales", {
                            Condition("< 40%", [](Player* bot, Unit* target) {
                                return bot->GetHealthPct() < 40.0f;
                            }),
                            bot::ai::Action("Cast Obsidian Scales", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(OBSIDIAN_SCALES, bot))
                                {
                                    this->CastSpell(OBSIDIAN_SCALES, bot);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Renewing Blaze", {
                            bot::ai::Action("Cast Renewing Blaze", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(RENEWING_BLAZE, bot))
                                {
                                    this->CastSpell(RENEWING_BLAZE, bot);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 2: Burst Cooldowns
                Sequence("Burst Phase", {
                    Condition("Has target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim();
                    }),
                    Condition("3+ essence", [this](Player*, Unit*) {
                        return this->_resource.essence >= 3;
                    }),
                    Selector("Use cooldowns", {
                        Sequence("Dragonrage", {
                            Condition("Not active", [this](Player*, Unit*) {
                                return !this->_dragonrageTracker.IsActive();
                            }),
                            bot::ai::Action("Cast Dragonrage", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(DRAGONRAGE, bot))
                                {
                                    this->CastSpell(DRAGONRAGE, bot);
                                    this->_dragonrageTracker.Activate();
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 3: Core Rotation
                Sequence("Core Rotation", {
                    Condition("Has target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim();
                    }),
                    Condition("Not channeling", [this](Player*, Unit*) {
                        return !this->_empowermentTracker.IsChanneling();
                    }),
                    Selector("Cast spells", {
                        Sequence("Shattering Star", {
                            bot::ai::Action("Cast Shattering Star", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(SHATTERING_STAR, target))
                                {
                                    this->CastSpell(SHATTERING_STAR, target);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Eternity's Surge", {
                            Condition("3+ essence", [this](Player*, Unit*) {
                                return this->_resource.essence >= 3;
                            }),
                            bot::ai::Action("Cast Eternity's Surge", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(ETERNITY_SURGE, target))
                                {
                                    this->StartEmpoweredSpell(ETERNITY_SURGE, EmpowerLevel::RANK_3, target);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Disintegrate", {
                            Condition("3+ essence", [this](Player*, Unit*) {
                                return this->_resource.essence >= 3;
                            }),
                            bot::ai::Action("Cast Disintegrate", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(DISINTEGRATE, target))
                                {
                                    this->CastSpell(DISINTEGRATE, target);
                                    this->ConsumeEssence(3);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 4: Essence Generation
                Sequence("Generate Essence", {
                    Condition("Has target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim();
                    }),
                    Condition("< 4 essence", [this](Player*, Unit*) {
                        return this->_resource.essence < 4;
                    }),
                    Selector("Generate", {
                        Sequence("Azure Strike", {
                            bot::ai::Action("Cast Azure Strike", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(AZURE_STRIKE, target))
                                {
                                    this->CastSpell(AZURE_STRIKE, target);
                                    this->GenerateEssence(2);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Living Flame", {
                            bot::ai::Action("Cast Living Flame", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(LIVING_FLAME, target))
                                {
                                    this->CastSpell(LIVING_FLAME, target);
                                    this->GenerateEssence(1);
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
    DevastationEmpowermentTracker _empowermentTracker;
    DragonrageTracker _dragonrageTracker;
    uint32 _essenceBurstStacks;
    uint32 _lastEternityTime;
    uint32 _lastFireBreathTime;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
