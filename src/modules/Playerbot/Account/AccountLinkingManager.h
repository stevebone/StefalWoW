/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * AccountLinkingManager: Links human player accounts with bot accounts,
 * enabling permission-based access to bot inventories, trade, control,
 * guild sharing, and friends list sharing.
 *
 * Thread Safety: Protected by mutex. All public methods are thread-safe.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <mutex>

class Player;

namespace Playerbot
{

/// Permission flags for account linking
enum class LinkPermission : uint16
{
    NONE            = 0x0000,
    VIEW_INVENTORY  = 0x0001,  // See bot's inventory
    TRADE           = 0x0002,  // Trade items with bot
    CONTROL         = 0x0004,  // Issue movement/combat commands
    SHARE_GUILD     = 0x0008,  // Bot auto-joins owner's guild
    SHARE_FRIENDS   = 0x0010,  // Share friends list
    SUMMON          = 0x0020,  // Summon bot to player location
    DISMISS         = 0x0040,  // Log bot out
    RENAME          = 0x0080,  // Rename the bot
    EQUIP           = 0x0100,  // Change bot equipment
    SPEC            = 0x0200,  // Change bot spec/talents

    // Common presets
    BASIC           = VIEW_INVENTORY | SUMMON | DISMISS,
    STANDARD        = BASIC | TRADE | CONTROL | SHARE_GUILD,
    FULL            = 0x03FF   // All permissions
};

inline LinkPermission operator|(LinkPermission a, LinkPermission b)
{
    return static_cast<LinkPermission>(static_cast<uint16>(a) | static_cast<uint16>(b));
}

inline LinkPermission operator&(LinkPermission a, LinkPermission b)
{
    return static_cast<LinkPermission>(static_cast<uint16>(a) & static_cast<uint16>(b));
}

inline LinkPermission& operator|=(LinkPermission& a, LinkPermission b)
{
    return a = a | b;
}

inline bool HasPermission(LinkPermission flags, LinkPermission check)
{
    return (static_cast<uint16>(flags) & static_cast<uint16>(check)) == static_cast<uint16>(check);
}

/// A link between a human account and a bot account
struct AccountLink
{
    uint32 linkId = 0;
    uint32 ownerAccountId = 0;     // Human player's account
    uint32 botAccountId = 0;       // Bot's account
    ObjectGuid botGuid;            // Specific bot character (Empty = all bots on account)
    LinkPermission permissions = LinkPermission::STANDARD;
    uint32 createdTime = 0;
    uint32 lastAccessTime = 0;
    bool active = true;
};

/// Per-player link cache for fast lookups
struct PlayerLinkCache
{
    uint32 accountId = 0;
    std::vector<AccountLink> ownedLinks;     // Links this player owns
    std::vector<AccountLink> linkedToMe;     // Links where this player's bots are linked
};

/// Manager metrics
struct AccountLinkingMetrics
{
    uint32 totalLinks = 0;
    uint32 activeLinks = 0;
    uint32 totalAccounts = 0;
};

/**
 * @class AccountLinkingManager
 * @brief Manages account-to-account links for bot ownership and permissions
 *
 * Allows human players to claim ownership of bot accounts, granting
 * permission-based access to bot features (inventory, trade, control, etc.).
 */
class TC_GAME_API AccountLinkingManager
{
public:
    static AccountLinkingManager* instance()
    {
        static AccountLinkingManager inst;
        return &inst;
    }

    /// Initialize: load links from DB
    bool Initialize();

    /// Shutdown
    void Shutdown();

    // ========================================================================
    // Link Management
    // ========================================================================

    /// Create a new account link
    /// @return link ID (0 on failure)
    uint32 CreateLink(uint32 ownerAccountId, uint32 botAccountId,
                      LinkPermission permissions = LinkPermission::STANDARD);

    /// Create a link to a specific bot character
    uint32 CreateLink(uint32 ownerAccountId, ObjectGuid botGuid,
                      LinkPermission permissions = LinkPermission::STANDARD);

    /// Remove a link by ID
    bool RemoveLink(uint32 linkId);

    /// Remove all links for an owner account
    void RemoveAllLinks(uint32 ownerAccountId);

    /// Update permissions on an existing link
    bool UpdatePermissions(uint32 linkId, LinkPermission newPermissions);

    /// Activate/deactivate a link
    bool SetLinkActive(uint32 linkId, bool active);

    // ========================================================================
    // Permission Queries
    // ========================================================================

    /// Check if a player has a specific permission over a bot
    bool HasPermission(uint32 ownerAccountId, ObjectGuid botGuid, LinkPermission permission) const;

    /// Check if a player has a specific permission over any bot on an account
    bool HasPermissionForAccount(uint32 ownerAccountId, uint32 botAccountId, LinkPermission permission) const;

    /// Get all permissions a player has over a specific bot
    LinkPermission GetPermissions(uint32 ownerAccountId, ObjectGuid botGuid) const;

    /// Check if a bot is linked to any player
    bool IsBotLinked(ObjectGuid botGuid) const;

    /// Get the owner account for a linked bot
    uint32 GetOwnerAccount(ObjectGuid botGuid) const;

    // ========================================================================
    // Link Queries
    // ========================================================================

    /// Get all links owned by an account
    std::vector<AccountLink> GetOwnedLinks(uint32 ownerAccountId) const;

    /// Get all links pointing to a bot account
    std::vector<AccountLink> GetLinksForBotAccount(uint32 botAccountId) const;

    /// Get link by ID
    AccountLink const* GetLink(uint32 linkId) const;

    /// Get count of links an owner has
    uint32 GetLinkCount(uint32 ownerAccountId) const;

    // ========================================================================
    // Metrics
    // ========================================================================

    AccountLinkingMetrics GetMetrics() const;

    // ========================================================================
    // Permission name helpers
    // ========================================================================

    static char const* PermissionToString(LinkPermission perm);
    static LinkPermission StringToPermission(std::string const& str);

private:
    AccountLinkingManager();
    ~AccountLinkingManager() = default;
    AccountLinkingManager(AccountLinkingManager const&) = delete;
    AccountLinkingManager& operator=(AccountLinkingManager const&) = delete;

    void LoadLinksFromDB();
    void SaveLinkToDB(AccountLink const& link);
    void DeleteLinkFromDB(uint32 linkId);

    // Links indexed by linkId
    std::unordered_map<uint32, AccountLink> _links;

    // Fast lookup: ownerAccountId → set of linkIds
    std::unordered_map<uint32, std::unordered_set<uint32>> _ownerIndex;

    // Fast lookup: botAccountId → set of linkIds
    std::unordered_map<uint32, std::unordered_set<uint32>> _botAccountIndex;

    // Fast lookup: botGuid → set of linkIds
    std::unordered_map<ObjectGuid, std::unordered_set<uint32>> _botGuidIndex;

    uint32 _nextLinkId = 1;
    bool _initialized = false;
    mutable std::mutex _mutex;

    static constexpr uint32 MAX_LINKS_PER_ACCOUNT = 50;
};

#define sAccountLinkingManager AccountLinkingManager::instance()

} // namespace Playerbot
