-- Duskwood Fixes

-- NPC: 48 Skeletal Warrior
-- NPC: 202 Rotting Horror
-- NPC: 203 Skeletal Mage
-- NPC: 217 Venom Web Spider
-- NPC: 511 Insane Ghoul
-- NPC: 533 Nightbane Shadow Weaver
-- NPC: 898 Nightbane Worgen
-- NPC: 930 Black Widow Hatchling
-- NPC: 1270 Fetid Corpse
-- NPC: 43704 Dire Wolf
-- NPC: 43732 Corpseweed
-- NPC: 44087 Nightbane Stalker
-- NPC: 44089 Blackbelly Forager

-- Quest: 26717 The Yorgen Worgen

-- Creature Difficulty
DELETE FROM `creature_template_difficulty` WHERE `DifficultyID` = 1 AND `Entry` IN (
48, 202, 203, 217, 511, 533, 898, 930, 1270, 43704, 43732, 44087, 44089
);

UPDATE `creature_template_difficulty` SET `DamageModifier` = 0.2 WHERE `Entry` IN (48, 202, 203, 217, 511, 533, 898, 930, 1270, 43704, 43732, 44087, 44089);

UPDATE `creature_template_difficulty` SET `SkinLootID` = 3 WHERE `Entry` = 43704;
UPDATE `creature_template_difficulty` SET `LootID` = 44087 WHERE `Entry` = 44087;
UPDATE `creature_template_difficulty` SET `LootID` = 44089, `SkinLootID` = 3 WHERE `Entry` = 44089;

-- Remove incorrect template flags
UPDATE `creature_template` SET `unit_flags` = 0 WHERE `entry` IN (44089);

-- Wrong Creature Spawns
DELETE FROM `creature` WHERE `guid` = 317602 AND `id` = 494;
DELETE FROM `creature` WHERE `guid` = 317599 AND `id` = 228;
DELETE FROM `creature` WHERE `guid` = 317779 AND `id` = 272;

-- New Creature Spawns
SET @CGUID := 900000;
DELETE FROM `creature` WHERE `guid` = @CGUID+1246;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `phaseId`, `phaseGroup`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`) VALUES
-- Marus <The Pack Leader>
(@CGUID+1246, 45771, 0, 10, 241, 0, 0, 0, 0, 1, -11065.4, -778.968, 63.8591, 5.47845, 3600);

-- Creature Spawn Fixes
UPDATE creature
SET MovementType = 1,
    wander_distance = 8
WHERE id IN (
    889,212,628,44016,44020,210,604,948,43923,45614,
    43732,44089,44087,533,202,930,898,205,45517,217,
    48,203,206,920,3,827,828
);

-- Creature Spawn Fixes
UPDATE creature
SET MovementType = 1,
    wander_distance = 3
WHERE id IN (
    494,228,886,999
);

-- Specific quest related creature spawn changes
-- 317260 dead Blackbelly Forager at Yorgen Farmstead
UPDATE creature SET MovementType = 0, wander_distance = 0, `unit_flags` = 537133824, `unit_flags3` = 8192 WHERE `guid` IN (317260);
-- Creature Addons
DELETE FROM creature_addon WHERE guid IN (317260);
INSERT INTO creature_addon (guid, StandState) VALUES (317260, 7);

