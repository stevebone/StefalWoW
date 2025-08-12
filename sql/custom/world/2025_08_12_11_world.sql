-- Quest: 29786 Battle for the skies
-- NPC: 65559 Vision of Zhao-Ren
-- NPC: 55650 Shang Xi's Hot Air Balloon
-- NPC: 55592 Dafeng Spirit of Air
-- NPC: 65560 Dafeng spawn ??
-- NPC: 64532 Dafeng spawn (quest accept)
-- NPC: 55595 Aysa with Dafeng
-- NPC: 64543 Aysa spawn  (quest accept)
-- NPC: 55786 Zhaoren

DELETE FROM `quest_template_addon` WHERE `ID` IN (29786);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29786', '0', '0', '0', '29785', '29787', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `creature_queststarter` WHERE `quest` IN (29786);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55595', '29786', '0');

DELETE FROM `creature_template_addon` WHERE `entry` IN (55650,65560,64507,64505,64506,55874);
INSERT INTO `creature_template_addon` (`entry`, `AnimTier`, `VisFlags`, `SheathState`, `auras`) VALUES ('55650', '0', '1', '0', '82358');
INSERT INTO `creature_template_addon` (`entry`, `PathId`) VALUES ('65560', '17');
INSERT INTO `creature_template_addon` (`entry`, `auras`) VALUES ('64507', '81312');
INSERT INTO `creature_template_addon` (`entry`, `SheathState`, `emote`) VALUES ('64505', '1', '27');
INSERT INTO `creature_template_addon` (`entry`, `StandState`, `SheathState`, `emote`) VALUES ('64506', '0', '1', '0');
INSERT INTO `creature_template_addon` (`entry`, `AnimTier`, `VisFlags`, `SheathState`, `auras`) VALUES ('55874', '0', '0', '1', '29266');

UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '65559') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '65559') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '55650') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '55650') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '65560') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '65560') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '55786') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '55786') and (`DifficultyID` = '1');

-- removed dragon, dead zhaoren, ji & aysa and master shang since they are spawned by script
DELETE FROM `creature` WHERE (`guid` = '451622');
DELETE FROM `creature` WHERE (`guid` = '451624');
DELETE FROM `creature` WHERE (`guid` = '451612');
DELETE FROM `creature` WHERE (`guid` = '451613');
DELETE FROM `creature` WHERE (`guid` = '451615');

UPDATE `creature` SET `wander_distance` = '50', `MovementType`=1 WHERE (`guid` = '451589');
UPDATE `creature_template` SET `ScriptName` = 'npc_zhaoren' WHERE `Entry` = 55786;

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55592,55595,64506,64505,64507,64532,64543);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55592,55595,64506,64505,64507,6450700,64532,6453200,64543,6454300);
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
('64543', '0', '1', '2', '58', '0', '100', '0', '12', '19', '0', '0', '0', '', '97', '15', '20', '0', '0', '0', '0', '1', '0', '0', '0', '675.445', '4204.72', '196.58', '0', 'Aysa Cloudsinger - On WP Ended - Jump to Pos'),
('64543', '0', '2', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Aysa Cloudsinger - Movement Inform - Despawn'),
('6453200', '9', '0', '0', '0', '0', '100', '0', '5000', '5000', '1000', '1000', '0', '', '53', '1', '18', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Dafeng - Start WP'),
('6454300', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '59', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Aysa Cloudsinger - Just Spawned - Run Script'),
('6454300', '9', '1', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '53', '1', '19', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Aysa Cloudsinger - Start WP');


DELETE FROM `creature_text` WHERE `creatureID` IN (55595,64506);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('55595', '0', '0', 'Dafeng! What\'s wrong? Why are you hiding back here?', '12', '0', '100', '1', '0', '27403', '54362', '0', 'Aysa Cloudsinger to Player'),
('64506', '0', '0', 'That\'s the way to do it!', '12', '0', '100', '0', '0', '27380', '64766', '0', 'Aysa Cloudsinger to Zhao-Ren'),
('64506', '1', '0', 'You\'ve done it!  Now quickly, lets attack it while it\'s grounded.', '12', '0', '100', '0', '0', '27381', '64767', '0', 'Aysa Cloudsinger to Zhao-Ren'),
('64506', '2', '0', 'It\'s down!  Let\'s finish it off.', '12', '0', '100', '0', '0', '27382', '64768', '0', 'Aysa Cloudsinger to Zhao-Ren');


DELETE FROM `areatrigger_scripts` WHERE `entry` = 7037;
INSERT INTO `areatrigger_scripts` VALUES
(7037, 'at_chamber_of_whispers');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 13 AND `SourceEntry` = 104855;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `ConditionTypeOrReference`, `ConditionValue1`, `ConditionValue2`, `Comment`) VALUES 
('13', '1', '104855', '51', '5', '55786', 'Overpacked Firework target Zhao-Ren');


DELETE FROM `waypoint_path` WHERE `PathId` IN (16,17,18,19);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(16,0,0,'Zhao-Ren encounter'),
(17,0,0,'Dafeng - Zhao-Ren encounter'), -- ???
(18,1,0,'Dafeng spawn - Zhao-Ren encounter'),
(19,1,0,'Aysa spawn - Zhao-Ren encounter');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (16,17,18,19);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
('16', '0', '688.196', '4186.41', '214.866'),
('16', '1', '682.382', '4176.08', '214.518'),
('16', '2', '682.057', '4157.68', '214.091'),
('16', '3', '691.184', '4145.19', '214.619'),
('16', '4', '709.08', '4137.22', '216.301'),
('16', '5', '734.297', '4134.32', '220.846'),
('16', '6', '750.481', '4142.9', '222.946'),
('16', '7', '760.123', '4158.66', '223.89'),
('16', '8', '752.167', '4179.3', '222.461'),
('16', '9', '736.622', '4189.58', '220.091'),
('16', '10', '720.887', '4193.51', '216.793'),
('16', '11', '704.3', '4195.27', '215.213'),
('17', '0', '1163.32', '4094.89', '196.724'),
('17', '1', '1160.19', '4088.9', '196.872'),
('17', '2', '1156.36', '4080.83', '199.682'),
('17', '3', '1157.8', '4075', '201.545'),
('17', '4', '1162.87', '4072.48', '203.953'),
('17', '5', '1170.45', '4076.58', '206.33'),
('17', '6', '1173.44', '4088.29', '209.173'),
('17', '7', '1171.33', '4097.33', '209.173'),
('17', '8', '1170.73', '4107.09', '208.22'),
('17', '9', '1177.14', '4113.9', '207.118'),
('17', '10', '1185.21', '4111.54', '205.337'),
('17', '11', '1186.82', '4103.22', '202.725'),
('17', '12', '1177.81', '4099.19', '202.077'),
('17', '13', '1169.81', '4098.71', '199.541'),
('18', '0', '546.745', '4318.28', '213.003'),
('18', '1', '552.755', '4312.86', '213.003'),
('18', '2', '567', '4299.03', '213.003'),
('18', '3', '579.771', '4287.23', '213.003'),
('18', '4', '590.753', '4277.32', '211.554'),
('18', '5', '602.771', '4266.65', '209.855'),
('18', '6', '623.247', '4247.82', '207.77'),
('18', '7', '637.07', '4235.15', '207.77'),
('18', '8', '646.76', '4226.14', '207.77'),
('18', '9', '653.672', '4218.67', '211.106'),
('18', '10', '666.007', '4207.71', '215.08'),
('18', '11', '675.144', '4200.54', '214.716'),
('18', '12', '684.406', '4193.85', '211.631'),
('19', '0', '551.445', '4312.52', '210.243'),
('19', '1', '558.695', '4306.77', '210.743'),
('19', '2', '566.695', '4299.52', '210.743'),
('19', '3', '575.195', '4291.52', '210.743'),
('19', '4', '585.195', '4282.52', '210.243'),
('19', '5', '591.445', '4276.27', '210.243'),
('19', '6', '597.195', '4271.52', '206.743'),
('19', '7', '606.945', '4262.02', '206.743'),
('19', '8', '611.945', '4257.27', '203.243'),
('19', '9', '621.945', '4248.27', '202.993'),
('19', '10', '635.445', '4236.02', '202.993'),
('19', '11', '646.945', '4227.52', '203.243'),
('19', '12', '656.695', '4219.77', '203.243');



