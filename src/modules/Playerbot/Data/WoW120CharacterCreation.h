/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * WoW 12.0 (The War Within Season 2+) Character Creation Data
 * Complete implementation guide for bot character creation
 *
 * WoW 12.0 Changes:
 * - Spirit stat re-added (MAX_STATS = 5)
 * - Difficulty type changed to int16
 * - Item Level Squish Era system added
 */

#ifndef PLAYERBOT_WOW120_CHARACTER_CREATION_H
#define PLAYERBOT_WOW120_CHARACTER_CREATION_H

#include "Define.h"
#include "SharedDefines.h"
#include "Position.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Playerbot
{

// ===================================================================================
// WOW 12.0 CHARACTER CREATION DATA
// ===================================================================================

namespace CharacterCreation
{
    // Starting level for new characters in WoW 12.0
    constexpr uint8 DEFAULT_STARTING_LEVEL = 10;  // New characters start at level 10 in TWW
    constexpr uint8 ALLIED_RACE_STARTING_LEVEL = 10;  // Allied races also start at 10
    constexpr uint8 DEMON_HUNTER_STARTING_LEVEL = 10;  // DH normalized to 10
    constexpr uint8 DEATH_KNIGHT_STARTING_LEVEL = 10;  // DK normalized to 10
    constexpr uint8 EVOKER_STARTING_LEVEL = 58;  // Evokers start at 58 in Forbidden Reach
    constexpr uint8 HERO_CLASS_MIN_LEVEL_REQUIREMENT = 0;  // No level requirement in 12.0

    // Maximum number of certain classes per realm
    constexpr uint8 MAX_EVOKERS_PER_REALM = 1;  // Only 1 Evoker per realm initially (lifted after first max level)
    constexpr uint8 MAX_DEMON_HUNTERS_PER_REALM = 1;  // Only 1 DH per realm (can be lifted)

    // ===================================================================================
    // RACE/CLASS COMBINATIONS (WoW 12.0 - The War Within Season 2+)
    // ===================================================================================

    struct RaceClassCombination
    {
        Races race;
        Classes playerClass;
        bool isAlliedRace;
        bool requiresUnlock;
        uint32 unlockAchievement;  // Achievement ID required to unlock (0 if none)
    };

    // Valid Race/Class combinations for WoW 12.0
    // Note: Almost all race/class restrictions have been lifted in recent expansions
    inline const ::std::vector<RaceClassCombination> VALID_COMBINATIONS = {
        // ========== ALLIANCE RACES ==========

        // Human (Can be all classes except Evoker)
        { RACE_HUMAN, CLASS_WARRIOR, false, false, 0 },
        { RACE_HUMAN, CLASS_PALADIN, false, false, 0 },
        { RACE_HUMAN, CLASS_HUNTER, false, false, 0 },
        { RACE_HUMAN, CLASS_ROGUE, false, false, 0 },
        { RACE_HUMAN, CLASS_PRIEST, false, false, 0 },
        { RACE_HUMAN, CLASS_DEATH_KNIGHT, false, false, 0 },
        { RACE_HUMAN, CLASS_SHAMAN, false, false, 0 },  // Added in 11.0
        { RACE_HUMAN, CLASS_MAGE, false, false, 0 },
        { RACE_HUMAN, CLASS_WARLOCK, false, false, 0 },
        { RACE_HUMAN, CLASS_MONK, false, false, 0 },
        { RACE_HUMAN, CLASS_DRUID, false, false, 0 },  // Added in 11.0
        { RACE_HUMAN, CLASS_DEMON_HUNTER, false, false, 0 },  // Added in 11.0

        // Dwarf (Can be all classes except Evoker and Demon Hunter)
        { RACE_DWARF, CLASS_WARRIOR, false, false, 0 },
        { RACE_DWARF, CLASS_PALADIN, false, false, 0 },
        { RACE_DWARF, CLASS_HUNTER, false, false, 0 },
        { RACE_DWARF, CLASS_ROGUE, false, false, 0 },
        { RACE_DWARF, CLASS_PRIEST, false, false, 0 },
        { RACE_DWARF, CLASS_DEATH_KNIGHT, false, false, 0 },
        { RACE_DWARF, CLASS_SHAMAN, false, false, 0 },
        { RACE_DWARF, CLASS_MAGE, false, false, 0 },
        { RACE_DWARF, CLASS_WARLOCK, false, false, 0 },
        { RACE_DWARF, CLASS_MONK, false, false, 0 },
        { RACE_DWARF, CLASS_DRUID, false, false, 0 },  // Added in 11.0

        // Night Elf (Can be all classes except Evoker)
        { RACE_NIGHTELF, CLASS_WARRIOR, false, false, 0 },
        { RACE_NIGHTELF, CLASS_PALADIN, false, false, 0 },  // Added in 11.0
        { RACE_NIGHTELF, CLASS_HUNTER, false, false, 0 },
        { RACE_NIGHTELF, CLASS_ROGUE, false, false, 0 },
        { RACE_NIGHTELF, CLASS_PRIEST, false, false, 0 },
        { RACE_NIGHTELF, CLASS_DEATH_KNIGHT, false, false, 0 },
        { RACE_NIGHTELF, CLASS_SHAMAN, false, false, 0 },  // Added in 11.0
        { RACE_NIGHTELF, CLASS_MAGE, false, false, 0 },
        { RACE_NIGHTELF, CLASS_WARLOCK, false, false, 0 },  // Added in 11.0
        { RACE_NIGHTELF, CLASS_MONK, false, false, 0 },
        { RACE_NIGHTELF, CLASS_DRUID, false, false, 0 },
        { RACE_NIGHTELF, CLASS_DEMON_HUNTER, false, false, 0 },

        // Gnome (Can be all classes except Evoker, Druid, and Demon Hunter)
        { RACE_GNOME, CLASS_WARRIOR, false, false, 0 },
        { RACE_GNOME, CLASS_PALADIN, false, false, 0 },  // Added in 11.0
        { RACE_GNOME, CLASS_HUNTER, false, false, 0 },
        { RACE_GNOME, CLASS_ROGUE, false, false, 0 },
        { RACE_GNOME, CLASS_PRIEST, false, false, 0 },
        { RACE_GNOME, CLASS_DEATH_KNIGHT, false, false, 0 },
        { RACE_GNOME, CLASS_SHAMAN, false, false, 0 },  // Added in 11.0
        { RACE_GNOME, CLASS_MAGE, false, false, 0 },
        { RACE_GNOME, CLASS_WARLOCK, false, false, 0 },
        { RACE_GNOME, CLASS_MONK, false, false, 0 },

        // Draenei
        { RACE_DRAENEI, CLASS_WARRIOR, false, false, 0 },
        { RACE_DRAENEI, CLASS_PALADIN, false, false, 0 },
        { RACE_DRAENEI, CLASS_HUNTER, false, false, 0 },
        { RACE_DRAENEI, CLASS_ROGUE, false, false, 0 },  // Added in 11.0
        { RACE_DRAENEI, CLASS_PRIEST, false, false, 0 },
        { RACE_DRAENEI, CLASS_DEATH_KNIGHT, false, false, 0 },
        { RACE_DRAENEI, CLASS_SHAMAN, false, false, 0 },
        { RACE_DRAENEI, CLASS_MAGE, false, false, 0 },
        { RACE_DRAENEI, CLASS_WARLOCK, false, false, 0 },  // Added in 11.0
        { RACE_DRAENEI, CLASS_MONK, false, false, 0 },
        { RACE_DRAENEI, CLASS_DRUID, false, false, 0 },  // Added in 11.0
        { RACE_DRAENEI, CLASS_DEMON_HUNTER, false, false, 0 },  // Added in 11.0

        // Worgen
        { RACE_WORGEN, CLASS_WARRIOR, false, false, 0 },
        { RACE_WORGEN, CLASS_PALADIN, false, false, 0 },  // Added in 11.0
        { RACE_WORGEN, CLASS_HUNTER, false, false, 0 },
        { RACE_WORGEN, CLASS_ROGUE, false, false, 0 },
        { RACE_WORGEN, CLASS_PRIEST, false, false, 0 },
        { RACE_WORGEN, CLASS_DEATH_KNIGHT, false, false, 0 },
        { RACE_WORGEN, CLASS_SHAMAN, false, false, 0 },  // Added in 11.0
        { RACE_WORGEN, CLASS_MAGE, false, false, 0 },
        { RACE_WORGEN, CLASS_WARLOCK, false, false, 0 },
        { RACE_WORGEN, CLASS_MONK, false, false, 0 },  // Added in 11.0
        { RACE_WORGEN, CLASS_DRUID, false, false, 0 },
        { RACE_WORGEN, CLASS_DEMON_HUNTER, false, false, 0 },  // Added in 11.0

        // Pandaren (Alliance)
        { RACE_PANDAREN_ALLIANCE, CLASS_WARRIOR, false, false, 0 },
        { RACE_PANDAREN_ALLIANCE, CLASS_PALADIN, false, false, 0 },  // Added in 11.0
        { RACE_PANDAREN_ALLIANCE, CLASS_HUNTER, false, false, 0 },
        { RACE_PANDAREN_ALLIANCE, CLASS_ROGUE, false, false, 0 },
        { RACE_PANDAREN_ALLIANCE, CLASS_PRIEST, false, false, 0 },
        { RACE_PANDAREN_ALLIANCE, CLASS_DEATH_KNIGHT, false, false, 0 },
        { RACE_PANDAREN_ALLIANCE, CLASS_SHAMAN, false, false, 0 },
        { RACE_PANDAREN_ALLIANCE, CLASS_MAGE, false, false, 0 },
        { RACE_PANDAREN_ALLIANCE, CLASS_WARLOCK, false, false, 0 },  // Added in 11.0
        { RACE_PANDAREN_ALLIANCE, CLASS_MONK, false, false, 0 },
        { RACE_PANDAREN_ALLIANCE, CLASS_DRUID, false, false, 0 },  // Added in 11.0

        // Dracthyr (Alliance) - ONLY Evoker class
        { RACE_DRACTHYR_ALLIANCE, CLASS_EVOKER, false, false, 0 },

        // === ALLIANCE ALLIED RACES ===

        // Void Elf (Allied Race - Requires achievement 12066)
        { RACE_VOID_ELF, CLASS_WARRIOR, true, true, 12066 },
        { RACE_VOID_ELF, CLASS_PALADIN, true, true, 12066 },  // Added in 11.0
        { RACE_VOID_ELF, CLASS_HUNTER, true, true, 12066 },
        { RACE_VOID_ELF, CLASS_ROGUE, true, true, 12066 },
        { RACE_VOID_ELF, CLASS_PRIEST, true, true, 12066 },
        { RACE_VOID_ELF, CLASS_DEATH_KNIGHT, true, true, 12066 },
        { RACE_VOID_ELF, CLASS_SHAMAN, true, true, 12066 },  // Added in 11.0
        { RACE_VOID_ELF, CLASS_MAGE, true, true, 12066 },
        { RACE_VOID_ELF, CLASS_WARLOCK, true, true, 12066 },
        { RACE_VOID_ELF, CLASS_MONK, true, true, 12066 },
        { RACE_VOID_ELF, CLASS_DEMON_HUNTER, true, true, 12066 },  // Added in 11.0

        // Lightforged Draenei (Allied Race - Requires achievement 12081)
        { RACE_LIGHTFORGED_DRAENEI, CLASS_WARRIOR, true, true, 12081 },
        { RACE_LIGHTFORGED_DRAENEI, CLASS_PALADIN, true, true, 12081 },
        { RACE_LIGHTFORGED_DRAENEI, CLASS_HUNTER, true, true, 12081 },
        { RACE_LIGHTFORGED_DRAENEI, CLASS_ROGUE, true, true, 12081 },  // Added in 11.0
        { RACE_LIGHTFORGED_DRAENEI, CLASS_PRIEST, true, true, 12081 },
        { RACE_LIGHTFORGED_DRAENEI, CLASS_DEATH_KNIGHT, true, true, 12081 },
        { RACE_LIGHTFORGED_DRAENEI, CLASS_SHAMAN, true, true, 12081 },  // Added in 11.0
        { RACE_LIGHTFORGED_DRAENEI, CLASS_MAGE, true, true, 12081 },
        { RACE_LIGHTFORGED_DRAENEI, CLASS_WARLOCK, true, true, 12081 },  // Added in 11.0
        { RACE_LIGHTFORGED_DRAENEI, CLASS_MONK, true, true, 12081 },  // Added in 11.0

        // Dark Iron Dwarf (Allied Race - Requires achievement 12515)
        { RACE_DARK_IRON_DWARF, CLASS_WARRIOR, true, true, 12515 },
        { RACE_DARK_IRON_DWARF, CLASS_PALADIN, true, true, 12515 },
        { RACE_DARK_IRON_DWARF, CLASS_HUNTER, true, true, 12515 },
        { RACE_DARK_IRON_DWARF, CLASS_ROGUE, true, true, 12515 },
        { RACE_DARK_IRON_DWARF, CLASS_PRIEST, true, true, 12515 },
        { RACE_DARK_IRON_DWARF, CLASS_DEATH_KNIGHT, true, true, 12515 },
        { RACE_DARK_IRON_DWARF, CLASS_SHAMAN, true, true, 12515 },
        { RACE_DARK_IRON_DWARF, CLASS_MAGE, true, true, 12515 },
        { RACE_DARK_IRON_DWARF, CLASS_WARLOCK, true, true, 12515 },
        { RACE_DARK_IRON_DWARF, CLASS_MONK, true, true, 12515 },
        { RACE_DARK_IRON_DWARF, CLASS_DRUID, true, true, 12515 },  // Added in 11.0

        // Kul Tiran (Allied Race - Requires achievement 12510)
        { RACE_KUL_TIRAN, CLASS_WARRIOR, true, true, 12510 },
        { RACE_KUL_TIRAN, CLASS_PALADIN, true, true, 12510 },  // Added in 11.0
        { RACE_KUL_TIRAN, CLASS_HUNTER, true, true, 12510 },
        { RACE_KUL_TIRAN, CLASS_ROGUE, true, true, 12510 },
        { RACE_KUL_TIRAN, CLASS_PRIEST, true, true, 12510 },
        { RACE_KUL_TIRAN, CLASS_DEATH_KNIGHT, true, true, 12510 },
        { RACE_KUL_TIRAN, CLASS_SHAMAN, true, true, 12510 },
        { RACE_KUL_TIRAN, CLASS_MAGE, true, true, 12510 },
        { RACE_KUL_TIRAN, CLASS_WARLOCK, true, true, 12510 },  // Added in 11.0
        { RACE_KUL_TIRAN, CLASS_MONK, true, true, 12510 },
        { RACE_KUL_TIRAN, CLASS_DRUID, true, true, 12510 },

        // Mechagnome (Allied Race - Requires achievement 13553)
        { RACE_MECHAGNOME, CLASS_WARRIOR, true, true, 13553 },
        { RACE_MECHAGNOME, CLASS_PALADIN, true, true, 13553 },  // Added in 11.0
        { RACE_MECHAGNOME, CLASS_HUNTER, true, true, 13553 },
        { RACE_MECHAGNOME, CLASS_ROGUE, true, true, 13553 },
        { RACE_MECHAGNOME, CLASS_PRIEST, true, true, 13553 },
        { RACE_MECHAGNOME, CLASS_DEATH_KNIGHT, true, true, 13553 },
        { RACE_MECHAGNOME, CLASS_SHAMAN, true, true, 13553 },  // Added in 11.0
        { RACE_MECHAGNOME, CLASS_MAGE, true, true, 13553 },
        { RACE_MECHAGNOME, CLASS_WARLOCK, true, true, 13553 },
        { RACE_MECHAGNOME, CLASS_MONK, true, true, 13553 },

        // Earthen Dwarf (Alliance) - NEW in 11.0 The War Within
        { RACE_EARTHEN_DWARF_ALLIANCE, CLASS_WARRIOR, true, false, 0 },  // No unlock required
        { RACE_EARTHEN_DWARF_ALLIANCE, CLASS_PALADIN, true, false, 0 },
        { RACE_EARTHEN_DWARF_ALLIANCE, CLASS_HUNTER, true, false, 0 },
        { RACE_EARTHEN_DWARF_ALLIANCE, CLASS_ROGUE, true, false, 0 },
        { RACE_EARTHEN_DWARF_ALLIANCE, CLASS_PRIEST, true, false, 0 },
        { RACE_EARTHEN_DWARF_ALLIANCE, CLASS_DEATH_KNIGHT, true, false, 0 },
        { RACE_EARTHEN_DWARF_ALLIANCE, CLASS_SHAMAN, true, false, 0 },
        { RACE_EARTHEN_DWARF_ALLIANCE, CLASS_MAGE, true, false, 0 },
        { RACE_EARTHEN_DWARF_ALLIANCE, CLASS_WARLOCK, true, false, 0 },
        { RACE_EARTHEN_DWARF_ALLIANCE, CLASS_MONK, true, false, 0 },

        // ========== HORDE RACES ==========

        // Orc (Can be all classes except Evoker and Paladin)
        { RACE_ORC, CLASS_WARRIOR, false, false, 0 },
        { RACE_ORC, CLASS_HUNTER, false, false, 0 },
        { RACE_ORC, CLASS_ROGUE, false, false, 0 },
        { RACE_ORC, CLASS_PRIEST, false, false, 0 },  // Added in 11.0
        { RACE_ORC, CLASS_DEATH_KNIGHT, false, false, 0 },
        { RACE_ORC, CLASS_SHAMAN, false, false, 0 },
        { RACE_ORC, CLASS_MAGE, false, false, 0 },
        { RACE_ORC, CLASS_WARLOCK, false, false, 0 },
        { RACE_ORC, CLASS_MONK, false, false, 0 },
        { RACE_ORC, CLASS_DRUID, false, false, 0 },  // Added in 11.0
        { RACE_ORC, CLASS_DEMON_HUNTER, false, false, 0 },  // Added in 11.0

        // Undead
        { RACE_UNDEAD_PLAYER, CLASS_WARRIOR, false, false, 0 },
        { RACE_UNDEAD_PLAYER, CLASS_PALADIN, false, false, 0 },  // Added in 11.0
        { RACE_UNDEAD_PLAYER, CLASS_HUNTER, false, false, 0 },
        { RACE_UNDEAD_PLAYER, CLASS_ROGUE, false, false, 0 },
        { RACE_UNDEAD_PLAYER, CLASS_PRIEST, false, false, 0 },
        { RACE_UNDEAD_PLAYER, CLASS_DEATH_KNIGHT, false, false, 0 },
        { RACE_UNDEAD_PLAYER, CLASS_SHAMAN, false, false, 0 },  // Added in 11.0
        { RACE_UNDEAD_PLAYER, CLASS_MAGE, false, false, 0 },
        { RACE_UNDEAD_PLAYER, CLASS_WARLOCK, false, false, 0 },
        { RACE_UNDEAD_PLAYER, CLASS_MONK, false, false, 0 },
        { RACE_UNDEAD_PLAYER, CLASS_DRUID, false, false, 0 },  // Added in 11.0
        { RACE_UNDEAD_PLAYER, CLASS_DEMON_HUNTER, false, false, 0 },  // Added in 11.0

        // Tauren
        { RACE_TAUREN, CLASS_WARRIOR, false, false, 0 },
        { RACE_TAUREN, CLASS_PALADIN, false, false, 0 },
        { RACE_TAUREN, CLASS_HUNTER, false, false, 0 },
        { RACE_TAUREN, CLASS_ROGUE, false, false, 0 },  // Added in 11.0
        { RACE_TAUREN, CLASS_PRIEST, false, false, 0 },
        { RACE_TAUREN, CLASS_DEATH_KNIGHT, false, false, 0 },
        { RACE_TAUREN, CLASS_SHAMAN, false, false, 0 },
        { RACE_TAUREN, CLASS_MAGE, false, false, 0 },  // Added in 11.0
        { RACE_TAUREN, CLASS_WARLOCK, false, false, 0 },  // Added in 11.0
        { RACE_TAUREN, CLASS_MONK, false, false, 0 },
        { RACE_TAUREN, CLASS_DRUID, false, false, 0 },
        { RACE_TAUREN, CLASS_DEMON_HUNTER, false, false, 0 },  // Added in 11.0

        // Troll
        { RACE_TROLL, CLASS_WARRIOR, false, false, 0 },
        { RACE_TROLL, CLASS_PALADIN, false, false, 0 },  // Added in 11.0
        { RACE_TROLL, CLASS_HUNTER, false, false, 0 },
        { RACE_TROLL, CLASS_ROGUE, false, false, 0 },
        { RACE_TROLL, CLASS_PRIEST, false, false, 0 },
        { RACE_TROLL, CLASS_DEATH_KNIGHT, false, false, 0 },
        { RACE_TROLL, CLASS_SHAMAN, false, false, 0 },
        { RACE_TROLL, CLASS_MAGE, false, false, 0 },
        { RACE_TROLL, CLASS_WARLOCK, false, false, 0 },
        { RACE_TROLL, CLASS_MONK, false, false, 0 },
        { RACE_TROLL, CLASS_DRUID, false, false, 0 },
        { RACE_TROLL, CLASS_DEMON_HUNTER, false, false, 0 },  // Added in 11.0

        // Goblin
        { RACE_GOBLIN, CLASS_WARRIOR, false, false, 0 },
        { RACE_GOBLIN, CLASS_PALADIN, false, false, 0 },  // Added in 11.0
        { RACE_GOBLIN, CLASS_HUNTER, false, false, 0 },
        { RACE_GOBLIN, CLASS_ROGUE, false, false, 0 },
        { RACE_GOBLIN, CLASS_PRIEST, false, false, 0 },
        { RACE_GOBLIN, CLASS_DEATH_KNIGHT, false, false, 0 },
        { RACE_GOBLIN, CLASS_SHAMAN, false, false, 0 },
        { RACE_GOBLIN, CLASS_MAGE, false, false, 0 },
        { RACE_GOBLIN, CLASS_WARLOCK, false, false, 0 },
        { RACE_GOBLIN, CLASS_MONK, false, false, 0 },  // Added in 11.0
        { RACE_GOBLIN, CLASS_DRUID, false, false, 0 },  // Added in 11.0
        { RACE_GOBLIN, CLASS_DEMON_HUNTER, false, false, 0 },  // Added in 11.0

        // Blood Elf
        { RACE_BLOODELF, CLASS_WARRIOR, false, false, 0 },
        { RACE_BLOODELF, CLASS_PALADIN, false, false, 0 },
        { RACE_BLOODELF, CLASS_HUNTER, false, false, 0 },
        { RACE_BLOODELF, CLASS_ROGUE, false, false, 0 },
        { RACE_BLOODELF, CLASS_PRIEST, false, false, 0 },
        { RACE_BLOODELF, CLASS_DEATH_KNIGHT, false, false, 0 },
        { RACE_BLOODELF, CLASS_SHAMAN, false, false, 0 },  // Added in 11.0
        { RACE_BLOODELF, CLASS_MAGE, false, false, 0 },
        { RACE_BLOODELF, CLASS_WARLOCK, false, false, 0 },
        { RACE_BLOODELF, CLASS_MONK, false, false, 0 },
        { RACE_BLOODELF, CLASS_DRUID, false, false, 0 },  // Added in 11.0
        { RACE_BLOODELF, CLASS_DEMON_HUNTER, false, false, 0 },

        // Pandaren (Horde)
        { RACE_PANDAREN_HORDE, CLASS_WARRIOR, false, false, 0 },
        { RACE_PANDAREN_HORDE, CLASS_PALADIN, false, false, 0 },  // Added in 11.0
        { RACE_PANDAREN_HORDE, CLASS_HUNTER, false, false, 0 },
        { RACE_PANDAREN_HORDE, CLASS_ROGUE, false, false, 0 },
        { RACE_PANDAREN_HORDE, CLASS_PRIEST, false, false, 0 },
        { RACE_PANDAREN_HORDE, CLASS_DEATH_KNIGHT, false, false, 0 },
        { RACE_PANDAREN_HORDE, CLASS_SHAMAN, false, false, 0 },
        { RACE_PANDAREN_HORDE, CLASS_MAGE, false, false, 0 },
        { RACE_PANDAREN_HORDE, CLASS_WARLOCK, false, false, 0 },  // Added in 11.0
        { RACE_PANDAREN_HORDE, CLASS_MONK, false, false, 0 },
        { RACE_PANDAREN_HORDE, CLASS_DRUID, false, false, 0 },  // Added in 11.0

        // Dracthyr (Horde) - ONLY Evoker class
        { RACE_DRACTHYR_HORDE, CLASS_EVOKER, false, false, 0 },

        // === HORDE ALLIED RACES ===

        // Nightborne (Allied Race - Requires achievement 12079)
        { RACE_NIGHTBORNE, CLASS_WARRIOR, true, true, 12079 },
        { RACE_NIGHTBORNE, CLASS_PALADIN, true, true, 12079 },  // Added in 11.0
        { RACE_NIGHTBORNE, CLASS_HUNTER, true, true, 12079 },
        { RACE_NIGHTBORNE, CLASS_ROGUE, true, true, 12079 },
        { RACE_NIGHTBORNE, CLASS_PRIEST, true, true, 12079 },
        { RACE_NIGHTBORNE, CLASS_DEATH_KNIGHT, true, true, 12079 },
        { RACE_NIGHTBORNE, CLASS_SHAMAN, true, true, 12079 },  // Added in 11.0
        { RACE_NIGHTBORNE, CLASS_MAGE, true, true, 12079 },
        { RACE_NIGHTBORNE, CLASS_WARLOCK, true, true, 12079 },
        { RACE_NIGHTBORNE, CLASS_MONK, true, true, 12079 },
        { RACE_NIGHTBORNE, CLASS_DRUID, true, true, 12079 },  // Added in 11.0
        { RACE_NIGHTBORNE, CLASS_DEMON_HUNTER, true, true, 12079 },  // Added in 11.0

        // Highmountain Tauren (Allied Race - Requires achievement 12080)
        { RACE_HIGHMOUNTAIN_TAUREN, CLASS_WARRIOR, true, true, 12080 },
        { RACE_HIGHMOUNTAIN_TAUREN, CLASS_PALADIN, true, true, 12080 },  // Added in 11.0
        { RACE_HIGHMOUNTAIN_TAUREN, CLASS_HUNTER, true, true, 12080 },
        { RACE_HIGHMOUNTAIN_TAUREN, CLASS_ROGUE, true, true, 12080 },  // Added in 11.0
        { RACE_HIGHMOUNTAIN_TAUREN, CLASS_PRIEST, true, true, 12080 },  // Added in 11.0
        { RACE_HIGHMOUNTAIN_TAUREN, CLASS_DEATH_KNIGHT, true, true, 12080 },
        { RACE_HIGHMOUNTAIN_TAUREN, CLASS_SHAMAN, true, true, 12080 },
        { RACE_HIGHMOUNTAIN_TAUREN, CLASS_MAGE, true, true, 12080 },  // Added in 11.0
        { RACE_HIGHMOUNTAIN_TAUREN, CLASS_WARLOCK, true, true, 12080 },  // Added in 11.0
        { RACE_HIGHMOUNTAIN_TAUREN, CLASS_MONK, true, true, 12080 },
        { RACE_HIGHMOUNTAIN_TAUREN, CLASS_DRUID, true, true, 12080 },

        // Mag'har Orc (Allied Race - Requires achievement 12518)
        { RACE_MAGHAR_ORC, CLASS_WARRIOR, true, true, 12518 },
        { RACE_MAGHAR_ORC, CLASS_HUNTER, true, true, 12518 },
        { RACE_MAGHAR_ORC, CLASS_ROGUE, true, true, 12518 },
        { RACE_MAGHAR_ORC, CLASS_PRIEST, true, true, 12518 },
        { RACE_MAGHAR_ORC, CLASS_DEATH_KNIGHT, true, true, 12518 },
        { RACE_MAGHAR_ORC, CLASS_SHAMAN, true, true, 12518 },
        { RACE_MAGHAR_ORC, CLASS_MAGE, true, true, 12518 },
        { RACE_MAGHAR_ORC, CLASS_WARLOCK, true, true, 12518 },  // Added in 11.0
        { RACE_MAGHAR_ORC, CLASS_MONK, true, true, 12518 },
        { RACE_MAGHAR_ORC, CLASS_DRUID, true, true, 12518 },  // Added in 11.0

        // Zandalari Troll (Allied Race - Requires achievement 13161)
        { RACE_ZANDALARI_TROLL, CLASS_WARRIOR, true, true, 13161 },
        { RACE_ZANDALARI_TROLL, CLASS_PALADIN, true, true, 13161 },
        { RACE_ZANDALARI_TROLL, CLASS_HUNTER, true, true, 13161 },
        { RACE_ZANDALARI_TROLL, CLASS_ROGUE, true, true, 13161 },
        { RACE_ZANDALARI_TROLL, CLASS_PRIEST, true, true, 13161 },
        { RACE_ZANDALARI_TROLL, CLASS_DEATH_KNIGHT, true, true, 13161 },
        { RACE_ZANDALARI_TROLL, CLASS_SHAMAN, true, true, 13161 },
        { RACE_ZANDALARI_TROLL, CLASS_MAGE, true, true, 13161 },
        { RACE_ZANDALARI_TROLL, CLASS_WARLOCK, true, true, 13161 },  // Added in 11.0
        { RACE_ZANDALARI_TROLL, CLASS_MONK, true, true, 13161 },
        { RACE_ZANDALARI_TROLL, CLASS_DRUID, true, true, 13161 },
        { RACE_ZANDALARI_TROLL, CLASS_DEMON_HUNTER, true, true, 13161 },  // Added in 11.0

        // Vulpera (Allied Race - Requires achievement 14002)
        { RACE_VULPERA, CLASS_WARRIOR, true, true, 14002 },
        { RACE_VULPERA, CLASS_PALADIN, true, true, 14002 },  // Added in 11.0
        { RACE_VULPERA, CLASS_HUNTER, true, true, 14002 },
        { RACE_VULPERA, CLASS_ROGUE, true, true, 14002 },
        { RACE_VULPERA, CLASS_PRIEST, true, true, 14002 },
        { RACE_VULPERA, CLASS_DEATH_KNIGHT, true, true, 14002 },
        { RACE_VULPERA, CLASS_SHAMAN, true, true, 14002 },
        { RACE_VULPERA, CLASS_MAGE, true, true, 14002 },
        { RACE_VULPERA, CLASS_WARLOCK, true, true, 14002 },
        { RACE_VULPERA, CLASS_MONK, true, true, 14002 },
        { RACE_VULPERA, CLASS_DRUID, true, true, 14002 },  // Added in 11.0

        // Earthen Dwarf (Horde) - NEW in 11.0 The War Within
        { RACE_EARTHEN_DWARF_HORDE, CLASS_WARRIOR, true, false, 0 },  // No unlock required
        { RACE_EARTHEN_DWARF_HORDE, CLASS_PALADIN, true, false, 0 },
        { RACE_EARTHEN_DWARF_HORDE, CLASS_HUNTER, true, false, 0 },
        { RACE_EARTHEN_DWARF_HORDE, CLASS_ROGUE, true, false, 0 },
        { RACE_EARTHEN_DWARF_HORDE, CLASS_PRIEST, true, false, 0 },
        { RACE_EARTHEN_DWARF_HORDE, CLASS_DEATH_KNIGHT, true, false, 0 },
        { RACE_EARTHEN_DWARF_HORDE, CLASS_SHAMAN, true, false, 0 },
        { RACE_EARTHEN_DWARF_HORDE, CLASS_MAGE, true, false, 0 },
        { RACE_EARTHEN_DWARF_HORDE, CLASS_WARLOCK, true, false, 0 },
        { RACE_EARTHEN_DWARF_HORDE, CLASS_MONK, true, false, 0 },
    };

    // ===================================================================================
    // STARTING ZONES AND POSITIONS
    // ===================================================================================

    struct StartingZone
    {
        uint32 mapId;
        uint32 zoneId;
        Position position;
        Position npePosition;  // New Player Experience position (if different)
    };

    inline const ::std::unordered_map<Races, StartingZone> STARTING_ZONES = {
        // Alliance
        { RACE_HUMAN,               { 0, 12, { -8949.95f, -132.493f, 83.5312f, 0.0f }, { -8949.95f, -132.493f, 83.5312f, 0.0f } } },  // Elwynn Forest
        { RACE_DWARF,               { 0, 1, { -6240.32f, 331.033f, 382.758f, 0.0f }, { -6240.32f, 331.033f, 382.758f, 0.0f } } },     // Dun Morogh
        { RACE_NIGHTELF,            { 1, 141, { 10311.3f, 832.463f, 1326.41f, 0.0f }, { 10311.3f, 832.463f, 1326.41f, 0.0f } } },   // Teldrassil
        { RACE_GNOME,               { 0, 1, { -6240.32f, 331.033f, 382.758f, 0.0f }, { -6240.32f, 331.033f, 382.758f, 0.0f } } },     // Dun Morogh
        { RACE_DRAENEI,             { 530, 3524, { -4192.62f, -13456.2f, 47.5078f, 0.0f }, { -4192.62f, -13456.2f, 47.5078f, 0.0f } } }, // Azuremyst Isle
        { RACE_WORGEN,              { 654, 4714, { -1451.53f, 1403.35f, 35.5561f, 0.0f }, { -1451.53f, 1403.35f, 35.5561f, 0.0f } } },  // Gilneas
        { RACE_PANDAREN_ALLIANCE,   { 860, 5736, { 1463.65f, 3466.18f, 181.659f, 0.0f }, { 1463.65f, 3466.18f, 181.659f, 0.0f } } },  // The Wandering Isle
        { RACE_DRACTHYR_ALLIANCE,   { 2444, 13645, { 5838.33f, -2996.38f, 248.93f, 0.0f }, { 5838.33f, -2996.38f, 248.93f, 0.0f } } }, // The Forbidden Reach

        // Alliance Allied Races
        { RACE_VOID_ELF,            { 1, 141, { 10311.3f, 832.463f, 1326.41f, 0.0f }, { 10311.3f, 832.463f, 1326.41f, 0.0f } } },   // Telogrus Rift then Stormwind
        { RACE_LIGHTFORGED_DRAENEI, { 0, 1519, { -8950.23f, 516.857f, 96.3568f, 0.0f }, { -8950.23f, 516.857f, 96.3568f, 0.0f } } },  // Vindicaar then Stormwind
        { RACE_DARK_IRON_DWARF,     { 0, 1, { -6240.32f, 331.033f, 382.758f, 0.0f }, { -6240.32f, 331.033f, 382.758f, 0.0f } } },     // Shadowforge City then Ironforge
        { RACE_KUL_TIRAN,           { 1643, 9042, { 1153.87f, -560.879f, 30.5977f, 0.0f }, { 1153.87f, -560.879f, 30.5977f, 0.0f } } }, // Boralus
        { RACE_MECHAGNOME,          { 2097, 10356, { 1435.68f, -4487.03f, 31.0835f, 0.0f }, { 1435.68f, -4487.03f, 31.0835f, 0.0f } } }, // Mechagon then Stormwind
        { RACE_EARTHEN_DWARF_ALLIANCE, { 2552, 14753, { 2749.09f, -2578.21f, 221.92f, 0.0f }, { 2749.09f, -2578.21f, 221.92f, 0.0f } } }, // Isle of Dorn (11.0)

        // Horde
        { RACE_ORC,                 { 1, 14, { -618.518f, -4251.67f, 38.718f, 0.0f }, { -618.518f, -4251.67f, 38.718f, 0.0f } } },    // Durotar
        { RACE_UNDEAD_PLAYER,       { 0, 85, { 1676.35f, 1677.55f, 121.67f, 0.0f }, { 1676.35f, 1677.55f, 121.67f, 0.0f } } },      // Tirisfal Glades
        { RACE_TAUREN,              { 1, 215, { -2917.58f, -257.98f, 52.9968f, 0.0f }, { -2917.58f, -257.98f, 52.9968f, 0.0f } } },  // Mulgore
        { RACE_TROLL,               { 1, 14, { -618.518f, -4251.67f, 38.718f, 0.0f }, { -618.518f, -4251.67f, 38.718f, 0.0f } } },    // Echo Isles
        { RACE_GOBLIN,              { 648, 4737, { 527.688f, 3273.53f, 0.197498f, 0.0f }, { 527.688f, 3273.53f, 0.197498f, 0.0f } } }, // The Lost Isles
        { RACE_BLOODELF,            { 530, 3430, { 10349.6f, -6357.29f, 33.4026f, 0.0f }, { 10349.6f, -6357.29f, 33.4026f, 0.0f } } }, // Eversong Woods
        { RACE_PANDAREN_HORDE,      { 860, 5736, { 1463.65f, 3466.18f, 181.659f, 0.0f }, { 1463.65f, 3466.18f, 181.659f, 0.0f } } },  // The Wandering Isle
        { RACE_DRACTHYR_HORDE,      { 2444, 13645, { 5838.33f, -2996.38f, 248.93f, 0.0f }, { 5838.33f, -2996.38f, 248.93f, 0.0f } } }, // The Forbidden Reach

        // Horde Allied Races
        { RACE_NIGHTBORNE,          { 1, 1637, { 1567.08f, -4196.73f, 53.6796f, 0.0f }, { 1567.08f, -4196.73f, 53.6796f, 0.0f } } }, // Suramar then Orgrimmar
        { RACE_HIGHMOUNTAIN_TAUREN, { 1, 1637, { 1567.08f, -4196.73f, 53.6796f, 0.0f }, { 1567.08f, -4196.73f, 53.6796f, 0.0f } } }, // Thunder Totem then Orgrimmar
        { RACE_MAGHAR_ORC,          { 1, 1637, { 1567.08f, -4196.73f, 53.6796f, 0.0f }, { 1567.08f, -4196.73f, 53.6796f, 0.0f } } }, // Gorgrond then Orgrimmar
        { RACE_ZANDALARI_TROLL,     { 1642, 8670, { -1130.16f, 788.269f, 497.062f, 0.0f }, { -1130.16f, 788.269f, 497.062f, 0.0f } } }, // Dazar'alor
        { RACE_VULPERA,              { 1, 1637, { 1567.08f, -4196.73f, 53.6796f, 0.0f }, { 1567.08f, -4196.73f, 53.6796f, 0.0f } } }, // Vol'dun then Orgrimmar
        { RACE_EARTHEN_DWARF_HORDE, { 2552, 14753, { 2749.09f, -2578.21f, 221.92f, 0.0f }, { 2749.09f, -2578.21f, 221.92f, 0.0f } } }, // Isle of Dorn (11.0)

        // Neutral/Special
        { RACE_PANDAREN_NEUTRAL,    { 860, 5736, { 1463.65f, 3466.18f, 181.659f, 0.0f }, { 1463.65f, 3466.18f, 181.659f, 0.0f } } },  // The Wandering Isle
    };

    // ===================================================================================
    // BASE STATS BY RACE/CLASS (12.0 Values)
    // ===================================================================================

    struct BaseStats
    {
        uint32 health;
        uint32 mana;
        uint32 strength;
        uint32 agility;
        uint32 stamina;
        uint32 intellect;
        uint32 spirit;  // WoW 12.0: Spirit stat re-added (MAX_STATS = 5)
    };

    // Simplified base stat calculation - actual values from DBC data
    inline BaseStats GetBaseStats(Races race, Classes playerClass, uint8 level)
    {
        BaseStats stats;

        // Base health at level 10 (TWW starting level)
        stats.health = 280;  // Base health for all classes at level 10

        // Class-based primary stats at level 10
        switch (playerClass)
        {
            case CLASS_WARRIOR:
            case CLASS_PALADIN:
            case CLASS_DEATH_KNIGHT:
                stats.strength = 31;
                stats.agility = 20;
                stats.intellect = 20;
                stats.stamina = 32;
                stats.spirit = 18;  // WoW 12.0: Spirit for minor mana regen
                break;

            case CLASS_HUNTER:
            case CLASS_ROGUE:
            case CLASS_MONK:
            case CLASS_DEMON_HUNTER:
                stats.strength = 20;
                stats.agility = 31;
                stats.intellect = 20;
                stats.stamina = 30;
                stats.spirit = 18;  // WoW 12.0: Spirit for minor mana regen
                break;

            case CLASS_PRIEST:
            case CLASS_MAGE:
            case CLASS_WARLOCK:
            case CLASS_EVOKER:
                stats.strength = 20;
                stats.agility = 20;
                stats.intellect = 31;
                stats.stamina = 29;
                stats.spirit = 28;  // WoW 12.0: Casters have high spirit for mana regen
                break;

            case CLASS_SHAMAN:
            case CLASS_DRUID:
                stats.strength = 22;
                stats.agility = 22;
                stats.intellect = 27;
                stats.stamina = 30;
                stats.spirit = 25;  // WoW 12.0: Hybrids have moderate spirit
                break;

            default:
                stats.strength = 20;
                stats.agility = 20;
                stats.intellect = 20;
                stats.stamina = 30;
                stats.spirit = 18;  // WoW 12.0: Default spirit value
                break;
        }

        // Mana for casters
        if (playerClass == CLASS_WARRIOR || playerClass == CLASS_ROGUE ||
            playerClass == CLASS_DEATH_KNIGHT || playerClass == CLASS_HUNTER ||
            playerClass == CLASS_DEMON_HUNTER)
        {
            stats.mana = 0;  // These classes don't use mana
        }
        else
        {
            stats.mana = stats.intellect * 15;  // Base mana calculation
        }

        // Apply racial modifiers (simplified)
        switch (race)
        {
            case RACE_TAUREN:
            case RACE_HIGHMOUNTAIN_TAUREN:
                stats.stamina += 2;
                stats.health += 20;
                break;
            case RACE_GNOME:
            case RACE_MECHAGNOME:
                stats.intellect += 2;
                break;
            case RACE_DWARF:
            case RACE_DARK_IRON_DWARF:
                stats.stamina += 1;
                stats.strength += 1;
                break;
            case RACE_NIGHTELF:
            case RACE_VOID_ELF:
                stats.agility += 2;
                break;
            case RACE_ORC:
            case RACE_MAGHAR_ORC:
                stats.strength += 2;
                break;
            default:
                break;
        }

        // Scale for Evoker starting level
        if (playerClass == CLASS_EVOKER && level == 58)
        {
            stats.health *= 20;
            stats.mana *= 20;
            stats.strength *= 3;
            stats.agility *= 3;
            stats.stamina *= 3;
            stats.intellect *= 3;
            stats.spirit *= 3;  // WoW 12.0: Spirit scaling for Evokers
        }

        return stats;
    }

    // ===================================================================================
    // HELPER FUNCTIONS
    // ===================================================================================

    inline bool IsValidRaceClassCombination(Races race, Classes playerClass)
    {
        for (const auto& combo : VALID_COMBINATIONS)
        {
            if (combo.race == race && combo.playerClass == playerClass)
                return true;
        }
        return false;
    }

    inline bool IsAlliedRace(Races race)
    {
        switch (race)
        {
            case RACE_VOID_ELF:
            case RACE_LIGHTFORGED_DRAENEI:
            case RACE_DARK_IRON_DWARF:
            case RACE_KUL_TIRAN:
            case RACE_MECHAGNOME:
            case RACE_NIGHTBORNE:
            case RACE_HIGHMOUNTAIN_TAUREN:
            case RACE_ZANDALARI_TROLL:
            case RACE_MAGHAR_ORC:
            case RACE_VULPERA:
            case RACE_EARTHEN_DWARF_ALLIANCE:
            case RACE_EARTHEN_DWARF_HORDE:
                return true;
            default:
                return false;
        }
    }

    inline bool IsHeroClass(Classes playerClass)
    {
        switch (playerClass)
        {
            case CLASS_DEATH_KNIGHT:
            case CLASS_DEMON_HUNTER:
            case CLASS_EVOKER:
                return true;
            default:
                return false;
        }
    }

    inline uint8 GetStartingLevel(Races race, Classes playerClass)
    {
        if (playerClass == CLASS_EVOKER)
            return EVOKER_STARTING_LEVEL;

        return DEFAULT_STARTING_LEVEL;
    }

    inline uint8 GetDefaultGender()
    {
        return 0;  // 0 = Male, 1 = Female
    }

    inline uint32 GetCinematicSequence(Races race)
    {
        // These would normally come from ChrRaces.db2
        // Return 0 to skip cinematic for bots
        return 0;
    }

    // ===================================================================================
    // CHARACTER CREATION IMPLEMENTATION GUIDE
    // ===================================================================================

    /*
     * IMPLEMENTATION NOTES FOR BotSpawner::CreateAndSpawnBot():
     *
     * 1. VALIDATE RACE/CLASS COMBINATION:
     *    - Use IsValidRaceClassCombination() to verify
     *    - Check allied race unlock requirements if needed
     *    - Verify hero class realm limits (Evoker/DH)
     *
     * 2. DETERMINE STARTING VALUES:
     *    - Level: Use GetStartingLevel()
     *    - Zone: Look up in STARTING_ZONES map
     *    - Stats: Use GetBaseStats()
     *    - Gender: Use provided or GetDefaultGender()
     *
     * 3. CREATE CHARACTER IN DATABASE:
     *    - Generate unique GUID
     *    - Insert into characters table with:
     *      - account, guid, name, race, class, gender, level
     *      - position_x, position_y, position_z, map, zone
     *      - health, mana (from base stats)
     *      - All other required fields
     *
     * 4. CREATE PLAYER INFO:
     *    - Use ObjectMgr::GetPlayerInfo(race, class) for template data
     *    - Apply starting items from template
     *    - Apply starting spells
     *    - Apply starting action bars
     *
     * 5. SPECIAL HANDLING:
     *    - Pandaren: Start as neutral, faction chosen later
     *    - Dracthyr: Only Evoker class allowed
     *    - Allied Races: May need special unlock flag handling
     *    - Hero Classes: Special starting zone handling
     *
     * 6. EQUIPMENT:
     *    - Apply default starting equipment from PlayerCreateInfo
     *    - Store in character_inventory table
     *
     * 7. SPAWN IN WORLD:
     *    - Create Player object
     *    - Set all attributes from database
     *    - Add to world at starting position
     *    - Initialize AI controller
     *
     * EXAMPLE USAGE:
     *
     * ObjectGuid botGuid;
     * bool success = CreateAndSpawnBot(
     *     accountId,           // Master account ID
     *     CLASS_WARRIOR,       // Class
     *     RACE_HUMAN,          // Race
     *     0,                   // Gender (0=male)
     *     "Botwarrior",        // Name
     *     botGuid              // Output GUID
     * );
     */

} // namespace CharacterCreation
} // namespace Playerbot

#endif // PLAYERBOT_WOW120_CHARACTER_CREATION_H