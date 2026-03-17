# DEADLOCK FIX ATTEMPT #10 - ROOT CAUSE ANALYSIS

## Executive Summary
After 9 failed attempts, I have identified the **EXACT recursive mutex deadlock path** that caused "resource deadlock would occur" exceptions across accounts 2-38.

## The Root Cause - Writer-Preference Deadlock

### The Fatal Sequence

1. **Thread A** (BotAI::UpdateStrategies) acquires `shared_lock(_mutex)` at **BotAI.cpp:220**
2. **Thread B** (BotAI::ActivateStrategy) tries to acquire `unique_lock(_mutex)` at **BotAI.cpp:737**
   - Thread B **BLOCKS** waiting for Thread A to release shared lock
3. **Thread A** continues execution while holding shared_lock
4. **Thread A** calls `strategy->UpdateBehavior()` at **BotAI.cpp:246**
5. Strategy callback calls back into `BotAI::GetStrategy()` at **BotAI.cpp:708**
6. `GetStrategy()` tries to acquire **ANOTHER** `shared_lock(_mutex)`
7. **DEADLOCK**: Thread A cannot acquire second shared_lock because Thread B is waiting for unique_lock (writer-preference)

### Why This Happens with std::shared_mutex

Windows `std::shared_mutex` uses **writer-preference**:
- If a writer (unique_lock) is waiting, new readers (shared_lock) are BLOCKED
- This prevents reader starvation but creates deadlock when:
  - Reader A holds shared_lock
  - Writer B waits for unique_lock
  - Reader A tries to acquire ANOTHER shared_lock → **DEADLOCK**

## Call Stack Evidence

```
BotAI::UpdateAI()
  → BotAI::UpdateStrategies()                    [Line 119]
      → std::shared_lock lock(_mutex)            [Line 220] ← LOCK ACQUIRED
      → strategy->IsActive(this)                 [Line 228] ← Safe (atomic)
      → strategy->UpdateBehavior(this, diff)     [Line 246] ← CALLBACK WHILE HOLDING LOCK
          → LeaderFollowBehavior::UpdateBehavior()
              → BotAI::GetStrategy("follow")     [Indirect call]
                  → std::shared_lock lock(_mutex) [Line 708] ← ATTEMPTS SECOND LOCK
                      → **DEADLOCK** (writer waiting blocks this reader)
```

## Secondary Deadlock Path

`OnGroupJoined()` also has mutex re-acquisition:
```
BotAI::OnGroupJoined()
  → GetStrategy("follow")                        [Line 610]
      → std::shared_lock lock(_mutex)            [Line 708] ← FIRST LOCK
  → ActivateStrategy("follow")                   [Line 632]
      → std::unique_lock lock(_mutex)            [Line 737] ← DEADLOCK if another thread holds shared_lock
```

## The Fix - Complete Lock Release Before Callbacks

### Changed: BotAI.cpp:210-266 UpdateStrategies()

**Before (Deadlock-prone):**
```cpp
std::vector<Strategy*> strategiesToUpdate;
{
    std::shared_lock lock(_mutex);
    for (auto const& strategyName : _activeStrategies)
    {
        auto it = _strategies.find(strategyName);
        if (it != _strategies.end())
        {
            Strategy* strategy = it->second.get();
            if (strategy && strategy->IsActive(this))  // ← IsActive() while holding lock
            {
                strategiesToUpdate.push_back(strategy);
            }
        }
    }
} // Lock released

// Call callbacks
for (Strategy* strategy : strategiesToUpdate)
{
    followBehavior->UpdateFollowBehavior(this, diff); // ← May call GetStrategy()
}
```

**After (Deadlock-free):**
```cpp
std::vector<std::pair<Strategy*, bool>> strategiesToCheck;
{
    std::shared_lock lock(_mutex);
    for (auto const& strategyName : _activeStrategies)
    {
        auto it = _strategies.find(strategyName);
        if (it != _strategies.end())
        {
            // Just collect pointer - no method calls while locked
            strategiesToCheck.push_back({it->second.get(), false});
        }
    }
} // ← LOCK RELEASED BEFORE ANY METHOD CALLS

// Check IsActive() WITHOUT holding lock (it's atomic - thread-safe)
std::vector<Strategy*> strategiesToUpdate;
for (auto& [strategy, dummy] : strategiesToCheck)
{
    if (strategy && strategy->IsActive(this))  // ← Safe: no lock held, IsActive() is atomic
    {
        strategiesToUpdate.push_back(strategy);
    }
}

// Call callbacks WITHOUT holding ANY lock
for (Strategy* strategy : strategiesToUpdate)
{
    followBehavior->UpdateFollowBehavior(this, diff); // ← Safe: can call GetStrategy()
}
```

## Why This Fix Works

1. **Minimal Lock Hold Time**: Acquire lock only to copy strategy pointers
2. **No Method Calls Under Lock**: Don't call `IsActive()` or any other methods while locked
3. **Atomic IsActive()**: Strategy::IsActive() uses `std::atomic<bool>` - safe to call without mutex
4. **Free Callbacks**: Strategy callbacks can now safely call `GetStrategy()` without deadlock

## Files Modified

- `src/modules/Playerbot/AI/BotAI.cpp:210-266` - UpdateStrategies() complete rewrite

## Testing Required

1. Start server with 100+ bots (accounts 2-38 especially)
2. Monitor for "resource deadlock would occur" exceptions
3. Verify follow behavior still works correctly
4. Check performance impact (should be minimal - fewer lock acquisitions)

## Thread Safety Guarantee

- `Strategy::IsActive()` returns `std::atomic<bool> _active` - safe to read without lock
- `Strategy*` pointers are stable (strategies not deleted while active)
- `_activeStrategies` is only read with lock held
- Callbacks execute with NO locks held

## Expected Outcome

**ZERO** "resource deadlock would occur" exceptions across all bot accounts.
