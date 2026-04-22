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
-- NPC: 64593 Korga temple faction choice
-- NPC: 57722 Delora temple faction choice
-- NPC: 55685 Uplifting Draf
-- NPC: 55694 Ji spawn temple spire

-- Quest: 29776 Morning Breeze Village
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
DELETE FROM `creature_template_gossip` WHERE (`CreatureID`=54786 AND `MenuID`=13158);
INSERT INTO `creature_template_gossip` (`CreatureID`, `MenuID`, `VerifiedBuild`) VALUES
(54786, 13158, 64978); -- Master Shang Xi

DELETE FROM `gossip_menu` WHERE (`MenuID`=13158 AND `TextID`=18536);
INSERT INTO `gossip_menu` (`MenuID`, `TextID`, `VerifiedBuild`) VALUES
(13158, 18536, 64978); -- 54786 (Master Shang Xi)

UPDATE creature_template SET ScriptName = 'npc_spirit_of_master_shang_xi_q31450' WHERE entry = 56013;

DELETE FROM `creature_template_addon` WHERE `entry` IN (55944, 57720, 57721, 64593, 56013, 60919, 60918, 60920, 60917, 54786, 57722);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureId`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('55944', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80797'),
('60919', '0', '0', '0', '3', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('60918', '6091800', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('60920', '0', '0', '0', '0', '3', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('60917', '0', '0', '0', '0', '3', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('54786', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '108900 49414 126160'), -- for Shang 1
(56013, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 82343), -- Spirit of Master temple
(57720, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 80797), -- Aysa Temple
(57721, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 80797), -- Ji Temple
(57722, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 80797), -- Delora temple
(64593, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 80797); -- Korga Temple

DELETE FROM `creature_template_difficulty` WHERE `entry` IN (56013,57721,57720,57722,64593);
INSERT INTO `creature_template_difficulty` VALUES
(56013, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28810, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(57720, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 26245, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(57721, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 26244, 0, 0, 0, 0, 0, 0, 0, 0, 805306624, 0, 0, 0, 0, 0, 0, 0, 56647),
(57722, 0, 0, 0, 1227, 5, 0.5, 1, 1, 0.2, 67905, 0, 0, 0, 0, 0, 0, 700, 1500, 0, 0, 0, 0, 0, 0, 0, 0, 40000),
(64593, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 57984, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647);

-- Creature
DELETE FROM `creature_addon` WHERE `guid` IN (451410);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('451410', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '108900 49415 126160'); -- Shang 2

UPDATE `creature` SET `PhaseId` = 1027 WHERE `guid` IN (451408); -- Huo
UPDATE `creature` SET `PhaseId` = 1028, `MovementType` = 2 WHERE `guid` IN (451409); -- Shu
UPDATE `creature` SET `PhaseId` = 1029 WHERE `guid` IN (451411); -- Wugou
UPDATE `creature` SET `PhaseId` = 1030 WHERE `guid` IN (451828); -- Dafeng

-- Liu Kang statue
SET @CGUID := 900000;
DELETE FROM `creature` WHERE `guid` IN (450021, @CGUID+741, @CGUID+742, @CGUID+747, @CGUID+748, @CGUID+749);
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `PhaseGroup`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `MovementType`, `npcflag`, `unit_flags`, `unit_flags2`, `unit_flags3`, `VerifiedBuild`) VALUES
(@CGUID+741, 56479, 860, 5736, 5820, '0', '1323', 0, 39406, 0, 942.28125, 3604.515625, 196.01605224609375, 6.248278617858886718, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Legacy of Liu Lang (Area: Храм Пяти Рассветов - Difficulty: 0) CreateObject1 (Auras: 132376 - GO_PA_TurtleShrine_01_Spell2_FireLoop) - !!! already present in database !!!
(@CGUID+742, 56479, 860, 5736, 5820, '0', '1324', 0, 39406, 0, 942.28125, 3604.515625, 196.01605224609375, 6.248278617858886718, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Legacy of Liu Lang (Area: Храм Пяти Рассветов - Difficulty: 0) CreateObject1 (Auras: 132376 - GO_PA_TurtleShrine_01_Spell2_FireLoop) - !!! already present in database !!!
(@CGUID+747, 56479, 860, 5736, 5820, '0', '1325', 0, 39406, 0, 942.28125, 3604.515625, 196.01605224609375, 6.248278617858886718, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978),
(@CGUID+748, 56479, 860, 5736, 5820, '0', '1326', 0, 39406, 0, 942.28125, 3604.515625, 196.01605224609375, 6.248278617858886718, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978),
(@CGUID+749, 56479, 860, 5736, 5820, '0', '1327', 0, 39406, 0, 942.28125, 3604.515625, 196.01605224609375, 6.248278617858886718, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978);

DELETE FROM `creature_addon` WHERE `guid` IN (450021, @CGUID+741, @CGUID+742, @CGUID+747, @CGUID+748, @CGUID+749);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(@CGUID+741, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 5, ''), -- Legacy of Liu Lang - 132376 - 
(@CGUID+742, 0, 0, 0, 0, 0, 1, 0, 420, 2188, 0, 0, 5, '132376'), -- Legacy of Liu Lang - 132376 - GO_PA_TurtleShrine_01_Spell2_FireLoop - 
(@CGUID+747, 0, 0, 0, 0, 0, 1, 0, 0, 2139, 0, 0, 5, ''),
(@CGUID+748, 0, 0, 0, 0, 0, 1, 0, 0, 1546, 0, 0, 5, '132377'),
(@CGUID+749, 0, 0, 0, 0, 0, 1, 0, 0, 1546, 0, 0, 5, '132378');

-- Faction choice event
DELETE FROM `spell_script_names` WHERE `spell_id` IN (108897, 113244, 113245, 130422, 132211);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES 
(108897, 'spell_pandaren_faction_choice'),
(113244, 'spell_faction_choice_trigger'),
(113245, 'spell_faction_choice_trigger'),
(130422, 'spell_balloon_exit_timer'),
(132211, 'spell_balloon_exit_timer');

DELETE FROM `spell_target_position` WHERE `ID` IN (116957, 116962);
INSERT INTO `spell_target_position` VALUES
('116957', '0', 0, '0', '-9128.09', '388.54', '91.163', '0', 0),
('116957', '1', 0, '0', '-9128.09', '388.54', '91.163', '0', 0),
('116962', '0', 0, '1', '1361.95', '-4375.25', '26.0891', 0, '20886'),
('116962', '1', 0, '1', '1361.95', '-4375.25', '26.0891', 0, '20886');

DELETE FROM `gossip_menu` WHERE `MenuID` IN (13726);
INSERT INTO `gossip_menu` VALUES 
(13726, 19723, 61967);

DELETE FROM `gossip_menu_option` WHERE `MenuID` IN (13726);
INSERT INTO `gossip_menu_option` VALUES
('13726', '36586', '0', '0', 'I\'m ready to decide.', '60279', '0', '0', '0', '0', NULL, '0', '0', NULL, '0', NULL, NULL, '61967');

DELETE FROM `creature_template_gossip` WHERE `CreatureID` IN (56013, 57720, 57721);
INSERT INTO `creature_template_gossip` (`CreatureID`, `MenuID`, `VerifiedBuild`) VALUES
('56013', '13726', '47936'),
('57720', '14670', '47936'),
('57721', '14671', '47936');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 15 AND `ConditionValue1` = 31450;
INSERT INTO `conditions` VALUES
(15, 13726, 0, 0, 0, 47, 0, 31450, 8, 0, '', 0, 0, 0, '', 'Player for which gossip text is shown has quest 31450 in progress');

-- Quest: 29776 morning-breeze-village
-- Master Shang casts Uplifting draft on the player
-- Ji spawns on the spire balcony and paths to the village
-- these flag 536870912 disable gravity for the bunny so it can float above
DELETE FROM `creature_template_difficulty` WHERE `entry` IN (55685, 55694);
INSERT INTO `creature_template_difficulty` VALUES
(55685, 0, 0, 0, 80, 4, 1, 1, 1, 1, 29316, 0, 0, 0, 0, 0, 0, 0, 0, 536870912, 0, 0, 0, 0, 0, 0, 0, 56647),
(55694, 0, 0, 0, 80, 4, 1, 1, 1, 1, 29290, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647);

UPDATE `creature` SET `ScriptName` = 'npc_shanxi_quest2' WHERE (`guid` = '451410');

DELETE FROM `creature_template_addon` WHERE `entry` IN (55685, 55694);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
(55685, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '99385'),
(55694, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, '');

UPDATE `creature_template` SET `AIName` = 'SmartAI', `VehicleId` = 1800 WHERE `Entry` IN (55685);
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55694);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55685,55694,5569400);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('55685', '0', '0', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '85', '46598', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Uplifting Draft - Just Spawned - Invoker Cast Ride Vehicle Hardcoded'),
('55685', '0', '1', '0', '27', '0', '100', '0', '0', '0', '0', '0', '0', '', '53', '1', '5568500', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Uplifting Draft - Passenger Boarded - Start WP'),
('55685', '0', '2', '3', '58', '0', '100', '0', '12', '5568500', '0', '0', '0', '', '11', '68576', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Uplifting Draft - WP Ended - Cast Eject All Passengers'),
('55685', '0', '3', '4', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '11', '104490', '2', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Uplifting Draft - WP Ended - Cast Forcecast Trigger Ji Air Plateau Departure'),
('55685', '0', '4', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Uplifting Draft - WP Ended - Despawn'),
('55694', '0', '0', '0', '8', '0', '100', '0', '104489', '0', '0', '0', '0', '', '80', '5569400', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - On Spellhit - Run Script'),
('55694', '0', '1', '0', '58', '0', '100', '0', '41', '5569400', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - WP Ended - Despawn'),
('5569400', '9', '0', '0', '0', '0', '100', '0', '1500', '1500', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5569400', '9', '1', '0', '0', '0', '100', '0', '3000', '3000', '1000', '1000', '0', '', '53', '1', '5569400', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Start WP');

DELETE FROM `vehicle_template` WHERE `creatureId` = 55685;
INSERT INTO `vehicle_template` VALUES (55685, 0, NULL,0);

DELETE FROM `waypoint_path` WHERE `PathId` IN (5568500,5569400);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(5568500,1,0,'Uplifting Draft at temple'),
(5569400,1,0,'Ji Firepaw at temple - to Morning Breeze');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (5568500,5569400);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
(5568500, '0', '930.606', '3609.28', '201.169'),
(5568500, '1', '938.901', '3615.79', '203.69'),
(5568500, '2', '950.292', '3614.42', '204.503'),
(5568500, '3', '960.127', '3607.95', '209.473'),
(5568500, '4', '951.721', '3595.49', '218.821'),
(5568500, '5', '941.727', '3596.61', '228.34'),
(5568500, '6', '932.559', '3604.83', '235.312'),
(5568500, '7', '939.153', '3613.67', '242.674'),
(5568500, '8', '946.714', '3611.3', '250.339'),
(5568500, '9', '948.613', '3602.81', '255.289'),
(5568500, '10', '942.122', '3596.85', '257.268'),
(5568500, '11', '933.01', '3600.14', '256.602'),
(5568500, '12', '920.45', '3604.77', '254.173'),
(5569400, '0', '919.644', '3631.51', '251.995'),
(5569400, '1', '919.951', '3640.1', '252.143'),
(5569400, '2', '911.599', '3653.28', '257.235'),
(5569400, '3', '902.828', '3667.67', '268.916'),
(5569400, '4', '896.158', '3678.36', '270.388'),
(5569400, '5', '884.846', '3698.18', '256.11'),
(5569400, '6', '873.531', '3716.29', '253.567'),
(5569400, '7', '870.245', '3729.18', '255.122'),
(5569400, '8', '872.625', '3741.65', '256.476'),
(5569400, '9', '874.821', '3754.83', '256.496'),
(5569400, '10', '873.896', '3771.67', '255.907'),
(5569400, '11', '871.934', '3791.37', '251.174'),
(5569400, '12', '871.807', '3815.84', '251.176'),
(5569400, '13', '871.632', '3847.02', '248.486'),
(5569400, '14', '876.247', '3875.8', '232.901'),
(5569400, '15', '882.415', '3883.23', '232.847'),
(5569400, '16', '884.998', '3894.71', '232.849'),
(5569400, '17', '882.644', '3903.48', '232.862'),
(5569400, '18', '872.936', '3912.66', '232.761'),
(5569400, '19', '871.109', '3927.02', '233.243'),
(5569400, '20', '871.368', '3946.04', '241.471'),
(5569400, '21', '871.648', '3962.31', '244.601'),
(5569400, '22', '871.74', '3983.4', '233.088'),
(5569400, '23', '871.255', '4019.36', '215.244'),
(5569400, '24', '873.498', '4039.44', '208.247'),
(5569400, '25', '887.143', '4063.88', '198.264'),
(5569400, '26', '897.17', '4083.92', '196.354'),
(5569400, '27', '905.981', '4105.24', '196.067'),
(5569400, '28', '913.248', '4119.95', '196.192'),
(5569400, '29', '924.639', '4129.2', '196.442'),
(5569400, '30', '939.283', '4134.46', '196.294'),
(5569400, '31', '953.13', '4143.33', '196.794'),
(5569400, '32', '963.005', '4155.36', '195.919'),
(5569400, '33', '976.026', '4159.32', '196.884'),
(5569400, '34', '993.514', '4155.73', '199.857'),
(5569400, '35', '1010.64', '4153.78', '202.817'),
(5569400, '36', '1026.41', '4152.24', '204.304'),
(5569400, '37', '1044.77', '4154.79', '206.585'),
(5569400, '38', '1062.83', '4159.12', '208.381'),
(5569400, '39', '1081.43', '4163.41', '206.348'),
(5569400, '40', '1096.34', '4167.47', '201.94'),
(5569400, '41', '1107.83', '4168.79', '196.201');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 13 AND `SourceEntry` = 104489;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `Comment`) VALUES 
(13, '1', '104489', '0', '0', '51', '0', '5', '55694', '0', '', '0', '0', '0', 'Trigger Ji Air Plateau Departure target Ji Firepaw');