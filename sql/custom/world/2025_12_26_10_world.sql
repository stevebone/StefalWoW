-- Quest: 26213 Hot On the Trail: The Riverpaw Clan
-- Quest: 26214 Hot On the Trail: Murlocs
INSERT IGNORE INTO `creature_loot_template` VALUES 
('117', '0', '57755', '30', '1', '1', '0', '1', '1', 'Riverpaw Gnoll Clue'),
('500', '0', '57755', '30', '1', '1', '0', '1', '1', 'Riverpaw Gnoll Clue'),
('126', '0', '57756', '20', '1', '1', '0', '1', '1', 'Murloc Clue'),
('515', '0', '57756', '20', '1', '1', '0', '1', '1', 'Murloc Clue');

-- Quest: 26215 Meet Two-Shoed Lou
DELETE FROM `quest_template_addon` WHERE `ID` = 26215;
INSERT INTO `retail_world`.`quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`) VALUES 
('26215', '0', '0', '79229', '26213', '26228');

-- Quest: 26232 Lou's Parting Thoughts
-- NPC: 42562 Thug Trigger
-- NPC: 42387 Thug
UPDATE `creature_template` SET `ScriptName` = 'npc_lous_parting_thoughts_trigger' WHERE (`entry` = '42562');
UPDATE `creature_template` SET `ScriptName` = 'npc_lous_parting_thoughts_thug' WHERE (`entry` = '42387');
DELETE FROM `creature` WHERE `ID` = 42387;

-- Quest: 102 Patrolling Westfall
-- NPC: 821 Captain Danuvin

DELETE FROM disables where sourceType = 1 AND entry = 102;
DELETE FROM `creature_queststarter` WHERE `quest` IN (102);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('821', '102', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (102);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('821', '102', '0');

-- Quest: 22 Goretusk Liver Pie
-- NPC: 235 Salma Saldean
DELETE FROM disables where sourceType = 1 AND entry = 22;
DELETE FROM `creature_queststarter` WHERE `quest` IN (22);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('235', '22', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (22);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('235', '22', '0');

-- Quest: 26271 Feeding the hungry
-- NPC: 42384 Homeless Stormwind Citizen
-- NPC: 42386 Homeless Stormwind Citizen (Female)

UPDATE `creature_template` SET `ScriptName` = 'npc_hungry_hobo' WHERE (`entry` = '42384');
UPDATE `creature_template` SET `ScriptName` = 'npc_hungry_hobo' WHERE (`entry` = '42386');
UPDATE `creature_template` SET `npcflag` = '0' WHERE (`entry` = '42384');
UPDATE `creature_template` SET `npcflag` = '0' WHERE (`entry` = '42386');

-- Quest: 9 The killing fields
-- NPC: 233 Farmer Saldean
DELETE FROM disables where sourceType = 1 AND entry = 9;
DELETE FROM `creature_queststarter` WHERE `quest` IN (9);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('233', '9', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (9);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('233', '9', '0');

-- Quest: 12 The People's militia
-- NPC: 234 Gryan Stoutmantle
DELETE FROM disables where sourceType = 1 AND entry = 12;
DELETE FROM `creature_queststarter` WHERE `quest` IN (12);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('234', '12', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (12);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('234', '12', '0');

-- Quest: 13 The People's militia
-- NPC: 234 Gryan Stoutmantle
DELETE FROM disables where sourceType = 1 AND entry = 13;
DELETE FROM `creature_queststarter` WHERE `quest` IN (13);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('234', '13', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (13);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('234', '13', '0');

-- Quest: 14 The People's militia
-- NPC: 234 Gryan Stoutmantle
DELETE FROM disables where sourceType = 1 AND entry = 14;
DELETE FROM `creature_queststarter` WHERE `quest` IN (14);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('234', '14', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (14);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('234', '14', '0');

-- Quest: 109 Report to Gryan Stoutmantle
-- NPC: 233 Farmer Saldean
-- NPC: 234 Gryan Stoutmantle
DELETE FROM disables where sourceType = 1 AND entry = 109;
DELETE FROM `creature_queststarter` WHERE `quest` IN (109);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('233', '109', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (109);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('234', '109', '0');

UPDATE `quest_template_addon` SET `NextQuestID` = '14' WHERE (`ID` = '13');
UPDATE `quest_template_addon` SET `NextQuestID` = '0' WHERE (`ID` = '14');
INSERT IGNORE INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`) VALUES ('12', '0', '0', '0', '0', '13');