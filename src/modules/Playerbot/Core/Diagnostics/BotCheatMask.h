/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * BotCheatMask: Debug/testing cheat system for bots. Allows selective
 * cheats per-bot (speed, damage, god mode, etc.) via bitmask flags.
 * Controlled via `.bot cheat` chat commands.
 *
 * Thread Safety: Per-bot flags accessed from bot's thread only.
 * Global enable/disable is atomic.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <mutex>
#include <string>
#include <vector>

class Player;

namespace Playerbot
{

/// Cheat flags as a bitmask
enum class BotCheatFlag : uint32
{
    NONE            = 0x00000000,

    // Movement cheats
    SPEED           = 0x00000001,  // 2x movement speed
    FLY             = 0x00000002,  // Enable flying
    NO_FALL_DAMAGE  = 0x00000004,  // Immune to fall damage
    TELEPORT        = 0x00000008,  // Instant teleport to target

    // Combat cheats
    DAMAGE          = 0x00000010,  // 10x damage output
    HEALTH          = 0x00000020,  // Infinite health (auto-heal to full)
    MANA            = 0x00000040,  // Infinite mana/resources
    COOLDOWNS       = 0x00000080,  // No spell cooldowns
    GOD_MODE        = 0x00000100,  // Immune to all damage
    ONE_SHOT        = 0x00000200,  // Kill targets in one hit
    INSTANT_CAST    = 0x00000400,  // All spells are instant cast

    // Utility cheats
    NO_AGGRO        = 0x00000800,  // NPCs won't aggro
    LOOT_ALL        = 0x00001000,  // Auto-loot everything
    UNLIMITED_BAG   = 0x00002000,  // Never run out of bag space
    XP_BOOST        = 0x00004000,  // 10x XP gain

    // Presets
    ALL_COMBAT      = DAMAGE | HEALTH | MANA | COOLDOWNS | GOD_MODE | ONE_SHOT | INSTANT_CAST,
    ALL_MOVEMENT    = SPEED | FLY | NO_FALL_DAMAGE | TELEPORT,
    ALL             = 0x00007FFF   // All flags
};

inline BotCheatFlag operator|(BotCheatFlag a, BotCheatFlag b)
{
    return static_cast<BotCheatFlag>(static_cast<uint32>(a) | static_cast<uint32>(b));
}

inline BotCheatFlag operator&(BotCheatFlag a, BotCheatFlag b)
{
    return static_cast<BotCheatFlag>(static_cast<uint32>(a) & static_cast<uint32>(b));
}

inline BotCheatFlag operator~(BotCheatFlag a)
{
    return static_cast<BotCheatFlag>(~static_cast<uint32>(a));
}

inline BotCheatFlag& operator|=(BotCheatFlag& a, BotCheatFlag b)
{
    return a = a | b;
}

inline BotCheatFlag& operator&=(BotCheatFlag& a, BotCheatFlag b)
{
    return a = a & b;
}

inline bool HasCheat(BotCheatFlag flags, BotCheatFlag check)
{
    return (static_cast<uint32>(flags) & static_cast<uint32>(check)) != 0;
}

/// Per-bot cheat state
struct BotCheatState
{
    BotCheatFlag flags = BotCheatFlag::NONE;
    float speedMultiplier = 2.0f;      // Custom speed when SPEED enabled
    float damageMultiplier = 10.0f;    // Custom damage when DAMAGE enabled
    float xpMultiplier = 10.0f;        // Custom XP when XP_BOOST enabled
};

/// Named cheat info for command parsing
struct CheatInfo
{
    char const* name;
    char const* description;
    BotCheatFlag flag;
};

/**
 * @class BotCheatMask
 * @brief Manages per-bot cheat flags for testing and debugging
 *
 * Usage from chat commands:
 *   .bot cheat speed          - Toggle speed boost
 *   .bot cheat god            - Toggle god mode
 *   .bot cheat all            - Enable all cheats
 *   .bot cheat off            - Disable all cheats
 *   .bot cheat list           - List active cheats
 *   .bot cheat damage 5.0     - Set custom damage multiplier
 */
class TC_GAME_API BotCheatMask
{
public:
    static BotCheatMask* instance()
    {
        static BotCheatMask inst;
        return &inst;
    }

    /// Initialize the cheat system
    void Initialize();

    // ========================================================================
    // Per-Bot Cheat Management
    // ========================================================================

    /// Set a cheat flag on a specific bot
    void EnableCheat(ObjectGuid botGuid, BotCheatFlag cheat);

    /// Clear a cheat flag on a specific bot
    void DisableCheat(ObjectGuid botGuid, BotCheatFlag cheat);

    /// Toggle a cheat flag on a specific bot
    void ToggleCheat(ObjectGuid botGuid, BotCheatFlag cheat);

    /// Set all cheat flags at once
    void SetCheats(ObjectGuid botGuid, BotCheatFlag cheats);

    /// Clear all cheats on a bot
    void ClearAllCheats(ObjectGuid botGuid);

    /// Clear all cheats on all bots
    void ClearAllBotCheats();

    // ========================================================================
    // Queries
    // ========================================================================

    /// Check if a specific cheat is active on a bot
    bool HasCheat(ObjectGuid botGuid, BotCheatFlag cheat) const;

    /// Get all active cheat flags for a bot
    BotCheatFlag GetCheats(ObjectGuid botGuid) const;

    /// Get the full cheat state for a bot
    BotCheatState GetCheatState(ObjectGuid botGuid) const;

    /// Check if any cheats are active on a bot
    bool HasAnyCheats(ObjectGuid botGuid) const;

    /// Get count of bots with active cheats
    uint32 GetCheatBotCount() const;

    // ========================================================================
    // Multiplier Configuration
    // ========================================================================

    /// Set custom speed multiplier for a bot
    void SetSpeedMultiplier(ObjectGuid botGuid, float mult);

    /// Set custom damage multiplier for a bot
    void SetDamageMultiplier(ObjectGuid botGuid, float mult);

    /// Set custom XP multiplier for a bot
    void SetXPMultiplier(ObjectGuid botGuid, float mult);

    /// Get speed multiplier (returns 1.0 if no cheat active)
    float GetSpeedMultiplier(ObjectGuid botGuid) const;

    /// Get damage multiplier (returns 1.0 if no cheat active)
    float GetDamageMultiplier(ObjectGuid botGuid) const;

    /// Get XP multiplier (returns 1.0 if no cheat active)
    float GetXPMultiplier(ObjectGuid botGuid) const;

    // ========================================================================
    // Cheat Application (called from BotAI/combat systems)
    // ========================================================================

    /// Apply cheat effects to a bot (called during bot update)
    void ApplyCheatEffects(Player* bot);

    /// Modify outgoing damage if DAMAGE cheat is active
    /// @return modified damage value
    uint32 ModifyDamage(ObjectGuid botGuid, uint32 baseDamage) const;

    /// Check if bot should take damage (false if GOD_MODE)
    bool ShouldTakeDamage(ObjectGuid botGuid) const;

    /// Check if spell should have a cooldown (false if COOLDOWNS cheat)
    bool ShouldHaveCooldown(ObjectGuid botGuid) const;

    // ========================================================================
    // Command Parsing
    // ========================================================================

    /// Get available cheat names and descriptions
    static std::vector<CheatInfo> const& GetCheatList();

    /// Parse a cheat name string to flag
    static BotCheatFlag ParseCheatName(std::string const& name);

    /// Get a cheat name from flag (single flag only)
    static char const* GetCheatName(BotCheatFlag flag);

    /// Format active cheats as a readable string
    std::string FormatActiveCheats(ObjectGuid botGuid) const;

private:
    BotCheatMask() = default;
    ~BotCheatMask() = default;
    BotCheatMask(BotCheatMask const&) = delete;
    BotCheatMask& operator=(BotCheatMask const&) = delete;

    std::unordered_map<ObjectGuid, BotCheatState> _botCheats;
    mutable std::mutex _mutex;
    bool _initialized = false;
};

#define sBotCheatMask BotCheatMask::instance()

} // namespace Playerbot
