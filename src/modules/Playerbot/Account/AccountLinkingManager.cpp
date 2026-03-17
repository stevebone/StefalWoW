/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * AccountLinkingManager: Links human player accounts with bot accounts.
 */

#include "AccountLinkingManager.h"
#include "Database/PlayerbotDatabase.h"
#include "Log.h"
#include "GameTime.h"
#include "Player.h"
#include "StringFormat.h"
#include <algorithm>

namespace Playerbot
{

AccountLinkingManager::AccountLinkingManager()
    : _nextLinkId(1)
    , _initialized(false)
{
}

bool AccountLinkingManager::Initialize()
{
    if (_initialized)
        return true;

    TC_LOG_INFO("module.playerbot", "AccountLinkingManager: Initializing...");

    LoadLinksFromDB();

    _initialized = true;
    TC_LOG_INFO("module.playerbot", "AccountLinkingManager: Initialized with {} account links", _links.size());
    return true;
}

void AccountLinkingManager::Shutdown()
{
    if (!_initialized)
        return;

    TC_LOG_INFO("module.playerbot", "AccountLinkingManager: Shutting down ({} links)", _links.size());

    std::lock_guard<std::mutex> lock(_mutex);
    _links.clear();
    _ownerIndex.clear();
    _botAccountIndex.clear();
    _botGuidIndex.clear();
    _initialized = false;

    TC_LOG_INFO("module.playerbot", "AccountLinkingManager: Shutdown complete");
}

// ============================================================================
// Link Management
// ============================================================================

uint32 AccountLinkingManager::CreateLink(uint32 ownerAccountId, uint32 botAccountId,
                                          LinkPermission permissions)
{
    std::lock_guard<std::mutex> lock(_mutex);

    // Check max links per account
    auto ownerIt = _ownerIndex.find(ownerAccountId);
    if (ownerIt != _ownerIndex.end() && ownerIt->second.size() >= MAX_LINKS_PER_ACCOUNT)
    {
        TC_LOG_WARN("module.playerbot", "AccountLinkingManager: Account {} already has max links ({})",
            ownerAccountId, MAX_LINKS_PER_ACCOUNT);
        return 0;
    }

    // Check for duplicate
    if (ownerIt != _ownerIndex.end())
    {
        for (uint32 linkId : ownerIt->second)
        {
            auto it = _links.find(linkId);
            if (it != _links.end() && it->second.botAccountId == botAccountId &&
                it->second.botGuid.IsEmpty() && it->second.active)
            {
                TC_LOG_WARN("module.playerbot", "AccountLinkingManager: Duplicate link {} -> {} already exists",
                    ownerAccountId, botAccountId);
                return it->second.linkId;
            }
        }
    }

    AccountLink link;
    link.linkId = _nextLinkId++;
    link.ownerAccountId = ownerAccountId;
    link.botAccountId = botAccountId;
    link.permissions = permissions;
    link.createdTime = GameTime::GetGameTime();
    link.lastAccessTime = link.createdTime;
    link.active = true;

    uint32 id = link.linkId;

    _links[id] = link;
    _ownerIndex[ownerAccountId].insert(id);
    _botAccountIndex[botAccountId].insert(id);

    SaveLinkToDB(link);

    TC_LOG_INFO("module.playerbot", "AccountLinkingManager: Created link #{} (account {} -> bot account {}, perms=0x{:04X})",
        id, ownerAccountId, botAccountId, static_cast<uint16>(permissions));

    return id;
}

uint32 AccountLinkingManager::CreateLink(uint32 ownerAccountId, ObjectGuid botGuid,
                                          LinkPermission permissions)
{
    std::lock_guard<std::mutex> lock(_mutex);

    // Check max links
    auto ownerIt = _ownerIndex.find(ownerAccountId);
    if (ownerIt != _ownerIndex.end() && ownerIt->second.size() >= MAX_LINKS_PER_ACCOUNT)
        return 0;

    // Check duplicate
    auto guidIt = _botGuidIndex.find(botGuid);
    if (guidIt != _botGuidIndex.end())
    {
        for (uint32 linkId : guidIt->second)
        {
            auto it = _links.find(linkId);
            if (it != _links.end() && it->second.ownerAccountId == ownerAccountId && it->second.active)
                return it->second.linkId;
        }
    }

    AccountLink link;
    link.linkId = _nextLinkId++;
    link.ownerAccountId = ownerAccountId;
    link.botGuid = botGuid;
    link.permissions = permissions;
    link.createdTime = GameTime::GetGameTime();
    link.lastAccessTime = link.createdTime;
    link.active = true;

    uint32 id = link.linkId;

    _links[id] = link;
    _ownerIndex[ownerAccountId].insert(id);
    _botGuidIndex[botGuid].insert(id);

    SaveLinkToDB(link);

    TC_LOG_INFO("module.playerbot", "AccountLinkingManager: Created link #{} (account {} -> bot {}, perms=0x{:04X})",
        id, ownerAccountId, botGuid.ToString(), static_cast<uint16>(permissions));

    return id;
}

bool AccountLinkingManager::RemoveLink(uint32 linkId)
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _links.find(linkId);
    if (it == _links.end())
        return false;

    AccountLink const& link = it->second;

    // Remove from indices
    if (auto ownerIt = _ownerIndex.find(link.ownerAccountId); ownerIt != _ownerIndex.end())
        ownerIt->second.erase(linkId);

    if (link.botAccountId > 0)
    {
        if (auto botIt = _botAccountIndex.find(link.botAccountId); botIt != _botAccountIndex.end())
            botIt->second.erase(linkId);
    }

    if (!link.botGuid.IsEmpty())
    {
        if (auto guidIt = _botGuidIndex.find(link.botGuid); guidIt != _botGuidIndex.end())
            guidIt->second.erase(linkId);
    }

    DeleteLinkFromDB(linkId);
    _links.erase(it);

    TC_LOG_INFO("module.playerbot", "AccountLinkingManager: Removed link #{}", linkId);
    return true;
}

void AccountLinkingManager::RemoveAllLinks(uint32 ownerAccountId)
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto ownerIt = _ownerIndex.find(ownerAccountId);
    if (ownerIt == _ownerIndex.end())
        return;

    // Copy IDs since we'll modify the set
    std::vector<uint32> linkIds(ownerIt->second.begin(), ownerIt->second.end());

    for (uint32 linkId : linkIds)
    {
        auto it = _links.find(linkId);
        if (it == _links.end())
            continue;

        AccountLink const& link = it->second;

        if (link.botAccountId > 0)
            if (auto botIt = _botAccountIndex.find(link.botAccountId); botIt != _botAccountIndex.end())
                botIt->second.erase(linkId);

        if (!link.botGuid.IsEmpty())
            if (auto guidIt = _botGuidIndex.find(link.botGuid); guidIt != _botGuidIndex.end())
                guidIt->second.erase(linkId);

        DeleteLinkFromDB(linkId);
        _links.erase(it);
    }

    _ownerIndex.erase(ownerIt);

    TC_LOG_INFO("module.playerbot", "AccountLinkingManager: Removed all {} links for account {}",
        linkIds.size(), ownerAccountId);
}

bool AccountLinkingManager::UpdatePermissions(uint32 linkId, LinkPermission newPermissions)
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _links.find(linkId);
    if (it == _links.end())
        return false;

    it->second.permissions = newPermissions;
    SaveLinkToDB(it->second);
    return true;
}

bool AccountLinkingManager::SetLinkActive(uint32 linkId, bool active)
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _links.find(linkId);
    if (it == _links.end())
        return false;

    it->second.active = active;
    SaveLinkToDB(it->second);
    return true;
}

// ============================================================================
// Permission Queries
// ============================================================================

bool AccountLinkingManager::HasPermission(uint32 ownerAccountId, ObjectGuid botGuid,
                                           LinkPermission permission) const
{
    std::lock_guard<std::mutex> lock(_mutex);

    // Check guid-specific links
    auto guidIt = _botGuidIndex.find(botGuid);
    if (guidIt != _botGuidIndex.end())
    {
        for (uint32 linkId : guidIt->second)
        {
            auto it = _links.find(linkId);
            if (it != _links.end() && it->second.ownerAccountId == ownerAccountId &&
                it->second.active && Playerbot::HasPermission(it->second.permissions, permission))
                return true;
        }
    }

    return false;
}

bool AccountLinkingManager::HasPermissionForAccount(uint32 ownerAccountId, uint32 botAccountId,
                                                     LinkPermission permission) const
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto botIt = _botAccountIndex.find(botAccountId);
    if (botIt != _botAccountIndex.end())
    {
        for (uint32 linkId : botIt->second)
        {
            auto it = _links.find(linkId);
            if (it != _links.end() && it->second.ownerAccountId == ownerAccountId &&
                it->second.active && Playerbot::HasPermission(it->second.permissions, permission))
                return true;
        }
    }

    return false;
}

LinkPermission AccountLinkingManager::GetPermissions(uint32 ownerAccountId, ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    LinkPermission result = LinkPermission::NONE;

    auto guidIt = _botGuidIndex.find(botGuid);
    if (guidIt != _botGuidIndex.end())
    {
        for (uint32 linkId : guidIt->second)
        {
            auto it = _links.find(linkId);
            if (it != _links.end() && it->second.ownerAccountId == ownerAccountId && it->second.active)
                result |= it->second.permissions;
        }
    }

    return result;
}

bool AccountLinkingManager::IsBotLinked(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _botGuidIndex.find(botGuid);
    if (it == _botGuidIndex.end())
        return false;

    for (uint32 linkId : it->second)
    {
        auto linkIt = _links.find(linkId);
        if (linkIt != _links.end() && linkIt->second.active)
            return true;
    }
    return false;
}

uint32 AccountLinkingManager::GetOwnerAccount(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _botGuidIndex.find(botGuid);
    if (it == _botGuidIndex.end())
        return 0;

    for (uint32 linkId : it->second)
    {
        auto linkIt = _links.find(linkId);
        if (linkIt != _links.end() && linkIt->second.active)
            return linkIt->second.ownerAccountId;
    }
    return 0;
}

// ============================================================================
// Link Queries
// ============================================================================

std::vector<AccountLink> AccountLinkingManager::GetOwnedLinks(uint32 ownerAccountId) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<AccountLink> result;

    auto it = _ownerIndex.find(ownerAccountId);
    if (it == _ownerIndex.end())
        return result;

    for (uint32 linkId : it->second)
    {
        auto linkIt = _links.find(linkId);
        if (linkIt != _links.end())
            result.push_back(linkIt->second);
    }
    return result;
}

std::vector<AccountLink> AccountLinkingManager::GetLinksForBotAccount(uint32 botAccountId) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<AccountLink> result;

    auto it = _botAccountIndex.find(botAccountId);
    if (it == _botAccountIndex.end())
        return result;

    for (uint32 linkId : it->second)
    {
        auto linkIt = _links.find(linkId);
        if (linkIt != _links.end())
            result.push_back(linkIt->second);
    }
    return result;
}

AccountLink const* AccountLinkingManager::GetLink(uint32 linkId) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _links.find(linkId);
    return it != _links.end() ? &it->second : nullptr;
}

uint32 AccountLinkingManager::GetLinkCount(uint32 ownerAccountId) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _ownerIndex.find(ownerAccountId);
    return it != _ownerIndex.end() ? static_cast<uint32>(it->second.size()) : 0;
}

// ============================================================================
// Metrics
// ============================================================================

AccountLinkingMetrics AccountLinkingManager::GetMetrics() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    AccountLinkingMetrics metrics;
    metrics.totalLinks = static_cast<uint32>(_links.size());

    for (auto const& [id, link] : _links)
        if (link.active)
            ++metrics.activeLinks;

    metrics.totalAccounts = static_cast<uint32>(_ownerIndex.size());
    return metrics;
}

// ============================================================================
// Permission Helpers
// ============================================================================

char const* AccountLinkingManager::PermissionToString(LinkPermission perm)
{
    switch (perm)
    {
        case LinkPermission::VIEW_INVENTORY: return "view_inventory";
        case LinkPermission::TRADE:          return "trade";
        case LinkPermission::CONTROL:        return "control";
        case LinkPermission::SHARE_GUILD:    return "share_guild";
        case LinkPermission::SHARE_FRIENDS:  return "share_friends";
        case LinkPermission::SUMMON:         return "summon";
        case LinkPermission::DISMISS:        return "dismiss";
        case LinkPermission::RENAME:         return "rename";
        case LinkPermission::EQUIP:          return "equip";
        case LinkPermission::SPEC:           return "spec";
        case LinkPermission::BASIC:          return "basic";
        case LinkPermission::STANDARD:       return "standard";
        case LinkPermission::FULL:           return "full";
        default:                             return "unknown";
    }
}

LinkPermission AccountLinkingManager::StringToPermission(std::string const& str)
{
    if (str == "view_inventory") return LinkPermission::VIEW_INVENTORY;
    if (str == "trade")          return LinkPermission::TRADE;
    if (str == "control")        return LinkPermission::CONTROL;
    if (str == "share_guild")    return LinkPermission::SHARE_GUILD;
    if (str == "share_friends")  return LinkPermission::SHARE_FRIENDS;
    if (str == "summon")         return LinkPermission::SUMMON;
    if (str == "dismiss")        return LinkPermission::DISMISS;
    if (str == "rename")         return LinkPermission::RENAME;
    if (str == "equip")          return LinkPermission::EQUIP;
    if (str == "spec")           return LinkPermission::SPEC;
    if (str == "basic")          return LinkPermission::BASIC;
    if (str == "standard")       return LinkPermission::STANDARD;
    if (str == "full")           return LinkPermission::FULL;
    return LinkPermission::NONE;
}

// ============================================================================
// Database Operations
// ============================================================================

void AccountLinkingManager::LoadLinksFromDB()
{
    QueryResult result = sPlayerbotDatabase->Query(
        "SELECT link_id, owner_account_id, bot_account_id, bot_guid, "
        "permissions, created_time, last_access_time, active "
        "FROM playerbot_account_links");

    if (!result)
    {
        TC_LOG_INFO("module.playerbot", "AccountLinkingManager: No existing links found in database");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        AccountLink link;

        link.linkId           = fields[0].GetUInt32();
        link.ownerAccountId   = fields[1].GetUInt32();
        link.botAccountId     = fields[2].GetUInt32();
        uint64 botGuidLow     = fields[3].GetUInt64();
        link.permissions      = static_cast<LinkPermission>(fields[4].GetUInt16());
        link.createdTime      = fields[5].GetUInt32();
        link.lastAccessTime   = fields[6].GetUInt32();
        link.active           = fields[7].GetBool();

        if (botGuidLow > 0)
            link.botGuid = ObjectGuid::Create<HighGuid::Player>(botGuidLow);

        // Update next ID
        if (link.linkId >= _nextLinkId)
            _nextLinkId = link.linkId + 1;

        uint32 id = link.linkId;

        // Build indices
        _ownerIndex[link.ownerAccountId].insert(id);
        if (link.botAccountId > 0)
            _botAccountIndex[link.botAccountId].insert(id);
        if (!link.botGuid.IsEmpty())
            _botGuidIndex[link.botGuid].insert(id);

        _links[id] = std::move(link);
        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("module.playerbot", "AccountLinkingManager: Loaded {} account links from database", count);
}

void AccountLinkingManager::SaveLinkToDB(AccountLink const& link)
{
    std::string sql = Trinity::StringFormat(
        "REPLACE INTO playerbot_account_links "
        "(link_id, owner_account_id, bot_account_id, bot_guid, permissions, "
        "created_time, last_access_time, active) VALUES "
        "({}, {}, {}, {}, {}, {}, {}, {})",
        link.linkId, link.ownerAccountId, link.botAccountId,
        link.botGuid.IsEmpty() ? uint64(0) : link.botGuid.GetCounter(),
        static_cast<uint16>(link.permissions),
        link.createdTime, link.lastAccessTime, link.active ? 1 : 0);

    sPlayerbotDatabase->Execute(sql);
}

void AccountLinkingManager::DeleteLinkFromDB(uint32 linkId)
{
    std::string sql = Trinity::StringFormat(
        "DELETE FROM playerbot_account_links WHERE link_id = {}", linkId);
    sPlayerbotDatabase->Execute(sql);
}

} // namespace Playerbot
