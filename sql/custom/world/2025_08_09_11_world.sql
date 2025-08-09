-- NPC: 64554 Zhao-Ren <The Onyx Serpent>
-- Area: 8276 Temple Spire spawn Zhao-ren


DELETE FROM `areatrigger_scripts` WHERE `entry` IN (8276);
INSERT INTO `areatrigger_scripts` VALUES
(8276, 'at_temple_of_five_dawns_summon_zhaoren');

DELETE FROM `creature_template_addon` WHERE `entry` IN (64554);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('64554', '15', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '5', '');

DELETE FROM `creature` WHERE (`guid` = '451413');
DELETE FROM `creature` WHERE (`guid` = '451478');

UPDATE `creature_template` SET `movementType` = 2, `movementId` = 106, `speed_walk` = '4', `speed_run` = '6' WHERE `Entry` = 64554;

-- these flags disable gravity for Zhao so it can fly above
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912', `StaticFlags5` = 5 WHERE (`Entry` = '64554') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912', `StaticFlags5` = 5 WHERE (`Entry` = '64554') and (`DifficultyID` = '1');

DELETE FROM `waypoint_path` WHERE `PathId` IN (15);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(15,1,0,'Zhao-Ren at temple spire');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (15);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
('15', '0', '780.884', '4174.23', '303.268'),
('15', '1', '831.736', '4009.06', '253.521'),
('15', '2', '845.007', '3905.53', '248.783'),
('15', '3', '841.078', '3822.78', '261.598'),
('15', '4', '792.554', '3761.06', '270.393'),
('15', '5', '705.901', '3709.15', '293.381'),
('15', '6', '611.443', '3720.79', '321.788'),
('15', '7', '554.779', '3809.91', '327.349'),
('15', '8', '574.174', '3940.24', '331.119'),
('15', '9', '599.104', '4099.64', '319.644'),
('15', '10', '626.389', '4260.89', '305.464'),
('15', '11', '617.488', '4362.79', '288.137'),
('15', '12', '650.88', '4468.45', '288.137');
