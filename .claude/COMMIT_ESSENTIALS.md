# Essential Files to Commit to playerbot-dev

## 🎯 MUST COMMIT - Overnight Autonomous System (Core)

### Python Scripts (Essential)
```bash
git add .claude/scripts/overnight_autonomous_mode.py
git add .claude/scripts/autonomous_crash_monitor_with_approval.py
git add .claude/scripts/autonomous_crash_monitor_enhanced.py
git add .claude/scripts/claude_auto_processor.py
```

### Batch Files (Essential)
```bash
git add .claude/scripts/start_overnight_mode.bat
git add .claude/scripts/start_autonomous_system.bat
```

### Documentation (Essential)
```bash
git add .claude/OVERNIGHT_MODE_GUIDE.md
git add .claude/AUTONOMOUS_SYSTEM_COMPLETE.md
git add .claude/THREE_AUTONOMOUS_MODES.md
git add .claude/CORRECT_AUTONOMOUS_WORKFLOW.md
git add .claude/CORRECTED_IMPLEMENTATION_SUMMARY.md
git add .claude/TEST_RUN_CHECKLIST.md
```

### Quick References (Essential)
```bash
git add .claude/QUICK_START_CORRECT_WORKFLOW.md
git add .claude/QUICK_START_AUTONOMOUS.md
```

---

## 📋 OPTIONAL - Supporting Files

### Configuration Files (Optional but Useful)
```bash
git add .claude/hooks/periodic_crash_check.json
git add .claude/commands/analyze-crash-auto.md
```

### Crash Queue Directories (Optional - Create but Empty)
```bash
mkdir -p .claude/crash_analysis_queue/requests
mkdir -p .claude/crash_analysis_queue/responses
mkdir -p .claude/crash_analysis_queue/approvals
mkdir -p .claude/crash_analysis_queue/auto_process
mkdir -p .claude/crash_analysis_queue/overnight_deployed
# Add .gitkeep to track empty directories
touch .claude/crash_analysis_queue/requests/.gitkeep
touch .claude/crash_analysis_queue/responses/.gitkeep
touch .claude/crash_analysis_queue/approvals/.gitkeep
git add .claude/crash_analysis_queue/*/.gitkeep
```

---

## ❌ DO NOT COMMIT - Session/History Files

These are session-specific documentation and should NOT be committed:

```
.claude/BOT_LOGIN_REFACTORING_IMPLEMENTATION_SUMMARY.md
.claude/BOT_RESURRECTION_FIX_PLAN_2025-10-30.md
.claude/CRASH_ANALYSIS_BIH_COLLISION_2025-10-30.md
.claude/DEATH_RESURRECTION_ANALYSIS.md
.claude/GHOST_RESURRECTION_FIX_COMPLETE.md
.claude/IMPLEMENTATION_COMPLETE.md
.claude/SESSION_SUMMARY_*.md
.claude/WEEK_*_*.md
.claude/PHASE_*_*.md
.claude/SPELL_CPP_603_CRASH_FIX_COMPLETE.md
... (all other session summaries)
```

### Reason
These are historical notes from previous work sessions. They're useful for context but clutter the repository.

---

## ⚠️ CHECK CAREFULLY - Code Changes

### Modified File
```
M src/modules/Playerbot/AI/BotAI.cpp
```

**Action Required:**
```bash
# Check what changed
git diff src/modules/Playerbot/AI/BotAI.cpp

# If it's intentional and tested:
git add src/modules/Playerbot/AI/BotAI.cpp

# If it's accidental or unfinished:
git checkout src/modules/Playerbot/AI/BotAI.cpp
```

### New Code Files (Review Needed)
```
?? src/modules/Playerbot/AI/Combat/CombatStateManager.cpp
?? src/modules/Playerbot/AI/Combat/CombatStateManager.h
?? src/modules/Playerbot/Packets/PacketFilter.cpp
?? src/modules/Playerbot/Packets/PacketFilter.h
?? src/modules/Playerbot/Scripting/
```

**Action Required:**
If these are part of completed features:
```bash
git add src/modules/Playerbot/AI/Combat/CombatStateManager.*
git add src/modules/Playerbot/Packets/PacketFilter.*
git add src/modules/Playerbot/Scripting/
```

If they're work in progress:
```bash
# Leave uncommitted or stash
git stash save "WIP: CombatStateManager and PacketFilter"
```

---

## ❌ DO NOT COMMIT - Backup Files

```
?? src/modules/Playerbot/Lifecycle/DeathRecoveryManager.h.backup
?? src/modules/Playerbot/Session/BotSessionEnhanced.cpp.backup
?? src/server/game/Entities/Player/Player.cpp.backup_logineffect
?? src/server/game/Spells/Spell.cpp.backup_pre_spellevent_fix
```

**Action: Delete these**
```bash
rm src/modules/Playerbot/Lifecycle/DeathRecoveryManager.h.backup
rm src/modules/Playerbot/Session/BotSessionEnhanced.cpp.backup
rm src/server/game/Entities/Player/Player.cpp.backup_logineffect
rm src/server/game/Spells/Spell.cpp.backup_pre_spellevent_fix
```

---

## ❌ DO NOT COMMIT - Temporary/Script Files

```
?? apply_final_spellpacketbuilder_fixes.py
?? apply_spellevent_fix.py
?? apply_spellevent_fix_clean.py
?? fix_all_playerbot_config.sh
?? fix_spell_mod_taking.sed
?? fix_stateintegration.py
?? monitor_logs.sh
?? remove_spellmod_accesses.py
```

**Action: Move to a temp directory or delete**
```bash
mkdir -p temp_scripts
mv apply_*.py temp_scripts/
mv fix_*.sh temp_scripts/
mv fix_*.sed temp_scripts/
mv remove_*.py temp_scripts/
mv monitor_logs.sh temp_scripts/
```

---

## 🎯 RECOMMENDED COMMIT SEQUENCE

### Commit 1: Autonomous System Core
```bash
cd /c/TrinityBots/TrinityCore

# Add overnight mode
git add .claude/scripts/overnight_autonomous_mode.py
git add .claude/scripts/start_overnight_mode.bat
git add .claude/OVERNIGHT_MODE_GUIDE.md

# Add human approval mode
git add .claude/scripts/autonomous_crash_monitor_with_approval.py
git add .claude/CORRECT_AUTONOMOUS_WORKFLOW.md

# Add supporting scripts
git add .claude/scripts/autonomous_crash_monitor_enhanced.py
git add .claude/scripts/claude_auto_processor.py
git add .claude/scripts/start_autonomous_system.bat

# Commit
git commit -m "feat(automation): Add autonomous crash fixing system with overnight mode

Two operational modes:
1. Human Approval Mode - Reviews fix before deployment
2. Overnight Mode - Fully autonomous with separate git branch

Features:
- Zero human intervention for overnight mode
- Git branch safety (overnight-YYYYMMDD)
- Compilation verification before commit
- Auto-deployment to M:/Wplayerbot
- Continuous crash processing

Implements user-requested autonomous crash fixing system."
```

### Commit 2: Documentation
```bash
git add .claude/AUTONOMOUS_SYSTEM_COMPLETE.md
git add .claude/THREE_AUTONOMOUS_MODES.md
git add .claude/CORRECTED_IMPLEMENTATION_SUMMARY.md
git add .claude/TEST_RUN_CHECKLIST.md
git add .claude/QUICK_START_CORRECT_WORKFLOW.md
git add .claude/QUICK_START_AUTONOMOUS.md

git commit -m "docs(automation): Add comprehensive autonomous system documentation"
```

### Commit 3: Configuration
```bash
git add .claude/hooks/periodic_crash_check.json
git add .claude/commands/analyze-crash-auto.md

# Create queue directories
mkdir -p .claude/crash_analysis_queue/{requests,responses,approvals,auto_process,overnight_deployed}
touch .claude/crash_analysis_queue/requests/.gitkeep
touch .claude/crash_analysis_queue/responses/.gitkeep
git add .claude/crash_analysis_queue/*/.gitkeep

git commit -m "chore(automation): Add autonomous system configuration and directories"
```

### Commit 4: Code Changes (If Applicable)
```bash
# ONLY if BotAI.cpp change is intentional
git add src/modules/Playerbot/AI/BotAI.cpp
git commit -m "fix(playerbot): [describe the change]"

# ONLY if new files are complete features
git add src/modules/Playerbot/AI/Combat/CombatStateManager.*
git add src/modules/Playerbot/Packets/PacketFilter.*
git commit -m "feat(playerbot): Add CombatStateManager and PacketFilter"
```

### Push Everything
```bash
git push origin playerbot-dev
```

---

## ✅ MINIMAL COMMIT (If You Want to Test Quickly)

Just commit the essentials for overnight mode to work:

```bash
cd /c/TrinityBots/TrinityCore

# Essential overnight mode files
git add .claude/scripts/overnight_autonomous_mode.py
git add .claude/scripts/start_overnight_mode.bat
git add .claude/OVERNIGHT_MODE_GUIDE.md
git add .claude/TEST_RUN_CHECKLIST.md

# Commit
git commit -m "feat: Add overnight autonomous crash fixing mode"

# Push
git push origin playerbot-dev
```

**This is enough to test the overnight mode!**

---

## 📋 Summary

### MUST COMMIT (12 files)
1. `overnight_autonomous_mode.py` ⭐ Core
2. `autonomous_crash_monitor_with_approval.py`
3. `autonomous_crash_monitor_enhanced.py`
4. `claude_auto_processor.py`
5. `start_overnight_mode.bat` ⭐ Startup
6. `start_autonomous_system.bat`
7. `OVERNIGHT_MODE_GUIDE.md` ⭐ Docs
8. `AUTONOMOUS_SYSTEM_COMPLETE.md`
9. `THREE_AUTONOMOUS_MODES.md`
10. `CORRECT_AUTONOMOUS_WORKFLOW.md`
11. `CORRECTED_IMPLEMENTATION_SUMMARY.md`
12. `TEST_RUN_CHECKLIST.md` ⭐ Test Guide

### OPTIONAL (Add if you want)
- Quick reference docs
- Configuration files
- Empty queue directories

### DO NOT COMMIT
- Session summaries (50+ files)
- Backup files (.backup)
- Temporary scripts (apply_*.py, fix_*.sh)

---

**Recommendation: Commit the 12 essential files, push, then test overnight mode.**
