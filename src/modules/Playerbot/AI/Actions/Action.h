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
#include "G3D/Vector3.h"
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <atomic>
#include <unordered_map>
#include <functional>

// Forward declarations for TrinityCore types
class WorldObject;
class Unit;
class Player;

namespace Playerbot
{

// Forward declarations
class BotAI;

// Action result
enum class ActionResult
{
    SUCCESS,      // Action completed successfully
    FAILED,       // Action failed
    IN_PROGRESS,  // Action still executing
    IMPOSSIBLE,   // Action cannot be performed
    CANCELLED     // Action was cancelled
};

// Action context
struct ActionContext
{
    WorldObject* target = nullptr;
    float x = 0, y = 0, z = 0;  // Position data
    uint32 spellId = 0;
    uint32 itemId = 0;
    ::std::string text;
    ::std::unordered_map<::std::string, float> values;
};

// Base action class
class TC_GAME_API Action
{
public:
    Action(::std::string const& name);
    virtual ~Action() = default;

    // Core action interface
    virtual bool IsPossible(BotAI* ai) const = 0;
    virtual bool IsUseful(BotAI* ai) const = 0;
    virtual ActionResult Execute(BotAI* ai, ActionContext const& context) = 0;

    // Legacy compatibility
    virtual bool Execute(BotAI* ai)
    {
        ActionContext context;
        return Execute(ai, context) == ActionResult::SUCCESS;
    }
    virtual float GetRelevance(BotAI* ai) const { return IsUseful(ai) ? 1.0f : 0.0f; }

    // Action properties
    ::std::string const& GetName() const { return _name; }
    float GetRelevanceScore() const { return _relevance; }
    void SetRelevance(float relevance) { _relevance = relevance; }

    // Cost calculation
    virtual float GetCost(BotAI* /*ai*/) const { return 1.0f; }
    virtual float GetCooldown() const { return 0.0f; }
    bool IsOnCooldown() const;

    // Action chaining
    void SetNextAction(::std::shared_ptr<Action> action) { _nextAction = action; }
    ::std::shared_ptr<Action> GetNextAction() const { return _nextAction; }

    // Prerequisites
    void AddPrerequisite(::std::shared_ptr<Action> action);
    ::std::vector<::std::shared_ptr<Action>> const& GetPrerequisites() const { return _prerequisites; }

    // Performance tracking
    uint32 GetExecutionCount() const { return _executionCount; }
    uint32 GetSuccessCount() const { return _successCount; }
    float GetSuccessRate() const;
    ::std::chrono::milliseconds GetAverageExecutionTime() const { return _avgExecutionTime; }

    // Legacy compatibility
    uint32 GetPriority() const { return static_cast<uint32>(_relevance * 100); }
    void SetPriority(uint32 priority) { _relevance = priority / 100.0f; }
    bool IsExecuting() const { return _executing; }
    void SetCooldown(uint32 /*cooldownMs*/) { /* handled by cooldown system */ }

protected:
    // Helper methods for derived classes
    bool CanCast(BotAI* ai, uint32 spellId, ::Unit* target = nullptr) const;
    bool DoCast(BotAI* ai, uint32 spellId, ::Unit* target = nullptr);
    bool DoMove(BotAI* ai, float x, float y, float z);
    bool DoSay(BotAI* ai, ::std::string const& text);
    bool DoEmote(BotAI* ai, uint32 emoteId);
    bool UseItem(BotAI* ai, uint32 itemId, ::Unit* target = nullptr);

    // Target selection helpers
    ::Unit* GetNearestEnemy(BotAI* ai, float range = 30.0f) const;
    ::Unit* GetLowestHealthAlly(BotAI* ai, float range = 40.0f) const;
    Player* GetNearestPlayer(BotAI* ai, float range = 100.0f) const;

protected:
    ::std::string _name;
    float _relevance = 1.0f;
    ::std::shared_ptr<Action> _nextAction;
    ::std::vector<::std::shared_ptr<Action>> _prerequisites;

    // Cooldown tracking
    mutable ::std::chrono::steady_clock::time_point _lastExecution;

    // Performance metrics
    ::std::atomic<uint32> _executionCount{0};
    ::std::atomic<uint32> _successCount{0};
    ::std::chrono::milliseconds _avgExecutionTime{0};
    bool _executing = false;
};

// Movement action
class TC_GAME_API MovementAction : public Action
{
public:
    MovementAction(::std::string const& name) : Action(name) {}

    virtual bool IsPossible(BotAI* ai) const;
    virtual ActionResult Execute(BotAI* ai, ActionContext const& context);

    // Movement-specific methods
    virtual bool GeneratePath(BotAI* ai, float x, float y, float z);
    virtual void SetSpeed(float speed) { _speed = speed; }
    virtual void SetFormation(uint32 formation) { _formation = formation; }

protected:
    float _speed = 1.0f;
    uint32 _formation = 0;
    ::std::vector<G3D::Vector3> _path;
};

// Combat action
class TC_GAME_API CombatAction : public Action
{
public:
    CombatAction(::std::string const& name) : Action(name) {}

    virtual bool IsUseful(BotAI* ai) const;

    // Combat-specific methods
    virtual float GetThreat(BotAI* /*ai*/) const { return 0.0f; }
    virtual bool RequiresFacing() const { return true; }
    virtual float GetRange() const { return 5.0f; }
    virtual bool BreaksCC() const { return false; }
};

// Spell action
class TC_GAME_API SpellAction : public CombatAction
{
public:
    SpellAction(::std::string const& name, uint32 spellId)
        : CombatAction(name), _spellId(spellId) {}

    virtual bool IsPossible(BotAI* ai) const;
    virtual bool IsUseful(BotAI* ai) const;
    virtual ActionResult Execute(BotAI* ai, ActionContext const& context);

    uint32 GetSpellId() const { return _spellId; }

protected:
    uint32 _spellId;
};

// Action factory
class TC_GAME_API ActionFactory final
{
    ActionFactory() = default;
    ~ActionFactory() = default;
    ActionFactory(ActionFactory const&) = delete;
    ActionFactory& operator=(ActionFactory const&) = delete;

public:
    static ActionFactory* instance();

    // Action registration
    void RegisterAction(::std::string const& name,
                       ::std::function<::std::shared_ptr<Action>()> creator);

    // Action creation
    ::std::shared_ptr<Action> CreateAction(::std::string const& name);
    ::std::vector<::std::shared_ptr<Action>> CreateClassActions(uint8 classId, uint8 spec);
    ::std::vector<::std::shared_ptr<Action>> CreateCombatActions(uint8 classId);
    ::std::vector<::std::shared_ptr<Action>> CreateMovementActions();

    // Available actions
    ::std::vector<::std::string> GetAvailableActions() const;
    bool HasAction(::std::string const& name) const;

private:
    ::std::unordered_map<::std::string, ::std::function<::std::shared_ptr<Action>()>> _creators;
};

#define sActionFactory ActionFactory::instance()

} // namespace Playerbot