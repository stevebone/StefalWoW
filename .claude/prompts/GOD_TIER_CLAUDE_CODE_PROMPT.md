# GOD-TIER BOT SYSTEM - Claude Code Implementation Prompt

**Project**: TrinityBots Playerbot Module  
**Goal**: Implement complete humanization and god-tier bot capabilities  
**Total Effort**: ~250h  
**Start Date**: 2026-01-27

---

## 🎯 MISSION

Implementiere ein vollständiges "God-Tier" Bot-System für TrinityCore Playerbot, das:

1. **JEDE Quest** abschließen kann (alle 21 TrinityCore Objective Types)
2. **Automatisch Reiten lernt** und Mounts verwaltet
3. **Menschlich wirkt** (Sessions, Pausen, City Life, AFK)
4. **Achievements grindet**
5. **Collections sammelt** (Mounts, Pets, Transmog)
6. **Reputation farmt**
7. **Gold farmt**
8. **Alte Instanzen solo farmt**

---

## 📁 PROJECT CONTEXT

**Codebase Location**: `C:\TrinityBots\TrinityCore\src\modules\Playerbot\`

**Key Existing Files**:
- `Quest/QuestCompletion.h/cpp` - Quest completion system (MODIFY)
- `Companion/MountManager.h/cpp` - Basic mount handling (EXTEND)
- `Movement/BotIdleBehaviorManager.h/cpp` - Idle behaviors (REFERENCE)
- `Professions/GatheringManager.h/cpp` - Gathering system (REFERENCE)
- `AI/BotAI.h/cpp` - Main bot AI (INTEGRATE)

**Documentation**:
- `.claude/analysis/GOD_TIER_BOT_ANALYSIS.md` - Full analysis
- `.claude/prompts/GOD_TIER_IMPLEMENTATION_PLAN_PART1.md` - Detailed plan Part 1
- `.claude/prompts/GOD_TIER_IMPLEMENTATION_PLAN_PART2.md` - Detailed plan Part 2
- `.claude/analysis/QUEST_HUMANIZATION_ANALYSIS.md` - Humanization analysis

---

## 🚀 PHASE 1: QUEST SYSTEM COMPLETION (30h)

### Objective
Implement all missing quest objective handlers to achieve 100% quest coverage.

### Files to Modify

**1. `Quest/QuestCompletion.h`**

Add new enum values:
```cpp
enum class QuestObjectiveType : uint8
{
    // ... existing ...
    CURRENCY            = 4,   // NEW
    LEARN_SPELL         = 5,   // FIX
    MONEY               = 8,   // NEW
    PLAYER_KILLS        = 9,   // NEW
    PET_BATTLE_NPC      = 11,  // NEW
    DEFEAT_BATTLE_PET   = 12,  // NEW
    PVP_PET_BATTLES     = 13,  // NEW
    CRITERIA_TREE       = 14,  // FIX
    PROGRESS_BAR        = 15,  // FIX
    HAVE_CURRENCY       = 16,  // NEW
    OBTAIN_CURRENCY     = 17,  // NEW
    INCREASE_REPUTATION = 18,  // NEW
};
```

Add new method declarations:
```cpp
void HandleCurrencyObjective(Player* bot, QuestObjectiveData& objective);
void HandleLearnSpellObjective(Player* bot, QuestObjectiveData& objective);
void HandleMoneyObjective(Player* bot, QuestObjectiveData& objective);
void HandlePlayerKillsObjective(Player* bot, QuestObjectiveData& objective);
void HandlePetBattleNPCObjective(Player* bot, QuestObjectiveData& objective);
void HandleDefeatBattlePetObjective(Player* bot, QuestObjectiveData& objective);
void HandlePvPPetBattlesObjective(Player* bot, QuestObjectiveData& objective);
void HandleCriteriaTreeObjective(Player* bot, QuestObjectiveData& objective);
void HandleProgressBarObjective(Player* bot, QuestObjectiveData& objective);
void HandleHaveCurrencyObjective(Player* bot, QuestObjectiveData& objective);
void HandleObtainCurrencyObjective(Player* bot, QuestObjectiveData& objective);
void HandleIncreaseReputationObjective(Player* bot, QuestObjectiveData& objective);

// Helper methods
Creature* FindTrainerForSpell(Player* bot, uint32 spellId);
void StartPvPActivity(Player* bot);
void StartDungeonActivity(Player* bot);
void QueueForBattleground(Player* bot, uint32 bgTypeId);
```

**2. `Quest/QuestCompletion.cpp`**

Update `ExecuteObjective()` switch statement and implement all new handlers.

### Implementation Order
1. HandleCurrencyObjective (easy - currency check)
2. HandleMoneyObjective (easy - gold check)
3. HandleLearnSpellObjective (medium - trainer interaction)
4. HandleIncreaseReputationObjective (medium - rep tracking)
5. HandlePlayerKillsObjective (medium - PvP integration)
6. HandleCriteriaTreeObjective (hard - criteria evaluation)
7. Pet Battle handlers (hard - new system)

### Testing
- Test with quests that use each objective type
- Verify progress tracking works
- Verify turn-in triggers correctly

---

## 🚀 PHASE 2: RIDING MANAGER (15h)

### Objective
Create RidingManager that automatically learns riding skills and manages mounts.

### Files to Create

```
Humanization/Mounts/
├── RidingManager.h
├── RidingManager.cpp
├── MountDatabase.h
└── MountDatabase.cpp
```

### Key Features
1. Detect current riding skill level
2. Find riding trainer for faction
3. Travel to trainer
4. Purchase riding skill
5. Purchase first mount if needed
6. Auto-mount for long distances
7. Select best mount for situation (ground/flying)

### Integration Points
- Hook into `BotAI::UpdateAI()` for auto-mounting
- Hook into `GameSystemsManager` for per-bot instance
- Use existing `MountManager` for mount casting

### Testing
- Create bot at level 10, verify it learns Apprentice Riding
- Create bot at level 30, verify it learns Expert Riding
- Test auto-mounting behavior

---

## 🚀 PHASE 3: HUMANIZATION CORE (40h)

### Objective
Create HumanizationManager that makes bots behave like humans.

### Files to Create

```
Humanization/
├── CMakeLists.txt
├── Core/
│   ├── HumanizationManager.h
│   ├── HumanizationManager.cpp
│   ├── HumanizationConfig.h
│   ├── HumanizationConfig.cpp
│   ├── PersonalityProfile.h
│   ├── PersonalityProfile.cpp
│   └── ActivityType.h
└── Sessions/
    ├── ActivitySession.h
    ├── ActivitySessionManager.h
    └── ActivitySessionManager.cpp
```

### Key Features
1. **Activity Sessions**: Track what bot is doing and for how long
2. **Session Transitions**: Natural switching between activities
3. **Personality Profiles**: Different play styles (casual, hardcore, etc.)
4. **Configuration**: Expose settings in worldserver.conf

### Config Options to Add
```ini
Playerbot.Humanization.Enabled = true
Playerbot.Humanization.Session.MinDuration = 1800000
Playerbot.Humanization.Session.MaxDuration = 5400000
Playerbot.Humanization.Gathering.MinDuration = 1800000
Playerbot.Humanization.Fishing.MinDuration = 1800000
```

### Integration Points
- Hook into `BotAI::UpdateAI()` for session management
- Integrate with existing behavior systems
- Coordinate with GatheringManager, Quest system

---

## 🚀 PHASE 4: GATHERING SESSIONS (12h)

### Objective
GatheringSessionManager that keeps bots gathering for 30-60+ minutes.

### Files to Create

```
Humanization/Sessions/
├── GatheringSessionManager.h
└── GatheringSessionManager.cpp
```

### Key Features
1. **Minimum 30 minute sessions**
2. **Zone/Route selection**
3. **Natural breaks within session**
4. **Progress tracking**
5. **Bag full detection**

### Integration
- Wrap existing `GatheringManager`
- Hook into `HumanizationManager`

---

## 🚀 PHASE 5: CITY LIFE (10h)

### Objective
CityLifeBehaviorManager for human-like city behavior.

### Files to Create

```
Humanization/CityLife/
├── CityLifeBehaviorManager.h
├── CityLifeBehaviorManager.cpp
└── CityLocations.h
```

### Key Features
1. **AH Browsing** (10-30 min)
2. **Mailbox checking** (2-5 min)
3. **Inn resting** (5-15 min for rested XP)
4. **Trainer visits**
5. **Idle wandering**
6. **Emote usage**

### City Locations Database
Store locations for:
- Alliance: Stormwind, Ironforge, Darnassus, Exodar
- Horde: Orgrimmar, Undercity, Thunder Bluff, Silvermoon
- Neutral: Dalaran, Shattrath, Boralus, etc.

---

## 🚀 PHASE 6: ACHIEVEMENT SYSTEM (20h)

### Objective
AchievementManager for tracking and grinding achievements.

### Files to Create

```
Humanization/Achievements/
├── AchievementManager.h
├── AchievementManager.cpp
├── AchievementGrinder.h
└── AchievementGrinder.cpp
```

### Key Features
1. **Progress tracking** for all achievements
2. **Difficulty estimation**
3. **Time estimation**
4. **Category-specific grinding**
5. **Meta achievement support**
6. **Reward awareness** (mounts, pets, titles)

---

## 📋 IMPLEMENTATION CHECKLIST

### Task 1: Quest System
- [ ] Update QuestObjectiveType enum
- [ ] Implement HandleCurrencyObjective
- [ ] Implement HandleLearnSpellObjective
- [ ] Implement HandleMoneyObjective
- [ ] Implement HandlePlayerKillsObjective
- [ ] Implement HandlePetBattleNPCObjective
- [ ] Implement HandleDefeatBattlePetObjective
- [ ] Implement HandlePvPPetBattlesObjective
- [ ] Implement HandleCriteriaTreeObjective
- [ ] Implement HandleProgressBarObjective
- [ ] Implement HandleHaveCurrencyObjective
- [ ] Implement HandleObtainCurrencyObjective
- [ ] Implement HandleIncreaseReputationObjective
- [ ] Update ExecuteObjective switch
- [ ] Add helper methods
- [ ] Test with sample quests

### Task 2: RidingManager
- [ ] Create RidingManager.h
- [ ] Create RidingManager.cpp
- [ ] Implement GetRidingLevel()
- [ ] Implement CanLearnNextRiding()
- [ ] Implement LearnNextRiding()
- [ ] Implement FindRidingTrainer()
- [ ] Implement GetBestMount()
- [ ] Implement MountUp()
- [ ] Implement ShouldMount()
- [ ] Create MountDatabase
- [ ] Add to GameSystemsManager
- [ ] Test at various levels

### Task 3: HumanizationManager
- [ ] Create directory structure
- [ ] Create ActivityType.h
- [ ] Create PersonalityProfile.h/cpp
- [ ] Create HumanizationConfig.h/cpp
- [ ] Create ActivitySession.h
- [ ] Create ActivitySessionManager.h/cpp
- [ ] Create HumanizationManager.h/cpp
- [ ] Add config options
- [ ] Integrate with BotAI
- [ ] Test session management

### Task 4: GatheringSessionManager
- [ ] Create GatheringSessionManager.h
- [ ] Create GatheringSessionManager.cpp
- [ ] Implement 30+ min sessions
- [ ] Implement route management
- [ ] Implement break system
- [ ] Integrate with HumanizationManager
- [ ] Test gathering sessions

### Task 5: CityLifeBehaviorManager
- [ ] Create CityLifeBehaviorManager.h
- [ ] Create CityLifeBehaviorManager.cpp
- [ ] Create CityLocations data
- [ ] Implement AH browsing
- [ ] Implement mailbox checking
- [ ] Implement inn resting
- [ ] Implement wandering
- [ ] Test in major cities

### Task 6: AchievementManager
- [ ] Create AchievementManager.h
- [ ] Create AchievementManager.cpp
- [ ] Implement progress tracking
- [ ] Implement difficulty estimation
- [ ] Implement category grinding
- [ ] Create AchievementGrinder.h/cpp
- [ ] Test achievement grinding

---

## 🔧 BUILD INSTRUCTIONS

After creating new files, update CMakeLists.txt:

```cmake
# In src/modules/Playerbot/CMakeLists.txt

# Add new source directories
set(PLAYERBOT_HUMANIZATION_SOURCES
    Humanization/Core/HumanizationManager.cpp
    Humanization/Core/HumanizationConfig.cpp
    Humanization/Core/PersonalityProfile.cpp
    Humanization/Sessions/ActivitySessionManager.cpp
    Humanization/Sessions/GatheringSessionManager.cpp
    Humanization/Sessions/FishingSessionManager.cpp
    Humanization/CityLife/CityLifeBehaviorManager.cpp
    Humanization/Mounts/RidingManager.cpp
    Humanization/Mounts/MountDatabase.cpp
    Humanization/Achievements/AchievementManager.cpp
    Humanization/Achievements/AchievementGrinder.cpp
    Humanization/Simulation/AFKSimulator.cpp
)

list(APPEND PLAYERBOT_SOURCES ${PLAYERBOT_HUMANIZATION_SOURCES})
```

---

## 🎯 SUCCESS CRITERIA

### Phase 1 Complete When:
- [ ] All 21 quest objective types have handlers
- [ ] Bots can complete currency quests
- [ ] Bots can complete spell-learning quests
- [ ] Bots can complete reputation quests

### Phase 2 Complete When:
- [ ] Bots automatically learn riding at appropriate levels
- [ ] Bots purchase their first mount
- [ ] Bots auto-mount for long distances
- [ ] Bots select appropriate mount for zone

### Phase 3 Complete When:
- [ ] Bots have activity sessions with durations
- [ ] Bots transition naturally between activities
- [ ] Personality affects behavior
- [ ] Config options work

### Phase 4 Complete When:
- [ ] Bots gather for 30+ minutes continuously
- [ ] Bots take natural breaks
- [ ] Session ends appropriately

### Phase 5 Complete When:
- [ ] Bots spend 10-30 minutes in cities
- [ ] Bots browse AH realistically
- [ ] Bots rest in inns
- [ ] Bots use emotes occasionally

### Phase 6 Complete When:
- [ ] Bots track achievement progress
- [ ] Bots can grind specific achievements
- [ ] Bots prioritize easy achievements

---

## 💡 TIPS FOR IMPLEMENTATION

1. **Start with Quest System** - Most impactful, unlocks other features
2. **Test incrementally** - Each handler should be tested before moving on
3. **Use existing patterns** - Follow existing code style in Playerbot
4. **Thread safety** - Use existing mutex patterns from other managers
5. **Performance** - Use caching where appropriate (mount lists, achievement progress)
6. **Logging** - Add debug logging for troubleshooting
7. **Configuration** - Make things configurable from the start

---

## 📚 REFERENCE CODE

### Existing Manager Pattern
```cpp
// Follow this pattern for new managers
class TC_GAME_API NewManager
{
public:
    explicit NewManager(Player* bot, BotAI* ai);
    ~NewManager();
    
    void Initialize();
    void Update(uint32 diff);
    
private:
    Player* _bot;
    BotAI* _ai;
    
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::SOME_LEVEL> _mutex;
};
```

### GameSystemsManager Integration
```cpp
// In GameSystemsManager.h, add:
RidingManager* GetRidingManager() { return _ridingManager.get(); }
HumanizationManager* GetHumanizationManager() { return _humanizationManager.get(); }
AchievementManager* GetAchievementManager() { return _achievementManager.get(); }

// In GameSystemsManager constructor, create instances
```

---

## 🚦 START HERE

1. Read the analysis documents in `.claude/analysis/`
2. Read the implementation plans in `.claude/prompts/`
3. Start with **Task 1: Quest System** - modify `Quest/QuestCompletion.h/cpp`
4. Build and test after each handler
5. Move to **Task 2: RidingManager** once Quest System is complete
6. Continue through phases in order

**Good luck! Make these bots indistinguishable from real players!**
