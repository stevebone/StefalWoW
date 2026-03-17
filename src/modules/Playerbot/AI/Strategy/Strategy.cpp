/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "Strategy.h"
#include "BotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Group.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "QuestDef.h"
#include "DBCEnums.h"
#include "SharedDefines.h"
#include "Log.h"

namespace Playerbot
{

// Strategy implementation
Strategy::Strategy(::std::string const& name) : _name(name)
{
}

float Strategy::GetRelevance(BotAI* ai) const
{
    if (!ai)
        return 0.0f;

    StrategyRelevance relevance = CalculateRelevance(ai);
    return relevance.GetOverallRelevance();
}

StrategyRelevance Strategy::CalculateRelevance(BotAI* ai) const
{
    StrategyRelevance relevance;
    // Base implementation returns neutral relevance
    // Derived classes should override this
    return relevance;
}

void Strategy::AddAction(::std::string const& name, ::std::shared_ptr<Action> action)
{
    if (action)
        _actions[name] = action;
}

::std::shared_ptr<Action> Strategy::GetAction(::std::string const& name) const
{
    auto it = _actions.find(name);
    return (it != _actions.end()) ? it->second : nullptr;
}

::std::vector<::std::shared_ptr<Action>> Strategy::GetActions() const
{
    ::std::vector<::std::shared_ptr<Action>> actions;
    actions.reserve(_actions.size());

    for (auto const& pair : _actions)
        actions.push_back(pair.second);

    return actions;
}

void Strategy::AddTrigger(::std::shared_ptr<Trigger> trigger)
{
    if (trigger)
        _triggers.push_back(trigger);
}

::std::vector<::std::shared_ptr<Trigger>> Strategy::GetTriggers() const
{
    return _triggers;
}

void Strategy::AddValue(::std::string const& name, ::std::shared_ptr<Value> value)
{
    if (value)
        _values[name] = value;
}

::std::shared_ptr<Value> Strategy::GetValue(::std::string const& name) const
{
    auto it = _values.find(name);
    return (it != _values.end()) ? it->second : nullptr;
}

// CombatStrategy implementation
void CombatStrategy::InitializeActions()
{
    // Combat actions will be initialized by derived classes
}

void CombatStrategy::InitializeTriggers()
{
    // Combat triggers will be initialized by derived classes
}

float CombatStrategy::GetRelevance(BotAI* ai) const
{
    if (!ai)
        return 0.0f;

    StrategyRelevance relevance = CalculateRelevance(ai);

    // Combat strategies are more relevant when in combat or under threat
    Player* bot = ai->GetBot();
    if (bot && bot->IsInCombat())
        relevance.combatRelevance += 100.0f;

    if (bot && bot->getAttackers().size() > 0)
        relevance.combatRelevance += 50.0f;

    return relevance.GetOverallRelevance();
}

bool CombatStrategy::ShouldFlee(BotAI* ai) const
{
    if (!ai)
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    // Flee if health is critically low
    if (bot->GetHealthPct() < 15.0f)
        return true;

    // Flee if outnumbered significantly
    if (bot->getAttackers().size() > 3)
        return true;

    return false;
}

::Unit* CombatStrategy::SelectTarget(BotAI* ai) const
{
    if (!ai)
        return nullptr;

    Player* bot = ai->GetBot();
    if (!bot)
        return nullptr;

    // Priority: Current target if valid
    if (::Unit* currentTarget = bot->GetSelectedUnit())
    {
        if (currentTarget->IsAlive() && bot->IsValidAttackTarget(currentTarget))
            return currentTarget;
    }

    // Find nearest hostile target
    ::Unit* nearestEnemy = nullptr;
    float nearestDistanceSq = 30.0f * 30.0f; // 900.0f - Max combat range squared

    auto const& attackers = bot->getAttackers();
    for (::Unit* attacker : attackers)
    {
        if (!attacker || !attacker->IsAlive())
            continue;

        float distanceSq = bot->GetExactDistSq(attacker);
        if (distanceSq < nearestDistanceSq)
        {
            nearestDistanceSq = distanceSq;
            nearestEnemy = attacker;
        }
    }

    return nearestEnemy;
}

// NOTE: QuestStrategy implementation is in QuestStrategy.cpp

// SocialStrategy implementation
void SocialStrategy::InitializeActions()
{
    // Social actions will be initialized by derived classes
}

void SocialStrategy::InitializeTriggers()
{
    // Social triggers will be initialized by derived classes
}

float SocialStrategy::GetRelevance(BotAI* ai) const
{
    if (!ai)
        return 0.0f;

    StrategyRelevance relevance = CalculateRelevance(ai);

    // Social strategies are more relevant when grouped or near other players
    Player* bot = ai->GetBot();
    if (bot)
    {
        if (Group* group = bot->GetGroup())
        {
            relevance.socialRelevance += 30.0f;
            relevance.socialRelevance += group->GetMembersCount() * 10.0f;
        }

        // Check for nearby players using TrinityCore's Map API
        Map* map = bot->GetMap();
        if (map)
        {
            uint32 nearbyPlayerCount = 0;
            float maxRange = 30.0f; // 30 yard detection range
            float maxRangeSq = maxRange * maxRange; // 900.0f

            // Iterate through all players on the map
            Map::PlayerList const& players = map->GetPlayers();
            for (Map::PlayerList::const_iterator iter = players.begin(); iter != players.end(); ++iter)
            {
                Player* player = iter->GetSource();
                if (player && player != bot && player->IsInWorld())
                {
                    // Check if player is within range
    if (bot->GetExactDistSq(player) <= maxRangeSq)
                    {
                        ++nearbyPlayerCount;
                    }
                }
            }

            relevance.socialRelevance += nearbyPlayerCount * 5.0f;
        }
    }

    return relevance.GetOverallRelevance();
}

bool SocialStrategy::ShouldGroupWith(Player* player) const
{
    if (!player)
        return false;

    // Basic criteria for grouping
    // Could be expanded with more sophisticated logic
    return player->GetLevel() >= 10 && !player->GetGroup();
}

bool SocialStrategy::ShouldTrade(Player* player) const
{
    if (!player)
        return false;

    // Basic criteria for trading
    return player->GetLevel() >= 5;
}

::std::string SocialStrategy::GenerateResponse(::std::string const& message) const
{
    // Simple response generation
    // Could be expanded with more sophisticated AI
    if (message.find("hello") != ::std::string::npos ||
        message.find("hi") != ::std::string::npos)
        return "Hello there!";

    if (message.find("help") != ::std::string::npos)
        return "I'm here to help!";

    return "Interesting!";
}

// StrategyFactory implementation
StrategyFactory* StrategyFactory::instance()
{
    static StrategyFactory instance;
    return &instance;
}

void StrategyFactory::RegisterStrategy(::std::string const& name,
                                     ::std::function<::std::unique_ptr<Strategy>()> creator)
{
    _creators[name] = creator;
}

::std::unique_ptr<Strategy> StrategyFactory::CreateStrategy(::std::string const& name)
{
    auto it = _creators.find(name);
    if (it != _creators.end())
        return it->second();

    return nullptr;
}

::std::vector<::std::unique_ptr<Strategy>> StrategyFactory::CreateClassStrategies(uint8 classId, uint8 spec)
{
    ::std::vector<::std::unique_ptr<Strategy>> strategies;
    ChrSpecialization specEnum = static_cast<ChrSpecialization>(spec);

    // Helper lambda to determine role from spec
    auto getSpecRole = [](ChrSpecialization s) -> uint8 {
        // Returns: 0=Tank, 1=Healer, 2=DPS
        switch (s)
        {
            // Tank specs
            case ChrSpecialization::WarriorProtection:
            case ChrSpecialization::PaladinProtection:
            case ChrSpecialization::DeathKnightBlood:
            case ChrSpecialization::DruidGuardian:
            case ChrSpecialization::DemonHunterVengeance:
            case ChrSpecialization::MonkBrewmaster:
                return 0; // Tank

            // Healer specs
            case ChrSpecialization::PaladinHoly:
            case ChrSpecialization::PriestHoly:
            case ChrSpecialization::PriestDiscipline:
            case ChrSpecialization::ShamanRestoration:
            case ChrSpecialization::DruidRestoration:
            case ChrSpecialization::MonkMistweaver:
            case ChrSpecialization::EvokerPreservation:
                return 1; // Healer

            // All other specs are DPS
            default:
                return 2; // DPS
        }
    };

    uint8 role = getSpecRole(specEnum);

    // ============================================
    // ROLE-BASED STRATEGY CLASSES
    // ============================================

    // Tank Strategy - Threat generation, positioning, damage mitigation
    class TankStrategy : public CombatStrategy
    {
    public:
        TankStrategy() : CombatStrategy("tank_strategy") { _priority = 200; }

        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        float GetRelevance(BotAI* ai) const override
        {
            if (!ai)
                return 0.0f;
            // High relevance when enemies are present
            return CombatStrategy::GetRelevance(ai) * 1.5f;
        }

        float GetThreatModifier() const override { return 3.0f; }

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 1.5f;
            rel.survivalRelevance = 0.8f;
            return rel;
        }
    };

    // Healer Strategy - Healing priority, mana management, positioning
    class HealerStrategy : public CombatStrategy
    {
    public:
        HealerStrategy() : CombatStrategy("healer_strategy") { _priority = 200; }

        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        float GetRelevance(BotAI* ai) const override
        {
            if (!ai)
                return 0.0f;
            return CombatStrategy::GetRelevance(ai) * 1.2f;
        }

        float GetThreatModifier() const override { return 0.5f; }

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai);
            rel.survivalRelevance = 1.0f;
            return rel;
        }
    };

    // DPS Strategy - Damage optimization, threat management
    class DPSStrategy : public CombatStrategy
    {
    public:
        DPSStrategy() : CombatStrategy("dps_strategy") { _priority = 150; }

        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        float GetRelevance(BotAI* ai) const override
        {
            if (!ai)
                return 0.0f;
            return CombatStrategy::GetRelevance(ai);
        }

        float GetThreatModifier() const override { return 1.0f; }
    };

    // Add role-specific strategy
    switch (role)
    {
        case 0: // Tank
            strategies.push_back(::std::make_unique<TankStrategy>());
            break;
        case 1: // Healer
            strategies.push_back(::std::make_unique<HealerStrategy>());
            break;
        case 2: // DPS
        default:
            strategies.push_back(::std::make_unique<DPSStrategy>());
            break;
    }

    // ============================================
    // CLASS-SPECIFIC STRATEGY CLASSES
    // ============================================

    switch (classId)
    {
        case CLASS_WARRIOR:
        {
            // Warrior-specific strategies
            class WarriorRotationStrategy : public CombatStrategy
            {
            public:
                WarriorRotationStrategy() : CombatStrategy("warrior_rotation") { _priority = 180; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<WarriorRotationStrategy>());

            // Spec-specific
            if (specEnum == ChrSpecialization::WarriorArms)
            {
                class ArmsExecuteStrategy : public CombatStrategy
                {
                public:
                    ArmsExecuteStrategy() : CombatStrategy("arms_execute_phase") { _priority = 190; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<ArmsExecuteStrategy>());
            }
            else if (specEnum == ChrSpecialization::WarriorFury)
            {
                class FuryEnrageStrategy : public CombatStrategy
                {
                public:
                    FuryEnrageStrategy() : CombatStrategy("fury_enrage_management") { _priority = 190; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<FuryEnrageStrategy>());
            }
            else if (specEnum == ChrSpecialization::WarriorProtection)
            {
                class ProtThreatStrategy : public CombatStrategy
                {
                public:
                    ProtThreatStrategy() : CombatStrategy("prot_threat_management") { _priority = 195; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                    float GetThreatModifier() const override { return 4.0f; }
                };
                strategies.push_back(::std::make_unique<ProtThreatStrategy>());
            }
            break;
        }

        case CLASS_PALADIN:
        {
            class PaladinRotationStrategy : public CombatStrategy
            {
            public:
                PaladinRotationStrategy() : CombatStrategy("paladin_rotation") { _priority = 180; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<PaladinRotationStrategy>());

            if (specEnum == ChrSpecialization::PaladinHoly)
            {
                class HolyPaladinHealingStrategy : public CombatStrategy
                {
                public:
                    HolyPaladinHealingStrategy() : CombatStrategy("holy_paladin_healing") { _priority = 200; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<HolyPaladinHealingStrategy>());
            }
            else if (specEnum == ChrSpecialization::PaladinProtection)
            {
                class ProtPaladinTankStrategy : public CombatStrategy
                {
                public:
                    ProtPaladinTankStrategy() : CombatStrategy("prot_paladin_tank") { _priority = 195; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                    float GetThreatModifier() const override { return 3.5f; }
                };
                strategies.push_back(::std::make_unique<ProtPaladinTankStrategy>());
            }
            else if (specEnum == ChrSpecialization::PaladinRetribution)
            {
                class RetPaladinBurstStrategy : public CombatStrategy
                {
                public:
                    RetPaladinBurstStrategy() : CombatStrategy("ret_paladin_burst") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<RetPaladinBurstStrategy>());
            }
            break;
        }

        case CLASS_HUNTER:
        {
            class HunterRotationStrategy : public CombatStrategy
            {
            public:
                HunterRotationStrategy() : CombatStrategy("hunter_rotation") { _priority = 180; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<HunterRotationStrategy>());

            // Pet management strategy for all hunter specs
            class HunterPetStrategy : public CombatStrategy
            {
            public:
                HunterPetStrategy() : CombatStrategy("hunter_pet_management") { _priority = 175; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<HunterPetStrategy>());

            if (specEnum == ChrSpecialization::HunterBeastMastery)
            {
                class BMHunterPetFocusStrategy : public CombatStrategy
                {
                public:
                    BMHunterPetFocusStrategy() : CombatStrategy("bm_hunter_pet_focus") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<BMHunterPetFocusStrategy>());
            }
            else if (specEnum == ChrSpecialization::HunterMarksmanship)
            {
                class MMHunterPrecisionStrategy : public CombatStrategy
                {
                public:
                    MMHunterPrecisionStrategy() : CombatStrategy("mm_hunter_precision") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<MMHunterPrecisionStrategy>());
            }
            else if (specEnum == ChrSpecialization::HunterSurvival)
            {
                class SurvivalHunterMeleeStrategy : public CombatStrategy
                {
                public:
                    SurvivalHunterMeleeStrategy() : CombatStrategy("survival_hunter_melee") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<SurvivalHunterMeleeStrategy>());
            }
            break;
        }

        case CLASS_ROGUE:
        {
            class RogueRotationStrategy : public CombatStrategy
            {
            public:
                RogueRotationStrategy() : CombatStrategy("rogue_rotation") { _priority = 180; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<RogueRotationStrategy>());

            // Stealth management for all rogues
            class RogueStealthStrategy : public CombatStrategy
            {
            public:
                RogueStealthStrategy() : CombatStrategy("rogue_stealth_management") { _priority = 190; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<RogueStealthStrategy>());

            if (specEnum == ChrSpecialization::RogueAssassination)
            {
                class AssassinationPoisonStrategy : public CombatStrategy
                {
                public:
                    AssassinationPoisonStrategy() : CombatStrategy("assassination_poison") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<AssassinationPoisonStrategy>());
            }
            else if (specEnum == ChrSpecialization::RogueOutlaw)
            {
                class OutlawComboStrategy : public CombatStrategy
                {
                public:
                    OutlawComboStrategy() : CombatStrategy("outlaw_combo_management") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<OutlawComboStrategy>());
            }
            else if (specEnum == ChrSpecialization::RogueSubtely)
            {
                class SubtletyShadowStrategy : public CombatStrategy
                {
                public:
                    SubtletyShadowStrategy() : CombatStrategy("subtlety_shadow_dance") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<SubtletyShadowStrategy>());
            }
            break;
        }

        case CLASS_PRIEST:
        {
            class PriestRotationStrategy : public CombatStrategy
            {
            public:
                PriestRotationStrategy() : CombatStrategy("priest_rotation") { _priority = 180; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<PriestRotationStrategy>());

            if (specEnum == ChrSpecialization::PriestDiscipline)
            {
                class DiscPriestAtonementStrategy : public CombatStrategy
                {
                public:
                    DiscPriestAtonementStrategy() : CombatStrategy("disc_priest_atonement") { _priority = 200; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<DiscPriestAtonementStrategy>());
            }
            else if (specEnum == ChrSpecialization::PriestHoly)
            {
                class HolyPriestHealingStrategy : public CombatStrategy
                {
                public:
                    HolyPriestHealingStrategy() : CombatStrategy("holy_priest_healing") { _priority = 200; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<HolyPriestHealingStrategy>());
            }
            else if (specEnum == ChrSpecialization::PriestShadow)
            {
                class ShadowPriestDPSStrategy : public CombatStrategy
                {
                public:
                    ShadowPriestDPSStrategy() : CombatStrategy("shadow_priest_dps") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<ShadowPriestDPSStrategy>());
            }
            break;
        }

        case CLASS_DEATH_KNIGHT:
        {
            class DeathKnightRotationStrategy : public CombatStrategy
            {
            public:
                DeathKnightRotationStrategy() : CombatStrategy("dk_rotation") { _priority = 180; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<DeathKnightRotationStrategy>());

            // Rune management for all DKs
            class DKRuneStrategy : public CombatStrategy
            {
            public:
                DKRuneStrategy() : CombatStrategy("dk_rune_management") { _priority = 185; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<DKRuneStrategy>());

            if (specEnum == ChrSpecialization::DeathKnightBlood)
            {
                class BloodDKTankStrategy : public CombatStrategy
                {
                public:
                    BloodDKTankStrategy() : CombatStrategy("blood_dk_tank") { _priority = 195; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                    float GetThreatModifier() const override { return 4.0f; }
                };
                strategies.push_back(::std::make_unique<BloodDKTankStrategy>());
            }
            else if (specEnum == ChrSpecialization::DeathKnightFrost)
            {
                class FrostDKBurstStrategy : public CombatStrategy
                {
                public:
                    FrostDKBurstStrategy() : CombatStrategy("frost_dk_burst") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<FrostDKBurstStrategy>());
            }
            else if (specEnum == ChrSpecialization::DeathKnightUnholy)
            {
                class UnholyDKPetStrategy : public CombatStrategy
                {
                public:
                    UnholyDKPetStrategy() : CombatStrategy("unholy_dk_pet") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<UnholyDKPetStrategy>());
            }
            break;
        }

        case CLASS_SHAMAN:
        {
            class ShamanRotationStrategy : public CombatStrategy
            {
            public:
                ShamanRotationStrategy() : CombatStrategy("shaman_rotation") { _priority = 180; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<ShamanRotationStrategy>());

            // Totem management for all shamans
            class ShamanTotemStrategy : public CombatStrategy
            {
            public:
                ShamanTotemStrategy() : CombatStrategy("shaman_totem_management") { _priority = 175; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<ShamanTotemStrategy>());

            if (specEnum == ChrSpecialization::ShamanElemental)
            {
                class ElementalShamanDPSStrategy : public CombatStrategy
                {
                public:
                    ElementalShamanDPSStrategy() : CombatStrategy("elemental_shaman_dps") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<ElementalShamanDPSStrategy>());
            }
            else if (specEnum == ChrSpecialization::ShamanEnhancement)
            {
                class EnhancementShamanMeleeStrategy : public CombatStrategy
                {
                public:
                    EnhancementShamanMeleeStrategy() : CombatStrategy("enhancement_shaman_melee") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<EnhancementShamanMeleeStrategy>());
            }
            else if (specEnum == ChrSpecialization::ShamanRestoration)
            {
                class RestoShamanHealingStrategy : public CombatStrategy
                {
                public:
                    RestoShamanHealingStrategy() : CombatStrategy("resto_shaman_healing") { _priority = 200; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<RestoShamanHealingStrategy>());
            }
            break;
        }

        case CLASS_MAGE:
        {
            class MageRotationStrategy : public CombatStrategy
            {
            public:
                MageRotationStrategy() : CombatStrategy("mage_rotation") { _priority = 180; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<MageRotationStrategy>());

            if (specEnum == ChrSpecialization::MageArcane)
            {
                class ArcaneMageManaStrategy : public CombatStrategy
                {
                public:
                    ArcaneMageManaStrategy() : CombatStrategy("arcane_mage_mana") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<ArcaneMageManaStrategy>());
            }
            else if (specEnum == ChrSpecialization::MageFire)
            {
                class FireMageCombustionStrategy : public CombatStrategy
                {
                public:
                    FireMageCombustionStrategy() : CombatStrategy("fire_mage_combustion") { _priority = 190; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<FireMageCombustionStrategy>());
            }
            else if (specEnum == ChrSpecialization::MageFrost)
            {
                class FrostMageShatterStrategy : public CombatStrategy
                {
                public:
                    FrostMageShatterStrategy() : CombatStrategy("frost_mage_shatter") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<FrostMageShatterStrategy>());
            }
            break;
        }

        case CLASS_WARLOCK:
        {
            class WarlockRotationStrategy : public CombatStrategy
            {
            public:
                WarlockRotationStrategy() : CombatStrategy("warlock_rotation") { _priority = 180; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<WarlockRotationStrategy>());

            // Pet management for all warlocks
            class WarlockPetStrategy : public CombatStrategy
            {
            public:
                WarlockPetStrategy() : CombatStrategy("warlock_pet_management") { _priority = 175; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<WarlockPetStrategy>());

            if (specEnum == ChrSpecialization::WarlockAffliction)
            {
                class AfflictionDotStrategy : public CombatStrategy
                {
                public:
                    AfflictionDotStrategy() : CombatStrategy("affliction_dot_management") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<AfflictionDotStrategy>());
            }
            else if (specEnum == ChrSpecialization::WarlockDemonology)
            {
                class DemonologyDemonStrategy : public CombatStrategy
                {
                public:
                    DemonologyDemonStrategy() : CombatStrategy("demonology_demon_management") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<DemonologyDemonStrategy>());
            }
            else if (specEnum == ChrSpecialization::WarlockDestruction)
            {
                class DestructionChaosBoltStrategy : public CombatStrategy
                {
                public:
                    DestructionChaosBoltStrategy() : CombatStrategy("destruction_chaos_bolt") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<DestructionChaosBoltStrategy>());
            }
            break;
        }

        // Note: CLASS_MONK (10) is intentionally excluded per requirements

        case CLASS_DRUID:
        {
            class DruidRotationStrategy : public CombatStrategy
            {
            public:
                DruidRotationStrategy() : CombatStrategy("druid_rotation") { _priority = 180; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<DruidRotationStrategy>());

            // Shapeshifting strategy for all druids
            class DruidShapeshiftStrategy : public CombatStrategy
            {
            public:
                DruidShapeshiftStrategy() : CombatStrategy("druid_shapeshift_management") { _priority = 185; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<DruidShapeshiftStrategy>());

            if (specEnum == ChrSpecialization::DruidBalance)
            {
                class BalanceDruidEclipseStrategy : public CombatStrategy
                {
                public:
                    BalanceDruidEclipseStrategy() : CombatStrategy("balance_druid_eclipse") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<BalanceDruidEclipseStrategy>());
            }
            else if (specEnum == ChrSpecialization::DruidFeral)
            {
                class FeralDruidBleedStrategy : public CombatStrategy
                {
                public:
                    FeralDruidBleedStrategy() : CombatStrategy("feral_druid_bleed") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<FeralDruidBleedStrategy>());
            }
            else if (specEnum == ChrSpecialization::DruidGuardian)
            {
                class GuardianDruidTankStrategy : public CombatStrategy
                {
                public:
                    GuardianDruidTankStrategy() : CombatStrategy("guardian_druid_tank") { _priority = 195; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                    float GetThreatModifier() const override { return 3.5f; }
                };
                strategies.push_back(::std::make_unique<GuardianDruidTankStrategy>());
            }
            else if (specEnum == ChrSpecialization::DruidRestoration)
            {
                class RestoDruidHealingStrategy : public CombatStrategy
                {
                public:
                    RestoDruidHealingStrategy() : CombatStrategy("resto_druid_healing") { _priority = 200; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<RestoDruidHealingStrategy>());
            }
            break;
        }

        case CLASS_DEMON_HUNTER:
        {
            class DemonHunterRotationStrategy : public CombatStrategy
            {
            public:
                DemonHunterRotationStrategy() : CombatStrategy("demon_hunter_rotation") { _priority = 180; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<DemonHunterRotationStrategy>());

            // Momentum/positioning for all DHs
            class DHMobilityStrategy : public CombatStrategy
            {
            public:
                DHMobilityStrategy() : CombatStrategy("dh_mobility_management") { _priority = 175; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<DHMobilityStrategy>());

            if (specEnum == ChrSpecialization::DemonHunterHavoc)
            {
                class HavocDHBurstStrategy : public CombatStrategy
                {
                public:
                    HavocDHBurstStrategy() : CombatStrategy("havoc_dh_burst") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<HavocDHBurstStrategy>());
            }
            else if (specEnum == ChrSpecialization::DemonHunterVengeance)
            {
                class VengeanceDHTankStrategy : public CombatStrategy
                {
                public:
                    VengeanceDHTankStrategy() : CombatStrategy("vengeance_dh_tank") { _priority = 195; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                    float GetThreatModifier() const override { return 3.5f; }
                };
                strategies.push_back(::std::make_unique<VengeanceDHTankStrategy>());
            }
            break;
        }

        case CLASS_EVOKER:
        {
            class EvokerRotationStrategy : public CombatStrategy
            {
            public:
                EvokerRotationStrategy() : CombatStrategy("evoker_rotation") { _priority = 180; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<EvokerRotationStrategy>());

            // Empowerment management for all evokers
            class EvokerEmpowerStrategy : public CombatStrategy
            {
            public:
                EvokerEmpowerStrategy() : CombatStrategy("evoker_empower_management") { _priority = 185; }
                void InitializeActions() override { CombatStrategy::InitializeActions(); }
                void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                void InitializeValues() override {}
            };
            strategies.push_back(::std::make_unique<EvokerEmpowerStrategy>());

            if (specEnum == ChrSpecialization::EvokerDevastation)
            {
                class DevastationEvokerDPSStrategy : public CombatStrategy
                {
                public:
                    DevastationEvokerDPSStrategy() : CombatStrategy("devastation_evoker_dps") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<DevastationEvokerDPSStrategy>());
            }
            else if (specEnum == ChrSpecialization::EvokerPreservation)
            {
                class PreservationEvokerHealingStrategy : public CombatStrategy
                {
                public:
                    PreservationEvokerHealingStrategy() : CombatStrategy("preservation_evoker_healing") { _priority = 200; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<PreservationEvokerHealingStrategy>());
            }
            else if (specEnum == ChrSpecialization::EvokerAugmentation)
            {
                class AugmentationEvokerSupportStrategy : public CombatStrategy
                {
                public:
                    AugmentationEvokerSupportStrategy() : CombatStrategy("augmentation_evoker_support") { _priority = 185; }
                    void InitializeActions() override { CombatStrategy::InitializeActions(); }
                    void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
                    void InitializeValues() override {}
                };
                strategies.push_back(::std::make_unique<AugmentationEvokerSupportStrategy>());
            }
            break;
        }

        default:
            TC_LOG_DEBUG("module.playerbot.strategy", "StrategyFactory: Unknown class {} for CreateClassStrategies", classId);
            break;
    }

    TC_LOG_DEBUG("module.playerbot.strategy", "StrategyFactory: Created {} strategies for class {} spec {}",
        strategies.size(), classId, spec);

    return strategies;
}

::std::vector<::std::unique_ptr<Strategy>> StrategyFactory::CreateLevelStrategies(uint8 level)
{
    ::std::vector<::std::unique_ptr<Strategy>> strategies;

    // ============================================
    // LEVEL-APPROPRIATE STRATEGY CLASSES
    // ============================================

    // Basic leveling strategy - Simple rotation, auto-attack focus (level 1-10)
    class BasicLevelingStrategy : public CombatStrategy
    {
    public:
        BasicLevelingStrategy() : CombatStrategy("basic_leveling") { _priority = 100; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 0.8f;
            rel.survivalRelevance = 0.6f;
            return rel;
        }
    };

    // Resource management strategy - Learning mana/rage/energy (level 10-20)
    class ResourceManagementStrategy : public CombatStrategy
    {
    public:
        ResourceManagementStrategy() : CombatStrategy("resource_management") { _priority = 110; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai);
            rel.survivalRelevance = 0.7f;
            return rel;
        }
    };

    // Intermediate combat strategy - Multi-target, cooldown usage (level 20-50)
    class IntermediateCombatStrategy : public CombatStrategy
    {
    public:
        IntermediateCombatStrategy() : CombatStrategy("intermediate_combat") { _priority = 130; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 1.1f;
            rel.survivalRelevance = 0.8f;
            return rel;
        }
    };

    // Cooldown management strategy - Proper cooldown usage (level 20+)
    class CooldownManagementStrategy : public CombatStrategy
    {
    public:
        CooldownManagementStrategy() : CombatStrategy("cooldown_management") { _priority = 120; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai);
            return rel;
        }
    };

    // Advanced combat strategy - Full rotation, defensive cooldowns (level 50+)
    class AdvancedCombatStrategy : public CombatStrategy
    {
    public:
        AdvancedCombatStrategy() : CombatStrategy("advanced_combat") { _priority = 150; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 1.2f;
            rel.survivalRelevance = 0.9f;
            return rel;
        }
    };

    // Defensive cooldown strategy - Using defensive abilities (level 40+)
    class DefensiveCooldownStrategy : public CombatStrategy
    {
    public:
        DefensiveCooldownStrategy() : CombatStrategy("defensive_cooldowns") { _priority = 140; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.survivalRelevance = 1.0f;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 0.5f;
            return rel;
        }
    };

    // Endgame strategy - Full optimization, mythic+ and raid-ready (level 70+)
    class EndgameCombatStrategy : public CombatStrategy
    {
    public:
        EndgameCombatStrategy() : CombatStrategy("endgame_combat") { _priority = 170; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 1.5f;
            rel.survivalRelevance = 1.0f;
            return rel;
        }
    };

    // Create level-appropriate strategies
    if (level < 10)
    {
        // Basic leveling - simple rotation, auto-attack focus
        strategies.push_back(::std::make_unique<BasicLevelingStrategy>());
    }
    else if (level < 20)
    {
        // Learning resources - mana/rage/energy management
        strategies.push_back(::std::make_unique<ResourceManagementStrategy>());
        strategies.push_back(::std::make_unique<BasicLevelingStrategy>());
    }
    else if (level < 50)
    {
        // Intermediate - multi-target, cooldown usage
        strategies.push_back(::std::make_unique<IntermediateCombatStrategy>());
        strategies.push_back(::std::make_unique<CooldownManagementStrategy>());
    }
    else if (level < 70)
    {
        // Advanced - full rotation, defensive cooldowns
        strategies.push_back(::std::make_unique<AdvancedCombatStrategy>());
        strategies.push_back(::std::make_unique<DefensiveCooldownStrategy>());
        strategies.push_back(::std::make_unique<CooldownManagementStrategy>());
    }
    else
    {
        // Endgame - full optimization
        strategies.push_back(::std::make_unique<EndgameCombatStrategy>());
        strategies.push_back(::std::make_unique<AdvancedCombatStrategy>());
        strategies.push_back(::std::make_unique<DefensiveCooldownStrategy>());
        strategies.push_back(::std::make_unique<CooldownManagementStrategy>());
    }

    TC_LOG_DEBUG("module.playerbot.strategy", "StrategyFactory: Created {} level strategies for level {}",
        strategies.size(), level);

    return strategies;
}

::std::vector<::std::unique_ptr<Strategy>> StrategyFactory::CreatePvPStrategies()
{
    ::std::vector<::std::unique_ptr<Strategy>> strategies;

    // ============================================
    // PVP-SPECIFIC STRATEGY CLASSES
    // ============================================

    // PvP targeting strategy - Focus healers, low HP targets
    class PvPTargetingStrategy : public CombatStrategy
    {
    public:
        PvPTargetingStrategy() : CombatStrategy("pvp_targeting") { _priority = 200; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 1.5f;
            return rel;
        }

        ::Unit* SelectTarget(BotAI* ai) const override
        {
            // PvP targeting prioritizes healers, then low HP
            return CombatStrategy::SelectTarget(ai);
        }
    };
    strategies.push_back(::std::make_unique<PvPTargetingStrategy>());

    // PvP crowd control strategy - CC chains
    class PvPCrowdControlStrategy : public CombatStrategy
    {
    public:
        PvPCrowdControlStrategy() : CombatStrategy("pvp_crowd_control") { _priority = 190; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 1.3f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<PvPCrowdControlStrategy>());

    // PvP positioning strategy - LoS, pillar play
    class PvPPositioningStrategy : public CombatStrategy
    {
    public:
        PvPPositioningStrategy() : CombatStrategy("pvp_positioning") { _priority = 185; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai);
            rel.survivalRelevance = 0.8f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<PvPPositioningStrategy>());

    // PvP burst strategy - Cooldown stacking for kills
    class PvPBurstStrategy : public CombatStrategy
    {
    public:
        PvPBurstStrategy() : CombatStrategy("pvp_burst") { _priority = 195; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 1.4f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<PvPBurstStrategy>());

    // PvP defensive strategy - Trinket usage, defensives
    class PvPDefensiveStrategy : public CombatStrategy
    {
    public:
        PvPDefensiveStrategy() : CombatStrategy("pvp_defensive") { _priority = 200; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.survivalRelevance = 1.2f;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 0.5f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<PvPDefensiveStrategy>());

    // Flag carrier strategy - WSG/TP flag running
    class FlagCarrierStrategy : public CombatStrategy
    {
    public:
        FlagCarrierStrategy() : CombatStrategy("flag_carrier") { _priority = 180; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.survivalRelevance = 1.5f;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 0.3f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<FlagCarrierStrategy>());

    // Base assault strategy - AB/EOTS base capture
    class BaseAssaultStrategy : public CombatStrategy
    {
    public:
        BaseAssaultStrategy() : CombatStrategy("base_assault") { _priority = 175; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 1.2f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<BaseAssaultStrategy>());

    // Objective defense strategy - Defend objectives
    class ObjectiveDefenseStrategy : public CombatStrategy
    {
    public:
        ObjectiveDefenseStrategy() : CombatStrategy("objective_defense") { _priority = 170; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai);
            rel.survivalRelevance = 0.8f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<ObjectiveDefenseStrategy>());

    // Arena strategy - Arena-specific burst and CC timing
    class ArenaStrategy : public CombatStrategy
    {
    public:
        ArenaStrategy() : CombatStrategy("arena") { _priority = 210; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 1.5f;
            rel.survivalRelevance = 1.0f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<ArenaStrategy>());

    TC_LOG_DEBUG("module.playerbot.strategy", "StrategyFactory: Created {} PvP strategies", strategies.size());

    return strategies;
}

::std::vector<::std::unique_ptr<Strategy>> StrategyFactory::CreatePvEStrategies()
{
    ::std::vector<::std::unique_ptr<Strategy>> strategies;

    // ============================================
    // PVE-SPECIFIC STRATEGY CLASSES
    // ============================================

    // Tanking strategy - Threat generation, positioning
    class TankingStrategy : public CombatStrategy
    {
    public:
        TankingStrategy() : CombatStrategy("pve_tanking") { _priority = 200; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        float GetThreatModifier() const override { return 5.0f; }

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 1.3f;
            rel.survivalRelevance = 1.0f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<TankingStrategy>());

    // Healing priority strategy - Triage, mana management
    class HealingPriorityStrategy : public CombatStrategy
    {
    public:
        HealingPriorityStrategy() : CombatStrategy("pve_healing_priority") { _priority = 200; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        float GetThreatModifier() const override { return 0.3f; }

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai);
            rel.survivalRelevance = 1.2f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<HealingPriorityStrategy>());

    // DPS optimization strategy - Maximum damage rotation
    class DPSOptimizationStrategy : public CombatStrategy
    {
    public:
        DPSOptimizationStrategy() : CombatStrategy("pve_dps_optimization") { _priority = 180; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 1.2f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<DPSOptimizationStrategy>());

    // Add management strategy - Handle spawning adds
    class AddManagementStrategy : public CombatStrategy
    {
    public:
        AddManagementStrategy() : CombatStrategy("pve_add_management") { _priority = 175; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 1.1f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<AddManagementStrategy>());

    // Boss mechanics strategy - Generic boss avoidance
    class BossMechanicsStrategy : public CombatStrategy
    {
    public:
        BossMechanicsStrategy() : CombatStrategy("pve_boss_mechanics") { _priority = 195; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.survivalRelevance = 1.5f;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai);
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<BossMechanicsStrategy>());

    // Interrupt rotation strategy - Coordinate interrupts
    class InterruptRotationStrategy : public CombatStrategy
    {
    public:
        InterruptRotationStrategy() : CombatStrategy("pve_interrupt_rotation") { _priority = 185; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 1.2f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<InterruptRotationStrategy>());

    // Dispel priority strategy - Cleanse/dispel management
    class DispelPriorityStrategy : public CombatStrategy
    {
    public:
        DispelPriorityStrategy() : CombatStrategy("pve_dispel_priority") { _priority = 180; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai);
            rel.survivalRelevance = 0.8f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<DispelPriorityStrategy>());

    // Cooldown rotation strategy - Raid cooldowns management
    class CooldownRotationStrategy : public CombatStrategy
    {
    public:
        CooldownRotationStrategy() : CombatStrategy("pve_cooldown_rotation") { _priority = 190; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 1.3f;
            rel.survivalRelevance = 0.9f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<CooldownRotationStrategy>());

    // Mythic+ strategy - M+ specific optimizations
    class MythicPlusStrategy : public CombatStrategy
    {
    public:
        MythicPlusStrategy() : CombatStrategy("pve_mythic_plus") { _priority = 205; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 1.4f;
            rel.survivalRelevance = 1.1f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<MythicPlusStrategy>());

    // Dungeon strategy - Dungeon-specific behavior
    class DungeonStrategy : public CombatStrategy
    {
    public:
        DungeonStrategy() : CombatStrategy("pve_dungeon") { _priority = 170; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai);
            rel.questRelevance = 0.5f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<DungeonStrategy>());

    // Raid strategy - Raid-specific behavior
    class RaidStrategy : public CombatStrategy
    {
    public:
        RaidStrategy() : CombatStrategy("pve_raid") { _priority = 175; }
        void InitializeActions() override { CombatStrategy::InitializeActions(); }
        void InitializeTriggers() override { CombatStrategy::InitializeTriggers(); }
        void InitializeValues() override {}

        StrategyRelevance CalculateRelevance(BotAI* ai) const override
        {
            StrategyRelevance rel;
            rel.combatRelevance = CombatStrategy::GetRelevance(ai) * 1.1f;
            rel.socialRelevance = 0.3f;
            return rel;
        }
    };
    strategies.push_back(::std::make_unique<RaidStrategy>());

    TC_LOG_DEBUG("module.playerbot.strategy", "StrategyFactory: Created {} PvE strategies", strategies.size());

    return strategies;
}

::std::vector<::std::string> StrategyFactory::GetAvailableStrategies() const
{
    ::std::vector<::std::string> strategies;
    strategies.reserve(_creators.size());

    for (auto const& pair : _creators)
        strategies.push_back(pair.first);

    return strategies;
}

bool StrategyFactory::HasStrategy(::std::string const& name) const
{
    return _creators.find(name) != _creators.end();
}

} // namespace Playerbot