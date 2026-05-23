-- Quest: 4161 Recipe of the Keldorei
-- NPC: 6286 Zarrin
-- NPC: 1998 Webwood Lurker
-- Item: 5465 Small Spider Leg

DELETE FROM `disables` WHERE `entry` = 4161 AND `sourceType` = 1; -- quest source

DELETE FROM `creature_questender` WHERE `quest` = 4161;
INSERT IGNORE INTO `creature_questender` VALUES
(6286, 4161, 0);

DELETE FROM `creature_queststarter` WHERE `quest` = 4161;
INSERT IGNORE INTO `creature_queststarter` VALUES
(6286, 4161, 0);

UPDATE `quest_template` SET `ContentTuningID` = '72', `Expansion` = '0' WHERE (`ID` = '4161');
