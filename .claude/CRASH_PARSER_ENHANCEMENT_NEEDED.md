# Crash Parser Enhancement Required

**Date:** 2025-10-31
**Issue:** Parser not correctly identifying crashing thread in multi-threaded crash dumps
**Priority:** HIGH

---

## 🔍 Problem Identified

The current crash analyzer (`crash_analyzer.py`) has issues parsing Windows crash dumps correctly:

### Issue 1: Incorrect Exception Code Extraction

**Current Code (Line 217):**
```python
exception_match = re.search(r'Exception: ([A-F0-9]+)', content)
```

**Actual Dump Format (Line 14):**
```
Exception code: C0000005 ACCESS_VIOLATION
```

**Result:** Exception code shows as "UNKNOWN" instead of "C0000005"

---

### Issue 2: Incorrect Address Extraction

**Current Code (Line 218):**
```python
address_match = re.search(r'Instruction: ([A-F0-9]+)', content, re.IGNORECASE)
```

**Actual Dump Format (Line 15):**
```
Fault address:  00007FF65911FB20 01:00000000018CEB20 M:\Wplayerbot\worldserver.exe
```

**Result:** Address shows as "UNKNOWN" instead of "00007FF65911FB20"

---

### Issue 3: Wrong Call Stack Selection (CRITICAL)

**Current Code (Line 233):**
```python
stack_section = re.search(r'Call stack:(.*?)(?:\n\n|\Z)', content, re.DOTALL | re.IGNORECASE)
```

**Problem:** This captures the FIRST call stack, which is NOT the crashing thread!

**Crash Dump Structure:**
```
Call stack:   # Thread 1 (Main thread - MapUpdater waiting)
Address   Frame     Function      SourceFile
...

Call stack:   # Thread 2 (ASIO thread pool worker)
...

Call stack:   # Thread 3 (ASIO thread pool worker)
...

Call stack:   # Thread 4 (Playerbot WorkerThread - sleeping)
...

Call stack:   # Thread 5 (CRASHING THREAD!) ← This is what we need!
Address   Frame     Function      SourceFile
00007FFE57206FD3  ...  UnhandledExceptionFilter+1F3
00007FFE59CCA623  ...  strncpy+2693
00007FFE59C819B3  ...  __C_specific_handler+93
00007FFE59CC632F  ...  __chkstk+9F
00007FFE59B72327  ...  RtlLocateExtendedFeature+597
00007FFE59CC5C6E  ...  KiUserExceptionDispatcher+2E
00007FF65911FB20  ...  Playerbot::Events::EventDispatcher::UnsubscribeAll+70  ← ACTUAL CRASH LOCATION!
00007FF65921F632  ...  Playerbot::CombatStateManager::OnShutdown+132
00007FF65921DD58  ...  Playerbot::CombatStateManager::~CombatStateManager+F8
...
```

**Result:**
- Wrong call stack selected (main thread instead of crashing thread)
- Wrong crash location ("Unknown" instead of "EventDispatcher.cpp:106")
- Wrong crash function ("Unknown" instead of "Playerbot::Events::EventDispatcher::UnsubscribeAll")

---

## 🎯 Required Enhancements

### Enhancement 1: Correct Exception Code Parsing

```python
# Match both formats:
# - "Exception: C0000005"
# - "Exception code: C0000005 ACCESS_VIOLATION"
exception_match = re.search(r'Exception(?:\s+code)?:\s*([A-F0-9]+)', content, re.IGNORECASE)
```

### Enhancement 2: Correct Fault Address Parsing

```python
# Match "Fault address:  00007FF65911FB20"
address_match = re.search(r'Fault address:\s+([A-F0-9]+)', content, re.IGNORECASE)
```

### Enhancement 3: Identify Crashing Thread (CRITICAL)

**Algorithm:**
1. Split crash dump into individual thread call stacks
2. For each thread, check if it contains exception handler markers:
   - `UnhandledExceptionFilter`
   - `KiUserExceptionDispatcher`
   - `__C_specific_handler`
   - `RtlUserExceptionDispatcher`
3. The thread with these markers is the crashing thread
4. Extract call stack ONLY from the crashing thread
5. First Playerbot/TrinityCore function in that stack is the crash location

**Implementation Pseudo-code:**
```python
def parse_crash_dump_multi_thread(content):
    # 1. Extract exception code and fault address
    exception_match = re.search(r'Exception(?:\s+code)?:\s*([A-F0-9]+)', content, re.IGNORECASE)
    address_match = re.search(r'Fault address:\s+([A-F0-9]+)', content, re.IGNORECASE)

    # 2. Split into individual thread stacks
    # Each thread starts with "Call stack:\nAddress   Frame     Function      SourceFile"
    thread_stacks = []
    for match in re.finditer(r'Call stack:\s*\nAddress\s+Frame\s+Function\s+SourceFile\s*\n(.*?)(?=Call stack:|$)', content, re.DOTALL):
        stack_text = match.group(1)
        thread_stacks.append(stack_text)

    # 3. Find the crashing thread
    crashing_thread_stack = None
    exception_markers = [
        'UnhandledExceptionFilter',
        'KiUserExceptionDispatcher',
        '__C_specific_handler',
        'RtlUserExceptionDispatcher'
    ]

    for stack in thread_stacks:
        if any(marker in stack for marker in exception_markers):
            crashing_thread_stack = stack
            break

    if not crashing_thread_stack:
        # Fallback: use first stack
        crashing_thread_stack = thread_stacks[0] if thread_stacks else ""

    # 4. Parse the crashing thread's call stack
    call_stack = []
    crash_location = "Unknown"
    crash_function = "Unknown"

    for line in crashing_thread_stack.split('\n'):
        # Parse: "00007FF65911FB20  ...  Function+Offset  File line N"
        # Look for TrinityCore/Playerbot functions (skip Windows/system functions)

        # Match pattern: "FunctionName  SourceFile line LineNumber"
        match = re.search(r'([A-Za-z0-9_:]+(?:::[A-Za-z0-9_~<>]+)*)\+[0-9A-F]+\s+([A-Za-z]:[^\s]+\\([A-Za-z0-9_\.]+))\s+line\s+(\d+)', line)

        if match:
            function_name = match.group(1)
            full_path = match.group(2)
            file_name = match.group(3)
            line_num = match.group(4)

            call_stack.append(f"{function_name} @ {file_name}:{line_num}")

            # First non-system function is likely the crash location
            if crash_location == "Unknown" and not any(sys in function_name for sys in ['Rtl', 'Nt', 'Kernel', '__C_', 'BaseThread']):
                crash_location = f"{file_name}:{line_num}"
                crash_function = function_name

    return {
        'exception_code': exception_match.group(1) if exception_match else "UNKNOWN",
        'fault_address': address_match.group(1) if address_match else "UNKNOWN",
        'crash_location': crash_location,
        'crash_function': crash_function,
        'call_stack': call_stack[:15]  # Top 15 frames
    }
```

---

## 📊 Example: Current vs Enhanced Output

### Current Output (WRONG)

```
Exception Code: UNKNOWN
Fault Address: UNKNOWN
Location: Unknown
Function: boost::asio::thread_pool::thread_function::operator
Call Stack: [main thread stack - NOT the crashing thread]
```

### Expected Output (CORRECT)

```
Exception Code: C0000005
Fault Address: 00007FF65911FB20
Location: EventDispatcher.cpp:106
Function: Playerbot::Events::EventDispatcher::UnsubscribeAll
Call Stack: [crashing thread stack with exception handlers]
  1. Playerbot::Events::EventDispatcher::UnsubscribeAll @ EventDispatcher.cpp:106
  2. Playerbot::CombatStateManager::OnShutdown @ CombatStateManager.cpp:150
  3. Playerbot::CombatStateManager::~CombatStateManager @ CombatStateManager.cpp:67
  4. Playerbot::BotAI::~BotAI @ BotAI.cpp:328
  5. Playerbot::WarlockAI::`scalar deleting destructor'
  6. Playerbot::BotSession::~BotSession @ BotSession.cpp:449
  ...
```

---

## 🔧 Implementation Priority

**Priority:** HIGH (blocks accurate crash analysis)

**Impact:**
- ❌ Current: Incorrect crash location → Wrong analysis → Wrong fix
- ✅ Enhanced: Correct crash location → Accurate analysis → Correct fix

**Effort:** ~2 hours
- ~1 hour: Enhanced parser implementation
- ~30 min: Testing with multiple crash dumps
- ~30 min: Documentation update

---

## 🎓 Why This Matters

### Current Situation (TEST RUN)

```
💥 Server crashed! Analyzing...

🔍 Step 2: Python basic analysis...

Exception Code: UNKNOWN       ← WRONG
Location: Unknown              ← WRONG
Function: boost::asio::...    ← WRONG (this is a background thread, not the crash!)
Category: UNKNOWN_PATTERN      ← WRONG (because location is unknown)
Severity: LOW                  ← WRONG (should be HIGH - null pointer in EventDispatcher)
```

### With Enhancement

```
💥 Server crashed! Analyzing...

🔍 Step 2: Python basic analysis...

Exception Code: C0000005 ACCESS_VIOLATION  ← CORRECT!
Location: EventDispatcher.cpp:106           ← CORRECT!
Function: Playerbot::Events::EventDispatcher::UnsubscribeAll  ← CORRECT!
Category: PLAYERBOT_CRASH (EventDispatcher)  ← CORRECT!
Severity: HIGH (crash in critical bot component)  ← CORRECT!
Root Cause: Access violation in EventDispatcher::UnsubscribeAll during CombatStateManager shutdown
```

---

## ✅ Next Steps

1. **Implement enhanced parser** in `crash_analyzer.py`
2. **Test with multiple crash dumps** to ensure robustness
3. **Update crash pattern matching** to use correct crash location
4. **Verify Claude Code analysis** receives accurate crash info

---

## 📖 References

**Crash Dump Location:** `M:/Wplayerbot/Crashes/7d14957a32df+_worldserver.exe_[2025_10_31_11_14_33].txt`

**Crashing Thread Call Stack (Lines 513-545):**
- Line 519: `UnhandledExceptionFilter` (exception handler)
- Line 524: `KiUserExceptionDispatcher` (Windows exception dispatcher)
- Line 525: **CRASH HERE** → `Playerbot::Events::EventDispatcher::UnsubscribeAll+70` at `EventDispatcher.cpp line 106`
- Lines 526-534: Cleanup chain (CombatStateManager destructor → BotAI destructor → BotSession destructor)

**Root Cause:** Null pointer or invalid memory access when unsubscribing all events during BotSession cleanup.

---

**Status:** 📋 Enhancement Specification Complete - Ready for Implementation
**Priority:** 🔴 HIGH
**Estimated Effort:** ~2 hours

**🤖 Generated with [Claude Code](https://claude.com/claude-code)**
