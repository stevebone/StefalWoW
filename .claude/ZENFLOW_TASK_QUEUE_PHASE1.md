# Zenflow Task Queue - Phase 1 Quick Wins

**Basierend auf**: Zenflow Code Review vom 2026-01-23  
**Parallel ausführbar**: JA - keine Abhängigkeiten  
**Erwartete Verbesserung**: 30-45% CPU

---

## Übersicht: 6 Tasks parallel starten

| Task | Priorität | Aufwand | Impact | Zenflow Workflow |
|------|-----------|---------|--------|------------------|
| QW-1: Recursive Mutex | P0 | 8-16h | 15-25% CPU | Bugfix |
| QW-2: Target Selection O(n) | P0 | 8-12h | 80-90% reduction | Bugfix |
| QW-3: Container reserve() | P1 | 8-16h | 2-5% CPU | Quick Change |
| QW-4: ClassAI Static Bug | P0 | 8-12h | Memory Leak Fix | Bugfix |
| QW-5: Spatial Query Cache | P1 | 12-16h | 80% reduction | Feature |
| QW-6: Naming Consistency | P2 | 4-8h | Code Quality | Quick Change |

---

## Task 1: Fix Recursive Mutex (COPY INTO ZENFLOW)

**Workflow**: Playerbot Bugfix  
**Priority**: P0 CRITICAL

### Description
```
Fix performance degradation caused by DEADLOCK FIX #18 which replaced std::shared_mutex 
with std::recursive_mutex in phmap::parallel_flat_hash_map, serializing ALL cache access.

PROBLEM:
- Location: Database/BotDatabasePool.h:202, Account/BotAccountMgr.h:209
- Impact: 15-25% performance degradation in database operations
- Cause: std::recursive_mutex serializes all reads AND writes

SOLUTION:
Switch from phmap::parallel_flat_hash_map to phmap::parallel_node_hash_map which 
is safe with std::shared_mutex.

BEFORE:
using CacheMap = phmap::parallel_flat_hash_map<ObjectGuid, CachedData, std::recursive_mutex>;

AFTER:
using CacheMap = phmap::parallel_node_hash_map<ObjectGuid, CachedData,
    std::hash<ObjectGuid>,
    std::equal_to<ObjectGuid>,
    std::allocator<std::pair<ObjectGuid const, CachedData>>,
    4,  // 4 submaps
    std::shared_mutex>;

FILES TO MODIFY:
1. Database/BotDatabasePool.h:202
2. Account/BotAccountMgr.h:209

VALIDATION:
- Build all configs (Debug, RelWithDebInfo, Release)
- No deadlocks under load
```

---

## Task 2: Target Selection O(n²) → O(n) (COPY INTO ZENFLOW)

**Workflow**: Playerbot Bugfix  
**Priority**: P0 CRITICAL

### Description
```
Optimize target selection algorithm from O(n×m) to O(n) complexity.

PROBLEM:
- Location: AI/Combat/TargetSelector.cpp:48-135
- Current: O(n×m) where n=enemies, m=threat list
- Impact: 50 enemies × 50 threats = 2,500 iterations per bot per update
- With 40 bots: 100,000 iterations per update!

ROOT CAUSE:
BotThreatManager::GetThreat() iterates through threat map instead of hash lookup.

SOLUTION 1 - Fix GetThreat():
File: AI/Combat/BotThreatManager.cpp

BEFORE:
float BotThreatManager::GetThreat(Unit* target) const {
    for (auto& [guid, threatValue] : _threatMap) {
        if (guid == target->GetGUID())
            return threatValue;
    }
}

AFTER:
float BotThreatManager::GetThreat(Unit* target) const {
    auto it = _threatMap.find(target->GetGUID());
    return it != _threatMap.end() ? it->second : 0.0f;  // O(1) lookup
}

SOLUTION 2 - Add threat score caching to TargetSelector:
File: AI/Combat/TargetSelector.cpp

Add member:
std::unordered_map<ObjectGuid, float> _threatScoreCache;
uint32 _lastThreatUpdate = 0;

Add method:
void RefreshThreatCache() {
    if (GameTime::GetGameTimeMS() - _lastThreatUpdate < 500)
        return;
    _threatScoreCache.clear();
    _threatManager->CopyThreatsTo(_threatScoreCache);
    _lastThreatUpdate = GameTime::GetGameTimeMS();
}

VALIDATION:
- Target selection drops from 100ms → 10ms (40 bots)
- Behavior unchanged (compare target selection logs)
```

---

## Task 3: Container reserve() Calls (COPY INTO ZENFLOW)

**Workflow**: Quick Change  
**Priority**: P1 HIGH

### Description
```
Add reserve() calls to hot path vectors to reduce heap allocations.

PROBLEM:
- 450+ std::vector declarations with only 50 reserve() calls (11%)
- Vectors grow incrementally causing multiple reallocations
- Heap fragmentation and cache misses

FILES TO MODIFY:

1. AI/Combat/TargetSelector.cpp:66,206,259
   Add: candidates.reserve(50);  // Typical max combat targets

2. AI/Actions/Action.cpp (181 push_back calls)
   Add ReservePrerequisites(size_t count) method
   Call _prerequisites.reserve(count);

3. AI/Strategy/BaselineRotationManager.cpp (33 emplace_back calls)
   Add reserve(20) for spell actions

4. AI/Combat/CombatStateManager.cpp
   Add reserve(20) for state vectors

5. AI/Combat/ThreatCoordinator.cpp
   Add reserve(40) for threat lists

6. AI/Combat/InterruptManager.cpp:382
   Add reserve(5) for class interrupt spells

7. AI/Combat/CrowdControlManager.cpp:153,536,573
   Add reserve(10) for CC spell lists

PATTERN:
- Nearby enemies: reserve(50)
- Spell actions: reserve(20)
- Target candidates: reserve(30)
- Class abilities: reserve(5-10)

VALIDATION:
- Heap profiler shows reduced allocation count
- 2-5% CPU improvement expected
```

---

## Task 4: Fix ClassAI Static Object Bug (COPY INTO ZENFLOW)

**Workflow**: Playerbot Bugfix  
**Priority**: P0 CRITICAL (Memory Leak)

### Description
```
Fix static specialization objects in ClassAI that cause memory leaks and crashes.

PROBLEM:
- Location: All 13 ClassAI UpdateRotation() methods
- Bug: Static objects created per call with stale bot pointer
- Impact: Memory leak, crashes when accessing deleted Player*

EXAMPLE (AI/ClassAI/Mages/MageAI.cpp:37-42):
case 62: // Arcane
{
    static ArcaneMageRefactored arcane(_bot);  // BUG: Static with stale pointer!
    arcane.UpdateRotation(target);
    break;
}

SOLUTION:
Replace static local with per-call instance:

BEFORE:
static StrategyContext _strategyContext(bot);  // ❌ Bug

AFTER:
StrategyContext _strategyContext(bot);  // ✅ Per-call instance

FILES TO UPDATE (all 13 ClassAI):
1. AI/ClassAI/Warrior/WarriorBotAI.cpp
2. AI/ClassAI/Paladin/PaladinBotAI.cpp
3. AI/ClassAI/Mage/MageBotAI.cpp
4. AI/ClassAI/Rogue/RogueBotAI.cpp
5. AI/ClassAI/Priest/PriestBotAI.cpp
6. AI/ClassAI/Shaman/ShamanBotAI.cpp
7. AI/ClassAI/Druid/DruidBotAI.cpp
8. AI/ClassAI/Hunter/HunterBotAI.cpp
9. AI/ClassAI/Monk/MonkBotAI.cpp
10. AI/ClassAI/DeathKnight/DeathKnightBotAI.cpp
11. AI/ClassAI/DemonHunter/DemonHunterBotAI.cpp
12. AI/ClassAI/Warlock/WarlockBotAI.cpp
13. AI/ClassAI/Evoker/EvokerBotAI.cpp

Search pattern: "static.*Refactored\|static.*Context"

VALIDATION:
- Valgrind/ASAN: Zero memory leaks
- No crashes under 1,000+ bot load
```

---

## Task 5: Spatial Query Cache (COPY INTO ZENFLOW)

**Workflow**: Playerbot Feature  
**Priority**: P1 HIGH

### Description
```
Create unified spatial query cache to eliminate duplicate queries.

PROBLEM:
- 5 different managers perform independent spatial queries for same data
- 5 queries × 0.5ms = 2.5ms per bot per update
- 40 bots: 100ms wasted per update

LOCATIONS WITH DUPLICATE QUERIES:
1. TargetSelector.cpp:459 - GetNearbyEnemies()
2. CombatStateAnalyzer.cpp - Duplicate query
3. InterruptAwareness.cpp:327 - Duplicate query
4. FormationManager.cpp - Duplicate query for allies
5. PositionManager.cpp - Duplicate query

SOLUTION:
Create new file: AI/Combat/CombatDataCache.h

class CombatDataCache {
public:
    struct CacheEntry {
        std::vector<Unit*> nearbyEnemies;
        std::vector<Unit*> nearbyAllies;
        std::vector<Unit*> castingEnemies;
        uint32 timestamp;
        
        bool IsValid(uint32 cacheTimeMs = 100) const {
            return (GameTime::GetGameTimeMS() - timestamp) < cacheTimeMs;
        }
    };
    
    static CacheEntry const& GetOrUpdate(Player* bot, uint32 cacheTimeMs = 100) {
        auto& cache = _cache[bot->GetGUID()];
        
        if (cache.IsValid(cacheTimeMs))
            return cache;
        
        // Single spatial query for ALL managers
        cache.nearbyEnemies = SpatialGridQueryHelpers::FindHostileCreaturesInRange(bot, 40.0f, true);
        cache.nearbyAllies = SpatialGridQueryHelpers::FindFriendlyPlayersInRange(bot, 40.0f);
        cache.castingEnemies.clear();
        
        for (Unit* enemy : cache.nearbyEnemies) {
            if (enemy->HasUnitState(UNIT_STATE_CASTING))
                cache.castingEnemies.push_back(enemy);
        }
        
        cache.timestamp = GameTime::GetGameTimeMS();
        return cache;
    }
    
private:
    static std::unordered_map<ObjectGuid, CacheEntry> _cache;
};

MIGRATION:
Update all 5 locations to use CombatDataCache::GetOrUpdate(_bot)

VALIDATION:
- Cache hit rate >80%
- Spatial query time: 100ms → 20ms (40 bots)
```

---

## Task 6: Naming Consistency (COPY INTO ZENFLOW)

**Workflow**: Quick Change  
**Priority**: P2 MEDIUM

### Description
```
Standardize manager class naming from "Mgr" to "Manager" suffix.

PROBLEM:
- Inconsistent naming: BotLifecycleManager vs BotLifecycleMgr
- Confusing for developers
- Code search harder

SOLUTION:
Standardize on "Manager" suffix.

CHANGES:
1. Rename BotLifecycleMgr → merge into BotLifecycleManager (or deprecate)
2. Rename BotAccountMgr → BotAccountManager (if separate file)
3. Update all references

Note: This may overlap with consolidation tasks. If unsure, just add 
deprecation comments and aliases for now.

// Example alias for backward compatibility
using BotLifecycleMgr = BotLifecycleManager; // [[deprecated("Use BotLifecycleManager")]]

VALIDATION:
- All builds pass
- No broken references
```

---

## Empfohlene Ausführung

### Option A: Alle 6 parallel (EMPFOHLEN für Zenflow)
```
Zenflow kann alle 6 in separaten Worktrees ausführen:
- Keine Konflikte da verschiedene Dateien
- Jeder Task bekommt Multi-Agent Verification
- Geschätzte Zeit: 4-6 Stunden (vs. 50-60h sequentiell)
```

### Option B: Prioritätsbasiert
```
Zuerst P0 (Critical):
1. QW-1: Recursive Mutex
2. QW-2: Target Selection
4. QW-4: ClassAI Static Bug

Dann P1 (High):
3. QW-3: Container reserve()
5. QW-5: Spatial Query Cache

Zuletzt P2:
6. QW-6: Naming Consistency
```

### Option C: Nach Impact
```
Höchster Impact zuerst:
1. QW-2: Target Selection (80-90% reduction!)
2. QW-5: Spatial Query Cache (80% reduction)
3. QW-1: Recursive Mutex (15-25% CPU)
4. QW-4: ClassAI Static Bug (Memory Leak)
5. QW-3: Container reserve() (2-5% CPU)
6. QW-6: Naming Consistency (Code Quality)
```
