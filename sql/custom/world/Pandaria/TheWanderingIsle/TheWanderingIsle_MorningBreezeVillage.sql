-- Quest: 29777 Tools of the enemy
-- Quest: 29778 Rewritten wisdoms
-- Quest: 29783 Stronger than stone
-- Quest: 29779 The direct solution
-- Quest: 29780 Do no evil
-- Quest: 29781 Monkey Advisory warning
-- Quest: 29784 Balanced perspective
-- Quest: 29785 Dafeng-the-spirit-of-air
-- Quest: 29786 Battle for the skies
-- Quest: 29787 Worthy Of Passing

-- NPC: 57623 Shen Stonecarver
-- NPC: 55588 Elder Shaopai at Morning Breeze
-- NPC: 55583 Ji Firepaw at Morning Breeze
-- NPC: 55585 Jojo at Morning Breeze
-- NPC: 57670 Jojo spawn at Morning Breeze
-- NPC: 55601 Fe-feng Wiseman
-- NPC: 55632 Fe-Feng Ruffian
-- NPC: 57465 Fe-Feng Ruffian
-- NPC: 57466 Fe-Feng Firethief
-- NPC: 55633 Fe-Feng Firethief
-- NPC: 55634 Ruk-Ruk
-- NPC: 64322 Ruk-Ruk's rocket
-- NPC: 65558 Ji Firepaw (with player)
-- NPC: 65559 Vision of Zhao-Ren
-- NPC: 55650 Shang Xi's Hot Air Balloon
-- NPC: 55592 Dafeng Spirit of Air
-- NPC: 65560 Dafeng spawn ??
-- NPC: 64532 Dafeng spawn (quest accept)
-- NPC: 55595 Aysa with Dafeng
-- NPC: 64543 Aysa spawn  (quest accept)
-- NPC: 55786 Zhaoren
-- NPC: 64507 Firework Launcher
-- NPC: 64505 Ji at Zhaoren
-- NPC: 64506 Aysha at Zhaoren
-- NPC: 55874 Dead Zhaoren
-- NPC: 55586 Master Shang Xi - Zhaoren dead phase
-- NPC: 56159 Master Shang (spawned for quest 29787)
-- NPC: 55672 Master Shang for the forest quests
-- NPC: 56274 Guardian of the Elders

DELETE FROM `creature_queststarter` WHERE `quest` IN (29777,29778,29783,29779,29780,29781,29785,29786,29787);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES 
('55588', '29777', '0'),
('55583', '29778', '0'),
('55585', '29783', '0'),
('55583', '29779', '0'),
('55583', '29780', '0'),
('55583', '29781', '0'),
('55595', '29785', '0'),
('55595', '29786', '0'),
('55586', '29787', '0');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29777,29778,29783);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29777', '0', '0', '0', '29776', '0', '-29778', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29778', '0', '0', '0', '29776', '0', '-29778', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29783', '0', '0', '0', '29776', '0', '-29778', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29779,29780,29781);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29779', '0', '0', '0', '29778', '29784', '-29779', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29780', '0', '0', '0', '29778', '29784', '-29779', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29781', '0', '0', '0', '29778', '29784', '-29779', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29784,29785);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29784', '0', '0', '116992', '29780', '29785', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29785', '0', '0', '0', '29784', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'quest_29785_dafeng_the_spririt_of_air');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29786);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29786', '0', '0', '0', '29785', '29787', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'quest_29786_battle_for_the_skies');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29787);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29787', '0', '0', '105333', '29786', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

-- Scripts for Jojo and monkeys
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55601,57670,65467,56394,56393);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55601,57670,5767000,65467,56394,56393,6546700,5639400,5639300) AND `source_type` IN (0,9);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('55601', '0', '0', '0', '0', '0', '100', '0', '3000', '6000', '18000', '22000', '0', '', '11', '128751', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
('55601', '0', '1', '0', '0', '0', '100', '0', '2000', '4000', '8000', '12000', '0', '', '11', '128413', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
('57670', '0', '0', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5767000', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Just Spawned - Run Script'),
('5767000', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Talk'),
('5767000', '9', '1', '0', '0', '0', '100', '0', '2500', '2500', '1000', '1000', '0', '', '128', '1', '1', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Set AI AnimKit'),
('5767000', '9', '2', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '69', '1', '0', '0', '0', '0', '0', '8', '0', '0', '0', '1077.31', '4179.94', '205.774', '0', 'Jojo Ironbrow - Move to Pos'),
('5767000', '9', '3', '0', '0', '0', '100', '0', '3000', '3000', '1000', '1000', '0', '', '11', '129294', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Cast Jojo Headbash, Blocks Cast'),
('5767000', '9', '4', '0', '0', '0', '100', '0', '2500', '2500', '1000', '1000', '0', '', '1', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Talk'),
('5767000', '9', '5', '0', '0', '0', '100', '0', '100', '100', '0', '0', '0', '', '28', '108846', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Remove Auras'),
('5767000', '9', '6', '0', '0', '0', '100', '0', '100', '100', '0', '0', '0', '', '28', '108846', '0', '0', '0', '0', '0', '18', '50', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Remove Player Auras'),
('5767000', '9', '7', '0', '0', '0', '100', '0', '100', '100', '0', '0', '0', '', '28', '108846', '0', '0', '0', '0', '0', '11', '0', '10', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Remove Creatures Auras'),
('5767000', '9', '8', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '45', '1', '1', '0', '0', '0', '0', '11', '0', '10', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Set Data'),
('65467', '0', '0', '0', '38', '0', '100', '0', '1', '1', '0', '0', '0', '', '80', '6546700', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Mesmerized Onlooker - On Data Set - Run Script'),
('56394', '0', '0', '0', '38', '0', '100', '0', '1', '1', '0', '0', '0', '', '80', '5639400', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Mesmerized Child - On Data Set - Run Script'),
('56393', '0', '0', '0', '38', '0', '100', '0', '1', '1', '0', '0', '0', '', '80', '5639300', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Mesmerized Onlooker - On Data Set - Run Script'),
('5639300', '9', '0', '0', '0', '0', '100', '0', '2000', '2000', '1000', '1000', '0', '', '5', '4', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Excited Onlooker - Play Emote'),
('5639300', '9', '1', '0', '0', '0', '100', '0', '3500', '3500', '1000', '1000', '0', '', '5', '21', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Excited Onlooker - Play Emote'),
('5639400', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '5', '18', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Mesmerized Child - Play Emote'),
('6546700', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '5', '4', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Excited Onlooker - Play Emote'),
('6546700', '9', '1', '0', '0', '0', '100', '0', '4000', '4000', '1000', '1000', '0', '', '5', '21', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Excited Onlooker - Play Emote'),
('6546700', '9', '2', '0', '0', '0', '100', '0', '5000', '5000', '1000', '1000', '0', '', '5', '4', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Excited Onlooker - Play Emote');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (57465,57466,55633,55632);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (57465,57466,55633,55632) AND `source_type` = 0;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('57466', '0', '0', '0', '0', '0', '100', '0', '4000', '8000', '15000', '20000', '0', '', '11', '109098', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Firethief - Update IC - Cast Set Ablaze'),
('55633', '0', '0', '0', '0', '0', '100', '0', '4000', '8000', '8000', '12000', '0', '', '11', '127940', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Firethief - Update IC - Cast Shoot'),
('55633', '0', '1', '0', '0', '0', '100', '0', '10000', '15000', '10000', '15000', '0', '', '11', '109104', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Firethief - Update IC - Cast Shoot ALL the fireworks!'),
('57465', '0', '0', '0', '0', '0', '100', '0', '4000', '8000', '8000', '12000', '0', '', '11', '128414', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Ruffian - Update IC - Cast Display of Fury'),
('55632', '0', '0', '0', '0', '0', '100', '0', '4000', '8000', '8000', '12000', '0', '', '11', '128414', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Ruffian - Update IC - Cast Display of Fury');

DELETE FROM `npc_vendor` WHERE `entry` IN (57623);
INSERT INTO `npc_vendor` (entry, slot, item, maxcount, incrtime, ExtendedCost, type, BonusListIDs, PlayerConditionID, IgnoreFiltering, VerifiedBuild) VALUES
('57623', '1', '2880', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '2', '2901', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '3', '3466', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '4', '3857', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '5', '5956', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '6', '18567', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '7', '160298', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '8', '180733', '0', '0', '0', '1', NULL, '0', '0', '63906');

UPDATE `creature_template` SET `ScriptName` = 'npc_ruk_ruk' WHERE `Entry` = 55634;
UPDATE `creature_template` SET `ScriptName` = 'npc_ruk_ruk_rocket' WHERE `Entry` = 64322;

UPDATE `creature_template` SET `faction` = '16' WHERE (`entry` = '57465'); -- was 14
UPDATE `creature_template` SET `unit_flags` = '0' WHERE (`entry` = '55632'); -- was 537133824

DELETE FROM `creature_template_difficulty` WHERE `entry` IN (55601);
INSERT INTO `creature_template_difficulty` VALUES
('55601', '0', '0', '0', '80', '4', '1', '1', '1', '0.2', '29467', '0', '0', '0', '55601', '55601', '0', '5', '5', '268435456', '0', '0', '0', '0', '0', '0', '0', '56647');

-- Floating Flag Zhaoren, Balloon
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '65559') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '55650') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '65560') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '55786') and (`DifficultyID` = '0');

UPDATE `creature_template_difficulty` SET `ContentTuningID` = '80' WHERE (`Entry` = '56274') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '80' WHERE (`Entry` = '55466') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '80' WHERE (`Entry` = '55672') and (`DifficultyID` = '0');

-- Fe-Feng Firethief stolen cannons aura
DELETE FROM `creature_template_addon` WHERE `entry` = 55633;
INSERT INTO `creature_template_addon` (`entry`, `SheathState`, `auras`) VALUES ('55633', '1', '127932');

DELETE FROM `creature_addon` WHERE `guid` IN (451477,451621);
INSERT INTO `creature_addon` (`guid`, `AnimTier`, `VisFlags`, `SheathState`, `auras`) VALUES ('451477', '2', '1', '1', '78718 121801 81312'); -- NPC: 55595 Aysa at Morning Breeze
INSERT INTO `creature_addon` (`guid`, `StandState`, `SheathState`) VALUES ('451621', '8', '1'); -- NPC: 55595 Aysa at ZhaoRen

DELETE FROM `phase_area` WHERE `PhaseId` IN (524,536, 1836) OR `AreaId` IN (5886,5829);
INSERT INTO `phase_area` VALUES
(5886, 1836, 'The Wandering Isle - Chamber of Whispers'),
(5829, 1836, 'The Wandering Isle - Zhao-Ren Dragon Area'),
(5886, 524, 'The Wandering Isle - Chamber of Whispers'),
(5829, 524, 'The Wandering Isle - Zhao-Ren Dragon Area'),
(5829, 536, 'The Wandering Isle - Zhao-Ren Dragon Area'),
(5886, 536, 'The Wandering Isle - Chamber of Whispers');

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 26
  AND `SourceGroup` IN (1836, 524, 536);

-- Phase 1836: before 29786
INSERT INTO `conditions` VALUES
(26, 1836, 5886, 0, 0, 47, 0, 29786, 75, 0, '', 1, 0, 0, '', 'Phase 1836 active if 29786 NOT in progress, taken, complete, rewarded'),
(26, 1836, 5829, 0, 0, 47, 0, 29786, 75, 0, '', 1, 0, 0, '', 'Phase 1836 active if 29786 NOT in progress, taken, complete, rewarded');

-- Phase 524: after 29785 but before 29786
INSERT INTO `conditions` VALUES
(26, 524, 5829, 0, 0, 47, 0, 29785, 64, 0, '', 0, 0, 0, '', 'Phase 524 active if 29785 rewarded'),
(26, 524, 5829, 0, 0, 47, 0, 29786, 66, 0, '', 1, 0, 0, '', 'Phase 524 active if 29786 NOT complete or rewarded');

-- Phase 524: after  but before 29786
INSERT INTO `conditions` VALUES
(26, 524, 5886, 0, 0, 47, 0, 29785, 64, 0, '', 0, 0, 0, '', 'Phase 524 active if 29785 rewarded'),
(26, 524, 5886, 0, 0, 47, 0, 29786, 66, 0, '', 1, 0, 0, '', 'Phase 524 active if 29786 NOT complete or rewarded');

-- Phase 536: after 29786
INSERT INTO `conditions` VALUES
(26, 536, 5829, 0, 0, 47, 0, 29786, 2, 0, '', 0, 0, 0, '', 'Phase 536 active if 29786 complete'),
(26, 536, 5886, 0, 0, 47, 0, 29786, 2, 0, '', 0, 0, 0, '', 'Phase 536 active if 29786 complete');

-- Phase 536: after 29786
INSERT INTO `conditions` VALUES
(26, 536, 5829, 0, 0, 47, 0, 29787, 74, 0, '', 1, 0, 0, '', 'Phase 536 active if 29787 in progress or complete or rewarded'),
(26, 536, 5886, 0, 0, 47, 0, 29787, 74, 0, '', 1, 0, 0, '', 'Phase 536 active if 29787 in progress or complete or rewarded');

DELETE FROM `creature_template_addon` WHERE `entry` IN (55650,65560,64507,64505,64506,55874);
INSERT INTO `creature_template_addon` (`entry`, `AnimTier`, `VisFlags`, `SheathState`, `visibilityDistanceType`, `auras`) VALUES ('55650', '3', '1', '0', 3, '82358'); -- Shang Xi's Hot Air Balloon
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `AnimTier`) VALUES ('65560', '6556000', 3); -- Dafeng spawn ??
INSERT INTO `creature_template_addon` (`entry`, `auras`) VALUES ('64507', '81312'); -- NPC: 64507 Firework Launcher
INSERT INTO `creature_template_addon` (`entry`, `SheathState`, `emote`) VALUES ('64505', '1', '27'); -- NPC: 64505 Ji at Zhaoren
INSERT INTO `creature_template_addon` (`entry`, `StandState`, `SheathState`, `emote`) VALUES ('64506', '0', '1', '0'); -- NPC: 64506 Aysha at Zhaoren
INSERT INTO `creature_template_addon` (`entry`, `AnimTier`, `VisFlags`, `SheathState`, `auras`) VALUES ('55874', '0', '0', '1', '29266'); -- NPC: 55874 Dead Zhaoren

UPDATE `creature` SET `PhaseId` = 524 WHERE `guid` IN (451612,451613,451615,451614,451616,451617,451618,451619,451620);
UPDATE `creature` SET `PhaseId` = 536 WHERE `guid` IN (451621, 451622, 451623, 451624);
UPDATE `creature` SET `PhaseId` = 1836 WHERE `guid` IN (451498,451501,451610, 451599);

UPDATE `gameobject` SET `PhaseId` = 1836 WHERE `guid` IN (300567, 300568);

UPDATE `creature` SET `wander_distance` = '50', `MovementType`=1 WHERE (`guid` = '451589');
UPDATE `creature_template` SET `ScriptName` = 'npc_zhaoren' WHERE `Entry` = 55786;
UPDATE `creature_template` SET `ScriptName` = 'npc_aysa_outside_chambers_of_whispers' WHERE `Entry` = 55744;

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55592,55595,64506,64505,64507,64532,64543);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55592,55595,64506,64505,64507,6450700,64532,6453200,64543,6454300) AND `source_type` IN (0,9);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('55592', '0', '0', '0', '11', '0', '100', '0', '0', '0', '0', '0', '0', '', '48', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Dafeng - on Respawn - Set Active'),
('55595', '0', '0', '0', '19', '0', '100', '0', '29786', '0', '0', '0', '0', '', '85', '126059', '2', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Aysa Cloudsinger - On Accepted Quest - Invoker Cast Summon Aysa'),
('64505', '0', '0', '0', '0', '4', '100', '0', '3000', '5000', '3000', '5000', '0', '', '11', '128630', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Update IC - Cast Jab'),
('64505', '0', '1', '0', '0', '4', '100', '0', '5000', '8000', '8000', '12000', '0', '', '11', '128631', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Update IC - Cast Blackout Kick'),
('64505', '0', '2', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '69', '2', '0', '0', '0', '0', '0', '1', '0', '0', '0', '714.385', '4163.74', '195.89', '0', 'Ji Firepaw - On Data Set - Move to Pos'),
('64506', '0', '0', '0', '38', '0', '100', '0', '1', '1', '0', '0', '0', '', '1', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Aysa Cloudsinger - On Data Set - Talk'),
('64506', '0', '1', '0', '38', '0', '100', '0', '2', '2', '0', '0', '0', '', '1', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Aysa Cloudsinger - On Data Set - Talk'),
('64506', '0', '2', '0', '38', '0', '100', '0', '3', '3', '0', '0', '0', '', '1', '2', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Aysa Cloudsinger - On Data Set - Talk'),
('64506', '0', '3', '0', '38', '0', '100', '0', '2', '2', '0', '0', '0', '', '49', '0', '0', '0', '0', '0', '0', '11', '55786', '30', '0', '0', '0', '0', '0', 'Aysa Cloudsinger - On Data Set - Start Attack'),
('64506', '0', '4', '0', '60', '2', '100', '0', '4000', '8000', '8000', '12000', '0', '', '11', '117312', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Aysa Cloudsinger - Update - Cast Combat Roll'),
('64507', '0', '0', '0', '11', '0', '100', '0', '0', '0', '0', '0', '0', '', '22', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Firework Launcher - On Respawn - Set Event Phase 1'),
('64507', '0', '1', '2', '8', '1', '100', '0', '125961', '0', '0', '0', '0', '', '11', '125970', '2', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Firework Launcher - On Spellhit - Cast AICast Overpacked Firework'),
('64507', '0', '2', '3', '61', '1', '100', '0', '0', '0', '0', '0', '0', '', '117', '2538', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Firework Launcher - On Spellhit - Play Oneshot AnimKit'),
('64507', '0', '3', '4', '61', '1', '100', '0', '0', '0', '0', '0', '0', '', '11', '125964', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Firework Launcher - On Spellhit - Cast Firework Launcher Inactive'),
('64507', '0', '4', '5', '61', '1', '100', '0', '0', '0', '0', '0', '0', '', '11', '104080', '0', '0', '0', '0', '0', '10', '451613', '64505', '0', '0', '0', '0', '0', 'Firework Launcher - On Spellhit - Set Data'),
('64507', '0', '5', '6', '61', '1', '100', '0', '0', '0', '0', '0', '0', '', '80', '6450700', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Firework Launcher - On Spellhit - Run Script'),
('64507', '0', '6', '0', '61', '1', '100', '0', '0', '0', '0', '0', '0', '', '22', '2', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Firework Launcher - On Spellhit - Set Event Phase 2'),
('64507', '0', '7', '0', '60', '2', '100', '0', '5000', '5000', '5000', '5000', '0', '', '11', '104080', '0', '0', '0', '0', '0', '10', '451613', '64505', '0', '0', '0', '0', '0', 'Firework Launcher - Update - Cast Firework Launcher Inactive'),
('64507', '0', '8', '0', '38', '0', '100', '0', '1', '1', '0', '0', '0', '', '22', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Firework Launcher - On Data Set - Set Event Phase 1'),
('6450700', '9', '0', '0', '0', '0', '100', '0', '21000', '21000', '1000', '1000', '0', '', '22', '1', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Firework Launcher - Set Event Phase 1'),
('64532', '0', '1', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '6453200', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Dafeng - Just Spawned - Run Script'),
('64532', '0', '2', '0', '38', '0', '100', '0', '5', '5', '0', '0', '0', '', '69', '1', '0', '0', '0', '0', '0', '8', '0', '0', '0', '709.238', '4177.15', '198.796', '0', 'Dafeng - On Data Set - Move to Pos'),
('64543', '0', '0', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '6454300', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Aysa Cloudsinger - Just Spawned - Run Script'),
('64543', '0', '1', '2', '58', '0', '100', '0', '12', '6454300', '0', '0', '0', '', '97', '15', '20', '0', '0', '0', '0', '1', '0', '0', '0', '675.445', '4204.72', '196.58', '0', 'Aysa Cloudsinger - On WP Ended - Jump to Pos'),
('64543', '0', '2', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Aysa Cloudsinger - Movement Inform - Despawn'),
('6453200', '9', '0', '0', '0', '0', '100', '0', '5000', '5000', '1000', '1000', '0', '', '53', '1', '6453200', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Dafeng - Start WP'),
('6454300', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '59', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Aysa Cloudsinger - Just Spawned - Run Script'),
('6454300', '9', '1', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '53', '1', '6454300', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Aysa Cloudsinger - Start WP');


DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 13 AND `SourceEntry` = 104855;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `ConditionTypeOrReference`, `ConditionValue1`, `ConditionValue2`, `Comment`) VALUES 
('13', '1', '104855', '51', '5', '55786', 'Overpacked Firework target Zhao-Ren');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (56159,56274,55672);
UPDATE `gameobject_template` SET `AIName` = 'SmartGameObjectAI' WHERE `Entry` IN (209922);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (56159,5615900,56274,209922,55672);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('56159', '0', '0', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5615900', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Just Spawned - Run Script'),
('56159', '0', '1', '2', '40', '0', '100', '0', '7', '5615900', '0', '0', '0', '', '59', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On WP Reached - Set Walk'),
('56159', '0', '2', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '1', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On WP Reached - Talk'),
('56159', '0', '3', '0', '40', '0', '100', '0', '8', '5615900', '0', '0', '0', '', '1', '2', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On WP Reached - Talk'),
('56159', '0', '4', '0', '40', '0', '100', '0', '9', '5615900', '0', '0', '0', '', '1', '3', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On WP Reached - Talk'),
('56159', '0', '5', '0', '58', '0', '100', '0', '12', '5615900', '0', '0', '0', '', '66', '0', '0', '0', '0', '0', '0', '8', '0', '0', '0', '0', '0', '0', '5.48033', 'Master Shang - On WP Ended - Set Orientation'),
('56159', '0', '6', '7', '38', '0', '100', '0', '1', '1', '0', '0', '0', '', '55', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On Data Set - Stop WP'),
('56159', '0', '7', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '1', '4', '2000', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On Data Set - Talk'),
('56159', '0', '8', '0', '52', '0', '100', '0', '4', '56159', '0', '0', '0', '', '53', '1', '5615901', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On Text Over - Start WP'),
('56159', '0', '9', '10', '58', '0', '100', '0', '6', '5615901', '0', '0', '0', '', '11', '109144', '2', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On WP Ended - Cast Force Trigger Master Shang Xi Final Escort Say'),
('56159', '0', '10', '11', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '66', '0', '0', '0', '0', '0', '0', '8', '0', '0', '0', '0', '0', '0', '4.53786', 'Master Shang - On WP Ended - Set Orientation'),
('56159', '0', '11', '12', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '44', '1527', '1', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On WP Ended - Add Phase'),
('56159', '0', '12', '13', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '28', '105333', '0', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On WP Ended - Remove Aura'),
('56159', '0', '13', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On WP Ended - Despawn'),
('5615900', '9', '0', '0', '0', '0', '100', '0', '2000', '2000', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Talk'),
('5615900', '9', '1', '0', '0', '0', '100', '0', '3000', '3000', '1000', '1000', '0', '', '53', '1', '5615900', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Start WP'),
('56274', '0', '0', '0', '0', '0', '100', '0', '8000', '12000', '8000', '12000', '0', '', '11', '125218', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Guardian of the Elders - Update IC - Cast Pounce'),
('56274', '0', '1', '2', '6', '0', '100', '0', '0', '0', '0', '0', '0', '', '45', '1', '1', '0', '0', '0', '0', '9', '56159', '0', '50', '0', '0', '0', '0', 'Guardian of the Elders - On Death - Set Data'),
('56274', '0', '2', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '45', '1', '1', '0', '0', '0', '0', '15', '209922', '0', '50', '0', '0', '0', '0', 'Guardian of the Elders - On Death - Set Data'),
('209922', '1', '0', '0', '38', '0', '100', '0', '1', '1', '0', '0', '0', '', '41', '1000', '20000', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Spirit Wall - On Data Set - Despawn'),
('55672', '0', '0', '0', '19', '0', '100', '0', '29790', '0', '0', '0', '0', '', '85', '106623', '2', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On Accepted Quest - Invoker Cast Summon Master Shang Xi'),
('55672', '0', '1', '0', '20', '0', '100', '0', '29787', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On Reward Quest - Master Shang Xi Talk');

DELETE FROM `creature_loot_template` WHERE `entry` IN (55601);
INSERT INTO `creature_loot_template` (Entry, ItemType, Item, Chance, QuestRequired, LootMode, GroupId, MinCount, MaxCount, Comment) VALUES
('55601', '0', '117', '2.55494', '0', '1', '0', '1', '1', 'Tough Jerky'),
('55601', '0', '118', '0.885809', '0', '1', '0', '1', '1', 'Minor Healing Potion'),
('55601', '0', '159', '1.62347', '0', '1', '0', '1', '1', 'Refreshing Spring Water'),
('55601', '0', '727', '0.185685', '0', '1', '0', '1', '1', 'Notched Shortsword'),
('55601', '0', '766', '0.29324', '0', '1', '0', '1', '1', 'Flanged Mace'),
('55601', '0', '767', '0.185685', '0', '1', '0', '1', '1', 'Long Bo Staff'),
('55601', '0', '774', '1', '0', '1', '0', '1', '1', 'Malachite'),
('55601', '0', '818', '0.341944', '0', '1', '0', '1', '1', 'Tigerseye'),
('55601', '0', '1179', '0.10857', '0', '1', '0', '1', '1', 'Ice Cold Milk'),
('55601', '0', '1411', '0.214096', '0', '1', '0', '1', '1', 'Withered Staff'),
('55601', '0', '1412', '0.299328', '0', '1', '0', '1', '1', 'Crude Bastard Sword'),
('55601', '0', '1413', '0.28005', '0', '1', '0', '1', '1', 'Feeble Sword'),
('55601', '0', '1414', '0.194817', '0', '1', '0', '1', '1', 'Cracked Sledge'),
('55601', '0', '1415', '0.261785', '0', '1', '0', '1', '1', 'Carpenter\'s Mallet'),
('55601', '0', '1416', '0.260771', '0', '1', '0', '1', '1', 'Rusty Hatchet'),
('55601', '0', '1417', '0.233375', '0', '1', '0', '1', '1', 'Beaten Battle Axe'),
('55601', '0', '1418', '0.150172', '0', '1', '0', '1', '1', 'Worn Leather Belt'),
('55601', '0', '1419', '0.133937', '0', '1', '0', '1', '1', 'Worn Leather Boots'),
('55601', '0', '1420', '0.180612', '0', '1', '0', '1', '1', 'Worn Leather Bracers'),
('55601', '0', '1422', '0.202934', '0', '1', '0', '1', '1', 'Worn Leather Gloves'),
('55601', '0', '1423', '0.245551', '0', '1', '0', '1', '1', 'Worn Leather Pants'),
('55601', '0', '1425', '0.157274', '0', '1', '0', '1', '1', 'Worn Leather Vest'),
('55601', '0', '1427', '0.144083', '0', '1', '0', '1', '1', 'Patchwork Shoes'),
('55601', '0', '1429', '0.15626', '0', '1', '0', '1', '1', 'Patchwork Cloak'),
('55601', '0', '1430', '0.191773', '0', '1', '0', '1', '1', 'Patchwork Gloves'),
('55601', '0', '1431', '0.164377', '0', '1', '0', '1', '1', 'Patchwork Pants'),
('55601', '0', '1433', '0.172494', '0', '1', '0', '1', '1', 'Patchwork Armor'),
('55601', '0', '2138', '0.218155', '0', '1', '0', '1', '1', 'Sharpened Letter Opener'),
('55601', '0', '2212', '0.15626', '0', '1', '0', '1', '1', 'Cracked Buckler'),
('55601', '0', '2213', '0.161333', '0', '1', '0', '1', '1', 'Worn Large Shield'),
('55601', '0', '2287', '0.203949', '0', '1', '0', '1', '1', 'Haunch of Meat'),
('55601', '0', '2635', '0.350062', '0', '1', '0', '1', '1', 'Loose Chain Belt'),
('55601', '0', '2642', '0.485013', '0', '1', '0', '1', '1', 'Loose Chain Boots'),
('55601', '0', '2643', '0.228301', '0', '1', '0', '1', '1', 'Loose Chain Bracers'),
('55601', '0', '2645', '0.158289', '0', '1', '0', '1', '1', 'Loose Chain Gloves'),
('55601', '0', '2646', '0.160318', '0', '1', '0', '1', '1', 'Loose Chain Pants'),
('55601', '0', '2648', '0.143069', '0', '1', '0', '1', '1', 'Loose Chain Vest'),
('55601', '0', '2773', '0.250624', '0', '1', '0', '1', '1', 'Cracked Shortbow'),
('55601', '0', '2774', '0.222213', '0', '1', '0', '1', '1', 'Rust-Covered Blunderbuss'),
('55601', '0', '3189', '0.244536', '0', '1', '0', '1', '1', 'Wood Chopper'),
('55601', '0', '3370', '0.165392', '0', '1', '0', '1', '1', 'Patchwork Belt'),
('55601', '0', '3373', '0.194817', '0', '1', '0', '1', '1', 'Patchwork Bracers'),
('55601', '0', '4560', '0.127849', '0', '1', '0', '1', '1', 'Fine Scimitar'),
('55601', '0', '4562', '0.115673', '0', '1', '0', '1', '1', 'Severing Axe'),
('55601', '0', '4563', '0.277005', '0', '1', '0', '1', '1', 'Billy Club'),
('55601', '0', '4565', '0.126834', '0', '1', '0', '1', '1', 'Simple Dagger'),
('55601', '0', '8177', '0.27802', '0', '1', '0', '1', '1', 'Practice Sword'),
('55601', '0', '8178', '0.141039', '0', '1', '0', '1', '1', 'Training Sword'),
('55601', '0', '8179', '0.107555', '0', '1', '0', '1', '1', 'Cadet\'s Bow'),
('55601', '0', '8181', '0.152201', '0', '1', '0', '1', '1', 'Hunting Rifle'),
('55601', '0', '8182', '0.334842', '0', '1', '0', '1', '1', 'Pellet Rifle'),
('55601', '0', '74615', '100', '1', '1', '0', '1', '1', 'Paint Soaked Brush');

UPDATE `gameobject_template` SET `Data6` = 120000, `Data23` = 1 WHERE `entry` IN (209656, 209660, 209661, 209663);

UPDATE `gameobject_template_addon` SET `flags` = 0 WHERE `entry` IN (209660, 209661); -- was 36

DELETE FROM `gameobject`
WHERE `guid` IN (
    300646, 300791, 300643, 300789, 300540, 300642, 300648, 300539,
    300644, 300787, 300788, 300759, 300751, 300753, 300466, 300560,
    300793, 300488, 300650, 300792, 300490, 300562, 300790, 300489,
    300561, 300649
); -- duplicate spawns or maybe for another phase????