-- Quest: 153 Red Leather Bandanas
-- NPC: 878 Scout Galiaan

UPDATE `quest_template` SET `QuestPackageID` = '2640', `ContentTuningID` = '6' WHERE (`ID` = '153');
DELETE FROM disables where sourceType = 1 AND entry = 153;

DELETE FROM `creature_queststarter` WHERE `quest` IN (153);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('878', '153', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (153);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('878', '153', '0');

-- Quest: 103 Keeper of the flame
-- NPC: 392 Captain Grayson

UPDATE `quest_template` SET `QuestPackageID` = '2687', `ContentTuningID` = '6' WHERE (`ID` = '103');
DELETE FROM disables where sourceType = 1 AND entry = 103;

DELETE FROM `creature_queststarter` WHERE `quest` IN (103);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('392', '103', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (103);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('392', '103', '0');

-- Quest: 399 Humble Beginnings
-- NPC: 1646 Baros Alexston

UPDATE `quest_template` SET `QuestPackageID` = '2687', `ContentTuningID` = '6' WHERE (`ID` = '399');
DELETE FROM disables where sourceType = 1 AND entry = 399;

DELETE FROM `creature_queststarter` WHERE `quest` IN (399);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('1646', '399', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (399);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('1646', '399', '0');

-- Quest: 64 Forgotten Heirloom
-- NPC: 233 Farmer Saldean

UPDATE `quest_template` SET `QuestPackageID` = '2640', `ContentTuningID` = '6', `LogDescription` = 'Farmer Saldean wants you to retrieve his cousin pocket watch from the wardrobe in his farmhouse at the pumpkin farm to the West.', `QuestDescription` = 'It was horrible!  Now that my cousin is dead I have nothing to remember him by.  Except his watch that was taken by hooligans.  It was forgotten behind when my cousin left in a rush and he forgot to pack his pocket watch.  Verna\'s pa gave him that watch on their wedding day and I feel just sick knowing those thieves have it.  My cousin left the pocket watch in the wardrobe at the farmhouse.  Look for the field of pumpkins to the West -- you can\'t miss it.  If you bring it back to me, I\'d sure be grateful!' WHERE (`ID` = '64');
DELETE FROM disables where sourceType = 1 AND entry = 64;

DELETE FROM `creature_queststarter` WHERE `quest` IN (64);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('233', '64', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (64);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('233', '64', '0');