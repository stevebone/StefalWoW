-- NPC: 53705 Pangolin
-- NPC: 55946 Pei-wu tiger
-- NPC: 56172 Ash moth
-- NPC: 56174 Tiger cub

-- NPC: 54786 Shang 1/2 temple
-- NPC: 55944 Delora Lionheart
-- NPC: 56013 Spirit of Master Shang
-- NPC: 57720 Ji temple
-- NPC: 57721 Aysa temple
-- NPC: 60917 Huo temple
-- NPC: 60918 Shu temple
-- NPC: 60919 Wugou temple
-- NPC: 60920 Dafeng temple
-- NPC: 64593 Korga

-- Quest: 29792 Bidden to Greatness

DELETE FROM `creature_queststarter` WHERE `quest` IN (29792);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES 
('56012', '29792', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (29792);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES 
('55943', '29792', '0');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29792);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29792', '0', '0', '0', '29791', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

-- Creature Templates
DELETE FROM `creature_template_addon` WHERE `entry` IN (55944, 57720, 57721, 64593, 56013, 60919, 60918, 60920, 60917, 54786);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureId`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('55944', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80797'),
('57720', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80797'),
('57721', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80797'),
('64593', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80797'),
('56013', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('60919', '0', '0', '0', '3', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('60918', '6091800', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('60920', '0', '0', '0', '0', '3', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('60917', '0', '0', '0', '0', '3', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('54786', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '108900 49414 126160'); -- for Shang 1


-- Creature
DELETE FROM `creature_addon` WHERE `guid` IN (451410);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('451410', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '108900 49415 126160'); -- Shang 2

UPDATE `creature` SET `PhaseId` = 1027 WHERE `guid` IN (451408); -- Huo
UPDATE `creature` SET `PhaseId` = 1028, `MovementType` = 2 WHERE `guid` IN (451409); -- Shu
UPDATE `creature` SET `PhaseId` = 1029 WHERE `guid` IN (451411); -- Wugou
UPDATE `creature` SET `PhaseId` = 1030 WHERE `guid` IN (451828); -- Dafeng

