-- Southern Barrens Fixes

-- NPC: 10378 Omusa Thunderhorn <Wind Rider Master>
-- NPC: 5944 Yonada <Tailoring and Leatherworking Supplier>
-- NPC: 8016 Barrens Guard
-- NPC: 3703 Krulmoo Fullmoon <Leatherworking Trainer>
-- NPC: 6387 Dranh <Skinning Trainer>
-- NPC: 37167 Stonetalon Prisoner
-- NPC: 37743 Taurajo Looter

-- Template fixes
UPDATE `creature_template` SET `unit_flags3` = 8193, `npcflag` = 0 WHERE `entry` IN (10378,5944,3703,6387);
UPDATE `creature_template` SET `unit_flags3` = 8193 WHERE `entry` = 8016;
UPDATE `creature_template` SET `unit_flags3` = 8193 WHERE `entry` = 37933;
UPDATE `creature_template_addon` SET `StandState` = 7 WHERE `Entry` IN (10378,5944,3703,6387, 8016);

-- Template Addons
DELETE FROM `creature_template_addon` WHERE `Entry` IN (37167,37933);
INSERT INTO `creature_template_addon` (`Entry`, `StandState`) VALUES 
(37167, 1),
(37933, 7);

-- Creature Fixes
-- Remove duplicate or wrong spawns
DELETE FROM `creature` WHERE `guid` IN (298627, 298634, 298635, 298629, 298633, 298640, 298643, 298644, 298638, 298645);

UPDATE creature
SET MovementType = 1,
    wander_distance = 8
WHERE id IN (
    37743,
    3240,
    37207,
    37585,
    37208,
    37557,
    37553, 37556,
	37088, 2620, 37922, 37555, 37091, 37090, 37093, 37092, 37086, 37511, 3261, 37084, 37083, 37085, 37925, 6132
);
