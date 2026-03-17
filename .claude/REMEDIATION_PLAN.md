# Playerbot Codebase Remediation Plan

**Date:** November 27, 2025
**Version:** 1.0
**Status:** IN PROGRESS

---

## Executive Summary

This remediation plan addresses all incomplete implementations, stubs, placeholders, and workarounds identified in the comprehensive audit. Each fix is designed with enterprise-grade quality standards: complete implementation, proper error handling, TrinityCore API compliance, and full documentation.

---

## Remediation Phases

### PHASE 1: Critical Code Cleanup (Immediate)
**Priority:** HIGH
**Impact:** Removes technical debt, improves code clarity

| Task | File | Action | Status |
|------|------|--------|--------|
| 1.1 | InteractionManager.cpp | Remove 32 obsolete TODO comments | PENDING |
| 1.2 | 4 Combat Headers | Update docstrings (remove "stub" language) | PENDING |

### PHASE 2: Combat System Completion
**Priority:** HIGH
**Impact:** Core combat functionality

| Task | File | Action | Status |
|------|------|--------|--------|
| 2.1 | TargetManager.cpp | Verify full implementation | PENDING |
| 2.2 | CrowdControlManager.cpp | Verify full implementation | PENDING |
| 2.3 | DefensiveManager.cpp | Verify full implementation | PENDING |
| 2.4 | MovementIntegration.cpp | Verify full implementation | PENDING |
| 2.5 | TargetAssistAction.cpp | Implement proper TrinityCore API | PENDING |

### PHASE 3: Strategy System Completion
**Priority:** MEDIUM
**Impact:** Bot autonomy and decision making

| Task | File | Action | Status |
|------|------|--------|--------|
| 3.1 | SoloStrategy.cpp | Add 11 missing manager helper methods | PENDING |
| 3.2 | QuestManager | Add HasActiveQuests(), ProgressQuests(), etc. | PENDING |
| 3.3 | GatheringManager | Add HasGatheringProfession(), GatherNearestResource() | PENDING |
| 3.4 | TradeManager | Add HasItemsToSell(), HasItemsToBuy(), ProcessPendingTrades() | PENDING |
| 3.5 | AuctionManager | Add HasPendingAuctions(), HasExpiredAuctions(), ProcessAuctions() | PENDING |

### PHASE 4: Persistence & Database
**Priority:** MEDIUM
**Impact:** Bot state management

| Task | File | Action | Status |
|------|------|--------|--------|
| 4.1 | BotStatePersistence.cpp | Replace 6 placeholders with real database ops | PENDING |
| 4.2 | BotAccountMgr.cpp | Implement proper timestamp handling | PENDING |

### PHASE 5: Class Rotations & AI
**Priority:** MEDIUM
**Impact:** Class-specific bot behavior

| Task | File | Action | Status |
|------|------|--------|--------|
| 5.1 | BaselineRotationManager.cpp | Complete all class rotations | PENDING |
| 5.2 | BehaviorTreeFactory.cpp | Replace placeholder spell IDs | PENDING |
| 5.3 | Hunter PetInfo | Implement proper pet system integration | PENDING |

### PHASE 6: Utility Systems
**Priority:** LOW
**Impact:** Supporting functionality

| Task | File | Action | Status |
|------|------|--------|--------|
| 6.1 | MountManager.cpp | Implement DBC/DB2 mount loading | PENDING |
| 6.2 | FlightMasterManager.cpp | Implement flight cost calculation | PENDING |
| 6.3 | AuctionManager.cpp | Implement price history tracking | PENDING |

---

## Detailed Implementation Specifications

### PHASE 1.1: InteractionManager.cpp Cleanup

**Current State:** Contains 32 TODO comments referencing files that "need to be created" but already exist as separate implementations.

**Implementation:**
1. Remove all obsolete `// TODO: Create this file` comments
2. Update header comment to document the facade pattern
3. Ensure all existing handler includes are properly documented

**Files Referenced (Already Exist):**
- TrainerInteractionManager.cpp
- VendorInteractionManager.cpp
- InnkeeperInteractionManager.cpp
- FlightMasterInteractionManager.cpp
- BankInteractionManager.cpp
- MailboxInteractionManager.cpp
- AuctioneerInteractionManager.cpp
- RepairInteractionManager.cpp
- StableMasterInteractionManager.cpp
- SpiritHealerInteractionManager.cpp

### PHASE 1.2: Combat Header Docstring Updates

**Current State:** Headers contain `**Problem**: Stub implementation with NO-OP methods`

**Implementation:**
1. Review corresponding .cpp files to verify implementation status
2. Update docstrings to reflect actual implementation state
3. Document any remaining gaps with specific action items

### PHASE 2.5: TargetAssistAction TrinityCore API Fix

**Current State:** `// This implementation is a stub due to TrinityCore API compatibility issues`

**Implementation:**
```cpp
// Research required TrinityCore APIs:
// - Unit::GetThreatManager()
// - ThreatReference::GetThreat()
// - Unit::GetVictim()
// - Player::GetGroup()
// - Group::GetMemberSlots()

// Implementation approach:
// 1. Get group members
// 2. Find tank/current target holder
// 3. Assist tank's target with proper threat consideration
// 4. Handle solo vs group scenarios
```

### PHASE 3: SoloStrategy Missing Methods

**QuestManager Methods to Add:**
```cpp
bool HasActiveQuests() const;
void ProgressQuests();
bool HasAvailableQuests() const;
bool HasCompletedQuests() const;
```

**GatheringManager Methods to Add:**
```cpp
bool HasGatheringProfession() const;
bool GatherNearestResource();
```

**TradeManager Methods to Add:**
```cpp
bool HasItemsToSell() const;
bool HasItemsToBuy() const;
void ProcessPendingTrades();
```

**AuctionManager Methods to Add:**
```cpp
bool HasPendingAuctions() const;
bool HasExpiredAuctions() const;
void ProcessAuctions();
```

### PHASE 4.1: BotStatePersistence Database Implementation

**Current State:** 6 placeholder implementations

**Implementation:**
```cpp
// Line 93: Replace placeholder async marking
// Use proper database transaction with CharacterDatabase

// Line 155: Replace placeholder return
// Query actual database for bot state

// Line 268: Replace placeholder implementation
// Use CharacterDatabasePreparedStatement

// Line 378: Replace placeholder implementation
// Implement proper state serialization

// Line 675: Replace placeholder database query
// Use CharacterDatabase.PQuery or prepared statements

// Line 700: Replace placeholder persistence
// Implement full state save with transaction
```

### PHASE 5.1: BaselineRotationManager Class Rotations

**Classes Needing Full Rotations:**
- Death Knight (Blood, Frost, Unholy)
- Demon Hunter (Havoc, Vengeance)
- Druid (Balance, Feral, Guardian, Restoration)
- Evoker (Devastation, Preservation, Augmentation)
- Hunter (Beast Mastery, Marksmanship, Survival)
- Mage (Arcane, Fire, Frost)
- Monk (Brewmaster, Mistweaver, Windwalker)
- Paladin (Holy, Protection, Retribution)
- Priest (Discipline, Holy, Shadow)
- Rogue (Assassination, Outlaw, Subtlety)
- Shaman (Elemental, Enhancement, Restoration)
- Warlock (Affliction, Demonology, Destruction)
- Warrior (Arms, Fury, Protection)

### PHASE 5.2: BehaviorTreeFactory Spell ID Fixes

**Current State:** Contains `// Placeholder spell ID` and `// Cast AoE heal (placeholder)`

**Implementation:**
- Query DBC/DB2 for actual class-specific AoE heal spell IDs
- Implement class detection and appropriate spell selection
- Handle talent variations that modify spell IDs

### PHASE 6.1: MountManager DBC/DB2 Loading

**Current State:** `// This is a stub implementation - full mount database would load from DBC/DB2`

**Implementation:**
```cpp
// Use MountStore from DBC
// Load mount capabilities (flying, swimming, ground)
// Implement speed tier detection
// Support profession mounts, achievement mounts, etc.
```

---

## Quality Standards

### Code Requirements
- Full implementation - no TODOs, stubs, or placeholders
- Comprehensive error handling
- TrinityCore API compliance
- Thread safety where applicable
- Memory management (RAII patterns)
- Performance optimization

### Documentation Requirements
- Doxygen-style comments for all public APIs
- Implementation notes for complex logic
- Update header docstrings to reflect implementation status

### Testing Requirements
- Unit tests for new functionality
- Integration tests for API changes
- Build verification after each phase

---

## Progress Tracking

### Completed
- [x] Audit complete
- [x] Remediation plan created

### In Progress
- [ ] Phase 1: Critical Code Cleanup

### Pending
- [ ] Phase 2: Combat System Completion
- [ ] Phase 3: Strategy System Completion
- [ ] Phase 4: Persistence & Database
- [ ] Phase 5: Class Rotations & AI
- [ ] Phase 6: Utility Systems

---

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Build breakage | Medium | High | Incremental builds after each fix |
| API incompatibility | Low | High | Research TrinityCore APIs before implementation |
| Missing dependencies | Low | Medium | Verify includes before implementation |
| Performance regression | Low | Medium | Profile after implementation |

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-11-27 | Initial remediation plan |
