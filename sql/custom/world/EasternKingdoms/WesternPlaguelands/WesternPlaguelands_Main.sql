-- Western Plaguelands Fixes

-- NPC: 1835 Scarlet Invoker
-- NPC: 44483 Kirtonos the Herald
-- NPC: 50937 Hamhide
-- NPC: 1847 Foulmane
-- NPC: 50345 Alit

-- Difficulty Fixes
DELETE FROM `creature_template_difficulty` WHERE Entry IN (1847,51058,50345) AND `DifficultyID` = 0;
UPDATE `creature_template_difficulty` SET `DifficultyID` = 0 WHERE Entry IN (1847,51058,50345) AND `DifficultyID` = 1;

UPDATE creature_template_difficulty SET ContentTuningID = 25 WHERE Entry IN (50937);
UPDATE creature_template_difficulty SET StaticFlags1 = StaticFlags1 | 0x20000000 WHERE Entry = 44483;

-- Template Fixes
UPDATE `creature_template` SET `unit_flags3` = 8193 WHERE `entry` = 1835;

-- Template Addon Fixes
UPDATE creature_template_addon SET StandState = 7 WHERE Entry = 1835;
UPDATE creature_template_addon SET emote = 1008 WHERE Entry = 44435;

-- Remove Duplicate/Wrong spawns
DELETE FROM creature WHERE id = 44836 AND guid = 328618;
DELETE FROM creature WHERE id = 44323 AND guid = 328331;

-- Creature Position Fixes
-- 44435/argent-lumberjack
UPDATE creature SET position_x = 2438.8713, position_y = -1617.9844, position_z = 106.45748, orientation = 4.693958 WHERE guid = 328371;
UPDATE creature SET position_x = 2393.5945, position_y = -1534.2042, position_z = 103.336426, orientation = 2.7489376 WHERE guid = 328179;
UPDATE creature SET position_x = 2396.4932, position_y = -1530.8401, position_z = 102.477905, orientation = 2.3471854 WHERE guid = 328178;

-- Creature Random Movement
UPDATE creature
SET MovementType = 1,
    wander_distance = 8
WHERE id IN (
    44284,
    44478,
    44479,
    1817,
    44482,
	1802,
    1804,
    44483,
    1824,
    1822,
    44432,
    44476,
    1815,
    44473,
    44475,
    44551,
	44474,
	45166,
	4472,
	44483,
	45212
);

UPDATE creature
SET MovementType = 1,
    wander_distance = 4
WHERE guid IN (
    328297,
    328244
);


UPDATE creature
SET MovementType = 1
WHERE guid IN (
    328855,
    328859
);

UPDATE creature
SET wander_distance = 6
WHERE guid IN (
    328855,
    328859
);

UPDATE creature
SET MovementType = 1,
    wander_distance = 8
WHERE guid IN (
	328896,
    328846
    328374,
    328383,
    328378,
    328379,
    328412,
	328507,
	328506,
	328252,
    328402,
    328202,
    328197,
    328198,
    328199,
    328200,
    328201,
    328248,
    328254,
    328258
);
