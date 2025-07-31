-- NPC: 55020 Old Man Liang
-- NPC: 54975 Aysa Cloudsinger

UPDATE `creature_template_addon` SET `auras` = '0' WHERE (`entry` = '55020'); 

DELETE FROM `creature_addon` WHERE `guid` IN (451049,451022,450772,451092,451042,451091);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 

('451022', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80797'), -- Old Man Liang at his house
('450772', '0', '0', '0', '1', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80797'), -- Aysa at the pool

('451092', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '85096'), -- Old Man Liang at the pool2
('451091', '0', '0', '0', '1', '0', '1', '1', '0', '0', '0', '0', '0', '0', '85096'), -- Aysa at the pool2

('451049', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82358'), -- Old Man Liang at the tower
('451042', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82358'); -- Aysa at the tower