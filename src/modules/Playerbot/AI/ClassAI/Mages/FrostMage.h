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

#ifndef PLAYERBOT_FROSTMAGEREFACTORED_H
#define PLAYERBOT_FROSTMAGEREFACTORED_H

#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "../SpellValidation_WoW120.h"
#include "../HeroTalentDetector.h"      // Hero talent tree detection
#include "Player.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include <unordered_map>
#include "Log.h"
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

// WoW 12.0 (The War Within) - Frost Mage Spell IDs
// Use central spell registry (SpellValidation_WoW120.h)
namespace FrostMageSpells
{
    // Frost Spec Spells
    constexpr uint32 FROST_FROSTBOLT              = WoW120Spells::Mage::FROSTBOLT;
    constexpr uint32 FROST_ICE_LANCE              = WoW120Spells::Mage::Frost::ICE_LANCE;
    constexpr uint32 FROST_FLURRY                 = WoW120Spells::Mage::Frost::FLURRY;
    constexpr uint32 FROST_FROZEN_ORB             = WoW120Spells::Mage::Frost::FROZEN_ORB;
    constexpr uint32 FROST_BLIZZARD               = WoW120Spells::Mage::Frost::BLIZZARD;
    constexpr uint32 FROST_COMET_STORM            = WoW120Spells::Mage::Frost::COMET_STORM;
    constexpr uint32 FROST_RAY_OF_FROST           = WoW120Spells::Mage::Frost::RAY_OF_FROST;
    constexpr uint32 FROST_GLACIAL_SPIKE          = WoW120Spells::Mage::Frost::GLACIAL_SPIKE;
    constexpr uint32 FROST_ICY_VEINS              = WoW120Spells::Mage::Frost::ICY_VEINS;
    constexpr uint32 FROST_CONE_OF_COLD           = WoW120Spells::Mage::Frost::CONE_OF_COLD;
    constexpr uint32 FROST_FREEZE                 = WoW120Spells::Mage::Frost::FREEZE;
    constexpr uint32 FROST_SUMMON_WATER_ELEMENTAL = WoW120Spells::Mage::Frost::SUMMON_WATER_ELEMENTAL;
    constexpr uint32 FROST_ICE_BARRIER            = WoW120Spells::Mage::Frost::ICE_BARRIER;

    // Common Mage Spells
    constexpr uint32 FROST_ICE_BLOCK              = WoW120Spells::Mage::ICE_BLOCK;
    constexpr uint32 FROST_MIRROR_IMAGE           = WoW120Spells::Mage::MIRROR_IMAGE;
    constexpr uint32 FROST_SHIFTING_POWER         = WoW120Spells::Mage::SHIFTING_POWER_COMMON;
}

// Make spells available in the namespace
using namespace FrostMageSpells;

// Fingers of Frost proc tracker (2 free Ice Lance charges)
class FingersOfFrostTracker
{
public:
    FingersOfFrostTracker() : _fofStacks(0), _fofEndTime(0) {}

    void ActivateProc(uint32 stacks = 1)
    {
        _fofStacks = ::std::min(_fofStacks + stacks, 2u); // Max 2 stacks
        _fofEndTime = GameTime::GetGameTimeMS() + 15000; // 15 sec duration
    }

    void ConsumeProc()
    {
        if (_fofStacks > 0)
            _fofStacks--;
    }

    [[nodiscard]] bool IsActive() const
    {
        return _fofStacks > 0 && GameTime::GetGameTimeMS() < _fofEndTime;
    }

    [[nodiscard]] uint32 GetStacks() const { return _fofStacks; }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        if (Aura* aura = bot->GetAura(44544)) // Fingers of Frost buff ID
        {
            _fofStacks = aura->GetStackAmount();
            _fofEndTime = GameTime::GetGameTimeMS() + aura->GetDuration();        }
        else
        {
            _fofStacks = 0;
        }
    }

private:
    CooldownManager _cooldowns;
    uint32 _fofStacks;
    uint32 _fofEndTime;
};

// Brain Freeze proc tracker (free instant Flurry)
class BrainFreezeTracker
{
public:
    BrainFreezeTracker() : _brainFreezeActive(false), _brainFreezeEndTime(0) {}

    void ActivateProc()
    {
        _brainFreezeActive = true;
        _brainFreezeEndTime = GameTime::GetGameTimeMS() + 15000; // 15 sec duration
    }

    void ConsumeProc()
    {
        _brainFreezeActive = false;
    }

    [[nodiscard]] bool IsActive() const
    {
        return _brainFreezeActive && GameTime::GetGameTimeMS() < _brainFreezeEndTime;
    }    void Update(Player* bot)
    {
        if (!bot)
            return;

        if (bot->HasAura(190446)) // Brain Freeze buff ID
        {
            _brainFreezeActive = true;
            if (Aura* aura = bot->GetAura(190446))
                _brainFreezeEndTime = GameTime::GetGameTimeMS() + aura->GetDuration();        }
        else
        {
            _brainFreezeActive = false;
        }
    }

private:
    bool _brainFreezeActive;
    uint32 _brainFreezeEndTime;
};

// Icicle tracker for Glacial Spike (requires 5 icicles)
class IcicleTracker
{
public:
    IcicleTracker() : _icicles(0), _maxIcicles(5) {}

    void AddIcicle(uint32 amount = 1)
    {
        _icicles = ::std::min(_icicles + amount, _maxIcicles);
    }

    void ConsumeIcicles()
    {
        _icicles = 0;
    }

    [[nodiscard]] uint32 GetIcicles() const { return _icicles; }
    [[nodiscard]] bool IsMaxIcicles() const { return _icicles >= _maxIcicles; }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        // Track icicles from Frostbolt casts
        // Real implementation would track Icicles aura stacks
        // Placeholder: maintain current count
    }

private:
    uint32 _icicles;
    uint32 _maxIcicles;
};

class FrostMageRefactored : public RangedDpsSpecialization<ManaResource>
{
public:
    using Base = RangedDpsSpecialization<ManaResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    explicit FrostMageRefactored(Player* bot)        : RangedDpsSpecialization<ManaResource>(bot)
        , _fofTracker()
        , _brainFreezeTracker()
        , _icicleTracker()
        , _icyVeinsActive(false)
        , _icyVeinsEndTime(0)
    {
        // Register cooldowns for major abilities
        // COMMENTED OUT:         _cooldowns.RegisterBatch({
        // COMMENTED OUT:             {FROST_ICY_VEINS, 180000, 1},
        // COMMENTED OUT:             {FROST_FROZEN_ORB, 60000, 1},
        // COMMENTED OUT:             {FROST_COMET_STORM, 30000, 1},
        // COMMENTED OUT:             {FROST_RAY_OF_FROST, 75000, 1},
        // COMMENTED OUT:             {FROST_SHIFTING_POWER, 60000, 1},
        // COMMENTED OUT:             {FROST_ICE_BLOCK, 240000, 1},
        // COMMENTED OUT:             {FROST_MIRROR_IMAGE, 120000, 1}
        // COMMENTED OUT:         });
        TC_LOG_DEBUG("playerbot", "FrostMageRefactored initialized for bot {}", bot->GetGUID().GetCounter());

        // Phase 5: Initialize decision systems
        InitializeFrostMechanics();
    }

    void UpdateRotation(::Unit* target) override
    {
        Player* bot = this->GetBot();
        if (!target || !bot)
            return;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero talent rotation branches
        if (_heroTalents.IsTree(HeroTalentTree::FROSTFIRE))
        {
            // Frostfire: Frostfire Bolt for enhanced hybrid damage
            if (this->CanCastSpell(WoW120Spells::Mage::Frost::FROST_FROSTFIRE_BOLT, target))
            {
                this->CastSpell(WoW120Spells::Mage::Frost::FROST_FROSTFIRE_BOLT, target);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::SPELLSLINGER))
        {
            // Spellslinger: Splinterstorm for empowered frost shards
            if (_fofTracker.IsActive() &&
                this->CanCastSpell(WoW120Spells::Mage::Frost::FROST_SPLINTERSTORM, target))
            {
                this->CastSpell(WoW120Spells::Mage::Frost::FROST_SPLINTERSTORM, target);
                return;
            }
        }

        UpdateFrostState();

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

        // Ice Barrier for absorb shield
        if (!bot->HasAura(FROST_ICE_BARRIER))
        {
            if (this->CanCastSpell(FROST_ICE_BARRIER, bot))
            {
                this->CastSpell(FROST_ICE_BARRIER, bot);
            }
        }

        // Summon Water Elemental (permanent pet)
        if (!bot->GetPet())
        {
            if (this->CanCastSpell(FROST_SUMMON_WATER_ELEMENTAL, bot))
            {
                this->CastSpell(FROST_SUMMON_WATER_ELEMENTAL, bot);
            }
        }
    }

    void UpdateDefensives()
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        float healthPct = bot->GetHealthPct();

        // Ice Block (critical emergency)
        if (healthPct < 20.0f && this->CanCastSpell(FROST_ICE_BLOCK, bot))
        {
            this->CastSpell(FROST_ICE_BLOCK, bot);
            return;
        }

        // Mirror Image (defensive decoy)
        if (healthPct < 40.0f && this->CanCastSpell(FROST_MIRROR_IMAGE, bot))
        {
            this->CastSpell(FROST_MIRROR_IMAGE, bot);
            return;
        }

        // Shifting Power (reset cooldowns)
        if (healthPct < 50.0f && this->CanCastSpell(FROST_SHIFTING_POWER, bot))
        {
            this->CastSpell(FROST_SHIFTING_POWER, bot);
            return;
        }
    }

private:
    

    void UpdateFrostState()
    {
        Player* bot = this->GetBot();
        // Resource (mana) is managed by the base template class automatically
        _fofTracker.Update(bot);
        _brainFreezeTracker.Update(bot);
        _icicleTracker.Update(bot);
        UpdateCooldownStates();
    }

    void UpdateCooldownStates()
    {
        Player* bot = this->GetBot();
        // Icy Veins state
        if (_icyVeinsActive && GameTime::GetGameTimeMS() >= _icyVeinsEndTime)
            _icyVeinsActive = false;

        if (bot->HasAura(FROST_ICY_VEINS))
        {
            _icyVeinsActive = true;
            if (Aura* aura = bot->GetAura(FROST_ICY_VEINS))
                _icyVeinsEndTime = GameTime::GetGameTimeMS() + aura->GetDuration();        }
    }

    void ExecuteSingleTargetRotation(::Unit* target)
    {
        Player* bot = this->GetBot();        // Icy Veins (major DPS cooldown)
        if (!_icyVeinsActive)
        {
            if (this->CanCastSpell(FROST_ICY_VEINS, bot))
            {
                this->CastSpell(FROST_ICY_VEINS, bot);
                _icyVeinsActive = true;
                _icyVeinsEndTime = GameTime::GetGameTimeMS() + 20000; // 20 sec
                _lastIcyVeinsTime = GameTime::GetGameTimeMS();
                return;
            }
        }

        // Frozen Orb (generates Fingers of Frost procs)
        if ((GameTime::GetGameTimeMS() - _lastFrozenOrbTime) >= 60000) // 60 sec CD
        {
            if (this->CanCastSpell(FROST_FROZEN_ORB, target))
            {
                this->CastSpell(FROST_FROZEN_ORB, target);
                _lastFrozenOrbTime = GameTime::GetGameTimeMS();
                _fofTracker.ActivateProc(2); // Generate 2 FoF procs
                return;
            }
        }

        // Glacial Spike with 5 icicles (if talented)        if (bot->HasSpell(FROST_GLACIAL_SPIKE) && _icicleTracker.IsMaxIcicles())
        {
            if (this->CanCastSpell(FROST_GLACIAL_SPIKE, target))
            {
                this->CastSpell(FROST_GLACIAL_SPIKE, target);
                _icicleTracker.ConsumeIcicles();
                return;
            }
        }

        // Ray of Frost with Icy Veins (if talented - channeled damage)        if (bot->HasSpell(FROST_RAY_OF_FROST) && _icyVeinsActive)
        {            if (this->CanCastSpell(FROST_RAY_OF_FROST, target))
            {
                this->CastSpell(FROST_RAY_OF_FROST, target);
                return;
            }
        }

        // Flurry with Brain Freeze proc (instant cast, Winter's Chill debuff)
        if (_brainFreezeTracker.IsActive())
        {
            if (this->CanCastSpell(FROST_FLURRY, target))
            {
                this->CastSpell(FROST_FLURRY, target);
                _brainFreezeTracker.ConsumeProc();
                // Follow up with Ice Lance while target has Winter's Chill
                if (this->CanCastSpell(FROST_ICE_LANCE, target))
                {
                    this->CastSpell(FROST_ICE_LANCE, target);
                }
                return;
            }
        }

        // Ice Lance with Fingers of Frost proc (free shatter damage)
        if (_fofTracker.IsActive())
        {
            if (this->CanCastSpell(FROST_ICE_LANCE, target))
            {
                this->CastSpell(FROST_ICE_LANCE, target);
                _fofTracker.ConsumeProc();
                return;
            }
        }

        // Comet Storm (if talented - burst damage)        if (bot->HasSpell(FROST_COMET_STORM))
        {
            if (this->CanCastSpell(FROST_COMET_STORM, target))
            {
                this->CastSpell(FROST_COMET_STORM, target);
                return;
            }
        }

        // Frostbolt (builder - generates icicles and procs)
        if (this->CanCastSpell(FROST_FROSTBOLT, target))
        {            this->CastSpell(FROST_FROSTBOLT, target);
            _icicleTracker.AddIcicle();

            // Chance to proc Brain Freeze
            if (rand() % 100 < 15) // 15% chance (simplified)
                _brainFreezeTracker.ActivateProc();

            return;
        }
    }

    void ExecuteAoERotation(::Unit* target, uint32 enemyCount)
    {
        Player* bot = this->GetBot();
        // Icy Veins for AoE burst
        if (!_icyVeinsActive && enemyCount >= 4)
        {
            if (this->CanCastSpell(FROST_ICY_VEINS, bot))
            {
                this->CastSpell(FROST_ICY_VEINS, bot);
                _icyVeinsActive = true;
                _icyVeinsEndTime = GameTime::GetGameTimeMS() + 20000;
                _lastIcyVeinsTime = GameTime::GetGameTimeMS();
                return;
            }
        }

        // Frozen Orb (AoE damage and FoF procs)
        if ((GameTime::GetGameTimeMS() - _lastFrozenOrbTime) >= 60000)
        {
            if (this->CanCastSpell(FROST_FROZEN_ORB, target))
            {
                this->CastSpell(FROST_FROZEN_ORB, target);
                _lastFrozenOrbTime = GameTime::GetGameTimeMS();
                _fofTracker.ActivateProc(2);
                return;
            }
        }

        // Comet Storm for AoE damage
        if (bot->HasSpell(FROST_COMET_STORM) && enemyCount >= 3)        {
            if (this->CanCastSpell(FROST_COMET_STORM, target))
            {
                this->CastSpell(FROST_COMET_STORM, target);
                return;
            }
        }

        // Blizzard (ground AoE)
        if (enemyCount >= 3)
        {
            if (this->CanCastSpell(FROST_BLIZZARD, target))
            {
                this->CastSpell(FROST_BLIZZARD, target);
                return;
            }
        }

        // Cone of Cold (close-range AoE)
        if (this->GetEnemiesInRange(12.0f) >= 3)
        {
            if (this->CanCastSpell(FROST_CONE_OF_COLD, target))
            {
                this->CastSpell(FROST_CONE_OF_COLD, target);
                return;
            }
        }

        // Flurry with Brain Freeze
        if (_brainFreezeTracker.IsActive())
        {
            if (this->CanCastSpell(FROST_FLURRY, target))
            {
                this->CastSpell(FROST_FLURRY, target);
                _brainFreezeTracker.ConsumeProc();
                return;
            }
        }

        // Ice Lance with Fingers of Frost (AoE shatter)
        if (_fofTracker.IsActive())
        {
            if (this->CanCastSpell(FROST_ICE_LANCE, target))
            {
                this->CastSpell(FROST_ICE_LANCE, target);
                _fofTracker.ConsumeProc();
                return;
            }
        }

        // Frostbolt as filler
        if (this->CanCastSpell(FROST_FROSTBOLT, target))
        {
            this->CastSpell(FROST_FROSTBOLT, target);
            _icicleTracker.AddIcicle();

            // Chance to proc Brain Freeze
            if (rand() % 100 < 15)
                _brainFreezeTracker.ActivateProc();

            return;
        }
    }

    [[nodiscard]] uint32 GetEnemiesInRange(float range) const
    {
        Player* bot = this->GetBot();
        if (!bot)
            return 0;

        uint32 count = 0;
        // Simplified enemy counting
        return ::std::min(count, 10u);
    }

    void InitializeFrostMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        BotAI* ai = this;
        if (!ai) return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // EMERGENCY: Defensive immunity
            queue->RegisterSpell(FROST_ICE_BLOCK, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(FROST_ICE_BLOCK, [this](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 20.0f;
            }, "Bot HP < 20% (immunity)");

            // CRITICAL: Major burst cooldown
            queue->RegisterSpell(FROST_ICY_VEINS, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(FROST_ICY_VEINS, [this](Player* bot, Unit* target) {
                return target && !this->_icyVeinsActive && bot->GetPowerPct(POWER_MANA) >= 70;
            }, "Major burst (20s, 30% haste), 70%+ mana");

            // CRITICAL: Frozen Orb generates procs and AoE
            queue->RegisterSpell(FROST_FROZEN_ORB, SpellPriority::CRITICAL, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(FROST_FROZEN_ORB, [this](Player*, Unit* target) {
                return target;
            }, "AoE damage + FoF procs");

            // HIGH: Glacial Spike at 5 icicles (if talented)
            queue->RegisterSpell(FROST_GLACIAL_SPIKE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(FROST_GLACIAL_SPIKE, [this](Player* bot, Unit* target) {
                return target && bot->HasSpell(FROST_GLACIAL_SPIKE) && this->_icicleTracker.IsMaxIcicles();
            }, "5 icicles (massive burst)");

            // HIGH: Flurry with Brain Freeze proc
            queue->RegisterSpell(FROST_FLURRY, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(FROST_FLURRY, [this](Player*, Unit* target) {
                return target && this->_brainFreezeTracker.IsActive();
            }, "Brain Freeze proc (instant, Winter's Chill)");

            // HIGH: Ice Lance with Fingers of Frost proc
            queue->RegisterSpell(FROST_ICE_LANCE, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(FROST_ICE_LANCE, [this](Player*, Unit* target) {
                return target && this->_fofTracker.IsActive();
            }, "FoF proc (shatter damage)");

            // MEDIUM: Comet Storm (if talented)
            queue->RegisterSpell(FROST_COMET_STORM, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(FROST_COMET_STORM, [this](Player* bot, Unit* target) {
                return target && bot->HasSpell(FROST_COMET_STORM);
            }, "AoE burst damage");

            // MEDIUM: Blizzard for AoE
            queue->RegisterSpell(FROST_BLIZZARD, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(FROST_BLIZZARD, [this](Player*, Unit* target) {
                return target && this->GetEnemiesInRange(40.0f) >= 3;
            }, "3+ enemies (ground AoE)");

            // LOW: Frostbolt (builder)
            queue->RegisterSpell(FROST_FROSTBOLT, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(FROST_FROSTBOLT, [](Player*, Unit* target) {
                return target != nullptr;
            }, "Builder (icicles + Brain Freeze procs)");
        }

        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Frost Mage DPS", {
                // Tier 1: Burst Cooldowns (Icy Veins, Frozen Orb)
                Sequence("Burst Cooldowns", {
                    Condition("In combat with target", [this](Player* bot, Unit*) {
                        return bot && bot->IsInCombat() && bot->GetVictim();
                    }),
                    Selector("Use burst abilities", {
                        Sequence("Icy Veins", {
                            Condition("Icy Veins ready", [this](Player* bot, Unit*) {
                                return bot && !this->_icyVeinsActive && bot->GetPowerPct(POWER_MANA) >= 70;
                            }),
                            bot::ai::Action("Cast Icy Veins", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(FROST_ICY_VEINS, bot))
                                {
                                    this->CastSpell(FROST_ICY_VEINS, bot);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Frozen Orb", {
                            Condition("Has target", [this](Player* bot, Unit*) {
                                return bot && bot->GetVictim();
                            }),
                            bot::ai::Action("Cast Frozen Orb", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(FROST_FROZEN_ORB, target))
                                {
                                    this->CastSpell(FROST_FROZEN_ORB, target);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 2: Proc Windows (Brain Freeze → Flurry → Ice Lance, Fingers of Frost → Ice Lance)
                Sequence("Proc Windows", {
                    Condition("Has target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim();
                    }),
                    Selector("Use procs", {
                        Sequence("Brain Freeze combo", {
                            Condition("Brain Freeze active", [this](Player*, Unit*) {
                                return this->_brainFreezeTracker.IsActive();
                            }),
                            bot::ai::Action("Cast Flurry then Ice Lance", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(FROST_FLURRY, target))
                                {
                                    this->CastSpell(FROST_FLURRY, target);
                                    // Follow up with Ice Lance
                                    if (this->CanCastSpell(FROST_ICE_LANCE, target))
                                        this->CastSpell(FROST_ICE_LANCE, target);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Fingers of Frost", {
                            Condition("FoF proc active", [this](Player*, Unit*) {
                                return this->_fofTracker.IsActive();
                            }),
                            bot::ai::Action("Cast Ice Lance", [this](Player* bot, Unit*) {
                                Unit* target = bot->GetVictim();
                                if (target && this->CanCastSpell(FROST_ICE_LANCE, target))
                                {
                                    this->CastSpell(FROST_ICE_LANCE, target);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        })
                    })
                }),

                // Tier 3: Icicle Spender (Glacial Spike at 5 icicles)
                Sequence("Icicle Spender", {
                    Condition("5 icicles ready", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim() && bot->HasSpell(FROST_GLACIAL_SPIKE) &&
                               this->_icicleTracker.IsMaxIcicles();
                    }),
                    bot::ai::Action("Cast Glacial Spike", [this](Player* bot, Unit*) {
                        Unit* target = bot->GetVictim();
                        if (target && this->CanCastSpell(FROST_GLACIAL_SPIKE, target))
                        {
                            this->CastSpell(FROST_GLACIAL_SPIKE, target);
                            return NodeStatus::SUCCESS;
                        }
                        return NodeStatus::FAILURE;
                    })
                }),

                // Tier 4: Builder (Frostbolt generates icicles and Brain Freeze procs)
                Sequence("Builder", {
                    Condition("Has target", [this](Player* bot, Unit*) {
                        return bot && bot->GetVictim();
                    }),
                    bot::ai::Action("Cast Frostbolt", [this](Player* bot, Unit*) {
                        Unit* target = bot->GetVictim();
                        if (target && this->CanCastSpell(FROST_FROSTBOLT, target))
                        {
                            this->CastSpell(FROST_FROSTBOLT, target);
                            return NodeStatus::SUCCESS;
                        }
                        return NodeStatus::FAILURE;
                    })
                })
            });

            behaviorTree->SetRoot(root);
        }
    }

    // Member variables
    FingersOfFrostTracker _fofTracker;
    BrainFreezeTracker _brainFreezeTracker;
    IcicleTracker _icicleTracker;

    bool _icyVeinsActive;
    uint32 _icyVeinsEndTime;
    uint32 _lastIcyVeinsTime;
    uint32 _lastFrozenOrbTime;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot

#endif // PLAYERBOT_FROSTMAGEREFACTORED_H
