/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Retribution Paladin Specialization - REFACTORED
 *
 * This demonstrates how to migrate existing specializations to the new
 * template-based architecture, eliminating code duplication.
 *
 * BEFORE: 433 lines of code with duplicate UpdateCooldowns, CanUseAbility, etc.
 * AFTER: ~150 lines focusing ONLY on Retribution-specific logic
 */

#pragma once


#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "../CombatSpecializationTemplates.h"
#include "../ResourceTypes.h"
#include "../SpellValidation_WoW120_Part2.h"  // Central spell registry
#include "../HeroTalentDetector.h"

// Phase 5 Integration: Decision Systems
#include "../../Decision/ActionPriorityQueue.h"
#include "../../Decision/BehaviorTree.h"
#include "../../BotAI.h"

namespace Playerbot
{

// ============================================================================
// RETRIBUTION PALADIN SPELL ALIASES (WoW 12.0 - The War Within)
// Consolidated spell IDs from central registry - NO duplicates
// NOTE: Seals, Exorcism, Holy Wrath removed (don't exist in WoW 12.0)
// ============================================================================

namespace RetributionPaladinSpells
{
    // Holy Power Generators
    constexpr uint32 SPELL_BLADE_OF_JUSTICE    = WoW120Spells::Paladin::Retribution::BLADE_OF_JUSTICE;
    constexpr uint32 SPELL_CRUSADER_STRIKE     = WoW120Spells::Paladin::CRUSADER_STRIKE;
    constexpr uint32 SPELL_JUDGMENT            = WoW120Spells::Paladin::JUDGMENT;
    constexpr uint32 SPELL_WAKE_OF_ASHES       = WoW120Spells::Paladin::Retribution::WAKE_OF_ASHES;
    constexpr uint32 SPELL_HAMMER_OF_WRATH     = WoW120Spells::Paladin::HAMMER_OF_WRATH;

    // Holy Power Spenders
    constexpr uint32 SPELL_TEMPLARS_VERDICT    = WoW120Spells::Paladin::Retribution::TEMPLARS_VERDICT;
    constexpr uint32 SPELL_FINAL_VERDICT       = WoW120Spells::Paladin::Retribution::FINAL_VERDICT;
    constexpr uint32 SPELL_DIVINE_STORM        = WoW120Spells::Paladin::Retribution::DIVINE_STORM;
    constexpr uint32 SPELL_JUSTICARS_VENGEANCE = WoW120Spells::Paladin::Retribution::JUSTICARS_VENGEANCE;

    // Cooldowns
    constexpr uint32 SPELL_AVENGING_WRATH      = WoW120Spells::Paladin::AVENGING_WRATH;
    constexpr uint32 SPELL_CRUSADE             = WoW120Spells::Paladin::Retribution::CRUSADE;
    constexpr uint32 SPELL_EXECUTION_SENTENCE  = WoW120Spells::Paladin::Retribution::EXECUTION_SENTENCE;
    constexpr uint32 SPELL_FINAL_RECKONING     = WoW120Spells::Paladin::Retribution::FINAL_RECKONING;
    constexpr uint32 SPELL_SHIELD_OF_VENGEANCE = WoW120Spells::Paladin::Retribution::SHIELD_OF_VENGEANCE;

    // Utility
    constexpr uint32 SPELL_CONSECRATION        = WoW120Spells::Paladin::CONSECRATION;
    constexpr uint32 SPELL_HAMMER_OF_JUSTICE   = WoW120Spells::Paladin::HAMMER_OF_JUSTICE;
    constexpr uint32 SPELL_REBUKE              = WoW120Spells::Paladin::REBUKE;
    constexpr uint32 SPELL_BLESSING_OF_FREEDOM = WoW120Spells::Paladin::BLESSING_OF_FREEDOM;
    constexpr uint32 SPELL_DIVINE_SHIELD       = WoW120Spells::Paladin::DIVINE_SHIELD;

    // Auras
    constexpr uint32 SPELL_RETRIBUTION_AURA    = WoW120Spells::Paladin::RETRIBUTION_AURA;

    // Procs
    constexpr uint32 SPELL_ART_OF_WAR          = WoW120Spells::Paladin::Retribution::ART_OF_WAR;
    constexpr uint32 SPELL_DIVINE_PURPOSE      = WoW120Spells::Paladin::Retribution::DIVINE_PURPOSE_RET;
    constexpr uint32 SPELL_BLADE_OF_WRATH      = WoW120Spells::Paladin::Retribution::BLADE_OF_WRATH;
}
using namespace RetributionPaladinSpells;

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
/**
 * Refactored Retribution Paladin using template architecture
 *
 * Key improvements:
 * - Inherits from MeleeDpsSpecialization<ManaResource> for role defaults
 * - Automatically gets UpdateCooldowns, CanUseAbility, OnCombatStart/End
 * - Uses HolyPowerSystem as secondary resource
 * - Eliminates ~280 lines of duplicate code
 */
class RetributionPaladinRefactored : public MeleeDpsSpecialization<ManaResource>
{
public:
    using Base = MeleeDpsSpecialization<ManaResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::CanUseAbility;
    using Base::IsInMeleeRange;
    using Base::_resource;
    explicit RetributionPaladinRefactored(Player* bot)
        : MeleeDpsSpecialization<ManaResource>(bot)
        , _holyPower()
        , _hasArtOfWar(false)
        , _hasDivinePurpose(false)
        , _cooldowns()
    {
        // Register cooldowns for major abilities
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT:             {RET_AVENGING_WRATH, 120000, 1},
        // COMMENTED OUT:             {RET_CRUSADE, 120000, 1},
        // COMMENTED OUT:             {RET_EXECUTION_SENTENCE, 60000, 1},
        // COMMENTED OUT:             {RET_FINAL_RECKONING, 60000, 1}
        // COMMENTED OUT:         });

        // Initialize Holy Power system
        _holyPower.Initialize(bot);

        // Initialize Phase 5 systems
        InitializeRetributionMechanics();
    }

    // ========================================================================
    // CORE ROTATION - Only Retribution-specific logic
    // ========================================================================

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !target->IsHostileTo(this->GetBot()))
            return;

        // Lazy hero talent detection (once per bot lifetime until respec)
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Update procs and buffs
        CheckForProcs();

        // Execute priority rotation
        ExecutePriorityRotation(target);
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();

        // Maintain Retribution Aura (WoW 12.0)
        if (!bot->HasAura(SPELL_RETRIBUTION_AURA))
        {
            this->CastSpell(SPELL_RETRIBUTION_AURA, bot);
        }

        // Note: Seals removed in WoW 7.0 (Legion) - no longer exist
        // Blessings handled by group coordination
    }

protected:
    // ========================================================================
    // RETRIBUTION-SPECIFIC MECHANICS
    // ========================================================================

    /**
     * Execute abilities based on priority system (WoW 12.0)
     */
    void ExecutePriorityRotation(::Unit* target)
    {
        // Hero talent rotation branching
        if (_heroTalents.IsTree(HeroTalentTree::TEMPLAR))
        {
            // Templar: Radiant Glory procs empower Templar's Verdict
            // Shake the Heavens grants bonus damage after spenders
            if (this->GetBot()->HasAura(WoW120Spells::Paladin::Retribution::SHAKE_THE_HEAVENS))
            {
                // Empowered state: prioritize Templar's Verdict for amplified damage
                if (_holyPower.GetAvailable() >= 3 && this->CanUseAbility(SPELL_TEMPLARS_VERDICT))
                {
                    this->CastSpell(SPELL_TEMPLARS_VERDICT, target);
                    _holyPower.Consume(3);
                    return;
                }
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::HERALD_OF_THE_SUN))
        {
            // Herald of the Sun: Dawnlight empowers abilities with solar energy
            // Cast Dawnlight when available for HoT + damage amplification
            if (this->CanUseAbility(WoW120Spells::Paladin::Retribution::RET_DAWNLIGHT))
            {
                this->CastSpell(WoW120Spells::Paladin::Retribution::RET_DAWNLIGHT, target);
                return;
            }
            // Sun's Avatar for burst healing/damage during Avenging Wrath
            if (_heroTalents.IsTree(HeroTalentTree::HERALD_OF_THE_SUN) &&
                this->GetBot()->HasAura(SPELL_AVENGING_WRATH))
            {
                if (this->CanUseAbility(WoW120Spells::Paladin::Retribution::RET_SUNS_AVATAR))
                {
                    this->CastSpell(WoW120Spells::Paladin::Retribution::RET_SUNS_AVATAR, this->GetBot());
                    return;
                }
            }
        }

        // Hammer of Wrath (Execute phase or during Avenging Wrath)
        if (target->GetHealthPct() < 20.0f && this->CanUseAbility(SPELL_HAMMER_OF_WRATH))
        {
            this->CastSpell(SPELL_HAMMER_OF_WRATH, target);
            _holyPower.Generate(1);
            return;
        }

        // Templar's Verdict at 3+ Holy Power
        if (_holyPower.GetAvailable() >= 3 && this->CanUseAbility(SPELL_TEMPLARS_VERDICT))
        {
            this->CastSpell(SPELL_TEMPLARS_VERDICT, target);
            _holyPower.Consume(3);
            return;
        }

        // Divine Storm for AoE (3+ enemies)
        if (_holyPower.GetAvailable() >= 3 && this->GetEnemiesInRange(8.0f) >= 3)
        {
            if (this->CanUseAbility(SPELL_DIVINE_STORM))
            {
                this->CastSpell(SPELL_DIVINE_STORM, this->GetBot());
                _holyPower.Consume(3);
                return;
            }
        }

        // Blade of Justice - Primary Holy Power generator (WoW 12.0)
        if (this->CanUseAbility(SPELL_BLADE_OF_JUSTICE))
        {
            this->CastSpell(SPELL_BLADE_OF_JUSTICE, target);
            _holyPower.Generate(2);  // Blade of Justice generates 2 HP
            return;
        }

        // Wake of Ashes - AoE HP generator (WoW 12.0)
        if (_hasArtOfWar && this->CanUseAbility(SPELL_WAKE_OF_ASHES))
        {
            this->CastSpell(SPELL_WAKE_OF_ASHES, this->GetBot());
            _holyPower.Generate(3);  // Wake of Ashes generates 3 HP
            _hasArtOfWar = false;
            return;
        }

        // Judgment - Secondary HP generator
        if (this->CanUseAbility(SPELL_JUDGMENT))
        {
            this->CastSpell(SPELL_JUDGMENT, target);
            _holyPower.Generate(1);
            return;
        }

        // Crusader Strike - Filler HP generator
        if (this->CanUseAbility(SPELL_CRUSADER_STRIKE))
        {
            this->CastSpell(SPELL_CRUSADER_STRIKE, target);
            _holyPower.Generate(1);
            return;
        }

        // Consecration if in melee range
        if (this->IsInMeleeRange(target) && this->CanUseAbility(SPELL_CONSECRATION))
        {
            this->CastSpell(SPELL_CONSECRATION, this->GetBot());
            return;
        }
    }

    /**
     * Check for Retribution-specific procs (WoW 12.0)
     */
    void CheckForProcs()
    {
        Player* bot = this->GetBot();

        // Art of War proc (resets Blade of Justice cooldown)
        _hasArtOfWar = bot->HasAura(SPELL_ART_OF_WAR);

        // Divine Purpose proc (free 3 Holy Power ability)
        _hasDivinePurpose = bot->HasAura(SPELL_DIVINE_PURPOSE);
    }

    /**
     * NOTE: Seal twisting removed - Seals don't exist in WoW 12.0 (removed in Legion 7.0)
     * This function is kept as a stub for API compatibility.
     */
    void UpdateSealTwisting()
    {
        // No-op: Seals were removed from WoW in Legion 7.0
    }

    /**     * Determine if we should use offensive cooldowns
     */
    bool ShouldUseCooldowns(::Unit* target) const
    {
        // Use on bosses or when multiple enemies
        return (target->GetMaxHealth() > this->GetBot()->GetMaxHealth() * 10) ||               this->GetEnemiesInRange(10.0f) >= 3;
    }

    // ========================================================================
    // COMBAT LIFECYCLE HOOKS
    // ========================================================================

    void OnCombatStartSpecific(::Unit* target) override
    {
        // Hero talent detection on combat start
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Pop offensive cooldowns at start for burst (WoW 12.0)
        if (ShouldUseCooldowns(target))
        {
            Player* bot = this->GetBot();

            // Avenging Wrath or Crusade (talent replacement)
            if (this->CanUseAbility(SPELL_CRUSADE))
            {
                this->CastSpell(SPELL_CRUSADE, bot);
            }
            else if (this->CanUseAbility(SPELL_AVENGING_WRATH))
            {
                this->CastSpell(SPELL_AVENGING_WRATH, bot);
            }

            // Shield of Vengeance (Retribution defensive, replaces Guardian)
            if (this->CanUseAbility(SPELL_SHIELD_OF_VENGEANCE))
            {
                this->CastSpell(SPELL_SHIELD_OF_VENGEANCE, bot);
            }
        }

        // Reset Holy Power tracking
        _holyPower.Initialize(this->GetBot());
    }

    void OnCombatEndSpecific() override
    {
        // Reset proc tracking
        _hasArtOfWar = false;
        _hasDivinePurpose = false;
    }

    void InitializeRetributionMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        // ========================================================================
        // PHASE 5 INTEGRATION: ActionPriorityQueue (DPS + Holy Power Focus)
        // ========================================================================
        BotAI* ai = this;
        if (!ai)
            return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // ====================================================================
            // CRITICAL TIER - Holy Power spenders (burst damage)
            // ====================================================================
            queue->RegisterSpell(SPELL_TEMPLARS_VERDICT,
                SpellPriority::CRITICAL,
                SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SPELL_TEMPLARS_VERDICT,
                [this](Player* bot, Unit*) {
                    // Use TV when we have 3+ HP
                    return this->_holyPower.GetAvailable() >= 3;
                },
                "3+ HP (burst single target)");

            queue->RegisterSpell(SPELL_DIVINE_STORM,
                SpellPriority::CRITICAL,
                SpellCategory::DAMAGE_AOE);
            queue->AddCondition(SPELL_DIVINE_STORM,
                [this](Player* bot, Unit*) {
                    // AoE spender when 3+ HP and 3+ enemies
                    return this->_holyPower.GetAvailable() >= 3 &&
                           bot->getAttackers().size() >= 3;
                },
                "3+ HP and 3+ enemies (AoE burst)");

            // ====================================================================
            // HIGH TIER - Holy Power generators and execute
            // ====================================================================
            queue->RegisterSpell(SPELL_BLADE_OF_JUSTICE,
                SpellPriority::HIGH,
                SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SPELL_BLADE_OF_JUSTICE,
                [this](Player* bot, Unit*) {
                    // Primary HP generator (generates 2 HP)
                    return this->_holyPower.GetAvailable() < 4;
                },
                "HP < 4 (primary HP generation)");

            queue->RegisterSpell(SPELL_CRUSADER_STRIKE,
                SpellPriority::HIGH,
                SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SPELL_CRUSADER_STRIKE,
                [this](Player* bot, Unit*) {
                    // Generate HP when below max
                    return this->_holyPower.GetAvailable() < 5;
                },
                "HP < 5 (HP generation)");

            queue->RegisterSpell(SPELL_HAMMER_OF_WRATH,
                SpellPriority::HIGH,
                SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(SPELL_HAMMER_OF_WRATH,
                [](Player* bot, Unit* target) {
                    // Execute phase
                    return target && target->GetHealthPct() < 20.0f;
                },
                "Target < 20% (execute phase)");

            queue->RegisterSpell(SPELL_WAKE_OF_ASHES,
                SpellPriority::HIGH,
                SpellCategory::DAMAGE_AOE);
            queue->AddCondition(SPELL_WAKE_OF_ASHES,
                [this](Player* bot, Unit*) {
                    // AoE HP generator (generates 3 HP)
                    return this->_holyPower.GetAvailable() < 3;
                },
                "HP < 3 (burst HP generation)");

            // ====================================================================
            // MEDIUM TIER - Standard rotation
            // ====================================================================
            queue->RegisterSpell(SPELL_JUDGMENT,
                SpellPriority::MEDIUM,
                SpellCategory::DAMAGE_SINGLE);

            queue->RegisterSpell(SPELL_CONSECRATION,
                SpellPriority::MEDIUM,
                SpellCategory::DAMAGE_AOE);
            queue->AddCondition(SPELL_CONSECRATION,
                [](Player* bot, Unit* target) {
                    // Use when in melee range
                    return target && bot->IsWithinMeleeRange(target);
                },
                "In melee range");

            queue->RegisterSpell(SPELL_AVENGING_WRATH,
                SpellPriority::MEDIUM,
                SpellCategory::OFFENSIVE);
            queue->AddCondition(SPELL_AVENGING_WRATH,
                [](Player* bot, Unit* target) {
                    // Use on bosses or packs
                    return (target && target->GetMaxHealth() > 500000) ||
                           bot->getAttackers().size() >= 3;
                },
                "Boss or 3+ enemies (burst)");

            queue->RegisterSpell(SPELL_CRUSADE,
                SpellPriority::MEDIUM,
                SpellCategory::OFFENSIVE);
            queue->AddCondition(SPELL_CRUSADE,
                [](Player* bot, Unit* target) {
                    // Talent replacement for Avenging Wrath
                    return (target && target->GetMaxHealth() > 500000) ||
                           bot->getAttackers().size() >= 3;
                },
                "Boss or 3+ enemies (talent burst)");

            queue->RegisterSpell(SPELL_SHIELD_OF_VENGEANCE,
                SpellPriority::MEDIUM,
                SpellCategory::DEFENSIVE);
            queue->AddCondition(SPELL_SHIELD_OF_VENGEANCE,
                [](Player* bot, Unit*) {
                    // Absorb shield + damage
                    return bot->GetHealthPct() < 80.0f;
                },
                "HP < 80% (absorb + damage)");

            // ====================================================================
            // LOW TIER - Fillers and Utility
            // ====================================================================
            queue->RegisterSpell(SPELL_FINAL_RECKONING,
                SpellPriority::LOW,
                SpellCategory::DAMAGE_AOE);
            queue->AddCondition(SPELL_FINAL_RECKONING,
                [](Player* bot, Unit*) {
                    // AoE burst cooldown
                    return bot->getAttackers().size() >= 2;
                },
                "2+ enemies (AoE burst)");

            TC_LOG_INFO("module.playerbot", "  RETRIBUTION PALADIN: Registered {} spells in ActionPriorityQueue",
                queue->GetSpellCount());
        }

        // ========================================================================
        // PHASE 5 INTEGRATION: BehaviorTree (DPS + Holy Power Flow)
        // ========================================================================
        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Retribution Paladin DPS", {
                // ================================================================
                // TIER 1: EXECUTE PHASE (Target < 20%)
                // ================================================================
                Sequence("Execute Phase", {
                    Condition("Target < 20%", [](Player* bot, Unit* target) {
                        return target && target->GetHealthPct() < 20.0f;
                    }),
                    Selector("Execute Priority", {
                        // Hammer of Wrath (execute ability)
                        bot::ai::Action("Cast Hammer of Wrath", [this](Player* bot, Unit* target) {
                            if (this->CanCastSpell(SPELL_HAMMER_OF_WRATH, target))
                            {
                                this->CastSpell(SPELL_HAMMER_OF_WRATH, target);
                                this->_holyPower.Generate(1);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        }),
                        // Templar's Verdict if we have HP
                        bot::ai::Action("Cast Templar's Verdict", [this](Player* bot, Unit* target) {
                            if (this->_holyPower.GetAvailable() >= 3 &&
                                this->CanCastSpell(SPELL_TEMPLARS_VERDICT, target))
                            {
                                this->CastSpell(SPELL_TEMPLARS_VERDICT, target);
                                this->_holyPower.Consume(3);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        })
                    })
                }),

                // ================================================================
                // TIER 2: HOLY POWER MANAGEMENT
                // ================================================================
                Sequence("Holy Power Management", {
                    Selector("HP Generation and Spending", {
                        // Spend HP at 3+
                        Sequence("Spend Holy Power", {
                            Condition("HP >= 3", [this](Player* bot, Unit*) {
                                return this->_holyPower.GetAvailable() >= 3;
                            }),
                            Selector("HP Spender Priority", {
                                // Divine Storm for AoE
                                Sequence("Divine Storm AoE", {
                                    Condition("3+ enemies", [](Player* bot, Unit*) {
                                        return bot->getAttackers().size() >= 3;
                                    }),
                                    bot::ai::Action("Cast Divine Storm", [this](Player* bot, Unit*) {
                                        if (this->CanCastSpell(SPELL_DIVINE_STORM, bot))
                                        {
                                            this->CastSpell(SPELL_DIVINE_STORM, bot);
                                            this->_holyPower.Consume(3);
                                            return NodeStatus::SUCCESS;
                                        }
                                        return NodeStatus::FAILURE;
                                    })
                                }),
                                // Templar's Verdict single target
                                bot::ai::Action("Cast Templar's Verdict", [this](Player* bot, Unit* target) {
                                    if (this->CanCastSpell(SPELL_TEMPLARS_VERDICT, target))
                                    {
                                        this->CastSpell(SPELL_TEMPLARS_VERDICT, target);
                                        this->_holyPower.Consume(3);
                                        return NodeStatus::SUCCESS;
                                    }
                                    return NodeStatus::FAILURE;
                                })
                            })
                        }),
                        // Generate HP (WoW 12.0)
                        Sequence("Generate Holy Power", {
                            Condition("HP < 5", [this](Player* bot, Unit*) {
                                return this->_holyPower.GetAvailable() < 5;
                            }),
                            Selector("HP Generator Priority", {
                                // Blade of Justice (primary generator, 2 HP)
                                bot::ai::Action("Cast Blade of Justice", [this](Player* bot, Unit* target) {
                                    if (this->CanCastSpell(SPELL_BLADE_OF_JUSTICE, target))
                                    {
                                        this->CastSpell(SPELL_BLADE_OF_JUSTICE, target);
                                        this->_holyPower.Generate(2);
                                        return NodeStatus::SUCCESS;
                                    }
                                    return NodeStatus::FAILURE;
                                }),
                                // Wake of Ashes (AoE generator, 3 HP)
                                Sequence("Wake of Ashes", {
                                    Condition("HP < 3", [this](Player* bot, Unit*) {
                                        return this->_holyPower.GetAvailable() < 3;
                                    }),
                                    bot::ai::Action("Cast Wake of Ashes", [this](Player* bot, Unit*) {
                                        if (this->CanCastSpell(SPELL_WAKE_OF_ASHES, bot))
                                        {
                                            this->CastSpell(SPELL_WAKE_OF_ASHES, bot);
                                            this->_holyPower.Generate(3);
                                            return NodeStatus::SUCCESS;
                                        }
                                        return NodeStatus::FAILURE;
                                    })
                                }),
                                // Crusader Strike (filler generator)
                                bot::ai::Action("Cast Crusader Strike", [this](Player* bot, Unit* target) {
                                    if (this->CanCastSpell(SPELL_CRUSADER_STRIKE, target))
                                    {
                                        this->CastSpell(SPELL_CRUSADER_STRIKE, target);
                                        this->_holyPower.Generate(1);
                                        return NodeStatus::SUCCESS;
                                    }
                                    return NodeStatus::FAILURE;
                                })
                            })
                        })
                    })
                }),

                // ================================================================
                // TIER 3: COOLDOWN USAGE (WoW 12.0)
                // ================================================================
                Sequence("Use Cooldowns", {
                    Condition("Boss or pack", [this](Player* bot, Unit* target) {
                        return this->ShouldUseCooldowns(target);
                    }),
                    Selector("Cooldown Priority", {
                        // Crusade (talent, replaces Avenging Wrath)
                        bot::ai::Action("Cast Crusade", [this](Player* bot, Unit*) {
                            if (this->CanCastSpell(SPELL_CRUSADE, bot))
                            {
                                this->CastSpell(SPELL_CRUSADE, bot);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        }),
                        // Avenging Wrath (baseline)
                        bot::ai::Action("Cast Avenging Wrath", [this](Player* bot, Unit*) {
                            if (this->CanCastSpell(SPELL_AVENGING_WRATH, bot))
                            {
                                this->CastSpell(SPELL_AVENGING_WRATH, bot);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        }),
                        // Shield of Vengeance (Retribution defensive)
                        bot::ai::Action("Cast Shield of Vengeance", [this](Player* bot, Unit*) {
                            if (this->CanCastSpell(SPELL_SHIELD_OF_VENGEANCE, bot))
                            {
                                this->CastSpell(SPELL_SHIELD_OF_VENGEANCE, bot);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        })
                    })
                }),

                // ================================================================
                // TIER 4: STANDARD DPS ROTATION (WoW 12.0)
                // ================================================================
                Sequence("Standard Rotation", {
                    Selector("Rotation Priority", {
                        // Judgment (HP generator)
                        bot::ai::Action("Cast Judgment", [this](Player* bot, Unit* target) {
                            if (this->CanCastSpell(SPELL_JUDGMENT, target))
                            {
                                this->CastSpell(SPELL_JUDGMENT, target);
                                this->_holyPower.Generate(1);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        }),

                        // Consecration (melee range)
                        Sequence("Consecration", {
                            Condition("In melee range", [](Player* bot, Unit* target) {
                                return target && bot->IsWithinMeleeRange(target);
                            }),
                            bot::ai::Action("Cast Consecration", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(SPELL_CONSECRATION, bot))
                                {
                                    this->CastSpell(SPELL_CONSECRATION, bot);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),

                        // Final Reckoning (AoE burst talent)
                        Sequence("Final Reckoning", {
                            Condition("2+ enemies", [](Player* bot, Unit*) {
                                return bot->getAttackers().size() >= 2;
                            }),
                            bot::ai::Action("Cast Final Reckoning", [this](Player* bot, Unit* target) {
                                if (this->CanCastSpell(SPELL_FINAL_RECKONING, target))
                                {
                                    this->CastSpell(SPELL_FINAL_RECKONING, target);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                })
            });

            behaviorTree->SetRoot(root);
            TC_LOG_INFO("module.playerbot", " RETRIBUTION PALADIN: BehaviorTree initialized with DPS flow");
        }
    }

private:
    CooldownManager _cooldowns;
    // ========================================================================
    // SPELL IDs now defined in RetributionPaladinSpells namespace above
    // Legacy spells removed: Seals, Exorcism, Holy Wrath (don't exist in WoW 12.0)
    // ========================================================================

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    // Secondary resource system
    HolyPowerSystem _holyPower;

    // Proc tracking
    bool _hasArtOfWar;
    bool _hasDivinePurpose;
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
