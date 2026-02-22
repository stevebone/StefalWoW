-- Spawn wild battle pets in Elwynn Forest and Stormwind for testing
-- Uses impossible date (2015_13_32) per project conventions to avoid upstream conflicts
-- Battle pet creature templates already exist with npc_flag & 0x40000000

-- Species and their creature_template entries:
-- 60649 = Black Lamb       (Beast)
-- 61071 = Small Frog       (Aquatic)
-- 61080 = Rabbit           (Critter)
-- 61081 = Squirrel         (Critter)
-- 61142 = Snake            (Beast)
-- 61165 = Fawn             (Critter)
-- 61327 = Spider           (Beast)
-- 62019 = Cat              (Beast)
-- 62664 = Chicken          (Flying)
-- 62954 = Stormwind Rat    (Critter)

SET @GUID := 900000;

-- ============================================================================
-- Elwynn Forest - Goldshire area (around the inn and farms)
-- ============================================================================

-- Black Lamb (60649) - pastures near Goldshire
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `terrainSwapMap`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curHealthPct`, `MovementType`) VALUES
(@GUID+0,  60649, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9454.52, 68.14, 56.32, 3.14, 300, 10, 0, 100, 1),
(@GUID+1,  60649, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9430.11, 101.22, 57.45, 1.57, 300, 10, 0, 100, 1),
(@GUID+2,  60649, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9478.33, 120.55, 56.88, 5.23, 300, 10, 0, 100, 1),
(@GUID+3,  60649, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9503.77, 55.91, 56.12, 0.78, 300, 10, 0, 100, 1);

-- Small Frog (61071) - near streams and ponds
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `terrainSwapMap`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curHealthPct`, `MovementType`) VALUES
(@GUID+4,  61071, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9412.63, 406.87, 48.22, 2.44, 300, 8, 0, 100, 1),
(@GUID+5,  61071, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9449.18, 462.33, 49.80, 4.71, 300, 8, 0, 100, 1),
(@GUID+6,  61071, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9380.22, 440.11, 47.55, 1.20, 300, 8, 0, 100, 1);

-- Fawn (61165) - forest areas
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `terrainSwapMap`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curHealthPct`, `MovementType`) VALUES
(@GUID+7,  61165, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9560.80, 612.49, 45.08, 0.52, 300, 12, 0, 100, 1),
(@GUID+8,  61165, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9600.44, 580.33, 42.11, 3.92, 300, 12, 0, 100, 1),
(@GUID+9,  61165, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9634.22, 766.55, 34.88, 5.50, 300, 12, 0, 100, 1),
(@GUID+10, 61165, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9510.11, 700.22, 38.44, 2.10, 300, 12, 0, 100, 1);

-- Spider (61327) - wooded/darker areas
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `terrainSwapMap`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curHealthPct`, `MovementType`) VALUES
(@GUID+11, 61327, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9481.55, 469.33, 52.10, 4.18, 300, 8, 0, 100, 1),
(@GUID+12, 61327, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9520.11, 500.88, 48.90, 1.95, 300, 8, 0, 100, 1),
(@GUID+13, 61327, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9550.66, 450.22, 50.33, 0.30, 300, 8, 0, 100, 1);

-- Chicken (62664) - near farms and Goldshire
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `terrainSwapMap`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curHealthPct`, `MovementType`) VALUES
(@GUID+14, 62664, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9461.41, 49.14, 56.64, 2.80, 300, 6, 0, 100, 1),
(@GUID+15, 62664, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9471.91, 51.67, 57.15, 0.44, 300, 6, 0, 100, 1),
(@GUID+16, 62664, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9440.33, 80.22, 56.90, 5.88, 300, 6, 0, 100, 1);

-- Additional Rabbit (61080) spawns near Goldshire
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `terrainSwapMap`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curHealthPct`, `MovementType`) VALUES
(@GUID+17, 61080, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9500.22, 150.44, 56.10, 1.22, 300, 10, 0, 100, 1),
(@GUID+18, 61080, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9420.88, 200.33, 60.55, 3.66, 300, 10, 0, 100, 1);

-- Additional Squirrel (61081) spawns in forest
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `terrainSwapMap`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curHealthPct`, `MovementType`) VALUES
(@GUID+19, 61081, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9696.56, 796.09, 30.43, 4.10, 300, 10, 0, 100, 1),
(@GUID+20, 61081, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9580.33, 650.11, 43.88, 2.55, 300, 10, 0, 100, 1);

-- Cat (62019) - near Goldshire buildings
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `terrainSwapMap`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curHealthPct`, `MovementType`) VALUES
(@GUID+21, 62019, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9465.22, 35.88, 56.44, 0.90, 300, 5, 0, 100, 1),
(@GUID+22, 62019, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9445.11, 65.33, 56.80, 4.55, 300, 5, 0, 100, 1);

-- Snake (61142) - grasslands
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `terrainSwapMap`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curHealthPct`, `MovementType`) VALUES
(@GUID+23, 61142, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9530.44, 300.22, 52.66, 3.33, 300, 10, 0, 100, 1),
(@GUID+24, 61142, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9490.88, 350.11, 50.44, 5.10, 300, 10, 0, 100, 1);

-- ============================================================================
-- Stormwind City - Stormwind Rat
-- ============================================================================

-- Stormwind Rat (62954) - in Stormwind alleys, canals, trade district
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `terrainSwapMap`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curHealthPct`, `MovementType`) VALUES
(@GUID+25, 62954, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9735.33, 95.11, 14.25, 2.10, 300, 5, 0, 100, 1),
(@GUID+26, 62954, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9722.44, 110.33, 14.80, 0.55, 300, 5, 0, 100, 1),
(@GUID+27, 62954, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9748.88, 78.66, 14.10, 4.22, 300, 5, 0, 100, 1),
(@GUID+28, 62954, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -9760.11, 65.44, 14.55, 5.80, 300, 5, 0, 100, 1),
(@GUID+29, 62954, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -8960.22, 516.88, 96.66, 1.33, 300, 5, 0, 100, 1),
(@GUID+30, 62954, 0, 0, 0, '0', 0, 0, 0, -1, 0, 0, -8945.55, 530.11, 96.44, 3.77, 300, 5, 0, 100, 1);
