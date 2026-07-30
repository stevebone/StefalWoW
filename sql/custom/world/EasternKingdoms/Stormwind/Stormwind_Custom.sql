-- Stormwind custom stuff (mainly bringing back removed stuff)

-- Quests: 1274 The Missing Diplomat
-- Quests: 1241 The Missing Diplomat
-- Quests: 1242 The Missing Diplomat
-- Quests: 1243 The Missing Diplomat
-- Quests: 1244 The Missing Diplomat
-- Quests: 1245 The Missing Diplomat
-- Quests: 1246 The Missing Diplomat
-- Quests: 1447 The Missing Diplomat
-- Quests: 1247 The Missing Diplomat
-- Quests: 1248 The Missing Diplomat
-- Quests: 1249 The Missing Diplomat
-- Quests: 1250 The Missing Diplomat
-- Quests: 1264 The Missing Diplomat
-- Quests: 1265 The Missing Diplomat
-- Quests: 1266 The Missing Diplomat

-- NPC: 4982 Thomas
-- NPC: 4960 Bishop DeLavey
-- NPC: 4959 Jorgen
-- NPC: 482 Elling Trias
-- NPC: 840 Watcher Backus
-- NPC: 4961 Dashel Stonefist
-- NPC: 4963 Mikhail
-- NPC: 4962 Tapoke Slim Jahn
-- NPC: 4964 Commander Samaul
-- NPC: 4967 Archmage Tervosh
-- NPC: 4966 Private Hendel

-- TO-DO Private Hendel requires a script or SAI for quest 1266

-- Remove Disables
DELETE FROM `disables` WHERE `sourceType` = 1 and `entry` IN (
1274,1241,1242,1243,1244,1245,1246,1447,1247,1248,1249,1250,1264,1265,1266);

-- Quest starters and enders
DELETE FROM `creature_queststarter` WHERE `quest` IN (1274,1241,1242,1243,1244,1245,1246,1447,1247,1248,1249,1250,1264,1265,1266);
DELETE FROM `creature_questender` WHERE `quest` IN (1274,1241,1242,1243,1244,1245,1246,1447,1247,1248,1249,1250,1264,1265,1266);

INSERT INTO `creature_queststarter` VALUES
(4982, 1274,0),
(4960, 1241, 0),
(4959, 1242, 0),
(482, 1243, 0),
(840, 1244, 0),
(840, 1245, 0),
(482, 1246, 0),
(4961, 1447, 0),
(4961, 1247, 0),
(482, 1248, 0),
(4963, 1249, 0),
(4962, 1250, 0),
(4963, 1264, 0),
(4964, 1265, 0),
(4967, 1266, 0);

INSERT INTO `creature_questender` VALUES
(4960, 1274, 0),
(4959, 1241, 0),
(482, 1242, 0),
(840, 1243, 0),
(840, 1244, 0),
(482, 1245, 0),
(4961, 1246, 0),
(4961, 1447, 0),
(482, 1247, 0),
(4963, 1248, 0),
(4963, 1249, 0),
(4963, 1250, 0),
(4964, 1264, 0),
(4967, 1265, 0),
(4966, 1266, 0);

-- Quest Templates
UPDATE `quest_template` SET `ContentTuningID` = 73, `AllowableRaces` = 18446744073709551615, `Expansion` = 0 WHERE `ID` IN (
1274,1241,1242,1243,1244,1245,1246,1447,1247,1248,1249,1250,1264,1265,1266);

-- Creature Template Fixes
UPDATE `creature_template` SET npcflag = 2 WHERE `entry` = 840; -- Watcher Backus needs quest flag

-- Creature Difficulty Fixes
DELETE FROM `creature_template_difficulty` WHERE `Entry` IN (4961,4969,38867,4962,4971);
INSERT INTO `creature_template_difficulty` VALUES
(4962, 0, 0, 0, 15, 0, 1, 1, 1, 0.2, 4290, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, 44730),
(4971, 0, 0, 0, 15, 0, 1, 1, 1, 0.2, 4299, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, 47187),
(4961, 0, 0, 0, 864, 11, 1, 1, 1, 0.2, 4289, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, 53262),
(4969, 0, 0, 0, 864, 11, 1, 1, 1, 0.2, 4297, 0, 0, 0, 4969, 0, 0, 36, 52, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(38867, 0, 0, 0, 864, 11, 1, 1, 1, 0.2, 48904, 0, 0, 0, 38867, 0, 0, 36, 52, 524288, 0, 0, 0, 0, 0, 0, 0, -1);

-- SmartAI

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` = 4969;

DELETE FROM `smart_scripts` WHERE `EntryOrGuid` IN (4969,496900) AND `source_type` IN (0,9);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,
`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,
`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,
`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES
(4969, 0, 0, 1, 54, 0, 100, 0, 0, 0, 0, 0, 0, 42, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 'Old Town Thug - Just summoned - Set invinciblity'),
(4969, 0, 1, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 49, 0, 0, 0, 0, 0, 0, 21, 30, 0, 0, 0, 0, 0, 0, 0, 'Old Town Thug - Just summoned - Start attack'),
(4969, 0, 2, 0, 38, 0, 100, 1, 1, 1, 0, 0, 0, 80, 496900, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 'Old Town Thug - On data set - action list'),
(496900, 9, 0, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 27, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 'Old Town Thug - action list - combat stop'),
(496900, 9, 1, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 2, 35, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 'Old Town Thug - action list - combat stop'),
(496900, 9, 2, 0, 0, 0, 100, 0, 18000, 18000, 0, 0, 0, 41, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 'Old Town Thug - action list - despawn');

DELETE FROM `creature_loot_template` WHERE `Entry` IN (4969,38867);
INSERT INTO `creature_loot_template` VALUES
(4969, 0, 422, 4.9625, 0, 1, 0, 1, 1, 'Old Town Thung'),
(4969, 0, 929, 1.4981, 0, 1, 0, 1, 1, 'Old Town Thung'),
(4969, 0, 1205, 2.1848, 0, 1, 0, 1, 1, 'Old Town Thung'),
(4969, 0, 1478, 0.52, 0, 1, 0, 1, 1, 'Old Town Thung'),
(4969, 0, 1711, 0.8, 0, 1, 0, 1, 1, 'Old Town Thung'),
(4969, 0, 1712, 1.06, 0, 1, 0, 1, 1, 'Old Town Thung'),
(4969, 0, 2290, 0.37, 0, 1, 0, 1, 1, 'Old Town Thung'),
(4969, 0, 2589, 15.1998, 0, 1, 0, 1, 3, 'Old Town Thung'),
(4969, 0, 2592, 28.2147, 0, 1, 0, 1, 2, 'Old Town Thung'),
(4969, 0, 3385, 0.9988, 0, 1, 0, 1, 1, 'Old Town Thung'),
(4969, 0, 4306, 5.9925, 0, 1, 0, 1, 1, 'Old Town Thung'), 
(4969, 1, 3058, 0.68, 0, 1, 0, 1, 1, 'Old Town Thung'),
(4969, 1, 24059, 5, 0, 1, 1, 1, 1, 'Old Town Thung'),

(38867, 0, 422, 4.9625, 0, 1, 0, 1, 1, 'Old Town Thung'),
(38867, 0, 929, 1.4981, 0, 1, 0, 1, 1, 'Old Town Thung'),
(38867, 0, 1205, 2.1848, 0, 1, 0, 1, 1, 'Old Town Thung'),
(38867, 0, 1478, 0.52, 0, 1, 0, 1, 1, 'Old Town Thung'),
(38867, 0, 1711, 0.8, 0, 1, 0, 1, 1, 'Old Town Thung'),
(38867, 0, 1712, 1.06, 0, 1, 0, 1, 1, 'Old Town Thung'),
(38867, 0, 2290, 0.37, 0, 1, 0, 1, 1, 'Old Town Thung'),
(38867, 0, 2589, 15.1998, 0, 1, 0, 1, 3, 'Old Town Thung'),
(38867, 0, 2592, 28.2147, 0, 1, 0, 1, 2, 'Old Town Thung'),
(38867, 0, 3385, 0.9988, 0, 1, 0, 1, 1, 'Old Town Thung'),
(38867, 0, 4306, 5.9925, 0, 1, 0, 1, 1, 'Old Town Thung'), 
(38867, 1, 3058, 0.68, 0, 1, 0, 1, 1, 'Old Town Thung'),
(38867, 1, 24059, 5, 0, 1, 1, 1, 1, 'Old Town Thung');
