/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * TransmogManager - Bot transmogrification and outfit management
 *
 * Manages transmog appearance collection and application for bots.
 * Bots collect appearances from equipped items and can apply themed
 * transmog sets based on their class and personality.
 */

#ifndef TRINITYCORE_BOT_TRANSMOG_MANAGER_H
#define TRINITYCORE_BOT_TRANSMOG_MANAGER_H

#include "Define.h"
#include "ObjectGuid.h"
#include "DBCEnums.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <shared_mutex>

class Player;
class Item;

namespace Playerbot
{

/**
 * @enum TransmogSlot
 * @brief Equipment slots that can be transmogrified
 */
enum class TransmogSlot : uint8
{
    HEAD        = 0,
    SHOULDER    = 1,
    CHEST       = 2,
    WAIST       = 3,
    LEGS        = 4,
    FEET        = 5,
    WRIST       = 6,
    HANDS       = 7,
    BACK        = 8,
    MAIN_HAND   = 9,
    OFF_HAND    = 10,
    TABARD      = 11,
    MAX
};

/**
 * @enum TransmogTheme
 * @brief Visual theme categories for transmog sets
 */
enum class TransmogTheme : uint8
{
    NONE        = 0,    // No specific theme
    TIER_SET    = 1,    // Class tier set appearance
    PVP_SET     = 2,    // PvP season gear
    PROFESSION  = 3,    // Profession-themed outfit
    CASUAL      = 4,    // City clothes / casual outfit
    MATCHING    = 5,    // Color-coordinated random pieces
    MAX
};

/**
 * @struct TransmogOutfit
 * @brief A saved transmog outfit (set of appearances per slot)
 */
struct TransmogOutfit
{
    std::string name;
    TransmogTheme theme = TransmogTheme::NONE;
    std::unordered_map<TransmogSlot, uint32> slotAppearances; // slot -> itemModifiedAppearanceId

    bool IsComplete() const
    {
        return slotAppearances.size() >= 6; // At least chest, legs, feet, hands, shoulder, head
    }
};

/**
 * @class TransmogManager
 * @brief Manages transmog collection and application for a single bot
 *
 * Features:
 * - Collect appearances from equipped/obtained items
 * - Save and load transmog outfits
 * - Apply themed transmog sets
 * - Periodically refresh transmog at transmogrifier NPCs
 *
 * Usage:
 * @code
 * TransmogManager transmogMgr(bot);
 * transmogMgr.CollectAppearancesFromEquipment();
 *
 * if (transmogMgr.HasSavedOutfit("raiding"))
 *     transmogMgr.ApplyOutfit("raiding");
 * @endcode
 */
class TC_GAME_API TransmogManager
{
public:
    explicit TransmogManager(Player* bot);
    ~TransmogManager() = default;

    /**
     * @brief Collect all appearances from currently equipped items
     *
     * Scans all equipment slots and adds item appearances to the
     * bot's collection via CollectionMgr.
     */
    void CollectAppearancesFromEquipment();

    /**
     * @brief Save current equipment look as a named outfit
     *
     * @param outfitName Name for the outfit
     * @param theme Optional visual theme tag
     * @return true if outfit saved successfully
     */
    bool SaveCurrentOutfit(std::string const& outfitName, TransmogTheme theme = TransmogTheme::NONE);

    /**
     * @brief Apply a saved outfit to current equipment
     *
     * Applies transmog from the named outfit to all matching slots.
     * Requires the bot to have the appearances in their collection.
     *
     * @param outfitName Name of the outfit to apply
     * @return true if transmog applied successfully
     */
    bool ApplyOutfit(std::string const& outfitName);

    /**
     * @brief Check if an outfit exists
     */
    bool HasSavedOutfit(std::string const& outfitName) const;

    /**
     * @brief Get list of saved outfit names
     */
    std::vector<std::string> GetSavedOutfits() const;

    /**
     * @brief Remove transmog from all slots (show actual gear)
     */
    void ClearTransmog();

    /**
     * @brief Get the number of unique appearances collected
     */
    uint32 GetCollectedAppearanceCount() const;

    /**
     * @brief Check if bot should visit transmogrifier during city life
     *
     * Returns true if bot has new gear that could be transmogged
     * and hasn't visited a transmogrifier recently.
     *
     * @return true if transmog visit is recommended
     */
    bool ShouldVisitTransmogrifier() const;

    /**
     * @brief Called when bot visits a transmogrifier NPC
     *
     * Applies a random saved outfit or generates one based on theme.
     */
    void OnTransmogrifierVisit();

    /**
     * @brief Generate a themed outfit from collected appearances
     *
     * Creates an outfit by selecting matching pieces from the
     * bot's appearance collection based on the given theme.
     *
     * @param theme Theme to generate
     * @param outfitName Name for the generated outfit
     * @return true if enough appearances found to create a valid outfit
     */
    bool GenerateThemedOutfit(TransmogTheme theme, std::string const& outfitName);

    /**
     * @brief Update transmog state (called periodically)
     *
     * @param diff Time since last update in milliseconds
     */
    void Update(uint32 diff);

private:
    Player* _bot;

    // Saved outfits for this bot
    std::unordered_map<std::string, TransmogOutfit> _savedOutfits;

    // Tracking
    uint32 _lastEquipmentHash = 0;      // Hash of equipped item IDs — detects gear changes
    uint32 _lastTransmogVisitMs = 0;    // Last time we visited a transmogrifier
    uint32 _lastUpdateMs = 0;

    static constexpr uint32 TRANSMOG_VISIT_COOLDOWN_MS = 3600000; // 1 hour between visits
    static constexpr uint32 UPDATE_INTERVAL_MS = 30000;           // Check every 30 seconds

    /**
     * @brief Calculate hash of current equipment for change detection
     */
    uint32 CalculateEquipmentHash() const;

    /**
     * @brief Convert equipment slot index to TransmogSlot
     */
    static TransmogSlot EquipmentSlotToTransmogSlot(uint8 equipSlot);

    /**
     * @brief Get the item appearance ID for a given item
     */
    static uint32 GetItemAppearanceId(Item const* item);
};

/**
 * @class TransmogCoordinator
 * @brief Singleton coordinator for bot transmog across all bots
 *
 * Manages global transmog state and ensures bots don't all have
 * identical transmog (personality-based variation).
 */
class TC_GAME_API TransmogCoordinator final
{
public:
    static TransmogCoordinator* instance()
    {
        static TransmogCoordinator inst;
        return &inst;
    }

    /**
     * @brief Get or create TransmogManager for a bot
     */
    TransmogManager* GetManager(Player* bot);

    /**
     * @brief Remove manager when bot logs out
     */
    void RemoveManager(ObjectGuid botGuid);

    /**
     * @brief Cleanup expired managers
     */
    void CleanupExpired(uint32 currentTimeMs);

private:
    TransmogCoordinator() = default;
    ~TransmogCoordinator() = default;
    TransmogCoordinator(const TransmogCoordinator&) = delete;
    TransmogCoordinator& operator=(const TransmogCoordinator&) = delete;

    mutable std::shared_mutex _mutex;
    std::unordered_map<ObjectGuid, std::unique_ptr<TransmogManager>> _managers;
};

#define sTransmogCoordinator TransmogCoordinator::instance()

} // namespace Playerbot

#endif // TRINITYCORE_BOT_TRANSMOG_MANAGER_H
