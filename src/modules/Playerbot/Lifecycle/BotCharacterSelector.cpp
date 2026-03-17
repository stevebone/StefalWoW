/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotCharacterSelector.h"
#include "Account/BotAccountMgr.h"
#include "Character/BotCharacterMgr.h"
#include "Logging/Log.h"
#include "CharacterCache.h"
#include "AccountMgr.h"
#include "Player.h"
#include <chrono>

namespace Playerbot
{

BotCharacterSelector::BotCharacterSelector()
{
}

bool BotCharacterSelector::Initialize()
{
    TC_LOG_INFO("module.playerbot.character.selector",
        "Initializing BotCharacterSelector for async character selection");

    ResetStats();

    TC_LOG_INFO("module.playerbot.character.selector",
        "BotCharacterSelector initialized successfully");

    return true;
}

void BotCharacterSelector::Shutdown()
{
    TC_LOG_INFO("module.playerbot.character.selector", "Shutting down BotCharacterSelector");

    auto const& stats = GetStats();
    TC_LOG_INFO("module.playerbot.character.selector",
        "Final Selection Statistics - Total: {}, Cache Hit Rate: {:.1f}%, Avg Time: {}μs",
        stats.totalSelections.load(), stats.GetCacheHitRate(), stats.avgSelectionTimeUs.load());

    // Clear cache
    {
        ::std::lock_guard lock(_cacheMutex);
        _characterCache.clear();
    }

    // Clear pending requests
    {
        ::std::lock_guard lock(_requestMutex);
        while (!_pendingRequests.empty())
            _pendingRequests.pop();
    }
}

// === ASYNC CHARACTER SELECTION ===

void BotCharacterSelector::SelectCharacterAsync(SpawnRequest const& request, CharacterCallback callback)
{
    auto start = ::std::chrono::high_resolution_clock::now();

    try
    {
        // Get available accounts for this request
        ::std::vector<uint32> accounts = GetAvailableAccounts(request);
        if (accounts.empty())
        {
            TC_LOG_WARN("module.playerbot.character.selector",
                "No available accounts found for spawn request");
            callback(ObjectGuid::Empty);
            return;
        }

        // Start recursive account processing
        SelectCharacterFromAccounts(::std::move(accounts), 0, request, ::std::move(callback));

        auto end = ::std::chrono::high_resolution_clock::now();
        auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - start);
        RecordSelection(duration.count(), false);
    }
    catch (::std::exception const& ex)
    {
        TC_LOG_ERROR("module.playerbot.character.selector",
            "Exception in SelectCharacterAsync: {}", ex.what());
        callback(ObjectGuid::Empty);
    }
}

void BotCharacterSelector::GetAvailableCharactersAsync(uint32 accountId, SpawnRequest const& request, CharacterListCallback callback)
{
    try
    {
        // Check cache first
        ::std::vector<ObjectGuid> cachedCharacters = GetCachedCharacters(accountId);
        if (!cachedCharacters.empty())
        {
            auto filtered = FilterCharactersByRequest(cachedCharacters, request);
            RecordSelection(0, true); // Cache hit
            callback(::std::move(filtered));
            return;
        }

        // For now, return empty list - would implement async DB query in full version
        callback(::std::vector<ObjectGuid>());
        RecordSelection(0, false); // Cache miss
    }
    catch (::std::exception const& ex)
    {
        TC_LOG_ERROR("module.playerbot.character.selector",
            "Exception in GetAvailableCharactersAsync: {}", ex.what());
        callback(::std::vector<ObjectGuid>());
    }
}

// === BATCH OPERATIONS ===

void BotCharacterSelector::ProcessBatchSelection(::std::vector<SpawnRequest> const& requests,
                                                ::std::function<void(::std::vector<ObjectGuid>)> callback)
{
    // Simplified batch processing - would implement proper async batching in full version
    ::std::vector<ObjectGuid> results;
    results.reserve(requests.size());

    for (auto const& request : requests)
    {
        auto accounts = GetAvailableAccounts(request);
        if (!accounts.empty())
        {
            // For now, just create a dummy GUID - would select actual character in full version
            results.push_back(ObjectGuid::Create<HighGuid::Player>(accounts[0]));
        }
    }

    callback(::std::move(results));
}

// === CHARACTER CREATION ===

ObjectGuid BotCharacterSelector::CreateCharacterForAccount(uint32 accountId, SpawnRequest const& request)
{
    // Simplified character creation - would use BotCharacterMgr in full version
    TC_LOG_DEBUG("module.playerbot.character.selector",
        "Creating character for account {} (simplified)", accountId);

    _stats.charactersCreated.fetch_add(1);

    // Return dummy GUID for now
    return ObjectGuid::Create<HighGuid::Player>(accountId);
}

ObjectGuid BotCharacterSelector::CreateBotCharacter(uint32 accountId)
{
    SpawnRequest defaultRequest;
    return CreateCharacterForAccount(accountId, defaultRequest);
}

// === VALIDATION ===

bool BotCharacterSelector::ValidateCharacter(ObjectGuid characterGuid, SpawnRequest const& request) const
{
    if (characterGuid.IsEmpty())
        return false;

    // Simplified validation - would check actual character data in full version
    return MatchesRequestCriteria(characterGuid, request);
}

uint32 BotCharacterSelector::GetAccountIdFromCharacter(ObjectGuid characterGuid) const
{
    if (CharacterCacheEntry const* characterInfo = sCharacterCache->GetCharacterCacheByGuid(characterGuid))
        return characterInfo->AccountId;

    return 0;
}

// === PERFORMANCE METRICS ===

void BotCharacterSelector::ResetStats()
{
    _stats.totalSelections.store(0);
    _stats.cacheHits.store(0);
    _stats.cacheMisses.store(0);
    _stats.charactersCreated.store(0);
    _stats.avgSelectionTimeUs.store(0);
}

// === PRIVATE IMPLEMENTATION ===

void BotCharacterSelector::SelectCharacterFromAccounts(::std::vector<uint32> accounts, size_t index,
                                                      SpawnRequest const& request, CharacterCallback callback)
{
    if (index >= accounts.size())
    {
        // No more accounts to try
        callback(ObjectGuid::Empty);
        return;
    }

    uint32 accountId = accounts[index];

    // Get characters for this account
    GetAvailableCharactersAsync(accountId, request,
        [this, accounts = ::std::move(accounts), index, request, callback = ::std::move(callback)]
        (::std::vector<ObjectGuid> characters) mutable {
            ProcessAccountCharacters(accounts[index], request, ::std::move(characters),
                [this, accounts = ::std::move(accounts), index, request, callback = ::std::move(callback)]
                (ObjectGuid selectedCharacter) mutable {
                    if (!selectedCharacter.IsEmpty())
                    {
                        callback(selectedCharacter);
                    }
                    else
                    {
                        // Try next account
                        SelectCharacterFromAccounts(::std::move(accounts), index + 1, request, ::std::move(callback));
                    }
                });
        });
}

void BotCharacterSelector::ProcessAccountCharacters(uint32 accountId, SpawnRequest const& request,
                                                   ::std::vector<ObjectGuid> characters, CharacterCallback callback)
{
    if (!characters.empty())
    {
        // Return first suitable character
    for (auto const& characterGuid : characters)
        {
            if (ValidateCharacter(characterGuid, request))
            {
                callback(characterGuid);
                return;
            }
        }
    }

    // No suitable characters found, try to create one
    HandleCharacterCreation(accountId, request, ::std::move(callback));
}

void BotCharacterSelector::HandleCharacterCreation(uint32 accountId, SpawnRequest const& request, CharacterCallback callback)
{
    // Simplified character creation
    ObjectGuid newCharacter = CreateCharacterForAccount(accountId, request);

    if (!newCharacter.IsEmpty())
    {
        // Invalidate cache for this account
        InvalidateCache(accountId);

        TC_LOG_DEBUG("module.playerbot.character.selector",
            "Created new character {} for account {}", newCharacter.ToString(), accountId);
    }

    callback(newCharacter);
}

// === CHARACTER FILTERING ===

::std::vector<ObjectGuid> BotCharacterSelector::FilterCharactersByRequest(::std::vector<ObjectGuid> const& characters,
                                                                        SpawnRequest const& request) const
{
    ::std::vector<ObjectGuid> filtered;
    filtered.reserve(characters.size());

    for (auto const& characterGuid : characters)
    {
        if (MatchesRequestCriteria(characterGuid, request))
        {
            filtered.push_back(characterGuid);
        }
    }

    return filtered;
}

bool BotCharacterSelector::MatchesRequestCriteria(ObjectGuid characterGuid, SpawnRequest const& request) const
{
    // Simplified criteria matching - would check actual character stats in full version
    if (characterGuid.IsEmpty())
        return false;

    // For now, just return true for any valid GUID
    return true;
}

// === ACCOUNT MANAGEMENT ===

::std::vector<uint32> BotCharacterSelector::GetAvailableAccounts(SpawnRequest const& request) const
{
    // Simplified account selection - would use BotAccountMgr in full version
    ::std::vector<uint32> accounts;

    // For now, just return some dummy account IDs
    for (uint32 i = 1; i <= 10; ++i)
    {
        accounts.push_back(i);
    }

    return accounts;
}

uint32 BotCharacterSelector::AcquireSuitableAccount(SpawnRequest const& request) const
{
    auto accounts = GetAvailableAccounts(request);
    return accounts.empty() ? 0 : accounts[0];
}

// === CHARACTER CACHING ===

void BotCharacterSelector::UpdateCharacterCache(uint32 accountId, ::std::vector<ObjectGuid> const& characters)
{
    ::std::lock_guard lock(_cacheMutex);

    if (_characterCache.size() >= MAX_CACHED_ACCOUNTS)
    {
        // Simple LRU - remove oldest entry
        auto oldest = _characterCache.begin();
        for (auto it = _characterCache.begin(); it != _characterCache.end(); ++it)
        {
            if (it->second.lastUpdate < oldest->second.lastUpdate)
                oldest = it;
        }
        _characterCache.erase(oldest);
    }

    CharacterCacheEntry& entry = _characterCache[accountId];
    entry.characters = characters;
    entry.lastUpdate = ::std::chrono::steady_clock::now();
    entry.isValid = true;
}

::std::vector<ObjectGuid> BotCharacterSelector::GetCachedCharacters(uint32 accountId) const
{
    ::std::lock_guard lock(_cacheMutex);

    auto it = _characterCache.find(accountId);
    if (it == _characterCache.end() || !it->second.isValid)
        return {};

    auto now = ::std::chrono::steady_clock::now();
    auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - it->second.lastUpdate);

    if (elapsed.count() > CACHE_VALIDITY_MS)
    {
        // Cache expired
        return {};
    }

    return it->second.characters;
}

void BotCharacterSelector::InvalidateCache(uint32 accountId)
{
    ::std::lock_guard lock(_cacheMutex);

    auto it = _characterCache.find(accountId);
    if (it != _characterCache.end())
    {
        it->second.isValid = false;
    }
}

// === REQUEST QUEUE MANAGEMENT ===

void BotCharacterSelector::QueueRequest(SpawnRequest const& request, CharacterCallback callback)
{
    ::std::lock_guard lock(_requestMutex);

    if (_pendingRequests.size() >= MAX_PENDING_REQUESTS)
    {
        TC_LOG_WARN("module.playerbot.character.selector",
            "Request queue full, dropping request");
        return;
    }

    PendingRequest pending;
    pending.request = request;
    pending.callback = ::std::move(callback);
    pending.timestamp = ::std::chrono::steady_clock::now();

    _pendingRequests.push(::std::move(pending));
}

void BotCharacterSelector::ProcessPendingRequests()
{
    // Simplified queue processing - would implement proper async processing in full version
    ::std::lock_guard lock(_requestMutex);

    while (!_pendingRequests.empty())
    {
        auto request = ::std::move(_pendingRequests.front());
        _pendingRequests.pop();

        // Process immediately for now
        SelectCharacterAsync(request.request, ::std::move(request.callback));
    }
}

// === PERFORMANCE TRACKING ===

void BotCharacterSelector::RecordSelection(uint64 durationMicroseconds, bool cacheHit)
{
    _stats.totalSelections.fetch_add(1);

    if (cacheHit)
        _stats.cacheHits.fetch_add(1);
    else
        _stats.cacheMisses.fetch_add(1);

    // Update average time
    uint64 currentAvg = _stats.avgSelectionTimeUs.load();
    uint32 count = _stats.totalSelections.load();
    uint64 newAvg = (currentAvg * (count - 1) + durationMicroseconds) / count;
    _stats.avgSelectionTimeUs.store(newAvg);
}

} // namespace Playerbot