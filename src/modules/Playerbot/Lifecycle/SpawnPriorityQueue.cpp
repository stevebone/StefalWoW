/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "SpawnPriorityQueue.h"
#include "Log.h"
#include "GameTime.h"
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// SpawnPriorityQueue Implementation
// ============================================================================

bool SpawnPriorityQueue::EnqueuePrioritySpawnRequest(PrioritySpawnRequest request)
{
    // ========================================================================
    // Phase 3.1 Fix: Only check duplicates for specific character GUIDs
    // ========================================================================
    // Zone/random spawn requests have empty GUID (character selected later)
    // We must allow multiple empty GUID requests to avoid rejecting all zone spawns
    if (!request.characterGuid.IsEmpty() && _requestLookup.contains(request.characterGuid))
    {
        TC_LOG_DEBUG("module.playerbot.spawn",
            "Spawn request for character {} already queued, rejecting duplicate",
            request.characterGuid.ToString());
        return false;
    }

    // Set request time if not already set
    if (request.requestTime == TimePoint{})
        request.requestTime = GameTime::Now();

    // Add to priority queue and lookup map
    _queue.push(request);

    // Only add to lookup map if GUID is specified (for duplicate detection)
    // Zone/random spawns have empty GUID and are selected later
    if (!request.characterGuid.IsEmpty())
        _requestLookup[request.characterGuid] = true;

    ++_totalRequestsEnqueued;

    TC_LOG_DEBUG("module.playerbot.spawn",
        "Enqueued spawn request: Character={}, Priority={}, QueueSize={}",
        request.characterGuid.ToString(),
        GetSpawnPriorityName(request.priority),
        _queue.size());

    return true;
}

::std::optional<PrioritySpawnRequest> SpawnPriorityQueue::DequeueNextRequest()
{
    if (_queue.empty())
        return ::std::nullopt;

    // Get highest priority request
    PrioritySpawnRequest request = _queue.top();
    _queue.pop();

    // Remove from lookup map (only if GUID was added during enqueue)
    if (!request.characterGuid.IsEmpty())
        _requestLookup.erase(request.characterGuid);
    ++_totalRequestsDequeued;

    // Update queue time metrics
    Milliseconds queueTime = request.GetAge();
    _totalQueueTime += queueTime;

    TC_LOG_DEBUG("module.playerbot.spawn",
        "Dequeued spawn request: Character={}, Priority={}, QueueTime={}ms, QueueSize={}",
        request.characterGuid.ToString(),
        GetSpawnPriorityName(request.priority),
        queueTime.count(),
        _queue.size());

    return request;
}

size_t SpawnPriorityQueue::GetQueueSize(SpawnPriority priority) const
{
    // NOTE: This is O(N) operation - counts requests at specific priority
    // For performance-critical code, cache this value or use GetMetrics()
    size_t count = 0;

    // Create temporary copy of queue for counting
    ::std::priority_queue<PrioritySpawnRequest> tempQueue = _queue;

    while (!tempQueue.empty())
    {
        if (tempQueue.top().priority == priority)
            ++count;
        tempQueue.pop();
    }

    return count;
}

void SpawnPriorityQueue::ClearQueue()
{
    size_t clearedCount = _queue.size();

    // Clear queue and lookup map
    while (!_queue.empty())
        _queue.pop();

    _requestLookup.clear();

    if (clearedCount > 0)
    {
        TC_LOG_INFO("module.playerbot.spawn",
            "Cleared spawn priority queue: {} requests removed",
            clearedCount);
    }
}

bool SpawnPriorityQueue::RemoveRequest(ObjectGuid guid)
{
    if (!_requestLookup.contains(guid))
        return false;

    // NOTE: This is O(N) operation requiring queue rebuild
    // Extract all requests except the one to remove
    ::std::vector<PrioritySpawnRequest> tempRequests;
    bool found = false;

    while (!_queue.empty())
    {
        PrioritySpawnRequest request = _queue.top();
        _queue.pop();

        if (request.characterGuid == guid)
        {
            found = true;
            TC_LOG_DEBUG("module.playerbot.spawn",
                "Removed spawn request for character {}",
                guid.ToString());
        }
        else
        {
            tempRequests.push_back(request);
        }
    }

    // Rebuild queue without removed request
    for (const PrioritySpawnRequest& request : tempRequests)
        _queue.push(request);

    // Remove from lookup map
    if (found)
        _requestLookup.erase(guid);

    return found;
}

bool SpawnPriorityQueue::ContainsRequest(ObjectGuid guid) const
{
    return _requestLookup.contains(guid);
}

QueueMetrics SpawnPriorityQueue::GetMetrics() const
{
    QueueMetrics metrics;
    metrics.totalRequests = static_cast<uint32>(_queue.size());

    if (_queue.empty())
        return metrics;

    // Count requests by priority and find oldest
    ::std::priority_queue<PrioritySpawnRequest> tempQueue = _queue;
    TimePoint oldestTime = GameTime::Now();

    while (!tempQueue.empty())
    {
        const PrioritySpawnRequest& request = tempQueue.top();

        // Count by priority
    switch (request.priority)
        {
            case SpawnPriority::CRITICAL:
                ++metrics.criticalRequests;
                break;
            case SpawnPriority::HIGH:
                ++metrics.highRequests;
                break;
            case SpawnPriority::NORMAL:
                ++metrics.normalRequests;
                break;
            case SpawnPriority::LOW:
                ++metrics.lowRequests;
                break;
        }

        // Track oldest request
    if (request.requestTime < oldestTime)
            oldestTime = request.requestTime;

        tempQueue.pop();
    }

    // Calculate age of oldest request
    metrics.oldestRequestAge = ::std::chrono::duration_cast<Milliseconds>(
        GameTime::Now() - oldestTime);

    // Calculate average queue time
    if (_totalRequestsDequeued > 0)
    {
        metrics.averageQueueTime = Milliseconds(
            _totalQueueTime.count() / _totalRequestsDequeued);
    }

    return metrics;
}

::std::vector<PrioritySpawnRequest> SpawnPriorityQueue::GetRequestsByPriority(SpawnPriority priority) const
{
    ::std::vector<PrioritySpawnRequest> requests;

    // Extract all requests at specified priority
    ::std::priority_queue<PrioritySpawnRequest> tempQueue = _queue;

    while (!tempQueue.empty())
    {
        const PrioritySpawnRequest& request = tempQueue.top();
        if (request.priority == priority)
            requests.push_back(request);
        tempQueue.pop();
    }

    return requests;
}

void SpawnPriorityQueue::RebuildLookupMap()
{
    _requestLookup.clear();

    // Rebuild lookup map from current queue
    ::std::priority_queue<PrioritySpawnRequest> tempQueue = _queue;

    while (!tempQueue.empty())
    {
        _requestLookup[tempQueue.top().characterGuid] = true;
        tempQueue.pop();
    }
}

// ============================================================================
// Utility Functions
// ============================================================================

const char* GetSpawnPriorityName(SpawnPriority priority)
{
    switch (priority)
    {
        case SpawnPriority::CRITICAL:
            return "CRITICAL";
        case SpawnPriority::HIGH:
            return "HIGH";
        case SpawnPriority::NORMAL:
            return "NORMAL";
        case SpawnPriority::LOW:
            return "LOW";
        default:
            return "UNKNOWN";
    }
}

} // namespace Playerbot
