# Phase 2: AI Framework - Part 2: Action & Trigger Systems
## Week 12 Continued: Core Components

### Day 3-4: Action System Implementation

#### File: `src/modules/Playerbot/AI/Actions/Action.h`
```cpp
#pragma once

#include "Define.h"
#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace Playerbot
{

// Forward declarations
class BotAI;
class WorldObject;
class Unit;
class Player;

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
    std::string text;
    std::unordered_map<std::string, float> values;
};

// Base action class
class TC_GAME_API Action
{
public:
    Action(std::string const& name);
    virtual ~Action() = default;
    
    // Core action interface
    virtual bool IsPossible(BotAI* ai) const = 0;
    virtual bool IsUseful(BotAI* ai) const = 0;
    virtual ActionResult Execute(BotAI* ai, ActionContext const& context) = 0;
    
    // Action properties
    std::string const& GetName() const { return _name; }
    float GetRelevance() const { return _relevance; }
    void SetRelevance(float relevance) { _relevance = relevance; }
    
    // Cost calculation
    virtual float GetCost(BotAI* ai) const { return 1.0f; }
    virtual float GetCooldown() const { return 0.0f; }
    bool IsOnCooldown() const;
    
    // Action chaining
    void SetNextAction(std::shared_ptr<Action> action) { _nextAction = action; }
    std::shared_ptr<Action> GetNextAction() const { return _nextAction; }
    
    // Prerequisites
    void AddPrerequisite(std::shared_ptr<Action> action);
    std::vector<std::shared_ptr<Action>> const& GetPrerequisites() const { return _prerequisites; }
    
    // Performance tracking
    uint32 GetExecutionCount() const { return _executionCount; }
    uint32 GetSuccessCount() const { return _successCount; }
    float GetSuccessRate() const;
    std::chrono::milliseconds GetAverageExecutionTime() const { return _avgExecutionTime; }

protected:
    // Helper methods for derived classes
    bool CanCast(BotAI* ai, uint32 spellId, Unit* target = nullptr) const;
    bool DoCast(BotAI* ai, uint32 spellId, Unit* target = nullptr);
    bool DoMove(BotAI* ai, float x, float y, float z);
    bool DoSay(BotAI* ai, std::string const& text);
    bool DoEmote(BotAI* ai, uint32 emoteId);
    bool UseItem(BotAI* ai, uint32 itemId, Unit* target = nullptr);
    
    // Target selection helpers
    Unit* GetNearestEnemy(BotAI* ai, float range = 30.0f) const;
    Unit* GetLowestHealthAlly(BotAI* ai, float range = 40.0f) const;
    Player* GetNearestPlayer(BotAI* ai, float range = 100.0f) const;

protected:
    std::string _name;
    float _relevance = 1.0f;
    std::shared_ptr<Action> _nextAction;
    std::vector<std::shared_ptr<Action>> _prerequisites;
    
    // Cooldown tracking
    mutable std::chrono::steady_clock::time_point _lastExecution;
    
    // Performance metrics
    std::atomic<uint32> _executionCount{0};
    std::atomic<uint32> _successCount{0};
    std::chrono::milliseconds _avgExecutionTime{0};
};

// Movement action
class TC_GAME_API MovementAction : public Action
{
public:
    MovementAction(std::string const& name) : Action(name) {}
    
    virtual bool IsPossible(BotAI* ai) const override;
    virtual ActionResult Execute(BotAI* ai, ActionContext const& context) override;
    
    // Movement-specific methods
    virtual bool GeneratePath(BotAI* ai, float x, float y, float z);
    virtual void SetSpeed(float speed) { _speed = speed; }
    virtual void SetFormation(uint32 formation) { _formation = formation; }

protected:
    float _speed = 1.0f;
    uint32 _formation = 0;
    std::vector<G3D::Vector3> _path;
};

// Combat action
class TC_GAME_API CombatAction : public Action
{
public:
    CombatAction(std::string const& name) : Action(name) {}
    
    virtual bool IsUseful(BotAI* ai) const override;
    
    // Combat-specific methods
    virtual float GetThreat(BotAI* ai) const { return 0.0f; }
    virtual bool RequiresFacing() const { return true; }
    virtual float GetRange() const { return 5.0f; }
    virtual bool BreaksCC() const { return false; }
};

// Spell action
class TC_GAME_API SpellAction : public CombatAction
{
public:
    SpellAction(std::string const& name, uint32 spellId)
        : CombatAction(name), _spellId(spellId) {}
    
    virtual bool IsPossible(BotAI* ai) const override;
    virtual bool IsUseful(BotAI* ai) const override;
    virtual ActionResult Execute(BotAI* ai, ActionContext const& context) override;
    
    uint32 GetSpellId() const { return _spellId; }
    
protected:
    uint32 _spellId;
};

// Action factory
class TC_GAME_API ActionFactory
{
    ActionFactory() = default;
    ~ActionFactory() = default;
    ActionFactory(ActionFactory const&) = delete;
    ActionFactory& operator=(ActionFactory const&) = delete;

public:
    static ActionFactory* instance();
    
    // Action registration
    void RegisterAction(std::string const& name,
                       std::function<std::shared_ptr<Action>()> creator);
    
    // Action creation
    std::shared_ptr<Action> CreateAction(std::string const& name);
    std::vector<std::shared_ptr<Action>> CreateClassActions(uint8 classId, uint8 spec);
    std::vector<std::shared_ptr<Action>> CreateCombatActions(uint8 classId);
    std::vector<std::shared_ptr<Action>> CreateMovementActions();
    
    // Available actions
    std::vector<std::string> GetAvailableActions() const;
    bool HasAction(std::string const& name) const;

private:
    phmap::parallel_flat_hash_map<std::string,
        std::function<std::shared_ptr<Action>()>> _creators;
};

#define sActionFactory ActionFactory::instance()

} // namespace Playerbot
```

### Day 5: Trigger System Implementation

#### File: `src/modules/Playerbot/AI/Triggers/Trigger.h`
```cpp
#pragma once

#include "Define.h"
#include <memory>
#include <string>
#include <functional>

namespace Playerbot
{

// Forward declarations
class BotAI;
class Action;

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
    std::shared_ptr<Action> suggestedAction;
    ActionContext context;
};

class TC_GAME_API Trigger
{
public:
    Trigger(std::string const& name, TriggerType type);
    virtual ~Trigger() = default;

    // Core trigger interface
    virtual bool Check(BotAI* ai) const = 0;
    virtual TriggerResult Evaluate(BotAI* ai) const;

    // Trigger properties
    std::string const& GetName() const { return _name; }
    TriggerType GetType() const { return _type; }
    bool IsActive() const { return _active; }
    void SetActive(bool active) { _active = active; }

    // Associated action
    void SetAction(std::shared_ptr<Action> action) { _action = action; }
    std::shared_ptr<Action> GetAction() const { return _action; }

    // Urgency calculation
    virtual float CalculateUrgency(BotAI* ai) const { return 0.5f; }

    // Trigger conditions
    void AddCondition(std::function<bool(BotAI*)> condition);
    bool CheckConditions(BotAI* ai) const;

    // Performance tracking
    uint32 GetTriggerCount() const { return _triggerCount; }
    float GetAverageTriggerRate() const;

protected:
    std::string _name;
    TriggerType _type;
    bool _active = true;
    std::shared_ptr<Action> _action;
    std::vector<std::function<bool(BotAI*)>> _conditions;

    // Statistics
    mutable std::atomic<uint32> _triggerCount{0};
    mutable std::chrono::steady_clock::time_point _firstTrigger;
    mutable std::chrono::steady_clock::time_point _lastTrigger;
};

// Health trigger
class TC_GAME_API HealthTrigger : public Trigger
{
public:
    HealthTrigger(std::string const& name, float threshold)
        : Trigger(name, TriggerType::HEALTH), _threshold(threshold) {}

    virtual bool Check(BotAI* ai) const override;
    virtual float CalculateUrgency(BotAI* ai) const override;

    void SetThreshold(float threshold) { _threshold = threshold; }
    float GetThreshold() const { return _threshold; }

protected:
    float _threshold;  // 0-1 percentage
};

// Combat trigger
class TC_GAME_API CombatTrigger : public Trigger
{
public:
    CombatTrigger(std::string const& name)
        : Trigger(name, TriggerType::COMBAT) {}

    virtual bool Check(BotAI* ai) const override;
    virtual float CalculateUrgency(BotAI* ai) const override;
};

// Timer trigger
class TC_GAME_API TimerTrigger : public Trigger
{
public:
    TimerTrigger(std::string const& name, uint32 intervalMs)
        : Trigger(name, TriggerType::TIMER), _interval(intervalMs) {}

    virtual bool Check(BotAI* ai) const override;

    void SetInterval(uint32 intervalMs) { _interval = intervalMs; }
    uint32 GetInterval() const { return _interval; }

protected:
    uint32 _interval;
    mutable std::chrono::steady_clock::time_point _lastCheck;
};

// Distance trigger
class TC_GAME_API DistanceTrigger : public Trigger
{
public:
    DistanceTrigger(std::string const& name, float distance)
        : Trigger(name, TriggerType::DISTANCE), _distance(distance) {}

    virtual bool Check(BotAI* ai) const override;
    
    void SetDistance(float distance) { _distance = distance; }
    float GetDistance() const { return _distance; }

protected:
    float _distance;
    Unit* _referenceUnit = nullptr;
};

// Quest trigger
class TC_GAME_API QuestTrigger : public Trigger
{
public:
    QuestTrigger(std::string const& name)
        : Trigger(name, TriggerType::QUEST) {}

    virtual bool Check(BotAI* ai) const override;
    
    // Quest-specific methods
    virtual bool HasAvailableQuest(BotAI* ai) const;
    virtual bool HasCompletedQuest(BotAI* ai) const;
    virtual bool HasQuestObjective(BotAI* ai) const;
};

// Trigger factory
class TC_GAME_API TriggerFactory
{
    TriggerFactory() = default;
    ~TriggerFactory() = default;
    TriggerFactory(TriggerFactory const&) = delete;
    TriggerFactory& operator=(TriggerFactory const&) = delete;

public:
    static TriggerFactory* instance();
    
    // Trigger registration
    void RegisterTrigger(std::string const& name,
                        std::function<std::shared_ptr<Trigger>()> creator);
    
    // Trigger creation
    std::shared_ptr<Trigger> CreateTrigger(std::string const& name);
    std::vector<std::shared_ptr<Trigger>> CreateDefaultTriggers();
    std::vector<std::shared_ptr<Trigger>> CreateCombatTriggers();
    std::vector<std::shared_ptr<Trigger>> CreateQuestTriggers();
    
    // Available triggers
    std::vector<std::string> GetAvailableTriggers() const;
    bool HasTrigger(std::string const& name) const;

private:
    phmap::parallel_flat_hash_map<std::string,
        std::function<std::shared_ptr<Trigger>()>> _creators;
};

#define sTriggerFactory TriggerFactory::instance()

} // namespace Playerbot
```

## Implementation Examples

### Example: Common Actions Implementation
```cpp
// File: src/modules/Playerbot/AI/Actions/CommonActions.cpp

namespace Playerbot
{

// Move to position action
class MoveToPositionAction : public MovementAction
{
public:
    MoveToPositionAction() : MovementAction("move_to_position") {}
    
    bool IsPossible(BotAI* ai) const override
    {
        Player* bot = ai->GetBot();
        return bot && !bot->IsInCombat() && !bot->HasUnitState(UNIT_STATE_ROOT);
    }
    
    bool IsUseful(BotAI* ai) const override
    {
        return true; // Always useful if possible
    }
    
    ActionResult Execute(BotAI* ai, ActionContext const& context) override
    {
        if (context.x == 0 && context.y == 0 && context.z == 0)
            return ActionResult::FAILED;
            
        Player* bot = ai->GetBot();
        if (!bot)
            return ActionResult::FAILED;
            
        // Generate path
        if (!GeneratePath(ai, context.x, context.y, context.z))
            return ActionResult::FAILED;
            
        // Move along path
        bot->GetMotionMaster()->MovePoint(0, context.x, context.y, context.z);
        
        return ActionResult::SUCCESS;
    }
};

// Follow target action
class FollowAction : public MovementAction
{
public:
    FollowAction() : MovementAction("follow") {}
    
    bool IsPossible(BotAI* ai) const override
    {
        Player* bot = ai->GetBot();
        return bot && !bot->IsInCombat();
    }
    
    bool IsUseful(BotAI* ai) const override
    {
        // Check if we have a follow target
        return ai->GetFollowTarget() != nullptr;
    }
    
    ActionResult Execute(BotAI* ai, ActionContext const& context) override
    {
        Player* bot = ai->GetBot();
        Unit* target = ai->GetFollowTarget();
        
        if (!bot || !target)
            return ActionResult::FAILED;
            
        float distance = ai->GetFollowDistance();
        float angle = ai->GetFollowAngle();
        
        bot->GetMotionMaster()->MoveFollow(target, distance, angle);
        
        return ActionResult::SUCCESS;
    }
};

// Attack action
class AttackAction : public CombatAction
{
public:
    AttackAction() : CombatAction("attack") {}
    
    bool IsPossible(BotAI* ai) const override
    {
        Player* bot = ai->GetBot();
        return bot && bot->IsAlive() && !bot->IsNonMeleeSpellCast(false);
    }
    
    bool IsUseful(BotAI* ai) const override
    {
        Unit* target = ai->GetTarget();
        return target && target->IsAlive() && target->IsHostileTo(ai->GetBot());
    }
    
    ActionResult Execute(BotAI* ai, ActionContext const& context) override
    {
        Player* bot = ai->GetBot();
        Unit* target = context.target ? context.target->ToUnit() : ai->GetTarget();
        
        if (!bot || !target)
            return ActionResult::FAILED;
            
        // Start auto attack
        bot->Attack(target, true);
        
        // Move to melee range if needed
        float distance = bot->GetDistance(target);
        if (distance > GetRange())
        {
            bot->GetMotionMaster()->MoveChase(target, GetRange() - 1.0f);
        }
        
        return ActionResult::SUCCESS;
    }
    
    float GetRange() const override { return 5.0f; }
};

} // namespace Playerbot
```

## Next Parts
- [Part 1: Overview & Architecture](AI_FRAMEWORK_PART1_OVERVIEW.md)
- [Part 3: Main AI Controller](AI_FRAMEWORK_PART3_CONTROLLER.md)
- [Part 4: Class-Specific AI](AI_FRAMEWORK_PART4_CLASSES.md)
- [Part 5: Testing & Configuration](AI_FRAMEWORK_PART5_TESTING.md)

---

**Status**: Ready for implementation
**Dependencies**: Part 1 Strategy System
**Estimated Completion**: Week 12 Day 3-5