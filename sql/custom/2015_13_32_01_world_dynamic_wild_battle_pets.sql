-- Dynamic wild battle pet marking
-- Instead of creature_template having UNIT_NPC_FLAG_WILD_BATTLE_PET (0x40000000) baked in,
-- the server now dynamically assigns this flag at spawn time to ~40% of eligible critters.
-- This mimics retail behavior where regular critters are selectively marked for pet battles.
--
-- Uses impossible date (2015_13_32) per project conventions to avoid upstream conflicts.

-- Step 1: Strip UNIT_NPC_FLAG_WILD_BATTLE_PET (0x40000000) from all creature_template entries.
-- The C++ code now handles this flag dynamically based on BattlePetSpecies DB2 data.
-- This ensures creature templates define the base critter, not the battle pet version.
UPDATE `creature_template` SET `npcflag` = `npcflag` & ~0x40000000
WHERE (`npcflag` & 0x40000000) != 0;

-- Step 2: Remove dedicated battle pet spawn entries from the old approach.
-- These were manually placed spawns that are no longer needed — the existing world critter
-- spawns will be dynamically marked as wild battle pets by the server.
DELETE FROM `creature` WHERE `guid` BETWEEN 900000 AND 900030;
