-- NPC: 55213 Shu (spawned by spell)
-- NPC: 55539 Wugou

UPDATE `creature_template` SET `ScriptName` = 'npc_shu_follower' WHERE `Entry` IN (55213);
UPDATE `creature_template` SET `movementId` = '65' WHERE (`entry` = '55213');

DELETE FROM `creature_template_addon` WHERE `entry` IN (55213,55539);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('55213', '4', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', '103245'),
('55539', '0', '0', '0', '3', '0', '1', '0', '0', '0', '0', '0', '0', '0', '80797 42386');

DELETE FROM `waypoint_path` WHERE `PathId` IN (4);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(4,1,0,'Spawned Shu quest');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (4);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
('4', '0', '634.3', '3126.2', '88.7482'),
('4', '1', '642.205', '3124.58', '89.3339'),
('4', '2', '649.877', '3126.44', '89.808'),
('4', '3', '655.521', '3131.44', '89.3411'),
('4', '4', '660.047', '3135.83', '88.807'),
('4', '5', '665.788', '3136.77', '87.8995');