-- Ghostlands (Burning Crusade) Fixes

-- NPC: 22062 Dr. Whitherlimb

-- New Spawns
SET @CGUID := 900000;
DELETE FROM `creature` WHERE `guid` IN (@CGUID+3147,@CGUID+3148,@CGUID+3149,@CGUID+3150);
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `phaseId`, `phaseGroup`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `MovementType`) VALUES 
(@CGUID+3147, 22062, 530, 3433, 3500, 0, 0, 0, 0, 0, 6332.93, -6269.96, 80.8146, 5.58654, 3600, 5, 1),
(@CGUID+3148, 22062, 530, 3433, 3500, 0, 0, 0, 0, 0, 6328.94, -6442.69, 86.0114, 0.795612, 3600, 5, 1),
(@CGUID+3149, 22062, 530, 3433, 3501, 0, 0, 0, 0, 0, 7188.17, -6417.2, 59.1657, 3.23113, 3600, 5, 5, 1),
(@CGUID+3150, 22062, 530, 3433, 3502, 0, 0, 0, 0, 0, 7200.36, -6623.17, 63.6589, 6.01929, 3600, 5, 5, 1);

-- Spawn Pools
SET @POOLID := 900000;
INSERT INTO pool_template (entry, max_limit, description) VALUES 
(@POOLID+13, 1, 'Ghostlands - Dr. Whitherlimb (entry 22062)');

INSERT INTO pool_members (`type`, `spawnId`, `poolSpawnId`, `chance`, `description`) VALUES 
(0, @CGUID+3147, @POOLID+13, 0, 'Ghostlands - Dr. Whitherlimb (entry 22062)'), 
(0, @CGUID+3148, @POOLID+13, 0, 'Ghostlands - Dr. Whitherlimb (entry 22062)'), 
(0, @CGUID+3149, @POOLID+13, 0, 'Ghostlands - Dr. Whitherlimb (entry 22062)'), 
(0, @CGUID+3150, @POOLID+13, 0, 'Ghostlands - Dr. Whitherlimb (entry 22062)');

