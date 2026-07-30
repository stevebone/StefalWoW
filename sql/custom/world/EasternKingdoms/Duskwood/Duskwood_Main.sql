-- Duskwood Fixes

-- Wrong Creature Spawns
DELETE FROM `creature` WHERE `guid` = 317602 AND `id` = 494;
DELETE FROM `creature` WHERE `guid` = 317599 AND `id` = 228;

-- New Creature Spawns
SET @CGUID := 900000;
DELETE FROM `creature` WHERE `guid` = @CGUID+1246;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `phaseId`, `phaseGroup`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`) VALUES
-- Marus <The Pack Leader>
(@CGUID+1246, 45771, 0, 10, 241, 0, 0, 0, 0, 1, -11065.4, -778.968, 63.8591, 5.47845, 3600);

-- Creature Spawn Fixes
UPDATE creature
SET MovementType = 1,
    wander_distance = 8
WHERE id IN (
    889,212,628,44016,44020,210,604,948,43923,45614,
    43732,44089,44087,533,202,930,898,205,45517,217,
    48,203,206,920,3
);

-- Creature Spawn Fixes
UPDATE creature
SET MovementType = 1,
    wander_distance = 3
WHERE id IN (
    494,228,886
);
