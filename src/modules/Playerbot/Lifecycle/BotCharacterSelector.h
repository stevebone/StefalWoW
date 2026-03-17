/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include "Lifecycle/SpawnRequest.h"
#include <vector>
#include <functional>
#include <memory>
#include <queue>
#include <mutex>

namespace Playerbot
{

/**
 * @class BotCharacterSelector
 * @brief Handles async character selection and creation for bot spawning
 *
 * SINGLE RESPONSIBILITY: Manages all character selection logic extracted
 * from the monolithic BotSpawner class.
 *
 * Responsibilities:
 * - Async character queries and selection
 * - Account management and character discovery
 * - Character creation when needed
 * - Callback-based async workflow management
 * - Character filtering and validation
 *
 * Performance Features:
 * - Fully async database operations (no blocking)
 * - Character result caching
 * - Batched character queries
 * - Connection pooling integration
 * - Recursive account processing
 */
class TC_GAME_API BotCharacterSelector
{
public:
    BotCharacterSelector();
    ~BotCharacterSelector() = default;

    // Lifecycle
    bool Initialize();
    void Shutdown();

    // === ASYNC CHARACTER SELECTION ===
    using CharacterCallback = ::std::function<void(ObjectGuid)>;
    using CharacterListCallback = ::std::function<void(::std::vector<ObjectGuid>)>;

    void SelectCharacterAsync(SpawnRequest const& request, CharacterCallback callback);
    void GetAvailableCharactersAsync(uint32 accountId, SpawnRequest const& request, CharacterListCallback callback);

    // === BATCH OPERATIONS ===
    void ProcessBatchSelection(::std::vector<SpawnRequest> const& requests,
                              ::std::function<void(::std::vector<ObjectGuid>)> callback);

    // === CHARACTER CREATION ===
    ObjectGuid CreateCharacterForAccount(uint32 accountId, SpawnRequest const& request);
    ObjectGuid CreateBotCharacter(uint32 accountId);

    // === VALIDATION ===
    bool ValidateCharacter(ObjectGuid characterGuid, SpawnRequest const& request) const;
    uint32 GetAccountIdFromCharacter(ObjectGuid characterGuid) const;

    // === PERFORMANCE METRICS ===
    struct SelectionStats
    {
        ::std::atomic<uint32> totalSelections{0};
        ::std::atomic<uint32> cacheHits{0};
        ::std::atomic<uint32> cacheMisses{0};
        ::std::atomic<uint32> charactersCreated{0};
        ::std::atomic<uint64> avgSelectionTimeUs{0};

        float GetCacheHitRate() const
        {
            uint32 total = cacheHits.load() + cacheMisses.load();
            return total > 0 ? static_cast<float>(cacheHits.load()) / total * 100.0f : 0.0f;
        }
    };

    SelectionStats const& GetStats() const { return _stats; }
    void ResetStats();

private:
    // === ASYNC WORKFLOW IMPLEMENTATION ===
    void SelectCharacterFromAccounts(::std::vector<uint32> accounts, size_t index,
                                    SpawnRequest const& request, CharacterCallback callback);

    void ProcessAccountCharacters(uint32 accountId, SpawnRequest const& request,
                                ::std::vector<ObjectGuid> characters, CharacterCallback callback);

    void HandleCharacterCreation(uint32 accountId, SpawnRequest const& request, CharacterCallback callback);

    // === CHARACTER FILTERING ===
    ::std::vector<ObjectGuid> FilterCharactersByRequest(::std::vector<ObjectGuid> const& characters,
                                                     SpawnRequest const& request) const;

    bool MatchesRequestCriteria(ObjectGuid characterGuid, SpawnRequest const& request) const;

    // === ACCOUNT MANAGEMENT ===
    ::std::vector<uint32> GetAvailableAccounts(SpawnRequest const& request) const;
    uint32 AcquireSuitableAccount(SpawnRequest const& request) const;

    // === CHARACTER CACHING ===
    struct CharacterCacheEntry
    {
        ::std::vector<ObjectGuid> characters;
        ::std::chrono::steady_clock::time_point lastUpdate;
        bool isValid = false;
    };

    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_SPAWNER> _cacheMutex;
    ::std::unordered_map<uint32, CharacterCacheEntry> _characterCache; // accountId -> characters

    void UpdateCharacterCache(uint32 accountId, ::std::vector<ObjectGuid> const& characters);
    ::std::vector<ObjectGuid> GetCachedCharacters(uint32 accountId) const;
    void InvalidateCache(uint32 accountId);

    // === REQUEST QUEUE MANAGEMENT ===
    struct PendingRequest
    {
        SpawnRequest request;
        CharacterCallback callback;
        ::std::chrono::steady_clock::time_point timestamp;
    };

    ::std::queue<PendingRequest> _pendingRequests;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_SPAWNER> _requestMutex;
    ::std::atomic<bool> _processingRequests{false};

    void QueueRequest(SpawnRequest const& request, CharacterCallback callback);
    void ProcessPendingRequests();

    // === PERFORMANCE TRACKING ===
    mutable SelectionStats _stats;

    void RecordSelection(uint64 durationMicroseconds, bool cacheHit);

    // === CONFIGURATION ===
    static constexpr uint32 CACHE_VALIDITY_MS = 30000;     // 30 seconds
    static constexpr uint32 MAX_CACHED_ACCOUNTS = 1000;    // Memory limit
    static constexpr uint32 MAX_PENDING_REQUESTS = 5000;   // Queue limit

    // Non-copyable
    BotCharacterSelector(BotCharacterSelector const&) = delete;
    BotCharacterSelector& operator=(BotCharacterSelector const&) = delete;
};

} // namespace Playerbot