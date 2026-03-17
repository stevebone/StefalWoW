/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * GroupMemberResolver - Central utility for reliable group member lookups
 *
 * PURPOSE:
 * This class provides a single, reliable way to look up group members that
 * works for both regular players AND bots. It solves the problem where
 * GroupReference::GetSource() returns nullptr for bots not properly
 * registered in ObjectAccessor.
 *
 * USAGE:
 * Replace:
 *   for (GroupReference& ref : group->GetMembers())
 *   {
 *       Player* member = ref.GetSource();  // May fail for bots!
 *       ...
 *   }
 *
 * With:
 *   for (Player* member : GroupMemberResolver::GetGroupMembers(group))
 *   {
 *       // member is guaranteed non-null
 *       ...
 *   }
 *
 * Or for range-based queries:
 *   auto members = GroupMemberResolver::GetGroupMembersInRange(bot, 40.0f);
 *
 * LOOKUP CHAIN:
 * 1. ObjectAccessor::FindPlayer()        - Fastest, works for most players
 * 2. ObjectAccessor::FindConnectedPlayer() - Works for connected players on other maps
 * 3. BotWorldSessionMgr::GetPlayerBot()  - Works for ALL bots
 *
 * THREAD SAFETY:
 * All methods are thread-safe and can be called from any thread.
 *
 * DIAGNOSTICS:
 * When GroupMemberDiagnostics is enabled, all lookups are tracked for
 * performance analysis and debugging.
 */

#ifndef PLAYERBOT_GROUP_MEMBER_RESOLVER_H
#define PLAYERBOT_GROUP_MEMBER_RESOLVER_H

#include "Define.h"
#include "ObjectGuid.h"
#include <vector>
#include <functional>

class Player;
class Group;
class Unit;

namespace Playerbot
{

/**
 * @class GroupMemberResolver
 * @brief Central utility for reliable group member lookups
 *
 * This class ensures that group member lookups work reliably for both
 * regular players and bots. It should be used instead of direct
 * GroupReference::GetSource() calls throughout the codebase.
 */
class TC_GAME_API GroupMemberResolver
{
public:
    // =====================================================================
    // CORE LOOKUP METHODS
    // =====================================================================

    /**
     * @brief Resolve a single group member by GUID
     *
     * This is the core lookup method that tries multiple strategies to
     * find a player/bot by GUID.
     *
     * @param guid The player/bot GUID to resolve
     * @return Player* if found, nullptr otherwise
     *
     * LOOKUP CHAIN:
     * 1. ObjectAccessor::FindPlayer() - Fast, same-map players
     * 2. ObjectAccessor::FindConnectedPlayer() - Connected players, any map
     * 3. BotWorldSessionMgr::GetPlayerBot() - Bot registry
     */
    static Player* ResolveMember(ObjectGuid guid);

    /**
     * @brief Resolve a member with diagnostic tracking
     *
     * Same as ResolveMember but records diagnostics when enabled.
     * Use this in critical code paths where you want to track lookup success.
     *
     * @param guid The player/bot GUID to resolve
     * @param callerFunc __FUNCTION__ of caller (for diagnostics)
     * @param callerFile __FILE__ of caller (for diagnostics)
     * @param callerLine __LINE__ of caller (for diagnostics)
     */
    static Player* ResolveMemberDiag(
        ObjectGuid guid,
        const char* callerFunc,
        const char* callerFile,
        uint32 callerLine);

    // =====================================================================
    // GROUP ITERATION METHODS
    // =====================================================================

    /**
     * @brief Get all members of a group (guaranteed non-null results)
     *
     * This replaces the common pattern of iterating group->GetMembers()
     * and checking for null. The returned vector contains only valid
     * Player pointers.
     *
     * @param group The group to iterate
     * @return Vector of non-null Player pointers
     *
     * USAGE:
     *   for (Player* member : GroupMemberResolver::GetGroupMembers(group))
     *   {
     *       // member is guaranteed non-null
     *   }
     */
    static std::vector<Player*> GetGroupMembers(Group* group);

    /**
     * @brief Get group members excluding a specific player
     *
     * Common pattern for getting "other" group members.
     *
     * @param group The group to iterate
     * @param exclude Player to exclude from results
     */
    static std::vector<Player*> GetGroupMembersExcept(Group* group, Player* exclude);

    /**
     * @brief Get group members within range of a player
     *
     * This is the most common pattern for combat/healing code.
     *
     * @param player The player to measure distance from
     * @param range Maximum distance in yards
     * @param includeSelf Whether to include the player in results
     */
    static std::vector<Player*> GetGroupMembersInRange(
        Player* player, 
        float range, 
        bool includeSelf = true);

    /**
     * @brief Get group members matching a filter predicate
     *
     * Flexible method for custom filtering logic.
     *
     * @param group The group to iterate
     * @param predicate Function that returns true for members to include
     */
    static std::vector<Player*> GetGroupMembersFiltered(
        Group* group,
        std::function<bool(Player*)> predicate);

    // =====================================================================
    // COMBAT-SPECIFIC HELPERS
    // =====================================================================

    /**
     * @brief Check if any group member is in combat
     *
     * @param group The group to check
     * @param excludePlayer Optional player to exclude from check
     * @return true if any member (except excluded) is in combat
     */
    static bool IsGroupInCombat(Group* group, Player* excludePlayer = nullptr);

    /**
     * @brief Find the first group member in combat
     *
     * @param group The group to check
     * @param excludePlayer Optional player to exclude from check
     * @return Player in combat, or nullptr if none
     */
    static Player* FindGroupMemberInCombat(Group* group, Player* excludePlayer = nullptr);

    /**
     * @brief Get all group members currently in combat
     *
     * @param group The group to check
     * @return Vector of members in combat
     */
    static std::vector<Player*> GetGroupMembersInCombat(Group* group);

    /**
     * @brief Get group members who need healing
     *
     * @param group The group to check
     * @param healthThreshold Health percentage threshold (default 90%)
     * @return Vector of members below health threshold, sorted by health
     */
    static std::vector<Player*> GetGroupMembersNeedingHealing(
        Group* group,
        float healthThreshold = 90.0f);

    // =====================================================================
    // ROLE-BASED HELPERS
    // =====================================================================

    /**
     * @brief Get the main tank(s) in the group
     *
     * @param group The group to check
     * @return Vector of players with tank role/spec
     */
    static std::vector<Player*> GetGroupTanks(Group* group);

    /**
     * @brief Get healers in the group
     *
     * @param group The group to check
     * @return Vector of players with healer role/spec
     */
    static std::vector<Player*> GetGroupHealers(Group* group);

    /**
     * @brief Get DPS in the group
     *
     * @param group The group to check
     * @return Vector of players with DPS role/spec
     */
    static std::vector<Player*> GetGroupDPS(Group* group);

    // =====================================================================
    // UTILITY METHODS
    // =====================================================================

    /**
     * @brief Check if a GUID belongs to a known bot
     *
     * @param guid The GUID to check
     * @return true if this GUID is registered as a bot
     */
    static bool IsBot(ObjectGuid guid);

    /**
     * @brief Check if a player is a bot
     *
     * @param player The player to check
     * @return true if this player is a bot
     */
    static bool IsBot(Player* player);

    /**
     * @brief Get the group leader (with reliable lookup)
     *
     * @param group The group
     * @return Group leader Player*, or nullptr
     */
    static Player* GetGroupLeader(Group* group);

    /**
     * @brief Count actual group members (not just slots)
     *
     * This counts members that can actually be resolved, not just
     * the number of slots in the group.
     *
     * @param group The group to count
     * @return Number of members that can be resolved
     */
    static uint32 GetResolvedMemberCount(Group* group);

private:
    GroupMemberResolver() = delete;  // Static-only class
};

// =====================================================================
// CONVENIENCE MACROS
// =====================================================================

/**
 * Macro for diagnostic-tracked member resolution
 * Usage: RESOLVE_GROUP_MEMBER(guid)
 */
#define RESOLVE_GROUP_MEMBER(guid) \
    Playerbot::GroupMemberResolver::ResolveMemberDiag(guid, __FUNCTION__, __FILE__, __LINE__)

/**
 * Macro for iterating group members with automatic null-safety
 * Usage: FOR_EACH_GROUP_MEMBER(group, member) { ... }
 */
#define FOR_EACH_GROUP_MEMBER(group, memberVar) \
    for (Player* memberVar : Playerbot::GroupMemberResolver::GetGroupMembers(group))

/**
 * Macro for iterating group members in range
 * Usage: FOR_EACH_MEMBER_IN_RANGE(player, 40.0f, member) { ... }
 */
#define FOR_EACH_MEMBER_IN_RANGE(player, range, memberVar) \
    for (Player* memberVar : Playerbot::GroupMemberResolver::GetGroupMembersInRange(player, range))

} // namespace Playerbot

#endif // PLAYERBOT_GROUP_MEMBER_RESOLVER_H
