# Phase 2: AI Framework - Part 1: Overview & Architecture
## Detailed Implementation Plan

## Executive Summary
**Component**: Intelligent AI System with Strategy Pattern  
**Duration**: Weeks 12-14 (3 weeks)  
**Foundation**: Leverages ALL Phase 1 & Phase 2 components  
**Scope**: Decision making, action system, movement, combat, social behaviors  

## Architecture Overview

### AI System Integration
```cpp
// AI Framework leverages entire enterprise stack
BotAI
├── Strategy System       // High-level decision making
│   ├── Uses: Intel TBB (parallel strategy evaluation)
│   ├── Uses: phmap (strategy caching)
│   └── Uses: BotSession (state management)
├── Action System         // Executable behaviors
│   ├── Uses: Trinity APIs (Player, Spell, Movement)
│   ├── Uses: Action queues (lock-free)
│   └── Uses: Async execution
├── Trigger System        // Event-driven responses
│   ├── Uses: World events
│   ├── Uses: Combat events
│   └── Uses: Social events
└── Value System          // Decision weights
    ├── Uses: Machine learning ready
    ├── Uses: Dynamic adjustment
    └── Uses: Player behavior mimicry
```

### Performance Targets
- **AI Decision**: <5ms per bot per tick
- **Action Execution**: <1ms per action
- **Pathfinding**: <10ms for complex paths
- **Memory**: <100KB per bot AI state
- **CPU**: <0.05% per bot with batching
- **Scalability**: Linear to 1000 bots

## Week 12: Core AI Framework

### Day 1-2: Strategy System Implementation

#### File: `src/modules/Playerbot/AI/Strategy/Strategy.h`
```cpp
#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <phmap.h>

namespace Playerbot
{

// Forward declarations
class BotAI;
class Action;
class Trigger;
class Value;

// Strategy relevance score
struct StrategyRelevance
{
    float combatRelevance = 0.0f;
    float questRelevance = 0.0f;
    float socialRelevance = 0.0f;
    float survivalRelevance = 0.0f;
    float economicRelevance = 0.0f;
    
    float GetOverallRelevance() const
    {
        return combatRelevance + questRelevance + socialRelevance +
               survivalRelevance + economicRelevance;
    }
};

class TC_GAME_API Strategy
{
public:
    Strategy(std::string const& name);
    virtual ~Strategy() = default;

    // Core strategy interface
    virtual void InitializeActions() = 0;
    virtual void InitializeTriggers() = 0;
    virtual void InitializeValues() = 0;
    
    // Strategy evaluation
    virtual float GetRelevance(BotAI* ai) const;
    virtual StrategyRelevance CalculateRelevance(BotAI* ai) const;
    virtual bool IsActive(BotAI* ai) const { return _active; }
    
    // Action management
    void AddAction(std::string const& name, std::shared_ptr<Action> action);
    std::shared_ptr<Action> GetAction(std::string const& name) const;
    std::vector<std::shared_ptr<Action>> GetActions() const;
    
    // Trigger management
    void AddTrigger(std::shared_ptr<Trigger> trigger);
    std::vector<std::shared_ptr<Trigger>> GetTriggers() const;
    
    // Value management
    void AddValue(std::string const& name, std::shared_ptr<Value> value);
    std::shared_ptr<Value> GetValue(std::string const& name) const;
    
    // Strategy metadata
    std::string const& GetName() const { return _name; }
    uint32 GetPriority() const { return _priority; }
    void SetPriority(uint32 priority) { _priority = priority; }
    
    // Activation control
    virtual void OnActivate(BotAI* ai) {}
    virtual void OnDeactivate(BotAI* ai) {}
    void SetActive(bool active) { _active = active; }

protected:
    std::string _name;
    uint32 _priority = 100;
    std::atomic<bool> _active{false};
    
    // Strategy components
    phmap::parallel_flat_hash_map<std::string, std::shared_ptr<Action>> _actions;
    std::vector<std::shared_ptr<Trigger>> _triggers;
    phmap::parallel_flat_hash_map<std::string, std::shared_ptr<Value>> _values;
};

// Combat strategy base
class TC_GAME_API CombatStrategy : public Strategy
{
public:
    CombatStrategy(std::string const& name) : Strategy(name) {}
    
    virtual void InitializeActions() override;
    virtual void InitializeTriggers() override;
    virtual float GetRelevance(BotAI* ai) const override;
    
    // Combat-specific methods
    virtual bool ShouldFlee(BotAI* ai) const;
    virtual Unit* SelectTarget(BotAI* ai) const;
    virtual float GetThreatModifier() const { return 1.0f; }
};

// Quest strategy base
class TC_GAME_API QuestStrategy : public Strategy
{
public:
    QuestStrategy(std::string const& name) : Strategy(name) {}
    
    virtual void InitializeActions() override;
    virtual void InitializeTriggers() override;
    virtual float GetRelevance(BotAI* ai) const override;
    
    // Quest-specific methods
    virtual Quest const* SelectQuest(BotAI* ai) const;
    virtual bool ShouldAbandonQuest(Quest const* quest) const;
};

// Social strategy base
class TC_GAME_API SocialStrategy : public Strategy
{
public:
    SocialStrategy(std::string const& name) : Strategy(name) {}
    
    virtual void InitializeActions() override;
    virtual void InitializeTriggers() override;
    virtual float GetRelevance(BotAI* ai) const override;
    
    // Social-specific methods
    virtual bool ShouldGroupWith(Player* player) const;
    virtual bool ShouldTrade(Player* player) const;
    virtual std::string GenerateResponse(std::string const& message) const;
};

// Strategy factory
class TC_GAME_API StrategyFactory
{
    StrategyFactory() = default;
    ~StrategyFactory() = default;
    StrategyFactory(StrategyFactory const&) = delete;
    StrategyFactory& operator=(StrategyFactory const&) = delete;

public:
    static StrategyFactory* instance();
    
    // Strategy registration
    void RegisterStrategy(std::string const& name,
                         std::function<std::unique_ptr<Strategy>()> creator);
    
    // Strategy creation
    std::unique_ptr<Strategy> CreateStrategy(std::string const& name);
    std::vector<std::unique_ptr<Strategy>> CreateClassStrategies(uint8 classId, uint8 spec);
    std::vector<std::unique_ptr<Strategy>> CreateLevelStrategies(uint8 level);
    std::vector<std::unique_ptr<Strategy>> CreatePvPStrategies();
    std::vector<std::unique_ptr<Strategy>> CreatePvEStrategies();
    
    // Available strategies
    std::vector<std::string> GetAvailableStrategies() const;
    bool HasStrategy(std::string const& name) const;

private:
    phmap::parallel_flat_hash_map<std::string,
        std::function<std::unique_ptr<Strategy>()>> _creators;
};

#define sStrategyFactory StrategyFactory::instance()

} // namespace Playerbot
```

## Integration Summary

### Leverages ALL Enterprise Components
```cpp
// Uses Phase 1 enterprise infrastructure
BotSession* session = bot->GetSession(); // Enterprise session
sBotDBPool->ExecuteAsync(stmt);         // Async database
tbb::parallel_for(...);                  // Intel TBB parallelism
phmap::parallel_flat_hash_map           // Lock-free containers

// Uses Phase 2 components
sBotAccountMgr->GetAccountForBot(guid); // Account management
sBotCharacterMgr->GetBotInfo(guid);     // Character management
sBotSpawner->SpawnBot(request);         // Lifecycle spawning
sBotScheduler->ScheduleBot(guid);       // Activity scheduling
```

## Next Parts
- [Part 2: Action & Trigger Systems](AI_FRAMEWORK_PART2_ACTIONS.md)
- [Part 3: Main AI Controller](AI_FRAMEWORK_PART3_CONTROLLER.md)
- [Part 4: Class-Specific AI](AI_FRAMEWORK_PART4_CLASSES.md)
- [Part 5: Testing & Configuration](AI_FRAMEWORK_PART5_TESTING.md)

---

**Status**: Ready for implementation
**Dependencies**: ALL Phase 1 & Phase 2 components ✅
**Estimated Completion**: 3 weeks
**Quality**: Enterprise-grade, no shortcuts