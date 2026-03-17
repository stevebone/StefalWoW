# Bot Account Management System - Detailed Implementation Plan

## Overview
Automated bot account creation and management using Trinity's BattleNet account system with enterprise performance optimization.

## Architecture

```
BotAccountMgr
├── Account Creation (BattleNet + Legacy)
├── Account Pool Management
├── Account Metadata Storage
├── Account Lifecycle Control
└── Integration with BotSessionMgr
```

## Step-by-Step Implementation

### Variables from configuzation file
From playerbot.conf use variables:
1. Playerbot.MaxBotsTotal : This determines the number of needed Accounts to be created ( Playerbot.MaxBotsTotal / 10 = Number of Bot Accounts )
2. Playerbot.AutoCreateAccounts : Enables / Disables the automatic creation of Bot Accounts
3. Playerbot.AccountsToCreate : If  Playerbot.AccountsToCreate > (Playerbot.MaxBotsTotal / 10) use this value, else use calculated number from 1. 

### Step 1: Create BotAccountMgr Header
**File:** `src/modules/Playerbot/Account/BotAccountMgr.h`
**Time:** 2 hours

```cpp
#pragma once

#include "Define.h"
#include <memory>
#include <vector>
#include <queue>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <chrono>
#include <parallel_hashmap/phmap.h>

namespace Playerbot {

/**
 * Bot Account Management System
 *
 * Features:
 * - Automated BattleNet account creation
 * - Account pooling for instant availability
 * - Integration with enterprise BotSessionMgr
 * - Asynchronous database operations via BotDatabasePool
 * - Account limit enforcement
 */
class TC_GAME_API BotAccountMgr
{
public:
    // Singleton pattern
    static BotAccountMgr* instance()
    {
        static BotAccountMgr instance;
        return &instance;
    }

    struct BotAccountInfo
    {
        uint32 bnetAccountId;        // Primary BattleNet account ID
        uint32 legacyAccountId;      // Linked legacy account for characters
        std::string email;
        std::string passwordHash;
        std::chrono::system_clock::time_point createdAt;
        uint8 characterCount;
        bool isActive;
        bool isInPool;               // Available in pre-created pool
    };

    // Initialization
    bool Initialize();
    void Shutdown();

    // === ACCOUNT CREATION ===

    /**
     * Create a new bot account
     * @param requestedEmail Optional specific email, auto-generated if empty
     * @return BattleNet account ID, 0 on failure
     */
    uint32 CreateBotAccount(std::string const& requestedEmail = "");

    /**
     * Batch create multiple accounts
     * @param count Number of accounts to create
     * @param callback Async callback with created account IDs
     */
    void CreateBotAccountsBatch(uint32 count,
        std::function<void(std::vector<uint32>)> callback);

    // === ACCOUNT POOL MANAGEMENT ===

    /**
     * Pre-create accounts for instant availability
     * @param targetPoolSize Desired pool size
     */
    void RefillAccountPool(uint32 targetPoolSize = 100);

    /**
     * Get account from pool or create new
     * @return Account ID from pool or newly created
     */
    uint32 AcquireAccount();

    /**
     * Return account to pool when bot logs out
     */
    void ReleaseAccount(uint32 bnetAccountId);

    // === ACCOUNT QUERIES ===

    BotAccountInfo const* GetAccountInfo(uint32 bnetAccountId) const;
    uint32 GetTotalAccountCount() const { return _totalAccounts.load(); }
    uint32 GetActiveAccountCount() const { return _activeAccounts.load(); }
    uint32 GetPoolSize() const { return _accountPool.size(); }

    // === ACCOUNT DELETION ===

    /**
     * Delete bot account and all associated characters
     * @param bnetAccountId BattleNet account to delete
     * @param callback Async completion callback
     */
    void DeleteBotAccount(uint32 bnetAccountId,
        std::function<void(bool success)> callback = nullptr);

    /**
     * Delete all bot accounts (cleanup)
     */
    void DeleteAllBotAccounts(std::function<void(uint32 deleted)> callback = nullptr);

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

    // === CONFIGURATION ===

    void SetMaxAccounts(uint32 max) { _maxAccounts = max; }
    void SetAccountPoolSize(uint32 size) { _targetPoolSize = size; }
    void SetEmailDomain(std::string const& domain) { _emailDomain = domain; }

private:
    BotAccountMgr() = default;
    ~BotAccountMgr() = default;
    BotAccountMgr(BotAccountMgr const&) = delete;
    BotAccountMgr& operator=(BotAccountMgr const&) = delete;

    // === INTERNAL METHODS ===

    std::string GenerateUniqueEmail();
    std::string GenerateSecurePassword();
    void StoreAccountMetadata(BotAccountInfo const& info);
    void LoadAccountMetadata();
    void ProcessAccountPoolRefill();

    // === DATA MEMBERS ===

    // Account storage with parallel hashmap for performance
    using AccountMap = phmap::parallel_flat_hash_map<
        uint32,                     // BattleNet account ID
        BotAccountInfo,
        std::hash<uint32>,
        std::equal_to<>,
        std::allocator<std::pair<uint32, BotAccountInfo>>,
        4,                          // 4 submaps for concurrency
        std::shared_mutex
    >;

    AccountMap _accounts;

    // Pre-created account pool for instant availability
    std::queue<uint32> _accountPool;
    mutable std::mutex _poolMutex;

    // Email generation
    std::atomic<uint32> _emailCounter{1};
    std::string _emailDomain{"playerbot.local"};

    // Statistics
    std::atomic<uint32> _totalAccounts{0};
    std::atomic<uint32> _activeAccounts{0};
    std::atomic<uint32> _poolRefillInProgress{0};

    // Configuration
    uint32 _maxAccounts{5000};
    uint32 _targetPoolSize{100};
    uint32 _maxCharactersPerAccount{10}; // Trinity hardcoded limit

    // Thread safety
    mutable std::shared_mutex _accountsMutex;
};

} // namespace Playerbot

#define sBotAccountMgr Playerbot::BotAccountMgr::instance()
```

### Step 2: Implement Core Account Creation
**File:** `src/modules/Playerbot/Account/BotAccountMgr.cpp`
**Time:** 4 hours

```cpp
#include "BotAccountMgr.h"
#include "AccountMgr.h"
#include "BotDatabasePool.h"
#include "BotSessionMgr.h"
#include "Log.h"
#include "Util.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <random>

namespace Playerbot {

bool BotAccountMgr::Initialize()
{
    TC_LOG_INFO("module.playerbot.account", "Initializing BotAccountMgr...");

    // Load existing bot accounts from database
    LoadAccountMetadata();

    // Start account pool refill if needed
    if (_accountPool.size() < _targetPoolSize)
    {
        RefillAccountPool(_targetPoolSize);
    }

    TC_LOG_INFO("module.playerbot.account",
        "✅ BotAccountMgr initialized: {} accounts, {} in pool",
        _totalAccounts.load(), _accountPool.size());

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
        "✅ BotAccountMgr shutdown: {} accounts saved", _accounts.size());
}

uint32 BotAccountMgr::CreateBotAccount(std::string const& requestedEmail)
{
    // Check account limit
    if (_totalAccounts.load() >= _maxAccounts)
    {
        TC_LOG_ERROR("module.playerbot.account",
            "Cannot create account: limit {} reached", _maxAccounts);
        return 0;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // Generate email and password
    std::string email = requestedEmail.empty() ? GenerateUniqueEmail() : requestedEmail;
    std::string password = GenerateSecurePassword();

    // Create BattleNet account using Trinity's system
    uint32 bnetAccountId = sAccountMgr->CreateBattlenetAccount(email, password);

    if (bnetAccountId == 0)
    {
        TC_LOG_ERROR("module.playerbot.account",
            "Failed to create BattleNet account for email: {}", email);
        return 0;
    }

    // Get the automatically created legacy account ID
    uint32 legacyAccountId = sAccountMgr->GetAccountIdByBattlenetAccount(bnetAccountId);

    if (legacyAccountId == 0)
    {
        TC_LOG_ERROR("module.playerbot.account",
            "Failed to retrieve legacy account for BNet account: {}", bnetAccountId);
        return 0;
    }

    // Create account info
    BotAccountInfo info;
    info.bnetAccountId = bnetAccountId;
    info.legacyAccountId = legacyAccountId;
    info.email = email;
    info.passwordHash = password; // Store hash in production
    info.createdAt = std::chrono::system_clock::now();
    info.characterCount = 0;
    info.isActive = false;
    info.isInPool = false;

    // Store in memory
    {
        std::unique_lock lock(_accountsMutex);
        _accounts[bnetAccountId] = info;
    }

    // Store in database asynchronously
    StoreAccountMetadata(info);

    // Update statistics
    _totalAccounts.fetch_add(1);

    auto endTime = std::chrono::high_resolution_clock::now();
    uint32 creationTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - startTime).count();

    TC_LOG_DEBUG("module.playerbot.account",
        "Created bot account: BNet {} (Legacy {}), Email: {}, Time: {}μs",
        bnetAccountId, legacyAccountId, email, creationTimeUs);

    return bnetAccountId;
}

void BotAccountMgr::CreateBotAccountsBatch(uint32 count,
    std::function<void(std::vector<uint32>)> callback)
{
    TC_LOG_INFO("module.playerbot.account", "Creating batch of {} bot accounts...", count);

    // Use async execution via BotDatabasePool's thread pool
    sBotDBPool->ExecuteAsync(nullptr,
        [this, count, callback](PreparedQueryResult /*result*/)
        {
            std::vector<uint32> createdAccounts;
            createdAccounts.reserve(count);

            for (uint32 i = 0; i < count; ++i)
            {
                if (uint32 accountId = CreateBotAccount())
                {
                    createdAccounts.push_back(accountId);
                }

                // Small delay to avoid overwhelming the system
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            TC_LOG_INFO("module.playerbot.account",
                "Batch creation complete: {}/{} accounts created",
                createdAccounts.size(), count);

            if (callback)
            {
                callback(createdAccounts);
            }
        });
}

void BotAccountMgr::RefillAccountPool(uint32 targetPoolSize)
{
    // Check if refill already in progress
    uint32 expected = 0;
    if (!_poolRefillInProgress.compare_exchange_strong(expected, 1))
    {
        return; // Already refilling
    }

    TC_LOG_INFO("module.playerbot.account",
        "Refilling account pool to {} accounts...", targetPoolSize);

    // Calculate how many accounts to create
    uint32 currentSize = 0;
    {
        std::lock_guard<std::mutex> lock(_poolMutex);
        currentSize = _accountPool.size();
    }

    uint32 toCreate = (currentSize < targetPoolSize) ? (targetPoolSize - currentSize) : 0;

    if (toCreate > 0)
    {
        CreateBotAccountsBatch(toCreate,
            [this](std::vector<uint32> const& accounts)
            {
                // Add to pool
                std::lock_guard<std::mutex> lock(_poolMutex);
                for (uint32 accountId : accounts)
                {
                    _accountPool.push(accountId);

                    // Mark as in pool
                    if (auto it = _accounts.find(accountId); it != _accounts.end())
                    {
                        it->second.isInPool = true;
                    }
                }

                TC_LOG_INFO("module.playerbot.account",
                    "✅ Account pool refilled: {} accounts now available",
                    _accountPool.size());

                _poolRefillInProgress.store(0);
            });
    }
    else
    {
        _poolRefillInProgress.store(0);
    }
}

uint32 BotAccountMgr::AcquireAccount()
{
    // Try to get from pool first
    {
        std::lock_guard<std::mutex> lock(_poolMutex);
        if (!_accountPool.empty())
        {
            uint32 accountId = _accountPool.front();
            _accountPool.pop();

            // Mark as active
            if (auto it = _accounts.find(accountId); it != _accounts.end())
            {
                it->second.isActive = true;
                it->second.isInPool = false;
            }

            _activeAccounts.fetch_add(1);

            TC_LOG_DEBUG("module.playerbot.account",
                "Acquired account {} from pool", accountId);

            // Trigger pool refill if getting low
            if (_accountPool.size() < _targetPoolSize / 2)
            {
                RefillAccountPool(_targetPoolSize);
            }

            return accountId;
        }
    }

    // Pool empty, create new account
    TC_LOG_DEBUG("module.playerbot.account",
        "Account pool empty, creating new account...");

    uint32 newAccountId = CreateBotAccount();

    if (newAccountId > 0)
    {
        // Mark as active
        if (auto it = _accounts.find(newAccountId); it != _accounts.end())
        {
            it->second.isActive = true;
        }
        _activeAccounts.fetch_add(1);
    }

    return newAccountId;
}

void BotAccountMgr::ReleaseAccount(uint32 bnetAccountId)
{
    std::lock_guard<std::mutex> lock(_poolMutex);

    // Check if account exists
    auto it = _accounts.find(bnetAccountId);
    if (it == _accounts.end())
    {
        TC_LOG_WARN("module.playerbot.account",
            "Cannot release unknown account {}", bnetAccountId);
        return;
    }

    // Return to pool if under limit
    if (_accountPool.size() < _targetPoolSize * 2) // Allow some overflow
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

bool BotAccountMgr::CanCreateCharacter(uint32 bnetAccountId) const
{
    std::shared_lock lock(_accountsMutex);

    auto it = _accounts.find(bnetAccountId);
    if (it == _accounts.end())
    {
        return false;
    }

    return it->second.characterCount < _maxCharactersPerAccount;
}

void BotAccountMgr::UpdateCharacterCount(uint32 bnetAccountId, int8 delta)
{
    std::unique_lock lock(_accountsMutex);

    auto it = _accounts.find(bnetAccountId);
    if (it != _accounts.end())
    {
        int16 newCount = it->second.characterCount + delta;
        it->second.characterCount = std::max(0, std::min(static_cast<int>(
            _maxCharactersPerAccount), static_cast<int>(newCount)));

        TC_LOG_DEBUG("module.playerbot.account",
            "Updated character count for account {}: {} characters",
            bnetAccountId, it->second.characterCount);
    }
}

void BotAccountMgr::DeleteBotAccount(uint32 bnetAccountId,
    std::function<void(bool success)> callback)
{
    TC_LOG_INFO("module.playerbot.account",
        "Deleting bot account {}...", bnetAccountId);

    // Get legacy account ID for character deletion
    uint32 legacyAccountId = sAccountMgr->GetAccountIdByBattlenetAccount(bnetAccountId);

    // Delete all characters first (async)
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARS_BY_ACCOUNT);
    stmt->SetData(0, legacyAccountId);

    CharacterDatabase.AsyncExecute(stmt,
        [this, bnetAccountId, callback](bool charDeleteSuccess)
        {
            if (!charDeleteSuccess)
            {
                TC_LOG_ERROR("module.playerbot.account",
                    "Failed to delete characters for account {}", bnetAccountId);
                if (callback) callback(false);
                return;
            }

            // Delete BattleNet account (also deletes linked legacy account)
            AccountOpResult result = sAccountMgr->DeleteBattlenetAccount(bnetAccountId);

            if (result == AOR_OK)
            {
                // Remove from memory
                {
                    std::unique_lock lock(_accountsMutex);
                    _accounts.erase(bnetAccountId);
                }

                // Remove from database metadata
                PreparedStatement* metaStmt = sBotDBPool->GetPreparedStatement(BOT_DEL_ACCOUNT_META);
                metaStmt->SetData(0, bnetAccountId);
                sBotDBPool->ExecuteAsyncNoResult(metaStmt);

                _totalAccounts.fetch_sub(1);

                TC_LOG_INFO("module.playerbot.account",
                    "✅ Deleted bot account {}", bnetAccountId);

                if (callback) callback(true);
            }
            else
            {
                TC_LOG_ERROR("module.playerbot.account",
                    "Failed to delete BattleNet account {}: {}", bnetAccountId, result);
                if (callback) callback(false);
            }
        });
}

std::string BotAccountMgr::GenerateUniqueEmail()
{
    uint32 counter = _emailCounter.fetch_add(1);

    std::ostringstream email;
    email << "bot" << std::setfill('0') << std::setw(6) << counter
          << "@" << _emailDomain;

    return email.str();
}

std::string BotAccountMgr::GenerateSecurePassword()
{
    // Generate random secure password
    static const char charset[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "!@#$%^&*";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);

    std::string password;
    password.reserve(16);

    for (int i = 0; i < 16; ++i)
    {
        password += charset[dis(gen)];
    }

    return password;
}

void BotAccountMgr::StoreAccountMetadata(BotAccountInfo const& info)
{
    // Store bot metadata in playerbot database
    PreparedStatement* stmt = sBotDBPool->GetPreparedStatement(BOT_INS_ACCOUNT_META);
    stmt->SetData(0, info.legacyAccountId);
    stmt->SetData(1, info.bnetAccountId);
    stmt->SetData(2, info.email);
    stmt->SetData(3, info.characterCount);
    stmt->SetData(4, info.isActive ? 1 : 0);

    sBotDBPool->ExecuteAsyncNoResult(stmt);
}

void BotAccountMgr::LoadAccountMetadata()
{
    TC_LOG_INFO("module.playerbot.account", "Loading bot account metadata...");

    PreparedStatement* stmt = sBotDBPool->GetPreparedStatement(BOT_SEL_ALL_ACCOUNT_META);

    sBotDBPool->ExecuteAsync(stmt,
        [this](PreparedQueryResult result)
        {
            if (!result)
            {
                TC_LOG_INFO("module.playerbot.account", "No existing bot accounts found");
                return;
            }

            uint32 count = 0;
            do
            {
                Field* fields = result->Fetch();

                BotAccountInfo info;
                info.legacyAccountId = fields[0].GetUInt32();
                info.bnetAccountId = fields[1].GetUInt32();
                info.email = fields[2].GetString();
                info.characterCount = fields[3].GetUInt8();
                info.isActive = fields[4].GetBool();
                info.isInPool = false;
                info.createdAt = std::chrono::system_clock::now(); // Approximate

                _accounts[info.bnetAccountId] = info;
                ++count;

            } while (result->NextRow());

            _totalAccounts.store(count);

            TC_LOG_INFO("module.playerbot.account",
                "✅ Loaded {} bot account metadata entries", count);
        });
}

} // namespace Playerbot
```

### Step 3: Database Schema
**File:** `sql/playerbot/account_tables.sql`
**Time:** 1 hour

```sql
-- Bot account metadata (links to Trinity's auth database)
CREATE TABLE IF NOT EXISTS `playerbot_account_meta` (
    `legacy_account_id` INT UNSIGNED NOT NULL,          -- For character operations
    `bnet_account_id` INT UNSIGNED NOT NULL PRIMARY KEY, -- Primary BattleNet account
    `email` VARCHAR(255) NOT NULL,
    `character_count` TINYINT UNSIGNED DEFAULT 0,
    `is_active` TINYINT(1) DEFAULT 0,
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `last_active` TIMESTAMP NULL,

    UNIQUE KEY `idx_legacy_account` (`legacy_account_id`),
    UNIQUE KEY `idx_email` (`email`),
    INDEX `idx_active` (`is_active`),
    INDEX `idx_character_count` (`character_count`),

    FOREIGN KEY (`legacy_account_id`) REFERENCES `auth`.`account`(`id`) ON DELETE CASCADE,
    FOREIGN KEY (`bnet_account_id`) REFERENCES `auth`.`bnet_account`(`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Bot account metadata';

-- Prepared statements enum (add to BotDatabaseStatements.h)
enum BotDatabaseStatements
{
    BOT_INS_ACCOUNT_META,
    BOT_SEL_ALL_ACCOUNT_META,
    BOT_SEL_ACCOUNT_META_BY_BNET,
    BOT_UPD_ACCOUNT_CHAR_COUNT,
    BOT_DEL_ACCOUNT_META,
    BOT_MAX
};
```

### Step 4: Integration Tests
**File:** `src/modules/Playerbot/Tests/AccountTests.cpp`
**Time:** 2 hours

```cpp
#include "catch.hpp"
#include "BotAccountMgr.h"
#include "BotSessionMgr.h"

TEST_CASE("BotAccountMgr creates accounts correctly", "[account]")
{
    SECTION("Single account creation")
    {
        uint32 accountId = sBotAccountMgr->CreateBotAccount();
        REQUIRE(accountId > 0);

        auto info = sBotAccountMgr->GetAccountInfo(accountId);
        REQUIRE(info != nullptr);
        REQUIRE(info->bnetAccountId == accountId);
        REQUIRE(info->legacyAccountId > 0);
        REQUIRE(info->characterCount == 0);
    }

    SECTION("Batch account creation")
    {
        std::promise<std::vector<uint32>> promise;
        auto future = promise.get_future();

        sBotAccountMgr->CreateBotAccountsBatch(10,
            [&promise](std::vector<uint32> const& accounts)
            {
                promise.set_value(accounts);
            });

        auto accounts = future.get();
        REQUIRE(accounts.size() == 10);

        for (uint32 accountId : accounts)
        {
            REQUIRE(accountId > 0);
        }
    }
}

TEST_CASE("Account pool management", "[account][pool]")
{
    SECTION("Pool refill")
    {
        uint32 initialPoolSize = sBotAccountMgr->GetPoolSize();
        sBotAccountMgr->RefillAccountPool(initialPoolSize + 10);

        // Wait for async refill
        std::this_thread::sleep_for(std::chrono::seconds(2));

        REQUIRE(sBotAccountMgr->GetPoolSize() >= initialPoolSize);
    }

    SECTION("Acquire and release")
    {
        uint32 accountId = sBotAccountMgr->AcquireAccount();
        REQUIRE(accountId > 0);

        auto info = sBotAccountMgr->GetAccountInfo(accountId);
        REQUIRE(info->isActive == true);

        sBotAccountMgr->ReleaseAccount(accountId);
        REQUIRE(info->isActive == false);
    }
}

TEST_CASE("Character limit enforcement", "[account][character]")
{
    uint32 accountId = sBotAccountMgr->CreateBotAccount();

    // Should allow up to 10 characters
    for (int i = 0; i < 10; ++i)
    {
        REQUIRE(sBotAccountMgr->CanCreateCharacter(accountId) == true);
        sBotAccountMgr->UpdateCharacterCount(accountId, 1);
    }

    // 11th character should be blocked
    REQUIRE(sBotAccountMgr->CanCreateCharacter(accountId) == false);
}

TEST_CASE("Integration with BotSessionMgr", "[account][session]")
{
    // Create account
    uint32 accountId = sBotAccountMgr->AcquireAccount();
    REQUIRE(accountId > 0);

    // Create session for account
    BotSession* session = sBotSessionMgr->CreateSession(accountId);
    REQUIRE(session != nullptr);
    REQUIRE(session->GetBnetAccountId() == accountId);

    // Cleanup
    sBotSessionMgr->ReleaseSession(accountId);
    sBotAccountMgr->ReleaseAccount(accountId);
}
```

## Key Implementation Details

### Performance Optimizations
1. **Account Pooling**: Pre-created accounts for instant availability
2. **Async Operations**: All database operations via BotDatabasePool
3. **Parallel Hashmap**: Concurrent access with minimal locking
4. **Batch Processing**: Efficient bulk account creation

### Trinity Integration
1. **Uses CreateBattlenetAccount()**: Proper BNet account creation
2. **Legacy Account Linking**: Automatic legacy account for characters
3. **Character Limits**: Enforces Trinity's 10 character limit
4. **Database Foreign Keys**: Proper referential integrity

### Error Handling
1. **Account Limit Checks**: Prevents exceeding configured limits
2. **Duplicate Prevention**: Unique email generation
3. **Rollback Support**: Clean deletion of accounts and characters
4. **Async Error Callbacks**: Proper error propagation

## Next Steps
1. Implement BotCharacterMgr for character creation
2. Create name generation system
3. Build equipment template system
4. Integrate with lifecycle management