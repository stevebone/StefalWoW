-- Northern Barrens Fixes

-- NPC: 5837 Stonearm
-- NPC: 3295 Sludge Anomaly
-- NPC: 5841 Rocklance
-- NPC: 5828 Humar the Pridelord
-- NPC: 5835 Foreman Grills
-- NPC: 5836 Engineer Whirleygig
-- NPC: 3672 Boahn <Druid of the Fang>
-- NPC: 5831 Swiftmane
-- NPC: 3652 Trigore the Lasher
-- NPC: 3270 Elder Mystic Razorsnout
-- NPC: 38554 Dead Taurajo Refugee

-- Creature Difficulty
DELETE FROM `creature_template_difficulty` WHERE Entry IN (5837,3295,5841,5828,5835,5836,3672,5831,3270) AND `DifficultyID` = 0;
UPDATE `creature_template_difficulty` SET `DifficultyID` = 0 WHERE Entry IN (5837,3295,5841,5828,5835,5836,3672,5831,3270) AND `DifficultyID` = 1;

UPDATE creature_template_difficulty SET ContentTuningID = 3 WHERE Entry = 3652;

-- Template Fixes
UPDATE `creature_template` SET `unit_flags3` = 8193 WHERE `Entry` = 38554;
UPDATE creature_template SET faction = 48 WHERE entry = 34640; -- hostile faction

-- Template Addons
DELETE FROM `creature_template_addon` WHERE `Entry` = 38554;
INSERT INTO `creature_template_addon` (`Entry`, `StandState`) VALUES
(38554, 7);

-- New spawns
SET @CGUID := 900000;
DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID+3151 AND @CGUID+3161;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `MovementType`) VALUES 
(@CGUID+3151, 5838, 1, 17, 17, 1, -1095.17, -2824.77, 93.7668, 4.7331, 3600, 8, 1),
(@CGUID+3152, 5838, 1, 17, 17, 1, -1393.59, -2763.22, 91.7401, 5.0375, 3600, 8, 1),
(@CGUID+3153, 5838, 1, 17, 388, 1, -1333.14, -3122.84, 91.6667, 4.73373, 3600, 8, 1),

(@CGUID+3154, 5831, 1, 17, 17, 0, -685.513, -3594.81, 91.914, 6.23073, 3600, 15, 1),

(@CGUID+3155, 3652, 1, 17, 17, 0, -621.612, -2270.82, 18.8419, 2.67112, 3600, 3, 1),
(@CGUID+3156, 3652, 1, 17, 6511, 0, -639.188, -2255.07, 13.244, 2.61535, 3600, 3, 1),
(@CGUID+3157, 3652, 1, 17, 6511, 0, -637.609, -2256.01, 13.244, 6.10475, 3600, 3, 1),

(@CGUID+3158, 5830, 1, 17, 383, 0, 574.006, -1392, 92.39, 1.638, 3600, 15, 1),
(@CGUID+3159, 5830, 1, 17, 383, 0, 517.216, -1501.07, 94.5103, 3.22689, 3600, 15, 1),
(@CGUID+3160, 5830, 1, 17, 383, 0, 726.5, -1312.93, 92.2256, 2.82053, 3600, 15, 1),
(@CGUID+3161, 5830, 1, 17, 383, 0, 526.462, -1264.57, 91.668, 5.38044, 3600, 15, 1);

-- Spawn Pools
SET @POOLID := 900000;
DELETE FROM `pool_template` WHERE `entry` BETWEEN @POOLID+14 AND @POOLID+16;
INSERT INTO `pool_template` (`entry`, `max_limit`, `description`) VALUES 
(@POOLID+14, 1, 'Northern Barrens - Brokespear (entry 5838)'),
(@POOLID+15, 1, 'Northern Barrens - Trigore the Lasher (entry 3652)'),
(@POOLID+16, 1, 'Northern Barrens - Sister Rathtalon (entry 5830)');

DELETE FROM `pool_members` WHERE `poolSpawnId` BETWEEN @POOLID+14 AND @POOLID+16;
INSERT INTO `pool_members` (`type`, `spawnId`, `poolSpawnId`, `chance`, `description`) VALUES 
(0, @CGUID+3151, @POOLID+14, 0, 'Northern Barrens - Brokespear (entry 5838)'), 
(0, @CGUID+3152, @POOLID+14, 0, 'Northern Barrens - Brokespear (entry 5838)'), 
(0, @CGUID+3153, @POOLID+14, 0, 'Northern Barrens - Brokespear (entry 5838)'), 

(0, @CGUID+3155, @POOLID+15, 0, 'Northern Barrens - Trigore the Lasher (entry 3652)'), 
(0, @CGUID+3156, @POOLID+15, 0, 'Northern Barrens - Trigore the Lasher (entry 3652)'), 
(0, @CGUID+3157, @POOLID+15, 0, 'Northern Barrens - Trigore the Lasher (entry 3652)'),

(0, @CGUID+3158, @POOLID+16, 0, 'Northern Barrens - Sister Rathtalon (entry 5830)'), 
(0, @CGUID+3159, @POOLID+16, 0, 'Northern Barrens - Sister Rathtalon (entry 5830)'), 
(0, @CGUID+3160, @POOLID+16, 0, 'Northern Barrens - Sister Rathtalon (entry 5830)'), 
(0, @CGUID+3161, @POOLID+16, 0, 'Northern Barrens - Sister Rathtalon (entry 5830)');

-- Creature updates
UPDATE `creature` SET `spawntimesecs` = 3600 WHERE `id` = 5835;

UPDATE creature
SET MovementType = 1,
    wander_distance = 8
WHERE id IN (
    3274, 3397, 3632, 3634, 20797, 3835, 3278, 3276, 3277, 4127,
	3273, 3272, 4127, 3234, 3285, 3282, 3284, 52357, 3452, 34576,
	3426
);

-- Paths

UPDATE `creature` SET `MovementType` = 2 WHERE `guid` = 303533;

DELETE FROM `creature_addon` WHERE `guid` IN (303533);
INSERT INTO `creature_addon` (`guid`, `PathId`) VALUES (303533, 583500);

DELETE FROM `waypoint_path` WHERE `PathId` = 583500;
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES 
(583500, 0, 0, 'Northern Barrens - Foreman Grills (entry 5835)');

DELETE FROM `waypoint_path_node` WHERE `PathId` = 583500;
INSERT INTO waypoint_path_node (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `Delay`) VALUES
(583500, 1, 1062.0583, -3087.0090, 105.1254, 1.8063, 0),
(583500, 2, 1058.4043, -3077.0347, 105.2611, 2.1197, 0),
(583500, 3, 1050.2454, -3072.1628, 105.1250, 3.0221, 0),
(583500, 4, 1042.6886, -3072.9700, 105.8323, 3.7289, 0),
(583500, 5, 1033.2261, -3077.6335, 105.1252, 3.4933, 0),
(583500, 6, 1028.6377, -3070.9653, 105.1252, 1.7505, 0),
(583500, 7, 1030.1234, -3064.8872, 104.9982, 1.2793, 0),
(583500, 8, 1032.2015, -3057.9614, 101.0607, 1.2793, 0),
(583500, 9, 1036.0170, -3051.3350, 98.7935, 0.7327, 0),
(583500, 10, 1039.5625, -3054.3618, 98.3049, 5.5746, 0),
(583500, 11, 1047.9338, -3057.1824, 94.4371, 5.8888, 0),
(583500, 12, 1054.2332, -3062.5337, 91.8595, 5.3021, 0),
(583500, 13, 1056.4337, -3070.8804, 91.7589, 4.9604, 0),
(583500, 14, 1059.3556, -3082.4160, 91.7455, 4.9604, 0),
(583500, 15, 1061.1584, -3090.1321, 91.7455, 3.8091, 0),
(583500, 16, 1051.9489, -3093.9277, 87.5551, 3.4509, 0),
(583500, 17, 1042.1445, -3098.5186, 81.0501, 3.5687, 0),
(583500, 18, 1033.5245, -3101.8982, 80.7440, 3.5687, 0),
(583500, 19, 1041.7455, -3097.6250, 81.0082, 0.4028, 0),
(583500, 20, 1049.9049, -3094.1487, 86.3586, 0.4028, 0),
(583500, 21, 1057.3108, -3090.9934, 91.1242, 0.4028, 0),
(583500, 22, 1060.6167, -3083.3765, 91.7454, 1.7764, 0),
(583500, 23, 1054.1158, -3071.3237, 91.7626, 2.9545, 0),
(583500, 24, 1043.1576, -3075.1890, 91.7343, 3.5836, 0),
(583500, 25, 1026.8083, -3081.9939, 91.7343, 3.4658, 0),
(583500, 26, 1039.0289, -3074.5139, 91.7343, 0.5834, 0),
(583500, 27, 1049.0547, -3067.8965, 91.7660, 0.5834, 0),
(583500, 28, 1051.5635, -3060.3057, 93.0695, 1.7215, 0),
(583500, 29, 1044.0995, -3054.2937, 96.8191, 2.7818, 0),
(583500, 30, 1036.1587, -3053.6028, 98.8376, 3.8020, 0),
(583500, 31, 1031.1947, -3061.5789, 103.0287, 4.3156, 0),
(583500, 32, 1030.6305, -3069.0747, 105.1251, 5.0225, 0),
(583500, 33, 1034.3976, -3078.7253, 105.1251, 5.0626, 0),
(583500, 34, 1038.1198, -3088.9170, 104.9407, 5.0626, 0),
(583500, 35, 1043.9911, -3095.8867, 105.4453, 5.8118, 0),
(583500, 36, 1050.5968, -3097.0237, 105.6445, 0.3156, 0),
(583500, 37, 1058.4193, -3091.8806, 105.4149, 1.0649, 0);
