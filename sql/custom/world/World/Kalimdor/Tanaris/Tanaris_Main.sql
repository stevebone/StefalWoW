-- Tanaris Fixes

-- NPC: 8207 Emberwind
-- NPC: 47386 Ainamiss the Hive Queen
-- NPC: 44759 Andre Firebeard
-- NPC: 44761 Aquementas the Unchained
-- NPC: 44750 Caliph Scorpidsting
-- NPC: 44714 Fronkle the Disturbed
-- NPC: 47387 Harakiss the Infestor
-- NPC: 44767 Occulus the Corrupted
-- NPC: 39183 Scorpitar
-- NPC: 39185 Slaverjaw

-- Template fixes
UPDATE creature_template SET faction = 7 WHERE entry = 227795;

-- Difficulty fixes
-- Missing Content Tuning
UPDATE creature_template_difficulty SET `ContentTuningID` = 28 WHERE `Entry` = 47386 AND `DifficultyID` = 0;
UPDATE creature_template_difficulty SET `ContentTuningID` = 28 WHERE `Entry` = 47387 AND `DifficultyID` = 0;
UPDATE creature_template_difficulty SET `ContentTuningID` = 28, `LootID` = 44759, `GoldMin` = 1000, `GoldMax` = 2000 WHERE `Entry` = 44759 AND `DifficultyID` = 0;

UPDATE `creature_template_difficulty` SET `LootID` = 44761, StaticFlags1 = StaticFlags1 | 0x10000000 WHERE `Entry` = 44761 AND `DifficultyID` = 0;
UPDATE `creature_template_difficulty` SET `ContentTuningID` = 28, `LootID` = 44750, StaticFlags1 = StaticFlags1 | 0x10000000 WHERE `Entry` = 44750 AND `DifficultyID` = 0;
UPDATE `creature_template_difficulty` SET `ContentTuningID` = 28, `LootID` = 8207 WHERE `Entry` = 8207 AND `DifficultyID` = 0;
UPDATE `creature_template_difficulty` SET `LootID` = 44714, StaticFlags1 = StaticFlags1 | 0x10000000, `GoldMin` = 1000, `GoldMax` = 2000 WHERE `Entry` = 44714 AND `DifficultyID` = 0;
UPDATE `creature_template_difficulty` SET `ContentTuningID` = 28, `LootID` = 44767 WHERE `Entry` = 44767 AND `DifficultyID` = 0;
UPDATE `creature_template_difficulty` SET `ContentTuningID` = 28, `LootID` = 39183 WHERE `Entry` = 39183 AND `DifficultyID` = 0;
UPDATE `creature_template_difficulty` SET `ContentTuningID` = 28, `LootID` = 39185 WHERE `Entry` = 39185 AND `DifficultyID` = 0;

-- Flight flag
UPDATE creature_template_difficulty SET StaticFlags1 = StaticFlags1 | 0x20000000 WHERE Entry IN (44767);

-- Swim flag
UPDATE creature_template_difficulty SET StaticFlags1 = StaticFlags1 | 0x10000000 WHERE Entry IN (7855,7858,14123,39020);

-- Creature Equipment
DELETE FROM `creature_equip_template` WHERE `CreatureID` IN (224049,224035) AND `ID` = 1 AND `ItemID1` = 38632;
DELETE FROM `creature_equip_template` WHERE `CreatureID` = 40583 AND `ID` = 1 AND `ItemID1` = 2147;
INSERT INTO `creature_equip_template` (`CreatureID`, `ID`, `ItemID1`, `AppearanceModID1`, `ItemVisual1`,  `ItemID2`, `AppearanceModID2`, `ItemVisual2`,  `ItemID3`, `AppearanceModID3`, `ItemVisual3`, `VerifiedBuild`) VALUES 
(224049, 1, 38632, 0, 0, 0, 0, 0, 0, 0, 0, 67186),
(224035, 1, 38632, 0, 0, 0, 0, 0, 0, 0, 0, 67186),
(40583, 1, 2147, 0, 0, 0, 0, 0, 0, 0, 0, 67186);

-- Template Addons
DELETE FROM `creature_template_addon` WHERE `Entry` IN (227257);
INSERT INTO `creature_template_addon` (`Entry`, `StandState`) VALUES 
(227257, 1);


-- New Spawns
SET @CGUID := 900000;
DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID+3176 AND @CGUID+3201;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `MovementType`) VALUES 
(@CGUID+3176, 224049, 1, 440, 2300, 1, -8224.15, -4609.47, 10.4682, 0.407393, 180, 0, 0),
(@CGUID+3177, 224035, 1, 440, 2300, 1, -8218.33, -4614.39, 10.4194, 1.15403, 180, 0, 0),
(@CGUID+3178, 227257, 1, 1519, 10523, 0, -7164.8, -3379.98, 9.76389, 4.11326, 120, 0, 0),
(@CGUID+3179, 44750, 1, 440, 984, 1, -8908.15, -3451.35, 12.227, 2.20501, 3600, 3, 1),

(@CGUID+3180, 8204, 1, 440, 982, 0, -7721.94, -2599.2, -58.1597, 2.61717, 3600, 3, 1),
(@CGUID+3181, 8204, 1, 440, 982, 0, -7990.38, -2590.1, -45.2702, 1.25433, 3600, 3, 1),

(@CGUID+3182, 8205, 1, 440, 981, 0, -8985.45, -4145.46, -31.2654, 1.46914, 3600, 3, 1),
(@CGUID+3183, 8205, 1, 440, 981, 0, -9064.72, -4143.17, -29.9676, 2.87498, 3600, 3, 1),

(@CGUID+3184, 44767, 1, 440, 2300, 0, -8197.52, -4462.81, 17.5755, 4.74729, 3600, 15, 1),

(@CGUID+3185, 39183, 1, 440, 440, 0, -8591.21, -3639.23, 14.4425, 0.456948, 3600, 15, 1),

(@CGUID+3186, 44759, 1, 440, 1940, 1, -8515.19, -5075.47, 14.1935, 0.898602, 3600, 3, 1),

(@CGUID+3187, 44761, 1, 440, 1336, 0, -8174.38, -5080.22, 14.1584, 0.896055, 3600, 3, 1),

(@CGUID+3188, 44722, 1, 440, 977, 1, -6725.08, -4719.96, 14.3175, 1.04159, 3600, 3, 1),

(@CGUID+3189, 8207, 1, 440, 440, 0, -7641, -3873, 10.05, 1.963, 3600, 9, 1),
(@CGUID+3190, 8207, 1, 440, 1939, 0, -7606, -3023, 19.96, 0.984, 3600, 9, 1),
(@CGUID+3191, 8207, 1, 440, 1939, 0, -7558, -3546, 16.586, 2.195, 3600, 9, 1),
(@CGUID+3192, 8207, 1, 440, 440, 0, -7485, -3756, 11.398, 5.928, 3600, 9, 1),
(@CGUID+3193, 8207, 1, 440, 1939, 0, -7468, -3185, 13.995, 3.645, 3600, 7, 1),

(@CGUID+3194, 8200, 1, 440, 979, 1, -7220, -3015, 8.796, 2.374, 3600, 0, 0),
(@CGUID+3195, 8200, 1, 440, 979, 1, -7022.87, -2749.89, 11.991, 1.41769, 3600, 3, 1),

(@CGUID+3196, 8208, 1, 440, 440, 0, -7059, -3449, 12.664, 4.051, 3600, 15, 1),

(@CGUID+3197, 8201, 1, 440, 992, 1, -9037.34, -3192.4, 51.7767, 5.49653, 3600, 0, 0),
(@CGUID+3198, 8201, 1, 440, 984, 1, -8761.82, -3342.01, 11.1406, 1.19248, 3600, 0, 0),
(@CGUID+3199, 8201, 1, 440, 983, 1, -8450.21, -2816.06, 8.84061, 5.66118, 3600, 3, 1),

(@CGUID+3200, 47387, 1, 440, 981, 0, -9158.65, -3877.39, 11.1267, 0.0184154, 3600, 15, 1),
(@CGUID+3201, 47387, 1, 440, 981, 0, -9291.84, -3672.71, 10.0494, 2.67316, 3600, 15, 1);



-- Spawn Pools
SET @POOLID := 900000;
DELETE FROM `pool_template` WHERE `entry` BETWEEN @POOLID+18 AND @POOLID+23;
INSERT INTO `pool_template` (`entry`, `max_limit`, `description`) VALUES 
(@POOLID+18, 1, 'Tanaris - Soriid the Devourer (entry 8204)'),
(@POOLID+19, 1, 'Tanaris - Haarka the Ravenous (entry 8205)'),
(@POOLID+20, 1, 'Tanaris - Emberwing (entry 8207)'),
(@POOLID+21, 1, 'Tanaris - Jin''Zallah the Sandbringer (entry 8200)'),
(@POOLID+22, 1, 'Tanaris - Omgorn the Lost (entry 8201)'),
(@POOLID+23, 1, 'Tanaris - Harakiss the Infestor (entry 47387)');

DELETE FROM `pool_members` WHERE `poolSpawnId` BETWEEN @POOLID+18 AND @POOLID+23;
INSERT INTO `pool_members` (`type`, `spawnId`, `poolSpawnId`, `chance`, `description`) VALUES 
(0, @CGUID+3180, @POOLID+18, 0, 'Tanaris - Soriid the Devourer (entry 8204)'), 
(0, @CGUID+3181, @POOLID+18, 0, 'Tanaris - Soriid the Devourer (entry 8204)'), 

(0, @CGUID+3182, @POOLID+19, 0, 'Tanaris - Haarka the Ravenous (entry 8205)'), 
(0, @CGUID+3183, @POOLID+19, 0, 'Tanaris - Haarka the Ravenous (entry 8205)'),

(0, @CGUID+3189, @POOLID+20, 0, 'Tanaris - Emberwing (entry 8207)'), 
(0, @CGUID+3190, @POOLID+20, 0, 'Tanaris - Emberwing (entry 8207)'), 
(0, @CGUID+3191, @POOLID+20, 0, 'Tanaris - Emberwing (entry 8207)'), 
(0, @CGUID+3192, @POOLID+20, 0, 'Tanaris - Emberwing (entry 8207)'), 
(0, @CGUID+3193, @POOLID+20, 0, 'Tanaris - Emberwing (entry 8207)'),

(0, @CGUID+3194, @POOLID+21, 0, 'Jin''Zallah the Sandbringer (entry 8200)'), 
(0, @CGUID+3195, @POOLID+21, 0, 'Jin''Zallah the Sandbringer (entry 8200)'),

(0, @CGUID+3197, @POOLID+22, 0, 'Tanaris - Omgorn the Lost (entry 8201)'), 
(0, @CGUID+3198, @POOLID+22, 0, 'Tanaris - Omgorn the Lost (entry 8201)'), 
(0, @CGUID+3199, @POOLID+22, 0, 'Tanaris - Omgorn the Lost (entry 8201)'),

(0, @CGUID+3200, @POOLID+23, 0, 'Harakiss the Infestor (entry 47387)'), 
(0, @CGUID+3201, @POOLID+23, 0, 'Harakiss the Infestor (entry 47387)'), 
(0, 271819, 	 @POOLID+23, 0, 'Harakiss the Infestor (entry 47387)');


-- Creature Fixes
-- Equipment
UPDATE creature SET equipment_id = 1 WHERE id = 40583;

UPDATE `creature` SET `spawntimesecs` = 3600, `wander_distance` = 15, `MovementType` = 1 WHERE `guid` = 271819;


UPDATE creature SET MovementType = 1, wander_distance = 8 WHERE id IN (
    5429, 8667, 5419, 5429, 39022, 5455, 5647, 5645, 5645, 40717, 5426, 5465,
	40657, 5646, 5460, 5471, 5472, 5420, 5427, 44611, 44613, 44612, 5450, 5451, 
	5473, 5430, 44594, 44599, 44595, 38914, 38998, 38997, 40635, 40632, 44587, 
	49833, 48686, 38646, 38719, 38648, 38649, 14123, 40527, 44880, 5452, 5431
);

UPDATE creature SET MovementType = 1, wander_distance = 8 WHERE guid IN (
	271348, 271346, 271307, 271309, 271308, 271316, 271296, 271291, 271288, 
	271283, 271318, 271293, 271289, 271286, 271313, 271293
);

-- Some spawns need to be fixed in place
UPDATE creature SET MovementType = 0, wander_distance = 0 WHERE guid IN (272850,272849,272856,272855);

-- Creature Addons

DELETE FROM `creature_addon` WHERE `guid` IN (271390,271391);
INSERT INTO `creature_addon` (`guid`, `StandState`) VALUES 
(271390, 2),
(271391, 2);
