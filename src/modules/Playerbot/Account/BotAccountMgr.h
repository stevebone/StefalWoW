/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include <memory>
#include <vector>
#include <queue>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <chrono>
#include <functional>
#include <parallel_hashmap/phmap.h>

namespace Playerbot {

/**
 * Bot Account Management System
 *
 * Implements IBotAccountMgr for dependency injection compatibility.
 *
 * Features:
 * - Automated BattleNet account creation based on configuration
 * - Account pooling for instant availability
 * - Integration with enterprise BotSessionMgr
 * - Asynchronous database operations via BotDatabasePool
 * - Account limit enforcement from playerbots.conf
 *
 * Configuration Variables:
 * - Playerbot.MaxBotsTotal: Determines accounts needed (MaxBotsTotal / 10)
 * - Playerbot.AutoCreateAccounts: Enables/disables automatic creation
 * - Playerbot.AccountsToCreate: Override for calculated account number
 */
class TC_GAME_API BotAccountMgr final
{
public:
    // Singleton pattern
    static BotAccountMgr* instance()
    {
        static BotAccountMgr instance;
        return &instance;
    }

    // BotAccountInfo structure
    struct BotAccountInfo
    {
        uint32 bnetAccountId{0};
        uint32 legacyAccountId{0};
        ::std::string email;
        ::std::string passwordHash;
        ::std::chrono::system_clock::time_point createdAt;
        uint32 characterCount{0};
        bool isActive{false};
        bool isInPool{false};
    };

    // IBotAccountMgr interface implementation
    bool Initialize();
    void Shutdown();

    // Update method (call from main thread)
    void Update(uint32 diff);

    // Thread-safe callback processing (call from main thread)
    void ProcessPendingCallbacks();

    // === ACCOUNT CREATION ===

    /**
     * Create a new bot account
     * @param requestedEmail Optional specific email, auto-generated if empty
     * @return BattleNet account ID, 0 on failure
     */
    uint32 CreateBotAccount(::std::string const& requestedEmail = "");

    /**
     * Batch create multiple accounts
     * @param count Number of accounts to create
     * @param callback Async callback with created account IDs
     */
    void CreateBotAccountsBatch(uint32 count,
        ::std::function<void(::std::vector<uint32>)> callback);

    // === ACCOUNT POOL MANAGEMENT ===

    /**
     * Pre-create accounts for instant availability
     * Uses configuration to determine target pool size
     */
    void RefillAccountPool();

    /**
     * Get account from pool or create new
     * @return Account ID from pool or newly created
     */
    uint32 AcquireAccount();

    /**
     * Return account to pool when bot logs out
     */
    void ReleaseAccount(uint32 bnetAccountId);

    /**
     * Mark an account as in-use without acquiring it from the pool
     * Used when loading pool bots from database to prevent their accounts
     * from being given to other spawn requests
     * @param bnetAccountId Account to mark as in-use
     * @return true if account was found and marked
     */
    bool MarkAccountInUse(uint32 bnetAccountId);

    // === ACCOUNT QUERIES ===

    BotAccountInfo const* GetAccountInfo(uint32 bnetAccountId) const;
    uint32 GetTotalAccountCount() const { return _totalAccounts.load(); }
    uint32 GetTotalBotAccounts() const { return _totalAccounts.load(); }
    uint32 GetActiveAccountCount() const { return _activeAccounts.load(); }
    uint32 GetPoolSize() const;

    // === ACCOUNT DELETION ===

    /**
     * Delete bot account and all associated characters
     * @param bnetAccountId BattleNet account to delete
     * @param callback Async completion callback
     */
    void DeleteBotAccount(uint32 bnetAccountId,
        ::std::function<void(bool success)> callback = nullptr);

    /**
     * Delete all bot accounts (cleanup)
     */
    void DeleteAllBotAccounts(::std::function<void(uint32 deleted)> callback = nullptr);

    // === CHARACTER LIMIT ENFORCEMENT ===

    /**
     * Check if account can create more characters
     * @param bnetAccountId Account to check
     * @return true if under 10 character limit
     */
    bool CanCreateCharacter(uint32 bnetAccountId) const;

    /**
     * Update character count for account
     */
    void UpdateCharacterCount(uint32 bnetAccountId, int8 delta);

    // === CONFIGURATION MANAGEMENT ===

    /**
     * Update configuration from playerbots.conf
     * Called automatically during initialization and reload
     */
    void UpdateConfiguration();

    /**
     * Get calculated number of accounts needed
     * Based on Playerbot.MaxBotsTotal / 10 or Playerbot.AccountsToCreate
     */
    uint32 GetRequiredAccountCount() const;

    /**
     * Check if automatic account creation is enabled
     */
    bool IsAutoCreateEnabled() const { return _autoCreateAccounts; }

    /**
     * Ensure we have capacity to create additional accounts
     * Dynamically increases _requiredAccounts limit if needed
     * @param additionalNeeded Number of additional accounts we need to create
     * @return true if capacity is now available
     */
    bool EnsureAccountCapacity(uint32 additionalNeeded);

private:
    BotAccountMgr() = default;
    ~BotAccountMgr() = default;
    BotAccountMgr(BotAccountMgr const&) = delete;
    BotAccountMgr& operator=(BotAccountMgr const&) = delete;

    // === INTERNAL METHODS ===

    ::std::string GenerateUniqueEmail();
    ::std::string GenerateSecurePassword();
    void StoreAccountMetadata(BotAccountInfo const& info);
    void LoadAccountMetadata();
    void ProcessAccountPoolRefill();

    // Configuration loading
    void LoadConfigurationValues();

    // Thread-safe callback queuing
    void QueueCallback(::std::function<void()> callback);

    // === DATA MEMBERS ===

    // Account storage with parallel hashmap for performance
    // DEADLOCK FIX #18: Changed mutex type from std::shared_mutex to std::recursive_mutex
    // ROOT CAUSE: phmap::parallel_flat_hash_map uses this mutex type internally
    // When bots access account info during update, the hashmap's internal std::shared_mutex
    // throws "resource deadlock would occur" on recursive lock attempts
    using AccountMap = phmap::parallel_flat_hash_map<
        uint32,                     // BattleNet account ID
        BotAccountInfo,
        ::std::hash<uint32>,
        ::std::equal_to<>,
        ::std::allocator<::std::pair<uint32, BotAccountInfo>>,
        4,                          // 4 submaps for concurrency
        ::std::recursive_mutex        // CHANGED: ::std::shared_mutex -> ::std::recursive_mutex
    >;

    AccountMap _accounts;

    // Pre-created account pool for instant availability
    ::std::queue<uint32> _accountPool;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _poolMutex;

    // Email generation
    ::std::atomic<uint32> _emailCounter{1};
    ::std::string _emailDomain{"playerbot.local"};

    // Statistics
    ::std::atomic<uint32> _totalAccounts{0};
    ::std::atomic<uint32> _activeAccounts{0};
    ::std::atomic<uint32> _poolRefillInProgress{0};

    // Configuration from playerbots.conf
    ::std::atomic<uint32> _maxBotsTotal{1000};        // Playerbot.MaxBotsTotal
    ::std::atomic<bool> _autoCreateAccounts{false};   // Playerbot.AutoCreateAccounts
    ::std::atomic<uint32> _accountsToCreate{0};       // Playerbot.AccountsToCreate
    ::std::atomic<uint32> _maxCharactersPerAccount{10}; // Trinity hardcoded limit

    // Calculated values
    ::std::atomic<uint32> _requiredAccounts{0};       // Calculated from config
    ::std::atomic<uint32> _targetPoolSize{50};        // Pool size target

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _accountsMutex;

    // Callback processing for thread-safe operations
    struct PendingCallback
    {
        ::std::function<void()> callback;
        ::std::chrono::steady_clock::time_point submitTime;
    };
    ::std::queue<PendingCallback> _pendingCallbacks;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _callbackMutex;
};

} // namespace Playerbot

#define sBotAccountMgr Playerbot::BotAccountMgr::instance()