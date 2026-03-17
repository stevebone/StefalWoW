# TrinityCore PlayerBot - 5000 Bot Testing Plan
**Date**: 2025-10-21
**Purpose**: Validate enterprise-grade spatial grid implementation, deadlock elimination, and automated world population system
**Target**: Support 5000+ concurrent bots without deadlocks

---

## ✅ PRE-REQUISITES

### 1. Build Latest Worldserver
**CRITICAL**: Must build worldserver.exe from commit `193ee13c9f` or later (includes DynamicObject/AreaTrigger danger detection)

**Option A: Using MSBuild (Recommended - avoids CMake reconfiguration)**
```powershell
cd C:\TrinityBots\TrinityCore\build
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
    TrinityCore.sln `
    /t:worldserver `
    /p:Configuration=RelWithDebInfo `
    /p:Platform=x64 `
    /m:4 `
    /v:minimal
```

**Option B: Using cmake (may require Boost reconfiguration)**
```powershell
cd C:\TrinityBots\TrinityCore\build
cmake --build . --config RelWithDebInfo --target worldserver -j 4
```

**Verification**:
```bash
# Check worldserver.exe timestamp (must be AFTER 2025-10-21 18:05)
ls -lh build/bin/RelWithDebInfo/worldserver.exe

# Verify it's from the correct commit
git log -1 --format="%H %ai"
# Should show: 193ee13c9f 2025-10-21 18:05:47 +0200
```

### 2. Database Setup
- Auth database configured and populated
- Characters database configured
- World database configured and populated with game data

### 3. Configuration Files
**worldserver.conf** - Core server settings
**playerbots.conf** - Bot configuration (see Configuration section below)

---

## 📋 TESTING PHASES

### **PHASE 1: Basic Functionality (100 Bots)** ⏱️ 30 minutes

**Objective**: Verify core systems work with small bot count

**Setup**:
```conf
# playerbots.conf
PlayerBot.MaxBots = 100
PlayerBot.Population.Enabled = 1
PlayerBot.Population.Alliance.Range1.Pct = 20    # Level 1-4: 20 bots
PlayerBot.Population.Alliance.Range2.Pct = 30    # Level 5-9: 30 bots
PlayerBot.Population.Alliance.Range17.Pct = 50   # Level 80: 50 bots
```

**Test Cases**:

1. **Server Startup**
   - [ ] Worldserver starts without errors
   - [ ] Spatial grid initializes successfully
   - [ ] ThreadPool starts with correct thread count
   - [ ] No warnings about missing Boost or dependencies

2. **Bot Spawning**
   - [ ] 100 bots spawn successfully
   - [ ] Level distribution matches configuration (20/30/50)
   - [ ] Bots have appropriate gear for their level
   - [ ] Bots spawn in correct zones (starter zones for 1-4, level-appropriate for 5+)

3. **Spatial Grid Population**
   - [ ] Grid populates with creatures, players, gameobjects
   - [ ] DynamicObjects populated (check logs for count)
   - [ ] AreaTriggers populated (check logs for count)
   - [ ] Query methods return results without errors

4. **Quest System**
   - [ ] Bots can pick up quests
   - [ ] Bots can complete quest objectives
   - [ ] Bots can turn in quests
   - [ ] No ObjectAccessor deadlocks in logs

5. **Gathering System**
   - [ ] Bots detect gathering nodes
   - [ ] Bots can gather herbs/ore
   - [ ] No false positives (non-gatherable objects)
   - [ ] No deadlocks during gathering

6. **Loot System**
   - [ ] Bots detect lootable corpses
   - [ ] Bots can loot creatures
   - [ ] Loot distribution works
   - [ ] No deadlocks during looting

7. **Combat Positioning**
   - [ ] Bots detect dangerous AoE effects
   - [ ] Bots move out of fire/poison zones
   - [ ] IsStandingInDanger() returns correct values
   - [ ] IsPositionSafe() prevents movement into hazards

**Success Criteria**:
- ✅ Zero crashes
- ✅ Zero deadlocks
- ✅ All 100 bots active and functional
- ✅ Futures 3-14 complete in <2 seconds
- ✅ Memory usage <10MB per bot (total <1GB)

---

### **PHASE 2: Performance Validation (1000 Bots)** ⏱️ 1 hour

**Objective**: Validate performance at medium scale

**Setup**:
```conf
# playerbots.conf
PlayerBot.MaxBots = 1000
PlayerBot.Population.Alliance.Range1.Pct = 5     # 50 bots at level 1-4
PlayerBot.Population.Alliance.Range2-16.Pct = 6  # 60 bots each (960 total)
PlayerBot.Population.Alliance.Range17.Pct = 13   # 130 bots at level 80
```

**Test Cases**:

1. **Spawning Performance**
   - [ ] 1000 bots spawn at ~25 bots/second (40 seconds total)
   - [ ] No memory leaks during spawn
   - [ ] ThreadPool handles load without blocking

2. **Spatial Grid Performance**
   - [ ] Grid update completes in <100ms (check logs)
   - [ ] Query latency <0.05ms average
   - [ ] Atomic buffer swap works correctly
   - [ ] No lock contention reported

3. **Combat AI Performance**
   - [ ] Bots engage combat smoothly
   - [ ] Target selection completes in <1ms
   - [ ] Rotation execution has no delays
   - [ ] No stuttering or lag

4. **Quest Throughput**
   - [ ] Multiple bots can quest simultaneously
   - [ ] No queueing delays for quest pickup/turnin
   - [ ] Snapshot-based queries scale linearly

5. **Memory Profiling**
   - [ ] Total memory <10GB (10MB per bot)
   - [ ] No memory growth over 10 minutes
   - [ ] Spatial grid memory stable

6. **CPU Profiling**
   - [ ] Total CPU <10% (0.01% per bot target is 10%)
   - [ ] No single-core saturation
   - [ ] ThreadPool load balanced

**Success Criteria**:
- ✅ Zero crashes after 30 minutes
- ✅ Zero deadlocks
- ✅ Futures 3-14 complete in <3 seconds
- ✅ Memory <10GB
- ✅ CPU <15%

---

### **PHASE 3: Stress Test (5000 Bots)** ⏱️ 2-3 hours

**Objective**: Verify deadlock elimination at maximum scale

**Setup**:
```conf
# playerbots.conf
PlayerBot.MaxBots = 5000
PlayerBot.Population.Alliance.Range1.Pct = 5     # 250 bots at level 1-4
PlayerBot.Population.Alliance.Range2-16.Pct = 6  # 300 bots each (4800 total)
PlayerBot.Population.Alliance.Range17.Pct = 13   # 650 bots at level 80

# ThreadPool settings
PlayerBot.ThreadPool.WorkerThreads = 8
PlayerBot.ThreadPool.TaskQueueSize = 10000
```

**Test Cases**:

1. **Deadlock Detection** (CRITICAL)
   - [ ] Futures 3-14 complete in <5 seconds
   - [ ] No "stuck future" warnings in logs
   - [ ] No ThreadPool hangs
   - [ ] Map::Update() completes without delays

2. **Spatial Grid Stress**
   - [ ] Grid populates 5000+ entities without errors
   - [ ] DynamicObject/AreaTrigger snapshots created correctly
   - [ ] Query performance degrades gracefully (linear, not exponential)
   - [ ] No buffer swap failures

3. **Combat System Stress**
   - [ ] 500+ bots in combat simultaneously
   - [ ] Target scanning completes without blocking
   - [ ] Danger detection (AoE avoidance) works at scale
   - [ ] No combat AI hangs

4. **Quest System Stress**
   - [ ] 1000+ bots questing simultaneously
   - [ ] Quest NPC queries scale correctly
   - [ ] No database connection exhaustion
   - [ ] Turn-in processing handles load

5. **Gathering System Stress**
   - [ ] 500+ bots gathering simultaneously
   - [ ] Node detection scales correctly
   - [ ] No false negatives (missed nodes)
   - [ ] Profession checks work correctly

6. **Memory Leak Detection**
   - [ ] Memory stable over 1 hour (no growth)
   - [ ] Total memory <50GB (10MB per bot)
   - [ ] No snapshot memory leaks
   - [ ] ThreadPool queue doesn't grow unbounded

7. **Long-Running Stability**
   - [ ] Run for 2 hours without crashes
   - [ ] No performance degradation over time
   - [ ] Logs show no recurring errors
   - [ ] All systems remain responsive

**Success Criteria**:
- ✅ **ZERO DEADLOCKS** (futures 3-14 complete <5s)
- ✅ Zero crashes for 2+ hours
- ✅ Memory <50GB and stable
- ✅ CPU <25%
- ✅ All bot systems functional

---

## 📊 METRICS TO COLLECT

### Server Performance
```sql
-- Database query counts
SELECT COUNT(*) FROM character_queststatus_daily;
SELECT COUNT(*) FROM character_inventory;
SELECT COUNT(*) FROM characters WHERE online = 1;
```

### Logs to Monitor
```bash
# Search for critical issues
grep -i "deadlock" Server.log
grep -i "future.*timeout" Server.log
grep -i "crash" Server.log
grep -i "error" Server.log | grep -v "known_safe_error"

# Performance metrics
grep "Spatial grid updated" Server.log | tail -20
grep "QueryNearby" Server.log | tail -50
grep "ThreadPool" Server.log | tail -20
```

### Windows Performance Counters
- Process: worldserver.exe
  - Private Bytes (memory)
  - % Processor Time (CPU)
  - Thread Count
  - Handle Count

### TrinityCore Console Commands
```
# Check bot count
.bot list

# Check spatial grid stats
.debug spatialindex

# Check thread pool status
.debug threadpool

# Monitor futures
.debug futures
```

---

## 🐛 KNOWN ISSUES & WORKAROUNDS

### Issue 1: Boost Not Found During Build
**Symptom**: CMake error "Could not find a package configuration file provided by Boost"
**Workaround**: Use MSBuild directly instead of cmake --build:
```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\TrinityCore.sln /t:worldserver /p:Configuration=RelWithDebInfo /p:Platform=x64 /m:4
```

### Issue 2: Git Bash Path Issues
**Symptom**: "No such file or directory" for Windows paths
**Workaround**: Use cmd.exe or PowerShell for Windows-specific commands

### Issue 3: Background Builds Still Running
**Symptom**: Multiple background bash processes consuming resources
**Workaround**: Kill them with:
```bash
# Check for background builds
ps aux | grep MSBuild

# Kill if needed
pkill -f MSBuild
```

---

## 📝 TEST RESULTS TEMPLATE

```markdown
# Test Results - [DATE]

## Environment
- **Commit**: 193ee13c9f
- **Build Time**: [timestamp]
- **OS**: Windows 11
- **CPU**: [processor]
- **RAM**: [total RAM]
- **Database**: MySQL 9.4

## Phase 1: 100 Bots
- ✅/❌ Server startup: [PASS/FAIL] - [notes]
- ✅/❌ Bot spawning: [PASS/FAIL] - [notes]
- ✅/❌ Spatial grid: [PASS/FAIL] - [notes]
- ✅/❌ Quest system: [PASS/FAIL] - [notes]
- ✅/❌ Gathering system: [PASS/FAIL] - [notes]
- ✅/❌ Loot system: [PASS/FAIL] - [notes]
- ✅/❌ Combat positioning: [PASS/FAIL] - [notes]

**Metrics**:
- Memory usage: [X GB]
- CPU usage: [X%]
- Futures 3-14 time: [X seconds]
- Errors found: [count]

## Phase 2: 1000 Bots
- ✅/❌ Spawning performance: [PASS/FAIL] - [notes]
- ✅/❌ Spatial grid performance: [PASS/FAIL] - [notes]
- ✅/❌ Combat AI performance: [PASS/FAIL] - [notes]
- ✅/❌ Quest throughput: [PASS/FAIL] - [notes]
- ✅/❌ Memory profiling: [PASS/FAIL] - [notes]
- ✅/❌ CPU profiling: [PASS/FAIL] - [notes]

**Metrics**:
- Memory usage: [X GB]
- CPU usage: [X%]
- Futures 3-14 time: [X seconds]
- Query latency avg: [X ms]
- Errors found: [count]

## Phase 3: 5000 Bots
- ✅/❌ Deadlock detection: [PASS/FAIL] - **CRITICAL**
- ✅/❌ Spatial grid stress: [PASS/FAIL] - [notes]
- ✅/❌ Combat system stress: [PASS/FAIL] - [notes]
- ✅/❌ Quest system stress: [PASS/FAIL] - [notes]
- ✅/❌ Gathering system stress: [PASS/FAIL] - [notes]
- ✅/❌ Memory leak detection: [PASS/FAIL] - [notes]
- ✅/❌ Long-running stability: [PASS/FAIL] - [notes]

**Metrics**:
- Memory usage: [X GB]
- CPU usage: [X%]
- Futures 3-14 time: [X seconds] - **MUST BE <5s**
- Uptime: [X hours]
- Deadlocks detected: [count] - **MUST BE 0**
- Errors found: [count]

## OVERALL VERDICT
✅ PRODUCTION READY
❌ ISSUES FOUND - [brief summary]

## Critical Issues
[List any blocking issues that prevent production deployment]

## Recommendations
[List any improvements or follow-up work needed]
```

---

## 🚀 NEXT STEPS AFTER TESTING

### If All Tests Pass ✅
1. Document final validation report
2. Tag release version (e.g., v1.0.0-playerbot-5k)
3. Prepare production deployment package
4. Create operational documentation
5. Begin quality improvement phase (gathering refinement, loot detection, etc.)

### If Issues Found ❌
1. Document all failures with reproduction steps
2. Prioritize blocking vs non-blocking issues
3. Create fix plan with estimates
4. Re-test after fixes applied
5. Iterate until clean pass

---

## ⚠️ CRITICAL REMINDERS

1. **Build from Latest Commit**: worldserver.exe MUST be from commit `193ee13c9f` or later
2. **Deadlock Test is Critical**: Futures 3-14 MUST complete in <5 seconds at 5000 bots
3. **Memory Leaks**: Watch for memory growth over time, especially in spatial grid
4. **Log Monitoring**: Check logs continuously for deadlock warnings
5. **Backup Database**: Before testing, backup character/world databases

---

## 📞 SUPPORT

If you encounter issues during testing:
1. Check logs in `Server.log` and `Error.log`
2. Review this document for known issues/workarounds
3. Consult `TROUBLESHOOTING_FAQ.md` in src/modules/Playerbot/docs/
4. Create detailed bug report with reproduction steps

---

**Good luck with testing!** 🎯
