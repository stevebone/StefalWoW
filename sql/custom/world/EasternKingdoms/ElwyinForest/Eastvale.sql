-- Eastvale Logging Camp changes and updates

-- NPC: 43 Mine Spider
-- NPC: 294 Marshal Haggard
-- NPC: 471 Mother Fang
-- NPC: 474 Rogue Wizzard
-- NPC: 476 Kobold Geomancer
-- NPC: 798 Solomon
-- NPC: 799 Kevin
-- NPC: 800 Kyle
-- NPC: 801 Eric
-- NPC: 802 Jay
-- NPC: 6093 Dead-Tooth Jack

-- Quest: 39 Deliver Thomas' Report
-- Quest: 1667 Dead-tooth Jack

-- Reactivating old/deprecated quests in Eastvale
DELETE FROM `disables` WHERE `entry` = 39 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 1667 AND `sourceType` = 1; -- quest source

-- Quest starters and enders
DELETE FROM `creature_queststarter` WHERE `quest` = 1667;
DELETE FROM `creature_queststarter` WHERE `quest` = 39;
DELETE FROM `creature_questender` WHERE `quest` = 1667;
DELETE FROM `creature_questender` WHERE `quest` = 39;

INSERT IGNORE INTO `creature_queststarter` VALUES
(261, 39, 0),
(294, 1667, 0);

INSERT IGNORE INTO `creature_questender` VALUES
(240, 39, 0),
(294, 1667, 0);

-- Quest Templates
UPDATE `quest_template` SET `ContentTuningID` = '73', `QuestSortID` = '12', `RewardFactionID1` = '72', `RewardFactionValue1` = '1', `RewardFactionFlags` = '1', `AllowableRaces` = '18446744073709551615', `Expansion` = '0' WHERE (`ID` = '1667');
UPDATE `quest_template_addon` SET `AllowableClasses` = '0' WHERE (`ID` = '1667');
UPDATE `quest_template` SET `ContentTuningID` = '73', `RewardNextQuest` = '0', `Expansion` = '0' WHERE (`ID` = '39');

-- Creature Templates
UPDATE `creature_template` SET `faction` = '12' WHERE `entry` IN (798, 799, 800, 801, 802); -- incorrect faction

-- Creature Texts
DELETE FROM `creature_text` WHERE `CreatureID` IN (474, 476);
INSERT INTO `creature_text` VALUES
('474', '0', '0', 'You\'re in the wrong place at the wrong time, $g pal:missy;!', '12', '0', '100', '0', '0', '0', '0', '42883', '0', 'combat Say'),
('474', '0', '1', 'Fate has brought you to me!', '12', '0', '100', '0', '0', '0', '0', '0', '0', 'combat Say'),
('474', '0', '2', 'Your money or your life!', '12', '0', '100', '0', '0', '0', '0', '0', '0', 'combat Say');

INSERT INTO `creature_text` VALUES
('476', '0', '0', 'You no take candle!', '12', '0', '100', '0', '0', '0', '0', '1868', '0', 'combat Say'),
('476', '0', '1', 'Yiieeeee! Me run!', '12', '0', '100', '0', '0', '0', '0', '0', '0', 'combat Say');

-- Scripts Cleanup
DELETE FROM `smart_scripts` WHERE  `entryorguid` IN (43, 294, 471, 474, 476, 798, 799, 800, 801, 802, 6093) AND `id` IN (10000, 10001, 11000, 11001, 11002, 11003, 11004);