/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_MAJOR_COOLDOWN_DATABASE_H
#define PLAYERBOT_MAJOR_COOLDOWN_DATABASE_H

#include "CooldownEvents.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

namespace Playerbot
{

/**
 * @brief Major cooldown information
 */
struct MajorCooldownInfo
{
    uint32 spellId;
    MajorCooldownTier tier;
    uint32 baseCooldownMs;      // Base cooldown duration in milliseconds
    std::string name;           // For debugging/logging
    bool isGroupWide;           // Affects entire group/raid
    bool requiresTarget;        // Requires a target (external CDs)
};

/**
 * @brief Database of major raid/group cooldowns
 *
 * This database contains all spells that are considered "major" cooldowns
 * worthy of coordination between bots. It's used by MajorCooldownTracker
 * to detect when important cooldowns are used and become available.
 *
 * Categories:
 * - RAID_OFFENSIVE: Bloodlust/Heroism, Power Infusion
 * - RAID_DEFENSIVE: Rallying Cry, Spirit Link, Darkness, AMZ
 * - EXTERNAL_MAJOR: Guardian Spirit, Pain Suppression, Ironbark, Life Cocoon
 * - EXTERNAL_MODERATE: Blessing of Sacrifice, Vigilance
 * - RESURRECTION: Battle Res (Rebirth, Soulstone, Raise Ally)
 */
class MajorCooldownDatabase
{
public:
    static MajorCooldownDatabase* instance()
    {
        static MajorCooldownDatabase instance;
        return &instance;
    }

    /**
     * @brief Check if a spell is a major cooldown
     * @param spellId The spell ID to check
     * @return true if it's a major cooldown, false otherwise
     */
    bool IsMajorCooldown(uint32 spellId) const
    {
        return _majorCooldowns.find(spellId) != _majorCooldowns.end();
    }

    /**
     * @brief Get the tier of a major cooldown
     * @param spellId The spell ID
     * @return The cooldown tier, or NONE if not a major cooldown
     */
    MajorCooldownTier GetCooldownTier(uint32 spellId) const
    {
        auto it = _majorCooldowns.find(spellId);
        return it != _majorCooldowns.end() ? it->second.tier : MajorCooldownTier::NONE;
    }

    /**
     * @brief Get full cooldown info
     * @param spellId The spell ID
     * @return Pointer to cooldown info, or nullptr if not found
     */
    MajorCooldownInfo const* GetCooldownInfo(uint32 spellId) const
    {
        auto it = _majorCooldowns.find(spellId);
        return it != _majorCooldowns.end() ? &it->second : nullptr;
    }

    /**
     * @brief Get all major cooldowns of a specific tier
     * @param tier The tier to filter by
     * @return Vector of spell IDs
     */
    std::vector<uint32> GetCooldownsByTier(MajorCooldownTier tier) const
    {
        std::vector<uint32> result;
        for (auto const& [spellId, info] : _majorCooldowns)
        {
            if (info.tier == tier)
                result.push_back(spellId);
        }
        return result;
    }

    /**
     * @brief Get all group-wide cooldowns (affects entire raid/group)
     * @return Vector of spell IDs
     */
    std::vector<uint32> GetGroupWideCooldowns() const
    {
        std::vector<uint32> result;
        for (auto const& [spellId, info] : _majorCooldowns)
        {
            if (info.isGroupWide)
                result.push_back(spellId);
        }
        return result;
    }

    /**
     * @brief Get all external cooldowns (target another player)
     * @return Vector of spell IDs
     */
    std::vector<uint32> GetExternalCooldowns() const
    {
        std::vector<uint32> result;
        for (auto const& [spellId, info] : _majorCooldowns)
        {
            if (info.requiresTarget)
                result.push_back(spellId);
        }
        return result;
    }

private:
    MajorCooldownDatabase()
    {
        InitializeCooldowns();
    }

    void InitializeCooldowns()
    {
        // ============================================================================
        // RAID OFFENSIVE COOLDOWNS
        // ============================================================================

        // Bloodlust / Heroism / Time Warp / Ancient Hysteria / Primal Rage / Drums
        RegisterCooldown(2825,   MajorCooldownTier::RAID_OFFENSIVE, 300000, "Bloodlust",           true,  false);
        RegisterCooldown(32182,  MajorCooldownTier::RAID_OFFENSIVE, 300000, "Heroism",             true,  false);
        RegisterCooldown(80353,  MajorCooldownTier::RAID_OFFENSIVE, 300000, "Time Warp",           true,  false);
        RegisterCooldown(90355,  MajorCooldownTier::RAID_OFFENSIVE, 300000, "Ancient Hysteria",    true,  false);
        RegisterCooldown(264667, MajorCooldownTier::RAID_OFFENSIVE, 300000, "Primal Rage",         true,  false);

        // Power Infusion
        RegisterCooldown(10060,  MajorCooldownTier::RAID_OFFENSIVE, 120000, "Power Infusion",      false, true);

        // Innervate
        RegisterCooldown(29166,  MajorCooldownTier::RAID_OFFENSIVE, 180000, "Innervate",           false, true);

        // ============================================================================
        // RAID DEFENSIVE COOLDOWNS
        // ============================================================================

        // Rallying Cry (Warrior)
        RegisterCooldown(97462,  MajorCooldownTier::RAID_DEFENSIVE, 180000, "Rallying Cry",        true,  false);

        // Spirit Link Totem (Shaman)
        RegisterCooldown(98008,  MajorCooldownTier::RAID_DEFENSIVE, 180000, "Spirit Link Totem",   true,  false);

        // Darkness (Demon Hunter)
        RegisterCooldown(196718, MajorCooldownTier::RAID_DEFENSIVE, 180000, "Darkness",            true,  false);

        // Anti-Magic Zone (Death Knight)
        RegisterCooldown(51052,  MajorCooldownTier::RAID_DEFENSIVE, 120000, "Anti-Magic Zone",     true,  false);

        // Power Word: Barrier (Priest)
        RegisterCooldown(62618,  MajorCooldownTier::RAID_DEFENSIVE, 180000, "Power Word: Barrier", true,  false);

        // Aura Mastery (Paladin)
        RegisterCooldown(31821,  MajorCooldownTier::RAID_DEFENSIVE, 180000, "Aura Mastery",        true,  false);

        // Healing Tide Totem (Shaman)
        RegisterCooldown(108280, MajorCooldownTier::RAID_DEFENSIVE, 180000, "Healing Tide Totem",  true,  false);

        // Tranquility (Druid)
        RegisterCooldown(740,    MajorCooldownTier::RAID_DEFENSIVE, 180000, "Tranquility",         true,  false);

        // Revival (Monk)
        RegisterCooldown(115310, MajorCooldownTier::RAID_DEFENSIVE, 180000, "Revival",             true,  false);

        // Divine Hymn (Priest)
        RegisterCooldown(64843,  MajorCooldownTier::RAID_DEFENSIVE, 180000, "Divine Hymn",         true,  false);

        // ============================================================================
        // EXTERNAL MAJOR COOLDOWNS
        // ============================================================================

        // Guardian Spirit (Priest)
        RegisterCooldown(47788,  MajorCooldownTier::EXTERNAL_MAJOR, 180000, "Guardian Spirit",     false, true);

        // Pain Suppression (Priest)
        RegisterCooldown(33206,  MajorCooldownTier::EXTERNAL_MAJOR, 180000, "Pain Suppression",    false, true);

        // Ironbark (Druid)
        RegisterCooldown(102342, MajorCooldownTier::EXTERNAL_MAJOR, 90000,  "Ironbark",            false, true);

        // Life Cocoon (Monk)
        RegisterCooldown(116849, MajorCooldownTier::EXTERNAL_MAJOR, 120000, "Life Cocoon",         false, true);

        // Blessing of Protection (Paladin)
        RegisterCooldown(1022,   MajorCooldownTier::EXTERNAL_MAJOR, 300000, "Blessing of Protection", false, true);

        // Blessing of Spellwarding (Paladin)
        RegisterCooldown(204018, MajorCooldownTier::EXTERNAL_MAJOR, 180000, "Blessing of Spellwarding", false, true);

        // ============================================================================
        // EXTERNAL MODERATE COOLDOWNS
        // ============================================================================

        // Blessing of Sacrifice (Paladin)
        RegisterCooldown(6940,   MajorCooldownTier::EXTERNAL_MODERATE, 120000, "Blessing of Sacrifice", false, true);

        // Vigilance (Warrior)
        RegisterCooldown(114030, MajorCooldownTier::EXTERNAL_MODERATE, 120000, "Vigilance",         false, true);

        // ============================================================================
        // PERSONAL MAJOR COOLDOWNS (for awareness, not typically coordinated)
        // ============================================================================

        // Ice Block (Mage)
        RegisterCooldown(45438,  MajorCooldownTier::PERSONAL_MAJOR, 240000, "Ice Block",           false, false);

        // Divine Shield (Paladin)
        RegisterCooldown(642,    MajorCooldownTier::PERSONAL_MAJOR, 300000, "Divine Shield",       false, false);

        // ============================================================================
        // RESURRECTION COOLDOWNS (Battle Res)
        // ============================================================================

        // Rebirth (Druid)
        RegisterCooldown(20484,  MajorCooldownTier::RESURRECTION, 0, "Rebirth",                    false, true);

        // Raise Ally (Death Knight)
        RegisterCooldown(61999,  MajorCooldownTier::RESURRECTION, 0, "Raise Ally",                 false, true);

        // Soulstone (Warlock) - Note: Pre-combat, spell ID for actual resurrection
        RegisterCooldown(20707,  MajorCooldownTier::RESURRECTION, 0, "Soulstone",                  false, true);

        // Engineering Battle Res (if applicable)
        // RegisterCooldown(XXXXX, MajorCooldownTier::RESURRECTION, 0, "Engineering Battle Res", false, true);
    }

    void RegisterCooldown(uint32 spellId, MajorCooldownTier tier, uint32 cooldownMs,
                         std::string const& name, bool isGroupWide, bool requiresTarget)
    {
        _majorCooldowns[spellId] = MajorCooldownInfo{
            spellId, tier, cooldownMs, name, isGroupWide, requiresTarget
        };
    }

    std::unordered_map<uint32, MajorCooldownInfo> _majorCooldowns;
};

} // namespace Playerbot

#endif // PLAYERBOT_MAJOR_COOLDOWN_DATABASE_H
