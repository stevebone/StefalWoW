# 🔄 Phase 7 Legacy Singleton Call Migration Plan

## Executive Summary

**Total Legacy Calls Found**: 320+
**Managers Affected**: 22 Phase 7 managers
**Critical Files**: 15 high-priority files requiring migration

---

## Priority Classification

### 🔴 **CRITICAL (Runtime Crash Risk)**
Files that will crash on first bot creation:

1. **UnifiedQuestManager.cpp** - 200+ calls
   - DynamicQuestSystem (45 calls)
   - QuestCompletion (36 calls)
   - QuestPickup (20 calls)
   - QuestTurnIn (59 calls)
   - QuestValidation (48 calls)

2. **UnifiedLootManager.cpp** - 13 calls
   - LootDistribution (all methods)

3. **ProfessionAuctionBridge.cpp** - 2 calls
   - AuctionHouse::instance()

4. **AI/Strategy/QuestStrategy.cpp** - 9 calls
   - ObjectiveTracker (all methods)

5. **Dungeon/DungeonBehavior.cpp** - 8 calls
   - InstanceCoordination (all instance methods)

6. **LFG/LFGBotManager.cpp** - 3 calls
   - LFGBotSelector::instance()

### 🟡 **HIGH (Functionality Broken)**

7. **ServiceRegistration.h** - 22 DI registrations
   - All Phase 7 managers still registered as singletons

### 🟢 **MEDIUM (Documentation/Cleanup)**

8. Various .md documentation files
9. Backup files (.backup)

---

## Migration Strategy

### **Pattern 1: Facade Pattern (UnifiedQuestManager, UnifiedLootManager)**

These are facade classes that delegate to underlying managers.

**OLD Pattern**:
```cpp
// UnifiedQuestManager.cpp
return DynamicQuestSystem::instance()->DiscoverAvailableQuests(bot);
```

**NEW Pattern**:
```cpp
// UnifiedQuestManager.cpp - needs BotAI* reference
BotAI* botAI = GET_BOT_AI(bot); // Need helper
return botAI->GetGameSystems()->GetDynamicQuestSystem()->DiscoverAvailableQuests();
```

**Challenge**: Facades only have Player*, need to get BotAI*

**Solution**: Add helper method to get BotAI from Player

---

### **Pattern 2: Strategy Classes**

**OLD Pattern**:
```cpp
// QuestStrategy.cpp
ObjectiveTracker::instance()->UpdateBotTracking(bot, diff);
```

**NEW Pattern**:
```cpp
BotAI* botAI = dynamic_cast<BotAI*>(bot->GetPlayerbotAI());
botAI->GetGameSystems()->GetObjectiveTracker()->UpdateBotTracking(diff);
```

---

### **Pattern 3: Behavior Classes**

**OLD Pattern**:
```cpp
// DungeonBehavior.cpp
InstanceCoordination::instance()->InitializeInstanceCoordination(group, instance);
```

**NEW Pattern**:
```cpp
// Need per-bot access - must iterate group members
for (auto member : group->GetMembers())
{
    if (BotAI* botAI = GetBotAI(member))
        botAI->GetGameSystems()->GetInstanceCoordination()->InitializeInstanceCoordination(...);
}
```

**Note**: Coordination methods may need refactoring to work per-bot

---

### **Pattern 4: ServiceRegistration.h**

**OLD Pattern**:
```cpp
container.RegisterInstance<IArenaAI>(
    std::shared_ptr<IArenaAI>(ArenaAI::instance(), [](IArenaAI*) {})
);
```

**NEW Pattern**:
```cpp
// REMOVE ENTIRELY - Already done in Phase 7 commits
// But code still exists physically in file
```

---

## Execution Plan

### **Phase A: Infrastructure (Hours 1-2)**

1. ✅ Complete legacy call audit (DONE)
2. Create helper functions:
   - `BotAI* GetBotAI(Player* player)`
   - `GameSystemsManager* GetGameSystems(Player* player)`
3. Add to Player or utility header

### **Phase B: Critical Facades (Hours 2-4)**

4. Migrate **UnifiedQuestManager.cpp** (200+ calls)
   - Create systematic sed/awk scripts
   - Test each manager migration
   - Handle Player* → BotAI* conversion

5. Migrate **UnifiedLootManager.cpp** (13 calls)
   - Similar pattern to quest manager

### **Phase C: Strategies & Behaviors (Hours 4-6)**

6. Migrate **QuestStrategy.cpp** (9 calls)
7. Migrate **DungeonBehavior.cpp** (8 calls)
8. Migrate **LFG files** (LFGBotManager, LFGBotSelector, LFGGroupCoordinator)

### **Phase D: Remaining Files (Hours 6-7)**

9. Migrate **ProfessionAuctionBridge.cpp**
10. Clean up **ServiceRegistration.h** (physical removal)

### **Phase E: Verification (Hours 7-8)**

11. Grep verification - ensure 0 instance() calls
12. Build attempt (if environment permits)
13. Code review
14. Documentation update

---

## Implementation Scripts

### Script 1: Helper Functions
```cpp
// Add to BotAI.h or utility header
inline BotAI* GetBotAI(Player* player)
{
    if (!player)
        return nullptr;
    return dynamic_cast<BotAI*>(player->GetPlayerbotAI());
}

inline GameSystemsManager* GetGameSystems(Player* player)
{
    BotAI* botAI = GetBotAI(player);
    return botAI ? botAI->GetGameSystems() : nullptr;
}
```

### Script 2: UnifiedQuestManager Migration Template
```python
# For each singleton call in UnifiedQuestManager.cpp:
# OLD: DynamicQuestSystem::instance()->Method(bot, ...)
# NEW:
if (!bot) return [default];
BotAI* botAI = GetBotAI(bot);
if (!botAI || !botAI->GetGameSystems()) return [default];
return botAI->GetGameSystems()->GetDynamicQuestSystem()->Method(...);
```

---

## Risk Assessment

### **High Risk Areas**:
1. **Group coordination methods** - May need architectural changes
2. **Facade pattern** - Adds BotAI dependency
3. **Performance** - Additional pointer indirection

### **Mitigation**:
1. Add null checks everywhere
2. Return sensible defaults on failure
3. Log warnings for failed lookups
4. Incremental testing

---

## Success Criteria

✅ Zero `instance()` calls to Phase 7 managers
✅ All facades migrated to per-bot access
✅ ServiceRegistration.h cleaned up
✅ Code compiles (if build environment available)
✅ Comprehensive testing plan documented
✅ Migration documented in commits

---

**Estimated Total Time**: 8-10 hours
**Token Budget**: 50k-70k tokens

**Status**: Ready for execution
