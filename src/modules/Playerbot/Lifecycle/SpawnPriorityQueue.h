/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_SPAWN_PRIORITY_QUEUE_H
#define PLAYERBOT_SPAWN_PRIORITY_QUEUE_H

#include "Define.h"
#include "Duration.h"
#include "ObjectGuid.h"
#include "GameTime.h"
#include "SpawnRequest.h"  // For original spawn request data
#include <queue>
#include <unordered_map>
#include <string>
#include <optional>

namespace Playerbot
{
    /**
     * @brief Priority levels for bot spawn requests
     *
     * Higher priority bots are spawned first during startup
     * and resource-constrained periods.
     */
    enum class SpawnPriority : uint8
    {
        CRITICAL = 0,   ///< Guild leaders, raid leaders - spawn immediately
        HIGH = 1,       ///< Party members, friends - spawn within 30s
        NORMAL = 2,     ///< Standard bots - spawn within 2 minutes
        LOW = 3         ///< Background/filler bots - spawn within 10 minutes
    };

    /**
     * @brief Bot spawn request with priority and metadata
     *
     * Represents a single bot spawn request in the priority queue.
     * Includes retry tracking and timing information for metrics.
     */
    struct TC_GAME_API PrioritySpawnRequest
    {
        ObjectGuid characterGuid;       ///< Character GUID to spawn (may be empty for zone/random spawns)
        uint32 accountId;               ///< Account ID owning the character
        SpawnPriority priority;         ///< Spawn priority level
        TimePoint requestTime;          ///< When request was created
        uint32 retryCount = 0;          ///< Number of spawn retry attempts
        ::std::string reason;             ///< Reason for spawn (for debugging/metrics)
        SpawnRequest originalRequest;   ///< Full original spawn request with all parameters

        /**
         * @brief Priority comparison operator for heap ordering
         * @param other Other spawn request to compare against
         * @return true if this request has LOWER priority (for max-heap)
         *
         * Priority ordering (highest first):
         * 1. SpawnPriority (CRITICAL > HIGH > NORMAL > LOW)
         * 2. Request time (older requests first within same priority)
         */
        bool operator<(const PrioritySpawnRequest& other) const
        {
            // Higher priority enum value = lower actual priority
            if (priority != other.priority)
                return priority > other.priority;  // Reverse for max-heap

            // Within same priority, older requests first
            return requestTime > other.requestTime;  // Reverse for max-heap
        }

        /**
         * @brief Get age of this spawn request
         * @return Duration since request was created
         */
        Milliseconds GetAge() const
        {
            return ::std::chrono::duration_cast<Milliseconds>(GameTime::Now() - requestTime);
        }
    };

    /**
     * @brief Priority queue metrics for monitoring and debugging
     */
    struct TC_GAME_API QueueMetrics
    {
        uint32 totalRequests = 0;        ///< Total requests in queue
        uint32 criticalRequests = 0;     ///< CRITICAL priority count
        uint32 highRequests = 0;         ///< HIGH priority count
        uint32 normalRequests = 0;       ///< NORMAL priority count
        uint32 lowRequests = 0;          ///< LOW priority count

        Milliseconds oldestRequestAge;   ///< Age of oldest request
        Milliseconds averageQueueTime;   ///< Average time requests spend in queue

        /**
         * @brief Check if queue is empty
         */
        bool IsEmpty() const { return totalRequests == 0; }
    };

    /**
     * @brief Priority-based bot spawn request queue
     *
     * Thread-safe priority queue for managing bot spawn requests with
     * support for multiple priority levels, request tracking, and metrics.
     *
     * Features:
     * - Priority-based ordering (CRITICAL > HIGH > NORMAL > LOW)
     * - FIFO within same priority level (older requests first)
     * - Duplicate detection (prevent multiple requests for same character)
     * - Request removal by GUID
     * - Comprehensive metrics tracking
     *
     * Performance:
     * - Enqueue: O(log N) - heap insertion
     * - Dequeue: O(log N) - heap extraction
     * - Contains check: O(1) - hash map lookup
     * - Memory: O(N) where N = number of queued requests
     *
     * Thread Safety:
     * - Not thread-safe, must be called from world update thread
     * - Designed for single-threaded access pattern
     *
     * @example
     * @code
     * SpawnPriorityQueue queue;
     *
     * // Enqueue spawn requests
     * PrioritySpawnRequest request;
     * request.characterGuid = guid;
     * request.priority = SpawnPriority::HIGH;
     * queue.EnqueuePrioritySpawnRequest(request);
     *
     * // Dequeue highest priority request
     * if (auto nextRequest = queue.DequeueNextRequest())
     * {
     *     // Spawn bot for nextRequest->characterGuid
     * }
     * @endcode
     */
    class TC_GAME_API SpawnPriorityQueue
    {
    public:
        SpawnPriorityQueue() = default;
        ~SpawnPriorityQueue() = default;

        /**
         * @brief Add spawn request to priority queue
         * @param request Spawn request to enqueue
         * @return true if request was added, false if duplicate
         *
         * Duplicates (same character GUID) are rejected to prevent
         * double-spawning. The existing request remains in queue.
         */
        bool EnqueuePrioritySpawnRequest(PrioritySpawnRequest request);

        /**
         * @brief Remove and return highest priority spawn request
         * @return PrioritySpawnRequest if queue not empty, std::nullopt otherwise
         *
         * Returns the highest priority request according to ordering:
         * 1. Priority level (CRITICAL > HIGH > NORMAL > LOW)
         * 2. Request age (older requests first within same priority)
         */
        ::std::optional<PrioritySpawnRequest> DequeueNextRequest();

        /**
         * @brief Get queue size for specific priority level
         * @param priority Priority level to query
         * @return Number of requests at this priority level
         */
        size_t GetQueueSize(SpawnPriority priority) const;

        /**
         * @brief Get total queue size (all priorities)
         * @return Total number of queued spawn requests
         */
        size_t GetTotalQueueSize() const { return _queue.size(); }

        /**
         * @brief Check if queue is empty
         * @return true if no requests queued
         */
        bool IsEmpty() const { return _queue.empty(); }

        /**
         * @brief Clear all queued requests
         */
        void ClearQueue();

        /**
         * @brief Remove specific request by character GUID
         * @param guid Character GUID to remove
         * @return true if request was found and removed
         *
         * This is O(N) operation requiring queue rebuild.
         * Use sparingly for exceptional cases only.
         */
        bool RemoveRequest(ObjectGuid guid);

        /**
         * @brief Check if character already has queued request
         * @param guid Character GUID to check
         * @return true if request exists in queue
         */
        bool ContainsRequest(ObjectGuid guid) const;

        /**
         * @brief Get queue metrics for monitoring
         * @return QueueMetrics Current queue state
         */
        QueueMetrics GetMetrics() const;

        /**
         * @brief Get requests by priority level (for debugging)
         * @param priority Priority level to query
         * @return Vector of requests at this priority
         */
        ::std::vector<PrioritySpawnRequest> GetRequestsByPriority(SpawnPriority priority) const;

    private:
        /**
         * @brief Rebuild hash map after queue modification
         *
         * Called after RemoveRequest() to reconstruct _requestLookup
         * from current queue state.
         */
        void RebuildLookupMap();

    private:
        // Priority queue using max-heap
        ::std::priority_queue<PrioritySpawnRequest> _queue;

        // Fast lookup for duplicate detection (GUID → exists)
        ::std::unordered_map<ObjectGuid, bool> _requestLookup;

        // Metrics tracking
        mutable uint64 _totalRequestsEnqueued = 0;
        mutable uint64 _totalRequestsDequeued = 0;
        mutable Milliseconds _totalQueueTime = Milliseconds(0);
    };

    /**
     * @brief Get string name for SpawnPriority enum
     * @param priority Priority enum value
     * @return String representation ("CRITICAL", "HIGH", "NORMAL", "LOW")
     */
    TC_GAME_API const char* GetSpawnPriorityName(SpawnPriority priority);

} // namespace Playerbot

#endif // PLAYERBOT_SPAWN_PRIORITY_QUEUE_H
