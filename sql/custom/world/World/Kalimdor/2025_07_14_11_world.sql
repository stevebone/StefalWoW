-- Quest: 456 The Balance of Nature
-- Quest: 457 Nature's Due
-- NPC: 2079 Ilthalaine

DELETE FROM `disables` WHERE `entry` = 456 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 457 AND `sourceType` = 1; -- quest source

UPDATE `quest_template` SET `ContentTuningID` = '1478', `RewardBonusMoney` = '120', `Expansion` = '0' WHERE (`ID` = '456');
UPDATE `quest_template` SET `ContentTuningID` = '1478', `RewardBonusMoney` = '120', `Expansion` = '0' WHERE (`ID` = '457');

DELETE FROM `creature_questender` WHERE `quest` = 456;
DELETE FROM `creature_questender` WHERE `quest` = 457;

INSERT IGNORE INTO `creature_questender` VALUES
(2079, 456, 0),
(2079, 457, 0);

DELETE FROM `creature_queststarter` WHERE `quest` = 456;
DELETE FROM `creature_queststarter` WHERE `quest` = 457;

INSERT IGNORE INTO `creature_queststarter` VALUES
(2079, 456, 0),
(2079, 457, 0);
