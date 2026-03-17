// IMMEDIATE MUTEX CONTENTION FIX
// Priority: CRITICAL - Apply these changes to resolve bot stalls

// ==================================================================
// FILE 1: src/modules/Playerbot/Economy/AuctionManager.cpp
// ==================================================================

// BEFORE (Line 56-82) - WITH BOTTLENECK:
/*
void AuctionManager::OnUpdate(uint32 elapsed)
{
    if (!GetBot() || !GetBot()->IsInWorld() || !IsEnabled())
        return;

    std::lock_guard<std::recursive_mutex> lock(_mutex);  // REMOVE THIS

    _updateTimer += elapsed;
    _marketScanTimer += elapsed;

    // Periodic market scan
    if (_marketScanTimer >= _marketScanInterval)
    {
        _marketScanTimer = 0;
    }

    // Clean up stale price data periodically
    auto now = std::chrono::steady_clock::now();
    for (auto it = _priceCache.begin(); it != _priceCache.end();)
    {
        if (now - it->second.LastUpdate > Minutes(_priceHistoryDays * 1440))
            it = _priceCache.erase(it);
        else
            ++it;
    }
}
*/

// AFTER - FIXED VERSION:
void AuctionManager::OnUpdate(uint32 elapsed)
{
    if (!GetBot() || !GetBot()->IsInWorld() || !IsEnabled())
        return;

    // NO LOCK - Each bot has its own AuctionManager instance
    // These members are not shared between bots

    _updateTimer += elapsed;
    _marketScanTimer += elapsed;

    // Periodic market scan
    if (_marketScanTimer >= _marketScanInterval)
    {
        _marketScanTimer = 0;
        // Market scanning is per-bot, no shared state
    }

    // Clean up stale price data periodically
    auto now = std::chrono::steady_clock::now();

    // If _priceCache becomes shared in the future, use this pattern:
    // std::vector<uint32> itemsToRemove;
    // for (auto it = _priceCache.begin(); it != _priceCache.end(); ++it)
    // {
    //     if (now - it->second.LastUpdate > Minutes(_priceHistoryDays * 1440))
    //         itemsToRemove.push_back(it->first);
    // }
    // for (uint32 itemId : itemsToRemove)
    //     _priceCache.erase(itemId);

    // For now, direct iteration is safe (no shared state)
    for (auto it = _priceCache.begin(); it != _priceCache.end();)
    {
        if (now - it->second.LastUpdate > Minutes(_priceHistoryDays * 1440))
            it = _priceCache.erase(it);
        else
            ++it;
    }
}

// ==================================================================
// FILE 2: src/modules/Playerbot/Core/Managers/ManagerRegistry.cpp
// ==================================================================

// BEFORE (Line 270-326) - WITH GLOBAL LOCK:
/*
uint32 ManagerRegistry::UpdateAll(uint32 diff)
{
    std::lock_guard<std::recursive_mutex> lock(_managerMutex);  // GLOBAL BOTTLENECK

    if (!_initialized)
        return 0;

    uint32 updateCount = 0;
    uint64 currentTime = getMSTime();

    for (auto& [managerId, entry] : _managers)
    {
        // ... update logic ...
    }

    return updateCount;
}
*/

// AFTER - PARALLEL UPDATE VERSION:
uint32 ManagerRegistry::UpdateAll(uint32 diff)
{
    // NO GLOBAL LOCK - Allow parallel manager updates

    if (!_initialized.load(std::memory_order_acquire))
        return 0;

    std::atomic<uint32> updateCount{0};
    uint64 currentTime = getMSTime();

    // Create a snapshot of managers to update (quick lock)
    std::vector<std::pair<std::string, IManagerBase*>> managersToUpdate;
    {
        std::lock_guard<std::recursive_mutex> lock(_managerMutex);
        for (auto& [managerId, entry] : _managers)
        {
            if (!entry.initialized || !entry.manager)
                continue;

            if (!entry.manager->IsActive())
                continue;

            uint32 updateInterval = entry.manager->GetUpdateInterval();
            uint64 timeSinceLastUpdate = currentTime - entry.lastUpdateTime;

            if (timeSinceLastUpdate >= updateInterval)
            {
                managersToUpdate.push_back({managerId, entry.manager.get()});
                entry.lastUpdateTime = currentTime;  // Update timestamp
            }
        }
    }
    // Lock released here - managers can now update in parallel

    // Update managers without holding the global lock
    for (auto& [managerId, manager] : managersToUpdate)
    {
        try
        {
            uint64 updateStartTime = getMSTime();

            manager->Update(diff);  // No lock held during actual update

            uint64 updateTime = getMSTimeDiff(updateStartTime, getMSTime());
            updateCount.fetch_add(1, std::memory_order_relaxed);

            // Log slow updates
            if (updateTime > 1)  // >1ms is concerning
            {
                TC_LOG_WARN("module.playerbot.managers",
                    "Manager '{}' update took {}ms (expected <1ms)",
                    managerId, updateTime);
            }
        }
        catch (std::exception const& ex)
        {
            TC_LOG_ERROR("module.playerbot.managers",
                "Exception updating manager '{}': {}", managerId, ex.what());
        }
    }

    // Update metrics atomically (if needed)
    {
        std::lock_guard<std::recursive_mutex> lock(_managerMutex);
        for (auto& [managerId, manager] : managersToUpdate)
        {
            auto it = _managers.find(managerId);
            if (it != _managers.end())
            {
                it->second.totalUpdates++;
                // Update other metrics if needed
            }
        }
    }

    return updateCount.load();
}

// ==================================================================
// FILE 3: src/modules/Playerbot/Professions/GatheringManager.cpp
// ==================================================================

// BEFORE (Line 80-121) - WITH UNNECESSARY LOCK:
/*
void GatheringManager::OnUpdate(uint32 elapsed)
{
    if (!GetBot() || !GetBot()->IsInWorld() || !_gatheringEnabled)
        return;

    // ... update logic with potential lock operations ...
}
*/

// AFTER - LOCK-FREE VERSION:
void GatheringManager::OnUpdate(uint32 elapsed)
{
    if (!GetBot() || !GetBot()->IsInWorld() || !_gatheringEnabled)
        return;

    // NO LOCK NEEDED - Per-bot instance data

    // Update node detection every few seconds
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastScanTime).count() >= NODE_SCAN_INTERVAL)
    {
        UpdateDetectedNodes();  // This method should also avoid unnecessary locks
        _lastScanTime = now;
    }

    // Process current gathering action using atomic flag
    if (_isGathering.load(std::memory_order_acquire))
    {
        ProcessCurrentGathering();
    }
    // If not gathering and nodes are available, select best node
    else if (_hasNearbyResources.load(std::memory_order_acquire) && !GetBot()->IsInCombat())
    {
        GatheringNode const* bestNode = SelectBestNode();
        if (bestNode && CanGatherFromNode(*bestNode))
        {
            if (IsInGatheringRange(*bestNode))
            {
                GatherFromNode(*bestNode);
            }
            else if (_gatherWhileMoving)
            {
                PathToNode(*bestNode);
            }
        }
    }

    // Clean up expired nodes
    CleanupExpiredNodes();

    // Update state flags atomically
    _detectedNodeCount.store(static_cast<uint32>(_detectedNodes.size()), std::memory_order_release);
    _hasNearbyResources.store(!_detectedNodes.empty(), std::memory_order_release);
}

// For FindNearestNode - use read-only iteration when possible:
GatheringNode const* GatheringManager::FindNearestNode(GatheringNodeType nodeType) const
{
    // If _detectedNodes is rarely modified, we can use a double-buffering approach
    // or copy-on-write semantics to avoid locks entirely

    // For now, if we must lock, use a shared_lock for reading:
    // std::shared_lock lock(_nodeMutex);  // Multiple readers OK

    // Better: Use lock-free container or snapshot approach
    GatheringNode const* nearest = nullptr;
    float minDistance = std::numeric_limits<float>::max();

    // Create a local snapshot if needed, or use atomic operations
    for (auto const& node : _detectedNodes)  // If this is thread-local, no lock needed
    {
        if (nodeType != GatheringNodeType::NONE && node.nodeType != nodeType)
            continue;

        if (node.distance < minDistance && node.isActive)
        {
            minDistance = node.distance;
            nearest = &node;
        }
    }

    return nearest;
}

// ==================================================================
// FILE 4: src/modules/Playerbot/Economy/AuctionManager.h
// ==================================================================

// BEFORE - WITH RECURSIVE MUTEX:
/*
class AuctionManager : public BehaviorManager
{
private:
    mutable std::recursive_mutex _mutex;  // PROBLEMATIC
    // ...
};
*/

// AFTER - WITH FINE-GRAINED LOCKING:
class AuctionManager : public BehaviorManager
{
private:
    // Remove the global mutex, use fine-grained locking only where needed
    // mutable std::recursive_mutex _mutex;  // REMOVED

    // If shared state exists, use appropriate synchronization:
    // For read-heavy operations:
    mutable std::shared_mutex _priceCacheMutex;  // Multiple readers, single writer

    // For frequently updated counters:
    std::atomic<uint32> _updateTimer{0};
    std::atomic<uint32> _marketScanTimer{0};

    // For per-bot data that's never shared:
    // No synchronization needed at all!
    std::vector<BotAuctionData> _myAuctions;  // Per-bot, no lock needed

public:
    // Example of lock-free getter:
    uint32 GetUpdateTimer() const noexcept {
        return _updateTimer.load(std::memory_order_relaxed);
    }

    // Example of fine-grained locking for truly shared data:
    ItemPriceData GetItemPriceData(uint32 itemId) const {
        std::shared_lock lock(_priceCacheMutex);  // Read lock
        auto it = _priceCache.find(itemId);
        if (it != _priceCache.end())
            return it->second;
        return ItemPriceData();
    }

    void UpdateItemPrice(uint32 itemId, uint64 price) {
        std::unique_lock lock(_priceCacheMutex);  // Write lock
        _priceCache[itemId].CurrentPrice = price;
        _priceCache[itemId].LastUpdate = std::chrono::steady_clock::now();
    }
};

// ==================================================================
// TESTING APPROACH
// ==================================================================

/*
1. Apply these changes to the identified files
2. Rebuild with: cmake --build . --target worldserver --config RelWithDebInfo
3. Test with 10 bots first:
   - Start server
   - Spawn 10 bots
   - Monitor for crashes
   - Check logs for "CRITICAL: bots stalled"

4. If stable, scale to 100 bots:
   - Spawn 100 bots
   - Monitor performance
   - Expected: No "CRITICAL: bots stalled" messages
   - Expected: Update times <1ms per bot

5. Stress test with 500 bots if hardware allows

6. Validation checks:
   - grep "CRITICAL.*stalled" logs/Server.log | wc -l
   - Should show 0 or very few stalls

   - grep "Manager.*update took.*ms" logs/Playerbot.log | tail -20
   - Should show update times <1ms
*/

// ==================================================================
// ADDITIONAL OPTIMIZATION: WORK-STEALING BOT UPDATES
// ==================================================================

// For even better performance, implement work-stealing:

class BotUpdateScheduler
{
private:
    struct alignas(64) WorkQueue  // Cache-line aligned
    {
        moodycamel::ConcurrentQueue<BotAI*> queue;
        std::atomic<uint32> size{0};
    };

    std::array<WorkQueue, 8> _queues;  // One per worker thread
    std::atomic<uint32> _nextQueue{0};

public:
    void ScheduleBotUpdate(BotAI* bot)
    {
        // Round-robin distribution
        uint32 queueIdx = _nextQueue.fetch_add(1, std::memory_order_relaxed) % _queues.size();
        _queues[queueIdx].queue.enqueue(bot);
        _queues[queueIdx].size.fetch_add(1, std::memory_order_relaxed);
    }

    void WorkerThread(uint32 workerId)
    {
        BotAI* bot = nullptr;
        uint32 stealAttempts = 0;

        while (_running)
        {
            // Try own queue first
            if (_queues[workerId].queue.try_dequeue(bot))
            {
                _queues[workerId].size.fetch_sub(1, std::memory_order_relaxed);
                bot->Update(GetDiff());  // No locks!
                stealAttempts = 0;
            }
            // Work stealing from other queues
            else if (stealAttempts++ < _queues.size())
            {
                uint32 victimId = (workerId + stealAttempts) % _queues.size();
                if (_queues[victimId].size.load(std::memory_order_relaxed) > 0)
                {
                    if (_queues[victimId].queue.try_dequeue(bot))
                    {
                        _queues[victimId].size.fetch_sub(1, std::memory_order_relaxed);
                        bot->Update(GetDiff());  // No locks!
                        stealAttempts = 0;
                    }
                }
            }
            else
            {
                // No work available, yield
                std::this_thread::yield();
                stealAttempts = 0;
            }
        }
    }
};