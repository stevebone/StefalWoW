/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Action.h"  // For ActionContext definition
#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <atomic>
#include <chrono>
#include <unordered_map>

class Unit;  // TrinityCore forward declaration
class Player;
class WorldObject;

namespace Playerbot
{

// Forward declarations
class BotAI;

// Trigger types
enum class TriggerType
{
    COMBAT,      // Combat events
    HEALTH,      // Health/mana thresholds
    TIMER,       // Time-based
    DISTANCE,    // Distance-based
    QUEST,       // Quest events
    SOCIAL,      // Social interactions
    INVENTORY,   // Inventory changes
    WORLD        // World events
};

// Trigger result
struct TriggerResult
{
    bool triggered = false;
    float urgency = 0.0f;  // 0-1, higher = more urgent
    ::std::shared_ptr<Action> suggestedAction;
    ActionContext context;
};

class TC_GAME_API Trigger
{
public:
    Trigger(::std::string const& name, TriggerType type);
    virtual ~Trigger() = default;

    // Core trigger interface
    virtual bool Check(BotAI* ai) const = 0;
    virtual TriggerResult Evaluate(BotAI* ai) const;

    // Legacy compatibility
    virtual bool IsActive(BotAI* ai) const { return Check(ai); }

    // Trigger properties
    ::std::string const& GetName() const { return _name; }
    TriggerType GetType() const { return _type; }
    bool IsActive() const { return _active; }
    void SetActive(bool active) { _active = active; }

    // Associated action
    void SetAction(::std::shared_ptr<Action> action) { _action = action; }
    ::std::shared_ptr<Action> GetAction() const { return _action; }

    // Legacy action access
    ::std::string GetActionName() const;
    void SetAction(::std::string const& actionName) { _actionName = actionName; }

    // Urgency calculation
    virtual float CalculateUrgency(BotAI* /*ai*/) const { return 0.5f; }

    // Trigger conditions
    void AddCondition(::std::function<bool(BotAI*)> condition);
    bool CheckConditions(BotAI* ai) const;

    // Performance tracking
    uint32 GetTriggerCount() const { return _triggerCount; }
    float GetAverageTriggerRate() const;

    // Legacy compatibility
    uint32 GetPriority() const { return _priority; }
    void SetPriority(uint32 priority) { _priority = priority; }

protected:
    ::std::string _name;
    TriggerType _type;
    bool _active = true;
    ::std::shared_ptr<Action> _action;
    ::std::string _actionName; // Legacy support
    ::std::vector<::std::function<bool(BotAI*)>> _conditions;
    uint32 _priority = 100;

    // Statistics
    mutable ::std::atomic<uint32> _triggerCount{0};
    mutable ::std::chrono::steady_clock::time_point _firstTrigger;
    mutable ::std::chrono::steady_clock::time_point _lastTrigger;
};

// Health trigger
class TC_GAME_API HealthTrigger : public Trigger
{
public:
    HealthTrigger(::std::string const& name, float threshold)
        : Trigger(name, TriggerType::HEALTH), _threshold(threshold) {}

    virtual bool Check(BotAI* ai) const;
    virtual float CalculateUrgency(BotAI* ai) const;

    void SetThreshold(float threshold) { _threshold = threshold; }
    float GetThreshold() const { return _threshold; }

protected:
    float _threshold;  // 0-1 percentage
};

// Combat trigger
class TC_GAME_API CombatTrigger : public Trigger
{
public:
    CombatTrigger(::std::string const& name)
        : Trigger(name, TriggerType::COMBAT) {}

    virtual bool Check(BotAI* ai) const;
    virtual float CalculateUrgency(BotAI* ai) const;
};

// Timer trigger
class TC_GAME_API TimerTrigger : public Trigger
{
public:
    TimerTrigger(::std::string const& name, uint32 intervalMs)
        : Trigger(name, TriggerType::TIMER), _interval(intervalMs) {}

    virtual bool Check(BotAI* ai) const;

    void SetInterval(uint32 intervalMs) { _interval = intervalMs; }
    uint32 GetInterval() const { return _interval; }

protected:
    uint32 _interval;
    mutable ::std::chrono::steady_clock::time_point _lastCheck;
};

// Distance trigger
class TC_GAME_API DistanceTrigger : public Trigger
{
public:
    DistanceTrigger(::std::string const& name, float distance)
        : Trigger(name, TriggerType::DISTANCE), _distance(distance) {}

    virtual bool Check(BotAI* ai) const;

    void SetDistance(float distance) { _distance = distance; }
    float GetDistance() const { return _distance; }

protected:
    float _distance;
    ::Unit* _referenceUnit = nullptr;
};

// Quest trigger
class TC_GAME_API QuestTrigger : public Trigger
{
public:
    QuestTrigger(::std::string const& name)
        : Trigger(name, TriggerType::QUEST) {}

    virtual bool Check(BotAI* ai) const;

    // Quest-specific methods
    virtual bool HasAvailableQuest(BotAI* ai) const;
    virtual bool HasCompletedQuest(BotAI* ai) const;
    virtual bool HasQuestObjective(BotAI* ai) const;
};

// Trigger factory
class TC_GAME_API TriggerFactory final
{
    TriggerFactory() = default;
    ~TriggerFactory() = default;
    TriggerFactory(TriggerFactory const&) = delete;
    TriggerFactory& operator=(TriggerFactory const&) = delete;

public:
    static TriggerFactory* instance();

    // Trigger registration
    void RegisterTrigger(::std::string const& name,
                        ::std::function<::std::shared_ptr<Trigger>()> creator);

    // Trigger creation
    ::std::shared_ptr<Trigger> CreateTrigger(::std::string const& name);
    ::std::vector<::std::shared_ptr<Trigger>> CreateDefaultTriggers();
    ::std::vector<::std::shared_ptr<Trigger>> CreateCombatTriggers();
    ::std::vector<::std::shared_ptr<Trigger>> CreateQuestTriggers();

    // Available triggers
    ::std::vector<::std::string> GetAvailableTriggers() const;
    bool HasTrigger(::std::string const& name) const;

private:
    ::std::unordered_map<::std::string, ::std::function<::std::shared_ptr<Trigger>()>> _creators;
};

#define sTriggerFactory TriggerFactory::instance()

} // namespace Playerbot