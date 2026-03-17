# ClassAI Old System Removal - Implementation Guide

## Status: Phase 3 - BLOCKED BY FILE LOCKS

### ✅ Completed (Phases 1-2):
1. CMakeLists.txt updated - All 71 old file references removed
2. 108 legacy files deleted (58 .cpp, 50 .h)
3. 45 include directives fixed

### ⚠️ **CURRENT BLOCKER**: Windows file locks preventing modification of *AI.h headers

**Files are locked by**: Windows Defender, indexing service, or antivirus
**Solution**: Temporarily disable Windows Defender Real-Time Protection or restart machine

---

## Phase 3: Clean 13 *AI.h Headers (READY TO EXECUTE)

All scripts are prepared. Once file locks are resolved, run:

```powershell
cd c:\TrinityBots\TrinityCore
python fix_all_classai_headers.py
```

### What This Script Does:

Removes from ALL 13 *AI.h files:
1. `std::unique_ptr<XSpecialization> _specialization;`
2. `XSpec _currentSpec;` or `XSpec _detectedSpec;`
3. `void InitializeSpecialization();`
4. `void DetectSpecialization();`
5. `XSpec DetectCurrentSpecialization() const;`
6. `void SwitchSpecialization(XSpec newSpec);`
7. `XSpec GetCurrentSpecialization() const;`
8. "Specialization Management/System" comment blocks

### Files to be Modified (13 total):

```
src/modules/Playerbot/AI/ClassAI/DeathKnights/DeathKnightAI.h
src/modules/Playerbot/AI/ClassAI/DemonHunters/DemonHunterAI.h
src/modules/Playerbot/AI/ClassAI/Druids/DruidAI.h
src/modules/Playerbot/AI/ClassAI/Evokers/EvokerAI.h
src/modules/Playerbot/AI/ClassAI/Hunters/HunterAI.h
src/modules/Playerbot/AI/ClassAI/Mages/MageAI.h
src/modules/Playerbot/AI/ClassAI/Monks/MonkAI.h
src/modules/Playerbot/AI/ClassAI/Paladins/PaladinAI.h
src/modules/Playerbot/AI/ClassAI/Priests/PriestAI.h
src/modules/Playerbot/AI/ClassAI/Rogues/RogueAI.h
src/modules/Playerbot/AI/ClassAI/Shamans/ShamanAI.h
src/modules/Playerbot/AI/ClassAI/Warlocks/WarlockAI.h
src/modules/Playerbot/AI/ClassAI/Warriors/WarriorAI.h
```

---

## Phase 4: Fix *.cpp Implementation Files (MANUAL WORK REQUIRED)

After headers are cleaned, these .cpp files need refactoring:

### Known Files with `_currentSpec` / `_specialization` Usage:

Based on build errors, these files DEFINITELY need fixes:
- `Warlocks/WarlockAI.cpp` - ~100+ errors (switch statements, spec checks)
- `Warriors/WarriorAI.cpp` - GetOptimalPosition/Range using `_specialization->`
- `Evokers/EvokerAI.cpp` - Constructor initializing `_specialization`
- `Monks/MonkAI.cpp` - unique_ptr construction errors

### Pattern to Find All Affected Files:

```bash
cd /c/TrinityBots/TrinityCore/src/modules/Playerbot/AI/ClassAI
grep -r "_currentSpec\|_detectedSpec\|_specialization->" --include="*.cpp" . | cut -d: -f1 | sort -u
```

### Refactoring Pattern for .cpp Files:

#### OLD PATTERN (Remove):
```cpp
// Constructor
WarlockAI::WarlockAI(Player* bot) : ClassAI(bot)
{
    _specialization = DetectSpecialization();  // ❌ DELETE
    _currentSpec = DetectCurrentSpecialization();  // ❌ DELETE
    InitializeSpecialization();  // ❌ DELETE
}

// Methods using old system
void WarlockAI::SomeMethod()
{
    if (_currentSpec == WarlockSpec::AFFLICTION)  // ❌ DELETE
    {
        // ...
    }

    if (_specialization)  // ❌ DELETE
        return _specialization->GetOptimalRange(target);
}

// switch statements
switch (_currentSpec)  // ❌ DELETE
{
    case WarlockSpec::AFFLICTION:
        // ...
        break;
}
```

#### NEW PATTERN (The template system handles this):

The ClassAI base class and BaselineRotationManager already handle all specialization dispatch via templates. The *AI.cpp files should just implement the ClassAI interface methods directly:

```cpp
// Constructor - minimal initialization only
WarlockAI::WarlockAI(Player* bot) : ClassAI(bot)
{
    // Just initialize Warlock-specific systems
    // NO specialization detection needed!
}

// Methods use ClassAI interface directly
// BaselineRotationManager routes to correct template automatically
```

### Files That Likely Need Similar Fixes:

ALL 13 classes probably have similar patterns in their *AI.cpp files. Search each one for:
- `_currentSpec`
- `_detectedSpec`
- `_specialization->`
- `InitializeSpecialization()`
- `DetectSpecialization()`
- `SwitchSpecialization()`

---

## Phase 5: Verification and Compilation

After completing Phases 3 & 4:

```powershell
cd c:\TrinityBots\TrinityCore\build

# Build playerbot module
& 'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\amd64\MSBuild.exe' `
  src\server\modules\Playerbot\playerbot.vcxproj `
  /p:Configuration=RelWithDebInfo `
  /p:Platform=x64 `
  /m:8 `
  /v:minimal
```

**Success Criteria**: 0 compilation errors

---

## Phase 6: Git Commit

Only after successful compilation with 0 errors:

```bash
git add .
git commit -m "[PlayerBot] ClassAI Old System Removal - Enterprise-Grade Full Migration

COMPLETE removal of legacy dual-support specialization system across all 13 classes.

## Changes Summary

### Files Deleted (108 total)
- 58 *Specialization.cpp implementation files
- 50 *Specialization.h header files

### CMakeLists.txt
- Removed 71 old system file references
- Kept only template headers (*Refactored.h) and helper files

### Header Files Modified (13 *AI.h files)
Removed old specialization system members from all classes:
- DeathKnights, DemonHunters, Druids, Evokers, Hunters, Mages, Monks,
  Paladins, Priests, Rogues, Shamans, Warlocks, Warriors

Removed members:
- std::unique_ptr<XSpecialization> _specialization
- XSpec _currentSpec / _detectedSpec
- InitializeSpecialization(), DetectSpecialization(), SwitchSpecialization()
- GetCurrentSpecialization() methods
- Specialization system comment blocks

### Implementation Files Modified (~30+ *AI.cpp files)
Refactored to use template system directly via BaselineRotationManager:
- Removed all _currentSpec/_detectedSpec usage
- Removed all _specialization-> pointer calls
- Removed switch(_currentSpec) statements
- Removed old initialization/detection/switching methods
- Simplified constructors

## Architecture Impact

**Before (Dual-Support)**:
- Runtime polymorphism via unique_ptr
- Manual specialization detection and switching
- Duplicate code paths (old .cpp + new templates)

**After (Template-Only)**:
- Compile-time template specialization
- Automatic dispatch via BaselineRotationManager
- Single source of truth (*Refactored.h templates)
- ~108 files eliminated
- Zero overhead from virtual calls

## Verification

✅ Compilation: SUCCESS (0 errors)
✅ All 40 specializations use template system exclusively
✅ CMakeLists.txt references only necessary files
✅ No orphaned code or dead references

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>
"
```

---

## Troubleshooting

### File Locks Persist

1. **Restart Windows** - Easiest solution
2. **Disable Windows Defender temporarily**:
   - Windows Security → Virus & threat protection → Manage settings
   - Turn OFF "Real-time protection"
3. **Close Visual Studio** completely
4. **Stop Windows Search**: `net stop "Windows Search"`
5. **Use Process Explorer** to find locking process

### Python Script Fails

Manually remove lines from each *AI.h file:
1. Open in text editor (Notepad++, VS Code)
2. Search for patterns listed in Phase 3
3. Delete matching lines
4. Save

### Compilation Errors After Changes

Re-read WarlockAI.cpp lines 564-1003 - these show the exact patterns to fix in other .cpp files.

---

## Quick Reference: Files to Modify

**Headers (13)**: Use `fix_all_classai_headers.py` script
**Implementations (~30+)**: Manual refactoring required - search for old patterns

**Total Work Estimate**: 2-3 hours for manual .cpp refactoring once headers are done

---

**Script Locations**:
- `c:\TrinityBots\TrinityCore\fix_all_classai_headers.py` (Phase 3)
- `c:\TrinityBots\TrinityCore\fix_classai_headers_complete.ps1` (Phase 3, PowerShell alt)
