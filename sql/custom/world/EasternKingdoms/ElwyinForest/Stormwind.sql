-- Stormwind changes and updates

-- NPC: 6089 Harry Burlguard
-- NPC: 6122 Gakin
-- NPC: 49540 Stormwind Rat

-- Quest: 1666 Marshal Haggard
-- Quest: 1688 Surena Caledon

-- Reactivating old/deprecated quests in Stormwind
DELETE FROM `disables` WHERE `entry` = 1666 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 1688 AND `sourceType` = 1; -- quest source

-- Quest starters and enders
DELETE FROM `creature_queststarter` WHERE `quest` = 1666;
DELETE FROM `creature_questender` WHERE `quest` = 1666;
DELETE FROM `creature_queststarter` WHERE `quest` = 1688;
DELETE FROM `creature_questender` WHERE `quest` = 1688;

INSERT IGNORE INTO `creature_queststarter` VALUES
(6089, 1666, 0),
(6122, 1688, 0);

INSERT IGNORE INTO `creature_questender` VALUES
(294, 1666, 0),
(6122, 1688, 0);

-- Quest Templates
UPDATE `quest_template` SET `ContentTuningID` = '73', `QuestSortID` = '12', `AllowableRaces` = '18446744073709551615', `Expansion` = '0', `RewardFactionID1` = '72', `RewardFactionValue1` = '1', `RewardFactionFlags` = '1' WHERE (`ID` = '1666');
UPDATE `quest_template_addon` SET `AllowableClasses` = '0', `PrevQuestID` = '0' WHERE (`ID` = '1666');
UPDATE `quest_template` SET `ContentTuningID` = '73', `QuestSortID` = '12', `AllowableRaces` = '18446744073709551615', `Expansion` = '0', `RewardFactionID1` = '72', `RewardFactionValue1` = '1', `RewardFactionFlags` = '1' WHERE (`ID` = '1688');
UPDATE `quest_template_addon` SET `AllowableClasses` = '0' WHERE (`ID` = '1688');

-- Creature Templates
UPDATE `creature_template` SET `AIName` = '' WHERE `Entry` IN (6089, 6122); -- remove AI name
UPDATE `creature_template` SET `faction` = '31' WHERE (`entry` = '49540'); -- fix Stormwind Rat


-- Scripts
DELETE FROM `smart_scripts` WHERE  `entryorguid` IN (6089, 6122) AND `id` IN (10000, 10001, 11000, 11001, 11002, 11003, 11004);