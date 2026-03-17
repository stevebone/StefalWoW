# Phase 2.7: Comprehensive Cleanup & Consolidation

**Duration**: 2-3 weeks (2025-03-03 to 2025-03-24)
**Status**: ⏳ PENDING
**Owner**: Development Team

---

## Objectives

1. Delete deprecated Automation singleton systems
2. Investigate and resolve duplicate/legacy code
3. Consolidate manager hierarchy into unified structure
4. Remove dead code and unused files
5. Update build system and documentation

---

## Files Identified for Action

### ❌ CONFIRMED DELETE (8 files, ~2000 lines)

**Automation Singletons** - Replaced by Manager instances:
- `Quest/QuestAutomation.h`
- `Quest/QuestAutomation.cpp`
- `Social/TradeAutomation.h`
- `Social/TradeAutomation.cpp`
- `Social/AuctionAutomation.h`
- `Social/AuctionAutomation.cpp`
- `Professions/GatheringAutomation.h`
- `Professions/GatheringAutomation.cpp`

### ⚠️ INVESTIGATE (10 files)

**Potential Duplicates** (4 files):
- `AI/ResourceManager.h` vs `AI/ClassAI/ResourceManager.h`
- `AI/CooldownManager.h` vs `AI/ClassAI/CooldownManager.h`
- `AI/InterruptManager.h` vs `AI/Combat/InterruptManager.h`
- `AI/PositionManager.h` vs `AI/Combat/PositionManager.h`

**Backup/Test Files** (3 files):
- `AI/ClassAI/BaselineRotationManagerFixed.cpp`
- `Interaction/Core/InteractionManager_COMPLETE_FIX.cpp`
- `Economy/AuctionManager_UnitTest.cpp`

**Potentially Unused** (6 files):
- `Advanced/AdvancedBehaviorManager.h/cpp`
- `Advanced/EconomyManager.h/cpp`
- `Advanced/SocialManager.h/cpp`
- `Lifecycle/BotLifecycleManager.h`
- `Movement/Core/MovementManager.h/cpp`

---

## Cleanup Tasks

### Task 1: Delete Deprecated Files (2 days)

**Step 1.1**: Remove Automation source files
```bash
cd src/modules/Playerbot
rm Quest/QuestAutomation.{h,cpp}
rm Social/TradeAutomation.{h,cpp}
rm Social/AuctionAutomation.{h,cpp}
rm Professions/GatheringAutomation.{h,cpp}
```

**Step 1.2**: Remove all includes
```bash
# Find all files that include these headers
grep -r "QuestAutomation.h" --include="*.cpp" --include="*.h" src/modules/Playerbot/
grep -r "TradeAutomation.h" --include="*.cpp" --include="*.h" src/modules/Playerbot/
grep -r "AuctionAutomation.h" --include="*.cpp" --include="*.h" src/modules/Playerbot/
grep -r "GatheringAutomation.h" --include="*.cpp" --include="*.h" src/modules/Playerbot/

# Remove these includes from found files
```

**Expected files to update**:
- AI/BotAI.cpp (remove Automation includes)
- AI/Strategy/IdleStrategy.cpp (already updated)

**Step 1.3**: Update CMakeLists.txt
Remove deleted files from `SOURCES` list

**Step 1.4**: Verify compilation
```bash
cd build
cmake .. -DBUILD_PLAYERBOT=1
cmake --build . --target worldserver
```

---

### Task 2: Investigate Duplicates (3 days)

For each suspected duplicate pair, perform analysis:

**Investigation Template**:
```bash
# 1. Compare implementations
diff AI/ResourceManager.h AI/ClassAI/ResourceManager.h

# 2. Find all usages
grep -r "ResourceManager" --include="*.cpp" --include="*.h" | grep -v Binary

# 3. Determine relationship
# - Are they inheritance (base/derived)?
# - Are they completely different purposes?
# - Is one unused?

# 4. Make decision
# KEEP_BOTH: Document why both exist
# DELETE_ONE: Document which to keep
# MERGE: Combine into single file
```

**Document decisions in**: `CLEANUP_DECISIONS.md`

---

### Task 3: Review Backup/Legacy Files (2 days)

**BaselineRotationManagerFixed.cpp**:
```bash
# Compare with current
diff AI/ClassAI/BaselineRotationManager.cpp \
     AI/ClassAI/BaselineRotationManagerFixed.cpp

# Decision:
# - If Fixed is newer: Replace old, delete Fixed
# - If Fixed is backup: Delete Fixed
# - If unclear: Keep both temporarily, add comment
```

**InteractionManager_COMPLETE_FIX.cpp**:
```bash
# Same process
diff Interaction/Core/InteractionManager.cpp \
     Interaction/Core/InteractionManager_COMPLETE_FIX.cpp
```

**AuctionManager_UnitTest.cpp**:
```bash
# Move to proper test directory
mkdir -p tests/unit/Economy
mv Economy/AuctionManager_UnitTest.cpp \
   tests/unit/Economy/AuctionManagerTest.cpp
```

---

### Task 4: Audit Unused Managers (4 days)

For each potentially unused manager:

**Audit Process**:
1. Search for all references
2. Check if instantiated anywhere
3. Check if methods are called
4. Check git history (when last modified)
5. Make decision: KEEP, INTEGRATE, or DELETE

**AdvancedBehaviorManager Analysis**:
```bash
# Find references
grep -r "AdvancedBehaviorManager" --include="*.cpp" --include="*.h"

# Check instantiation
grep -r "std::make_unique<AdvancedBehaviorManager>" .
grep -r "new AdvancedBehaviorManager" .

# Check usage
grep -r "->Update\|->Initialize" | grep "AdvancedBehaviorManager"

# Check history
git log --oneline --all -- Advanced/AdvancedBehaviorManager.*
```

**Decisions**:
- If used → KEEP + document
- If partially used → INTEGRATE properly
- If unused → DELETE

---

### Task 5: Consolidate Structure (3 days)

**Create new unified structure**:
```bash
mkdir -p src/modules/Playerbot/Managers
mv Game/QuestManager.{h,cpp} Managers/
mv Social/TradeManager.{h,cpp} Managers/
mv Economy/AuctionManager.{h,cpp} Managers/
# etc.
```

**New folder structure**:
```
src/modules/Playerbot/
├── AI/
│   ├── BotAI.h/cpp
│   ├── BehaviorManager.h/cpp  ← Base class
│   └── ClassAI/
│
├── Managers/  ← NEW consolidated folder
│   ├── QuestManager.h/cpp
│   ├── TradeManager.h/cpp
│   ├── AuctionManager.h/cpp
│   ├── GatheringManager.h/cpp
│   ├── CombatManager.h/cpp
│   └── MovementManager.h/cpp
│
├── Strategy/
│   ├── IdleStrategy.h/cpp
│   ├── LeaderFollowBehavior.h/cpp
│   └── CombatMovementStrategy.h/cpp
│
└── [OLD FOLDERS - EMPTY OR DELETED]
```

---

### Task 6: Update Build System (1 day)

**CMakeLists.txt changes**:
```cmake
# Remove deleted files
# (QuestAutomation, TradeAutomation, etc.)

# Add new files
src/modules/Playerbot/AI/BehaviorManager.cpp
src/modules/Playerbot/Managers/CombatManager.cpp
src/modules/Playerbot/Strategy/CombatMovementStrategy.cpp

# Update paths for moved files
# Game/QuestManager.cpp → Managers/QuestManager.cpp
```

**Verify**:
```bash
cmake --build . --target worldserver
# Should compile without errors
```

---

### Task 7: Documentation (2 days)

**Create Documentation Files**:

1. **DELETED_FILES.md**:
   - List all deleted files
   - Reason for deletion
   - Replacement (if any)

2. **MIGRATION_GUIDE.md**:
   - Old API → New API mapping
   - Code examples showing migration
   - FAQ for common issues

3. **CLEANUP_DECISIONS.md**:
   - Document all keep/delete/merge decisions
   - Rationale for each decision
   - Stakeholder approvals

4. Update **ARCHITECTURE.md**:
   - New folder structure
   - Manager pattern explanation
   - Strategy pattern explanation

---

## Success Criteria

### Quantitative
- ✅ 3000-5000 lines of code removed
- ✅ Zero duplicate functionality
- ✅ All managers in `Managers/` folder
- ✅ Clean compilation (zero warnings)
- ✅ All existing tests pass

### Qualitative
- ✅ New developer can understand structure in <30 minutes
- ✅ No "dead" code remaining
- ✅ Clear ownership of all functionality
- ✅ Documentation matches reality

---

## Risk Mitigation

### Risk: Accidentally delete active code
**Mitigation**:
- Create backup branch before cleanup
- Test after each deletion
- Use git to track all changes

### Risk: Break compilation
**Mitigation**:
- Compile after each file deletion
- Use incremental approach
- Keep backups of working state

### Risk: Lose important context
**Mitigation**:
- Document ALL decisions in CLEANUP_DECISIONS.md
- Git commit messages explain WHY deleted
- Code review before final merge

---

## Deliverables

1. ✅ 8 deprecated files deleted
2. ✅ Duplicate investigation complete (decisions documented)
3. ✅ Backup files resolved
4. ✅ Unused managers audited
5. ✅ Unified folder structure
6. ✅ CMakeLists.txt updated
7. ✅ DELETED_FILES.md created
8. ✅ MIGRATION_GUIDE.md created
9. ✅ CLEANUP_DECISIONS.md created
10. ✅ All tests passing

---

## Next Phase

After completion, proceed to **Phase 2.8: Final Documentation**

---

**Last Updated**: 2025-01-13
**Next Review**: 2025-03-10
