-- Quest: 3661 Favored of Elune
-- Quest: 978 Moontouched Wildkin
-- Quest: 979 Find Ranshalla
-- Quest: 4901 Guardians of the Altar
-- Quest: 4902 Wildkin of Elune
-- NPC: 7916 Erelas Ambersky
-- NPC: 10300 Ranshalla
-- NPC: 3516 Arch Druid Fandral Staghelm
-- Object: 175407 Moontouched Feather

DELETE FROM `disables` WHERE `entry` = 3661 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 978 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 979 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 4901 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 4902 AND `sourceType` = 1; -- quest source

DELETE FROM `creature_questender` WHERE `quest` = 3661;
DELETE FROM `creature_questender` WHERE `quest` = 978;
DELETE FROM `creature_questender` WHERE `quest` = 979;
DELETE FROM `creature_questender` WHERE `quest` = 4901;
DELETE FROM `creature_questender` WHERE `quest` = 4902;
INSERT IGNORE INTO `creature_questender` VALUES
(3516, 4902, 0),
(7916, 4901, 0),
(7916, 978, 0),
(7916, 3661, 0),
(10300, 979, 0);

DELETE FROM `creature_queststarter` WHERE `quest` = 3661;
DELETE FROM `creature_queststarter` WHERE `quest` = 978;
DELETE FROM `creature_queststarter` WHERE `quest` = 979;
DELETE FROM `creature_queststarter` WHERE `quest` = 4901;
DELETE FROM `creature_queststarter` WHERE `quest` = 4902;
INSERT IGNORE INTO `creature_queststarter` VALUES
(7916, 4902, 0),
(10300, 4901, 0),
(7916, 978, 0),
(7916, 979, 0),
(7916, 3661, 0);

UPDATE `quest_template` SET `ContentTuningID` = '20', `RewardFactionFlags` = 1, `Expansion` = '0' WHERE (`ID` = '3661');
UPDATE `quest_template` SET `ContentTuningID` = '33', `RewardFactionFlags` = 1, `Expansion` = '0' WHERE (`ID` = '978');
UPDATE `quest_template` SET `ContentTuningID` = '33', `RewardFactionFlags` = 1, `Expansion` = '0' WHERE (`ID` = '979');
UPDATE `quest_template` SET `ContentTuningID` = '33', `RewardFactionFlags` = 1, `Expansion` = '0' WHERE (`ID` = '4901');
UPDATE `quest_template` SET `ContentTuningID` = '33' WHERE (`ID` = '4902');



-- Spawns for feathers in Winterspring
SET @OGUID := 900000;
DELETE FROM `gameobject` WHERE id = 175407;
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID,175407,1,618,0,'0',0,0,0,-1,6276.51,-4808.29,758.241,3.6509,-0,-0,-0.967751,0.251909,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+1,175407,1,618,0,'0',0,0,0,-1,6215.37,-4850.27,756.138,3.85134,-0,-0,-0.937691,0.347471,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+2,175407,1,618,0,'0',0,0,0,-1,6101.42,-4830.28,750.171,2.74409,-0,-0,-0.980314,-0.197444,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+3,175407,1,618,0,'0',0,0,0,-1,6032.9,-4844.48,757.772,2.51116,-0,-0,-0.950729,-0.310022,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+4,175407,1,618,0,'0',0,0,0,-1,6031.12,-4966.17,764.559,5.37394,-0,-0,-0.439125,0.898426,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+5,175407,1,618,0,'0',0,0,0,-1,6088.88,-5002.99,785.366,0.682745,-0,-0,-0.334781,-0.942296,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+6,175407,1,618,0,'0',0,0,0,-1,6138.42,-5018.71,791.227,5.97711,-0,-0,-0.152439,0.988313,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+7,175407,1,618,0,'0',0,0,0,-1,6235.29,-5076.32,790.533,6.27085,-0,-0,-0.00616724,0.999981,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+8,175407,1,618,0,'0',0,0,0,-1,6310.46,-5026.9,765.956,0.640334,-0,-0,-0.314725,-0.949183,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+9,175407,1,618,0,'0',0,0,0,-1,6398.68,-5002.44,744.563,5.89919,-0,-0,-0.190818,0.981625,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+10,175407,1,618,0,'0',0,0,0,-1,6466.5,-5043.95,739.433,6.06491,-0,-0,-0.108922,0.99405,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+11,175407,1,618,0,'0',0,0,0,-1,6434.51,-4949.36,749.907,3.52369,-0,-0,-0.981806,0.189889,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+12,175407,1,618,0,'0',0,0,0,-1,7718.29,-4530.91,655.179,6.15386,-0,-0,-0.0646168,0.99791,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+13,175407,1,618,0,'0',0,0,0,-1,7745.66,-4442.94,650.769,1.27575,-0,-0,-0.595491,-0.803362,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+14,175407,1,618,0,'0',0,0,0,-1,7729.92,-4389.99,690.026,1.33003,-0,-0,-0.617071,-0.786908,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+15,175407,1,618,0,'0',0,0,0,-1,7756.22,-4363.83,706.16,0.353409,-0,-0,-0.175786,-0.984428,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+16,175407,1,618,0,'0',0,0,0,-1,7801.47,-4345.18,708.501,5.06361,-0,-0,-0.572693,0.81977,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+17,175407,1,618,0,'0',0,0,0,-1,7848.21,-4409.53,681.389,5.4814,-0,-0,-0.39024,0.920713,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+18,175407,1,618,0,'0',0,0,0,-1,7809.87,-4474.92,667.817,4.39718,-0,-0,-0.809326,0.58736,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+19,175407,1,618,0,'0',0,0,0,-1,7724.96,-4526.86,656.38,2.64575,-0,-0,-0.969424,-0.245392,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+20,175407,1,618,0,'0',0,0,0,-1,7736.53,-4315.56,710.663,5.45586,-0,-0,-0.401965,0.915655,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+21,175407,1,618,0,'0',0,0,0,-1,6553.74,-3599.19,719.491,3.04465,-0,-0,-0.998826,-0.0484528,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+22,175407,1,618,0,'0',0,0,0,-1,6526.42,-3564.72,665.706,5.31387,-0,-0,-0.465904,0.884835,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+23,175407,1,618,0,'0',0,0,0,-1,6556.27,-3468.96,644,1.18528,-0,-0,-0.558554,-0.829468,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+24,175407,1,618,0,'0',0,0,0,-1,6463.83,-3449.75,625.71,5.41858,-0,-0,-0.418963,0.908003,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+25,175407,1,618,0,'0',0,0,0,-1,6537.83,-3394.1,601.285,0.192373,-0,-0,-0.0960382,-0.995378,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+26,175407,1,618,0,'0',0,0,0,-1,6409.47,-3014.74,587.973,2.25566,-0,-0,-0.903484,-0.428623,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+27,175407,1,618,0,'0',0,0,0,-1,6377.22,-2967.44,601.151,1.29668,-0,-0,-0.603866,-0.797086,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+28,175407,1,618,0,'0',0,0,0,-1,6410.33,-2910.64,591.445,0.408399,-0,-0,-0.202783,-0.979224,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+29,175407,1,618,0,'0',0,0,0,-1,6479.39,-2860.35,570.117,0.893775,-0,-0,-0.432161,-0.901797,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+30,175407,1,618,0,'0',0,0,0,-1,6457.66,-2783.73,570.393,3.27275,-0,-0,-0.997851,0.0655304,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+31,175407,1,618,0,'0',0,0,0,-1,6437.24,-2716.25,550.693,2.35532,-0,-0,-0.923712,-0.383088,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+32,175407,1,618,0,'0',0,0,0,-1,6437.44,-2637.54,556.565,1.31681,-0,-0,-0.611858,-0.790968,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+33,175407,1,618,0,'0',0,0,0,-1,6446.48,-2550.23,563.175,0.0900224,-0,-0,-0.0449962,-0.998987,300,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+34,175407,1,618,0,'0',0,0,0,-1,6342.04,-2458.63,545.928,2.22866,-0,-0,-0.897616,-0.440778,300,255,1,'',NULL,0);

-- Spawns for Alter quest objects
DELETE FROM `gameobject` WHERE `id` = 177404;
DELETE FROM `gameobject` WHERE `id` = 177417;
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+35,177404,1,618,2250,'0',0,0,0,-1,5514.49,-4917.57,846.061,2.3911,0,0,0.930417,0.366502,900,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+36,177417,1,618,2250,'0',0,0,0,-1,5770.62,-4995.14,810.555,0,0,0,0,1,900,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+37,177417,1,618,2250,'0',0,0,0,-1,5607.76,-5028.4,809.257,0,0,0,0,1,900,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+38,177417,1,618,2250,'0',0,0,0,-1,5629.28,-4890.11,806.275,4.86947,-0,-0,-0.649448,0.760406,900,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+39,177417,1,618,2250,'0',0,0,0,-1,5695.09,-5057.97,808.777,0,0,0,0,1,900,255,1,'',NULL,0);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@OGUID+40,177417,1,618,2250,'0',0,0,0,-1,5464.57,-4983.42,852.027,0,0,0,0,1,900,255,1,'',NULL,0);

-- Update Ranshalla difficulty
DELETE FROM `creature_template_difficulty` WHERE `Entry` = 10300;
INSERT INTO `creature_template_difficulty` (`Entry`,`DifficultyID`,`LevelScalingDeltaMin`,`LevelScalingDeltaMax`,`ContentTuningID`,`HealthScalingExpansion`,`HealthModifier`,`ManaModifier`,`ArmorModifier`,`DamageModifier`,`CreatureDifficultyID`,`TypeFlags`,`TypeFlags2`,`LootID`,`PickPocketLootID`,`SkinLootID`,`GoldMin`,`GoldMax`,`StaticFlags1`,`StaticFlags2`,`StaticFlags3`,`StaticFlags4`,`StaticFlags5`,`StaticFlags6`,`StaticFlags7`,`StaticFlags8`,`VerifiedBuild`) VALUES (10300,0,0,0,33,0,1.35,1,1,1,7068,4096,0,0,0,0,0,0,524288,0,0,0,0,0,0,0,-1);
INSERT INTO `creature_template_difficulty` (`Entry`,`DifficultyID`,`LevelScalingDeltaMin`,`LevelScalingDeltaMax`,`ContentTuningID`,`HealthScalingExpansion`,`HealthModifier`,`ManaModifier`,`ArmorModifier`,`DamageModifier`,`CreatureDifficultyID`,`TypeFlags`,`TypeFlags2`,`LootID`,`PickPocketLootID`,`SkinLootID`,`GoldMin`,`GoldMax`,`StaticFlags1`,`StaticFlags2`,`StaticFlags3`,`StaticFlags4`,`StaticFlags5`,`StaticFlags6`,`StaticFlags7`,`StaticFlags8`,`VerifiedBuild`) VALUES (10300,1,0,0,33,0,1.35,1,1,1,7068,4096,0,0,0,0,0,0,524288,0,0,0,0,0,0,0,-1);


-- Spawn for Ranshalla
SET @CGUID := 900000;
DELETE FROM `creature` WHERE id = 10300;
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@CGUID,10300,1,618,2250,'0',0,0,0,-1,0,1,5717.89,-4794.96,778.064,0,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0);

-- Spawn for Archdruid Faldron
DELETE FROM `creature` WHERE id = 3516;
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@CGUID+1,3516,1,1657,1657,'0',0,0,0,-1,0,1,10174.7,2568.48,1366.96,0.903951,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0);

-- Escort waypoints
DELETE FROM `waypoint_path` WHERE `PathId` = 82402;
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES ('82402', '0', '0', 'Ranshalla - Quest 4901');

DELETE FROM `waypoint_path_node` WHERE `PathId` = 82402;
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,1,5720.45,-4798.45,778.23,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,2,5730.22,-4818.34,777.11,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,3,5728.12,-4835.76,778.15,0,1000);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,4,5718.85,-4865.62,787.56,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,5,5697.13,-4909.12,801.53,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,6,5684.2,-4913.75,801.6,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,7,5674.67,-4915.78,802.13,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,8,5665.61,-4919.22,804.85,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,9,5638.22,-4897.58,804.97,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,10,5632.67,-4892.05,805.44,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,11,5664.58,-4921.84,804.91,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,12,5684.21,-4943.81,802.8,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,13,5724.92,-4983.69,808.25,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,14,5753.39,-4990.73,809.84,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,15,5765.62,-4994.89,809.42,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,16,5724.94,-4983.58,808.29,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,17,5699.61,-4989.82,808.03,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,18,5686.8,-5012.17,807.27,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,19,5691.43,-5037.43,807.73,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,20,5694.24,-5054.64,808.85,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,21,5686.88,-5012.18,807.23,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,22,5664.94,-5001.12,807.78,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,23,5647.12,-5002.84,807.54,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,24,5629.23,-5014.88,807.94,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,25,5611.26,-5025.62,808.36,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,26,5647.13,-5002.85,807.57,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,27,5641.12,-4973.22,809.39,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,28,5622.97,-4953.58,811.12,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,29,5601.52,-4939.49,820.77,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,30,5571.87,-4936.22,831.35,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,31,5543.23,-4933.67,838.33,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,32,5520.86,-4942.05,843.02,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,33,5509.15,-4946.31,849.36,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,34,5498.45,-4950.08,849.98,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,35,5485.78,-4963.4,850.43,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,36,5467.92,-4980.67,851.89,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,37,5498.68,-4950.45,849.96,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,38,5518.68,-4921.94,844.65,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,39,5517.66,-4920.82,845.12,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,40,5518.38,-4913.47,845.57,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,41,5511.31,-4913.82,847.17,0,5000);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,42,5511.31,-4913.82,847.17,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,43,5510.47,-4922.38,846.07,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,44,5517.66,-4920.82,845.12,0,0);
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES (82402,45,5517.6,-4920.8,845.12,0,0);
