-- Stormwind changes and updates

-- NPC: 332 Master Mathias Shaw
-- NPC: 5566 Tannysa
-- NPC: 6089 Harry Burlguard
-- NPC: 6122 Gakin
-- NPC: 49540 Stormwind Rat

-- Quest: 1666 Marshal Haggard
-- Quest: 1688 Surena Caledon
-- Quest: 1861 Mirror Lake
-- Quest: 2206 Snatch and Grab

-- Reactivating old/deprecated quests in Stormwind
DELETE FROM `disables` WHERE `entry` = 1666 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 1688 AND `sourceType` = 1; -- quest source
DELETE FROM disables where sourceType = 1 AND entry = 1861;
DELETE FROM disables where sourceType = 1 AND entry = 2206;

-- Quest starters and enders
DELETE FROM `creature_queststarter` WHERE `quest` IN (1666, 1688, 1861, 2206);
DELETE FROM `creature_questender` WHERE `quest` IN (1666, 1688, 1861, 2206);

INSERT IGNORE INTO `creature_queststarter` VALUES
(332, 2206, 0),
(6089, 1666, 0),
(6122, 1688, 0),
(5497, 1861, 0);

INSERT IGNORE INTO `creature_questender` VALUES
(332, 2206, 0),
(294, 1666, 0),
(6122, 1688, 0),
(5497, 1861, 0);

-- Quest Templates
UPDATE `quest_template` SET `ContentTuningID` = '73', `QuestSortID` = '12', `AllowableRaces` = '18446744073709551615', `Expansion` = '0', `RewardFactionID1` = '72', `RewardFactionValue1` = '1', `RewardFactionFlags` = '1' WHERE (`ID` = '1666');
UPDATE `quest_template_addon` SET `AllowableClasses` = '0', `PrevQuestID` = '0' WHERE (`ID` = '1666');
UPDATE `quest_template` SET `ContentTuningID` = '73', `QuestSortID` = '12', `AllowableRaces` = '18446744073709551615', `Expansion` = '0', `RewardFactionID1` = '72', `RewardFactionValue1` = '1', `RewardFactionFlags` = '1' WHERE (`ID` = '1688');
UPDATE `quest_template` SET `ContentTuningID` = '73', `QuestSortID` = '12', `AllowableRaces` = '18446744073709551615', `Expansion` = '0' WHERE `ID` IN (1861, 2206);
UPDATE `quest_template_addon` SET `AllowableClasses` = '0' WHERE `ID` IN (1688, 1861, 2206);

-- Creature Templates
UPDATE `creature_template` SET `AIName` = '' WHERE `Entry` IN (6089, 6122); -- remove AI name
UPDATE `creature_template` SET `faction` = '31' WHERE (`entry` = '49540'); -- fix Stormwind Rat
UPDATE `creature_template` SET `faction` = '12' WHERE `entry` IN (332); -- incorrect faction

-- Scripts
DELETE FROM `smart_scripts` WHERE  `entryorguid` IN (332, 6089, 6122) AND `id` IN (10000, 10001, 11000, 11001, 11002, 11003, 11004);

-- Gameobjects
DELETE FROM `gameobject` WHERE `id` IN (102414);
SET @OGUID := 900000;
INSERT INTO `gameobject` VALUES
(@OGUID+0, '102414', '0', '12', '92', '0', '0', '0', '0', '-1', '-9358.1', '559.318', '61.5671', '3.06084', '-0', '-0', '-0.999185', '-0.0403646', '300', '255', '1', '', NULL, '0');
