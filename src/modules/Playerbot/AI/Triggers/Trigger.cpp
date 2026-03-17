/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Combat/ThreatManager.h removed - not used in this file
#include "Trigger.h"
#include "Action.h"
#include "BotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Object.h"
#include "QuestDef.h"
#include "ObjectMgr.h"
#include "Log.h"

namespace Playerbot
{

// Trigger implementation
Trigger::Trigger(::std::string const& name, TriggerType type)
    : _name(name), _type(type)
{
    _firstTrigger = ::std::chrono::steady_clock::now();
    _lastTrigger = _firstTrigger;
}

TriggerResult Trigger::Evaluate(BotAI* ai) const
{
    TriggerResult result;

    if (!_active || !ai)
        return result;

    // Check basic trigger condition
    result.triggered = Check(ai);

    if (result.triggered)
    {
        // Update statistics
        _triggerCount++;
        _lastTrigger = ::std::chrono::steady_clock::now();

        // Calculate urgency
        result.urgency = CalculateUrgency(ai);

        // Set suggested action
        result.suggestedAction = _action;

        // Check additional conditions
    if (!CheckConditions(ai))
        {
            result.triggered = false;
            result.urgency = 0.0f;
        }
    }

    return result;
}

::std::string Trigger::GetActionName() const
{
    if (_action)
        return _action->GetName();
    return _actionName;
}

void Trigger::AddCondition(::std::function<bool(BotAI*)> condition)
{
    if (condition)
        _conditions.push_back(condition);
}

bool Trigger::CheckConditions(BotAI* ai) const
{
    for (auto const& condition : _conditions)
    {
        if (!condition(ai))
            return false;
    }
    return true;
}

float Trigger::GetAverageTriggerRate() const
{
    uint32 triggerCount = _triggerCount.load();
    if (triggerCount == 0)
        return 0.0f;

    auto now = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::seconds>(now - _firstTrigger);

    if (duration.count() == 0)
        return 0.0f;

    return static_cast<float>(triggerCount) / static_cast<float>(duration.count());
}

// HealthTrigger implementation
bool HealthTrigger::Check(BotAI* ai) const
{
    if (!ai)
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    float healthPct = bot->GetHealthPct();
    return healthPct <= (_threshold * 100.0f);
}

float HealthTrigger::CalculateUrgency(BotAI* ai) const
{
    if (!ai)
        return 0.0f;

    Player* bot = ai->GetBot();
    if (!bot)
        return 0.0f;

    float healthPct = bot->GetHealthPct() / 100.0f;
    float urgency = 1.0f - (healthPct / _threshold);

    // Clamp urgency to 0-1 range
    return ::std::max(0.0f, ::std::min(1.0f, urgency));
}

// CombatTrigger implementation
bool CombatTrigger::Check(BotAI* ai) const
{
    if (!ai)
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    return bot->IsInCombat();
}

float CombatTrigger::CalculateUrgency(BotAI* ai) const
{
    if (!ai)
        return 0.0f;

    Player* bot = ai->GetBot();
    if (!bot)
        return 0.0f;

    if (!bot->IsInCombat())
        return 0.0f;

    // Higher urgency based on number of attackers and health
    float urgency = 0.5f; // Base urgency for being in combat

    // Increase urgency based on number of attackers
    auto const& attackers = bot->getAttackers();
    urgency += attackers.size() * 0.1f;

    // Increase urgency if health is low
    float healthPct = bot->GetHealthPct() / 100.0f;
    if (healthPct < 0.5f)
        urgency += (0.5f - healthPct);

    return ::std::min(1.0f, urgency);
}

// TimerTrigger implementation
bool TimerTrigger::Check(BotAI* ai) const
{
    auto now = ::std::chrono::steady_clock::now();

    if (_lastCheck.time_since_epoch().count() == 0)
    {
        _lastCheck = now;
        return false;
    }

    auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - _lastCheck);
    if (elapsed.count() >= _interval)
    {
        _lastCheck = now;
        return true;
    }

    return false;
}

// DistanceTrigger implementation
bool DistanceTrigger::Check(BotAI* ai) const
{
    if (!ai)
        return false;

    Player* bot = ai->GetBot();
    if (!bot || !_referenceUnit)
        return false;

    float distanceSq = bot->GetExactDistSq(_referenceUnit);
    float thresholdSq = _distance * _distance;
    return distanceSq <= thresholdSq;
}

// QuestTrigger implementation
bool QuestTrigger::Check(BotAI* ai) const
{
    if (!ai)
        return false;

    return HasAvailableQuest(ai) || HasCompletedQuest(ai) || HasQuestObjective(ai);
}

bool QuestTrigger::HasAvailableQuest(BotAI* ai) const
{
    if (!ai)
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    // Check if there are any available quests to pick up
    // This is a simplified implementation - could be expanded
    return false;
}

bool QuestTrigger::HasCompletedQuest(BotAI* ai) const
{
    if (!ai)
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    // Check for completed quests that can be turned in
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        if (uint32 questId = bot->GetQuestSlotQuestId(slot))
        {
            if (bot->CanCompleteQuest(questId))
                return true;
        }
    }

    return false;
}

bool QuestTrigger::HasQuestObjective(BotAI* ai) const
{
    if (!ai)
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    // Check if bot has active quest objectives
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        if (uint32 questId = bot->GetQuestSlotQuestId(slot))
        {
            if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
            {
                if (!bot->CanCompleteQuest(questId))
                    return true; // Has objectives to complete
            }
        }
    }

    return false;
}

// TriggerFactory implementation
TriggerFactory* TriggerFactory::instance()
{
    static TriggerFactory instance;
    return &instance;
}

void TriggerFactory::RegisterTrigger(::std::string const& name,
                                   ::std::function<::std::shared_ptr<Trigger>()> creator)
{
    _creators[name] = creator;
}

::std::shared_ptr<Trigger> TriggerFactory::CreateTrigger(::std::string const& name)
{
    auto it = _creators.find(name);
    if (it != _creators.end())
        return it->second();

    return nullptr;
}

::std::vector<::std::shared_ptr<Trigger>> TriggerFactory::CreateDefaultTriggers()
{
    ::std::vector<::std::shared_ptr<Trigger>> triggers;

    // Create basic health trigger
    auto healthTrigger = ::std::make_shared<HealthTrigger>("low_health", 0.3f);
    triggers.push_back(healthTrigger);

    // Create combat trigger
    auto combatTrigger = ::std::make_shared<CombatTrigger>("enter_combat");
    triggers.push_back(combatTrigger);

    // Create quest completion trigger
    auto questTrigger = ::std::make_shared<QuestTrigger>("quest_complete");
    triggers.push_back(questTrigger);

    return triggers;
}

::std::vector<::std::shared_ptr<Trigger>> TriggerFactory::CreateCombatTriggers()
{
    ::std::vector<::std::shared_ptr<Trigger>> triggers;

    // Combat-specific triggers
    auto combatTrigger = ::std::make_shared<CombatTrigger>("enter_combat");
    triggers.push_back(combatTrigger);

    auto lowHealthTrigger = ::std::make_shared<HealthTrigger>("combat_low_health", 0.2f);
    triggers.push_back(lowHealthTrigger);

    return triggers;
}

::std::vector<::std::shared_ptr<Trigger>> TriggerFactory::CreateQuestTriggers()
{
    ::std::vector<::std::shared_ptr<Trigger>> triggers;

    // Quest-specific triggers
    auto questCompleteTrigger = ::std::make_shared<QuestTrigger>("quest_complete");
    triggers.push_back(questCompleteTrigger);

    return triggers;
}

::std::vector<::std::string> TriggerFactory::GetAvailableTriggers() const
{
    ::std::vector<::std::string> triggers;
    triggers.reserve(_creators.size());

    for (auto const& pair : _creators)
        triggers.push_back(pair.first);

    return triggers;
}

bool TriggerFactory::HasTrigger(::std::string const& name) const
{
    return _creators.find(name) != _creators.end();
}

} // namespace Playerbot
