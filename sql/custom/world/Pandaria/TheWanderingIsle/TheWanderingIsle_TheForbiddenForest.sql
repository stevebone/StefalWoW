-- NPC: 55943 Wei Palerage
-- NPC: 60042 Korga
-- NPC: 67186 Provisioner Drog

-- Creature Templates
UPDATE `creature_template` SET `ScriptName` = 'npc_korga_hut' WHERE `entry` = 60042;

DELETE FROM `creature_template_addon` WHERE `entry` IN (55943, 60042, 67186);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureId`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('55943', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '110846'),
('60042', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('67186', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `npc_vendor` WHERE `entry` IN (67186);
INSERT INTO `npc_vendor` (entry, slot, item, maxcount, incrtime, ExtendedCost, `type`, BonusListIDs, PlayerConditionID, IgnoreFiltering, VerifiedBuild) VALUES
('67186', '1', '117', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '7', '159', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '4', '787', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '2', '2287', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '3', '3770', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '5', '4592', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '6', '4593', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '8', '90659', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '9', '90660', '0', '0', '0', '1', NULL, '0', '0', '64154');

-- Area Trigger Scripts
DELETE FROM `areatrigger_scripts` WHERE `entry` IN (7714);
INSERT INTO `areatrigger_scripts` VALUES
(7714, 'at_forlorn_hut_7714');