-- Vault of the Wardens Waypoint Paths

DELETE FROM `waypoint_path` WHERE `PathId` IN (9271800, 9963200, 9963201, 9963100, 9963101, 9298000, 9298600);
DELETE FROM `waypoint_path_node` WHERE `PathId` IN (9271800, 9963200, 9963201, 9963100, 9963101, 9298000, 9298600);

INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Velocity`, `Comment`) VALUES
(9271800, 1, 2, NULL, 'Vault of the Wardens - 92718 (Maiev Shadowsong)'),
(9963200, 0, 2, NULL, 'Vault of the Wardens - 99632 (Altruis the Sufferer) cell to warglaives'),
(9963201, 0, 2, NULL, 'Vault of the Wardens - 99632 (Altruis the Sufferer) warglaives to rally'),
(9963100, 0, 2, NULL, 'Vault of the Wardens - 99631 (Kayn Sunfury) cell to warglaives'),
(9963101, 0, 2, NULL, 'Vault of the Wardens - 99631 (Kayn Sunfury) warglaives to rally'),
(9298000, 1, 0, NULL, 'Vault of the Wardens - 92980 (Kayn Sunfury) rally to main-hall door'),
(9298600, 1, 2, NULL, 'Vault of the Wardens - 92986 (Altruis the Sufferer) rally to main-hall door');
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
