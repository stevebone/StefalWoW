-- NPC: 54786 Shang 1/2 temple
-- NPC: 55944 Delora Lionheart
-- NPC: 56013 Spirit of Master Shang
-- NPC: 57720 Ji temple faction choice
-- NPC: 60570 Ji final act
-- NPC: 57721 Aysa temple faction choice
-- NPC: 60566 Aysa final act
-- NPC: 60917 Huo temple
-- NPC: 60918 Shu temple
-- NPC: 60919 Wugou temple
-- NPC: 60920 Dafeng temple
-- NPC: 64593 Korga

-- Quest: 29792 Bidden to Greatness
-- Quest: 30987 Joining the Alliance
-- Quest: 31012 Joining the Horde
-- Quest: 31013 The Horde way
-- Quest: 31450 A new fate

DELETE FROM `creature_queststarter` WHERE `quest` IN (29792, 31450, 30987, 31012, 31013);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES 
(56012, 29792, 0),
(56013, 31450, 0),
(60570, 31012, 0),
(60566, 30987, 0),
(39605, 31013, 0); -- Garrosh Hellscream

DELETE FROM `creature_questender` WHERE `quest` IN (29792, 31450, 30987, 31012, 31013);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES 
('55943', '29792', '0'),
(60570, 31450, 0),
(60566, 31450, 0),
(39605, 31012, 0), -- Garrosh Hellscream
(39605, 31013, 0), -- Garrosh Hellscream
(62092, 31013, 0), -- Garrosh Hellscream
(29611, 30987, 0), -- King Varian Wrynn
(107574, 30987, 0); -- Anduin Wrynn

DELETE FROM `quest_template_addon` WHERE `ID` IN (29792, 31450, 30987, 31012, 31013);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29792', '0', '0', '0', '29791', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('31450', '0', '0', '0', '29800', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('30987', '0', '0', '0', '31450', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('31012', '0', '0', '0', '31450', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('31013', '0', '0', '0', '31012', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

-- Creature Templates
UPDATE creature_template SET ScriptName = 'npc_spirit_of_master_shang_xi_q31450' WHERE entry = 56013;

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

DELETE FROM `creature_template_gossip` WHERE `CreatureID` IN (56013, 57720, 57721);
INSERT INTO `creature_template_gossip` (`CreatureID`, `MenuID`, `VerifiedBuild`) VALUES
('56013', '13726', '47936'),
('57720', '14670', '47936'),
('57721', '14671', '47936');

DELETE FROM `gossip_menu` WHERE `MenuID` IN (13726);
INSERT INTO `gossip_menu` VALUES 
(13726, 19723, 61967);

DELETE FROM `gossip_menu_option` WHERE `MenuID` IN (13726);
INSERT INTO `gossip_menu_option` VALUES
('13726', '36586', '0', '0', 'I\'m ready to decide.', '60279', '0', '0', '0', '0', NULL, '0', '0', NULL, '0', NULL, NULL, '61967');



-- Creature
DELETE FROM `creature_addon` WHERE `guid` IN (451410);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('451410', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '108900 49415 126160'); -- Shang 2

UPDATE `creature` SET `PhaseId` = 1027 WHERE `guid` IN (451408); -- Huo
UPDATE `creature` SET `PhaseId` = 1028, `MovementType` = 2 WHERE `guid` IN (451409); -- Shu
UPDATE `creature` SET `PhaseId` = 1029 WHERE `guid` IN (451411); -- Wugou
UPDATE `creature` SET `PhaseId` = 1030 WHERE `guid` IN (451828); -- Dafeng

DELETE FROM `spell_script_names` WHERE `spell_id` IN (108897, 113244, 113245, 130422, 132211);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES 
(108897, 'spell_pandaren_faction_choice'),
(113244, 'spell_faction_choice_trigger'),
(113245, 'spell_faction_choice_trigger'),
(130422, 'spell_balloon_exit_timer'),
(132211, 'spell_balloon_exit_timer');

DELETE FROM `spell_target_position` WHERE `ID` IN (116957);
INSERT INTO `spell_target_position` VALUES
('116957', '0', 0, '0', '-9128.09', '388.54', '91.163', '0', 0),
('116957', '1', 0, '0', '-9128.09', '388.54', '91.163', '0', 0);
