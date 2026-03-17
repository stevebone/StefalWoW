# TrinityCore PlayerBot - Server Startup Checklist

## ✅ WHAT TO WATCH FOR DURING STARTUP

### 1. **Spatial Grid Initialization** (CRITICAL)
Look for these log messages:

```
[PLAYERBOT] SpatialGridManager initialized
[PLAYERBOT] DoubleBufferedSpatialGrid created for map [mapId]
[PLAYERBOT] Spatial grid initial population completed
```

**Expected**: No errors, grid initializes for each active map

---

### 2. **ThreadPool Startup** (CRITICAL)
Look for:

```
[PLAYERBOT] ThreadPool initialized with [N] worker threads
[PLAYERBOT] Work-stealing queue created
```

**Expected**: Worker thread count matches your CPU cores (typically 4-8)

---

### 3. **Automated Population System**
Look for:

```
[PLAYERBOT] BotLevelDistribution initialized
[PLAYERBOT] BotGearFactory cache built: [N] items
[PLAYERBOT] BotTalentManager loaded [N] talent loadouts
[PLAYERBOT] BotWorldPositioner zone cache ready
```

**Expected**: All caches build successfully without errors

---

### 4. **DynamicObject & AreaTrigger Support** (NEW)
Look for:

```
Spatial grid updated: [N] creatures, [N] players, [N] gameobjects, [N] dynamicObjects, [N] areaTriggers
```

**Expected**: Non-zero counts for dynamicObjects and areaTriggers when spell effects are active

---

### 5. **Database Connectivity**
Look for:

```
Connected to MySQL database
DatabasePool [auth] opened with [N] connections
DatabasePool [characters] opened with [N] connections
DatabasePool [world] opened with [N] connections
```

**Expected**: All 3 databases connected successfully

---

### 6. **Map Loading**
Look for:

```
Loading map [mapId]
Grid (X,Y) loaded for map [mapId]
```

**Expected**: Maps load without "failed to load" errors

---

## ⚠️ ERRORS TO WATCH FOR

### Critical Errors (Server Won't Start)

❌ **"Failed to initialize SpatialGridManager"**
- **Cause**: Spatial grid system failed
- **Action**: Check for missing dependencies, review error details

❌ **"ThreadPool initialization failed"**
- **Cause**: Thread creation failed
- **Action**: Check system resources, reduce worker thread count

❌ **"Database connection failed"**
- **Cause**: MySQL not running or wrong credentials
- **Action**: Verify MySQL service, check worldserver.conf

❌ **"SIGABRT" or "Segmentation fault"**
- **Cause**: Code crash during startup
- **Action**: Check for nullptr dereferences, review recent changes

---

### Warning Errors (Non-Critical)

⚠️ **"SpellInfo not found for spell [ID]"**
- **Cause**: Missing spell data in database
- **Impact**: Minor - that specific spell won't have proper detection
- **Action**: Update world database

⚠️ **"AreaTrigger has no spell"**
- **Cause**: Environmental AreaTrigger without spell (normal for some)
- **Impact**: None - these are marked as dangerous by default
- **Action**: None needed

⚠️ **"Gear cache: No items found for level [N]"**
- **Cause**: Missing item data for that level range
- **Impact**: Bots at that level won't get optimal gear
- **Action**: Check item_template table population

---

## 🎯 STARTUP SUCCESS CRITERIA

✅ **Server reaches "Ready to accept connections"**
✅ **No FATAL or ERROR messages**
✅ **Spatial grid initialized for all maps**
✅ **ThreadPool shows [N] worker threads**
✅ **DynamicObject/AreaTrigger counts > 0 (when effects active)**
✅ **Database pools all connected**

---

## 📊 FIRST TESTS AFTER STARTUP

### Test 1: Spawn a Single Bot
```
.bot add [name] [class] [race]
```

**Watch for**:
- Bot spawns successfully
- Spatial grid logs show bot in grid
- No deadlock warnings
- Bot has gear (if level 5+)

### Test 2: Check Spatial Grid
```
.debug spatialindex
```

**Expected output**:
- Grid shows populated cells
- Entity counts displayed
- No "grid not initialized" errors

### Test 3: Check ThreadPool
```
.debug threadpool
```

**Expected output**:
- Active threads: [N]
- Queued tasks: 0 (initially)
- Completed tasks: 0 (initially)

### Test 4: Check Futures (CRITICAL FOR DEADLOCK TEST)
```
.debug futures
```

**Expected output**:
- All futures show "completed" or "running"
- No futures stuck for >5 seconds
- Futures 3-14 specifically should complete quickly

---

## 🚦 NEXT STEPS AFTER SUCCESSFUL STARTUP

### Phase 1: Spawn 100 Bots

**Method 1: Using .bot add command**
```
.bot add [name] [class] [race]
```
Repeat 100 times with different names

**Method 2: Using automated population (if configured)**
```
.bot populate alliance 100
```

**Monitor**:
- Memory usage in Task Manager
- CPU usage
- Log for errors
- All 100 bots spawn successfully

**Success Criteria**:
- ✅ 100 bots spawned
- ✅ No crashes
- ✅ No deadlocks
- ✅ Memory < 2GB
- ✅ CPU < 5%

---

### Phase 2: Basic Functionality Tests

**Quest Test**:
1. Find a quest NPC
2. Watch bots pick up quests
3. Check logs for ObjectAccessor deadlocks (should be ZERO)

**Gathering Test**:
1. Spawn bots near gathering nodes
2. Watch bots gather
3. Check spatial grid queries in logs

**Combat Test**:
1. Spawn bots near hostile NPCs
2. Watch bots engage combat
3. Check danger detection (bots should avoid AoE)

**Loot Test**:
1. Watch bots kill mobs
2. Check if bots loot corpses
3. Verify no loot deadlocks

---

## 🔍 LOG FILES TO MONITOR

**Server.log** - Main server log
```bash
tail -f Server.log | grep -i "playerbot\|spatial\|threadpool"
```

**Error.log** - Error messages only
```bash
tail -f Error.log
```

**DBErrors.log** - Database errors
```bash
tail -f DBErrors.log
```

---

## ⏱️ TIMING BENCHMARKS

**Expected Startup Times**:
- Database connection: <2 seconds
- Map loading: 5-15 seconds per map
- Spatial grid init: <1 second per map
- ThreadPool init: <0.5 seconds
- Total startup: 30-60 seconds (depending on maps)

**If Startup Takes >2 Minutes**:
- Check database connectivity
- Check map file loading
- Check for hung initialization

---

## 🆘 TROUBLESHOOTING

### Server Hangs During Startup

**Symptom**: Server stops logging, appears frozen

**Common Causes**:
1. Database connection timeout
2. Map file loading stuck
3. Mutex deadlock in initialization
4. Infinite loop in startup code

**Actions**:
1. Check last log message before hang
2. Attach debugger and check stack trace
3. Kill and restart with more verbose logging
4. Check database connectivity

### Server Crashes Immediately

**Symptom**: Server starts then crashes with SIGABRT/SIGSEGV

**Common Causes**:
1. Nullptr dereference in initialization
2. Missing database tables
3. Corrupted configuration file
4. Memory corruption

**Actions**:
1. Review Error.log for crash location
2. Run with debugger to get stack trace
3. Verify all database tables exist
4. Check worldserver.conf syntax

### High Memory Usage at Startup

**Symptom**: >5GB memory used before bots spawn

**Common Causes**:
1. Memory leak in spatial grid initialization
2. Large gear cache
3. Map data loading issue
4. ThreadPool queue pre-allocation

**Actions**:
1. Profile memory usage
2. Check spatial grid cache size
3. Reduce gear cache size if needed
4. Check for memory leaks

---

## ✅ READY TO TEST!

Once server starts successfully:
1. ✅ No errors in logs
2. ✅ Spatial grid initialized
3. ✅ ThreadPool running
4. ✅ Databases connected

**You're ready to proceed with Phase 1 testing (100 bots)!**

Refer to `TESTING_PLAN_5000_BOTS.md` for detailed test procedures.

---

Good luck! 🚀
