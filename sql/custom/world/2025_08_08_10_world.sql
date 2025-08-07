-- Quest: 29774 Not In the Face!
-- NPC: 55556 Shu at the Farmstead
-- NPC: 55558 Shu spawn at the Farmstead
-- NPC: 55539 Wugou


UPDATE `creature_template` SET `AIName` = '' WHERE `Entry` IN (55556,55558);
UPDATE `creature_template` SET `ScriptName` = 'npc_shu_at_farmstead' WHERE `Entry` = 55556;

DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55556,55558,5555800);

DELETE FROM `waypoint_path` WHERE `PathId` IN (10);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(10,1,0,'Spawned Shu at the Farmstead after Wugou wakes up');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (10);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
('10', '0', '815.315', '3608.55', '173.987'),
('10', '1', '831.068', '3607.74', '175.247'),
('10', '2', '841.146', '3606.77', '175.188'),
('10', '3', '855.831', '3607.75', '173.619'),
('10', '4', '880.269', '3605.99', '192.267'),
('10', '5', '892.776', '3605.94', '192.93'),
('10', '6', '900.755', '3603.12', '193.08'),
('10', '7', '906.239', '3600.17', '193.1');

UPDATE `waypoint_path_node` SET `Orientation` = '6.21337' WHERE (`PathId` = 8 AND `NodeId` = 5);

UPDATE `creature_template` SET `AIName` = 'SmartAI', `unit_flags` = '33554944', `unit_flags2` = '2048' WHERE (`entry` = '66941');

DELETE FROM `creature_template_addon` WHERE `entry` IN (66941);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('66941', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '116695'); -- Water Spout Bunny at farmstead -- not sure of the invis aura ????? 

-- these flags disable gravity for the bunny so it can stay above water
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '66941') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '66941') and (`DifficultyID` = '1');

DELETE FROM `smart_scripts` WHERE `entryorguid` IN (66941,6694100); -- 15880 was the original model but not working
INSERT INTO `smart_scripts` VALUES -- we use model 21072 now taken from wowhead
('66941', '0', '0', '1', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '3', '0', '21072', '0', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '', '0', '0', '0', '0', 'Water Spout - Just Spawned - Morph'),
('66941', '0', '1', '0', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '6694100', '2', '0', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '', '0', '0', '0', '0', 'Water Spout - Just Spawned - Run Script'),
('6694100', '9', '0', '0', '', '0', '0', '100', '0', '5000', '5000', '1000', '1000', '0', '', '11', '117057', '2', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Cast Water Spout Geyser Aura'),
('6694100', '9', '1', '0', '', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '11', '116696', '2', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Cast Water Spout Burst'),
('6694100', '9', '2', '0', '', '0', '0', '100', '0', '0', '0', '3000', '3000', '0', '', '28', '116695', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Remove Aura'),
('6694100', '9', '3', '0', '', '0', '0', '100', '0', '3000', '3000', '1000', '1000', '0', '', '41', '0', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Despawn');

