-- Thousand Needles Fixes

-- NPC: 48147 Sparkleshell Tortoise
-- NPC: 48148 Sparkleshell Snapper

-- Template Fixes
UPDATE `creature_template` SET `unit_flags` = 537133824, `unit_flags2` = 2049, unit_flags3 = 8192 WHERE `entry` IN (48147,48148,47485);

UPDATE creature_template_difficulty SET StaticFlags1 = StaticFlags1 | 0x20000000 WHERE Entry IN (48147,48148,48155,48104,41097,48188,48166);
UPDATE creature_template_difficulty SET StaticFlags1 = StaticFlags1 | 0x10000000 WHERE Entry IN (48188,50741);

-- Creature Fixes

-- New Spawns
SET @CGUID := 900000;
DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID+3162 AND @CGUID+3175;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `MovementType`) VALUES 
(@CGUID+3162, 4132, 1, 400, 5591, 0, -6497, -3330, -95, 4.851, 3600, 9, 1),
(@CGUID+3163, 4132, 1, 400, 5591, 0, -6494, -3531, -70, 5.936, 3600, 7, 1),
(@CGUID+3164, 4132, 1, 400, 400, 0, -6492, -3242, -112, 4.511, 3600, 9, 1),
(@CGUID+3165, 4132, 1, 400, 5591, 0, -6382, -3444, -80, 4.419, 3600, 7, 1),
(@CGUID+3166, 4132, 1, 400, 5591, 0, -6484.92, -3498.92, -69.9512, 2.42601, 3600, 3, 1),

(@CGUID+3167, 51008, 1, 400, 400, 0, -6757.32, -3533.81, 86.1124, 0.650301, 3600, 15, 1),
(@CGUID+3168, 51001, 1, 400, 400, 0, -6780.33, -4012.47, 88.4456, 6.17703,  3600, 15, 1),
(@CGUID+3169, 50727, 1, 400, 5029, 0, -6359.26, -4599.72, 80.1442, 2.65946, 3600, 0, 0),
(@CGUID+3170, 50785, 1, 400, 5027, 0, -5663.11, -4572.62, 97.2865, 3.52123, 3600, 8, 1),
(@CGUID+3171, 50329, 1, 400, 5027, 0, -5135.49, -4369.99, 129.468, 3.63929, 3600, 8, 1),
(@CGUID+3172, 50892, 1, 400, 400, 0, -5212.05, -2859.67, 86.8517, 3.05719, 3600, 8, 1),
(@CGUID+3173, 50741, 1, 400, 400, 0, -5613.2099, -2086.8769, -63.894, 5.8677, 3600, 0, 0),
(@CGUID+3174, 50748, 1, 400, 481, 0, -5124.98, -2386, 0.701966, 2.39732, 3600, 0, 0),
(@CGUID+3175, 50952, 1, 400, 481, 0, -5009.26, -2224.02, -53.2817, 3.82609, 3600, 0, 0);

-- Spawn Pools
SET @POOLID := 900000;
DELETE FROM `pool_template` WHERE `entry` BETWEEN @POOLID+17 AND @POOLID+17;
INSERT INTO `pool_template` (`entry`, `max_limit`, `description`) VALUES 
(@POOLID+17, 1, 'Thousand Needles - Krkk''kx (entry 4132)');

DELETE FROM `pool_members` WHERE `poolSpawnId` BETWEEN @POOLID+17 AND @POOLID+17;
INSERT INTO `pool_members` (`type`, `spawnId`, `poolSpawnId`, `chance`, `description`) VALUES 
(0, @CGUID+3162, @POOLID+17, 0, 'Thousand Needles - Krkk''kx (entry 4132)'), 
(0, @CGUID+3163, @POOLID+17, 0, 'Thousand Needles - Krkk''kx (entry 4132)'), 
(0, @CGUID+3164, @POOLID+17, 0, 'Thousand Needles - Krkk''kx (entry 4132)'), 
(0, @CGUID+3165, @POOLID+17, 0, 'Thousand Needles - Krkk''kx (entry 4132)'), 
(0, @CGUID+3166, @POOLID+17, 0, 'Thousand Needles - Krkk''kx (entry 4132)');

-- creature addons
DELETE FROM `creature_addon` WHERE `guid` IN (331350,331354,331349);
INSERT INTO `creature_addon` (`guid`, `emote`) VALUES 
(331350, 379),
(331354, 379),
(331349, 379);

-- Position Fixes
UPDATE creature SET position_x = -4858.45, position_y = -2243.861, position_z = 59.14572, orientation = 3.0353801 WHERE guid = 329571;
UPDATE creature SET position_x = -4853.38, position_y = -2256.1743, position_z = 63.83853, orientation = 2.6902587 WHERE guid = 298332;
UPDATE creature SET position_x = -4864.673, position_y = -2262.3347, position_z = 61.8547, orientation = 1.9730656 WHERE guid = 298331;
UPDATE creature SET position_x = -4877.001, position_y = -2261.8538, position_z = 58.553143, orientation = 2.1571798 WHERE guid = 329576;

UPDATE creature
SET MovementType = 1,
    wander_distance = 8
WHERE id IN (
    4117, 4118, 48155, 45385, 45383, 45381, 7874, 7872, 7873, 48128, 48188, 48192,
	48179, 48130, 48138, 48137, 48177, 48178, 47481, 48191, 48131, 48132, 40062,
	40061, 40063, 40959, 15476, 40432
);

UPDATE creature
SET MovementType = 1,
    wander_distance = 3
WHERE id IN (
    48128
);
