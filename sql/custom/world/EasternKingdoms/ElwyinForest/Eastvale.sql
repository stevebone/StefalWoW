-- Eastvale Logging Camp changes and updates

-- NPC: 43 Mine Spider
-- NPC: 118 Prowler
-- NPC: 294 Marshal Haggard
-- NPC: 313 Theocritus
-- NPC: 471 Mother Fang
-- NPC: 472 Fedfennel
-- NPC: 474 Rogue Wizzard
-- NPC: 476 Kobold Geomancer
-- NPC: 798 Solomon
-- NPC: 799 Kevin
-- NPC: 800 Kyle
-- NPC: 801 Eric
-- NPC: 802 Jay
-- NPC: 958 Dawn Brightstar
-- NPC: 959 Morley Eberlein
-- NPC: 1975 Eastvale Lumberjack
-- NPC: 4732 Randal Hunter
-- NPC: 5405 Pinto
-- NPC: 6093 Dead-Tooth Jack
-- NPC: 11328 Eastvale Peasant
-- NPC: 12375 Chestnut Mare
-- NPC: 12376 Brown Horse

-- Quest: 39 Deliver Thomas' Report
-- Quest: 94 A Watchful Eye
-- Quest: 1667 Dead-tooth Jack
-- Quest: 14079 Learn to Ride in Elwynn Forest

-- Reactivating old/deprecated quests in Eastvale
DELETE FROM `disables` WHERE `entry` = 39 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 94 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 1667 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE (`sourceType` = '1') and (`entry` = '14079');

-- Quest starters and enders
DELETE FROM `creature_queststarter` WHERE `quest` IN (39, 94, 1667);
DELETE FROM `creature_questender` WHERE `quest` IN (39, 1667);
DELETE FROM `gameobject_questender` WHERE `quest` IN (94);

INSERT IGNORE INTO `creature_queststarter` VALUES
(261, 39, 0),
(313, 94, 0),
(294, 1667, 0);

INSERT IGNORE INTO `creature_questender` VALUES
(240, 39, 0),
(294, 1667, 0);

INSERT IGNORE INTO `gameobject_questender` VALUES
(31, 94, 0);

-- Quest Templates
UPDATE `quest_template` SET `ContentTuningID` = '73', `QuestSortID` = '12', `RewardFactionID1` = '72', `RewardFactionValue1` = '1', `RewardFactionFlags` = '1', `AllowableRaces` = '18446744073709551615', `Expansion` = '0' WHERE (`ID` = '1667');
UPDATE `quest_template_addon` SET `AllowableClasses` = '0' WHERE (`ID` = '1667');
UPDATE `quest_template` SET `ContentTuningID` = '73', `RewardNextQuest` = '0', `Expansion` = '0' WHERE (`ID` = '39');
UPDATE `quest_template` SET `RewardFactionID1` = '72', `Expansion` = '0', `AllowableRaces` = 18446744073709551615, `QuestSortID` = '12' WHERE (`ID` = '14079');
UPDATE `quest_template` SET `ContentTuningID` = '73', `Expansion` = '0', `AllowableRaces` = '18446744073709551615' WHERE (`ID` = '94');

-- Creature Templates
UPDATE `creature_template` SET `faction` = '12' WHERE `entry` IN (313, 798, 799, 800, 801, 802, 958, 959); -- incorrect faction
UPDATE `creature_template` SET `faction` = 35 WHERE `entry` IN (5405, 12375, 12376); -- incorrect faction
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_eastvale_lumberjack' WHERE `entry` = 1975;
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_eastvale_lumberjack' WHERE `entry` = 11328;
UPDATE `creature_template` SET `npcflag` = 2 WHERE `entry` IN (4732);
UPDATE `creature_template` SET `AIName` = '' WHERE `entry` IN (313, 4732);

-- Creature Texts
DELETE FROM `creature_text` WHERE `CreatureID` IN (472, 474, 476);
INSERT INTO `creature_text` VALUES
('472', '0', '0', 'Grrrr... fresh meat!', '12', '0', '100', '0', '0', '0', '0', '1868', '0', 'combat Say'),
('472', '0', '1', 'More bones to gnaw on...', '12', '0', '100', '0', '0', '0', '0', '0', '0', 'combat Say');

INSERT INTO `creature_text` VALUES
('474', '0', '0', 'You\'re in the wrong place at the wrong time, $g pal:missy;!', '12', '0', '100', '0', '0', '0', '0', '42883', '0', 'combat Say'),
('474', '0', '1', 'Fate has brought you to me!', '12', '0', '100', '0', '0', '0', '0', '0', '0', 'combat Say'),
('474', '0', '2', 'Your money or your life!', '12', '0', '100', '0', '0', '0', '0', '0', '0', 'combat Say');

INSERT INTO `creature_text` VALUES
('476', '0', '0', 'You no take candle!', '12', '0', '100', '0', '0', '0', '0', '1868', '0', 'combat Say'),
('476', '0', '1', 'Yiieeeee! Me run!', '12', '0', '100', '0', '0', '0', '0', '0', '0', 'combat Say');

-- Scripts Cleanup
DELETE FROM `smart_scripts` WHERE  `entryorguid` IN (43, 118, 294, 471, 474, 476, 798, 799, 800, 801, 802, 958, 959, 1975, 5405, 6093, 11328, 12375, 12376) AND `id` IN (10000, 10001, 11000, 11001, 11002, 11003, 11004);
DELETE FROM `smart_scripts` WHERE  `entryorguid` IN (313, 472, 4732) AND `id` IN (10000, 10001, 11000, 11001, 11002, 11003, 11004);

DELETE FROM `smart_scripts` WHERE `entryorguid` = 472 AND id = 2;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
(472,0,2,0,'',4,0,50,0,0,0,0,0,0,'',1,0,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Fedfennel - On Aggro - Say Line 0 (No Repeat)');