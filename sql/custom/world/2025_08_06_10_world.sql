-- NPC: 57132 Wu-song Villager
-- NPC: 55021 Jojo Ironbrow at first house
-- NPC: 55585 Jojo Ironbrow at Morning Breeze

-- Adding some creature text for villager script
DELETE FROM `creature_text` WHERE `creatureID` IN (57132);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('57132', '0', '0', 'I have had enough of this!', '12', '0', '100', '1', '0', '0', '0', '0', 'Wu-song Villager Random line'),
('57132', '0', '1', 'Just... go away!', '12', '0', '100', '1', '0', '0', '0', '0', 'Wu-song Villager Random line'),
('57132', '0', '2', 'You nasty... little creature!', '12', '0', '100', '1', '0', '0', '0', '0', 'Wu-song Villager Random line'),
('57132', '0', '3', 'Get off of me!', '12', '0', '100', '1', '0', '0', '0', '0', 'Wu-song Villager Random line');

-- Fix Visibility and state addons
DELETE FROM `creature_template_addon` WHERE `entry` IN (55585,55021);
INSERT INTO `creature_template_addon` 
(`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('55585', '0', '0', '0', '1', '0', '1', '0', '0', '461', '0', '0', '0', '0', '84886'),
('55021', '0', '0', '0', '1', '0', '1', '0', '0', '461', '0', '0', '0', '0', '82343');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (57132);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (5763800,57132);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('5763800', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Talk'),
('5763800', '9', '1', '0', '0', '0', '100', '0', '2000', '2000', '1000', '1000', '0', '', '128', '1', '1', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Set AI AnimKit'),
('5763800', '9', '2', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '69', '1', '0', '0', '0', '0', '0', '8', '0', '0', '0', '1039.19', '3284.26', '129.397', '0', 'Jojo Ironbrow - Move to Pos'),
('5763800', '9', '3', '0', '0', '0', '100', '0', '3200', '3200', '1000', '1000', '0', '', '11', '129272', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Cast Jojo Headbash, Reeds Cast'),
('5763800', '9', '4', '0', '0', '0', '100', '0', '2500', '2500', '1000', '1000', '0', '', '1', '1', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Talk'),
('5763800', '9', '5', '0', '0', '0', '100', '0', '100', '100', '0', '0', '0', '', '28', '108798', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Remove Auras'),
('5763800', '9', '6', '0', '0', '0', '100', '0', '100', '100', '0', '0', '0', '', '28', '108798', '0', '0', '0', '0', '0', '18', '50', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Remove Player Auras'),
('5763800', '9', '7', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '45', '1', '1', '0', '0', '0', '0', '11', '0', '10', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Set Data'),
('5763800', '9', '8', '0', '0', '0', '100', '0', '8000', '8000', '1000', '1000', '0', '', '53', '0', '7', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Start WP');

INSERT INTO `smart_scripts` VALUES
('57132', '0', '0', '0', '', '0', '0', '100', '0', '20000', '20000', '40000', '65000', '0', '', '1', '0', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Wu-song Villager - In Combat - Say Line 0');

-- Fix invalid scripts
UPDATE `smart_scripts` SET `event_param3` = '120000', `event_param4` = '120000' WHERE (`entryorguid` = '-450361') and (`source_type` = '0') and (`id` = '0') and (`link` = '0');
UPDATE `smart_scripts` SET `event_param3` = '120000', `event_param4` = '120000' WHERE (`entryorguid` = '-450358') and (`source_type` = '0') and (`id` = '0') and (`link` = '0');
