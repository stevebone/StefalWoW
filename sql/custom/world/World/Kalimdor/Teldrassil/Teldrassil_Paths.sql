-- Teldrassil Waypoint Paths

UPDATE `creature` SET `MovementType` = 2 WHERE `guid` IN (311235);

DELETE FROM `creature_addon` WHERE `guid` IN (311235);
INSERT INTO `creature_addon` (`guid`, `PathId`) VALUES
(311235, 4862300);

DELETE FROM `waypoint_path` WHERE `PathId` IN (4862300);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Velocity`, `Comment`) VALUES
(4862300, 0, 0, NULL, 'Teldrassil - Wisp');

UPDATE `waypoint_path_node` SET `PathId` = 4862300 WHERE `PathId` = 3568;