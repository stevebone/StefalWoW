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

#include "BotAccountMgr.h"
#include "AccountMgr.h"
#include "BattlenetAccountMgr.h"
#include "Config/PlayerbotConfig.h"
#include "DatabaseEnv.h"
#include "Database/PlayerbotDatabase.h"
#include "Log.h"
#include "Util.h"
#include "Random.h"
#include <sstream>
#include <iomanip>
#include <random>
#include <thread>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace Playerbot {

bool BotAccountMgr::Initialize()
{
    TC_LOG_INFO("module.playerbot.account", "Initializing BotAccountMgr...");

    // Load configuration values from playerbots.conf
    LoadConfigurationValues();

    // Load existing bot accounts from database
    LoadAccountMetadata();

    // Check if automatic account creation is enabled
    if (_autoCreateAccounts.load())
    {
        uint32 required = GetRequiredAccountCount();
        uint32 current = _totalAccounts.load();

        TC_LOG_INFO("module.playerbot.account",
            "Auto account creation enabled: {} required, {} exist",
            required, current);

        // Start account pool refill if needed
    if (current < required)
        {
            TC_LOG_INFO("module.playerbot.account",
                "Creating {} additional bot accounts...", required - current);
            RefillAccountPool();
        }
    }
    else
    {
        TC_LOG_INFO("module.playerbot.account",
            "Auto account creation disabled in configuration");
    }

    TC_LOG_INFO("module.playerbot.account",
        "BotAccountMgr initialized: {} accounts, {} in pool, auto-create: {}",
        _totalAccounts.load(), GetPoolSize(), _autoCreateAccounts.load());

    return true;
}

void BotAccountMgr::Shutdown()
{
    TC_LOG_INFO("module.playerbot.account", "Shutting down BotAccountMgr...");

    // Save all account metadata
    for (auto const& [id, info] : _accounts)
    {
        StoreAccountMetadata(info);
    }

    TC_LOG_INFO("module.playerbot.account",
        " BotAccountMgr shutdown: {} accounts saved", _accounts.size());
}

void BotAccountMgr::ProcessPendingCallbacks()
{
    // Process pending callbacks from worker threads on main thread
    constexpr uint32 MAX_CALLBACKS_PER_UPDATE = 5;
    uint32 processed = 0;

    while (processed < MAX_CALLBACKS_PER_UPDATE)
    {
        ::std::function<void()> callback;

        // Extract callback from queue
        {
            ::std::lock_guard lock(_callbackMutex);
            if (_pendingCallbacks.empty())
                break;

            callback = ::std::move(_pendingCallbacks.front().callback);
            _pendingCallbacks.pop();
        }

        // Execute callback on main thread
        try
        {
            callback();
            ++processed;
        }
        catch (::std::exception const& e)
        {
            TC_LOG_ERROR("module.playerbot.account",
                "Callback execution failed: {}", e.what());
        }
    }
}

void BotAccountMgr::QueueCallback(::std::function<void()> callback)
{
    ::std::lock_guard lock(_callbackMutex);

    PendingCallback pending;
    pending.callback = ::std::move(callback);
    pending.submitTime = ::std::chrono::steady_clock::now();

    _pendingCallbacks.push(::std::move(pending));
}

void BotAccountMgr::Update(uint32 /*diff*/)
{
    // Process callbacks from worker threads on main thread
    ProcessPendingCallbacks();
}

void BotAccountMgr::LoadConfigurationValues()
{
    TC_LOG_DEBUG("module.playerbot.account", "Loading configuration values...");

    // Load configuration from playerbots.conf using PlayerbotConfig
    _maxBotsTotal.store(::PlayerbotConfig::instance()->GetInt("Playerbot.MaxBots", 1000));
    _autoCreateAccounts.store(::PlayerbotConfig::instance()->GetBool("Playerbot.AutoCreateAccounts", true));
    uint32 configuredAccounts = ::PlayerbotConfig::instance()->GetInt("Playerbot.AccountsToCreate", 50);

    // AUTO-ADJUSTMENT: If MaxBots > 10 * AccountsToCreate, adjust accounts to the right number
    // Formula: accounts = ceil(MaxBots / 10.0) to support 10 characters per account
    uint32 calculatedAccounts = static_cast<uint32>(::std::ceil(_maxBotsTotal.load() / 10.0));

    // Use the maximum of configured and calculated to ensure we have enough accounts
    uint32 requiredAccounts = ::std::max(configuredAccounts, calculatedAccounts);

    _accountsToCreate.store(requiredAccounts);  // Store adjusted value
    _requiredAccounts.store(requiredAccounts);

    // Set target pool size (keep 25% in pool for instant availability)
    _targetPoolSize.store(::std::max(10U, requiredAccounts / 4));

    TC_LOG_ERROR("module.playerbot",
        " BotAccountMgr Config: MaxBotsTotal={}, AutoCreate={}, ConfiguredAccounts={}, CalculatedAccounts={}, RequiredAccounts={}, PoolTarget={}",
        _maxBotsTotal.load(), _autoCreateAccounts.load(), configuredAccounts, calculatedAccounts,
        _requiredAccounts.load(), _targetPoolSize.load());
}

void BotAccountMgr::UpdateConfiguration()
{
    LoadConfigurationValues();

    // If auto-create is enabled and we need more accounts, start creating them
    if (_autoCreateAccounts.load())
    {
        uint32 required = GetRequiredAccountCount();
        uint32 current = _totalAccounts.load();

        if (current < required)
        {
            TC_LOG_INFO("module.playerbot.account",
                "Configuration changed: creating {} additional accounts", required - current);
            RefillAccountPool();
        }
    }
}

uint32 BotAccountMgr::GetRequiredAccountCount() const
{
    return _requiredAccounts.load();
}

bool BotAccountMgr::EnsureAccountCapacity(uint32 additionalNeeded)
{
    if (additionalNeeded == 0)
        return true;

    // Check if auto-creation is enabled
    if (!_autoCreateAccounts.load())
    {
        TC_LOG_WARN("module.playerbot.account",
            "EnsureAccountCapacity: Cannot ensure capacity - auto-creation is disabled");
        return false;
    }

    // Calculate how many more accounts we need beyond current limit
    uint32 currentTotal = _totalAccounts.load();
    uint32 currentLimit = _requiredAccounts.load();
    uint32 neededTotal = currentTotal + additionalNeeded;

    // If we already have enough capacity, we're good
    if (neededTotal <= currentLimit)
    {
        TC_LOG_DEBUG("module.playerbot.account",
            "EnsureAccountCapacity: Already have capacity ({}/{} limit, need {})",
            currentTotal, currentLimit, additionalNeeded);
        return true;
    }

    // Increase the limit to accommodate the new accounts
    // Add 20% buffer to avoid frequent increases
    uint32 newLimit = static_cast<uint32>(neededTotal * 1.2);

    // Use compare_exchange to ensure thread-safe update (only increase, never decrease)
    uint32 expected = currentLimit;
    while (newLimit > expected)
    {
        if (_requiredAccounts.compare_exchange_weak(expected, newLimit))
        {
            TC_LOG_INFO("module.playerbot.account",
                "EnsureAccountCapacity: Increased account limit from {} to {} (need {} additional)",
                currentLimit, newLimit, additionalNeeded);
            return true;
        }
        // If someone else increased it, check if it's now enough
        if (expected >= neededTotal)
        {
            TC_LOG_DEBUG("module.playerbot.account",
                "EnsureAccountCapacity: Limit already increased to {} by another thread",
                expected);
            return true;
        }
    }

    return true;
}

uint32 BotAccountMgr::CreateBotAccount(::std::string const& requestedEmail)
{
    // Check if auto-creation is disabled
    if (!_autoCreateAccounts.load())
    {
        TC_LOG_WARN("module.playerbot.account",
            "Account creation requested but Playerbot.AutoCreateAccounts is disabled");
        return 0;
    }

    // Check account limit
    uint32 required = GetRequiredAccountCount();
    if (_totalAccounts.load() >= required)
    {
        TC_LOG_ERROR("module.playerbot.account",
            "Cannot create account: limit {} reached", required);
        return 0;
    }

    auto startTime = ::std::chrono::high_resolution_clock::now();

    // Generate email and password with retry logic for duplicates
    ::std::string email;
    ::std::string password = GenerateSecurePassword();
    ::std::string gameAccountName;
    AccountOpResult result;

    // Try up to 100 different email addresses to find a unique one
    const uint32 maxRetries = 100;
    uint32 baseCounter = _emailCounter.load(); // Get current counter value
    for (uint32 attempt = 0; attempt < maxRetries; ++attempt)
    {
        if (requestedEmail.empty())
        {
            // Generate email with base counter + attempt number
            ::std::ostringstream emailStream;
            emailStream << "bot" << ::std::setfill('0') << ::std::setw(6) << (baseCounter + attempt + 1)
                       << "@" << _emailDomain;
            email = emailStream.str();
        }
        else
        {
            email = requestedEmail;
        }

        // Create BattleNet account using Trinity's system
        result = Battlenet::AccountMgr::CreateBattlenetAccount(email, password, true, &gameAccountName);

        if (result == AccountOpResult::AOR_OK)
        {
            // Success! Update the email counter to reflect this account creation
    if (requestedEmail.empty())
            {
                _emailCounter.store(baseCounter + attempt + 1);
            }
            break;
        }
        else if (result == AccountOpResult::AOR_NAME_ALREADY_EXIST)
        {
            // Account already exists, try next email (only if we're auto-generating)
    if (!requestedEmail.empty())
            {
                // User requested specific email that already exists
                TC_LOG_ERROR("module.playerbot.account",
                    "Requested email {} already exists", requestedEmail);
                return 0;
            }

            TC_LOG_DEBUG("module.playerbot.account",
                "Email {} already exists, trying next number (attempt {}/{})",
                email, attempt + 1, maxRetries);
            continue;
        }
        else
        {
            // Other error (password too long, etc.)
            TC_LOG_ERROR("module.playerbot.account",
                "Failed to create BattleNet account for email: {}, result: {}", email, static_cast<uint32>(result));
            return 0;
        }
    }

    if (result != AccountOpResult::AOR_OK)
    {
        TC_LOG_ERROR("module.playerbot.account",
            "Failed to create unique BattleNet account after {} attempts, last result: {}",
            maxRetries, static_cast<uint32>(result));
        return 0;
    }

    // Get the created game account ID
    uint32 legacyAccountId = AccountMgr::GetId(gameAccountName);
    if (legacyAccountId == 0)
    {
        TC_LOG_ERROR("module.playerbot.account",
            "Failed to retrieve legacy account ID for game account: {}", gameAccountName);
        return 0;
    }

    // For now, use legacyAccountId as the primary identifier since we can't easily get BNet account ID
    uint32 bnetAccountId = legacyAccountId;

    // Create account info
    BotAccountInfo info;
    info.bnetAccountId = bnetAccountId;
    info.legacyAccountId = legacyAccountId;
    info.email = email;
    info.passwordHash = password; // Store hash in production
    info.createdAt = ::std::chrono::system_clock::now();
    info.characterCount = 0;
    info.isActive = false;
    info.isInPool = false;

    // Store in memory
    {
        ::std::unique_lock lock(_accountsMutex);
        _accounts[bnetAccountId] = info;
    }

    // Store in database asynchronously
    StoreAccountMetadata(info);

    // Update statistics
    _totalAccounts.fetch_add(1);

    auto endTime = ::std::chrono::high_resolution_clock::now();
    uint32 creationTimeUs = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        endTime - startTime).count();

    TC_LOG_DEBUG("module.playerbot.account",
        "Created bot account: BNet {} (Legacy {}), Email: {}, Time: {}μs",
        bnetAccountId, legacyAccountId, email, creationTimeUs);

    return bnetAccountId;
}

void BotAccountMgr::CreateBotAccountsBatch(uint32 count,
    ::std::function<void(::std::vector<uint32>)> callback)
{
    TC_LOG_INFO("module.playerbot.account", "Creating batch of {} bot accounts with throttling (50/sec)...", count);

    // Execute in a separate thread to avoid blocking
    ::std::thread([this, count, callback]()
    {
        ::std::vector<uint32> createdAccounts;
        createdAccounts.reserve(count);

        for (uint32 i = 0; i < count; ++i)
        {
            // SEQUENTIAL CREATION WITH VALIDATION
    if (uint32 accountId = CreateBotAccount())
            {
                // VALIDATION: Verify account exists in database before adding to list (prepared statement)
                LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BNET_ACCOUNT_EXISTS);
                stmt->setUInt32(0, accountId);
                PreparedQueryResult result = LoginDatabase.Query(stmt);

                if (result)
                {
                    createdAccounts.push_back(accountId);
                    TC_LOG_DEBUG("module.playerbot.account",
                        " Account {} created and validated ({}/{})", accountId, i + 1, count);
                }
                else
                {
                    TC_LOG_ERROR("module.playerbot.account",
                        " Account {} creation reported success but not found in database!", accountId);
                }
            }
            else
            {
                // If creation fails, we might have hit the limit
                TC_LOG_WARN("module.playerbot.account",
                    "Account creation failed at {}/{}, stopping batch", i + 1, count);
                break;
            }

            // THROTTLING: 50 accounts per second = 20ms delay between accounts
            ::std::this_thread::sleep_for(::std::chrono::milliseconds(20));
        }

        TC_LOG_INFO("module.playerbot.account",
            " Batch creation complete: {}/{} accounts created and validated",
            createdAccounts.size(), count);

        if (callback)
        {
            // Queue callback for main thread execution to prevent deadlocks
            QueueCallback([callback, createdAccounts]()
            {
                callback(createdAccounts);
            });
        }
    }).detach();
}

void BotAccountMgr::RefillAccountPool()
{
    // Check if refill already in progress
    uint32 expected = 0;
    if (!_poolRefillInProgress.compare_exchange_strong(expected, 1))
    {
        return; // Already refilling
    }

    uint32 targetPoolSize = _targetPoolSize.load();

    TC_LOG_INFO("module.playerbot.account",
        "Refilling account pool to {} accounts...", targetPoolSize);

    // Calculate how many accounts to create
    uint32 currentSize = GetPoolSize();
    uint32 requiredTotal = GetRequiredAccountCount();
    uint32 existingTotal = _totalAccounts.load();

    // Accounts needed for total requirement
    uint32 totalToCreate = (existingTotal < requiredTotal) ? (requiredTotal - existingTotal) : 0;

    // Additional accounts needed for pool
    uint32 poolToCreate = (currentSize < targetPoolSize) ? (targetPoolSize - currentSize) : 0;

    uint32 toCreate = ::std::max(totalToCreate, poolToCreate);

    if (toCreate > 0 && _autoCreateAccounts.load())
    {
        CreateBotAccountsBatch(toCreate,
            [this, targetPoolSize](::std::vector<uint32> const& accounts)
            {
                // Add to pool
                {
                    ::std::lock_guard lock(_poolMutex);
                    for (uint32 accountId : accounts)
                    {
                        _accountPool.push(accountId);

                        // Mark as in pool
                        auto it = _accounts.find(accountId);
                        if (it != _accounts.end())
                        {
                            it->second.isInPool = true;
                        }
                    }
                }

                TC_LOG_INFO("module.playerbot.account",
                    " Account pool refilled: {} accounts now available",
                    GetPoolSize());

                _poolRefillInProgress.store(0);
            });
    }
    else
    {
        if (!_autoCreateAccounts.load())
        {
            TC_LOG_INFO("module.playerbot.account",
                "Account pool refill skipped: auto-creation disabled");
        }
        _poolRefillInProgress.store(0);
    }
}

uint32 BotAccountMgr::AcquireAccount()
{
    // Try to get from pool first
    {
        ::std::lock_guard lock(_poolMutex);
        if (!_accountPool.empty())
        {
            uint32 accountId = _accountPool.front();
            _accountPool.pop();

            // Mark as active
            auto it = _accounts.find(accountId);
            if (it != _accounts.end())
            {
                it->second.isActive = true;
                it->second.isInPool = false;
            }

            _activeAccounts.fetch_add(1);

            TC_LOG_DEBUG("module.playerbot.account",
                "Acquired account {} from pool", accountId);

            // Trigger pool refill if getting low
    if (_accountPool.size() < _targetPoolSize.load() / 2)
            {
                RefillAccountPool();
            }

            return accountId;
        }
    }

    // Pool empty, create new account if auto-creation is enabled
    if (_autoCreateAccounts.load())
    {
        TC_LOG_DEBUG("module.playerbot.account",
            "Account pool empty, creating new account...");

        uint32 newAccountId = CreateBotAccount();

        if (newAccountId > 0)
        {
            // Mark as active
            auto it = _accounts.find(newAccountId);
            if (it != _accounts.end())
            {
                it->second.isActive = true;
            }
            _activeAccounts.fetch_add(1);
        }

        return newAccountId;
    }
    else
    {
        TC_LOG_WARN("module.playerbot.account",
            "Cannot acquire account: pool empty and auto-creation disabled");
        return 0;
    }
}

void BotAccountMgr::ReleaseAccount(uint32 bnetAccountId)
{
    ::std::lock_guard lock(_poolMutex);

    // Check if account exists
    auto it = _accounts.find(bnetAccountId);
    if (it == _accounts.end())
    {
        TC_LOG_WARN("module.playerbot.account",
            "Cannot release unknown account {}", bnetAccountId);
        return;
    }

    uint32 targetPoolSize = _targetPoolSize.load();

    // Return to pool if under limit
    if (_accountPool.size() < targetPoolSize * 2) // Allow some overflow
    {
        _accountPool.push(bnetAccountId);
        it->second.isActive = false;
        it->second.isInPool = true;
        _activeAccounts.fetch_sub(1);

        TC_LOG_DEBUG("module.playerbot.account",
            "Released account {} back to pool", bnetAccountId);
    }
    else
    {
        // Pool full, just mark as inactive
        it->second.isActive = false;
        it->second.isInPool = false;
        _activeAccounts.fetch_sub(1);

        TC_LOG_DEBUG("module.playerbot.account",
            "Released account {} (pool full)", bnetAccountId);
    }
}

bool BotAccountMgr::MarkAccountInUse(uint32 bnetAccountId)
{
    ::std::lock_guard lock(_poolMutex);

    // Check if account exists
    auto it = _accounts.find(bnetAccountId);
    if (it == _accounts.end())
    {
        // Account not tracked by BotAccountMgr - this is OK for pool bots
        // that were created before BotAccountMgr was introduced
        TC_LOG_DEBUG("module.playerbot.account",
            "MarkAccountInUse: Account {} not in BotAccountMgr tracking (may be legacy pool account)",
            bnetAccountId);
        return false;
    }

    // Check if already active
    if (it->second.isActive)
    {
        TC_LOG_DEBUG("module.playerbot.account",
            "MarkAccountInUse: Account {} already marked as active", bnetAccountId);
        return true;
    }

    // Remove from pool if it's there
    // We need to rebuild the queue without this account
    if (it->second.isInPool)
    {
        ::std::queue<uint32> newPool;
        while (!_accountPool.empty())
        {
            uint32 pooledId = _accountPool.front();
            _accountPool.pop();
            if (pooledId != bnetAccountId)
            {
                newPool.push(pooledId);
            }
        }
        _accountPool = ::std::move(newPool);
        it->second.isInPool = false;
    }

    // Mark as active
    it->second.isActive = true;
    _activeAccounts.fetch_add(1);

    TC_LOG_DEBUG("module.playerbot.account",
        "MarkAccountInUse: Account {} marked as in-use (pool bot)", bnetAccountId);

    return true;
}

uint32 BotAccountMgr::GetPoolSize() const
{
    ::std::lock_guard lock(_poolMutex);
    return _accountPool.size();
}

bool BotAccountMgr::CanCreateCharacter(uint32 bnetAccountId) const
{
    ::std::lock_guard lock(_accountsMutex);

    auto it = _accounts.find(bnetAccountId);
    if (it == _accounts.end())
    {
        return false;
    }

    return it->second.characterCount < _maxCharactersPerAccount.load();
}

void BotAccountMgr::UpdateCharacterCount(uint32 bnetAccountId, int8 delta)
{
    ::std::unique_lock lock(_accountsMutex);

    auto it = _accounts.find(bnetAccountId);
    if (it != _accounts.end())
    {
        int16 newCount = it->second.characterCount + delta;
        it->second.characterCount = ::std::max(0, ::std::min(static_cast<int>(
            _maxCharactersPerAccount.load()), static_cast<int>(newCount)));

        TC_LOG_DEBUG("module.playerbot.account",
            "Updated character count for account {}: {} characters",
            bnetAccountId, it->second.characterCount);

        // Update metadata in database
        StoreAccountMetadata(it->second);
    }
}

BotAccountMgr::BotAccountInfo const* BotAccountMgr::GetAccountInfo(uint32 bnetAccountId) const
{
    ::std::lock_guard lock(_accountsMutex);

    auto it = _accounts.find(bnetAccountId);
    return (it != _accounts.end()) ? &it->second : nullptr;
}

void BotAccountMgr::DeleteBotAccount(uint32 bnetAccountId,
    ::std::function<void(bool success)> callback)
{
    TC_LOG_INFO("module.playerbot.account",
        "Deleting bot account {}...", bnetAccountId);

    // For our simplified implementation, the bnetAccountId is actually the legacy account ID
    uint32 legacyAccountId = bnetAccountId;

    // Delete the account (this will delete characters too)
    AccountOpResult result = AccountMgr::DeleteAccount(legacyAccountId);

    if (result == AccountOpResult::AOR_OK)
    {
        // Remove from memory
        {
            ::std::unique_lock lock(_accountsMutex);
            _accounts.erase(bnetAccountId);
        }

        _totalAccounts.fetch_sub(1);

        TC_LOG_INFO("module.playerbot.account",
            " Deleted bot account {}", bnetAccountId);

        if (callback) callback(true);
    }
    else
    {
        TC_LOG_ERROR("module.playerbot.account",
            "Failed to delete account {}: {}", bnetAccountId, static_cast<uint32>(result));
        if (callback) callback(false);
    }
}

void BotAccountMgr::DeleteAllBotAccounts(::std::function<void(uint32 deleted)> callback)
{
    TC_LOG_WARN("module.playerbot.account", "Deleting ALL bot accounts...");

    ::std::vector<uint32> accountIds;
    {
        ::std::lock_guard lock(_accountsMutex);
        accountIds.reserve(_accounts.size());
        for (auto const& [id, info] : _accounts)
        {
            accountIds.push_back(id);
        }
    }

    if (accountIds.empty())
    {
        TC_LOG_INFO("module.playerbot.account", "No bot accounts to delete");
        if (callback) callback(0);
        return;
    }

    // Delete accounts in a separate thread
    ::std::thread([this, accountIds = ::std::move(accountIds), callback]()
    {
        uint32 deleted = 0;
        for (uint32 accountId : accountIds)
        {
            bool success = false;
            DeleteBotAccount(accountId, [&success](bool result) { success = result; });

            if (success)
            {
                ++deleted;
            }

            // Small delay between deletions
            ::std::this_thread::sleep_for(::std::chrono::milliseconds(50));
        }

        TC_LOG_WARN("module.playerbot.account",
            " Deleted {}/{} bot accounts", deleted, accountIds.size());

        if (callback) callback(deleted);
    }).detach();
}

::std::string BotAccountMgr::GenerateUniqueEmail()
{
    uint32 counter = _emailCounter.fetch_add(1);

    ::std::ostringstream email;
    email << "bot" << ::std::setfill('0') << ::std::setw(6) << counter
          << "@" << _emailDomain;

    return email.str();
}

::std::string BotAccountMgr::GenerateSecurePassword()
{
    // Generate random secure password
    static const char charset[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "!@#$%^&*";

    ::std::string password;
    password.reserve(16);

    // Use TrinityCore's thread-safe random number generator
    for (int i = 0; i < 16; ++i)
    {
        uint32 randomIndex = urand(0, sizeof(charset) - 2);
        password += charset[randomIndex];
    }

    return password;
}

void BotAccountMgr::StoreAccountMetadata(BotAccountInfo const& info)
{
    // Store bot account metadata in playerbots database
    // NOTE: This uses direct SQL execution. Should be converted to prepared statements
    // when PlayerbotDatabase prepared statement infrastructure is fully implemented.

    try
    {
        ::std::ostringstream ss;
        ss << "INSERT INTO bot_account_metadata "
           << "(account_id, bnet_account_id, email, character_count, expansion, locale, last_ip, join_date, total_time_played, notes) "
           << "VALUES ("
           << info.legacyAccountId << ", "
           << info.bnetAccountId << ", "
           << "'" << info.email << "', "
           << info.characterCount << ", "
           << "0, "  // expansion (default 0)
           << "'enUS', "  // locale (default enUS)
           << "NULL, "  // last_ip
           << "NOW(), "  // join_date
           << "0, "  // total_time_played
           << "NULL"  // notes
           << ") "
           << "ON DUPLICATE KEY UPDATE "
           << "email = VALUES(email), "
           << "character_count = VALUES(character_count), "
           << "updated_at = NOW()";

        if (sPlayerbotDatabase->Execute(ss.str()))
        {
            TC_LOG_DEBUG("module.playerbot.account",
                "Stored metadata for account {}: email={}, characters={}",
                info.bnetAccountId, info.email, info.characterCount);
        }
        else
        {
            TC_LOG_ERROR("module.playerbot.account",
                "Failed to store metadata for account {}: database execution failed",
                info.bnetAccountId);
        }
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.account",
            "Exception while storing account metadata for account {}: {}",
            info.bnetAccountId, e.what());
    }
}

void BotAccountMgr::LoadAccountMetadata()
{
    TC_LOG_INFO("module.playerbot.account", "Loading bot account metadata...");

    // Query existing BattleNet accounts to find bot accounts
    // Bot accounts are identified by email patterns in battlenet_accounts table
    // Based on investigation: bot accounts have emails like "1#1@playerbot.local", "2#1@playerbot.local", etc.

    uint32 loadedAccounts = 0;
    uint32 highestBotNumber = 0;

    try
    {
        // Query BattleNet account table for existing bot accounts (prepared statement)
        // Look for emails with pattern like "#@playerbot.local" or "bot%@playerbot.local"
        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BOT_ACCOUNTS_ALL);
        PreparedQueryResult result = LoginDatabase.Query(stmt);

        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                uint32 bnetAccountId = fields[0].GetUInt32();
                ::std::string email = fields[1].GetString();
                uint32 legacyAccountId = fields[2].GetUInt32();

                // Try to extract bot number from different email patterns
                uint32 botNumber = 0;
                bool isValidBotAccount = false;

                // Convert email to lowercase for case-insensitive matching
                ::std::string emailLower = email;
                ::std::transform(emailLower.begin(), emailLower.end(), emailLower.begin(), ::tolower);

                // Pattern 1: "bot######@playerbot.local"
    if (emailLower.find("bot") == 0 && emailLower.find("@playerbot.local") != ::std::string::npos)
                {
                    size_t atPos = emailLower.find("@");
                    ::std::string numberStr = emailLower.substr(3, atPos - 3);
                    try
                    {
                        botNumber = ::std::stoul(numberStr);
                        isValidBotAccount = true;
                    }
                    catch (...) { /* ignore */ }
                }
                // Pattern 2: "X#1@playerbot.local" or similar patterns
                else if (emailLower.find("#") != ::std::string::npos && emailLower.find("@") != ::std::string::npos)
                {
                    size_t hashPos = emailLower.find("#");
                    size_t atPos = emailLower.find("@");

                    if (hashPos < atPos)
                    {
                        ::std::string beforeHash = emailLower.substr(0, hashPos);
                        try
                        {
                            botNumber = ::std::stoul(beforeHash);
                            isValidBotAccount = true;
                        }
                        catch (...) { /* ignore */ }
                    }
                }
                // Pattern 3: Any email containing "playerbot" domain
                else if (emailLower.find("@playerbot.local") != ::std::string::npos)
                {
                    // For accounts with playerbot domain but unknown pattern,
                    // assign sequential numbers starting from current counter
                    botNumber = highestBotNumber + loadedAccounts + 1;
                    isValidBotAccount = true;
                }

                if (isValidBotAccount)
                {
                    highestBotNumber = ::std::max(highestBotNumber, botNumber);

                    // Create basic account info
                    BotAccountInfo info;
                    info.bnetAccountId = bnetAccountId;
                    info.legacyAccountId = legacyAccountId;
                    info.email = email;
                    info.characterCount = 0; // Could query this later
                    info.isActive = false;
                    info.isInPool = false;
                    info.createdAt = ::std::chrono::system_clock::now(); // Placeholder

                    _accounts[bnetAccountId] = info;
                    loadedAccounts++;

                    // CRITICAL FIX: Add loaded accounts to the account pool so they can be acquired
                    {
                        ::std::lock_guard lock(_poolMutex);
                        _accountPool.push(legacyAccountId);  // Use legacy account ID for the pool
                        _accounts[bnetAccountId].isInPool = true;  // Mark as in pool
                    }

                    TC_LOG_DEBUG("module.playerbot.account",
                        "Loaded bot account: BNet {}, Email: {}, Legacy: {}, Bot#: {}",
                        bnetAccountId, email, legacyAccountId, botNumber);
                }
            } while (result->NextRow());
        }

        // Set the email counter to start after the highest existing bot number
        _emailCounter.store(highestBotNumber + 1);
        _totalAccounts.store(loadedAccounts);

        TC_LOG_INFO("module.playerbot.account",
            "Loaded {} bot account metadata entries, highest bot number: {}, next counter: {}",
            loadedAccounts, highestBotNumber, _emailCounter.load());
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("module.playerbot.account",
            "Failed to load bot account metadata: {}", e.what());

        // Fallback: Set counter to 1 if we can't load existing accounts
        _emailCounter.store(1);
        _totalAccounts.store(0);
    }
}

} // namespace Playerbot