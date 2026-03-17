/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Arms Warrior Specialization - REFACTORED
 *
 * This demonstrates the complete migration of Arms Warrior to the template-based
 * architecture, eliminating code duplication while maintaining full functionality.
 *
 * BEFORE: 620 lines of code with duplicate UpdateCooldowns, CanUseAbility, etc.
 * AFTER: ~250 lines focusing ONLY on Arms-specific logic
 */

#pragma once


#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "WarriorAI.h"
#include "../CombatSpecializationTemplates.h"
#include "../ResourceTypes.h"
#include "../SpellValidation_WoW120_Part2.h"  // Central spell registry
#include "../HeroTalentDetector.h"            // Hero talent tree detection
// Phase 5 Integration
#include "../../Decision/ActionPriorityQueue.h"
#include "../../Decision/BehaviorTree.h"
#include "../../BotAI.h"
#include <unordered_map>
#include <queue>

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
// ARMS WARRIOR SPELL ALIASES - Using Central Registry (WoW 12.0.7)
// ============================================================================
// All spell IDs from WoW120Spells::Warrior and WoW120Spells::Warrior::Arms
namespace ArmsWarriorSpells
{
    // Core Warrior spells (from WoW120Spells::Warrior)
    using namespace WoW120Spells::Warrior;

    // Arms-specific spells (from WoW120Spells::Warrior::Arms)
    // Aliased with SPELL_ prefix for consistency
    constexpr uint32 SPELL_BATTLE_SHOUT      = WoW120Spells::Warrior::BATTLE_SHOUT;
    constexpr uint32 SPELL_COMMANDING_SHOUT  = WoW120Spells::Warrior::COMMANDING_SHOUT;
    constexpr uint32 SPELL_CHARGE            = WoW120Spells::Warrior::CHARGE;

    // Arms Rotation
    constexpr uint32 SPELL_MORTAL_STRIKE     = WoW120Spells::Warrior::Arms::MORTAL_STRIKE;
    constexpr uint32 SPELL_COLOSSUS_SMASH    = WoW120Spells::Warrior::Arms::COLOSSUS_SMASH;
    constexpr uint32 SPELL_OVERPOWER         = WoW120Spells::Warrior::Arms::OVERPOWER;
    constexpr uint32 SPELL_EXECUTE           = WoW120Spells::Warrior::Arms::EXECUTE;
    constexpr uint32 SPELL_WHIRLWIND         = WoW120Spells::Warrior::Arms::WHIRLWIND;
    constexpr uint32 SPELL_REND              = WoW120Spells::Warrior::Arms::REND;
    constexpr uint32 SPELL_CLEAVE            = WoW120Spells::Warrior::Arms::CLEAVE;
    constexpr uint32 SPELL_SLAM              = WoW120Spells::Warrior::Arms::SLAM;

    // Arms Cooldowns
    constexpr uint32 SPELL_WARBREAKER        = WoW120Spells::Warrior::Arms::WARBREAKER;
    constexpr uint32 SPELL_SWEEPING_STRIKES  = WoW120Spells::Warrior::Arms::SWEEPING_STRIKES;
    constexpr uint32 SPELL_BLADESTORM        = WoW120Spells::Warrior::Arms::BLADESTORM;
    constexpr uint32 SPELL_AVATAR            = WoW120Spells::Warrior::Arms::AVATAR;
    constexpr uint32 SPELL_DIE_BY_THE_SWORD  = WoW120Spells::Warrior::Arms::DIE_BY_THE_SWORD;
    constexpr uint32 SPELL_DEFENSIVE_STANCE  = WoW120Spells::Warrior::Arms::DEFENSIVE_STANCE;
    constexpr uint32 SPELL_SKULLSPLITTER     = WoW120Spells::Warrior::Arms::SKULLSPLITTER;
    constexpr uint32 SPELL_RAVAGER           = WoW120Spells::Warrior::Arms::RAVAGER;
    constexpr uint32 SPELL_THUNDEROUS_ROAR   = WoW120Spells::Warrior::Arms::THUNDEROUS_ROAR;
    constexpr uint32 SPELL_CHAMPIONS_SPEAR   = WoW120Spells::Warrior::Arms::CHAMPIONS_SPEAR;

    // Deep Wounds (DoT)
    constexpr uint32 SPELL_DEEP_WOUNDS       = WoW120Spells::Warrior::Arms::DEEP_WOUNDS;
    constexpr uint32 SPELL_DEEP_WOUNDS_DEBUFF = WoW120Spells::Warrior::Arms::DEEP_WOUNDS_DEBUFF;

    // Procs (from central registry)
    constexpr uint32 SPELL_OVERPOWER_PROC    = WoW120Spells::Warrior::OVERPOWER_PROC;
    constexpr uint32 SPELL_SUDDEN_DEATH_PROC = WoW120Spells::Warrior::SUDDEN_DEATH_PROC;

    // Hero Talents - Slayer
    constexpr uint32 SPELL_SLAYERS_STRIKE      = WoW120Spells::Warrior::Arms::SLAYERS_STRIKE;
    constexpr uint32 SPELL_OVERWHELMING_BLADES = WoW120Spells::Warrior::Arms::OVERWHELMING_BLADES;
    constexpr uint32 SPELL_SLAYERS_DOMINANCE   = WoW120Spells::Warrior::Arms::SLAYERS_DOMINANCE;

    // Hero Talents - Colossus
    constexpr uint32 SPELL_DEMOLISH          = WoW120Spells::Warrior::Arms::DEMOLISH;
    constexpr uint32 SPELL_COLOSSAL_MIGHT    = WoW120Spells::Warrior::Arms::COLOSSAL_MIGHT;
    constexpr uint32 SPELL_MARTIAL_PROWESS   = WoW120Spells::Warrior::Arms::MARTIAL_PROWESS;
}

/**
 * Refactored Arms Warrior using template architecture
 *
 * Key improvements:
 * - Inherits from MeleeDpsSpecialization<RageResource> for role defaults
 * - Automatically gets UpdateCooldowns, CanUseAbility, OnCombatStart/End
 * - Uses specialized rage management as primary resource
 * - Uses central spell registry (SpellValidation_WoW120_Part2.h)
 * - WoW 12.0.7 (The War Within) spell IDs
 */
class ArmsWarriorRefactored : public MeleeDpsSpecialization<RageResource>
{
public:
    // Use base class members with type alias for cleaner syntax
    using Base = MeleeDpsSpecialization<RageResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    using Base::IsInMeleeRange;
    using Base::CanUseAbility;

    // NOTE: BehaviorTree using declarations at namespace scope (lines 33-40) already accessible

private:
    // Forward declarations for methods called in constructor
    // NOTE: Function declarations removed - inline definitions provided below
    void InitializeDebuffTracking();
    void InitializeArmsRotation();

public:
    explicit ArmsWarriorRefactored(Player* bot)
        : MeleeDpsSpecialization<RageResource>(bot)
        , _deepWoundsTracking()
        , _colossusSmashActive(false)
        , _overpowerReady(false)
        , _suddenDeathProc(false)
        , _executePhaseActive(false)
        , _lastMortalStrike(0)
        , _lastColossusSmash(0)
    {
        // Initialize debuff tracking
        InitializeDebuffTracking();

        // Setup Arms-specific mechanics
        InitializeArmsRotation();
    }

    // ========================================================================
    // CORE ROTATION - Only Arms-specific logic
    // ========================================================================

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !target->IsAlive() || !target->IsHostileTo(this->GetBot()))
            return;

        // Update Arms-specific mechanics
        UpdateArmsState(target);

        // Execute phase has highest priority
        if (IsExecutePhase(target))
        {
            ExecutePhaseRotation(target);
            return;
        }

        // Standard rotation
        ExecuteArmsRotation(target);
    }

    void UpdateBuffs() override
    {
        using namespace ArmsWarriorSpells;
        Player* bot = this->GetBot();

        // Maintain Battle Shout
        if (!bot->HasAura(SPELL_BATTLE_SHOUT) && !bot->HasAura(SPELL_COMMANDING_SHOUT))
        {
            this->CastSpell(SPELL_BATTLE_SHOUT, bot);
        }

        // Sweeping Strikes for multiple enemies
        if (this->GetEnemiesInRange(8.0f) >= 2 && !bot->HasAura(SPELL_SWEEPING_STRIKES))
        {
            if (this->CanUseAbility(SPELL_SWEEPING_STRIKES))
            {
                this->CastSpell(SPELL_SWEEPING_STRIKES, bot);
            }
        }

        // Use Defensive Stance cooldown if low health (12.0.7 - ability, not stance)
        if (bot->GetHealthPct() < 40.0f && this->CanUseAbility(SPELL_DEFENSIVE_STANCE))
        {
            this->CastSpell(SPELL_DEFENSIVE_STANCE, bot);
        }
    }

protected:
    // ========================================================================
    // RESOURCE MANAGEMENT OVERRIDE
    // ========================================================================

    uint32 GetSpellResourceCost(uint32 spellId) const override
    {
        using namespace ArmsWarriorSpells;
        switch (spellId)
        {
            case SPELL_MORTAL_STRIKE:    return 30;
            case SPELL_COLOSSUS_SMASH:   return 20;
            case SPELL_OVERPOWER:        return 5;
            case SPELL_EXECUTE:          return 15; // Base cost, scales with available rage
            case SPELL_WHIRLWIND:        return 25;
            case SPELL_REND:             return 10;
            case SPELL_SLAM:             return 20;
            case SPELL_CLEAVE:           return 20;
            default:                     return 10;
        }
    }

    // ========================================================================
    // ARMS-SPECIFIC ROTATION LOGIC
    // ========================================================================

    void ExecuteArmsRotation(::Unit* target)
    {
        using namespace ArmsWarriorSpells;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero Talent: Colossus - Demolish replaces Colossus Smash burst window
        if (_heroTalents.IsTree(HeroTalentTree::COLOSSUS))
        {
            if (this->CanUseAbility(SPELL_DEMOLISH))
            {
                this->CastSpell(SPELL_DEMOLISH, target);
                _colossusSmashActive = true;
                _lastColossusSmash = GameTime::GetGameTimeMS();
                return;
            }
        }

        // Hero Talent: Slayer - Slayer's Strike for extra damage during Colossus Smash
        if (_heroTalents.IsTree(HeroTalentTree::SLAYER) && _colossusSmashActive)
        {
            if (this->CanUseAbility(SPELL_SLAYERS_STRIKE))
            {
                this->CastSpell(SPELL_SLAYERS_STRIKE, target);
                return;
            }
        }

        // Priority 1: Colossus Smash for vulnerability window
        if (ShouldUseColossusSmash(target) && this->CanUseAbility(SPELL_COLOSSUS_SMASH))
        {
            this->CastSpell(SPELL_COLOSSUS_SMASH, target);
            _colossusSmashActive = true;
            _lastColossusSmash = GameTime::GetGameTimeMS();
            return;
        }

        // Priority 2: Bladestorm for burst AoE
        if (ShouldUseBladestorm() && this->CanUseAbility(SPELL_BLADESTORM))
        {
            this->CastSpell(SPELL_BLADESTORM, this->GetBot());
            return;
        }

        // Priority 3: Avatar for damage increase
        if (ShouldUseAvatar(target) && this->CanUseAbility(SPELL_AVATAR))
        {
            this->CastSpell(SPELL_AVATAR, this->GetBot());
            return;
        }

        // Priority 4: Mortal Strike - Primary damage and healing reduction
        if (this->CanUseAbility(SPELL_MORTAL_STRIKE))
        {
            this->CastSpell(SPELL_MORTAL_STRIKE, target);
            _lastMortalStrike = GameTime::GetGameTimeMS();
            ApplyDeepWounds(target);
            return;
        }

        // Priority 5: Overpower when proc is available
        if (_overpowerReady && this->CanUseAbility(SPELL_OVERPOWER))
        {
            this->CastSpell(SPELL_OVERPOWER, target);
            _overpowerReady = false;
            ApplyDeepWounds(target);
            return;
        }

        // Priority 6: Warbreaker for AoE debuff
        if (this->GetEnemiesInRange(8.0f) >= 2 && this->CanUseAbility(SPELL_WARBREAKER))
        {
            this->CastSpell(SPELL_WARBREAKER, target);
            return;
        }

        // Priority 7: Whirlwind for AoE
        if (this->GetEnemiesInRange(8.0f) >= 2 && this->CanUseAbility(SPELL_WHIRLWIND))
        {
            this->CastSpell(SPELL_WHIRLWIND, this->GetBot());
            return;
        }

        // Priority 8: Rend for DoT (if not already applied)
        if (!HasRendDebuff(target) && this->_resource >= 10 && this->CanUseAbility(SPELL_REND))
        {
            this->CastSpell(SPELL_REND, target);
            _rendTracking[target->GetGUID()] = GameTime::GetGameTimeMS() + 21000;
            return;
        }

        // Priority 9: Slam as rage dump
        if (this->_resource >= 80 && this->CanUseAbility(SPELL_SLAM))
        {
            this->CastSpell(SPELL_SLAM, target);
            return;
        }
    }

    void ExecutePhaseRotation(::Unit* target)
    {
        using namespace ArmsWarriorSpells;

        // Priority 1: Execute with Sudden Death proc
        if (_suddenDeathProc && this->CanUseAbility(SPELL_EXECUTE))
        {
            this->CastSpell(SPELL_EXECUTE, target);
            _suddenDeathProc = false;
            return;
        }

        // Priority 2: Colossus Smash for execute damage
        if (!_colossusSmashActive && this->CanUseAbility(SPELL_COLOSSUS_SMASH))
        {
            this->CastSpell(SPELL_COLOSSUS_SMASH, target);
            _colossusSmashActive = true;
            _lastColossusSmash = GameTime::GetGameTimeMS();
            return;
        }

        // Priority 3: Execute spam with available rage
        if (this->CanUseAbility(SPELL_EXECUTE))
        {
            // Execute consumes up to 40 additional rage for bonus damage
            if (this->_resource >= 15) // Base cost
            {
                this->CastSpell(SPELL_EXECUTE, target);
                return;
            }
        }

        // Priority 4: Mortal Strike to maintain pressure
        if (this->CanUseAbility(SPELL_MORTAL_STRIKE))
        {
            this->CastSpell(SPELL_MORTAL_STRIKE, target);
            _lastMortalStrike = GameTime::GetGameTimeMS();
            return;
        }

        // Priority 5: Overpower if available
        if (_overpowerReady && this->CanUseAbility(SPELL_OVERPOWER))
        {
            this->CastSpell(SPELL_OVERPOWER, target);
            _overpowerReady = false;
            return;
        }
    }

    // ========================================================================
    // ARMS-SPECIFIC STATE MANAGEMENT
    // ========================================================================

    void UpdateArmsState(::Unit* target)
    {
        using namespace ArmsWarriorSpells;
        Player* bot = this->GetBot();
        uint32 currentTime = GameTime::GetGameTimeMS();

        // Check for Overpower proc (after dodge/parry)
        _overpowerReady = bot->HasAura(SPELL_OVERPOWER_PROC);

        // Check for Sudden Death proc (free Execute)
        _suddenDeathProc = bot->HasAura(SPELL_SUDDEN_DEATH_PROC);

        // Update Colossus Smash tracking
        if (_colossusSmashActive && currentTime > _lastColossusSmash + 10000)
        {
            _colossusSmashActive = false;
        }

        // Update Deep Wounds tracking
        CleanupExpiredDeepWounds();

        // Update execute phase state
        _executePhaseActive = (target->GetHealthPct() <= 20.0f);
    }    // ========================================================================
    // DEEP WOUNDS MANAGEMENT
    // ========================================================================

    void ApplyDeepWounds(::Unit* target)
    {
        if (!target)
            return;

        // Deep Wounds is applied by critical strikes
        _deepWoundsTracking[target->GetGUID()] = GameTime::GetGameTimeMS() + 21000; // 21 second duration
    }

    void CleanupExpiredDeepWounds()
    {
        uint32 currentTime = GameTime::GetGameTimeMS();
        for (auto it = _deepWoundsTracking.begin(); it != _deepWoundsTracking.end();)
        {
            if (it->second < currentTime)
                it = _deepWoundsTracking.erase(it);
            else
                ++it;
        }
    }

    // ========================================================================
    // CONDITION CHECKS
    // ========================================================================

    bool IsExecutePhase(::Unit* target) const
    {
        return target && (target->GetHealthPct() <= 20.0f || _suddenDeathProc);
    }

    bool ShouldUseColossusSmash(::Unit* target) const
    {
        // Use on cooldown for damage window
        return !_colossusSmashActive && target;
    }

    bool ShouldUseBladestorm() const
    {
        // Use for 3+ enemies or burst phase
        return this->GetEnemiesInRange(8.0f) >= 3 || this->_resource >= 80;
    }

    bool ShouldUseAvatar(::Unit* target) const
    {
        // Use during Colossus Smash window or execute phase
        return target && (_colossusSmashActive || _executePhaseActive);
    }

    bool HasRendDebuff(::Unit* target) const
    {
        if (!target)
            return false;

        auto it = _rendTracking.find(target->GetGUID());
        return it != _rendTracking.end() && it->second > GameTime::GetGameTimeMS();
    }

    // ========================================================================
    // COMBAT LIFECYCLE HOOKS
    // ========================================================================

    void OnCombatStartSpecific(::Unit* target) override
    {
        using namespace ArmsWarriorSpells;
        // Reset all Arms-specific state
        _colossusSmashActive = false;
        _overpowerReady = false;
        _suddenDeathProc = false;
        _executePhaseActive = false;
        _lastMortalStrike = 0;
        _lastColossusSmash = 0;
        _deepWoundsTracking.clear();
        _rendTracking.clear();

        // Use charge if not in range
        if (!this->IsInMeleeRange(target) && this->CanUseAbility(SPELL_CHARGE))
        {
            this->CastSpell(SPELL_CHARGE, target);
        }
    }

    void OnCombatEndSpecific() override
    {
        // Clear all combat state
        _colossusSmashActive = false;
        _overpowerReady = false;
        _suddenDeathProc = false;
        _executePhaseActive = false;
        _deepWoundsTracking.clear();
        _rendTracking.clear();
    }

private:
    CooldownManager _cooldowns;
    // ========================================================================
    // INITIALIZATION
    // ========================================================================
    // NOTE: InitializeDebuffTracking() and InitializeArmsRotation() are
    // declared at class level and should be defined in .cpp file

    // IMPLEMENTATION MOVED TO .CPP - REMOVED DUPLICATE INLINE DEFINITIONS

    // ========================================================================
    // INLINE HELPER METHODS
    // ========================================================================

    // Previous initialization code moved to .cpp to avoid C2535 duplicate definition error

    void inline SetupActionPriorityQueue()
    {
        using namespace ArmsWarriorSpells;
        // ========================================================================
        // PHASE 5 INTEGRATION: ActionPriorityQueue
        // ========================================================================
        BotAI* ai = this;
        if (!ai)
            return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // Emergency spells
            queue->RegisterSpell(SPELL_EXECUTE, SpellPriority::EMERGENCY, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SPELL_EXECUTE,
                ::std::function<bool(Player*, Unit*)>{[](Player* bot, Unit* target)
                {
                    return target && target->GetHealthPct() < 20.0f;
                }},
                "Target HP < 20% (Execute range)");

            // Critical cooldowns
            queue->RegisterSpell(SPELL_COLOSSUS_SMASH, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->RegisterSpell(SPELL_BLADESTORM, SpellPriority::CRITICAL, SpellCategory::DAMAGE_AOE);
            queue->RegisterSpell(SPELL_AVATAR, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);

            // High priority core rotation
            queue->RegisterSpell(SPELL_MORTAL_STRIKE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->RegisterSpell(SPELL_OVERPOWER, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SPELL_OVERPOWER,
                [](Player* bot, Unit*) {
                    return bot->HasAura(ArmsWarriorSpells::SPELL_OVERPOWER_PROC);
                },
                "Overpower proc active");

            // Medium priority
            queue->RegisterSpell(SPELL_WHIRLWIND, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(SPELL_WHIRLWIND,
                ::std::function<bool(Player*, Unit*)>{[this](Player* bot, Unit*)
                {
                    // Capture 'this' for member access if needed
                    return bot->getAttackers().size() >= 3;
                }},
                "3+ targets (AoE)");

            queue->RegisterSpell(SPELL_REND, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SPELL_REND,
                ::std::function<bool(Player*, Unit*)>{[](Player* bot, Unit* target)
                {
                    return target && !target->HasAura(ArmsWarriorSpells::SPELL_REND);
                }},
                "Rend not active on target");

            // Low priority fillers
            queue->RegisterSpell(SPELL_SLAM, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->RegisterSpell(SPELL_CLEAVE, SpellPriority::LOW, SpellCategory::DAMAGE_AOE);

            TC_LOG_INFO("module.playerbot", "  ARMS WARRIOR: Registered {} spells in ActionPriorityQueue",
                queue->GetSpellCount());
        }

        // ========================================================================
        // PHASE 5 INTEGRATION: BehaviorTree
        // ========================================================================
        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Arms Warrior Combat", {
                // ============================================================
                // 1. EXECUTE PHASE (Target < 20% HP)
                // ============================================================
                Sequence("Execute Phase", {
                    Condition("Target < 20% HP", [](Player* bot, Unit* target) {
                        return target && target->GetHealthPct() < 20.0f;
                    }),
                    Selector("Execute Priority", {
                        bot::ai::Action("Cast Execute", [this](Player* bot, Unit* target) {
                            if (this->CanCastSpell(SPELL_EXECUTE, target))
                            {
                                this->CastSpell(SPELL_EXECUTE, target);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        }),
                        bot::ai::Action("Cast Mortal Strike (Execute Phase)", [this](Player* bot, Unit* target)
                        {
                            if (this->CanCastSpell(SPELL_MORTAL_STRIKE, target))
                            {
                                this->CastSpell(SPELL_MORTAL_STRIKE, target);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        })
                    })
                }),

                // ============================================================
                // 2. COOLDOWN USAGE (Boss fights, burst windows)
                // ============================================================
                Sequence("Use Major Cooldowns", {
                    Condition("Should use cooldowns", [](Player* bot, Unit* target) {
                        // Use cooldowns on bosses or high HP targets
                        return target && (target->GetCreatureType() == CREATURE_TYPE_HUMANOID ||
                                        target->GetMaxHealth() > 500000);
                    }),
                    Selector("Cooldown Priority", {
                        bot::ai::Action("Cast Avatar", [this](Player* bot, Unit*) {
                            if (this->CanCastSpell(SPELL_AVATAR, bot))
                            {
                                this->CastSpell(SPELL_AVATAR, bot);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        }),
                        bot::ai::Action("Cast Bladestorm", [this](Player* bot, Unit*) {
                            if (this->CanCastSpell(SPELL_BLADESTORM, bot))
                            {
                                this->CastSpell(SPELL_BLADESTORM, bot);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        })
                    })
                }),

                // ============================================================
                // 3. STANDARD ROTATION
                // ============================================================
                Sequence("Standard Rotation", {
                    // Maintain Colossus Smash debuff
                    Selector("Maintain Colossus Smash", {
                        Condition("CS Active", [](Player* bot, Unit* target) {
                            return target && target->HasAura(SPELL_COLOSSUS_SMASH);
                        }),
                        bot::ai::Action("Cast Colossus Smash", [this](Player* bot, Unit* target) {
                            if (this->CanCastSpell(SPELL_COLOSSUS_SMASH, target))
                            {
                                this->CastSpell(SPELL_COLOSSUS_SMASH, target);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        })
                    }),

                    // Cast Mortal Strike on cooldown
                    Selector("Mortal Strike", {
                        bot::ai::Action("Cast Mortal Strike", [this](Player* bot, Unit* target) {
                            if (this->CanCastSpell(SPELL_MORTAL_STRIKE, target))
                            {
                                this->CastSpell(SPELL_MORTAL_STRIKE, target);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        })
                    }),

                    // Cast Overpower on proc
                    Sequence("Overpower on Proc", {
                        Condition("Has Overpower Proc", [](Player* bot, Unit*) {
                            return bot->HasAura(SPELL_OVERPOWER_PROC);
                        }),
                        bot::ai::Action("Cast Overpower", [this](Player* bot, Unit* target) {
                            if (this->CanCastSpell(SPELL_OVERPOWER, target))
                            {
                                this->CastSpell(SPELL_OVERPOWER, target);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        })
                    }),

                    // Filler spells
                    Selector("Filler", {
                        bot::ai::Action("Cast Whirlwind (AoE)", [this](Player* bot, Unit* target)
                        {
                            if (bot->getAttackers().size() >= 3 && this->CanCastSpell(ArmsWarriorSpells::SPELL_WHIRLWIND, target))
                            {
                                this->CastSpell(ArmsWarriorSpells::SPELL_WHIRLWIND, target);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        }),
                        bot::ai::Action("Cast Slam", [this](Player* bot, Unit* target) {
                            if (this->CanCastSpell(ArmsWarriorSpells::SPELL_SLAM, target))
                            {
                                this->CastSpell(ArmsWarriorSpells::SPELL_SLAM, target);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        })
                    })
                })
            });

            behaviorTree->SetRoot(root);
            TC_LOG_INFO("module.playerbot", " ARMS WARRIOR: BehaviorTree initialized with hierarchical combat flow");
        }
    }

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    // Debuff tracking
    ::std::unordered_map<ObjectGuid, uint32> _deepWoundsTracking;
    ::std::unordered_map<ObjectGuid, uint32> _rendTracking;

    // State tracking
    bool _colossusSmashActive;
    bool _overpowerReady;
    bool _suddenDeathProc;
    bool _executePhaseActive;

    // Timing tracking
    uint32 _lastMortalStrike;
    uint32 _lastColossusSmash;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
