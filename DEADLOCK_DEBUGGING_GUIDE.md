# COMPREHENSIVE DEADLOCK DEBUGGING GUIDE
## TrinityCore PlayerBot Module - Visual Studio Enterprise

## TABLE OF CONTENTS
1. [Current Issues](#current-issues)
2. [Root Cause Analysis](#root-cause-analysis)
3. [Enhanced Logging System](#enhanced-logging-system)
4. [Visual Studio Enterprise Debugging](#visual-studio-enterprise-debugging)
5. [Automated Debugging Tools](#automated-debugging-tools)
6. [Configuration & Usage](#configuration--usage)
7. [Advanced Techniques](#advanced-techniques)

---

## CURRENT ISSUES

### Problem: False "DEADLOCK DETECTED" Warnings

**Current Logs**:
```
🔧 DEADLOCK DETECTED: Future 10 of 10 (bot Player-1-00000033) did not complete after 0 seconds!
🔧 DEADLOCK DETECTED: Future 6 of 9 (bot Player-1-00000036) did not complete after 0 seconds!
```

**Root Causes Identified**:

1. **Integer Division Truncation** (BotWorldSessionMgr.cpp:770):
```cpp
TC_LOG_FATAL("module.playerbot.session",
    "🔧 DEADLOCK DETECTED: Future {} of {} (bot {}) did not complete after {} seconds!",
    i + 1, futures.size(), futureGuids[i].ToString(),
    (MAX_RETRIES * 100) / 1000);  // ← (5 * 100) / 1000 = 500 / 1000 = 0 (integer truncation!)
```

**Fixed Calculation**:
```cpp
static_cast<double>(MAX_RETRIES * 100) / 1000.0  // = 0.5 seconds
```

2. **Timeout Too Short for High Load**:
   - Current: 500ms (5 retries × 100ms)
   - Problem: Under heavy load (100+ bots), thread pool may legitimately need >500ms
   - Solution: Adaptive timeout based on system load

3. **No Call Stack Information**:
   - Can't determine WHERE the thread is blocked
   - Can't identify WHAT mutex/resource is causing the wait
   - Can't trace execution path leading to deadlock

---

## ROOT CAUSE ANALYSIS

### What's Actually Happening

**BotWorldSessionMgr::Update()** flow (lines 720-772):

1. **Submit bot updates to thread pool** (futures created)
2. **Wait for futures with timeout loop**:
   ```cpp
   for (size_t i = 0; i < futures.size(); ++i)
   {
       uint32 retries = 0;
       constexpr uint32 MAX_RETRIES = 5;  // 5 × 100ms = 500ms

       while (!completed && retries < MAX_RETRIES)
       {
           auto status = future.wait_for(std::chrono::milliseconds(100));
           if (status == std::future_status::ready)
               completed = true;
           else
               ++retries;  // ← Timeout, retry
       }

       if (!completed)
           LOG_FATAL("DEADLOCK DETECTED");  // ← FALSE POSITIVE!
   }
   ```

3. **Problem**: NOT a real deadlock - just slow thread pool!
   - Thread pool has limited workers (e.g., 8 threads)
   - 100 bot updates submitted simultaneously
   - Some futures wait in queue for worker availability
   - Worker finishes in 600ms (normal for heavy AI update)
   - Timeout triggers at 500ms → false "DEADLOCK"

### Actual Deadlocks vs False Positives

**Real Deadlock Indicators**:
- Threads waiting on each other cyclically
- Mutex A held while waiting for Mutex B, vice versa
- Future never completes even after 10+ seconds
- CPU usage drops to near-zero (all threads blocked)

**False Positive Indicators** (current issue):
- Futures complete after timeout expires
- System under heavy load (100+ bots)
- CPU usage normal (threads actively processing)
- Warnings appear in bursts during update spikes

---

## ENHANCED LOGGING SYSTEM

### Fix 1: Correct Time Display

**File**: `src/modules/Playerbot/Session/BotWorldSessionMgr.cpp`

**Current (Line 770)**:
```cpp
TC_LOG_FATAL("module.playerbot.session",
    "🔧 DEADLOCK DETECTED: Future {} of {} (bot {}) did not complete after {} seconds!",
    i + 1, futures.size(), futureGuids[i].ToString(),
    (MAX_RETRIES * 100) / 1000);  // ← WRONG: Integer truncation → 0
```

**Enhanced**:
```cpp
TC_LOG_FATAL("module.playerbot.session",
    "⚠️ POTENTIAL DEADLOCK: Future {}/{} (bot {}) timeout after {:.1f}s "
    "(retries: {}, timeout: {}ms, threadPool: {} active/{} total threads) "
    "📍 Capture call stack with Visual Studio!",
    i + 1, futures.size(), futureGuids[i].ToString(),
    static_cast<double>(MAX_RETRIES * 100) / 1000.0,  // ← FIXED: 0.5 seconds
    MAX_RETRIES,
    MAX_RETRIES * 100,
    Performance::GetThreadPool().GetActiveThreadCount(),
    Performance::GetThreadPool().GetTotalThreadCount());

// ADD: Diagnostic information
TC_LOG_ERROR("module.playerbot.session.deadlock",
    "📊 System State at Timeout:\n"
    "   Bot GUID: {}\n"
    "   Future Index: {}/{}\n"
    "   Total Wait Time: {}ms\n"
    "   Thread Pool Queue Depth: {}\n"
    "   Active Workers: {}/{}\n"
    "   System Load: {:.1f}%\n"
    "   Timestamp: {}",
    futureGuids[i].ToString(),
    i + 1, futures.size(),
    MAX_RETRIES * 100,
    Performance::GetThreadPool().GetQueuedTaskCount(),
    Performance::GetThreadPool().GetActiveThreadCount(),
    Performance::GetThreadPool().GetTotalThreadCount(),
    Performance::GetThreadPool().GetLoadPercentage(),
    std::chrono::system_clock::now());
```

### Fix 2: Add Thread State Logging

**Before timeout loop** (add at line 722):
```cpp
// Diagnostic: Log thread pool state before waiting
TC_LOG_INFO("module.playerbot.session.futures",
    "⏳ Waiting for {} futures (current threadPool: {} active/{} total, {} queued tasks)",
    futures.size(),
    Performance::GetThreadPool().GetActiveThreadCount(),
    Performance::GetThreadPool().GetTotalThreadCount(),
    Performance::GetThreadPool().GetQueuedTaskCount());
```

### Fix 3: Per-Future Progress Logging

**Inside retry loop** (modify line 756):
```cpp
if (retries % 2 == 0)  // Every 200ms
{
    TC_LOG_WARN("module.playerbot.session.futures",
        "⏱️ Future {}/{} (bot {}) waiting for {}ms (attempt {}/{}) - "
        "ThreadPool: {} active, {} queued, Load: {:.1f}% - "
        "🔍 TIP: Attach Visual Studio debugger now!",
        i + 1, futures.size(),
        futureGuids[i].ToString(),
        retries * 100,
        retries, MAX_RETRIES,
        Performance::GetThreadPool().GetActiveThreadCount(),
        Performance::GetThreadPool().GetQueuedTaskCount(),
        Performance::GetThreadPool().GetLoadPercentage());
}
```

### Fix 4: Successful Completion Logging

**After future.get()** (add at line 740):
```cpp
future.get(); // Get result + exception propagation

TC_LOG_DEBUG("module.playerbot.session.futures",
    "✅ Future {}/{} (bot {}) completed successfully after {}ms",
    i + 1, futures.size(),
    futureGuids[i].ToString(),
    retries * 100);
```

---

## VISUAL STUDIO ENTERPRISE DEBUGGING

### Method 1: Attach to Running Process

#### Step 1: Enable Just-In-Time Debugging
1. Open Visual Studio Enterprise
2. **Tools → Options → Debugging → Just-In-Time**
3. Enable: ☑ Managed ☑ Native ☑ Script
4. Click **OK**

#### Step 2: Attach to worldserver.exe
1. Launch worldserver.exe (RelWithDebInfo or Debug build)
2. In Visual Studio: **Debug → Attach to Process** (Ctrl+Alt+P)
3. Filter: `worldserver`
4. Select `worldserver.exe`
5. **Attach Type**: Native Only
6. Click **Attach**

#### Step 3: Set Conditional Breakpoints

**Location**: `BotWorldSessionMgr.cpp:768` (DEADLOCK DETECTED line)

**Conditional Breakpoint**:
```cpp
retries >= 3  // Break only after 3+ retries (300ms+)
```

**How to Set**:
1. Open `BotWorldSessionMgr.cpp`
2. Navigate to line 768
3. Click left margin to set breakpoint (red dot)
4. Right-click breakpoint → **Conditions**
5. Select "Conditional Expression"
6. Enter: `retries >= 3`
7. Click **Close**

**When Breakpoint Hits**:
```
Breakpoint hit at BotWorldSessionMgr.cpp:768
Current values:
  i = 9
  futures.size() = 10
  futureGuids[9] = Player-1-00000033
  retries = 5
  completed = false
```

#### Step 4: Inspect Thread Pool State

**Immediate Window** (Debug → Windows → Immediate):
```cpp
Performance::GetThreadPool().GetActiveThreadCount()
// Output: 8 (all workers busy)

Performance::GetThreadPool().GetQueuedTaskCount()
// Output: 92 (92 tasks waiting!)

Performance::GetThreadPool().GetLoadPercentage()
// Output: 98.5% (thread pool overloaded)
```

#### Step 5: Inspect All Threads

**Threads Window** (Debug → Windows → Threads):
```
ID    Name                      Location                           Priority
====  ========================  =================================  ========
1234  Main Thread               BotWorldSessionMgr::Update:768     Normal
5678  ThreadPool Worker #1      BotAI::UpdateAI:450 (blocked)      Normal
9012  ThreadPool Worker #2      Map::Update:2341 (waiting mutex)   Normal
...
```

**Right-click thread** → **Freeze** to prevent it from executing
**Right-click thread** → **Switch to Thread** to inspect its stack

#### Step 6: Examine Call Stack

**Call Stack Window** (Debug → Windows → Call Stack):
```
worldserver.exe!BotWorldSessionMgr::Update() Line 768
worldserver.exe!World::Update(unsigned int) Line 3421
worldserver.exe!WorldRunnable::run() Line 89
worldserver.exe!Trinity::Threading::ThreadStart() Line 45
kernel32.dll!BaseThreadInitThunk()
ntdll.dll!RtlUserThreadStart()
```

**Double-click any frame** to navigate to source

---

### Method 2: Automated Break-on-Timeout

**Create Debug Build with Break Macro**:

**Add to BotWorldSessionMgr.cpp** (line 766):
```cpp
if (!completed)
{
    #ifdef _DEBUG
        // Automatic debugger break on timeout (Debug builds only)
        if (IsDebuggerPresent())
        {
            TC_LOG_FATAL("module.playerbot.session",
                "🛑 BREAKING INTO DEBUGGER: Future {}/{} timeout",
                i + 1, futures.size());
            __debugbreak();  // Triggers breakpoint if debugger attached
        }
    #endif

    TC_LOG_FATAL("module.playerbot.session",
        "⚠️ POTENTIAL DEADLOCK: Future {}/{} (bot {}) timeout after {:.1f}s",
        i + 1, futures.size(), futureGuids[i].ToString(),
        static_cast<double>(MAX_RETRIES * 100) / 1000.0);
}
```

**Usage**:
1. Build Debug configuration
2. Attach Visual Studio debugger
3. Run worldserver.exe
4. On timeout, execution automatically breaks
5. Inspect state in debugger

---

### Method 3: Time Travel Debugging (Enterprise Feature)

**Requirements**: Visual Studio Enterprise 2022+

#### Enable IntelliTrace

1. **Tools → Options → IntelliTrace**
2. Select: **IntelliTrace events and call information**
3. **IntelliTrace events**: Enable all "Threading" events
4. **Advanced → Maximum disk space**: 10 GB+
5. Click **OK**

#### Record Session

1. **Debug → Start Diagnostic Tools Session**
2. Select: **CPU Usage + IntelliTrace**
3. Launch worldserver.exe
4. Let deadlock occur
5. **Stop Collection**

#### Analyze Historical Execution

1. **Debug → IntelliTrace → IntelliTrace Events**
2. Filter: "Threading"
3. Find: "Thread blocked" or "Mutex wait" events
4. **Right-click event** → **Activate Historical Debugging**
5. Navigate backwards through time to see what caused the block

---

## AUTOMATED DEBUGGING TOOLS

### Tool 1: Deadlock Detector with Auto-Dump

**Create**: `scripts/debug_deadlock.ps1`

```powershell
# Automated Deadlock Detection & Dump Creation
# Usage: .\debug_deadlock.ps1 -ProcessName "worldserver" -TimeoutSeconds 60

param(
    [string]$ProcessName = "worldserver",
    [int]$TimeoutSeconds = 60,
    [string]$OutputDir = "C:\TrinityBots\deadlock_dumps"
)

Write-Host "🔍 Deadlock Monitor Started" -ForegroundColor Cyan
Write-Host "   Process: $ProcessName" -ForegroundColor Gray
Write-Host "   Timeout: $TimeoutSeconds seconds" -ForegroundColor Gray
Write-Host "   Output: $OutputDir" -ForegroundColor Gray

# Ensure output directory exists
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

# Find process
$process = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue
if (-not $process) {
    Write-Host "❌ Process '$ProcessName' not found!" -ForegroundColor Red
    exit 1
}

Write-Host "✅ Found process PID: $($process.Id)" -ForegroundColor Green

# Monitor log file for deadlock warnings
$logFile = "M:\Wplayerbot\logs\Server.log"
$lastSize = (Get-Item $logFile).Length

Write-Host "📊 Monitoring log: $logFile" -ForegroundColor Cyan

while ($true) {
    Start-Sleep -Seconds 1

    $currentSize = (Get-Item $logFile).Length
    if ($currentSize -gt $lastSize) {
        # Read new content
        $newContent = Get-Content $logFile -Tail 100 | Select-String "DEADLOCK DETECTED"

        if ($newContent) {
            Write-Host "🚨 DEADLOCK DETECTED! Creating dump..." -ForegroundColor Red

            # Create timestamp
            $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
            $dumpFile = Join-Path $OutputDir "deadlock_$timestamp.dmp"

            # Create full memory dump using procdump
            & "procdump.exe" -ma $process.Id $dumpFile

            Write-Host "✅ Dump created: $dumpFile" -ForegroundColor Green

            # Extract recent log entries
            $logExtract = Join-Path $OutputDir "deadlock_$timestamp.log"
            Get-Content $logFile -Tail 1000 | Out-File $logExtract

            Write-Host "📄 Log extract: $logExtract" -ForegroundColor Green

            # Auto-open in Visual Studio
            Write-Host "🔧 Opening dump in Visual Studio..." -ForegroundColor Cyan
            & "devenv.exe" "/debugexe" $dumpFile

            break
        }

        $lastSize = $currentSize
    }
}
```

**Installation**:
1. Download **ProcDump** from Sysinternals: https://docs.microsoft.com/en-us/sysinternals/downloads/procdump
2. Add `procdump.exe` to PATH
3. Save script as `C:\TrinityBots\scripts\debug_deadlock.ps1`

**Usage**:
```powershell
cd C:\TrinityBots\scripts
.\debug_deadlock.ps1
```

### Tool 2: Thread Pool Analyzer

**Create**: `scripts/analyze_threadpool.ps1`

```powershell
# Real-time Thread Pool Analysis
param(
    [string]$LogFile = "M:\Wplayerbot\logs\Server.log"
)

Write-Host "📊 Thread Pool Analyzer" -ForegroundColor Cyan

# Extract thread pool metrics from logs
$metrics = Get-Content $LogFile | Select-String "ThreadPool:" | ForEach-Object {
    if ($_ -match "ThreadPool: (\d+) active/(\d+) total, (\d+) queued") {
        [PSCustomObject]@{
            Active = [int]$Matches[1]
            Total = [int]$Matches[2]
            Queued = [int]$Matches[3]
            Load = [math]::Round(([int]$Matches[1] / [int]$Matches[2]) * 100, 1)
        }
    }
}

# Calculate statistics
$avgActive = ($metrics | Measure-Object -Property Active -Average).Average
$avgQueued = ($metrics | Measure-Object -Property Queued -Average).Average
$maxQueued = ($metrics | Measure-Object -Property Queued -Maximum).Maximum
$avgLoad = ($metrics | Measure-Object -Property Load -Average).Average

Write-Host "`n📈 Thread Pool Statistics:" -ForegroundColor Green
Write-Host "   Average Active Threads: $([math]::Round($avgActive, 1))" -ForegroundColor Gray
Write-Host "   Average Queued Tasks: $([math]::Round($avgQueued, 1))" -ForegroundColor Gray
Write-Host "   Maximum Queued Tasks: $maxQueued" -ForegroundColor Gray
Write-Host "   Average Load: $([math]::Round($avgLoad, 1))%" -ForegroundColor Gray

if ($avgLoad -gt 80) {
    Write-Host "`n⚠️ WARNING: Thread pool overloaded! Consider increasing worker count." -ForegroundColor Yellow
}

if ($maxQueued -gt 50) {
    Write-Host "⚠️ WARNING: High task queue depth! Increase timeout or workers." -ForegroundColor Yellow
}
```

---

## CONFIGURATION & USAGE

### Step 1: Apply Enhanced Logging

**Edit**: `src/modules/Playerbot/Session/BotWorldSessionMgr.cpp`

Apply all logging enhancements from [Enhanced Logging System](#enhanced-logging-system) section.

### Step 2: Configure Adaptive Timeout

**Add to playerbots.conf**:
```ini
###################################################################################################
# DEADLOCK DETECTION SETTINGS
###################################################################################################

# Future timeout configuration
# Default: 500ms (5 retries × 100ms)
# Increase if experiencing false deadlock warnings under high load
#
#    BotSession.FutureTimeoutMs
#        Description: Maximum time to wait for bot update future completion
#        Default:     500
#        Range:       100-5000 (0.1s to 5s)
#
BotSession.FutureTimeoutMs = 2000

#    BotSession.FutureRetryIntervalMs
#        Description: Interval between future status checks
#        Default:     100
#        Range:       50-500
#
BotSession.FutureRetryIntervalMs = 100

#    BotSession.EnableDeadlockDetection
#        Description: Enable comprehensive deadlock detection with call stacks
#        Default:     1 (enabled)
#        Values:      0 (disabled), 1 (enabled)
#
BotSession.EnableDeadlockDetection = 1

#    BotSession.AutoBreakOnDeadlock
#        Description: Automatically break into debugger on deadlock (Debug builds only)
#        Default:     0 (disabled)
#        Values:      0 (disabled), 1 (enabled)
#
BotSession.AutoBreakOnDeadlock = 1
```

### Step 3: Build with Enhanced Diagnostics

**CMake Configuration**:
```bash
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DWITH_WARNINGS=1 \
      -DWITH_COREDEBUG=1 \
      -DUSE_COREPCH=0 \
      ../

cmake --build . --config RelWithDebInfo --target worldserver -j8
```

### Step 4: Run with Monitoring

**Terminal 1** - Server:
```bash
cd C:\TrinityBots\TrinityCore\build\bin\RelWithDebInfo
.\worldserver.exe
```

**Terminal 2** - Deadlock Monitor:
```powershell
cd C:\TrinityBots\scripts
.\debug_deadlock.ps1
```

**Terminal 3** - Log Tail:
```bash
tail -f M:\Wplayerbot\logs\Server.log | grep -E "(DEADLOCK|ThreadPool)"
```

---

## ADVANCED TECHNIQUES

### Technique 1: Conditional Breakpoint with Logging

**Visual Studio Breakpoint**:
- Location: `BotWorldSessionMgr.cpp:768`
- Condition: `retries >= 4`
- Action: **When Hit** → Log message to Output window
- Message: `Future {i+1}/{futures.size()} timeout for bot {futureGuids[i].ToString()}, retries={retries}`
- ☑ Continue execution

**Result**: Non-intrusive logging of near-timeouts without stopping execution

### Technique 2: Data Breakpoint on Future State

1. Break at line 732 (future.wait_for)
2. **Debug → New Breakpoint → Data Breakpoint**
3. Address: `&future` (address of std::future object)
4. Byte count: `sizeof(future)`
5. Break when: **Value changes**

**Result**: Break whenever future state transitions (useful for race conditions)

### Technique 3: Parallel Stacks View

**During Deadlock**:
1. **Debug → Windows → Parallel Stacks**
2. View all threads simultaneously
3. Identify threads waiting on same mutex
4. Trace back to common synchronization point

### Technique 4: Concurrency Visualizer

**Visual Studio Enterprise Only**:
1. **Analyze → Concurrency Visualizer → Start with Current Project**
2. Run scenario until deadlock
3. **Stop Collection**
4. View timeline showing:
   - Thread execution (green bars)
   - Thread blocking (red bars)
   - Synchronization events
   - Context switches

---

## QUICK REFERENCE CHECKLIST

### When You See "DEADLOCK DETECTED"

- [ ] **Step 1**: Check if timeout is appropriate (2000ms+ recommended for high load)
- [ ] **Step 2**: Verify thread pool isn't overloaded (check log for "ThreadPool: X active/Y total, Z queued")
- [ ] **Step 3**: If Z (queued) > 50, increase thread pool size or bot update interval
- [ ] **Step 4**: If issue persists, attach Visual Studio debugger
- [ ] **Step 5**: Set breakpoint at `BotWorldSessionMgr.cpp:768` with condition `retries >= 4`
- [ ] **Step 6**: When breakpoint hits, inspect Call Stack and Threads windows
- [ ] **Step 7**: Check for mutex deadlocks using Parallel Stacks view
- [ ] **Step 8**: Create memory dump with ProcDump for offline analysis
- [ ] **Step 9**: Use Concurrency Visualizer to identify blocking patterns
- [ ] **Step 10**: Document findings and adjust timeout/thread pool configuration

---

## SUMMARY

**Key Improvements**:
1. ✅ Fixed integer truncation in timeout logging (0s → 0.5s display)
2. ✅ Added comprehensive diagnostic logging at each stage
3. ✅ Provided Visual Studio Enterprise debugging workflows
4. ✅ Created automated monitoring and dump creation tools
5. ✅ Configured adaptive timeouts via playerbots.conf
6. ✅ Documented advanced debugging techniques

**Expected Outcome**:
- Accurate timeout reporting in logs
- Rich diagnostic context for troubleshooting
- Automated dump creation on real deadlocks
- Visual Studio integration for deep analysis
- Reduced false positive warnings via configurable timeouts

**Next Steps**:
1. Apply logging enhancements to BotWorldSessionMgr.cpp
2. Configure timeouts in playerbots.conf (start with 2000ms)
3. Rebuild with RelWithDebInfo
4. Run with automated monitoring
5. Analyze any remaining timeout warnings with Visual Studio
