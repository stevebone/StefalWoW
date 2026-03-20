-- Eastvale Logging Camp changes and updates

-- NPC: 294 Marshal Haggard
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

-- Scripts Cleanup
DELETE FROM `smart_scripts` WHERE  `entryorguid` IN (294, 6093) AND `id` IN (10000, 10001, 11000, 11001, 11002, 11003, 11004);