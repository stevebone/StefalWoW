/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file IProtectionProvider.h
 * @brief Interface for bot protection status providers
 *
 * This interface defines the contract for components that can provide
 * protection information for bots. The BotProtectionRegistry aggregates
 * information from multiple providers to build complete protection status.
 *
 * Providers include:
 * - Guild system (guild membership)
 * - Social system (friend lists)
 * - Group system (party membership)
 * - Mail system (pending mail)
 * - Auction system (active auctions)
 *
 * This abstraction allows the protection registry to query each system
 * without tight coupling, and enables easy extension for new protection
 * sources in the future.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "ProtectionReason.h"
#include <functional>
#include <mutex>
#include <vector>

namespace Playerbot
{

/**
 * @brief Information about a single protection source
 *
 * Returned by IProtectionProvider::GetProtectionInfo() to describe
 * what protection (if any) a provider grants to a specific bot.
 */
struct ProtectionInfo
{
    /**
     * @brief The protection reason this provider grants (if any)
     */
    ProtectionReason reason = ProtectionReason::None;

    /**
     * @brief Whether the protection is currently active
     */
    bool isActive = false;

    /**
     * @brief Additional protection score bonus from this provider
     *
     * Beyond the base weight for the protection reason, providers
     * can add bonus score (e.g., +10 per friend on friend list).
     */
    float scoreBonus = 0.0f;

    /**
     * @brief Human-readable description of the protection
     *
     * For logging and debugging (e.g., "Member of guild 'Epic Raiders'")
     */
    std::string description;

    /**
     * @brief Check if this provider grants any protection
     */
    bool GrantsProtection() const noexcept
    {
        return isActive && reason != ProtectionReason::None;
    }
};

/**
 * @brief Interface for protection status providers
 *
 * Each game system that can protect bots (guilds, friends, mail, etc.)
 * implements this interface to report protection status to the registry.
 *
 * Providers are queried:
 * 1. On startup to build initial protection cache
 * 2. Periodically to refresh protection status
 * 3. On-demand when protection events occur
 */
class TC_GAME_API IProtectionProvider
{
public:
    virtual ~IProtectionProvider() = default;

    /**
     * @brief Get the protection reason type this provider handles
     * @return The ProtectionReason this provider is responsible for
     */
    virtual ProtectionReason GetProvidedReason() const = 0;

    /**
     * @brief Get the display name of this provider
     * @return Human-readable name (e.g., "Guild Membership")
     */
    virtual std::string_view GetProviderName() const = 0;

    /**
     * @brief Query protection status for a specific bot
     * @param botGuid The bot to query
     * @return Protection information for this bot from this provider
     */
    virtual ProtectionInfo GetProtectionInfo(ObjectGuid botGuid) const = 0;

    /**
     * @brief Query protection status for multiple bots (batch operation)
     * @param botGuids List of bots to query
     * @return Map of bot GUID to protection info
     *
     * Default implementation calls GetProtectionInfo() for each bot.
     * Providers can override for batch-optimized queries.
     */
    virtual std::vector<std::pair<ObjectGuid, ProtectionInfo>> GetProtectionInfoBatch(
        std::vector<ObjectGuid> const& botGuids) const
    {
        std::vector<std::pair<ObjectGuid, ProtectionInfo>> results;
        results.reserve(botGuids.size());
        for (ObjectGuid const& guid : botGuids)
        {
            results.emplace_back(guid, GetProtectionInfo(guid));
        }
        return results;
    }

    /**
     * @brief Get all bots protected by this provider
     * @return List of bot GUIDs with active protection from this provider
     *
     * Used for bulk operations like counting protected bots per bracket.
     */
    virtual std::vector<ObjectGuid> GetAllProtectedBots() const = 0;

    /**
     * @brief Get count of bots protected by this provider
     * @return Number of bots with active protection
     *
     * Default implementation returns size of GetAllProtectedBots().
     * Providers can override for optimized counting.
     */
    virtual uint32 GetProtectedBotCount() const
    {
        return static_cast<uint32>(GetAllProtectedBots().size());
    }

    /**
     * @brief Check if this provider's data is stale and needs refresh
     * @return true if the provider needs to refresh its data
     */
    virtual bool NeedsRefresh() const { return false; }

    /**
     * @brief Refresh the provider's cached data
     *
     * Called periodically or when NeedsRefresh() returns true.
     */
    virtual void Refresh() {}

    /**
     * @brief Register callback for protection status changes
     * @param callback Function to call when protection changes
     *
     * The callback receives:
     * - botGuid: The affected bot
     * - newStatus: Whether bot is now protected (true) or unprotected (false)
     *
     * Providers should call this callback when:
     * - Bot joins/leaves guild
     * - Player adds/removes bot from friends
     * - Bot joins/leaves player group
     * - Mail arrives/is cleared
     * - Auction is created/expires
     */
    using ProtectionChangeCallback = std::function<void(ObjectGuid botGuid, bool nowProtected)>;

    virtual void RegisterChangeCallback(ProtectionChangeCallback callback) = 0;

    /**
     * @brief Unregister all change callbacks
     *
     * Called during shutdown to prevent callbacks to destroyed objects.
     */
    virtual void ClearChangeCallbacks() = 0;
};

/**
 * @brief Base implementation of IProtectionProvider with common functionality
 *
 * Provides default implementations for:
 * - Change callback registration
 * - Batch query (via individual queries)
 * - Count (via GetAllProtectedBots size)
 */
class TC_GAME_API ProtectionProviderBase : public IProtectionProvider
{
public:
    void RegisterChangeCallback(ProtectionChangeCallback callback) override
    {
        std::lock_guard<std::mutex> lock(_callbackMutex);
        _callbacks.push_back(std::move(callback));
    }

    void ClearChangeCallbacks() override
    {
        std::lock_guard<std::mutex> lock(_callbackMutex);
        _callbacks.clear();
    }

protected:
    /**
     * @brief Notify all registered callbacks of a protection change
     * @param botGuid The affected bot
     * @param nowProtected Whether bot is now protected
     */
    void NotifyProtectionChange(ObjectGuid botGuid, bool nowProtected)
    {
        std::lock_guard<std::mutex> lock(_callbackMutex);
        for (auto const& callback : _callbacks)
        {
            if (callback)
            {
                try
                {
                    callback(botGuid, nowProtected);
                }
                catch (std::exception const& e)
                {
                    // Log error but don't propagate - other callbacks should still fire
                    // TC_LOG_ERROR would go here
                    (void)e;
                }
            }
        }
    }

private:
    std::mutex _callbackMutex;
    std::vector<ProtectionChangeCallback> _callbacks;
};

/**
 * @brief Null protection provider for testing
 *
 * Always reports no protection. Used for unit tests.
 */
class TC_GAME_API NullProtectionProvider : public ProtectionProviderBase
{
public:
    ProtectionReason GetProvidedReason() const override
    {
        return ProtectionReason::None;
    }

    std::string_view GetProviderName() const override
    {
        return "NullProvider";
    }

    ProtectionInfo GetProtectionInfo(ObjectGuid /*botGuid*/) const override
    {
        return ProtectionInfo{};
    }

    std::vector<ObjectGuid> GetAllProtectedBots() const override
    {
        return {};
    }
};

} // namespace Playerbot
