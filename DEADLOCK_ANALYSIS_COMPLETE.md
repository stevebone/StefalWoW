# CRITICAL DEADLOCK ANALYSIS - Complete Root Cause Investigation

## Executive Summary
After analyzing 8 failed fix attempts, the root cause of the "resource deadlock would occur" exception has been identified. The deadlock occurs due to **recursive mutex acquisition** in the BotAI class when `std::shared_mutex _mutex` is acquired while already held on the same thread.

## Affected Systems
- **Error**: `std::system_error: resource deadlock would occur`  
- **Affected Accounts**: 26, 27, 28, 29, 33, 35
- **Trigger**: 50+ concurrent bots
- **Mutex**: `std::shared_mutex` (non-recursive)

## All BotAI Methods Acquiring _mutex

### Read Locks (std::shared_lock)
- **UpdateStrategies()** Line 220
- **OnGroupJoined()** Line 625
- **GetStrategy()** Line 708  
- **GetActiveStrategies()** Line 715

### Write Locks (std::unique_lock)
- **AddStrategy()** Line 689
- **RemoveStrategy()** Line 696
- **ActivateStrategy()** Line 731
- **DeactivateStrategy()** Line 755

## THE DEADLOCK - Lock Ordering Issue

**Root Cause:** `std::shared_mutex` blocks NEW readers when a writer is waiting (prevents writer starvation)

**Deadlock Scenario:**
1. Thread A: Holds shared_lock in UpdateStrategies [Line 220]
2. Thread B: Requests unique_lock in ActivateStrategy [Line 731] → BLOCKS
3. Thread A: Releases lock [Line 234], calls strategy->UpdateBehavior()
4. Strategy callback: Calls ai->GetStrategy() → tries shared_lock [Line 708]
5. **DEADLOCK**: Thread B's pending unique_lock blocks Thread A's new shared_lock

## THE FIX - Release Lock Before Callbacks

**File**: `src/modules/Playerbot/AI/BotAI.cpp`

### ActivateStrategy (Lines 729-751)
```cpp
void BotAI::ActivateStrategy(std::string const& name)
{
    Strategy* strategy = nullptr;
    {
        std::unique_lock lock(_mutex);
        auto it = _strategies.find(name);
        if (it == _strategies.end())
            return;
        if (std::find(_activeStrategies.begin(), _activeStrategies.end(), name) != _activeStrategies.end())
            return;
        _activeStrategies.push_back(name);
        it->second->SetActive(true);
        strategy = it->second.get();
    } // RELEASE LOCK BEFORE CALLBACK
    
    if (strategy)
        strategy->OnActivate(this);  // Call without lock
}
```

### DeactivateStrategy (Lines 753-774)
```cpp
void BotAI::DeactivateStrategy(std::string const& name)
{
    Strategy* strategy = nullptr;
    {
        std::unique_lock lock(_mutex);
        auto it = _strategies.find(name);
        if (it != _strategies.end())
        {
            strategy = it->second.get();
            it->second->SetActive(false);
        }
        _activeStrategies.erase(
            std::remove(_activeStrategies.begin(), _activeStrategies.end(), name),
            _activeStrategies.end()
        );
    } // RELEASE LOCK BEFORE CALLBACK
    
    if (strategy)
        strategy->OnDeactivate(this);  // Call without lock
}
```

## Why Previous Fixes Failed
- Attempts 1-8 focused on UpdateStrategies lock scope
- Missed the interaction between:
  * Pending unique_lock in ActivateStrategy (Thread B)
  * New shared_lock request from strategy callback (Thread A)
  * std::shared_mutex writer-preference blocking

## Verification
1. Apply fix to ActivateStrategy and DeactivateStrategy
2. Test with 100 concurrent bots
3. Monitor for deadlock exceptions for 24 hours
