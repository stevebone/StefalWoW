-- Duskwood Fixes

-- NPC: 3 Flesh Eater
-- NPC: 48 Skeletal Warrior
-- NPC: 202 Rotting Horror
-- NPC: 203 Skeletal Mage
-- NPC: 206 Nightbane Vile Fang
-- NPC: 210 Bone Chewer
-- NPC: 212 Splinter Fist Warrior
-- NPC: 217 Venom Web Spider
-- NPC: 218 Grave Robber
-- NPC: 300 Zzarc
-- NPC: 315 Stalvan Mistmantle
-- NPC: 511 Insane Ghoul
-- NPC: 533 Nightbane Shadow Weaver
-- NPC: 604 Plague Spreader
-- NPC: 628 Black Ravager
-- NPC: 889 Splinter Fist Ogre
-- NPC: 898 Nightbane Worgen
-- NPC: 920 Nightbane Tainted One
-- NPC: 930 Black Widow Hatchling
-- NPC: 948 Rotted One
-- NPC: 1251 Splinter Fist Firemonger
-- NPC: 1270 Fetid Corpse
-- NPC: 43704 Dire Wolf
-- NPC: 43732 Corpseweed
-- NPC: 43862 Stiches
-- NPC: 43923 Forlorn Spirit
-- NPC: 44016 Coalpelt Bear
-- NPC: 44020 Barn Owl
-- NPC: 44087 Nightbane Stalker
-- NPC: 44089 Blackbelly Forager
-- NPC: 45582 Black Widow
-- NPC: 45614 Anguished Spirit
-- NPC: 45811 Marina DeSirrus

-- NPC: 43799 Lurking Worgen
-- NPC: 43814 Lurking Worgen (Addle Stead)
-- NPC: 43950 Lurking Worgen (Raven Hill)
-- NPC: 43969 Cry For The Moon Credit
-- NPC: 43859 Spawned Jitters
-- NPC: 43858 Spawned Oliver Harris
-- NPC: 43861 Sven Yorgen

-- Quest: 26717 The Yorgen Worgen
-- Quest: 26720 A Curse We Cannot Lift
-- Quest: 26727 The Embalmer's Revenge
-- Quest: 26674 Mistmantle's Revenge
-- Quest: 26760 Cry For The Moon

-- Phase: 245 Stiches Attacks

-- Spell: 76630 Detect: Quest Invis 1
-- Spell: 82288 Raven Hill Questgiver Vision (Quest 01)
-- Spell: 82289 Raven Hill Questgiver Vision (Quest 02)
-- Spell: 82293 Cry For The Moon Quest Invis - Accept Cancel
-- Spell: 82286 Cry For The Moon - Regain Quest Invis Detection
-- Spell: 82056 Summon Jitters
-- Spell: 82055 Summon Oliver Harris
-- Spell: 82266 Duskwood Chocked by Sven

-- Spell Area
DELETE FROM `spell_area` WHERE (`spell` = 82289 AND `area` = 94);
INSERT INTO `spell_area` (`spell`, `area`, `quest_start`, `quest_start_status`, `quest_end_status`, `quest_end`, `aura_spell`, `racemask`, `gender`, `flags`) VALUES
(82289, 94, 26760, 2, 0, 0, 0, 0, 2, 3);

DELETE FROM `spell_area` WHERE (`spell` = 82288 AND `area` = 94);
INSERT INTO `spell_area` (`spell`, `area`, `quest_start`, `quest_start_status`, `quest_end_status`, `quest_end`, `aura_spell`, `racemask`, `gender`, `flags`) VALUES
(82288, 94, 0, 0, 1, 26760, 0, 0, 2, 3),
(82288, 94, 26760, 2, 0, 0, 0, 0, 2, 3);

-- Phases
DELETE FROM `phase_area` WHERE `PhaseId` IN (245);
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(42, 245, 'Duskwood - Darkshire');

-- Phase Conditions
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` IN (245);
INSERT INTO `conditions` VALUES
-- Darkshire Stiches
(26, 245, 0, 0, 0, 47, 0, 26727, 10, 0, '', 0, 0, 0, '', 'Darkshire Stiches - Phase 245 active if 26727 taken, complete');

-- Creature Difficulty
DELETE FROM `creature_template_difficulty` WHERE `DifficultyID` = 1 AND `Entry` IN (
48, 202, 203, 217, 511, 533, 898, 930, 1270, 43704, 43732, 44087, 44089, 628, 1251, 
212, 44020, 300, 889, 45582, 44016, 210, 604, 948, 3, 218, 45811, 43862, 206, 920,
315, 43923, 45614, 43814
);

UPDATE `creature_template_difficulty` SET `DamageModifier` = 0.2 WHERE `Entry` IN (
48, 202, 203, 217, 511, 533, 898, 930, 1270, 43704, 43732, 44087, 44089, 628, 1251, 
212, 44020, 300, 889, 45582, 44016, 210, 604, 948, 3, 218, 45811, 43862, 206, 920,
315, 43923, 45614, 43814
);

UPDATE `creature_template_difficulty` SET `SkinLootID` = 3 WHERE `Entry` IN (43704,44016);
UPDATE `creature_template_difficulty` SET `LootID` = 44087 WHERE `Entry` = 44087;
UPDATE `creature_template_difficulty` SET `LootID` = 44089, `SkinLootID` = 3 WHERE `Entry` = 44089;
UPDATE `creature_template_difficulty` SET `LootID` = 44020 WHERE `Entry` = 44020;
UPDATE `creature_template_difficulty` SET `LootID` = 45582 WHERE `Entry` = 45582;
UPDATE `creature_template_difficulty` SET `LootID` = 45811 WHERE `Entry` = 45811;
UPDATE `creature_template_difficulty` SET `LootID` = 43862, `GoldMin` = 300, `GoldMax` = 400 WHERE `Entry` = 43862;
UPDATE `creature_template_difficulty` SET `LootID` = 43923, `GoldMin` = 500, `GoldMax` = 800 WHERE `Entry` = 43923;
UPDATE `creature_template_difficulty` SET `LootID` = 45614, `GoldMin` = 500, `GoldMax` = 800 WHERE `Entry` = 45614;

-- Remove incorrect template flags
UPDATE `creature_template` SET `unit_flags` = 0 WHERE `entry` IN (44089);

-- Fix template factions
UPDATE `creature_template` SET `faction` = 11 WHERE `entry` IN (2470,887,576,11040);

-- Add template vehicle ids
UPDATE `creature_template` SET `VehicleId` = 1007 WHERE `entry` = 43950;

-- Creature Template Addons
UPDATE `creature_template_addon` SET `auras` = '49415' WHERE `Entry` = 43861;

-- Wrong Creature Spawns
DELETE FROM `creature` WHERE `guid` = 317602 AND `id` = 494;
DELETE FROM `creature` WHERE `guid` = 317599 AND `id` = 228;
DELETE FROM `creature` WHERE `guid` = 317779 AND `id` = 272;
-- remove the Lurking Worgen spawn as it is now spawned by script
DELETE FROM `creature` WHERE `id` IN (43799); 

-- New Creature Spawns
SET @CGUID := 900000;
DELETE FROM `creature` WHERE `guid` IN (@CGUID+1246,@CGUID+3142);
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `phaseId`, `phaseGroup`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`) VALUES
-- Marus <The Pack Leader>
(@CGUID+1246, 45771, 0, 10, 241, 0, 0, 0, 0, 1, -11065.4, -778.968, 63.8591, 5.47845, 3600),
(@CGUID+3142, 43861, 0, 10, 94, 0, 0, 0, 0, 0, -10761.7, 337.977, 37.8645, 5.2709, 180);

-- Creature Spawn Fixes
UPDATE creature
SET MovementType = 1,
    wander_distance = 8
WHERE id IN (
    889,212,628,44016,44020,210,604,948,43923,45614,
    43732,44089,44087,533,202,930,898,205,45517,217,
    48,203,206,920,3,827,1270
);

-- Creature Spawn Fixes
UPDATE creature
SET MovementType = 1,
    wander_distance = 3
WHERE id IN (
    494,228,886,999,2470,3137,885,1673
);

-- Specific quest related creature spawn changes
-- 317260 dead Blackbelly Forager at Yorgen Farmstead
UPDATE creature SET MovementType = 0, wander_distance = 0, `unit_flags` = 537133824, `unit_flags3` = 8192 WHERE `guid` IN (317260);
-- Creature Addons
DELETE FROM creature_addon WHERE guid IN (317260);
INSERT INTO creature_addon (guid, StandState) VALUES (317260, 7);

-- Stiches creature phase flag
UPDATE `creature` SET `phaseUseFlags` = 1 WHERE `guid` IN (317620,317622,317623,317624,317696,317748,317754,317756,317758,317761,317776);

-- Quest: 26717 The Yorgen Worgen - Script Names
UPDATE `creature_template` SET `ScriptName` = 'npc_apprentice_fess' WHERE `entry` = 43738;
UPDATE `creature_template` SET `ScriptName` = 'npc_lurking_worgen' WHERE `entry` = 43799;
UPDATE `gameobject_template` SET `ScriptName` = 'go_mound_of_loose_dirt' WHERE `entry` = 204777;

-- Quest: 26720 A Curse We Cannot Lift - Script Names
UPDATE `creature_template` SET `ScriptName` = 'npc_lurking_worgen_addle_stead' WHERE `entry` = 43814;

-- Quest: 26727 The Embalmer's Revenge - Script Names
UPDATE `creature_template` SET `ScriptName` = 'npc_ello_ebonlocke' WHERE `entry` = 263;
UPDATE `creature_template` SET `ScriptName` = 'npc_stiches' WHERE `entry` = 43862;

-- Quest: 26674 Mistmantle's Revenge - Script Names
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_call_stalvan';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(82029, 'spell_call_stalvan');

UPDATE `creature_template` SET `ScriptName` = 'npc_stalvan_mistmantle' WHERE `entry` = 315;
UPDATE `creature_template` SET `ScriptName` = 'npc_tobias_mistmantle' WHERE `entry` = 43453;

-- Quest: 26760 Cry For The Moon - Script Names
UPDATE `creature_template` SET `ScriptName` = 'npc_spawned_oliver_harris' WHERE `entry` = 43858;
UPDATE `creature_template` SET `ScriptName` = 'npc_spawned_jitters' WHERE `entry` = 43859;
UPDATE `creature_template` SET `ScriptName` = 'npc_lurking_worgen_raven_hill' WHERE `entry` = 43950;
UPDATE `quest_template_addon` SET `ScriptName` = 'quest_26760_cry_for_the_moon' WHERE `ID` = 26760;

-- GO Spawns
SET @OGUID := 900000;
DELETE FROM `gameobject` WHERE `guid` BETWEEN @OGUID+197 AND @OGUID+225;
INSERT INTO `gameobject` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `terrainSwapMap`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`, `ScriptName`, `StringId`, `VerifiedBuild`) VALUES
-- Spell focus for Manor Mistmantle
(@OGUID+225, 204811, 0, 10, 1098, 0, 0, 0, 0, -1, -10369.9, -1254.05, 35.9097, 2.12838, -0, -0, -0.874397, -0.485212, 300, 255, 1,'',NULL,0),
-- Go spawns for darkshire Stich attack phase
(@OGUID+197,204099,0,10,42,0,0,245,0,-1,-10543.1,-1164.34,27.8866,2.31095,-0,-0,-0.914987,-0.403482,300,255,1,'',NULL,0),
(@OGUID+198,204099,0,10,42,0,0,245,0,-1,-10541.3,-1157.03,33.6871,6.27445,-0,-0,-0.0043662,0.999991,300,255,1,'',NULL,0),
(@OGUID+199,204099,0,10,42,0,0,245,0,-1,-10533.3,-1207.74,28.119,5.03236,-0,-0,-0.585431,0.810722,300,255,1,'',NULL,0),
(@OGUID+200,204099,0,10,42,0,0,245,0,-1,-10529.3,-1205.4,32.8804,1.59739,-0,-0,-0.716447,-0.697641,300,255,1,'',NULL,0),
(@OGUID+201,204099,0,10,42,0,0,245,0,-1,-10537.1,-1206.17,32.8797,1.56887,-0,-0,-0.706426,-0.707787,300,255,1,'',NULL,0),
(@OGUID+202,204099,0,10,42,0,0,245,0,-1,-10533.9,-1203.86,41.2777,3.03978,-0,-0,-0.998705,-0.0508844,300,255,1,'',NULL,0),
(@OGUID+203,204099,0,10,42,0,0,245,0,-1,-10583.6,-1197.67,28.2591,3.21037,-0,-0,-0.999409,0.0343805,300,255,1,'',NULL,0),
(@OGUID+204,204099,0,10,42,0,0,245,0,-1,-10583.6,-1201.41,33.2623,0.0659838,-0,-0,-0.0329863,-0.999456,300,255,1,'',NULL,0),
(@OGUID+205,204099,0,10,42,0,0,245,0,-1,-10584.6,-1194.72,33.2616,0.0699107,-0,-0,-0.0349485,-0.999389,300,255,1,'',NULL,0),
(@OGUID+206,204099,0,10,42,0,0,245,0,-1,-10575.3,-1139.84,27.6212,2.043,-0,-0,-0.852892,-0.522087,300,255,1,'',NULL,0),
(@OGUID+207,204099,0,10,42,0,0,245,0,-1,-10565.1,-1146.47,27.7659,1.08308,-0,-0,-0.515456,-0.856916,300,255,1,'',NULL,0),
(@OGUID+208,204099,0,10,42,0,0,245,0,-1,-10562.4,-1138.75,33.132,4.77734,-0,-0,-0.683774,0.729694,300,255,1,'',NULL,0),
(@OGUID+209,204099,0,10,42,0,0,245,0,-1,-10553.5,-1138.12,33.3039,2.80657,-0,-0,-0.986002,-0.166731,300,255,1,'',NULL,0),
(@OGUID+210,204099,0,10,42,0,0,245,0,-1,-10551.3,-1136.93,29.248,2.78912,-0,-0,-0.984511,-0.175325,300,255,1,'',NULL,0),
(@OGUID+211,204099,0,10,42,0,0,245,0,-1,-10546.2,-1197.12,27.2321,0.0705008,-0,-0,-0.035243,-0.999379,300,255,1,'',NULL,0),
(@OGUID+212,204099,0,10,42,0,0,245,0,-1,-10543.5,-1196.89,36.4273,5.12177,-0,-0,-0.548615,0.836075,300,255,1,'',NULL,0),
(@OGUID+213,204099,0,10,42,0,0,245,0,-1,-10547.2,-1198.23,41.625,4.75147,-0,-0,-0.693156,0.720788,300,255,1,'',NULL,0),
(@OGUID+214,204099,0,10,42,0,0,245,0,-1,-10565.7,-1222.74,26.333,4.71509,-0,-0,-0.706151,0.708061,300,255,1,'',NULL,0),
(@OGUID+215,204099,0,10,42,0,0,245,0,-1,-10562.2,-1227.14,26.8703,4.33128,-0,-0,-0.828235,0.560381,300,255,1,'',NULL,0),
(@OGUID+216,204099,0,10,42,0,0,245,0,-1,-10585.3,-1165.17,30.0118,2.53769,-0,-0,-0.954759,-0.297382,300,255,1,'',NULL,0),
(@OGUID+217,204099,0,10,42,0,0,245,0,-1,-10578.6,-1155.32,27.5905,3.17502,-0,-0,-0.99986,0.0167149,300,255,1,'',NULL,0),
(@OGUID+218,204099,0,10,42,0,0,245,0,-1,-10584.2,-1188.63,27.7331,4.07313,-0,-0,-0.893476,0.449112,300,255,1,'',NULL,0),
(@OGUID+219,204099,0,10,42,0,0,245,0,-1,-10592.3,-1193.53,28.5001,1.50939,-0,-0,-0.685066,-0.728481,300,255,1,'',NULL,0),
(@OGUID+220,204099,0,10,42,0,0,245,0,-1,-10602.7,-1195.11,29.033,0.880036,-0,-0,-0.425956,-0.904744,300,255,1,'',NULL,0),
(@OGUID+221,204099,0,10,42,0,0,245,0,-1,-10570.4,-1212.4,26.1221,4.08898,-0,-0,-0.88989,0.456176,300,255,1,'',NULL,0),
(@OGUID+222,204099,0,10,42,0,0,245,0,-1,-10580.1,-1216.5,26.6655,4.26904,-0,-0,-0.845272,0.534336,300,255,1,'',NULL,0),
(@OGUID+223,204099,0,10,42,0,0,245,0,-1,-10537.1,-1171.22,28.1035,3.44543,-0,-0,-0.988483,0.151333,300,255,1,'',NULL,0),
(@OGUID+224,204099,0,10,42,0,0,245,0,-1,-10502.4,-1249.73,41.1167,4.23924,-0,-0,-0.853138,0.521685,300,255,1,'',NULL,0);
