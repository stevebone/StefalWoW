# Phase 3 Cross-Bot Coordination — Audit Results
## Sprint 1: Code-Audit & Duplikat-Analyse
**Generated:** 2026-02-06
**Branch:** playerbot-dev

---

## 1. Executive Summary

This document presents the complete audit findings for Phase 3 Cross-Bot Event Coordination. The audit analyzed **100+ coordination files** across 10 subsystems to determine status (ACTIVE/PARTIAL/DEAD/DUPLICATE), validate event flows, and identify gaps.

### Critical Findings

| Category | Count | Impact |
|----------|-------|--------|
| Files NOT COMPILED (Dead Code) | 34+ files | 🔴 CRITICAL |
| Missing EventBus Files | 11 files | 🔴 CRITICAL |
| Duplicate Systems | 5 pairs | 🟡 MEDIUM |
| Gaps Confirmed | 5 gaps | 🔴 CRITICAL |

### Action Priority

1. **IMMEDIATE**: Add missing files to CMakeLists.txt (Arena, Raid, Group dirs)
2. **SPRINT 2**: Create CooldownEventBus infrastructure (GAP 1)
3. **SPRINT 2**: Create BotMessageBus for claim coordination
4. **SPRINT 3**: Consolidate duplicate systems

---

## 2. CMakeLists.txt Audit — CRITICAL FINDINGS

### 2.1 Files EXIST but NOT COMPILED

The following directories contain source files that are NOT included in CMakeLists.txt and therefore are **DEAD CODE** that never compiles:

#### AI/Coordination/Arena/ — 🔴 NOT COMPILED (12 files)
```
ArenaCoordinator.cpp/h         — Main arena coordination
ArenaPositioning.cpp/h         — Arena positioning logic
BurstCoordinator.cpp/h         — Burst window coordination
CCChainManager.cpp/h           — CC chain management
DefensiveCoordinator.cpp/h     — Arena defensive CDs
KillTargetManager.cpp/h        — Kill target selection
```
**Status:** Files exist, implemented, but NEVER compile. Dead code.

#### AI/Coordination/Raid/ — 🔴 NOT COMPILED (18 files)
```
RaidCoordinator.cpp/h          — Main raid coordination
RaidCooldownRotation.cpp/h     — Raid CD rotation
RaidEncounterManager.cpp/h     — Encounter strategies
RaidGroupManager.cpp/h         — Raid group management
RaidHealCoordinator.cpp/h      — Heal coordination
RaidPositioningManager.cpp/h   — Raid positioning
RaidTankCoordinator.cpp/h      — Tank coordination
AddManagementSystem.cpp/h      — Add management
KitingManager.cpp/h            — Raid kiting
RaidState.h                    — Raid state definitions
```
**Status:** Files exist, implemented, but NEVER compile. Dead code.
**Note:** Only `AI/Coordination/RaidOrchestrator.cpp/h` compiles (in parent directory).

#### Group/ — 🔴 PARTIALLY NOT COMPILED (4 files)
```
GroupCoordination.cpp/h        — Group coordination (NOT COMPILED)
GroupFormation.cpp/h           — Group formation (NOT COMPILED)
```
**Status:** Files exist but are NOT in CMakeLists.txt.

### 2.2 Missing EventBus Files

The following EventBus files are marked as "TODO: File missing" in CMakeLists.txt:

| File | Location | Status |
|------|----------|--------|
| CooldownEventBus.h | Cooldown/ | 🔴 MISSING |
| CombatEventBus.h | Combat/ | 🔴 MISSING |
| GroupEventBus.h | Group/ | 🔴 MISSING |
| AuraEventBus.h | Aura/ | 🔴 REMOVED |
| QuestEventBus.h | Quest/ | 🔴 MISSING |
| LootEventBus.h | Loot/ | 🔴 MISSING |
| ResourceEventBus.h | Resource/ | 🔴 MISSING |
| InstanceEventBus.h | Instance/ | 🔴 MISSING |
| NPCEventBus.h | NPC/ | 🔴 MISSING |
| ProfessionEventBus.h | Professions/ | 🔴 MISSING |
| SocialEventBus.h | Social/ | 🔴 REMOVED |

**Impact:** The EventBus infrastructure exists as `*Events.cpp/h` files but the bus files themselves were never created or were removed.

### 2.3 Compiled vs Non-Compiled Summary

| Directory | Compiled Files | Non-Compiled Files |
|-----------|---------------|-------------------|
| AI/Coordination/Arena/ | 0 | 12 |
| AI/Coordination/Raid/ | 0 | 18+ |
| AI/Coordination/Dungeon/ | 12 | 0 |
| AI/Coordination/Battleground/ | 40+ | 0 |
| AI/Combat/ | 30+ | 0 |
| AI/CombatBehaviors/ | 10 | 0 |
| Group/ | 18 | 4 |
| Advanced/ | 8 | 0 |

---

## 3. Duplicate System Analysis

### 3.1 Interrupt System — 4 Files, 3 ACTIVE

| File | Status | Purpose | Recommendation |
|------|--------|---------|----------------|
| InterruptCoordinator.cpp/h | 🟢 ACTIVE | Group-level, ICombatEventSubscriber | Keep as group coordinator |
| UnifiedInterruptSystem.cpp/h | 🟢 ACTIVE | Singleton, comprehensive unified system | **CANONICAL** |
| InterruptRotationManager.cpp/h | 🟢 ACTIVE | Per-bot instance, tied to BotAI | Keep for per-bot tracking |
| InterruptManager.cpp/h | 🟡 PARTIAL | Simple manager | Deprecate, merge into Unified |
| InterruptAwareness.cpp/h | 🟢 ACTIVE | Awareness layer | Keep as detection layer |
| InterruptDatabase.cpp/h | 🟢 ACTIVE | Spell database | Keep as data source |

**Verdict:** Not true duplicates — complementary systems.
- **UnifiedInterruptSystem** = Canonical singleton for system-wide coordination
- **InterruptCoordinator** = Group-level event subscriber
- **InterruptRotationManager** = Per-bot rotation tracking

**Recommendation:** Keep all three but clarify responsibilities:
1. UnifiedInterruptSystem: Global coordination + spell database
2. InterruptCoordinator: Group-level event subscription
3. InterruptRotationManager: Per-bot cooldown/rotation tracking

### 3.2 Defensive System — 3 Files

| File | Status | Purpose | Recommendation |
|------|--------|---------|----------------|
| DefensiveManager.cpp/h (AI/Combat/) | 🟢 ACTIVE | General defensive CD management | Keep as base |
| DefensiveBehaviorManager.cpp/h (CombatBehaviors/) | 🟢 ACTIVE | Behavior-based defensive decisions | Keep, extends DefensiveManager |
| DefensiveCoordinator.cpp/h (Arena/) | 🔴 DEAD | Arena-specific (NOT COMPILED) | Add to CMakeLists, keep for Arena |

**Verdict:** DefensiveManager and DefensiveBehaviorManager have different scopes.
- **DefensiveManager**: Low-level CD tracking
- **DefensiveBehaviorManager**: High-level behavior decisions

**Recommendation:** Keep separate, add Arena/DefensiveCoordinator to CMakeLists.

### 3.3 Formation System — 2 Files

| File | Status | Purpose | Recommendation |
|------|--------|---------|----------------|
| FormationManager.cpp/h (AI/Combat/) | 🟢 ACTIVE | Combat formation positioning | Keep for combat |
| GroupFormation.cpp/h (Group/) | 🔴 DEAD | Group formation (NOT COMPILED) | Add to CMakeLists |

**Verdict:** FormationManager handles combat, GroupFormation handles out-of-combat.

**Recommendation:** Add GroupFormation to CMakeLists. Both serve different purposes.

### 3.4 Group Coordination — 2 Files

| File | Status | Purpose | Recommendation |
|------|--------|---------|----------------|
| GroupCoordinator.cpp/h (Advanced/) | 🟢 ACTIVE | Advanced group behaviors | Keep |
| GroupCoordination.cpp/h (Group/) | 🔴 DEAD | Basic group coordination (NOT COMPILED) | Add to CMakeLists |

**Verdict:** Need to examine code to determine overlap.

**Recommendation:** Add GroupCoordination to CMakeLists, then evaluate for merge.

### 3.5 Kiting System — 2 Files

| File | Status | Purpose | Recommendation |
|------|--------|---------|----------------|
| KitingManager.cpp/h (AI/Combat/) | 🟢 ACTIVE | General kiting behavior | Keep as base class |
| KitingManager.cpp/h (AI/Coordination/Raid/) | 🔴 DEAD | Raid kiting (NOT COMPILED) | Add to CMakeLists |

**Verdict:** Raid version likely extends Combat version.

**Recommendation:** Add Raid/KitingManager to CMakeLists. Implement inheritance if not already.

---

## 4. Gap Verification

### GAP 1: CooldownEventBus — 🔴 CONFIRMED DEAD

**Evidence:**
- `Cooldown/CooldownEventBus.h` marked as "TODO: File missing" in CMakeLists.txt
- `Cooldown/CooldownEvents.cpp/h` exists but publishes nowhere
- `Network/ParseCooldownPacket_Typed.cpp` exists but doesn't publish events
- No COOLDOWN_STARTED or COOLDOWN_ENDED events found being published

**Impact:** Bots cannot coordinate cooldowns. RaidCooldownRotation is blind.

**Fix Required:**
1. Create `CooldownEventBus.h/cpp`
2. Update `ParseCooldownPacket_Typed.cpp` to publish COOLDOWN_STARTED/COOLDOWN_ENDED
3. Subscribe RaidCooldownRotation, DefensiveBehaviorManager, BurstCoordinator

### GAP 2: Dispel Rotation — 🟡 CONFIRMED NO ASSIGNMENT

**Evidence:**
- `AI/CombatBehaviors/DispelCoordinator.cpp/h` exists and is compiled
- No claim system found - multiple bots may dispel same target
- DISPELLABLE_DETECTED events are published but no assignment logic

**Impact:** Wasted GCDs, mana, overlapping dispels.

**Fix Required:**
1. Implement dispeller priority queue in DispelCoordinator
2. Add claim system via BotMessageBus CLAIM_DISPEL

### GAP 3: External Defensive CD Rotation — 🟡 CONFIRMED NO ASSIGNMENT

**Evidence:**
- DefensiveBehaviorManager exists but no external CD assignment
- No "danger window" tracking (6s protection after external CD)
- No coordination between Pain Suppression, Guardian Spirit, etc.

**Impact:** Wasted external CDs on same target.

**Fix Required:**
1. Add ExternalCDTier enum to DefensiveBehaviorManager
2. Implement danger window tracking
3. Coordinate with RaidCooldownRotation

### GAP 4: BossAbilityDatabase — 🟡 NO DATABASE EXISTS

**Evidence:**
- No BossAbilityDatabase.cpp/h file found
- BossEncounterManager has basic phase tracking but no ability reactions
- No pre-positioning, pre-potting, or ability warnings

**Impact:** Bots don't react to boss abilities properly.

**Fix Required:**
1. Create BossAbilityDatabase.cpp/h
2. Map boss Entry + SpellID → BotReaction
3. Integrate with RaidEncounterManager and DungeonCoordinator

### GAP 5: M+ Affix Support — 🟡 PARTIAL IMPLEMENTATION

**Evidence:**
- MythicPlusManager.cpp exists and is compiled
- Only basic affix handling found

**Affixes Needing Implementation:**
- Incorporeal, Afflicted, Entangling, Bursting
- Spiteful, Storming, Volcanic, Raging

**Fix Required:**
1. Create IAffixHandler interface
2. Implement handler per affix
3. Register handlers in MythicPlusManager

---

## 5. Event Bus Status

| # | Event Bus | Status | Publishers | Subscribers | Notes |
|---|-----------|--------|------------|-------------|-------|
| 1 | CombatEventBus | 🟡 PARTIAL | CombatEventRouter | 5+ | Events.h exists, Bus.h missing |
| 2 | GroupEventBus | 🟡 PARTIAL | GroupEvents | 3+ | Events.h exists, Bus.h missing |
| 3 | LootEventBus | 🟡 PARTIAL | LootEvents | 2+ | Events.h exists, Bus.h missing |
| 4 | QuestEventBus | 🟡 PARTIAL | QuestEvents | 2+ | Events.h exists, Bus.h missing |
| 5 | AuctionEventBus | 🟡 PARTIAL | AuctionEvents | 1+ | Events.h exists, Bus.h missing |
| 6 | ResourceEventBus | 🟡 PARTIAL | ResourceEvents | 1+ | Events.h exists, Bus.h missing |
| 7 | SocialEventBus | 🟡 PARTIAL | SocialEvents | 1+ | Events.h removed |
| 8 | InstanceEventBus | 🟡 PARTIAL | InstanceEvents | 1+ | Events.h exists, Bus.h missing |
| 9 | NPCEventBus | 🟡 PARTIAL | NPCEvents | 1+ | Events.h exists, Bus.h missing |
| 10 | AuraEventBus | 🟡 PARTIAL | AuraEvents | 2+ | Events.h exists, Bus.h removed |
| 11 | ProfessionEventBus | 🟡 PARTIAL | ProfessionEvents | 1+ | Events.h exists, Bus.h missing |
| 12 | BotSpawnEventBus | 🟢 ACTIVE | Full implementation | 3+ | Complete |
| 13 | CooldownEventBus | 🔴 DEAD | 0 | 0 | No events published (GAP 1) |

**Pattern Observed:** Event data structures (*Events.h/cpp) exist, but dedicated EventBus wrappers were removed or never created. Events are likely published via GenericEventBus or EventDispatcher directly.

---

## 6. Subsystem Status Summary

### 6.1 Dungeon Coordination — 🟢 FULLY COMPILED

| File | Status | Implementation |
|------|--------|----------------|
| DungeonCoordinator | 🟢 ACTIVE | Full |
| TrashPullManager | 🟢 ACTIVE | Full |
| BossEncounterManager | 🟢 ACTIVE | Partial (no BossAbilityDatabase) |
| WipeRecoveryManager | 🟢 ACTIVE | Full |
| MythicPlusManager | 🟢 ACTIVE | Partial (missing affixes) |

### 6.2 Raid Coordination — 🔴 NOT COMPILED

| File | Status | Implementation |
|------|--------|----------------|
| RaidCoordinator | 🔴 DEAD | Full but NOT COMPILED |
| RaidCooldownRotation | 🔴 DEAD | Full but NOT COMPILED |
| RaidEncounterManager | 🔴 DEAD | Full but NOT COMPILED |
| RaidGroupManager | 🔴 DEAD | Full but NOT COMPILED |
| RaidHealCoordinator | 🔴 DEAD | Full but NOT COMPILED |
| RaidPositioningManager | 🔴 DEAD | Full but NOT COMPILED |
| RaidTankCoordinator | 🔴 DEAD | Full but NOT COMPILED |
| AddManagementSystem | 🔴 DEAD | Full but NOT COMPILED |
| Raid/KitingManager | 🔴 DEAD | Full but NOT COMPILED |

### 6.3 Arena Coordination — 🔴 NOT COMPILED

| File | Status | Implementation |
|------|--------|----------------|
| ArenaCoordinator | 🔴 DEAD | Full but NOT COMPILED |
| ArenaPositioning | 🔴 DEAD | Full but NOT COMPILED |
| BurstCoordinator | 🔴 DEAD | Full but NOT COMPILED |
| CCChainManager | 🔴 DEAD | Full but NOT COMPILED |
| DefensiveCoordinator | 🔴 DEAD | Full but NOT COMPILED |
| KillTargetManager | 🔴 DEAD | Full but NOT COMPILED |

### 6.4 Battleground Coordination — 🟢 FULLY COMPILED

| Component | Status | Files |
|-----------|--------|-------|
| BattlegroundCoordinatorManager | 🟢 ACTIVE | 2 |
| BGStrategyEngine | 🟢 ACTIVE | 2 |
| BGRoleManager | 🟢 ACTIVE | 2 |
| FlagCarrierManager | 🟢 ACTIVE | 2 |
| NodeController | 🟢 ACTIVE | 2 |
| ObjectiveManager | 🟢 ACTIVE | 2 |
| BGScripts (CTF, Domination, Siege, Resource, Epic) | 🟢 ACTIVE | 40+ |

### 6.5 Combat Coordination — 🟢 FULLY COMPILED

| File | Status | Duplicate Status |
|------|--------|-----------------|
| InterruptCoordinator | 🟢 ACTIVE | Complementary |
| UnifiedInterruptSystem | 🟢 ACTIVE | Canonical |
| InterruptRotationManager | 🟢 ACTIVE | Complementary |
| DefensiveManager | 🟢 ACTIVE | Base class |
| DefensiveBehaviorManager | 🟢 ACTIVE | Extends base |
| CrowdControlManager | 🟢 ACTIVE | Unique |
| ThreatCoordinator | 🟢 ACTIVE | Unique |
| FormationManager | 🟢 ACTIVE | Combat-specific |
| PositionManager | 🟢 ACTIVE | Unique |
| TargetManager | 🟢 ACTIVE | Unique |

---

## 7. Recommended Actions

### 7.1 Immediate — Add to CMakeLists.txt

**Files to add to `PLAYERBOT_AI_BASE_SOURCES`:**
```cmake
# AI Coordination - Arena
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/ArenaCoordinator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/ArenaCoordinator.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/ArenaPositioning.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/ArenaPositioning.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/BurstCoordinator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/BurstCoordinator.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/CCChainManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/CCChainManager.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/DefensiveCoordinator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/DefensiveCoordinator.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/KillTargetManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Arena/KillTargetManager.h

# AI Coordination - Raid
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidState.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidCoordinator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidCoordinator.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidCooldownRotation.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidCooldownRotation.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidEncounterManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidEncounterManager.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidGroupManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidGroupManager.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidHealCoordinator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidHealCoordinator.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidPositioningManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidPositioningManager.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidTankCoordinator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/RaidTankCoordinator.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/AddManagementSystem.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/AddManagementSystem.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/KitingManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/Coordination/Raid/KitingManager.h
```

**Files to add to `PLAYERBOT_GAMEPLAY_SOURCES`:**
```cmake
# Group Coordination (currently missing)
${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupCoordination.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupCoordination.h
${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupFormation.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Group/GroupFormation.h
```

### 7.2 Sprint 2 — New Infrastructure

| Component | New Files | Priority |
|-----------|-----------|----------|
| CooldownEventBus | Cooldown/CooldownEventBus.cpp/h | 🔴 CRITICAL |
| BotMessageBus | AI/Coordination/Messaging/BotMessageBus.cpp/h | 🔴 CRITICAL |
| ClaimResolver | AI/Coordination/Messaging/ClaimResolver.cpp/h | 🔴 CRITICAL |
| ContentContextManager | AI/Coordination/ContentContextManager.cpp/h | 🟡 HIGH |

### 7.3 Sprint 3 — Consolidation

| Action | From | To | Priority |
|--------|------|----|----------|
| Document role | InterruptCoordinator | Keep, clarify as group subscriber | 🟡 HIGH |
| Document role | UnifiedInterruptSystem | Keep as canonical singleton | 🟡 HIGH |
| Document role | InterruptRotationManager | Keep as per-bot tracker | 🟡 HIGH |
| Merge | InterruptManager | Into UnifiedInterruptSystem | 🟢 LOW |

---

## 8. Conclusion

The audit reveals that **Phase 3 Cross-Bot Coordination has significant infrastructure issues**:

1. **34+ coordination files exist but are NOT compiled** — the Arena and Raid coordination systems are complete but dead code
2. **11 EventBus files are missing** — event infrastructure is incomplete
3. **5 critical gaps confirmed** — CooldownEventBus dead, no dispel/defensive assignment
4. **Duplicate systems are mostly complementary** — not pure duplicates requiring aggressive consolidation

**Immediate Priority:**
1. Add Arena and Raid coordination files to CMakeLists.txt
2. Create CooldownEventBus and BotMessageBus infrastructure
3. Fix GAP 1-3 before any other coordination work

---

## 9. Sprint 1 Audit Completion Notes (2026-02-06)

### 9.1 API Migration Work Discovered

During attempt to add Arena/Raid files to CMakeLists.txt, the following API incompatibilities were discovered:

| Issue | Affected Files | Current API | Required API |
|-------|---------------|-------------|--------------|
| Event Type | 34+ files | `CombatEventData` | `CombatEvent` |
| Include Path | 6 files | `Core/Events/CombatEventData.h` | `Core/Events/CombatEvent.h` |
| Event Fields | 6+ files | `event.sourceGuid` | `event.source` |
| Role Detection | 2 files | `GetRoleBySpecialization()` | `IsTankSpecialization()` / `IsHealerSpecialization()` |
| Role Constants | 2 files | `ROLE_TANK`, `ROLE_HEALER` | Use spec enum checks |
| Include Path | 2 files | `Combat/CrowdControl/` | `AI/Combat/` |
| Class Constants | 1 file | Missing `CLASS_*` | Add `SharedDefines.h` |
| Override Issue | 2 files | `GetPriority() override` | Classes don't inherit ICombatEventSubscriber |

### 9.2 Partial Fixes Applied

The following files were partially fixed before determining scope too large for Sprint 1:

| File | Fixes Applied |
|------|--------------|
| RaidCoordinator.h | Include path, event type, remove override |
| RaidCoordinator.cpp | Role helpers, CombatEvent type, field names, comment out subscription |
| RaidHealCoordinator.h | Include path, event type |
| RaidTankCoordinator.h | Include path |
| RaidCooldownRotation.h | Include path |
| RaidEncounterManager.h | Include path |
| KitingManager.h | Include path |
| AddManagementSystem.h | Include path |
| RaidGroupManager.cpp | Role helpers, remove constants |
| ArenaCoordinator.h | Include path, remove override |
| BurstCoordinator.cpp | Add SharedDefines.h |
| CCChainManager.cpp | Fix CrowdControlManager include |
| ArenaCoordinator.cpp | Fix CrowdControlManager include |

### 9.3 CMakeLists.txt Current State

The Arena and Raid coordination files are **COMMENTED OUT** in CMakeLists.txt with explanation:

```cmake
# ============================================================================
# PHASE 3 AUDIT: Arena/Raid Coordination Files (DISCOVERED - NOT YET BUILDABLE)
# ============================================================================
# These 34 files were found during Sprint 1 audit but have stale APIs that
# need CombatEventData -> CombatEvent migration. This is Sprint 2+ work.
# See: .claude/PHASE3/AUDIT_RESULTS.md for full details
```

**Build Status:** ✅ playerbot-ai-base compiles successfully with Arena/Raid files commented out.

### 9.4 Sprint 2 Prerequisites

Before enabling Arena/Raid files in CMakeLists.txt, the following must be completed:

1. **CombatEventData → CombatEvent migration** in all 34 files
2. **Update method signatures** in all sub-managers (OnDamageEvent, OnAuraEvent, etc.)
3. **Decide inheritance**: Should RaidCoordinator/ArenaCoordinator inherit from ICombatEventSubscriber?
4. **Update event field access**: `sourceGuid` → `source`, `eventType` → `type`

### 9.5 Group Coordination Files

The Group coordination files (GroupCoordination.h/cpp, GroupFormation.h/cpp) **ARE** included in CMakeLists.txt (line 945-948) and compile successfully.

---

*Generated by Sprint 1 Code Audit*
*Audit Completed: 2026-02-06*
*Next: CONSOLIDATION_PLAN.md → Sprint 2 Implementation*
