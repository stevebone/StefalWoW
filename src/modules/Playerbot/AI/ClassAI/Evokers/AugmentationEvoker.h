/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Augmentation Evoker Refactored - Enterprise-Grade Header-Only Implementation
 *
 * Support specialization providing damage amplification buffs to allies.
 */

#pragma once

#include "../CombatSpecializationTemplates.h"
#include "../ResourceTypes.h"
#include "../SpellValidation_WoW120.h"
#include "../HeroTalentDetector.h"      // Hero talent tree detection
#include "Player.h"
#include "Group.h"
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
// AUGMENTATION EVOKER SPELL IDs (WoW 12.0 - The War Within)
// See central registry: WoW120Spells::Evoker and WoW120Spells::Evoker::Augmentation
// ============================================================================

enum AugmentationEvokerSpells
{
    // Core Buffs
    EBON_MIGHT           = WoW120Spells::Evoker::Augmentation::EBON_MIGHT,
    PRESCIENCE           = WoW120Spells::Evoker::Augmentation::PRESCIENCE,
    BLISTERING_SCALES    = WoW120Spells::Evoker::Augmentation::BLISTERING_SCALES,

    // Empowered Abilities
    BREATH_OF_EONS       = WoW120Spells::Evoker::Augmentation::BREATH_OF_EONS,

    // Essence Generation
    AZURE_STRIKE_AUG     = WoW120Spells::Evoker::AZURE_STRIKE,
    ERUPTION             = WoW120Spells::Evoker::Augmentation::ERUPTION,

    // Utility (shared with other Evoker specs)
    AUG_OBSIDIAN_SCALES  = WoW120Spells::Evoker::OBSIDIAN_SCALES,
    AUG_RENEWING_BLAZE   = WoW120Spells::Evoker::RENEWING_BLAZE,
    AUG_QUELL            = WoW120Spells::Evoker::QUELL,
    AUG_HOVER            = WoW120Spells::Evoker::HOVER
};

struct EssenceResourceAug
{
    uint32 essence{0};
    uint32 maxEssence{5};
    bool available{true};

    bool Consume(uint32 cost) { if (essence >= cost) { essence -= cost; return true; } return false; }
    void Regenerate(uint32) { available = true; }
    [[nodiscard]] uint32 GetAvailable() const { return essence; }
    [[nodiscard]] uint32 GetMax() const { return maxEssence; }
    void Initialize(Player* bot) { if (bot) { essence = 0; maxEssence = 5; } }
};

class AugmentationBuffTracker
{
public:
    ::std::unordered_map<ObjectGuid, uint32> _ebonMightTargets;
    ::std::unordered_map<ObjectGuid, uint32> _prescienceTargets;

    void ApplyEbonMight(ObjectGuid guid) { _ebonMightTargets[guid] = GameTime::GetGameTimeMS() + 10000; }
    void ApplyPrescience(ObjectGuid guid) { _prescienceTargets[guid] = GameTime::GetGameTimeMS() + 18000; }

    bool HasEbonMight(ObjectGuid guid) const
    {
        auto it = _ebonMightTargets.find(guid);
        return it != _ebonMightTargets.end() && GameTime::GetGameTimeMS() < it->second;
    }

    bool HasPrescience(ObjectGuid guid) const
    {
        auto it = _prescienceTargets.find(guid);
        return it != _prescienceTargets.end() && GameTime::GetGameTimeMS() < it->second;
    }

    void Update()
    {
        uint32 now = GameTime::GetGameTimeMS();
        for (auto it = _ebonMightTargets.begin(); it != _ebonMightTargets.end();)
            it = (now >= it->second) ? _ebonMightTargets.erase(it) : ++it;
        for (auto it = _prescienceTargets.begin(); it != _prescienceTargets.end();)
            it = (now >= it->second) ? _prescienceTargets.erase(it) : ++it;
    }
};

class AugmentationEvokerRefactored : public RangedDpsSpecialization<EssenceResourceAug>
{
public:
    using Base = RangedDpsSpecialization<EssenceResourceAug>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::GetEnemiesInRange;
    using Base::_resource;

    explicit AugmentationEvokerRefactored(Player* bot)
        : RangedDpsSpecialization<EssenceResourceAug>(bot)
        , _buffTracker()
    {
        // EssenceResourceAug::Initialize() is safe - only sets default values
        this->_resource.Initialize(bot);
        InitializeAugmentationMechanics();

        // Note: Do NOT call bot->GetName() here - Player data may not be loaded yet
        TC_LOG_DEBUG("playerbot", "AugmentationEvokerRefactored created for bot GUID: {}",
            bot ? bot->GetGUID().GetCounter() : 0);
    }

    void UpdateRotation(::Unit* target) override
    {
        if (!target) return;

        // Detect hero talents if not yet cached
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Hero talent rotation branches
        if (_heroTalents.IsTree(HeroTalentTree::CHRONOWARDEN))
        {
            // Chronowarden: Threads of Fate for enhanced ally buffs
            if (this->CanCastSpell(WoW120Spells::Evoker::Augmentation::THREADS_OF_FATE, target))
            {
                this->CastSpell(WoW120Spells::Evoker::Augmentation::THREADS_OF_FATE, target);
                return;
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::SCALECOMMANDER))
        {
            // Scalecommander: Extended Battle for prolonged ally buffs
            if (this->CanCastSpell(WoW120Spells::Evoker::Augmentation::EXTENDED_BATTLE, this->GetBot()))
            {
                this->CastSpell(WoW120Spells::Evoker::Augmentation::EXTENDED_BATTLE, this->GetBot());
                return;
            }
        }

        UpdateAugmentationState();

        // Priority 1: Maintain Ebon Might on DPS allies
        if (MaintainEbonMight()) return;

        // Priority 2: Maintain Prescience on top DPS
        if (MaintainPrescience()) return;

        // Priority 3: Use Breath of Eons
        if (this->_resource.essence >= 3 && this->CanCastSpell(BREATH_OF_EONS, target)) {
            this->CastSpell(BREATH_OF_EONS, target);
            this->_resource.Consume(3);
            return;
        }

        // Priority 4: Eruption AoE
        if (this->_resource.essence >= 3 && this->GetEnemiesInRange(25.0f) >= 2 && this->CanCastSpell(ERUPTION, target)) {
            this->CastSpell(ERUPTION, target);
            this->_resource.Consume(3);
            return;
        }

        // Priority 5: Generate essence
        if (this->_resource.essence < 4 && this->CanCastSpell(AZURE_STRIKE_AUG, target))
        {
            this->CastSpell(AZURE_STRIKE_AUG, target);
            this->_resource.essence = ::std::min<uint32>(this->_resource.essence + 2, this->_resource.maxEssence);
        }
    }

    void UpdateBuffs() override { /* Handled in UpdateRotation */ }

protected:
    bool MaintainEbonMight()
    {
        if (this->_resource.essence < 1) return false;

        ::std::vector<Unit*> allies = GetGroupDPS();
        for (Unit* ally : allies)
        {
            if (ally && !_buffTracker.HasEbonMight(ally->GetGUID()))
            {
                if (this->CanCastSpell(EBON_MIGHT, ally))
                {
                    this->CastSpell(EBON_MIGHT, ally);
                    this->_resource.Consume(1);
                    _buffTracker.ApplyEbonMight(ally->GetGUID());
                    return true;
                }
            }
        }
        return false;
    }

    bool MaintainPrescience()
    {
        if (this->_resource.essence < 1) return false;

        ::std::vector<Unit*> allies = GetGroupDPS();
        for (Unit* ally : allies)
        {
            if (ally && !_buffTracker.HasPrescience(ally->GetGUID()))
            {
                if (this->CanCastSpell(PRESCIENCE, ally))
                {
                    this->CastSpell(PRESCIENCE, ally);
                    this->_resource.Consume(1);
                    _buffTracker.ApplyPrescience(ally->GetGUID());
                    return true;
                }
            }
        }
        return false;
    }

    ::std::vector<Unit*> GetGroupDPS() const
    {
        ::std::vector<Unit*> dps;
        Player* bot = this->GetBot();
        if (!bot) return dps;

        Group* group = bot->GetGroup();
        if (!group) return dps;

        for (GroupReference const& ref : group->GetMembers())
        {
            if (Player* member = ref.GetSource()) {
                if (member->IsAlive() && bot->IsInMap(member) && !IsTankOrHealer(member))
                    dps.push_back(member);
            }
        }
        return dps;
    }

    bool IsTankOrHealer(Player* player) const
    {
        // Simple heuristic: check role or class
        uint8 cls = player->GetClass();
        return (cls == CLASS_PRIEST || cls == CLASS_PALADIN || cls == CLASS_DRUID ||
                cls == CLASS_SHAMAN || cls == CLASS_MONK);
    }

    void UpdateAugmentationState()
    {
        _buffTracker.Update();
        Player* bot = this->GetBot();
        if (bot)
            this->_resource.essence = bot->GetPower(POWER_ALTERNATE_POWER);
    }

    void InitializeAugmentationMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)

        auto* queue = this->GetActionPriorityQueue();
        if (queue)
        {
            queue->RegisterSpell(EBON_MIGHT, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
            queue->AddCondition(EBON_MIGHT, [this](Player*, Unit*) {
                if (this->_resource.essence < 1) return false;
                auto allies = this->GetGroupDPS();
                for (auto* a : allies) if (a && !_buffTracker.HasEbonMight(a->GetGUID())) return true;
                return false;
            }, "Ally without Ebon Might (10s damage buff)");

            queue->RegisterSpell(PRESCIENCE, SpellPriority::HIGH, SpellCategory::OFFENSIVE);
            queue->AddCondition(PRESCIENCE, [this](Player*, Unit*) {
                if (this->_resource.essence < 1) return false;
                auto allies = this->GetGroupDPS();
                for (auto* a : allies) if (a && !_buffTracker.HasPrescience(a->GetGUID())) return true;
                return false;
            }, "Ally without Prescience (18s crit buff)");

            queue->RegisterSpell(BREATH_OF_EONS, SpellPriority::HIGH, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(BREATH_OF_EONS, [this](Player*, Unit* target) {
                return target && this->_resource.essence >= 3;
            }, "3 essence (empowered damage + extend Ebon Might)");

            queue->RegisterSpell(ERUPTION, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);
            queue->AddCondition(ERUPTION, [this](Player*, Unit* target) {
                return target && this->_resource.essence >= 3 && this->GetEnemiesInRange(25.0f) >= 2;
            }, "3 essence, 2+ enemies (AoE damage)");

            queue->RegisterSpell(AZURE_STRIKE_AUG, SpellPriority::LOW, SpellCategory::DAMAGE_SINGLE);
            queue->AddCondition(AZURE_STRIKE_AUG, [this](Player*, Unit* target) {
                return target && this->_resource.essence < 4;
            }, "Essence < 4 (generates 2 essence)");

            queue->RegisterSpell(AUG_OBSIDIAN_SCALES, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
            queue->AddCondition(AUG_OBSIDIAN_SCALES, [](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 40.0f;
            }, "HP < 40% (30% dmg reduction)");
        }

        auto* tree = this->GetBehaviorTree();
        if (tree)
        {
            auto root = Selector("Augmentation Evoker Support", {
                Sequence("Maintain Buffs", {
                    Condition("Has essence", [this](Player*, Unit*) { return this->_resource.essence >= 1; }),
                    Selector("Apply buffs", {
                        Sequence("Ebon Might", {
                            bot::ai::Action("Cast Ebon Might", [this](Player*, Unit*) {
                                return this->MaintainEbonMight() ? NodeStatus::SUCCESS : NodeStatus::FAILURE;
                            })
                        }),
                        Sequence("Prescience", {
                            bot::ai::Action("Cast Prescience", [this](Player*, Unit*) {
                                return this->MaintainPrescience() ? NodeStatus::SUCCESS : NodeStatus::FAILURE;
                            })
                        })
                    })
                }),
                Sequence("Deal Damage", {
                    Condition("Has target", [this](Player* bot, Unit*) { return bot && bot->GetVictim(); }),
                    Condition("3+ essence", [this](Player*, Unit*) { return this->_resource.essence >= 3; }),
                    bot::ai::Action("Cast Breath of Eons", [this](Player* bot, Unit*) {
                        Unit* target = bot->GetVictim();
                        if (target && this->CanCastSpell(BREATH_OF_EONS, target))
                        {
                            this->CastSpell(BREATH_OF_EONS, target);
                            this->_resource.Consume(3);
                            return NodeStatus::SUCCESS;
                        }
                        return NodeStatus::FAILURE;
                    })
                }),
                Sequence("Generate Essence", {
                    Condition("Has target", [this](Player* bot, Unit*) { return bot && bot->GetVictim(); }),
                    Condition("< 4 essence", [this](Player*, Unit*) { return this->_resource.essence < 4; }),
                    bot::ai::Action("Cast Azure Strike", [this](Player* bot, Unit*) {
                        Unit* target = bot->GetVictim();
                        if (target && this->CanCastSpell(AZURE_STRIKE_AUG, target))
                        {
                            this->CastSpell(AZURE_STRIKE_AUG, target);
                            this->_resource.essence = ::std::min<uint32>(this->_resource.essence + 2, this->_resource.maxEssence);
                            return NodeStatus::SUCCESS;
                        }
                        return NodeStatus::FAILURE;
                    })
                })
            });

            tree->SetRoot(root);
        }
    }

private:
    AugmentationBuffTracker _buffTracker;

    // Hero talent detection cache (refreshed on combat start)
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
