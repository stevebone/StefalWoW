/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * CONSUMABLE MANAGER - Pre-combat buffing + combat potion usage
 *
 * Manages all consumable usage for bots:
 * - Pre-combat: Flasks/Phials, Food buffs, Augment Runes
 * - In-combat: Health potions, Mana potions, DPS/Healing potions
 * - Context-aware: Selects consumables based on spec, role, and content type
 *
 * Architecture:
 * - Per-bot instance owned by GameSystemsManager
 * - Throttled updates (5s out of combat, 500ms in combat)
 * - Scan inventory once, cache available consumables
 * - Integrates with CombatBehaviorIntegration for emergency usage
 */

#ifndef _PLAYERBOT_CONSUMABLE_MANAGER_H
#define _PLAYERBOT_CONSUMABLE_MANAGER_H

#include "Define.h"
#include <array>
#include <string>
#include <vector>
#include <unordered_map>

class Player;
class Item;

namespace Playerbot
{

class BotAI;

// ============================================================================
// CONSUMABLE CATEGORIES
// ============================================================================

enum class ConsumableCategory : uint8
{
    FLASK           = 0,    // Long-duration stat buff (1 hour, persists through death)
    PHIAL           = 1,    // TWW equivalent of flask
    FOOD            = 2,    // Well Fed stat buff (requires 10s eating channel)
    AUGMENT_RUNE    = 3,    // Primary stat buff (Veiled Augment Rune etc.)
    HEALTH_POTION   = 4,    // Emergency health restore (shared combat potion CD)
    MANA_POTION     = 5,    // Emergency mana restore (shared combat potion CD)
    DPS_POTION      = 6,    // Temporary DPS boost (shared combat potion CD)
    HEAL_POTION     = 7,    // Temporary healing boost (shared combat potion CD)
    HEALTHSTONE     = 8,    // Warlock healthstone (separate CD from potions)
    WEAPON_BUFF     = 9,    // Temporary weapon enchants (sharpening stones, etc.)
    BATTLE_ELIXIR   = 10,   // Battle elixir (if no flask active)
    GUARDIAN_ELIXIR = 11,   // Guardian elixir (if no flask active)
    MAX_CATEGORY
};

enum class ConsumableRole : uint8
{
    ANY         = 0,
    TANK        = 1,
    HEALER      = 2,
    MELEE_DPS   = 3,
    RANGED_DPS  = 4,
    CASTER_DPS  = 5
};

enum class ContentType : uint8
{
    OPEN_WORLD  = 0,    // Solo/questing - minimal consumable usage
    DUNGEON     = 1,    // 5-man content - food + flask for bosses
    RAID        = 2,    // Raid content - full consumable usage
    PVP         = 3,    // Battleground/Arena - health potions priority
    DELVE       = 4     // Delve content - moderate consumable usage
};

// ============================================================================
// CONSUMABLE DATA
// ============================================================================

struct ConsumableInfo
{
    uint32 itemId = 0;              // Item template entry
    ConsumableCategory category = ConsumableCategory::FLASK;
    ConsumableRole role = ConsumableRole::ANY;
    uint32 auraId = 0;              // Aura applied when consumed (for checking if already active)
    uint32 priority = 0;            // Higher = better quality (used for selection)
    std::string name;               // Display name for logging
};

struct ConsumableState
{
    bool hasFlaskOrPhial = false;
    bool hasFoodBuff = false;
    bool hasAugmentRune = false;
    bool hasWeaponBuff = false;
    bool hasBattleElixir = false;
    bool hasGuardianElixir = false;
    bool potionOnCooldown = false;
    bool healthstoneOnCooldown = false;
    uint32 lastPotionUseTime = 0;
    uint32 lastHealthstoneUseTime = 0;
    uint32 lastFoodBuffTime = 0;    // When we last started eating
};

// ============================================================================
// CONSUMABLE MANAGER
// ============================================================================

class TC_GAME_API ConsumableManager
{
public:
    explicit ConsumableManager(Player* bot, BotAI* ai);
    ~ConsumableManager() = default;

    // Non-copyable, non-movable
    ConsumableManager(const ConsumableManager&) = delete;
    ConsumableManager& operator=(const ConsumableManager&) = delete;
    ConsumableManager(ConsumableManager&&) = delete;
    ConsumableManager& operator=(ConsumableManager&&) = delete;

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize the consumable manager
     * Scans inventory and builds available consumable cache
     */
    void Initialize();

    /**
     * @brief Update consumable state and apply buffs as needed
     * @param diff Time since last update in milliseconds
     *
     * Out of combat: Checks for missing pre-combat buffs (5s throttle)
     * In combat: Monitors health/mana for emergency potion usage (500ms throttle)
     */
    void Update(uint32 diff);

    // ========================================================================
    // PRE-COMBAT BUFFING
    // ========================================================================

    /**
     * @brief Check and apply all missing pre-combat buffs
     * @return true if a consumable was used this tick (requires GCD/channel)
     *
     * Priority order:
     * 1. Flask/Phial (if in dungeon/raid/delve content)
     * 2. Food buff (if not in combat, requires 10s channel)
     * 3. Augment Rune (if in raid content)
     * 4. Battle/Guardian Elixir (if no flask and in group content)
     */
    bool ApplyPreCombatBuffs();

    /**
     * @brief Check if bot is missing any pre-combat consumable buffs
     * @return true if any expected buff is missing
     */
    [[nodiscard]] bool IsMissingPreCombatBuffs() const;

    // ========================================================================
    // COMBAT CONSUMABLE USAGE
    // ========================================================================

    /**
     * @brief Use emergency health potion/healthstone
     * @return true if a health consumable was used
     *
     * Called when health drops below threshold:
     * - Healthstone at <= 35% HP (separate CD from potions)
     * - Health potion at <= 30% HP
     */
    bool UseEmergencyHealthConsumable();

    /**
     * @brief Use emergency mana potion
     * @return true if a mana potion was used
     *
     * Called when mana drops below 20% for healers/casters
     */
    bool UseEmergencyManaConsumable();

    /**
     * @brief Use DPS/healing combat potion during burst window
     * @return true if a combat potion was used
     *
     * Called during burst windows (Bloodlust/Heroism, cooldown stacking)
     * Only for raid/dungeon content
     */
    bool UseCombatPotion();

    // ========================================================================
    // STATE QUERIES
    // ========================================================================

    /**
     * @brief Check if the combat potion is on cooldown
     * @return true if potion CD is active (5 minute shared CD)
     */
    [[nodiscard]] bool IsPotionOnCooldown() const;

    /**
     * @brief Check if the healthstone is on cooldown
     * @return true if healthstone CD is active (1 minute CD)
     */
    [[nodiscard]] bool IsHealthstoneOnCooldown() const;

    /**
     * @brief Get the current content type based on bot's location
     * @return ContentType enum value
     */
    [[nodiscard]] ContentType GetCurrentContentType() const;

    /**
     * @brief Get the bot's consumable role based on spec
     * @return ConsumableRole enum value
     */
    [[nodiscard]] ConsumableRole GetConsumableRole() const;

    /**
     * @brief Get current consumable state (for external inspection)
     * @return Reference to internal state
     */
    [[nodiscard]] const ConsumableState& GetState() const { return _state; }

    /**
     * @brief Refresh the inventory cache
     * Call after looting, trading, or vendor purchases
     */
    void RefreshInventoryCache();

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Scan bot inventory for available consumables
     * Populates _availableConsumables by category
     */
    void ScanInventory();

    /**
     * @brief Update current buff state by checking active auras
     */
    void UpdateBuffState();

    /**
     * @brief Determine content type from bot's current map/instance
     */
    ContentType DetermineContentType() const;

    /**
     * @brief Determine consumable role from bot's class/spec
     */
    ConsumableRole DetermineConsumableRole() const;

    /**
     * @brief Try to use an item by entry ID
     * @param itemId Item template entry
     * @return true if item was successfully used
     */
    bool TryUseItem(uint32 itemId);

    /**
     * @brief Find the best available consumable for a category
     * @param category The consumable category to search
     * @param role Optional role filter (ANY = no filter)
     * @return Item entry ID, or 0 if none available
     */
    [[nodiscard]] uint32 FindBestConsumable(ConsumableCategory category,
                                             ConsumableRole role = ConsumableRole::ANY) const;

    /**
     * @brief Check if bot currently has a specific aura
     * @param auraId Spell ID to check
     * @return true if aura is present
     */
    [[nodiscard]] bool HasAura(uint32 auraId) const;

    /**
     * @brief Check if any aura from a list is active on the bot
     * @param auraIds Array of aura spell IDs to check
     * @param count Number of entries in the array
     * @return true if any aura from the list is present
     */
    [[nodiscard]] bool HasAnyAura(const uint32* auraIds, size_t count) const;

    /**
     * @brief Check if bot should use consumables in current content
     * @return true if content is meaningful enough for consumable usage
     */
    [[nodiscard]] bool ShouldUseConsumablesForContent() const;

    /**
     * @brief Check if bot is currently eating/drinking
     * @return true if bot has food/drink channel active
     */
    [[nodiscard]] bool IsEatingOrDrinking() const;

    // ========================================================================
    // STATIC DATA - Consumable databases
    // ========================================================================

    /**
     * @brief Get the database of known flask/phial consumables
     */
    static const std::vector<ConsumableInfo>& GetFlaskDatabase();

    /**
     * @brief Get the database of known food consumables
     */
    static const std::vector<ConsumableInfo>& GetFoodDatabase();

    /**
     * @brief Get the database of known augment rune consumables
     */
    static const std::vector<ConsumableInfo>& GetAugmentRuneDatabase();

    /**
     * @brief Get the database of known health potion consumables
     */
    static const std::vector<ConsumableInfo>& GetHealthPotionDatabase();

    /**
     * @brief Get the database of known mana potion consumables
     */
    static const std::vector<ConsumableInfo>& GetManaPotionDatabase();

    /**
     * @brief Get the database of known DPS combat potion consumables
     */
    static const std::vector<ConsumableInfo>& GetDPSPotionDatabase();

    /**
     * @brief Get the database of known healthstone consumables
     */
    static const std::vector<ConsumableInfo>& GetHealthstoneDatabase();

    // ========================================================================
    // MEMBER DATA
    // ========================================================================

    Player* _bot;                   // Non-owning reference to bot player
    BotAI* _ai;                    // Non-owning reference to BotAI

    ConsumableState _state;         // Current consumable state
    ContentType _contentType;       // Cached content type
    ConsumableRole _role;           // Cached consumable role

    // Inventory cache: category -> list of available item IDs with priorities
    std::unordered_map<ConsumableCategory, std::vector<ConsumableInfo>> _availableConsumables;

    // Timers
    uint32 _outOfCombatUpdateTimer = 0;
    uint32 _inCombatUpdateTimer = 0;
    uint32 _inventoryScanTimer = 0;
    uint32 _lastInventoryScan = 0;

    bool _initialized = false;

    // Constants
    static constexpr uint32 OUT_OF_COMBAT_UPDATE_INTERVAL = 5000;   // 5 seconds
    static constexpr uint32 IN_COMBAT_UPDATE_INTERVAL = 500;         // 500ms
    static constexpr uint32 INVENTORY_SCAN_INTERVAL = 30000;         // 30 seconds
    static constexpr uint32 POTION_COOLDOWN_MS = 300000;             // 5 minutes
    static constexpr uint32 HEALTHSTONE_COOLDOWN_MS = 60000;         // 1 minute
    static constexpr uint32 FOOD_CHANNEL_TIME_MS = 10000;            // 10 second channel
    static constexpr float  HEALTH_POTION_THRESHOLD = 30.0f;         // Use at 30% HP
    static constexpr float  HEALTHSTONE_THRESHOLD = 35.0f;           // Use at 35% HP
    static constexpr float  MANA_POTION_THRESHOLD = 20.0f;           // Use at 20% mana
};

} // namespace Playerbot

#endif // _PLAYERBOT_CONSUMABLE_MANAGER_H
