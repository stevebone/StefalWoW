-- NPC: 55942 Ji at the Hut
-- NPC: 55943 Wei Palerage
-- NPC: 55944 Delora Lionheart
-- NPC: 60042 Korga
-- NPC: 60055 Makael Bay
-- NPC: 67186 Provisioner Drog
--

-- Quest: 29793 Evil from the seas
-- Quest: 29795 Stocking Stalks
-- Quest: 29796 Urgent News
-- Quest: 30589 Wrecking the Wreck
-- Quest: 30591 Praying on the predators

DELETE FROM `creature_queststarter` WHERE `quest` IN (30591, 29795, 30589, 29793, 29796);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES 
('55942', '29793', '0'),
('55942', '29796', '0'),
('55943', '30591', '0'),
('60042', '29795', '0'),
('60042', '30589', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (30591, 29795, 30589, 29793, 29796);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES 
('55942', '29793', '0'),
('55944', '29796', '0'),
('55943', '30591', '0'),
('60042', '29795', '0'),
('60055', '30589', '0');

DELETE FROM `quest_template_addon` WHERE `ID` IN (30591, 29795, 30589, 29793, 29796);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('30591', '0', '0', '0', '29792', '30589', '-30591', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29795', '0', '0', '0', '29792', '30589', '-30591', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('30589', '0', '0', '0', '30591', '30590', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29793', '0', '0', '0', '30589', '29796', '-29793', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29796', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

-- Creature Templates
UPDATE `creature_template` SET `ScriptName` = 'npc_korga_hut' WHERE `entry` = 60042;

DELETE FROM `creature_template_addon` WHERE `entry` IN (55943, 60042, 67186, 55942, 55944, 60055);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureId`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('55943', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '110846'),
('60042', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('67186', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('55942', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('55944', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80797'),
('60055', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '');

UPDATE `creature` SET `PhaseId` = '903' WHERE (`guid` = '451895');
UPDATE `creature` SET `PhaseId` = '1835' WHERE (`guid` = '451988');
UPDATE `creature` SET `PhaseId` = '0', `PhaseGroup` = '641' WHERE (`guid` = '451723');

DELETE FROM `npc_vendor` WHERE `entry` IN (67186);
INSERT INTO `npc_vendor` (entry, slot, item, maxcount, incrtime, ExtendedCost, `type`, BonusListIDs, PlayerConditionID, IgnoreFiltering, VerifiedBuild) VALUES
('67186', '1', '117', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '7', '159', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '4', '787', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '2', '2287', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '3', '3770', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '5', '4592', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '6', '4593', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '8', '90659', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '9', '90660', '0', '0', '0', '1', NULL, '0', '0', '64154');

-- Area Trigger Scripts
DELETE FROM `areatrigger_scripts` WHERE `entry` IN (7714);
INSERT INTO `areatrigger_scripts` VALUES
(7714, 'at_forlorn_hut_7714');