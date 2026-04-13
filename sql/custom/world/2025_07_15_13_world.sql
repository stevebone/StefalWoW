-- Quest: 940 Teldrassil
-- Quest: 952 Grove of the Ancients
-- NPC: 3516 Arch Druid Fandral Staghelm
-- NPC: 3519 Sentinel Arynia Cloudsbreak
-- NPC: 33072 Onu


DELETE FROM `disables` WHERE `entry` = 940 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 952 AND `sourceType` = 1; -- quest source

DELETE FROM `creature_questender` WHERE `quest` = 940;
DELETE FROM `creature_questender` WHERE `quest` = 952;
INSERT IGNORE INTO `creature_questender` VALUES
(3516, 940, 0),
(33072, 952, 0);

DELETE FROM `creature_queststarter` WHERE `quest` = 940;
DELETE FROM `creature_queststarter` WHERE `quest` = 952;
INSERT IGNORE INTO `creature_queststarter` VALUES
(3516, 952, 0),
(3519, 940, 0);

UPDATE `quest_template` SET `ContentTuningID` = '72', `RewardBonusMoney` = 60, `Expansion` = '0' WHERE (`ID` = '940');
UPDATE `quest_template` SET `ContentTuningID` = '72', `Expansion` = '0' WHERE (`ID` = '952');