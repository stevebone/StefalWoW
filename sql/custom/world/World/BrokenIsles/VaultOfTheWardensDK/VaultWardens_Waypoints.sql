-- Vault of the Wardens Waypoint Paths

DELETE FROM `waypoint_path` WHERE `PathId` IN (9271800, 9963200, 9963201, 9963100, 9963101, 9298000, 9298600, 9298001, 9298601);
DELETE FROM `waypoint_path_node` WHERE `PathId` IN (9271800, 9963200, 9963201, 9963100, 9963101, 9298000, 9298600, 9298001, 9298601);

INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Velocity`, `Comment`) VALUES
(9271800, 1, 2, NULL, 'Vault of the Wardens - 92718 (Maiev Shadowsong)'),
(9963200, 0, 2, NULL, 'Vault of the Wardens - 99632 (Altruis the Sufferer) cell to warglaives'),
(9963201, 0, 2, NULL, 'Vault of the Wardens - 99632 (Altruis the Sufferer) warglaives to rally'),
(9963100, 0, 2, NULL, 'Vault of the Wardens - 99631 (Kayn Sunfury) cell to warglaives'),
(9963101, 0, 2, NULL, 'Vault of the Wardens - 99631 (Kayn Sunfury) warglaives to rally'),
(9298000, 1, 0, NULL, 'Vault of the Wardens - 92980 (Kayn Sunfury) rally to main-hall door'),
(9298600, 1, 2, NULL, 'Vault of the Wardens - 92986 (Altruis the Sufferer) rally to main-hall door'),
(9298001, 1, 0, NULL, 'Vault of the Wardens - 92980 (Kayn Sunfury) quest-accept run to the battle line'),
(9298601, 1, 0, NULL, 'Vault of the Wardens - 92986 (Altruis the Sufferer) quest-accept run to the battle line');
-- Sniffed @MOVID+4: freed Maiev walks out of her cell (quest 38672 - Breaking Out).
-- The 29 captured nodes are three overlapping spline segments of one continuous
-- walk north (-583.01 -> -547.79); merged and sorted by Y for a clean route.
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`) VALUES
(9271800, 1, 4325.842, -583.1104, -281.4801, NULL),
(9271800, 2, 4325.8906, -583.0139, -281.7383, NULL),
(9271800, 3, 4325.8267, -582.8595, -281.51068, NULL),
(9271800, 4, 4325.842, -581.8604, -281.4801, NULL),
(9271800, 5, 4325.842, -581.6104, -281.4801, NULL),
(9271800, 6, 4325.8267, -581.6095, -281.51068, NULL),
(9271800, 7, 4325.842, -581.3604, -281.4801, NULL),
(9271800, 8, 4325.8267, -581.3595, -281.51068, NULL),
(9271800, 9, 4325.8267, -581.1095, -281.51068, NULL),
(9271800, 10, 4325.842, -580.6104, -281.4801, NULL),
(9271800, 11, 4325.8267, -580.3595, -281.51068, NULL),
(9271800, 12, 4325.842, -579.8604, -281.4801, NULL),
(9271800, 13, 4325.8267, -579.6095, -281.51068, NULL),
(9271800, 14, 4325.842, -575.8604, -281.4801, NULL),
(9271800, 15, 4325.8267, -575.6095, -281.51068, NULL),
(9271800, 16, 4325.757, -573.7674, -281.71014, NULL),
(9271800, 17, 4325.8267, -573.6095, -281.51068, NULL),
(9271800, 18, 4325.8267, -569.8595, -281.76068, NULL),
(9271800, 19, 4325.8267, -569.6095, -281.51068, NULL),
(9271800, 20, 4325.8267, -561.6095, -281.51068, NULL),
(9271800, 21, 4325.8447, -561.3884, -281.4262, NULL),
(9271800, 22, 4325.8267, -559.3595, -281.76068, NULL),
(9271800, 23, 4325.8447, -559.1384, -281.6762, NULL),
(9271800, 24, 4325.7544, -557.6215, -281.78027, NULL),
(9271800, 25, 4325.8447, -557.3884, -281.6762, NULL),
(9271800, 26, 4325.8447, -551.1384, -281.4262, NULL),
(9271800, 27, 4325.8447, -550.1384, -281.4262, NULL),
(9271800, 28, 4325.8447, -548.1384, -281.4262, NULL),
(9271800, 29, 4325.934, -547.7917, -281.6465, NULL);

-- Sniffed: freed prisoners walk from their cell to the warglaive pickup spot,
-- kneel (Emote 16), then walk to the rally point next to the door (quest 38672).
-- Mid nodes are the sniffed spline waypoints; last-node orientation is the
-- sniffed arrival facing (leg 1 arrives with Face:None, so orientation is NULL).
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`) VALUES
(9963200, 1, 4315.127, -590.3168, -281.66455, NULL),
(9963200, 2, 4316.877, -590.8168, -281.66455, NULL),
(9963200, 3, 4319.7046, -588.6343, -281.77585, 2.10763),
(9963201, 1, 4321.9766, -583.6597, -281.9241, NULL),
(9963201, 2, 4323.643, -583.6684, -281.74597, 6.13126),
(9963100, 1, 4337.4727, -590.4501, -281.41504, NULL),
(9963100, 2, 4336.7227, -590.7001, -281.66504, NULL),
(9963100, 3, 4334.9727, -590.9501, -281.66504, NULL),
(9963100, 4, 4331.918, -589.117, -281.69394, 0.90829),
(9963101, 1, 4329.474, -584.05383, -281.92447, NULL),
(9963101, 2, 4328.461, -583.6597, -281.58377, 3.38493);

-- Post-reward clones (quest 38672 rewarded): spawned at the rally point as
-- ReplaceObject clones of the main-hall questgiver statics, they run north and
-- despawn where the statics stand. Kayn's leg is a straight run (sniffed
-- @MOVID+16, PathType: None); Altruis follows the sniffed exact spline
-- (@MOVID+17, ExactPath).
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`) VALUES
(9298000, 1, 4330.08, -548.2882, -281.6477, NULL),
(9298600, 1, 4323.542, -581.8078, -281.73813, NULL),
(9298600, 2, 4323.495, -580.9224, -281.61078, NULL),
(9298600, 3, 4323.4585, -580.23346, -281.6905, NULL),
(9298600, 4, 4323.435, -579.79297, -281.61316, NULL),
(9298600, 5, 4323.226, -575.84015, -281.6125, NULL),
(9298600, 6, 4322.9346, -570.32635, -281.8561, NULL),
(9298600, 7, 4322.926, -570.1594, -281.75916, NULL),
(9298600, 8, 4322.414, -560.4824, -281.7644, NULL),
(9298600, 9, 4322.373, -559.70593, -281.82806, NULL),
(9298600, 10, 4322.3403, -559.0919, -281.8156, NULL),
(9298600, 11, 4322.104, -554.6232, -281.73575, NULL),
(9298600, 12, 4322.0093, -552.82513, -281.75284, NULL),
(9298600, 13, 4321.761, -548.13855, -281.74597, NULL),
(9298600, 14, 4321.7466, -547.86115, -281.72333, NULL);

-- 92980 - quest 38690 accept clone, sniffed @MOVID+28: four chained splines
-- re-sent mid-flight; nodes below are the actually-traversed trajectory
-- (leg-1 wps, then the north-corridor points), ending east flank -503.68
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`) VALUES
(9298001, 1, 4330.116, -547.16235, -281.4057, NULL),
(9298001, 2, 4329.866, -545.66235, -281.4057, NULL),
(9298001, 3, 4329.616, -542.41235, -282.1557, NULL),
(9298001, 4, 4329.366, -540.91235, -282.6557, NULL),
(9298001, 5, 4329.366, -539.91235, -282.9057, NULL),
(9298001, 6, 4329.116, -538.41235, -283.4057, NULL),
(9298001, 7, 4328.94, -533.1808, -285.2796, NULL),
(9298001, 8, 4328.3677, -526.8795, -286.97717, NULL),
(9298001, 9, 4328.8677, -523.1295, -287.47717, NULL),
(9298001, 10, 4329.371, -518.1005, -287.33997, NULL),
(9298001, 11, 4329.371, -517.8505, -287.33997, NULL),
(9298001, 12, 4329.621, -516.3505, -287.33997, NULL),
(9298001, 13, 4329.871, -514.8505, -287.83997, NULL),
(9298001, 14, 4330.121, -514.3505, -288.08997, NULL),
(9298001, 15, 4330.121, -513.8505, -288.08997, NULL),
(9298001, 16, 4331.121, -512.1005, -288.08997, NULL),
(9298001, 17, 4332.371, -509.85052, -288.08997, NULL),
(9298001, 18, 4332.621, -508.60052, -288.08997, NULL),
(9298001, 19, 4333.121, -508.10052, -288.08997, NULL),
(9298001, 20, 4334.371, -505.85052, -288.08997, NULL),
(9298001, 21, 4334.871, -505.35052, -288.08997, NULL),
(9298001, 22, 4335.121, -504.85052, -288.08997, NULL),
(9298001, 23, 4335.8145, -503.68057, -288.5678, NULL);

-- 92986 - quest 38689 accept clone, sniffed @MOVID+31: two chained splines,
-- all nodes kept (including the small backtrack at nodes 12-13 that the
-- spline itself performed), ending west flank -500.93
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`) VALUES
(9298601, 1, 4321.8591, -547.50574, -281.63052, NULL),
(9298601, 2, 4321.6091, -545.50574, -281.63052, NULL),
(9298601, 3, 4321.3591, -544.00574, -281.88052, NULL),
(9298601, 4, 4321.1091, -542.50574, -282.38052, NULL),
(9298601, 5, 4321.1091, -541.00574, -282.38052, NULL),
(9298601, 6, 4321.1091, -540.50574, -282.38052, NULL),
(9298601, 7, 4322.169, -534.2594, -283.1194, NULL),
(9298601, 8, 4322.919, -533.2594, -283.6194, NULL),
(9298601, 9, 4323.169, -528.0094, -285.8694, NULL),
(9298601, 10, 4323.1, -524.65622, -287.3797, NULL),
(9298601, 11, 4323.1318, -528.1138, -286.99213, NULL),
(9298601, 12, 4323.1, -524.65622, -287.3797, NULL),
(9298601, 13, 4323.0693, -524.4831, -287.4281, NULL),
(9298601, 14, 4322.8662, -523.33435, -287.61105, NULL),
(9298601, 15, 4321.9182, -517.9825, -287.61255, NULL),
(9298601, 16, 4321.7273, -516.9065, -287.61255, NULL),
(9298601, 17, 4321.3967, -515.0416, -288.32092, NULL),
(9298601, 18, 4321.356, -514.81134, -288.38092, NULL),
(9298601, 19, 4321.9973, -517.6479, -287.33997, NULL),
(9298601, 20, 4321.7473, -516.6479, -287.83997, NULL),
(9298601, 21, 4321.4973, -514.8979, -287.83997, NULL),
(9298601, 22, 4321.2473, -514.6479, -288.08997, NULL),
(9298601, 23, 4320.4973, -512.8979, -288.08997, NULL),
(9298601, 24, 4318.9973, -510.14792, -288.08997, NULL),
(9298601, 25, 4318.7473, -509.64792, -288.08997, NULL),
(9298601, 26, 4318.4973, -510.64792, -288.08997, NULL),
(9298601, 27, 4318.2473, -510.39792, -288.33997, NULL),
(9298601, 28, 4316.4973, -507.14792, -288.08997, NULL),
(9298601, 29, 4312.9973, -501.89792, -288.33997, NULL),
(9298601, 30, 4312.1536, -500.93057, -288.5531, NULL);
