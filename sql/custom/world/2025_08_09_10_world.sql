-- Quest: 29776 morning-breeze-village
-- NPC: 54786 Master Shang Xi at temple (451410)
-- NPC: 55685 Uplifting Draft

DELETE FROM `quest_template_addon` WHERE `ID` IN (29776);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29776', '0', '0', '0', '29775', '29778', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `creature_queststarter` WHERE `quest` IN (29776);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('54786', '29776', '0');

-- these flags disable gravity for the bunny so it can float above
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '55685') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '55685') and (`DifficultyID` = '1');

DELETE FROM `creature_addon` WHERE `guid` IN (451410);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('451410', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '108900 49415 126160');

DELETE FROM `creature_template_addon` WHERE `entry` IN (55685);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('55685', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '99385');

UPDATE `creature_template` SET `AIName` = 'SmartAI', `VehicleId` = 1800 WHERE `Entry` IN (55685);
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55694);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55685,55694,5569400);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('55685', '0', '0', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '85', '46598', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Uplifting Draft - Just Spawned - Invoker Cast Ride Vehicle Hardcoded'),
('55685', '0', '1', '0', '27', '0', '100', '0', '0', '0', '0', '0', '0', '', '53', '1', '13', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Uplifting Draft - Passenger Boarded - Start WP'),
('55685', '0', '2', '3', '58', '0', '100', '0', '12', '13', '0', '0', '0', '', '11', '68576', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Uplifting Draft - WP Ended - Cast Eject All Passengers'),
('55685', '0', '3', '4', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '11', '104490', '2', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Uplifting Draft - WP Ended - Cast Forcecast Trigger Ji Air Plateau Departure'),
('55685', '0', '4', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Uplifting Draft - WP Ended - Despawn'),
('55694', '0', '0', '0', '8', '0', '100', '0', '104489', '0', '0', '0', '0', '', '80', '5569400', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - On Spellhit - Run Script'),
('55694', '0', '1', '0', '58', '0', '100', '0', '41', '14', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - WP Ended - Despawn'),
('5569400', '9', '0', '0', '0', '0', '100', '0', '1500', '1500', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5569400', '9', '1', '0', '0', '0', '100', '0', '3000', '3000', '1000', '1000', '0', '', '53', '1', '14', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Start WP');

DELETE FROM `creature_text` WHERE `CreatureID` = 55694;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES 
('55694', '0', '0', 'Hey $p! This way to Morning Breeze Village.', '12', '0', '100', '0', '0', '27294', '54196', '0', 'Ji Firepaw to Player');


DELETE FROM `vehicle_template` WHERE `creatureId` = 55685;
INSERT INTO `vehicle_template` VALUES (55685, 0, NULL,0);

DELETE FROM `waypoint_path` WHERE `PathId` IN (13,14);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(13,1,0,'Uplifting Draft at temple'),
(14,1,0,'Ji Firepaw at temple - to Morning Breeze');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (13,14);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
('13', '0', '930.606', '3609.28', '201.169'),
('13', '1', '938.901', '3615.79', '203.69'),
('13', '2', '950.292', '3614.42', '204.503'),
('13', '3', '960.127', '3607.95', '209.473'),
('13', '4', '951.721', '3595.49', '218.821'),
('13', '5', '941.727', '3596.61', '228.34'),
('13', '6', '932.559', '3604.83', '235.312'),
('13', '7', '939.153', '3613.67', '242.674'),
('13', '8', '946.714', '3611.3', '250.339'),
('13', '9', '948.613', '3602.81', '255.289'),
('13', '10', '942.122', '3596.85', '257.268'),
('13', '11', '933.01', '3600.14', '256.602'),
('13', '12', '920.45', '3604.77', '254.173'),
('14', '0', '919.644', '3631.51', '251.995'),
('14', '1', '919.951', '3640.1', '252.143'),
('14', '2', '911.599', '3653.28', '257.235'),
('14', '3', '902.828', '3667.67', '268.916'),
('14', '4', '896.158', '3678.36', '270.388'),
('14', '5', '884.846', '3698.18', '256.11'),
('14', '6', '873.531', '3716.29', '253.567'),
('14', '7', '870.245', '3729.18', '255.122'),
('14', '8', '872.625', '3741.65', '256.476'),
('14', '9', '874.821', '3754.83', '256.496'),
('14', '10', '873.896', '3771.67', '255.907'),
('14', '11', '871.934', '3791.37', '251.174'),
('14', '12', '871.807', '3815.84', '251.176'),
('14', '13', '871.632', '3847.02', '248.486'),
('14', '14', '876.247', '3875.8', '232.901'),
('14', '15', '882.415', '3883.23', '232.847'),
('14', '16', '884.998', '3894.71', '232.849'),
('14', '17', '882.644', '3903.48', '232.862'),
('14', '18', '872.936', '3912.66', '232.761'),
('14', '19', '871.109', '3927.02', '233.243'),
('14', '20', '871.368', '3946.04', '241.471'),
('14', '21', '871.648', '3962.31', '244.601'),
('14', '22', '871.74', '3983.4', '233.088'),
('14', '23', '871.255', '4019.36', '215.244'),
('14', '24', '873.498', '4039.44', '208.247'),
('14', '25', '887.143', '4063.88', '198.264'),
('14', '26', '897.17', '4083.92', '196.354'),
('14', '27', '905.981', '4105.24', '196.067'),
('14', '28', '913.248', '4119.95', '196.192'),
('14', '29', '924.639', '4129.2', '196.442'),
('14', '30', '939.283', '4134.46', '196.294'),
('14', '31', '953.13', '4143.33', '196.794'),
('14', '32', '963.005', '4155.36', '195.919'),
('14', '33', '976.026', '4159.32', '196.884'),
('14', '34', '993.514', '4155.73', '199.857'),
('14', '35', '1010.64', '4153.78', '202.817'),
('14', '36', '1026.41', '4152.24', '204.304'),
('14', '37', '1044.77', '4154.79', '206.585'),
('14', '38', '1062.83', '4159.12', '208.381'),
('14', '39', '1081.43', '4163.41', '206.348'),
('14', '40', '1096.34', '4167.47', '201.94'),
('14', '41', '1107.83', '4168.79', '196.201');

DELETE FROM `spell_script_names` WHERE `spell_id` = 104450;
INSERT INTO `spell_script_names` VALUES (104450, 'spell_summon_ji_firepaw_temple');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 13 AND `SourceEntry` = 104489;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `Comment`) VALUES 
('13', '1', '104489', '0', '0', '51', '0', '5', '55694', '0', '', '0', '0', '0', 'Trigger Ji Air Plateau Departure target Ji Firepaw');
