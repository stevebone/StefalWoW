-- Quest: 916 Webwood Venom
-- Quest: 917 Webwood Egg
-- Quest: 920 Tenaron's Summons
-- NPC: 2082 Gilshalan Windwalker
-- NPC: 3514 Tenaron Stormgrip
-- Mob: 1986 Webwood Spider
-- Item: 5166 Webwood Venom Sac

DELETE FROM `disables` WHERE `entry` = 920 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 917 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 916 AND `sourceType` = 1; -- quest source

INSERT IGNORE INTO `creature_queststarter` VALUES
(2082, 916, 0),
(2082, 917, 0),
(2082, 920, 0);

INSERT IGNORE INTO `creature_questender` VALUES
(2082, 916, 0),
(2082, 917, 0),
(3514, 920, 0);

UPDATE `quest_template` SET `LogTitle` = 'Webwood Venom', `LogDescription` = 'Acquire 10 Webwood Venom Sacs', `QuestType` = '2', `Flags` = '524288', `FlagsEx2` = '268435456', `Expansion` = '0'  WHERE (`ID` = '916');
UPDATE `quest_objectives` SET `Type` = '1', `ObjectID` = '5166', `Amount` = '10', `Flags2` = '1' WHERE (`ID` = '254408');

UPDATE `quest_template` SET `ContentTuningID` = '1478' WHERE (`ID` = '916');
UPDATE `quest_template` SET `ContentTuningID` = '1478' WHERE (`ID` = '917');
UPDATE `quest_template` SET `ContentTuningID` = '1478' WHERE (`ID` = '920');

DELETE FROM `creature_loot_template` WHERE `Item` = 5166 AND `Entry` = 1986;
INSERT INTO `creature_loot_template` VALUES
(1986, 0, 5166, 75, 1, 1, 0, 1, 2, 'Webwood Venom Sac');