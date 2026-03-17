/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * GroupBuffCoordinator - Prevents duplicate raid-wide buff casting
 *
 * In WoW 12.0, most class buffs (Arcane Intellect, Power Word: Fortitude,
 * Mark of the Wild, Battle Shout) are raid-wide. When multiple bots of the
 * same class are in a group, they can waste GCDs by all trying to cast the
 * same buff simultaneously.
 *
 * This coordinator uses a claim-based system: before casting a raid-wide buff,
 * a bot "claims" the buff responsibility. Other bots see the claim and skip.
 * Claims expire after a short window to handle failed casts.
 */

#ifndef TRINITYCORE_GROUP_BUFF_COORDINATOR_H
#define TRINITYCORE_GROUP_BUFF_COORDINATOR_H

#include "Define.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <shared_mutex>
#include <atomic>

class Player;
class Group;

namespace Playerbot
{

/**
 * @enum RaidBuffCategory
 * @brief Categories of raid-wide buffs in WoW 12.0
 *
 * Each category represents a unique buff effect. Only one instance
 * of each category is needed per group/raid.
 */
enum class RaidBuffCategory : uint8
{
    INTELLECT       = 0,    // Arcane Intellect (Mage)
    STAMINA         = 1,    // Power Word: Fortitude (Priest)
    VERSATILITY     = 2,    // Mark of the Wild (Druid)
    ATTACK_POWER    = 3,    // Battle Shout (Warrior)
    PHYSICAL_DMG    = 4,    // Mystic Touch (Monk - 5% physical damage taken)
    MAGIC_DMG       = 5,    // Chaos Brand (DH - 5% magic damage taken)
    MOVEMENT_SPEED  = 6,    // Blessing of the Bronze (Evoker - 30% movement)
    MAX
};

/**
 * @brief Mapping from spell ID to buff category
 */
struct RaidBuffInfo
{
    uint32 spellId;
    RaidBuffCategory category;
    uint8 providerClassId;       // CLASS_MAGE, CLASS_PRIEST, etc.
    const char* name;
};

/**
 * @brief Known WoW 12.0 raid-wide buff spells
 */
namespace RaidBuffs
{
    // Spell IDs for raid-wide buffs
    constexpr uint32 ARCANE_INTELLECT       = 1459;
    constexpr uint32 POWER_WORD_FORTITUDE   = 21562;
    constexpr uint32 MARK_OF_THE_WILD       = 1126;
    constexpr uint32 BATTLE_SHOUT           = 6673;
    constexpr uint32 MYSTIC_TOUCH           = 8647;
    constexpr uint32 CHAOS_BRAND            = 1490;
    constexpr uint32 BLESSING_OF_THE_BRONZE = 381748;

    // Lookup table
    static constexpr RaidBuffInfo ALL_BUFFS[] =
    {
        { ARCANE_INTELLECT,       RaidBuffCategory::INTELLECT,      8,  "Arcane Intellect" },
        { POWER_WORD_FORTITUDE,   RaidBuffCategory::STAMINA,        5,  "Power Word: Fortitude" },
        { MARK_OF_THE_WILD,       RaidBuffCategory::VERSATILITY,    11, "Mark of the Wild" },
        { BATTLE_SHOUT,           RaidBuffCategory::ATTACK_POWER,   1,  "Battle Shout" },
        { MYSTIC_TOUCH,           RaidBuffCategory::PHYSICAL_DMG,   10, "Mystic Touch" },
        { CHAOS_BRAND,            RaidBuffCategory::MAGIC_DMG,      12, "Chaos Brand" },
        { BLESSING_OF_THE_BRONZE, RaidBuffCategory::MOVEMENT_SPEED, 13, "Blessing of the Bronze" },
    };

    static constexpr size_t BUFF_COUNT = sizeof(ALL_BUFFS) / sizeof(ALL_BUFFS[0]);

    /**
     * @brief Get buff category for a spell ID
     * @return Category or MAX if not a tracked raid buff
     */
    inline RaidBuffCategory GetCategory(uint32 spellId)
    {
        for (size_t i = 0; i < BUFF_COUNT; ++i)
        {
            if (ALL_BUFFS[i].spellId == spellId)
                return ALL_BUFFS[i].category;
        }
        return RaidBuffCategory::MAX;
    }

    /**
     * @brief Check if a spell is a tracked raid buff
     */
    inline bool IsRaidBuff(uint32 spellId)
    {
        return GetCategory(spellId) != RaidBuffCategory::MAX;
    }

    /**
     * @brief Get the spell ID for a given category
     */
    inline uint32 GetSpellForCategory(RaidBuffCategory category)
    {
        for (size_t i = 0; i < BUFF_COUNT; ++i)
        {
            if (ALL_BUFFS[i].category == category)
                return ALL_BUFFS[i].spellId;
        }
        return 0;
    }
}

/**
 * @struct BuffClaim
 * @brief A bot's claim to cast a specific buff category
 */
struct BuffClaim
{
    ObjectGuid claimerGuid;     // Bot that claimed the buff
    uint32 claimTimeMs;         // When the claim was made
    uint32 spellId;             // Specific spell being cast

    static constexpr uint32 CLAIM_EXPIRY_MS = 5000; // 5 seconds — enough for cast + GCD

    [[nodiscard]] bool IsExpired(uint32 currentTimeMs) const
    {
        return (currentTimeMs - claimTimeMs) > CLAIM_EXPIRY_MS;
    }
};

/**
 * @class GroupBuffCoordinator
 * @brief Singleton coordinator preventing duplicate raid buff casting
 *
 * Thread-safe. Claims are tracked per group instance ID + buff category.
 * When a bot wants to cast a raid buff, it calls TryClaimBuff().
 * If granted, the bot proceeds. Otherwise, it skips — another bot is handling it.
 *
 * Usage in ClassAI UpdateBuffs():
 * @code
 * if (!bot->HasAura(ARCANE_INTELLECT))
 * {
 *     if (sGroupBuffCoordinator->TryClaimBuff(bot, ARCANE_INTELLECT))
 *     {
 *         CastSpell(ARCANE_INTELLECT, bot);
 *     }
 *     // else: another bot is already casting this buff
 * }
 * @endcode
 */
class TC_GAME_API GroupBuffCoordinator final
{
public:
    static GroupBuffCoordinator* instance()
    {
        static GroupBuffCoordinator inst;
        return &inst;
    }

    /**
     * @brief Try to claim responsibility for casting a raid buff
     *
     * @param bot The bot that wants to cast
     * @param spellId The buff spell ID
     * @return true if this bot should proceed with casting
     * @return false if another bot already has a valid claim
     *
     * Also checks if ANY group member already has the buff active,
     * in which case no claim is needed and returns false.
     */
    bool TryClaimBuff(Player const* bot, uint32 spellId);

    /**
     * @brief Notify that a buff was successfully applied
     *
     * Clears the claim since the buff is now active.
     * Called after successful cast.
     *
     * @param bot The bot that cast the buff
     * @param spellId The buff spell ID
     */
    void OnBuffApplied(Player const* bot, uint32 spellId);

    /**
     * @brief Check if a group member already has a specific buff active
     *
     * Scans the group for any member with the given buff aura.
     * More efficient than each bot scanning independently.
     *
     * @param bot Bot whose group to check
     * @param spellId Buff spell ID
     * @return true if any group member has the buff
     */
    bool IsBuffActiveInGroup(Player const* bot, uint32 spellId) const;

    /**
     * @brief Get list of missing buff categories for a group
     *
     * Scans the group and returns which buff categories are not covered.
     * Useful for determining which buffs a bot should provide.
     *
     * @param bot Bot whose group to analyze
     * @return Vector of missing buff categories
     */
    std::vector<RaidBuffCategory> GetMissingBuffs(Player const* bot) const;

    /**
     * @brief Check which buff category a specific bot should provide
     *
     * Considers the bot's class and what buffs the group is missing.
     * Returns the highest priority missing buff this bot can provide.
     *
     * @param bot Bot to check
     * @return Spell ID of the buff to cast, or 0 if none needed
     */
    uint32 GetBuffToCast(Player const* bot) const;

    /**
     * @brief Clear all claims for a group
     *
     * Called when a group disbands or composition changes.
     *
     * @param groupId The group instance ID
     */
    void ClearGroupClaims(uint32 groupId);

    /**
     * @brief Periodic cleanup of expired claims
     *
     * Called from the module update loop.
     * Removes stale claims from disconnected bots or failed casts.
     *
     * @param currentTimeMs Current game time in milliseconds
     */
    void CleanupExpiredClaims(uint32 currentTimeMs);

private:
    GroupBuffCoordinator() = default;
    ~GroupBuffCoordinator() = default;
    GroupBuffCoordinator(const GroupBuffCoordinator&) = delete;
    GroupBuffCoordinator& operator=(const GroupBuffCoordinator&) = delete;

    // Key: (groupId << 8) | buffCategory — unique per group + buff combo
    using ClaimKey = uint64;

    static ClaimKey MakeKey(uint32 groupId, RaidBuffCategory category)
    {
        return (static_cast<uint64>(groupId) << 8) | static_cast<uint8>(category);
    }

    mutable std::shared_mutex _mutex;
    std::unordered_map<ClaimKey, BuffClaim> _claims;

    // Cleanup tracking
    uint32 _lastCleanupMs = 0;
    static constexpr uint32 CLEANUP_INTERVAL_MS = 10000; // 10 seconds
};

#define sGroupBuffCoordinator GroupBuffCoordinator::instance()

} // namespace Playerbot

#endif // TRINITYCORE_GROUP_BUFF_COORDINATOR_H
