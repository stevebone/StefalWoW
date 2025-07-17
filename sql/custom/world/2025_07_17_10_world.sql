-- Quest: 1666 Marshal Haggard
-- Quest: 1667 Dead-tooth Jack
-- Quest: 39 Deliver Thomas' Report
-- NPC: 6089 Harry Burlguard
-- NPC: 294 Marshal Haggard
-- NPC: 6093 Dead-tooth Jack
-- NPC: 261 Guard Thomas
-- NPC: 240 Marshal Dughan

DELETE FROM `disables` WHERE `entry` = 1666 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 1667 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 39 AND `sourceType` = 1; -- quest source

DELETE FROM `creature_questender` WHERE `quest` = 1666;
DELETE FROM `creature_questender` WHERE `quest` = 1667;
DELETE FROM `creature_questender` WHERE `quest` = 39;
INSERT IGNORE INTO `creature_questender` VALUES
(240, 39, 0),
(294, 1667, 0),
(294, 1666, 0);

DELETE FROM `creature_queststarter` WHERE `quest` = 1666;
DELETE FROM `creature_queststarter` WHERE `quest` = 1667;
DELETE FROM `creature_queststarter` WHERE `quest` = 39;
INSERT IGNORE INTO `creature_queststarter` VALUES
(261, 39, 0),
(294, 1667, 0),
(6089, 1666, 0);

UPDATE `quest_template` SET `ContentTuningID` = '73', `QuestSortID` = '12', `AllowableRaces` = '6130900294268439629', `Expansion` = '0', `RewardFactionID1` = '72', `RewardFactionValue1` = '1', `RewardFactionFlags` = '1' WHERE (`ID` = '1666');
UPDATE `quest_template_addon` SET `AllowableClasses` = '0', `PrevQuestID` = '0' WHERE (`ID` = '1666');

UPDATE `quest_template` SET `ContentTuningID` = '73', `QuestSortID` = '12', `RewardFactionID1` = '72', `RewardFactionValue1` = '1', `RewardFactionFlags` = '1', `AllowableRaces` = '6130900294268439629', `Expansion` = '0' WHERE (`ID` = '1667');
UPDATE `quest_template_addon` SET `AllowableClasses` = '0' WHERE (`ID` = '1667');

UPDATE `quest_template` SET `ContentTuningID` = '73', `RewardNextQuest` = '0', `Expansion` = '0' WHERE (`ID` = '39');
