/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Fury Warrior Specialization - REFACTORED
 *
 * This demonstrates the complete migration of Fury Warrior to the template-based
 * architecture, eliminating code duplication while maintaining full functionality.
 *
 * BEFORE: ~940 lines of code with duplicate UpdateCooldowns, CanUseAbility, etc.
 * AFTER: ~220 lines focusing ONLY on Fury-specific logic
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
#include "Item.h"
#include "ItemDefines.h"
#include "UnitDefines.h"
#include <unordered_map>

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
// FURY WARRIOR SPELL ALIASES - Using Central Registry (WoW 12.0.7)
// ============================================================================
namespace FuryWarriorSpells
{
    // Core Warrior spells (from WoW120Spells::Warrior)
    constexpr uint32 SPELL_BATTLE_SHOUT      = WoW120Spells::Warrior::BATTLE_SHOUT;
    constexpr uint32 SPELL_COMMANDING_SHOUT  = WoW120Spells::Warrior::COMMANDING_SHOUT;
    constexpr uint32 SPELL_CHARGE            = WoW120Spells::Warrior::CHARGE;
    constexpr uint32 SPELL_BERSERKER_RAGE    = WoW120Spells::Warrior::BERSERKER_RAGE;

    // Fury Core Rotation
    constexpr uint32 SPELL_BLOODTHIRST       = WoW120Spells::Warrior::Fury::BLOODTHIRST;
    constexpr uint32 SPELL_RAMPAGE           = WoW120Spells::Warrior::Fury::RAMPAGE;
    constexpr uint32 SPELL_RAGING_BLOW       = WoW120Spells::Warrior::Fury::RAGING_BLOW;
    constexpr uint32 SPELL_EXECUTE           = WoW120Spells::Warrior::Fury::EXECUTE;
    constexpr uint32 SPELL_WHIRLWIND         = WoW120Spells::Warrior::Fury::WHIRLWIND;
    constexpr uint32 SPELL_ONSLAUGHT         = WoW120Spells::Warrior::Fury::ONSLAUGHT;

    // Fury Cooldowns
    constexpr uint32 SPELL_RECKLESSNESS      = WoW120Spells::Warrior::Fury::RECKLESSNESS;
    constexpr uint32 SPELL_ENRAGED_REGENERATION = WoW120Spells::Warrior::Fury::ENRAGED_REGENERATION;
    constexpr uint32 SPELL_BLADESTORM        = WoW120Spells::Warrior::Fury::BLADESTORM;
    constexpr uint32 SPELL_RAVAGER           = WoW120Spells::Warrior::Fury::RAVAGER;
    constexpr uint32 SPELL_ODYN_FURY         = WoW120Spells::Warrior::Fury::ODYN_FURY;
    constexpr uint32 SPELL_THUNDEROUS_ROAR   = WoW120Spells::Warrior::Fury::THUNDEROUS_ROAR;
    constexpr uint32 SPELL_CHAMPIONS_SPEAR   = WoW120Spells::Warrior::Fury::CHAMPIONS_SPEAR;

    // Enrage and Buffs
    constexpr uint32 SPELL_ENRAGE            = WoW120Spells::Warrior::Fury::ENRAGE;
    constexpr uint32 SPELL_WHIRLWIND_BUFF    = WoW120Spells::Warrior::Fury::WHIRLWIND_BUFF;

    // Hero Talents
    constexpr uint32 SPELL_SLAYERS_STRIKE    = WoW120Spells::Warrior::Fury::SLAYERS_STRIKE;
    constexpr uint32 SPELL_THUNDER_BLAST     = WoW120Spells::Warrior::Fury::THUNDER_BLAST;
}

/**
 * Refactored Fury Warrior using template architecture
 *
 * Key improvements:
 * - Inherits from MeleeDpsSpecialization<RageResource> for role defaults
 * - Automatically gets UpdateCooldowns, CanUseAbility, OnCombatStart/End
 * - Specialized enrage and dual-wield mechanics
 * - Eliminates ~720 lines of duplicate code
 */
class FuryWarriorRefactored : public MeleeDpsSpecialization<RageResource>
{
public:
    using Base = MeleeDpsSpecialization<RageResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::CanUseAbility;
    using Base::IsInMeleeRange;
    using Base::_resource;
    explicit FuryWarriorRefactored(Player* bot)
        : MeleeDpsSpecialization<RageResource>(bot)
        , _isEnraged(false)
        , _enrageEndTime(0)
        , _hasWhirlwindBuff(false)
        , _rampageStacks(0)
        , _lastBloodthirst(0)
        , _lastRampage(0)
        , _executePhaseActive(false)
        , _hasDualWield(false)
    {
        // Verify dual-wield capability
        CheckDualWieldStatus();

        // Initialize Fury-specific systems
        InitializeFuryMechanics();
    }

    // ========================================================================
    // CORE ROTATION - Only Fury-specific logic
    // ========================================================================

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !target->IsAlive() || !target->IsHostileTo(this->GetBot()))
            return;

        // Update Fury-specific mechanics
        UpdateFuryState(target);

        // Execute phase has different priority
        if (IsExecutePhase(target))
        {
            ExecutePhaseRotation(target);
            return;
        }

        // Standard Fury rotation
        ExecuteFuryRotation(target);
    }

    void UpdateBuffs() override
    {
        using namespace FuryWarriorSpells;
        Player* bot = this->GetBot();

        // Maintain Battle Shout
        if (!bot->HasAura(SPELL_BATTLE_SHOUT) && !bot->HasAura(SPELL_COMMANDING_SHOUT))
        {
            this->CastSpell(SPELL_BATTLE_SHOUT, bot);
        }

        // Use Berserker Rage when needed for rage generation or fear break
        if (ShouldUseBerserkerRage() && this->CanUseAbility(SPELL_BERSERKER_RAGE))
        {
            this->CastSpell(SPELL_BERSERKER_RAGE, bot);
        }
    }

protected:
    // ========================================================================
    // RESOURCE MANAGEMENT (using GetSpellResourceCost from base class)
    // ========================================================================

    // ========================================================================
    // FURY-SPECIFIC ROTATION LOGIC
    // ========================================================================

    void ExecuteFuryRotation(::Unit* target)
    {
        using namespace FuryWarriorSpells;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero Talent: Mountain Thane - Thunder Blast for burst AoE damage
        if (_heroTalents.IsTree(HeroTalentTree::MOUNTAIN_THANE))
        {
            if (this->CanUseAbility(SPELL_THUNDER_BLAST))
            {
                this->CastSpell(SPELL_THUNDER_BLAST, target);
                return;
            }
        }

        // Hero Talent: Slayer - Slayer's Strike weaved into rotation
        if (_heroTalents.IsTree(HeroTalentTree::SLAYER) && _isEnraged)
        {
            if (this->CanUseAbility(SPELL_SLAYERS_STRIKE))
            {
                this->CastSpell(SPELL_SLAYERS_STRIKE, target);
                return;
            }
        }

        // Priority 1: Maintain Enrage with Rampage
        if (ShouldUseRampage() && this->CanUseAbility(SPELL_RAMPAGE))
        {
            this->CastSpell(SPELL_RAMPAGE, target);
            _lastRampage = GameTime::GetGameTimeMS();
            TriggerEnrage();
            return;
        }

        // Priority 2: Recklessness for burst
        if (ShouldUseRecklessness(target) && this->CanUseAbility(SPELL_RECKLESSNESS))
        {
            this->CastSpell(SPELL_RECKLESSNESS, this->GetBot());
            return;
        }

        // Priority 3: Bloodthirst on cooldown for rage generation and enrage chance
        if (this->CanUseAbility(SPELL_BLOODTHIRST))
        {
            this->CastSpell(SPELL_BLOODTHIRST, target);
            _lastBloodthirst = GameTime::GetGameTimeMS();

            // 30% chance to trigger Enrage
            if (!_isEnraged && (rand() % 100) < 30)
            {
                TriggerEnrage();
            }
            return;
        }

        // Priority 4: Raging Blow while Enraged
        if (_isEnraged && this->CanUseAbility(SPELL_RAGING_BLOW))
        {
            this->CastSpell(SPELL_RAGING_BLOW, target);
            return;
        }

        // Priority 5: Whirlwind for AoE or to gain buff
        if (ShouldUseWhirlwind() && this->CanUseAbility(SPELL_WHIRLWIND))
        {
            this->CastSpell(SPELL_WHIRLWIND, this->GetBot());
            _hasWhirlwindBuff = true;
            return;
        }

        // Priority 6: Onslaught as filler (12.0.7 - replaces Furious Slash)
        if (this->CanUseAbility(SPELL_ONSLAUGHT))
        {
            this->CastSpell(SPELL_ONSLAUGHT, target);
            return;
        }

        // Priority 7: Onslaught as rage dump if high rage
        if (this->_resource >= 80 && this->CanUseAbility(SPELL_ONSLAUGHT))
        {
            this->CastSpell(SPELL_ONSLAUGHT, target);
            return;
        }
    }

    void ExecutePhaseRotation(::Unit* target)
    {
        using namespace FuryWarriorSpells;

        // Priority 1: Maintain Enrage
        if (!_isEnraged && this->CanUseAbility(SPELL_RAMPAGE))
        {
            this->CastSpell(SPELL_RAMPAGE, target);
            TriggerEnrage();
            return;
        }

        // Priority 2: Execute spam
        if (this->CanUseAbility(SPELL_EXECUTE))
        {
            this->CastSpell(SPELL_EXECUTE, target);
            return;
        }

        // Priority 3: Bloodthirst for rage
        if (this->CanUseAbility(SPELL_BLOODTHIRST))
        {
            this->CastSpell(SPELL_BLOODTHIRST, target);
            _lastBloodthirst = GameTime::GetGameTimeMS();
            return;
        }

        // Priority 4: Raging Blow if enraged
        if (_isEnraged && this->CanUseAbility(SPELL_RAGING_BLOW))
        {
            this->CastSpell(SPELL_RAGING_BLOW, target);
            return;
        }

        // Priority 5: Rampage if high rage
        if (this->_resource >= 85 && this->CanUseAbility(SPELL_RAMPAGE))
        {
            this->CastSpell(SPELL_RAMPAGE, target);
            TriggerEnrage();
            return;
        }
    }

    // ========================================================================
    // FURY-SPECIFIC STATE MANAGEMENT
    // ========================================================================

    void UpdateFuryState(::Unit* target)
    {
        using namespace FuryWarriorSpells;
        Player* bot = this->GetBot();
        uint32 currentTime = GameTime::GetGameTimeMS();

        // Check Enrage status
        if (bot->HasAura(SPELL_ENRAGE))
        {
            if (!_isEnraged)
            {
                _isEnraged = true;
                _enrageEndTime = currentTime + 4000; // 4 second duration
            }
        }
        else if (_isEnraged && currentTime > _enrageEndTime)
        {
            _isEnraged = false;
            _enrageEndTime = 0;
        }

        // Update Whirlwind buff tracking (Meat Cleaver - affects next 2 abilities)
        _hasWhirlwindBuff = bot->HasAura(SPELL_WHIRLWIND_BUFF);

        // Check execute phase
        _executePhaseActive = (target->GetHealthPct() <= 20.0f);
    }

    void TriggerEnrage()
    {
        _isEnraged = true;
        _enrageEndTime = GameTime::GetGameTimeMS() + 4000; // 4 seconds
    }

    void CheckDualWieldStatus()
    {
        Player* bot = this->GetBot();

        // Check if bot has weapons in both hands
        Item* mainHand = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
        Item* offHand = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);        _hasDualWield = mainHand && offHand &&
                       offHand->GetTemplate() &&
                       offHand->GetTemplate()->GetClass() == ITEM_CLASS_WEAPON;
    }

    // ========================================================================
    // CONDITION CHECKS
    // ========================================================================

    bool IsExecutePhase(::Unit* target) const
    {
        return target && target->GetHealthPct() <= 20.0f;
    }

    bool ShouldUseRampage() const
    {
        // Use Rampage when:
        // 1. Not enraged and have enough rage
        // 2. Enrage is about to expire
        // 3. At rage cap

        uint32 currentTime = GameTime::GetGameTimeMS();

        if (!_isEnraged && this->_resource >= 85)
            return true;

        if (_isEnraged && (_enrageEndTime - currentTime) < 1000)
            return true;

        if (this->_resource >= 95)
            return true;

        return false;
    }

    bool ShouldUseWhirlwind() const
    {
        // Use Whirlwind for:
        // 1. AoE situations (2+ enemies)
        // 2. To get Whirlwind buff for cleave
        return GetEnemiesInRange(8.0f) >= 2 || !_hasWhirlwindBuff;
    }

    bool ShouldUseBerserkerRage() const
    {
        Player* bot = this->GetBot();

        // Use when:
        // 1. Feared, charmed, or incapacitated
        // 2. Low rage and not enraged
        if (bot->HasUnitState(UNIT_STATE_FLEEING | UNIT_STATE_CHARMED | UNIT_STATE_CONFUSED))
            return true;

        if (this->_resource < 20 && !_isEnraged)
            return true;

        return false;
    }

    bool ShouldUseRecklessness(::Unit* target) const
    {
        // Use during burst opportunities
        return target && (_executePhaseActive || this->_resource >= 60);
    }

    // ========================================================================
    // COMBAT LIFECYCLE HOOKS
    // ========================================================================

    void OnCombatStartSpecific(::Unit* target) override
    {
        using namespace FuryWarriorSpells;
        // Reset Fury state
        _isEnraged = false;
        _enrageEndTime = 0;
        _hasWhirlwindBuff = false;
        _rampageStacks = 0;
        _executePhaseActive = false;
        _lastBloodthirst = 0;
        _lastRampage = 0;

        // Use charge if not in range
        if (!this->IsInMeleeRange(target) && this->CanUseAbility(SPELL_CHARGE))
        {
            this->CastSpell(SPELL_CHARGE, target);
        }

        // Check dual-wield status
        CheckDualWieldStatus();
    }

    void OnCombatEndSpecific() override
    {
        // Clear combat state
        _isEnraged = false;
        _enrageEndTime = 0;
        _hasWhirlwindBuff = false;
        _rampageStacks = 0;
        _executePhaseActive = false;
    }

private:
    CooldownManager _cooldowns;
    // ========================================================================
    // INITIALIZATION
    // ========================================================================

    void InitializeFuryMechanics()
    {
        using namespace FuryWarriorSpells;

        // Setup Fury-specific mechanics
        CheckDualWieldStatus();

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
            queue->RegisterSpell(SPELL_ENRAGED_REGENERATION, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(SPELL_ENRAGED_REGENERATION,
                [](Player* bot, Unit*) {
                    return bot->GetHealthPct() < 30.0f;
                },
                "Emergency: HP < 30%");

            queue->RegisterSpell(SPELL_EXECUTE, SpellPriority::EMERGENCY, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SPELL_EXECUTE,
                ::std::function<bool(Player*, Unit*)>{[](Player* bot, Unit* target)
                {
                    return target && target->GetHealthPct() < 20.0f;
                }},
                "Target HP < 20% (Execute range)");

            // Critical cooldowns
            queue->RegisterSpell(SPELL_RECKLESSNESS, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(SPELL_RECKLESSNESS,
                ::std::function<bool(Player*, Unit*)>{[](Player* bot, Unit* target)
                {
                    // Use on bosses or high HP targets
                    return target && (target->GetMaxHealth() > 500000 || target->GetCreatureType() == CREATURE_TYPE_HUMANOID);
                }},
                "Boss fight or high HP target");

            queue->RegisterSpell(SPELL_RAMPAGE, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(SPELL_RAMPAGE,
                [](Player* bot, Unit*) {
                    // Rampage to trigger/maintain Enrage
                    return !bot->HasAura(FuryWarriorSpells::SPELL_ENRAGE) || (bot->GetAura(FuryWarriorSpells::SPELL_ENRAGE) ? bot->GetAura(FuryWarriorSpells::SPELL_ENRAGE)->GetDuration() : 0) < 2000;
                },
                "Trigger or refresh Enrage");

            // High priority core rotation
            queue->RegisterSpell(SPELL_BLOODTHIRST, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->RegisterSpell(SPELL_RAGING_BLOW, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SPELL_RAGING_BLOW,
                [](Player* bot, Unit*) {
                    return bot->HasAura(FuryWarriorSpells::SPELL_ENRAGE);
                },
                "While Enraged");

            // Medium priority
            queue->RegisterSpell(SPELL_WHIRLWIND, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(SPELL_WHIRLWIND,
                [](Player* bot, Unit*) {
                    return bot->getAttackers().size() >= 2; // Use for 2+ targets
                },
                "2+ targets (AoE)");

            queue->RegisterSpell(SPELL_ONSLAUGHT, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);

            // Low priority fillers
            queue->RegisterSpell(SPELL_ONSLAUGHT, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SPELL_ONSLAUGHT,
                [this](Player* bot, Unit*) {
                    return this->_resource >= 80; // Rage dump
                },
                "Rage >= 80 (rage dump)");

            queue->RegisterSpell(SPELL_BERSERKER_RAGE, SpellPriority::LOW, SpellCategory::UTILITY);
            queue->AddCondition(SPELL_BERSERKER_RAGE,
                [](Player* bot, Unit*) {
                    return !bot->HasAura(FuryWarriorSpells::SPELL_ENRAGE);
                },
                "Enrage not active");

            TC_LOG_INFO("module.playerbot", "  FURY WARRIOR: Registered {} spells in ActionPriorityQueue",
                queue->GetSpellCount());
        }

        // ========================================================================
        // PHASE 5 INTEGRATION: BehaviorTree
        // ========================================================================
        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Fury Warrior Combat", {
                // ============================================================
                // 1. EMERGENCY SURVIVAL
                // ============================================================
                Sequence("Emergency Survival", {
                    Condition("HP < 30%", [](Player* bot, Unit*) {
                        return bot->GetHealthPct() < 30.0f;
                    }),
                    bot::ai::Action("Cast Enraged Regeneration", [this](Player* bot, Unit*) {
                        if (this->CanCastSpell(SPELL_ENRAGED_REGENERATION, bot))
                        {
                            this->CastSpell(SPELL_ENRAGED_REGENERATION, bot);
                            return NodeStatus::SUCCESS;
                        }
                        return NodeStatus::FAILURE;
                    })
                }),

                // ============================================================
                // 2. EXECUTE PHASE (Target < 20% HP)
                // ============================================================
                Sequence("Execute Phase", {
                    Condition("Target < 20% HP", [](Player* bot, Unit* target) {
                        return target && target->GetHealthPct() < 20.0f;
                    }),
                    Selector("Execute Priority", {
                        // Maintain Enrage
                        Sequence("Rampage for Enrage", {
                            Condition("No Enrage", [](Player* bot, Unit*) {
                                return !bot->HasAura(SPELL_ENRAGE);
                            }),
                            Condition("Has Rage for Rampage", [this](Player* bot, Unit*) {
                                return this->_resource >= 85;
                            }),
                            bot::ai::Action("Cast Rampage", [this](Player* bot, Unit* target) {
                                if (this->CanCastSpell(SPELL_RAMPAGE, target))
                                {
                                    this->CastSpell(SPELL_RAMPAGE, target);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        // Execute spam
                        bot::ai::Action("Cast Execute", [this](Player* bot, Unit* target) {
                            if (this->CanCastSpell(SPELL_EXECUTE, target))
                            {
                                this->CastSpell(SPELL_EXECUTE, target);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        }),
                        // Bloodthirst for Enrage proc
                        bot::ai::Action("Cast Bloodthirst", [this](Player* bot, Unit* target) {
                            if (this->CanCastSpell(SPELL_BLOODTHIRST, target))
                            {
                                this->CastSpell(SPELL_BLOODTHIRST, target);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        }),
                        // Raging Blow while Enraged
                        Sequence("Raging Blow (Enraged)", {
                            Condition("Is Enraged", [](Player* bot, Unit*) {
                                return bot->HasAura(SPELL_ENRAGE);
                            }),
                            bot::ai::Action("Cast Raging Blow", [this](Player* bot, Unit* target) {
                                if (this->CanCastSpell(SPELL_RAGING_BLOW, target))
                                {
                                    this->CastSpell(SPELL_RAGING_BLOW, target);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // ============================================================
                // 3. COOLDOWN USAGE (Boss fights, burst windows)
                // ============================================================
                Sequence("Use Major Cooldowns", {
                    Condition("Should use cooldowns", [](Player* bot, Unit* target) {
                        return target && (target->GetMaxHealth() > 500000 ||
                                        target->GetCreatureType() == CREATURE_TYPE_HUMANOID);
                    }),
                    Selector("Cooldown Priority", {
                        bot::ai::Action("Cast Recklessness", [this](Player* bot, Unit*) {
                            if (this->CanCastSpell(SPELL_RECKLESSNESS, bot))
                            {
                                this->CastSpell(SPELL_RECKLESSNESS, bot);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        })
                    })
                }),

                // ============================================================
                // 4. STANDARD ROTATION - Maintain Enrage
                // ============================================================
                Sequence("Standard Rotation", {
                    Selector("Maintain Enrage", {
                        // Rampage if no Enrage
                        Sequence("Rampage for Enrage", {
                            Condition("No Enrage or expiring soon", [](Player* bot, Unit*) {
                                return !bot->HasAura(SPELL_ENRAGE) ||
                                       (bot->GetAura(SPELL_ENRAGE) ? bot->GetAura(SPELL_ENRAGE)->GetDuration() : 0) < 2000;
                            }),
                            Condition("Has Rage", [this](Player* bot, Unit*) {
                                return this->_resource >= 85;
                            }),
                            bot::ai::Action("Cast Rampage", [this](Player* bot, Unit* target) {
                                if (this->CanCastSpell(SPELL_RAMPAGE, target))
                                {
                                    this->CastSpell(SPELL_RAMPAGE, target);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        // Berserker Rage if no other way to get Enrage
                        Sequence("Berserker Rage for Enrage", {
                            Condition("No Enrage", [](Player* bot, Unit*) {
                                return !bot->HasAura(SPELL_ENRAGE);
                            }),
                            bot::ai::Action("Cast Berserker Rage", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(SPELL_BERSERKER_RAGE, bot))
                                {
                                    this->CastSpell(SPELL_BERSERKER_RAGE, bot);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    }),

                    // Core rotation - Bloodthirst > Raging Blow (Enraged) > Whirlwind (AoE)
                    Selector("Core Abilities", {
                        bot::ai::Action("Cast Bloodthirst", [this](Player* bot, Unit* target) {
                            if (this->CanCastSpell(SPELL_BLOODTHIRST, target))
                            {
                                this->CastSpell(SPELL_BLOODTHIRST, target);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        }),
                        Sequence("Raging Blow (Enraged)", {
                            Condition("Is Enraged", [](Player* bot, Unit*) {
                                return bot->HasAura(SPELL_ENRAGE);
                            }),
                            bot::ai::Action("Cast Raging Blow", [this](Player* bot, Unit* target) {
                                if (this->CanCastSpell(SPELL_RAGING_BLOW, target))
                                {
                                    this->CastSpell(SPELL_RAGING_BLOW, target);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Whirlwind (AoE)", {
                            Condition("2+ targets", [](Player* bot, Unit*) {
                                return bot->getAttackers().size() >= 2;
                            }),
                            bot::ai::Action("Cast Whirlwind", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(SPELL_WHIRLWIND, bot))
                                {
                                    this->CastSpell(SPELL_WHIRLWIND, bot);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    }),

                    // Filler spells
                    Selector("Filler", {
                        Sequence("Onslaught (Rage Dump)", {
                            Condition("High Rage", [this](Player* bot, Unit*) {
                                return this->_resource >= 80;
                            }),
                            bot::ai::Action("Cast Onslaught", [this](Player* bot, Unit* target) {
                                if (this->CanCastSpell(FuryWarriorSpells::SPELL_ONSLAUGHT, target))
                                {
                                    this->CastSpell(FuryWarriorSpells::SPELL_ONSLAUGHT, target);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        bot::ai::Action("Cast Onslaught", [this](Player* bot, Unit* target) {
                            if (this->CanCastSpell(FuryWarriorSpells::SPELL_ONSLAUGHT, target))
                            {
                                this->CastSpell(FuryWarriorSpells::SPELL_ONSLAUGHT, target);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        })
                    })
                })
            });

            behaviorTree->SetRoot(root);
            TC_LOG_INFO("module.playerbot", " FURY WARRIOR: BehaviorTree initialized with hierarchical combat flow");
        }
    }

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    // Enrage tracking
    bool _isEnraged;
    uint32 _enrageEndTime;

    // Buff tracking
    bool _hasWhirlwindBuff;
    uint32 _rampageStacks;

    // Timing tracking
    uint32 _lastBloodthirst;
    uint32 _lastRampage;

    // Combat state
    bool _executePhaseActive;
    bool _hasDualWield;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
