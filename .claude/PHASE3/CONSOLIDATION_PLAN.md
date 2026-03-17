# Phase 3 Cross-Bot Coordination — Consolidation Plan
## Sprint 1 Deliverable: Action Plan for Code Integration

**Generated:** 2026-02-06
**Based on:** AUDIT_RESULTS.md

---

## 1. Critical Path — CMakeLists.txt Fixes

### 1.1 Priority 1: Enable Dead Code (IMMEDIATE)

The following action MUST happen before any other Sprint 2+ work:

**Task:** Add 34+ files to CMakeLists.txt to enable compilation

**Impact:**
- Arena coordination will compile and be usable
- Raid coordination will compile and be usable
- Group formation will compile and be usable

**Estimated:** 1 hour

### 1.2 CMakeLists.txt Modification

Add to `PLAYERBOT_AI_BASE_SOURCES` section after existing Dungeon files (~line 296):

```cmake
# AI Coordination - Arena (ADDED BY PHASE 3 AUDIT)
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/ArenaCoordinator.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/ArenaCoordinator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/ArenaPositioning.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/ArenaPositioning.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/BurstCoordinator.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/BurstCoordinator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/CCChainManager.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/CCChainManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/DefensiveCoordinator.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/DefensiveCoordinator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/KillTargetManager.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/KillTargetManager.cpp

# AI Coordination - Raid (ADDED BY PHASE 3 AUDIT)
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidState.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidCoordinator.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidCoordinator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidCooldownRotation.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidCooldownRotation.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidEncounterManager.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidEncounterManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidGroupManager.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidGroupManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidHealCoordinator.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidHealCoordinator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidPositioningManager.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidPositioningManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidTankCoordinator.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidTankCoordinator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/AddManagementSystem.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/AddManagementSystem.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/KitingManager.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/KitingManager.cpp
```

Add to `PLAYERBOT_GAMEPLAY_SOURCES` section after existing Group files (~line 901):

```cmake
# Group Coordination (ADDED BY PHASE 3 AUDIT)
${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupCoordination.h
${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupCoordination.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupFormation.h
${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupFormation.cpp
```

Add include directories to `PLAYERBOT_INCLUDE_DIRS` (~line 1163):

```cmake
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid
```

---

## 2. Duplicate System Consolidation

### 2.1 Interrupt System — CLARIFY, DON'T MERGE

**Current State:**
- InterruptCoordinator: Group-level, ICombatEventSubscriber
- UnifiedInterruptSystem: Singleton, comprehensive
- InterruptRotationManager: Per-bot, BotAI instance

**Decision:** Keep all three — they serve different roles.

**Action Plan:**
1. Add header comment to each file clarifying its role
2. Document the relationship in code comments
3. Ensure UnifiedInterruptSystem uses InterruptCoordinator for group events
4. Ensure InterruptRotationManager is used by individual bots for cooldown tracking

**No code changes required** — just documentation.

### 2.2 Defensive System — KEEP SEPARATE

**Current State:**
- DefensiveManager: Low-level CD tracking (AI/Combat/)
- DefensiveBehaviorManager: High-level behavior decisions (CombatBehaviors/)
- DefensiveCoordinator: Arena-specific (Arena/ — NOT COMPILED)

**Decision:** Keep separate, add Arena/DefensiveCoordinator to build.

**Action Plan:**
1. Add DefensiveCoordinator to CMakeLists.txt (included in Section 1)
2. Verify inheritance relationship between DefensiveManager and DefensiveBehaviorManager
3. Keep DefensiveCoordinator for Arena-specific logic

### 2.3 Formation System — KEEP SEPARATE

**Current State:**
- FormationManager: Combat formation (AI/Combat/)
- GroupFormation: Out-of-combat group formation (Group/ — NOT COMPILED)

**Decision:** Keep both — different purposes.

**Action Plan:**
1. Add GroupFormation to CMakeLists.txt (included in Section 1)
2. Verify no code overlap
3. Document: FormationManager = combat, GroupFormation = travel/idle

### 2.4 Group Coordination — EVALUATE AFTER COMPILATION

**Current State:**
- GroupCoordinator: Advanced group behaviors (Advanced/)
- GroupCoordination: Basic group coordination (Group/ — NOT COMPILED)

**Decision:** Add to build first, then evaluate for merge.

**Action Plan:**
1. Add GroupCoordination to CMakeLists.txt (included in Section 1)
2. After compilation succeeds, compare APIs
3. If significant overlap: Create GroupCoordinationHub merging both
4. If different purposes: Keep separate, document roles

### 2.5 Kiting System — INHERITANCE PATTERN

**Current State:**
- KitingManager: General kiting (AI/Combat/)
- KitingManager: Raid kiting (AI/Coordination/Raid/ — NOT COMPILED)

**Decision:** Implement inheritance pattern.

**Action Plan:**
1. Add Raid/KitingManager to CMakeLists.txt (included in Section 1)
2. Verify Raid version inherits from Combat version
3. If not, refactor to use inheritance:
   ```cpp
   class RaidKitingManager : public KitingManager {
       // Raid-specific overrides
   };
   ```

---

## 3. New Infrastructure Components

### 3.1 BotMessageBus (Sprint 2)

**Purpose:** Direct bot-to-bot communication with claim resolution

**New Files:**
```
AI/Coordination/Messaging/
├── MessageTypes.h           — Message type enums
├── BotMessage.h             — Message data structure
├── BotMessageBus.h          — Bus interface
├── BotMessageBus.cpp        — Bus implementation
├── ClaimResolver.h          — Claim resolution interface
└── ClaimResolver.cpp        — First-claim-wins logic
```

**Key Features:**
- CLAIM_INTERRUPT, CLAIM_DISPEL, CLAIM_DEFENSIVE_CD
- ANNOUNCE_CD_USAGE, ANNOUNCE_BURST_WINDOW
- REQUEST_HEAL, REQUEST_TANK_SWAP
- 200ms claim timeout with priority override

### 3.2 CooldownEventBus Fix (Sprint 2)

**Purpose:** Enable cooldown coordination between bots

**Modified Files:**
```
Network/ParseCooldownPacket_Typed.cpp  — Add event publishing
Cooldown/CooldownEvents.cpp            — Add new event types
Cooldown/CooldownEvents.h              — Add new event types
```

**New Events:**
- COOLDOWN_STARTED { botGuid, spellId, duration, category }
- COOLDOWN_ENDED { botGuid, spellId }
- MAJOR_CD_AVAILABLE { botGuid, spellId }
- MAJOR_CD_USED { botGuid, spellId, duration }

**Subscribers:**
- RaidCooldownRotation → MAJOR_CD_*
- DefensiveBehaviorManager → MAJOR_CD_USED, COOLDOWN_ENDED
- BurstCoordinator → COOLDOWN_*

### 3.3 ContentContextManager (Sprint 2)

**Purpose:** Automatic content type detection for coordination level

**New Files:**
```
AI/Coordination/ContentContextManager.h
AI/Coordination/ContentContextManager.cpp
```

**Output Structure:**
```cpp
struct ContentContext {
    ContentType type;           // DUNGEON_HEROIC, RAID_MYTHIC, ARENA_3V3...
    float coordinationLevel;    // 0.0 (solo) - 1.0 (mythic raid)
    uint32 groupSize;
    uint32 difficultyId;
    uint32 mythicPlusLevel;     // 0 if not M+
    std::vector<uint32> activeAffixes;
    uint32 encounterEntry;      // Current boss (0 if trash)
};
```

### 3.4 BossAbilityDatabase (Sprint 5)

**Purpose:** Map boss abilities to bot reactions

**New Files:**
```
AI/Coordination/BossAbilityDatabase.h
AI/Coordination/BossAbilityDatabase.cpp
```

**Data Structure:**
```cpp
struct BossAbility {
    uint32 bossEntry;
    uint32 spellId;
    BossAbilityType type;     // FRONTAL, AOE, TANK_BUSTER, etc.
    float dangerLevel;
    std::vector<BotReaction> reactions;
};
```

---

## 4. Migration Steps

### 4.1 Step 1: CMakeLists.txt Update (IMMEDIATE)

1. Edit `src/modules/Playerbot/CMakeLists.txt`
2. Add all files from Section 1
3. Build and verify compilation succeeds
4. Fix any compilation errors in newly-added files

### 4.2 Step 2: Compilation Error Fixes (IMMEDIATE)

After adding files to CMakeLists.txt, expect potential:
- Missing include paths
- Forward declaration issues
- Namespace conflicts

Fix each error systematically.

### 4.3 Step 3: Sprint 2 Infrastructure (Days 1-5)

1. Create BotMessageBus
2. Create ClaimResolver
3. Fix CooldownEventBus
4. Create ContentContextManager

### 4.4 Step 4: Sprint 3 Combat Consolidation (Days 6-12)

1. Document interrupt system roles
2. Integrate BotMessageBus CLAIM_INTERRUPT
3. Fix GAP 2 (Dispel assignment)
4. Fix GAP 3 (Defensive CD assignment)

### 4.5 Steps 5-7: Content Coordination (Days 13-30)

1. Sprint 4: Dungeon/M+ enhancements
2. Sprint 5: Raid coordination + BossAbilityDatabase
3. Sprint 6: Arena + BG coordination

---

## 5. Verification Checklist

### After CMakeLists.txt Update
- [ ] All Arena files compile without error
- [ ] All Raid files compile without error
- [ ] GroupCoordination compiles without error
- [ ] GroupFormation compiles without error
- [ ] Full build succeeds with 0 errors

### After Sprint 2
- [ ] CooldownEventBus publishes COOLDOWN_STARTED events
- [ ] BotMessageBus delivers CLAIM_INTERRUPT messages
- [ ] ClaimResolver resolves claims in <200ms
- [ ] ContentContextManager detects M+ level correctly

### After Sprint 3
- [ ] Bots don't duplicate interrupts (claim system works)
- [ ] Bots don't duplicate dispels (claim system works)
- [ ] External defensive CDs are assigned without overlap
- [ ] Interrupt rotation fairness is maintained

---

## 6. Risk Mitigation

| Risk | Mitigation |
|------|------------|
| Compilation errors after adding files | Expect 1-2 hours of error fixing |
| Namespace conflicts | Use consistent `Playerbot::` namespace |
| Include order issues | Update include directories in CMakeLists |
| Circular dependencies | Use forward declarations |
| Performance regression | Profile before/after with 100+ bots |

---

*Generated by Sprint 1 Consolidation Planning*
*Proceed to: CMakeLists.txt modification*
