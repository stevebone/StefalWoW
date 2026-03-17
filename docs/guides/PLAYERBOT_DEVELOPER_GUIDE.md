# TrinityCore PlayerBot Developer Guide

**Version**: 1.0
**Last Updated**: 2025-10-04
**Target Audience**: C++ Developers, Contributors

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Development Environment Setup](#development-environment-setup)
3. [Adding New Bot Behaviors](#adding-new-bot-behaviors)
4. [Extending Class AI](#extending-class-ai)
5. [Performance Optimization](#performance-optimization)
6. [Testing and Debugging](#testing-and-debugging)
7. [Contributing Guidelines](#contributing-guidelines)

---

## Architecture Overview

### Module Structure

```
src/modules/Playerbot/
├── Config/              # Configuration management
├── Session/             # Bot session lifecycle
├── AI/                  # AI core and class-specific implementations
│   ├── ClassAI/        # 13 class implementations
│   ├── Combat/         # Combat systems
│   ├── Strategy/       # Behavioral strategies
│   ├── Actions/        # Executable actions
│   └── Triggers/       # Event triggers
├── Lifecycle/           # Bot spawning and management
├── Advanced/            # Phase 4 - Groups, economy, social
├── Performance/         # Phase 5 - Optimization systems
├── Game/                # Game system integration
├── Quest/               # Quest automation
└── Commands/            # Chat command handlers
```

### Key Design Patterns

#### 1. Strategy Pattern (AI Behaviors)
```cpp
class Strategy {
public:
    virtual void Update(BotAI* ai, uint32 diff) = 0;
    virtual void OnCombatStart(Unit* target) = 0;
    virtual void OnCombatEnd() = 0;
};

class AggressiveStrategy : public Strategy {
    void Update(BotAI* ai, uint32 diff) override {
        // Aggressive behavior implementation
    }
};
```

#### 2. Factory Pattern (Bot Creation)
```cpp
class BotAIFactory {
public:
    static BotAI* CreateAI(Player* bot) {
        switch (bot->GetClass()) {
            case CLASS_WARRIOR: return new WarriorAI(bot);
            case CLASS_MAGE: return new MageAI(bot);
            // ...
        }
    }
};
```

#### 3. Observer Pattern (Event Handling)
```cpp
class BotEventBus {
public:
    void Subscribe(EventType type, EventHandler* handler);
    void Publish(EventType type, EventData const& data);
};
```

#### 4. Object Pool Pattern (Performance)
```cpp
template<typename T>
class ObjectPool {
public:
    T* Acquire() { /* reuse or allocate */ }
    void Release(T* object) { /* return to pool */ }
};
```

---

## Development Environment Setup

### Prerequisites

- **IDE**: Visual Studio 2022, CLion, or VS Code with C++ extensions
- **Compiler**: MSVC 19.32+, GCC 11+, or Clang 13+ (C++20 support required)
- **Build System**: CMake 3.24+
- **Version Control**: Git 2.30+
- **Debugger**: GDB, LLDB, or Visual Studio Debugger

### Clone and Build

```bash
# Clone repository
git clone https://github.com/TrinityCore/TrinityCore.git
cd TrinityCore
git checkout playerbot-dev

# Configure for development
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_PLAYERBOT=1

# Build
cmake --build . -j$(nproc)
```

### IDE Configuration

#### Visual Studio 2022

1. Open `TrinityCore.sln` in build directory
2. Set `worldserver` as startup project
3. Configure debugging:
   - Working Directory: `$(OutDir)`
   - Command Arguments: `-c worldserver.conf`
   - Environment: Copy DLLs to output directory

#### VS Code

`.vscode/launch.json`:
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug WorldServer",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/bin/worldserver",
            "args": ["-c", "worldserver.conf"],
            "cwd": "${workspaceFolder}/build/bin",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb"
        }
    ]
}
```

---

## Adding New Bot Behaviors

### Creating a Custom Action

**Step 1: Define Action Interface**

`src/modules/Playerbot/AI/Actions/CustomAction.h`:
```cpp
#ifndef PLAYERBOT_CUSTOMACTION_H
#define PLAYERBOT_CUSTOMACTION_H

#include "Action.h"

namespace Playerbot {

class CustomAction : public Action
{
public:
    CustomAction(BotAI* ai) : Action(ai, "custom_action") {}

    bool Execute(Event event) override;
    bool IsInstant() override { return true; }
};

} // namespace Playerbot

#endif
```

**Step 2: Implement Action**

`src/modules/Playerbot/AI/Actions/CustomAction.cpp`:
```cpp
#include "CustomAction.h"
#include "../BotAI.h"

namespace Playerbot {

bool CustomAction::Execute(Event event)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    // Your custom logic here
    LOG_INFO("playerbot.ai", "Bot {} executing custom action", bot->GetName());

    return true;
}

} // namespace Playerbot
```

**Step 3: Register Action**

In `BotAI.cpp`:
```cpp
void BotAI::InitializeActions()
{
    // ... existing actions
    actions["custom_action"] = std::make_unique<CustomAction>(this);
}
```

### Creating a Custom Trigger

**Step 1: Define Trigger**

`src/modules/Playerbot/AI/Triggers/CustomTrigger.h`:
```cpp
#ifndef PLAYERBOT_CUSTOMTRIGGER_H
#define PLAYERBOT_CUSTOMTRIGGER_H

#include "Trigger.h"

namespace Playerbot {

class CustomTrigger : public Trigger
{
public:
    CustomTrigger(BotAI* ai) : Trigger(ai, "custom_trigger") {}

    bool IsActive() override;
};

} // namespace Playerbot

#endif
```

**Step 2: Implement Trigger**

```cpp
#include "CustomTrigger.h"
#include "../BotAI.h"

namespace Playerbot {

bool CustomTrigger::IsActive()
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    // Your trigger condition
    return bot->GetHealthPct() < 50.0f;
}

} // namespace Playerbot
```

**Step 3: Register and Link**

In `BotAI.cpp`:
```cpp
void BotAI::InitializeTriggers()
{
    // Register trigger
    triggers["custom_trigger"] = std::make_unique<CustomTrigger>(this);

    // Link trigger to action
    triggerActions["custom_trigger"] = "custom_action";
}
```

---

## Extending Class AI

### Adding a New Specialization

**Example: Adding Frost Death Knight AI**

**Step 1: Create Specialization Header**

`src/modules/Playerbot/AI/ClassAI/DeathKnights/FrostSpecialization.h`:
```cpp
#ifndef PLAYERBOT_FROSTSPECIALIZATION_H
#define PLAYERBOT_FROSTSPECIALIZATION_H

#include "../DeathKnightAI.h"

namespace Playerbot {

class FrostSpecialization : public DeathKnightAI
{
public:
    FrostSpecialization(Player* bot);

    // Core rotation
    void ExecuteRotation(Unit* target) override;
    void ExecuteDefensives() override;
    void ExecuteUtility() override;

    // Resource management
    bool HasEnoughRunicPower(uint32 amount) const;
    void GenerateRunicPower(uint32 amount);

    // Spec-specific abilities
    void CastObliterate(Unit* target);
    void CastFrostStrike(Unit* target);
    void CastHowlingBlast(Unit* target);
    void CastPillarOfFrost();

private:
    struct FrostAbilities {
        uint32 obliterate;
        uint32 frostStrike;
        uint32 howlingBlast;
        uint32 pillarOfFrost;
        uint32 icyTalons;
        uint32 killingMachine;
    } _abilities;

    void LoadAbilities();
    bool IsKillingMachineActive() const;
};

} // namespace Playerbot

#endif
```

**Step 2: Implement Rotation Logic**

`src/modules/Playerbot/AI/ClassAI/DeathKnights/FrostSpecialization.cpp`:
```cpp
#include "FrostSpecialization.h"
#include "Player.h"
#include "SpellInfo.h"

namespace Playerbot {

FrostSpecialization::FrostSpecialization(Player* bot)
    : DeathKnightAI(bot)
{
    LoadAbilities();
}

void FrostSpecialization::LoadAbilities()
{
    _abilities.obliterate = 49020;
    _abilities.frostStrike = 49143;
    _abilities.howlingBlast = 49184;
    _abilities.pillarOfFrost = 51271;
    // ... load all abilities
}

void FrostSpecialization::ExecuteRotation(Unit* target)
{
    if (!target || !target->IsAlive())
        return;

    // Priority 1: Pillar of Frost (cooldown)
    if (CanCast(_abilities.pillarOfFrost, target))
    {
        CastPillarOfFrost();
        return;
    }

    // Priority 2: Obliterate with Killing Machine
    if (IsKillingMachineActive() && CanCast(_abilities.obliterate, target))
    {
        CastObliterate(target);
        return;
    }

    // Priority 3: Frost Strike to dump Runic Power
    if (HasEnoughRunicPower(25) && CanCast(_abilities.frostStrike, target))
    {
        CastFrostStrike(target);
        return;
    }

    // Priority 4: Howling Blast (AoE/Rime proc)
    if (CanCast(_abilities.howlingBlast, target))
    {
        CastHowlingBlast(target);
        return;
    }

    // Priority 5: Obliterate (filler)
    if (CanCast(_abilities.obliterate, target))
    {
        CastObliterate(target);
        return;
    }
}

void FrostSpecialization::CastObliterate(Unit* target)
{
    if (CastSpell(_abilities.obliterate, target))
    {
        LOG_DEBUG("playerbot.ai", "Bot {} cast Obliterate", GetBot()->GetName());
        GenerateRunicPower(20);
    }
}

bool FrostSpecialization::HasEnoughRunicPower(uint32 amount) const
{
    return GetBot()->GetPower(POWER_RUNIC_POWER) >= amount * 10; // RP is stored as * 10
}

} // namespace Playerbot
```

**Step 3: Register Specialization**

In `DeathKnightAI.cpp`:
```cpp
DeathKnightAI* DeathKnightAI::Create(Player* bot)
{
    uint8 spec = bot->GetPrimaryTalentTree(bot->GetActiveSpec());

    switch (spec)
    {
        case TALENT_TREE_DEATH_KNIGHT_BLOOD:
            return new BloodSpecialization(bot);
        case TALENT_TREE_DEATH_KNIGHT_FROST:
            return new FrostSpecialization(bot);  // New!
        case TALENT_TREE_DEATH_KNIGHT_UNHOLY:
            return new UnholySpecialization(bot);
        default:
            return new DeathKnightAI(bot);
    }
}
```

---

## Performance Optimization

### Using ThreadPool for Asynchronous Tasks

```cpp
#include "Performance/ThreadPool/ThreadPool.h"

void BotScheduler::UpdateBotsAsync()
{
    ThreadPool& pool = GetThreadPool();

    for (auto& bot : _bots)
    {
        pool.Submit(TaskPriority::NORMAL, [&bot]() {
            bot->Update(diff);
        });
    }

    // Wait for all updates to complete
    pool.WaitForCompletion(std::chrono::milliseconds(100));
}
```

### Using MemoryPool for Bot Objects

```cpp
#include "Performance/MemoryPool/MemoryPool.h"

class BotFactory
{
private:
    MemoryPool<BotAI> _aiPool{MemoryPool<BotAI>::Configuration{
        .initialCapacity = 1000,
        .maxCapacity = 10000,
        .enableThreadCache = true
    }};

public:
    BotAI* CreateBot(Player* player)
    {
        // Use memory pool instead of new
        BotAI* ai = _aiPool.Allocate(player);
        return ai;
    }

    void DestroyBot(BotAI* ai)
    {
        // Return to pool instead of delete
        _aiPool.Deallocate(ai);
    }
};
```

### Profiling Performance-Critical Code

```cpp
#include "Performance/Profiler/Profiler.h"

void ClassAI::ExecuteRotation(Unit* target)
{
    PROFILE_FUNCTION();  // Automatic timing

    {
        PROFILE_SCOPE("Target Selection");
        target = SelectTarget();
    }

    {
        PROFILE_SCOPE("Spell Casting");
        CastSpells(target);
    }

    {
        PROFILE_SCOPE("Resource Management");
        ManageResources();
    }
}

// Generate report
void DumpPerformanceReport()
{
    auto results = Profiler::Instance().GetResults();
    for (auto& [section, data] : results.sections)
    {
        LOG_INFO("performance", "Section: {}, Avg: {:.2f}us, Calls: {}",
            section, data.GetAverage(), data.callCount.load());
    }
}
```

---

## Testing and Debugging

### Unit Testing with Google Test

`src/modules/Playerbot/Tests/BotAITest.cpp`:
```cpp
#include <gtest/gtest.h>
#include "../AI/BotAI.h"

namespace Playerbot {
namespace Tests {

class BotAITest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup test environment
    }

    void TearDown() override
    {
        // Cleanup
    }
};

TEST_F(BotAITest, BotCreation)
{
    // Test bot creation
    auto* bot = CreateTestBot(CLASS_WARRIOR);
    ASSERT_NE(bot, nullptr);
    EXPECT_EQ(bot->GetClass(), CLASS_WARRIOR);
}

TEST_F(BotAITest, RotationExecution)
{
    auto* bot = CreateTestBot(CLASS_WARRIOR);
    auto* ai = bot->GetBotAI();

    ai->ExecuteRotation(GetTestTarget());
    // Verify spell was cast
    EXPECT_GT(bot->GetSpellCastCount(), 0);
}

} // namespace Tests
} // namespace Playerbot
```

### Debugging Tips

#### Enable Debug Logging

In `playerbots.conf`:
```ini
Playerbot.Debug.Enable = 1
Playerbot.Debug.LogLevel = 4  # 0=Error, 1=Warn, 2=Info, 3=Debug, 4=Trace
```

#### Use Conditional Breakpoints

Visual Studio:
1. Set breakpoint
2. Right-click → Conditions
3. Add condition: `bot->GetName() == "SpecificBotName"`

#### Memory Leak Detection

Windows (Visual Studio):
```cpp
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

int main()
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    // ... program execution
    _CrtDumpMemoryLeaks();
}
```

Linux (Valgrind):
```bash
valgrind --leak-check=full --show-leak-kinds=all ./worldserver
```

#### Performance Profiling

Windows (Visual Studio Profiler):
1. Debug → Performance Profiler
2. Select "CPU Usage" or "Memory Usage"
3. Start profiling
4. Spawn bots and execute actions
5. Stop and analyze hot paths

Linux (perf):
```bash
# Record performance data
perf record -g ./worldserver

# Analyze
perf report
```

---

## Contributing Guidelines

### Code Style

Follow TrinityCore C++ coding standards:

```cpp
// Class names: PascalCase
class BotAI { };

// Function names: PascalCase
void ExecuteRotation();

// Variable names: camelCase (member: _prefix)
uint32 spellId;
Player* _bot;

// Constants: ALL_CAPS
constexpr uint32 MAX_BOTS = 5000;

// Namespaces: lowercase
namespace playerbot { }

// Indentation: 4 spaces (no tabs)
void Function()
{
    if (condition)
    {
        // 4 spaces
    }
}
```

### Commit Message Format

```
[PlayerBot] <Type>: <Short Description>

<Detailed description of changes>

**Changes**:
- Change 1
- Change 2

**Testing**: Describe how you tested

**CLAUDE.md Compliance**: ✅/❌

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Your Name <your@email.com>
```

Types: FEAT, FIX, REFACTOR, PERF, TEST, DOCS

### Pull Request Checklist

- [ ] Code compiles without errors
- [ ] All existing tests pass
- [ ] New tests added for new features
- [ ] No memory leaks (tested with Valgrind/CrtDbg)
- [ ] Performance impact measured (if applicable)
- [ ] Documentation updated
- [ ] CLAUDE.md compliance verified
- [ ] Code reviewed by at least one other developer

### CLAUDE.md Compliance

**CRITICAL RULES**:
- ✅ No shortcuts - Full implementations only
- ✅ Module-only - Code in `src/modules/Playerbot/`
- ✅ TrinityCore APIs - Use existing APIs, don't bypass
- ✅ Performance first - <0.1% CPU, <10MB memory per bot
- ✅ No TODOs - Complete code, no placeholders

### Testing Requirements

1. **Unit Tests**: Cover all new classes/functions
2. **Integration Tests**: Test with 10-50 bots
3. **Performance Tests**: Verify <0.1% CPU per bot
4. **Memory Tests**: No leaks over 24-hour run

---

## Advanced Topics

### Implementing Custom Combat Mechanics

```cpp
class CustomMechanic : public CombatMechanic
{
public:
    void OnSpellCast(Spell* spell) override {
        // Custom logic for spell casts
    }

    void OnDamageTaken(uint32 damage, Unit* attacker) override {
        // Custom logic for damage taken
    }

    void OnHealReceived(uint32 healing, Unit* healer) override {
        // Custom logic for healing
    }
};
```

### Creating Bot Group Strategies

```cpp
class RaidStrategy : public GroupStrategy
{
public:
    void AssignRoles(Group* group) override {
        // Tank assignment
        for (auto& member : group->GetMembers()) {
            if (IsTankClass(member->GetClass())) {
                AssignRole(member, ROLE_TANK);
            }
        }
        // Healer assignment
        // DPS assignment
    }

    void CoordinateMovement() override {
        // Formation logic
    }
};
```

---

## API Reference

See `docs/api/` for complete API documentation generated with Doxygen.

### Key Namespaces

- `Playerbot::` - Main PlayerBot namespace
- `Playerbot::Performance::` - Phase 5 optimization systems
- `Playerbot::Tests::` - Unit testing framework

### Key Classes

- `BotAI` - Base AI controller
- `ClassAI` - Class-specific AI base
- `Action` - Executable bot actions
- `Trigger` - Event triggers
- `Strategy` - Behavioral strategies
- `ThreadPool` - Performance optimization
- `MemoryPool` - Memory management
- `Profiler` - Performance profiling

---

**End of Developer Guide**

*Last Updated: 2025-10-04*
*Version: 1.0*
*TrinityCore PlayerBot - Enterprise Edition*
