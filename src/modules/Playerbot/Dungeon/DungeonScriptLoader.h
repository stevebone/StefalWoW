/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

/**
 * @brief Dungeon Script Loader - Registers all dungeon scripts at startup
 *
 * This class follows TrinityCore's ScriptLoader pattern, providing a central
 * registration point for all dungeon scripts. It is called during server
 * initialization to register scripts with the DungeonScriptMgr.
 *
 * ARCHITECTURE:
 * - Similar to TrinityCore's ScriptLoader
 * - Called once during server startup
 * - Calls all AddSC_*_playerbot() registration functions
 * - Thread-safe initialization
 * - No external dependencies except script headers
 *
 * USAGE:
 * In main server initialization (e.g., World.cpp or module loader):
 * ```cpp
 * #include "DungeonScriptLoader.h"
 *
 * void InitializePlayerbotModule()
 * {
 *     Playerbot::DungeonScriptLoader::LoadDungeonScripts();
 * }
 * ```
 *
 * ADDING NEW SCRIPTS:
 * 1. Create new dungeon script file (e.g., MyDungeonScript.cpp)
 * 2. Implement AddSC_my_dungeon_playerbot() function
 * 3. Add declaration in SCRIPT DECLARATIONS section below
 * 4. Add call in LoadDungeonScripts() method
 */

namespace Playerbot
{

class DungeonScriptLoader
{
public:
    /**
     * Load all dungeon scripts
     * Called once during server initialization
     */
    static void LoadDungeonScripts();

private:
    // Prevent instantiation
    DungeonScriptLoader() = delete;
    ~DungeonScriptLoader() = delete;
    DungeonScriptLoader(DungeonScriptLoader const&) = delete;
    DungeonScriptLoader& operator=(DungeonScriptLoader const&) = delete;
};

// ============================================================================
// SCRIPT DECLARATIONS
// ============================================================================

// Forward declarations for all dungeon script registration functions
// These are implemented in individual dungeon script files

// ============================================================================
// VANILLA DUNGEONS (Level 13-45)
// ============================================================================

/**
 * Deadmines (Map 36, Level 15-21)
 * Bosses: Rhahk'Zor, Sneed, Gilnid, Mr. Smite, Captain Greenskin, Edwin VanCleef
 */
void AddSC_deadmines_playerbot();

/**
 * Ragefire Chasm (Map 389, Level 13-18)
 * Bosses: Oggleflint, Taragaman, Jergosh, Bazzalan
 */
void AddSC_ragefire_chasm_playerbot();

/**
 * Wailing Caverns (Map 43, Level 15-25)
 * Bosses: Lady Anacondra, Lord Cobrahn, Lord Pythas, Lord Serpentis,
 *         Skum, Verdan, Mutanus the Devourer
 */
void AddSC_wailing_caverns_playerbot();

/**
 * The Stockade (Map 34, Level 15-30)
 * Bosses: Kam Deepfury, Hamhock, Bazil Thredd, Dextren Ward
 */
void AddSC_stockade_playerbot();

/**
 * Shadowfang Keep (Map 33, Level 18-25)
 * Bosses: Baron Ashbury, Baron Silverlaine, Commander Springvale,
 *         Lord Walden, Lord Godfrey/Archmage Arugal
 */
void AddSC_shadowfang_keep_playerbot();

/**
 * Blackfathom Deeps (Map 48, Level 18-24)
 * Bosses: Ghamoo-ra, Lady Sarevess, Gelihast, Lorgus Jett,
 *         Baron Aquanis, Twilight Lord Kelris, Aku'mai
 */
void AddSC_blackfathom_deeps_playerbot();

/**
 * Gnomeregan (Map 90, Level 24-34)
 * Bosses: Grubbis, Viscous Fallout, Electrocutioner 6000,
 *         Crowd Pummeler 9-60, Mekgineer Thermaplugg
 */
void AddSC_gnomeregan_playerbot();

/**
 * Razorfen Kraul (Map 47, Level 25-35)
 * Bosses: Roogug, Aggem Thorncurse, Death Speaker Jargba,
 *         Overlord Ramtusk, Agathelos the Raging, Charlga Razorflank
 */
void AddSC_razorfen_kraul_playerbot();

/**
 * Scarlet Monastery - All Wings (Map 189, Level 26-45)
 * Wings: Graveyard, Library, Armory, Cathedral
 * Bosses: Vishas, Bloodmage Thalnos, Ironspine, Azshir the Sleepless,
 *         Houndmaster Loksey, Arcanist Doan, Herod, High Inquisitor Fairbanks,
 *         Scarlet Commander Mograine, High Inquisitor Whitemane
 */
void AddSC_scarlet_monastery_playerbot();

/**
 * Razorfen Downs (Map 129, Level 35-45)
 * Bosses: Tuten'kash, Mordresh Fire Eye, Glutton,
 *         Ragglesnout, Amnennar the Coldbringer
 */
void AddSC_razorfen_downs_playerbot();

// ============================================================================
// FUTURE EXPANSIONS
// ============================================================================

// TBC Dungeons would be added here
// void AddSC_hellfire_ramparts_playerbot();
// void AddSC_blood_furnace_playerbot();
// etc.

// WotLK Dungeons would be added here
// void AddSC_utgarde_keep_playerbot();
// void AddSC_nexus_playerbot();
// etc.

} // namespace Playerbot
