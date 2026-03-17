/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PLAYERBOT_ARCANEMAGEREFACTORED_H
#define PLAYERBOT_ARCANEMAGEREFACTORED_H

#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "../SpellValidation_WoW120.h"
#include "../HeroTalentDetector.h"      // Hero talent tree detection
#include "Player.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Log.h"
#include <unordered_map>
#include "../CombatSpecializationTemplates.h"

// Phase 5 Integration: Decision Systems
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

// WoW 12.0 (The War Within) - Arcane Mage Spell IDs
// Use central spell registry (SpellValidation_WoW120.h)
namespace ArcaneMageSpells
{
    // Arcane Spec Spells
    constexpr uint32 ARCANE_BLAST       = WoW120Spells::Mage::Arcane::ARCANE_BLAST;
    constexpr uint32 ARCANE_MISSILES    = WoW120Spells::Mage::Arcane::ARCANE_MISSILES;
    constexpr uint32 ARCANE_BARRAGE     = WoW120Spells::Mage::Arcane::ARCANE_BARRAGE;
    constexpr uint32 ARCANE_SURGE       = WoW120Spells::Mage::Arcane::ARCANE_SURGE;
    constexpr uint32 ARCANE_ORB         = WoW120Spells::Mage::Arcane::ARCANE_ORB;
    constexpr uint32 EVOCATION          = WoW120Spells::Mage::Arcane::EVOCATION;
    constexpr uint32 TOUCH_OF_MAGE      = WoW120Spells::Mage::Arcane::TOUCH_OF_THE_MAGI;
    constexpr uint32 ARCANE_FAMILIAR    = WoW120Spells::Mage::Arcane::ARCANE_FAMILIAR;
    constexpr uint32 PRESENCE_OF_MIND   = WoW120Spells::Mage::Arcane::PRESENCE_OF_MIND;
    constexpr uint32 ARCANE_EXPLOSION   = WoW120Spells::Mage::Arcane::ARCANE_EXPLOSION;
    constexpr uint32 SUPERNOVA          = WoW120Spells::Mage::Arcane::SUPERNOVA;
    constexpr uint32 SHIFTING_POWER     = WoW120Spells::Mage::Arcane::SHIFTING_POWER;

    // Common Mage Spells
    constexpr uint32 ARCANE_INTELLECT   = WoW120Spells::Mage::ARCANE_INTELLECT;
    constexpr uint32 ICE_BLOCK          = WoW120Spells::Mage::ICE_BLOCK;
    constexpr uint32 MIRROR_IMAGE       = WoW120Spells::Mage::MIRROR_IMAGE;
    constexpr uint32 TIME_WARP          = WoW120Spells::Mage::TIME_WARP;
}

// Make spells available in the namespace
using namespace ArcaneMageSpells;

// Arcane Charge tracker (stacks 1-4)
class ArcaneChargeTracker
{
public:
    ArcaneChargeTracker() : _charges(0), _maxCharges(4) {}

    void AddCharge(uint32 amount = 1)
    {
        _charges = ::std::min(_charges + amount, _maxCharges);
    }

    void ClearCharges()
    {
        _charges = 0;
    }

    [[nodiscard]] uint32 GetCharges() const { return _charges; }
    [[nodiscard]] bool IsMaxCharges() const { return _charges >= _maxCharges; }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        // Sync with actual Arcane Charges buff
        if (Aura* aura = bot->GetAura(36032)) // Arcane Charges buff ID
            _charges = aura->GetStackAmount();
            else
            _charges = 0;
    }

private:
    uint32 _charges;
    uint32 _maxCharges;
};

// Clearcasting proc tracker (free Arcane Missiles)
class ClearcastingTracker
{
public:
    ClearcastingTracker() : _clearcastingActive(false), _clearcastingStacks(0), _clearcastingEndTime(0) {}

    void ActivateProc(uint32 stacks = 1)
    {
        _clearcastingActive = true;
        _clearcastingStacks = ::std::min(_clearcastingStacks + stacks, 3u); // Max 3 stacks
        _clearcastingEndTime = GameTime::GetGameTimeMS() + 15000; // 15 sec duration
    }

    void ConsumeProc()
    {
        if (_clearcastingStacks > 0)
            _clearcastingStacks--;

        if (_clearcastingStacks == 0)
            _clearcastingActive = false;
    }

    [[nodiscard]] bool IsActive() const
    {
        return _clearcastingActive && GameTime::GetGameTimeMS() < _clearcastingEndTime;
    }

    [[nodiscard]] uint32 GetStacks() const { return _clearcastingStacks; }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        if (Aura* aura = bot->GetAura(263725)) // Clearcasting buff ID
        {
            _clearcastingActive = true;
            _clearcastingStacks = aura->GetStackAmount();
            _clearcastingEndTime = GameTime::GetGameTimeMS() + aura->GetDuration();        }
        else
        {
            _clearcastingActive = false;
            _clearcastingStacks = 0;
        }
    }

private:
    bool _clearcastingActive;
    uint32 _clearcastingStacks;
    uint32 _clearcastingEndTime;
};

class ArcaneMageRefactored : public RangedDpsSpecialization<ManaResource>
{
public:
    using Base = RangedDpsSpecialization<ManaResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::IsSpellReady;
    using Base::GetEnemiesInRange;
    using Base::_resource;

    explicit ArcaneMageRefactored(Player* bot)        : RangedDpsSpecialization<ManaResource>(bot)
        , _chargeTracker()
        , _clearcastingTracker()
        , _arcaneSurgeActive(false)
        , _arcaneSurgeEndTime(0)
        , _cooldowns()
    {
        // Register cooldowns for major abilities
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT:             {ARCANE_SURGE, 120000, 1},  // 2 min major DPS cooldown
        // COMMENTED OUT:             {EVOCATION, 90000, 1},  // 90 sec mana recovery
        // COMMENTED OUT:             {PRESENCE_OF_MIND, 60000, 1},  // 1 min instant cast
        // COMMENTED OUT:             {ARCANE_ORB, 60000, 1},  // 1 min AoE builder
        // COMMENTED OUT:             {SHIFTING_POWER, 60000, 1},  // 1 min cooldown reset
        // COMMENTED OUT:             {ICE_BLOCK, 240000, 1},  // 4 min immunity
        // COMMENTED OUT:             {MIRROR_IMAGE, 120000, 1},  // 2 min defensive decoy
        // COMMENTED OUT:             {TIME_WARP, 600000, 1}  // 10 min Heroism/Bloodlust
        // COMMENTED OUT:         });

        // Phase 5 Integration: Initialize decision systems
        InitializeArcaneMechanics();

        TC_LOG_DEBUG("playerbot", "ArcaneMageRefactored initialized for bot {}", bot->GetGUID().GetCounter());
    }

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !this->GetBot())
            return;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero talent rotation branches
        if (_heroTalents.IsTree(HeroTalentTree::SPELLSLINGER))
        {
            // Spellslinger: Splinterstorm for empowered arcane barrages
            if (_chargeTracker.GetCharges() >= 3 &&
                this->CanCastSpell(WoW120Spells::Mage::Arcane::SPLINTERSTORM, target))
            {
                this->CastSpell(WoW120Spells::Mage::Arcane::SPLINTERSTORM, target);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::SUNFURY))
        {
            // Sunfury: Invocation of the Sunfury for amplified burst
            if (this->CanCastSpell(WoW120Spells::Mage::Arcane::INVOCATION_SUNFURY, this->GetBot()))
            {
                this->CastSpell(WoW120Spells::Mage::Arcane::INVOCATION_SUNFURY, this->GetBot());
                return;
            }
        }

        UpdateArcaneState();

        uint32 enemyCount = this->GetEnemiesInRange(40.0f);

        if (enemyCount >= 3)
            ExecuteAoERotation(target, enemyCount);
            else
            ExecuteSingleTargetRotation(target);
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // Arcane Intellect buff
        if (!bot->HasAura(ARCANE_INTELLECT))
        {
            if (this->CanCastSpell(ARCANE_INTELLECT, bot))
            {
                this->CastSpell(ARCANE_INTELLECT, bot);
            }
        }

        // Arcane Familiar (if talented)
        if (bot->HasSpell(ARCANE_FAMILIAR) && !bot->HasAura(ARCANE_FAMILIAR))
        {
            if (this->CanCastSpell(ARCANE_FAMILIAR, bot))
            {
                this->CastSpell(ARCANE_FAMILIAR, bot);
            }
        }
    }

    void UpdateDefensives()
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        float healthPct = bot->GetHealthPct();

        // Ice Block (critical emergency - immune)
        if (healthPct < 20.0f && this->CanCastSpell(ICE_BLOCK, bot))
        {
            this->CastSpell(ICE_BLOCK, bot);
            return;
        }

        // Mirror Image (defensive decoy)
        if (healthPct < 40.0f && this->CanCastSpell(MIRROR_IMAGE, bot))
        {
            this->CastSpell(MIRROR_IMAGE, bot);
            return;
        }

        // Shifting Power (reset cooldowns in emergency) - self-cast
        if (healthPct < 50.0f && this->CanCastSpell(SHIFTING_POWER, bot))
        {
            this->CastSpell(SHIFTING_POWER, bot);
            return;
        }
    }

private:
    

    void UpdateArcaneState()
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // Resource is managed by base template class
        _chargeTracker.Update(bot);
        _clearcastingTracker.Update(bot);
        UpdateCooldownStates();
    }

    void UpdateCooldownStates()
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // Arcane Surge state
        if (_arcaneSurgeActive && GameTime::GetGameTimeMS() >= _arcaneSurgeEndTime)
            _arcaneSurgeActive = false;

        if (bot->HasAura(ARCANE_SURGE))
        {
            _arcaneSurgeActive = true;
            if (Aura* aura = bot->GetAura(ARCANE_SURGE))                _arcaneSurgeEndTime = GameTime::GetGameTimeMS() + aura->GetDuration();        }
    }

    void ExecuteSingleTargetRotation(::Unit* target)
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        uint32 charges = _chargeTracker.GetCharges();
        uint32 manaPercent = bot->GetPowerPct(POWER_MANA);

        // Arcane Surge (major DPS cooldown at 4 charges)
        if (charges >= 4 && manaPercent >= 70 && !_arcaneSurgeActive)
        {
            if (this->CanCastSpell(ARCANE_SURGE, bot))
            {
                this->CastSpell(ARCANE_SURGE, bot);
                _arcaneSurgeActive = true;
                _arcaneSurgeEndTime = GameTime::GetGameTimeMS() + 15000; // 15 sec
                return;
            }
        }

        // Touch of the Magi (apply damage amplification debuff at 4 charges)
        if (charges >= 4 && bot->HasSpell(TOUCH_OF_MAGE))        {
            if (this->CanCastSpell(TOUCH_OF_MAGE, target))
            {
                this->CastSpell(TOUCH_OF_MAGE, target);
                return;
            }
        }

        // Arcane Missiles with Clearcasting proc (free cast, no mana cost)
        if (_clearcastingTracker.IsActive())
        {
            if (this->CanCastSpell(ARCANE_MISSILES, target))
            {
                this->CastSpell(ARCANE_MISSILES, target);
                _clearcastingTracker.ConsumeProc();
                return;
            }
        }

        // Arcane Barrage (spend charges when at max or low mana)
        if (charges >= 4 || (charges >= 2 && manaPercent < 30))
        {
            if (this->CanCastSpell(ARCANE_BARRAGE, target))
            {
                this->CastSpell(ARCANE_BARRAGE, target);
                _chargeTracker.ClearCharges();
                return;
            }
        }

        // Presence of Mind (instant cast Arcane Blast)
        if (charges < 4 && this->CanCastSpell(PRESENCE_OF_MIND, bot))
        {
            this->CastSpell(PRESENCE_OF_MIND, bot);
            // Follow up with instant Arcane Blast
            if (this->CanCastSpell(ARCANE_BLAST, target))
            {
                this->CastSpell(ARCANE_BLAST, target);
                _chargeTracker.AddCharge();
                return;
            }
        }

        // Arcane Blast (builder - generates charges)
        if (manaPercent > 20 || charges < 4)
        {
            if (this->CanCastSpell(ARCANE_BLAST, target))
            {
                this->CastSpell(ARCANE_BLAST, target);
                _chargeTracker.AddCharge();

                // Chance to proc Clearcasting
                if (rand() % 100 < 10) // 10% chance (simplified)
                    _clearcastingTracker.ActivateProc();

                return;
            }
        }

        // Evocation (emergency mana regen)
        if (manaPercent < 20)
        {
            if (this->CanCastSpell(EVOCATION, bot))
            {
                this->CastSpell(EVOCATION, bot);
                return;
            }
        }
    }

    void ExecuteAoERotation(::Unit* target, uint32 enemyCount)
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        uint32 charges = _chargeTracker.GetCharges();
        uint32 manaPercent = bot->GetPowerPct(POWER_MANA);

        // Arcane Surge for burst AoE
        if (charges >= 4 && manaPercent >= 70 && !_arcaneSurgeActive && enemyCount >= 4)
        {
            if (this->CanCastSpell(ARCANE_SURGE, bot))
            {
                this->CastSpell(ARCANE_SURGE, bot);
                _arcaneSurgeActive = true;
                _arcaneSurgeEndTime = GameTime::GetGameTimeMS() + 15000;
                return;
            }
        }

        // Arcane Orb (AoE builder)
        if (bot->HasSpell(ARCANE_ORB) && charges < 4)
        {
            if (this->CanCastSpell(ARCANE_ORB, target))
            {
                this->CastSpell(ARCANE_ORB, target);
                _chargeTracker.AddCharge(1);
                return;
            }
        }

        // Supernova (AoE damage and knockback)
        if (bot->HasSpell(SUPERNOVA) && enemyCount >= 3)
        {
            if (this->CanCastSpell(SUPERNOVA, target))
            {
                this->CastSpell(SUPERNOVA, target);
                return;
            }
        }

        // Arcane Barrage (AoE spender at max charges)
        if (charges >= 4)
        {
            if (this->CanCastSpell(ARCANE_BARRAGE, target))
            {
                this->CastSpell(ARCANE_BARRAGE, target);
                _chargeTracker.ClearCharges();
                return;
            }
        }

        // Arcane Missiles with Clearcasting
        if (_clearcastingTracker.IsActive())
        {
            if (this->CanCastSpell(ARCANE_MISSILES, target))
            {
                this->CastSpell(ARCANE_MISSILES, target);
                _clearcastingTracker.ConsumeProc();
                return;
            }
        }

        // Arcane Explosion (close-range AoE if enemies nearby)
        if (enemyCount >= 3 && this->GetEnemiesInRange(10.0f) >= 3)
        {
            if (this->CanCastSpell(ARCANE_EXPLOSION, bot))
            {
                this->CastSpell(ARCANE_EXPLOSION, bot);
                return;
            }
        }

        // Arcane Blast (builder)
        if (manaPercent > 20 || charges < 4)
        {
            if (this->CanCastSpell(ARCANE_BLAST, target))
            {
                this->CastSpell(ARCANE_BLAST, target);
                _chargeTracker.AddCharge();

                // Chance to proc Clearcasting
                if (rand() % 100 < 10) // 10% chance
                    _clearcastingTracker.ActivateProc();

                return;
            }
        }

        // Evocation for mana regen
        if (manaPercent < 20)
        {
            if (this->CanCastSpell(EVOCATION, bot))
            {
                this->CastSpell(EVOCATION, bot);
                return;
            }
        }
    }

    // Phase 5 Integration: Decision Systems Initialization
    void InitializeArcaneMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Defensive cooldowns
            queue->RegisterSpell(ICE_BLOCK, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(ICE_BLOCK, [this](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 20.0f;
            }, "Bot HP < 20% (immunity)");

            queue->RegisterSpell(MIRROR_IMAGE, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(MIRROR_IMAGE, [this](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 40.0f;
            }, "Bot HP < 40% (decoy)");

            // CRITICAL: Major burst cooldown
            queue->RegisterSpell(ARCANE_SURGE, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(ARCANE_SURGE, [this](Player* bot, Unit* target) {
                return target && this->_chargeTracker.GetCharges() >= 4 &&
                       bot->GetPowerPct(POWER_MANA) >= 70 && !this->_arcaneSurgeActive;
            }, "4 charges, 70%+ mana, not active (15s burst)");

            queue->RegisterSpell(TOUCH_OF_MAGE, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(TOUCH_OF_MAGE, [this](Player* bot, Unit* target) {
                return bot && bot->HasSpell(TOUCH_OF_MAGE) &&
                       target && this->_chargeTracker.GetCharges() >= 4;
            }, "Has talent, 4 charges (damage amplification)");

            // HIGH: Arcane Missiles with Clearcasting
            queue->RegisterSpell(ARCANE_MISSILES, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(ARCANE_MISSILES, [this](Player* bot, Unit* target) {
                return target && this->_clearcastingTracker.IsActive();
            }, "Clearcasting active (free cast, 3 charges)");

            queue->RegisterSpell(ARCANE_BARRAGE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(ARCANE_BARRAGE, [this](Player* bot, Unit* target) {
                return target && (this->_chargeTracker.GetCharges() >= 4 ||
                       (this->_chargeTracker.GetCharges() >= 2 && bot->GetPowerPct(POWER_MANA) < 30));
            }, "4 charges OR (2+ charges and mana < 30%)");

            // MEDIUM: Charge builders and mana regen
            queue->RegisterSpell(PRESENCE_OF_MIND, SpellPriority::MEDIUM, SpellCategory::OFFENSIVE);
            queue->AddCondition(PRESENCE_OF_MIND, [this](Player* bot, Unit* target) {
                return target && this->_chargeTracker.GetCharges() < 4;
            }, "< 4 charges (instant Arcane Blast)");

            queue->RegisterSpell(ARCANE_ORB, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(ARCANE_ORB, [this](Player* bot, Unit* target) {
                return bot && bot->HasSpell(ARCANE_ORB) &&
                       target && this->_chargeTracker.GetCharges() < 4;
            }, "Has talent, < 4 charges (AoE builder)");

            queue->RegisterSpell(ARCANE_BLAST, SpellPriority::MEDIUM, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(ARCANE_BLAST, [this](Player* bot, Unit* target) {
                return target && (bot->GetPowerPct(POWER_MANA) > 20 ||
                       this->_chargeTracker.GetCharges() < 4);
            }, "Mana > 20% OR < 4 charges (builder)");

            // LOW: Mana recovery
            queue->RegisterSpell(EVOCATION, SpellPriority::LOW, SpellCategory::UTILITY);
            queue->AddCondition(EVOCATION, [this](Player* bot, Unit*) {
                return bot && bot->GetPowerPct(POWER_MANA) < 20;
            }, "Mana < 20% (channel mana regen)");

            TC_LOG_INFO("module.playerbot", " ARCANE MAGE: Registered {} spells in ActionPriorityQueue", queue->GetSpellCount());
        }

        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Arcane Mage DPS", {
                // Tier 1: Burst Cooldowns (Arcane Surge, Touch of the Magi)
                Sequence("Burst Cooldowns", {
                    Condition("Target exists and 4 charges", [this](Player* bot, Unit* target) {
                        return target != nullptr && this->_chargeTracker.GetCharges() >= 4;
                    }),
                    Selector("Use Burst", {
                        Sequence("Cast Arcane Surge", {
                            Condition("70%+ mana, not active", [this](Player* bot, Unit*) {
                                return bot && bot->GetPowerPct(POWER_MANA) >= 70 &&
                                       !this->_arcaneSurgeActive;
                            }),
                            bot::ai::Action("Cast Arcane Surge", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(ARCANE_SURGE, bot))
                                {
                                    this->CastSpell(ARCANE_SURGE, bot);
                                    this->_arcaneSurgeActive = true;
                                    this->_arcaneSurgeEndTime = GameTime::GetGameTimeMS() + 15000;
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Cast Touch of the Magi", {
                            Condition("Has talent", [this](Player* bot, Unit*) {
                                return bot && bot->HasSpell(TOUCH_OF_MAGE);
                            }),
                            bot::ai::Action("Cast Touch of the Magi", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(TOUCH_OF_MAGE, target))
                                {
                                    this->CastSpell(TOUCH_OF_MAGE, target);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 2: Clearcasting Proc (Arcane Missiles)
                Sequence("Clearcasting Proc", {
                    Condition("Target exists and has proc", [this](Player* bot, Unit* target) {
                        return target != nullptr && this->_clearcastingTracker.IsActive();
                    }),
                    bot::ai::Action("Cast Arcane Missiles", [this](Player* bot, Unit* target) -> NodeStatus {
                        if (this->CanCastSpell(ARCANE_MISSILES, target))
                        {
                            this->CastSpell(ARCANE_MISSILES, target);
                            this->_clearcastingTracker.ConsumeProc();
                            return NodeStatus::SUCCESS;
                        }
                        return NodeStatus::FAILURE;
                    })
                }),

                // Tier 3: Charge Management (Arcane Barrage to spend)
                Sequence("Charge Management", {
                    Condition("Target exists", [this](Player* bot, Unit* target) {
                        return target != nullptr;
                    }),
                    Selector("Spend or Build", {
                        // Spend at 4 charges or low mana
                        Sequence("Spend Charges", {
                            Condition("4 charges OR (2+ charges and low mana)", [this](Player* bot, Unit*)
                            {
                                return this->_chargeTracker.GetCharges() >= 4 ||
                                       (this->_chargeTracker.GetCharges() >= 2 &&
                                        bot->GetPowerPct(POWER_MANA) < 30);
                            }),
                            bot::ai::Action("Cast Arcane Barrage", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(ARCANE_BARRAGE, target))
                                {
                                    this->CastSpell(ARCANE_BARRAGE, target);
                                    this->_chargeTracker.ClearCharges();
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        // Build with Presence of Mind
                        Sequence("Use Presence of Mind", {
                            Condition("< 4 charges", [this](Player* bot, Unit*) {
                                return this->_chargeTracker.GetCharges() < 4;
                            }),
                            bot::ai::Action("Cast Presence of Mind", [this](Player* bot, Unit* target) -> NodeStatus {
                                if (this->CanCastSpell(PRESENCE_OF_MIND, bot))
                                {
                                    this->CastSpell(PRESENCE_OF_MIND, bot);
                                    // Follow with instant Arcane Blast
                                    if (this->CanCastSpell(ARCANE_BLAST, target))
                                    {
                                        this->CastSpell(ARCANE_BLAST, target);
                                        this->_chargeTracker.AddCharge();
                                    }
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 4: Charge Builder (Arcane Blast)
                Sequence("Charge Builder", {
                    Condition("Target exists", [this](Player* bot, Unit* target) {
                        return target != nullptr &&
                               (bot->GetPowerPct(POWER_MANA) > 20 ||
                                this->_chargeTracker.GetCharges() < 4);
                    }),
                    bot::ai::Action("Cast Arcane Blast", [this](Player* bot, Unit* target) -> NodeStatus {
                        if (this->CanCastSpell(ARCANE_BLAST, target))
                        {
                            this->CastSpell(ARCANE_BLAST, target);
                            this->_chargeTracker.AddCharge();
                            // Chance to proc Clearcasting
                            if (rand() % 100 < 10)
                                this->_clearcastingTracker.ActivateProc();
                            return NodeStatus::SUCCESS;
                        }
                        return NodeStatus::FAILURE;
                    })
                })
            });

            behaviorTree->SetRoot(root);
            TC_LOG_INFO("module.playerbot", " ARCANE MAGE: BehaviorTree initialized with 4-tier DPS rotation");
        }
    }

    // Member variables
    ArcaneChargeTracker _chargeTracker;
    ClearcastingTracker _clearcastingTracker;

    bool _arcaneSurgeActive;
    uint32 _arcaneSurgeEndTime;

    CooldownManager _cooldowns;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot

#endif // PLAYERBOT_ARCANEMAGEREFACTORED_H
