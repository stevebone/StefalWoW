# Quick Start: Crash Automation System V2

**5-Minute Setup Guide** for the Enterprise Crash Analysis & Self-Healing System

**⚠️ IMPORTANT**: This guide now covers **Crash Automation V2** which is **PROJECT RULES COMPLIANT**. Always use V2 for this project.

---

## ⚡ Instant Setup (30 seconds)

```bash
# Navigate to scripts directory
cd c:\TrinityBots\TrinityCore\.claude\scripts

# Start real-time crash monitoring (recommended)
python crash_monitor.py
```

**Note**: `crash_monitor.py` works with both V1 and V2. For automated fixing, always use V2.

That's it! The system will now:
- Monitor Server.log and Playerbot.log in real-time
- Detect crashes immediately (<1 second)
- Analyze with full context (last 60 seconds of logs)
- Generate comprehensive reports
- Correlate errors and warnings

---

## 🎯 Common Usage Scenarios

### Scenario 1: "My server just crashed, what happened?"

```bash
# Analyze the most recent crash
python crash_analyzer.py --analyze M:/Wplayerbot/Crashes/worldserver_*.txt
```

**Output**: Detailed crash report with:
- Root cause hypothesis
- Fix suggestions
- Call stack analysis
- Similar crashes
- Severity assessment

### Scenario 2: "I want to automatically fix crashes while I sleep"

```bash
# Start automated crash loop (semi-automated with manual review)
# ⚠️ IMPORTANT: Use V2 (PROJECT RULES COMPLIANT)
python crash_auto_fix_v2.py --max-iterations 10
```

**Workflow**:
1. Runs server → detects crash
2. Analyzes crash with logs
3. Generates automated fix
4. **PROMPTS YOU** to review and apply fix
5. You apply fix → press Enter
6. System recompiles and repeats
7. Stops when stable or max iterations reached

### Scenario 3: "I need continuous crash monitoring on my dev server"

```bash
# Start background monitoring
nohup python crash_monitor.py > crash_monitor.log 2>&1 &

# Check status
tail -f crash_monitor.log

# Stop monitoring
pkill -f crash_monitor.py
```

### Scenario 4: "I'm brave and want full automation" ⚠️

```bash
# DANGER ZONE: Fully automated (applies fixes automatically)
# ⚠️ IMPORTANT: Use V2 (PROJECT RULES COMPLIANT)
python crash_auto_fix_v2.py --auto-run --max-iterations 5
```

**⚠️ WARNING**: This mode:
- Automatically applies generated fixes
- Automatically recompiles code
- Repeats until stable or max iterations
- **Use with caution** - review fixes first!

---

## 📋 What Each Script Does

### crash_monitor.py (Real-Time Monitoring)

**Best For**: Production servers, development testing, continuous monitoring

**What It Does**:
- Tails Server.log and Playerbot.log in real-time
- Detects crashes instantly
- Correlates crash with last 60 seconds of logs
- Shows errors before crash
- Identifies escalating warnings
- Finds last bot action before crash
- Generates comprehensive JSON + Markdown reports

**When To Use**: Always! Run this in background during development/testing.

### crash_analyzer.py (Single Crash Analysis)

**Best For**: Analyzing existing crash dumps, understanding specific crashes

**What It Does**:
- Parses .txt crash dump files
- Matches against 7 known crash patterns
- Generates root cause hypothesis
- Suggests automated fixes
- Finds similar historical crashes
- Builds crash pattern database

**When To Use**: When you have a crash dump and need deep analysis.

### crash_auto_fix_v2.py (Automated Crash Loop - PROJECT RULES COMPLIANT)

**⚠️ IMPORTANT**: Always use V2 for this project. V1 is deprecated.

**Best For**: Iterative debugging, overnight testing, CI/CD integration

**What It Does**:
- Runs server → waits for crash (or timeout)
- Analyzes crash with full context
- Generates **PROJECT RULES COMPLIANT** automated fix
- Follows file modification hierarchy (module-first)
- Optionally applies fix
- Optionally recompiles
- Repeats until stable
- Tracks crash history
- Detects crash loops (same crash 3+ times)

**When To Use**: When you want automated crash fixing with supervision.

**Key Difference from V1**: All generated fixes follow TrinityCore Playerbot project rules and prefer module-only solutions.

---

## 🔧 Configuration

All scripts work out-of-the-box with default paths:

| Path | Default | Description |
|------|---------|-------------|
| Trinity Root | `c:/TrinityBots/TrinityCore` | TrinityCore repository |
| Server Exe | `M:/Wplayerbot/worldserver.exe` | Server executable |
| Server Log | `M:/Wplayerbot/Logs/Server.log` | Server log file |
| Playerbot Log | `M:/Wplayerbot/Logs/Playerbot.log` | Playerbot log file |
| Crashes Dir | `M:/Wplayerbot/Crashes` | Crash dump directory |

### Override Defaults

```bash
# Custom paths
python crash_monitor.py \
    --server-log /path/to/Server.log \
    --playerbot-log /path/to/Playerbot.log \
    --crashes /path/to/crashes \
    --trinity-root /path/to/TrinityCore
```

---

## 📊 Output Files

All generated files are saved to `.claude/` directory:

```
.claude/
├── crash_reports/               # Comprehensive crash reports
│   ├── crash_context_*.json     # JSON format (machine-readable)
│   └── crash_report_*.md        # Markdown format (human-readable)
│
├── automated_fixes/             # Generated fix patches
│   └── fix_*.cpp                # C++ patches with instructions
│
├── crash_database.json          # Historical crash database
│
└── CRASH_LOOP_REPORT_*.md       # Reports for repeated crashes
```

### Example Report Structure

**crash_report_a3f8b2c1_20251031_011537.md**:
```markdown
# Crash Report - 2025-10-31 01:15:37

## Crash Details
- Crash ID: a3f8b2c1
- Location: Unit.cpp:10863
- Function: Unit::DealDamage
- Root Cause: Null pointer dereference

## Errors Before Crash (3)
...

## Escalating Warnings
...

## Server.log Context (Last 30 entries)
...

## Playerbot.log Context (Last 30 entries)
...

## Call Stack
...
```

---

## 🚨 Known Crash Patterns

The system recognizes 7 common crash patterns:

| Pattern | Location | Severity | Auto-Fix Available |
|---------|----------|----------|-------------------|
| BIH Collision | `BoundingIntervalHierarchy.cpp` | CRITICAL | ⚠️ Core bug - needs TC update |
| Spell.cpp:603 | `Spell.cpp:603` | CRITICAL | ✅ Teleport delay fix |
| Map.cpp:686 | `Map.cpp:686` | HIGH | ✅ Session state check |
| Unit.cpp:10863 | `Unit.cpp:10863` | HIGH | ✅ SpellMod removal |
| Socket Null | `Socket.cpp` | HIGH | ✅ Null check injection |
| Deadlock | Various | CRITICAL | ✅ Lock refactoring |
| Ghost Aura | `Spell.cpp` | MEDIUM | ✅ Duplicate removal |

**New crashes** are analyzed and added to the pattern database automatically.

---

## 💡 Pro Tips

### Tip 1: Always Run Monitoring During Testing

```bash
# Terminal 1: Start monitoring
python crash_monitor.py

# Terminal 2: Run your tests
cd M:/Wplayerbot
./worldserver.exe
```

**Benefit**: Instant crash analysis with full context when server crashes.

### Tip 2: Review Generated Fixes Before Applying

```bash
# Generated fixes are in .claude/automated_fixes/
cat .claude/automated_fixes/fix_*.cpp

# Review the fix, then apply manually to source code
```

**Benefit**: Understand what's being changed and verify correctness.

### Tip 3: Use Single-Iteration Mode for New Crashes

```bash
# Safe mode - one iteration at a time
python crash_auto_fix.py --single-iteration
```

**Benefit**: You control each step and can stop if needed.

### Tip 4: Check Crash Database for Patterns

```bash
# View historical crash data
cat .claude/crash_database.json | jq '.crashes | length'

# Find most common crashes
cat .claude/crash_database.json | jq '.crashes | group_by(.crash_location) | sort_by(length) | reverse | .[0:5]'
```

**Benefit**: Identify recurring issues that need permanent fixes.

### Tip 5: Integrate with Git Workflow

```bash
# Create branch for crash fixes
git checkout -b fix/crash-a3f8b2c1

# Apply generated fix
cat .claude/automated_fixes/fix_a3f8b2c1_*.cpp
# ... apply to source code ...

# Commit with crash context
git add .
git commit -m "fix: Resolve Unit.cpp:10863 null pointer crash

Root cause: Null object access in DealDamage
Fix: Added null check before object use

Crash ID: a3f8b2c1
See: .claude/crash_reports/crash_report_a3f8b2c1_*.md"
```

**Benefit**: Track crash fixes with full context in git history.

---

## ❓ Troubleshooting

### "No crash dump found"

**Check**: TrinityCore crash dump settings
```bash
# worldserver.conf
CrashDump.Enabled = 1
CrashDump.DetailLevel = 2
```

### "Log file not found"

**Check**: Log paths are correct
```bash
# Verify files exist
ls -la M:/Wplayerbot/Logs/Server.log
ls -la M:/Wplayerbot/Logs/Playerbot.log
```

### "Permission denied"

**Fix**: Run Git Bash as Administrator or adjust permissions
```bash
chmod +x .claude/scripts/*.py
```

### "Module not found"

**Fix**: Ensure Python 3.8+ and scripts are in correct directory
```bash
python --version  # Should be 3.8+
pwd  # Should be .claude/scripts/
```

---

## 🎯 Next Steps

After setting up crash automation:

1. **Read Full Documentation**: [ENTERPRISE_CRASH_AUTOMATION_SYSTEM.md](./ENTERPRISE_CRASH_AUTOMATION_SYSTEM.md)

2. **Integrate with Testing**: See [COMPREHENSIVE_TEST_COVERAGE_ANALYSIS.md](./src/modules/Playerbot/Tests/COMPREHENSIVE_TEST_COVERAGE_ANALYSIS.md)

3. **Add Custom Patterns**: Extend `crash_analyzer.py` with your own patterns

4. **CI/CD Integration**: Add to GitHub Actions workflow

5. **Production Monitoring**: Deploy on production servers with alerting

---

## 📞 Need Help?

1. Check full documentation: `ENTERPRISE_CRASH_AUTOMATION_SYSTEM.md`
2. Review generated crash reports in `.claude/crash_reports/`
3. Check crash database: `.claude/crash_database.json`
4. Review this guide again carefully
5. Create GitHub issue with crash context if problem persists

---

**Quick Start Version**: 1.0.0
**Last Updated**: 2025-10-31
**Estimated Setup Time**: 30 seconds
**Estimated Learning Time**: 5 minutes

**Status**: ✅ Ready to Use

---

## 🚀 One-Command Quick Start

```bash
# Copy and paste this to get started immediately:
cd c:\TrinityBots\TrinityCore\.claude\scripts && python crash_monitor.py
```

**That's it!** The system is now monitoring for crashes with full automation.

**For automated fixing with V2 (PROJECT RULES COMPLIANT)**:
```bash
cd c:\TrinityBots\TrinityCore\.claude\scripts && python crash_auto_fix_v2.py
```

---

## 📖 Additional Documentation

- **V2 Complete Guide**: [CRASH_AUTOMATION_V2_PROJECT_RULES_COMPLIANT.md](./CRASH_AUTOMATION_V2_PROJECT_RULES_COMPLIANT.md)
- **Full System Documentation**: [ENTERPRISE_CRASH_AUTOMATION_SYSTEM.md](./ENTERPRISE_CRASH_AUTOMATION_SYSTEM.md)
- **V1 Implementation (DEPRECATED)**: [CRASH_AUTOMATION_IMPLEMENTATION_COMPLETE.md](./CRASH_AUTOMATION_IMPLEMENTATION_COMPLETE.md)

---

*For advanced features, custom patterns, and CI/CD integration, see the full documentation.*
