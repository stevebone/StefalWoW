/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * ============================================================================
 * DEADMINES DUNGEON SCRIPT - COMPREHENSIVE EXAMPLE
 * ============================================================================
 *
 * This file serves as a COMPLETE EXAMPLE of how to create dungeon scripts
 * for the Playerbot module. It demonstrates ALL available functionality:
 *
 * 1. Basic Script Setup
 * 2. Lifecycle Hooks (enter/exit/update)
 * 3. Boss Hooks (engage/kill/wipe)
 * 4. Mechanic Handlers (interrupt/ground/adds/positioning)
 * 5. Custom Boss Logic
 * 6. Utility Methods
 * 7. Fallback to Generic Mechanics
 *
 * DUNGEON INFORMATION:
 * Name: The Deadmines
 * Level: 15-20 (Normal), 85 (Heroic - Cataclysm)
 * Location: Westfall
 * Faction: Alliance-leaning, but accessible to both
 * MapID: 36
 *
 * BOSSES (Normal - Level 15-20):
 * 1. Rhahk'Zor (Entry 644) - Simple tank-and-spank
 * 2. Sneed's Shredder (Entry 642) - Two-phase (shredder + Sneed himself)
 * 3. Gilnid (Entry 1763) - Simple fight
 * 4. Mr. Smite (Entry 646) - Simple fight with adds
 * 5. Captain Greenskin (Entry 647) - Simple fight
 * 6. Edwin VanCleef (Entry 639) - Final boss with adds
 *
 * ============================================================================
 */

#include "DungeonScript.h"
#include "DungeonScriptMgr.h"
#include "Player.h"
#include "Creature.h"
#include "InstanceScript.h"
#include "SpellInfo.h"
#include "Spell.h"  // For Spell class
#include "Log.h"
#include "ObjectAccessor.h"
#include "../../../Spatial/SpatialGridManager.h"
#include "../../../Spatial/DoubleBufferedSpatialGrid.h"

namespace Playerbot
{

/**
 * ============================================================================
 * DEADMINES SCRIPT CLASS
 * ============================================================================
 *
 * This class inherits from DungeonScript and overrides methods to provide
 * Deadmines-specific behavior.
 *
 * OVERRIDE PHILOSOPHY:
 * - Only override what needs custom behavior
 * - For generic mechanics, use base class (calls generic)
 * - Document WHY you're overriding
 */
class DeadminesScript : public DungeonScript
{
public:
    /**
     * Constructor
     * @param name Script identifier (must be unique)
     * @param mapId The Deadmines map ID (36)
     */
    DeadminesScript() : DungeonScript("deadmines", 36)
    {
        TC_LOG_INFO("playerbot", "DeadminesScript: Initialized");
    }

    // ========================================================================
    // LIFECYCLE HOOKS
    // ========================================================================

    /**
     * Called when player enters The Deadmines
     *
     * USE CASE:
     * - Initialize dungeon-specific state
     * - Announce dungeon entry
     * - Set up encounter variables
     *
     * DEFAULT BEHAVIOR: None (base class does nothing)
     * OVERRIDE REASON: Want to log entry and set up state
     */
    void OnDungeonEnter(::Player* player, ::InstanceScript* instance) override
    {
        // Call base class first (good practice)
        DungeonScript::OnDungeonEnter(player, instance);

        TC_LOG_INFO("playerbot", "DeadminesScript: Player {} entered Deadmines",
            player->GetName());

        // Example: Could initialize instance-specific data here
        // instanceData[player->GetGUID()] = DeadminesState();
    }

    /**
     * Called when player exits The Deadmines
     *
     * USE CASE:
     * - Clean up dungeon-specific state
     * - Log completion statistics
     *
     * DEFAULT BEHAVIOR: None
     * OVERRIDE REASON: Want to clean up state
     */
    void OnDungeonExit(::Player* player) override
    {
        DungeonScript::OnDungeonExit(player);

        TC_LOG_INFO("playerbot", "DeadminesScript: Player {} exited Deadmines",
            player->GetName());

        // Example: Clean up instance data
        // instanceData.erase(player->GetGUID());
    }

    /**
     * Called periodically (every 1-5 seconds) while in dungeon
     *
     * USE CASE:
     * - Check for patrol events
     * - Monitor dungeon-wide mechanics
     * - Update timers
     *
     * DEFAULT BEHAVIOR: None
     * OVERRIDE REASON: No continuous mechanics needed in Deadmines
     *
     * NOTE: We DON'T override this - using base class (does nothing)
     */
    // void OnUpdate(::Player* player, uint32 diff) override { }

    // ========================================================================
    // BOSS HOOKS
    // ========================================================================

    /**
     * Called when ANY boss in Deadmines is engaged
     *
     * USE CASE:
     * - Route to boss-specific handlers
     * - Set up boss-specific mechanics
     * - Initialize encounter state
     *
     * DEFAULT BEHAVIOR: None
     * OVERRIDE REASON: Need boss-specific logic
     */
    void OnBossEngage(::Player* player, ::Creature* boss) override
    {
        if (!boss)
            return;

        // Route to boss-specific handlers
    switch (boss->GetEntry())
        {
            case 644: // Rhahk'Zor
                HandleRhahkZorEngage(player, boss);
                break;

            case 642: // Sneed's Shredder
                HandleSneedShredderEngage(player, boss);
                break;

            case 1763: // Gilnid
                HandleGilnidEngage(player, boss);
                break;

            case 646: // Mr. Smite
                HandleMrSmiteEngage(player, boss);
                break;

            case 647: // Captain Greenskin
                HandleGreenskinEngage(player, boss);
                break;

            case 639: // Edwin VanCleef
                HandleVanCleefEngage(player, boss);
                break;

            default:
                TC_LOG_WARN("playerbot", "DeadminesScript: Unknown boss entry {}",
                    boss->GetEntry());
                break;
        }
    }

    /**
     * Called when boss is killed
     *
     * USE CASE:
     * - Clean up boss-specific state
     * - Handle post-boss events
     *
     * DEFAULT BEHAVIOR: None
     * OVERRIDE REASON: Want to log boss kills
     */
    void OnBossKill(::Player* player, ::Creature* boss) override
    {
        if (!boss)
            return;

        TC_LOG_INFO("playerbot", "DeadminesScript: Player {} killed boss {} ({})",
            player->GetName(), boss->GetName(), boss->GetEntry());

        // Example: Could award bonus loot or trigger events
    }

    /**
     * Called when group wipes on boss
     *
     * USE CASE:
     * - Reset boss-specific state
     * - Handle wipe recovery
     *
     * DEFAULT BEHAVIOR: None
     * OVERRIDE REASON: No special wipe handling needed
     *
     * NOTE: We DON'T override this - using base class
     */
    // void OnBossWipe(::Player* player, ::Creature* boss) override { }

    // ========================================================================
    // MECHANIC HANDLERS
    // ========================================================================

    /**
     * Handle interrupt priority for Deadmines bosses
     *
     * USE CASE:
     * - Define which spells MUST be interrupted
     * - Override generic interrupt logic
     *
     * DEFAULT BEHAVIOR: Generic interrupts (heals > damage > CC)
     * OVERRIDE REASON: Some bosses have specific spells to interrupt
     *
     * EXAMPLE: Captain Greenskin's Cleave should be interrupted
     */
    void HandleInterruptPriority(::Player* player, ::Creature* boss) override
    {
        if (!player || !boss)
            return;

        // Boss-specific interrupt logic
    switch (boss->GetEntry())
        {
            case 647: // Captain Greenskin
            {
                // Check if casting Cleave (SpellID 40504)
    if (boss->HasUnitState(UNIT_STATE_CASTING))
                {
                    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                    if (currentSpell && currentSpell->m_spellInfo)
                    {
                        uint32 spellId = currentSpell->m_spellInfo->Id;
                        if (spellId == 40504) // Cleave
                        {
                            if (HasInterruptAvailable(player))
                            {
                                UseInterruptSpell(player, boss);
                                TC_LOG_DEBUG("playerbot",
                                    "DeadminesScript: Interrupted Greenskin's Cleave");
                                return;
                            }
                        }
                    }
                }
                break;
            }

            case 639: // Edwin VanCleef
            {
                // VanCleef doesn't have priority interrupts
                // Fall through to generic
                break;
            }

            default:
                break;
        }

        // For all other cases, use generic interrupt logic
        DungeonScript::HandleInterruptPriority(player, boss);
    }

    /**
     * Handle ground effect avoidance
     *
     * USE CASE:
     * - Avoid boss-specific ground effects
     * - Custom ground pattern handling
     *
     * DEFAULT BEHAVIOR: Generic ground avoidance (detect and move)
     * OVERRIDE REASON: Using spatial grid for enterprise-grade consistency
     */
    void HandleGroundAvoidance(::Player* player, ::Creature* boss) override
    {
        // ENTERPRISE: Use lock-free spatial grid for thread-safe DynamicObject queries
        // Deadmines has minimal ground effects but using spatial grid for consistency
        Map* map = player->GetMap();
        if (map)
        {
            DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
            if (!spatialGrid)
            {
                sSpatialGridManager.CreateGrid(map);
                spatialGrid = sSpatialGridManager.GetGrid(map);
            }

            if (spatialGrid)
            {
                // Query nearby dynamic objects using immutable snapshots (lock-free!)
                auto dynamicObjectSnapshots = spatialGrid->QueryNearbyDynamicObjects(player->GetPosition(), 15.0f);

                for (auto const& snapshot : dynamicObjectSnapshots)
                {
                    if (!snapshot.IsActive())
                        continue;

                    if (snapshot.casterGuid != boss->GetGUID())
                        continue;

                    float distance = player->GetExactDist(snapshot.position);
                    if (distance < 10.0f)
                    {
                        if (DynamicObject* dynObj = ObjectAccessor::GetDynamicObject(*player, snapshot.guid))
                        {
                            if (IsDangerousGroundEffect(dynObj))
                            {
                                TC_LOG_DEBUG("playerbot", "DeadminesScript: Avoiding ground effect at distance {:.1f}", distance);
                                MoveAwayFromGroundEffect(player, dynObj);
                                return;
                            }
                        }
                    }
                }
            }
        }

        // Fall back to generic
        DungeonScript::HandleGroundAvoidance(player, boss);
    }

    /**
     * Handle add kill priority
     *
     * USE CASE:
     * - Define which adds to kill first
     * - Boss-specific add priority
     *
     * DEFAULT BEHAVIOR: Generic priority (healers > casters > low health)
     * OVERRIDE REASON: Sneed's Shredder has specific add priority
     *
     * IMPORTANT: Sneed emerges from shredder when it dies - MUST kill shredder first
     */
    void HandleAddPriority(::Player* player, ::Creature* boss) override
    {
        if (!player || !boss)
            return;

        // Boss-specific add logic
    switch (boss->GetEntry())
        {
            case 642: // Sneed's Shredder
            {
                // CRITICAL: Shredder spawns Sneed when killed
                // Always prioritize shredder until it's dead
                ::Creature* shredder = FindCreatureNearby(player, 642, 50.0f);
                if (shredder && shredder->IsAlive())
                {
                    player->SetSelection(shredder->GetGUID());
                    TC_LOG_DEBUG("playerbot",
                        "DeadminesScript: Prioritizing Sneed's Shredder");
                    return;
                }

                // After shredder dies, Sneed himself spawns (Entry 643)
                ::Creature* sneed = FindCreatureNearby(player, 643, 50.0f);
                if (sneed && sneed->IsAlive())
                {
                    player->SetSelection(sneed->GetGUID());
                    TC_LOG_DEBUG("playerbot", "DeadminesScript: Targeting Sneed");
                    return;
                }
                break;
            }

            case 646: // Mr. Smite
            {
                // Mr. Smite spawns adds - kill adds first if low health
                ::std::vector<::Creature*> adds = GetAddsInCombat(player, boss);
                for (::Creature* add : adds)
                {
                    if (add->GetHealthPct() < 30)
                    {
                        player->SetSelection(add->GetGUID());
                        TC_LOG_DEBUG("playerbot",
                            "DeadminesScript: Prioritizing low-health add for Mr. Smite");
                        return;
                    }
                }
                break;
            }

            case 639: // Edwin VanCleef
            {
                // VanCleef summons two Defias adds at low health
                // Kill adds first if they exist
                ::std::vector<::Creature*> adds = GetAddsInCombat(player, boss);
                if (!adds.empty())
                {
                    player->SetSelection(adds[0]->GetGUID());
                    TC_LOG_DEBUG("playerbot",
                        "DeadminesScript: Prioritizing VanCleef's adds");
                    return;
                }
                break;
            }

            default:
                break;
        }

        // Fall back to generic add priority for other bosses
        DungeonScript::HandleAddPriority(player, boss);
    }

    /**
     * Handle player positioning
     *
     * USE CASE:
     * - Position players for boss mechanics
     * - Custom positioning requirements
     *
     * DEFAULT BEHAVIOR: Tank front, DPS behind, healer at range
     * OVERRIDE REASON: Deadmines has no special positioning at level 15-20
     *
     * NOTE: We DON'T override this - using base class (generic is fine)
     */
    // void HandlePositioning(::Player* player, ::Creature* boss) override { }

    /**
     * Handle dispel mechanics
     *
     * USE CASE:
     * - Dispel boss debuffs/buffs
     *
     * DEFAULT BEHAVIOR: Generic dispel (harmful > helpful)
     * OVERRIDE REASON: No critical dispels in Deadmines
     *
     * NOTE: We DON'T override this - using base class
     */
    // void HandleDispelMechanic(::Player* player, ::Creature* boss) override { }

    /**
     * Handle movement mechanics
     *
     * USE CASE:
     * - Kiting, running out, etc.
     *
     * DEFAULT BEHAVIOR: Maintain optimal range
     * OVERRIDE REASON: No special movement in Deadmines
     *
     * NOTE: We DON'T override this - using base class
     */
    // void HandleMovementMechanic(::Player* player, ::Creature* boss) override { }

    /**
     * Handle tank swap mechanics
     *
     * USE CASE:
     * - Bosses requiring tank swaps
     *
     * DEFAULT BEHAVIOR: No tank swap
     * OVERRIDE REASON: No tank swaps in Deadmines (low-level dungeon)
     *
     * NOTE: We DON'T override this - using base class
     */
    // void HandleTankSwap(::Player* player, ::Creature* boss) override { }

    /**
     * Handle spread mechanic
     *
     * USE CASE:
     * - Spread out for AoE mechanics
     *
     * DEFAULT BEHAVIOR: 10 yards apart
     * OVERRIDE REASON: No spread mechanics in Deadmines
     *
     * NOTE: We DON'T override this - using base class
     */
    // void HandleSpreadMechanic(::Player* player, ::Creature* boss) override { }

    /**
     * Handle stack mechanic
     *
     * USE CASE:
     * - Stack together for healing/buffs
     *
     * DEFAULT BEHAVIOR: Stack on tank
     * OVERRIDE REASON: No stack mechanics in Deadmines
     *
     * NOTE: We DON'T override this - using base class
     */
    // void HandleStackMechanic(::Player* player, ::Creature* boss) override { }

private:
    // ========================================================================
    // BOSS-SPECIFIC HANDLERS (Private)
    // ========================================================================

    /**
     * Handle Rhahk'Zor engagement
     * Boss: Rhahk'Zor (Entry 644)
     * Mechanics: Simple tank-and-spank, no special mechanics
     */
    void HandleRhahkZorEngage(::Player* player, ::Creature* boss)
    {
        TC_LOG_INFO("playerbot", "DeadminesScript: Engaging Rhahk'Zor");

        // No special mechanics - generic positioning will handle
        // Tank tanks, DPS DPS, healer heals
    }

    /**
     * Handle Sneed's Shredder engagement
     * Boss: Sneed's Shredder (Entry 642) -> Sneed (Entry 643)
     * Mechanics: Two-phase fight
     *   Phase 1: Kill shredder
     *   Phase 2: Sneed emerges, kill him
     */
    void HandleSneedShredderEngage(::Player* player, ::Creature* boss)
    {
        TC_LOG_INFO("playerbot", "DeadminesScript: Engaging Sneed's Shredder");

        // Important: HandleAddPriority() will ensure shredder is killed first
        // Then automatically switch to Sneed when he spawns
    }

    /**
     * Handle Gilnid engagement
     * Boss: Gilnid (Entry 1763)
     * Mechanics: Simple fight, occasional adds
     */
    void HandleGilnidEngage(::Player* player, ::Creature* boss)
    {
        TC_LOG_INFO("playerbot", "DeadminesScript: Engaging Gilnid");

        // Generic add priority will handle any adds that spawn
    }

    /**
     * Handle Mr. Smite engagement
     * Boss: Mr. Smite (Entry 646)
     * Mechanics: Spawns adds at health thresholds
     */
    void HandleMrSmiteEngage(::Player* player, ::Creature* boss)
    {
        TC_LOG_INFO("playerbot", "DeadminesScript: Engaging Mr. Smite");

        // HandleAddPriority() will kill low-health adds first
    }

    /**
     * Handle Captain Greenskin engagement
     * Boss: Captain Greenskin (Entry 647)
     * Mechanics: Cleave (should be interrupted if possible)
     */
    void HandleGreenskinEngage(::Player* player, ::Creature* boss)
    {
        TC_LOG_INFO("playerbot", "DeadminesScript: Engaging Captain Greenskin");

        // HandleInterruptPriority() will interrupt Cleave
    }

    /**
     * Handle Edwin VanCleef engagement
     * Boss: Edwin VanCleef (Entry 639) - Final Boss
     * Mechanics: Summons two adds at 50% health
     */
    void HandleVanCleefEngage(::Player* player, ::Creature* boss)
    {
        TC_LOG_INFO("playerbot", "DeadminesScript: Engaging Edwin VanCleef (Final Boss)");

        // HandleAddPriority() will kill adds when they spawn at 50%
    }
};

// ============================================================================
// REGISTRATION FUNCTION
// ============================================================================

/**
 * Registration function for Deadmines script
 *
 * This function is called by DungeonScriptLoader to register the script
 * with the DungeonScriptMgr.
 *
 * NAMING CONVENTION: AddSC_<dungeonname>_playerbot
 *
 * IMPORTANT: This must be declared in DungeonScriptLoader.h and called
 * in DungeonScriptLoader.cpp
 */
void AddSC_deadmines_playerbot()
{
    DeadminesScript* script = new DeadminesScript();

    // Register script for map
    DungeonScriptMgr::instance()->RegisterScript(script);

    // Register individual bosses (allows boss-specific lookups)
    DungeonScriptMgr::instance()->RegisterBossScript(644, script);  // Rhahk'Zor
    DungeonScriptMgr::instance()->RegisterBossScript(642, script);  // Sneed's Shredder
    DungeonScriptMgr::instance()->RegisterBossScript(643, script);  // Sneed
    DungeonScriptMgr::instance()->RegisterBossScript(1763, script); // Gilnid
    DungeonScriptMgr::instance()->RegisterBossScript(646, script);  // Mr. Smite
    DungeonScriptMgr::instance()->RegisterBossScript(647, script);  // Captain Greenskin
    DungeonScriptMgr::instance()->RegisterBossScript(639, script);  // Edwin VanCleef
}

} // namespace Playerbot

/**
 * ============================================================================
 * USAGE GUIDE FOR CREATING NEW DUNGEON SCRIPTS
 * ============================================================================
 *
 * STEP 1: Copy this file as a template
 * - Rename to YourDungeonScript.cpp
 * - Change class name: class YourDungeonScript : public DungeonScript
 * - Update constructor with correct name and mapId
 *
 * STEP 2: Identify boss mechanics
 * - List all bosses and their entry IDs
 * - Document mechanics that need custom handling
 * - Note which mechanics can use generic fallback
 *
 * STEP 3: Override only what you need
 * - DON'T override methods you don't need to customize
 * - Use base class (calls generic) for standard mechanics
 * - Only override for boss-specific behavior
 *
 * STEP 4: Implement boss handlers
 * - Create HandleXXXEngage() methods for each boss
 * - Keep boss logic in private methods for organization
 * - Use utility methods from base class
 *
 * STEP 5: Register the script
 * - Create AddSC_yourdungeon_playerbot() function
 * - Register script with DungeonScriptMgr
 * - Register individual bosses for boss-specific lookups
 * - Add to DungeonScriptLoader.h and .cpp
 *
 * STEP 6: Test fallback behavior
 * - Test with script enabled (custom behavior)
 * - Test with script disabled (generic fallback)
 * - Verify both paths work correctly
 *
 * ============================================================================
 * BEST PRACTICES
 * ============================================================================
 *
 * 1. DOCUMENTATION
 *    - Document WHY you override each method
 *    - Explain boss mechanics in comments
 *    - Note spell IDs and creature entries
 *
 * 2. FALLBACK PHILOSOPHY
 *    - Only override when generic won't work
 *    - Call base class for standard behavior
 *    - Trust the generic mechanics
 *
 * 3. MAINTAINABILITY
 *    - Keep boss handlers in private methods
 *    - Group related functionality
 *    - Use meaningful variable names
 *
 * 4. PERFORMANCE
 *    - Avoid expensive operations in OnUpdate()
 *    - Cache frequently-used lookups
 *    - Use early returns
 *
 * 5. ERROR HANDLING
 *    - Always null-check parameters
 *    - Use TC_LOG_* for debugging
 *    - Handle edge cases gracefully
 *
 * ============================================================================
 */
