-- Sentinel Hill changes and updates

-- NPC: 234 Gryan Stoutmantle
-- NPC: 491 Quartermaster Lewis
-- NPC: 523 Thor
-- NPC: 821 Captain Danuvin
-- NPC: 878 Scout Galiaan
-- NPC: 42384 Homeless Stormwind Citizen
-- NPC: 42386 Homeless Stormwind Citizen (Female
-- NPC: 42575 Hope Saldean
-- NPC: 42635 Ripsnarl
-- NPCL 42748 Ripsnarl Spawned

-- GO: 204005 Heavy Wooden Stocks

-- Quest: 12 The People's militia
-- Quest: 13 The People's militia
-- Quest: 14 The People's militia
-- Quest: 102 Patrolling Westfall
-- Quest: 153 Red Leather Bandanas
-- Quest: 6181 A swift message
-- Quest: 6281 Continue to Stormwind
-- Quest: 26271 Feeding the hungry
-- Quest: 26286 In Defense of Westfall

-- Quest: 65 The Defias Brotherhood
-- Quest: 132 The Defias Brotherhood
-- Quest: 135 The Defias Brotherhood
-- Quest: 141 The Defias Brotherhood
-- Quest: 142 The Defias Brotherhood
-- Quest: 155 The Defias Brotherhood
-- Quest: 166 The Defias Brotherhood


-- Reactivating old/deprecated quests in Sentinel Hill
DELETE FROM `disables` WHERE (`sourceType` = '1') and `entry` IN (12, 13, 14, 102, 153, 65, 132, 135, 141, 142, 155, 166);
DELETE FROM `disables` WHERE (`sourceType` = '1') and `entry` IN (6181, 6281);

-- Quest Templates
UPDATE `quest_template` SET `ContentTuningID` = '6', `Expansion` = 0, `AllowableRaces` = 18446744073709551615 WHERE `ID` IN (12, 13, 14, 102, 153, 65, 132, 135, 141, 142, 155, 166, 6181, 6281);

-- Creature Quest Relations
DELETE FROM `creature_queststarter` WHERE `quest` IN (12, 13, 14, 102, 153, 65, 132, 135, 141, 142, 155, 166, 6181, 6281);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES 
('523', '6281', '0'),
('491', '6181', '0'),
('821', '102', '0'),
('878', '153', '0'),
('234', '12', '0'),
('234', '13', '0'),
('234', '14', '0'),
('234', '65', '0'),
('266', '132', '0'),
('234', '135', '0'),
('332', '141', '0'),
('234', '142', '0'),
('467', '155', '0'),
('234', '166', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (12, 13, 14, 102, 153, 65, 132, 135, 141, 142, 155, 166, 6181,6281);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES 
('1323', '6281', '0'),
('523', '6181', '0'),
('821', '102', '0'),
('878', '153', '0'),
('234', '12', '0'),
('234', '13', '0'),
('234', '14', '0'),
('266', '65', '0'),
('234', '132', '0'),
('332', '135', '0'),
('234', '141', '0'),
('234', '142', '0'),
('234', '155', '0'),
('234', '166', '0');

-- Quest: 26271 Feeding the hungry
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_westfall_stew_42617' WHERE (`entry` = 42617);

-- Update NEW phase for existing creatures
UPDATE `creature` SET `PhaseId` = 50005 WHERE `id` IN (54373,54371,54372,42407,42384,42386,42383,42391,42390,42385,42400,42677);
UPDATE `creature` SET `PhaseId` = 50005 WHERE `guid` IN (275164);

DELETE FROM `creature_addon` WHERE `guid` IN (275405,275407,275377,275376,275379,275378,275283,275259,275257,275258,275137,275096,
275094,274984,274925,274923,274921,274885,280419);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, 
`PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(280419, 4238300, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '79116'), -- Drifter with path (budy with cart)
(274885, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Homeless Stormwind Citizen
(274921, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Homeless Stormwind Citizen
(274923, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Homeless Stormwind Citizen
(274925, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Homeless Stormwind Citizen
(274984, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Homeless Stormwind Citizen
(275094, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Homeless Stormwind Citizen
(275096, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Homeless Stormwind Citizen
(275405, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Homeless Stormwind Citizen
(275137, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Homeless Stormwind Citizen
(275257, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Homeless Stormwind Citizen
(275258, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Homeless Stormwind Citizen
(275259, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Homeless Stormwind Citizen
(275283, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Homeless Stormwind Citizen
(275407, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Homeless Stormwind Citizen
(275376, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Homeless Stormwind Citizen
(275377, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Homeless Stormwind Citizen
(275379, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Homeless Stormwind Citizen
(275378, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''); -- Homeless Stormwind Citizen

-- Creature fixes
UPDATE `creature_template` SET `faction` = 2280 WHERE `entry` = 42400; -- wrong faction for drifters with cart

UPDATE `creature` SET `id` = 42383 WHERE `guid` = 280419 AND `id` = 42400; -- fix reverse id
UPDATE `creature` SET `id` = 42400 WHERE `guid` = 280420 AND `id` = 42383; -- fix reverse id

DELETE FROM `creature_formations` WHERE `leaderGUID` IN (280419);
INSERT INTO `creature_formations` (`leaderGUID`, `memberGUID`, `dist`, `angle`, `groupAI`, `point_1`, `point_2`) VALUES
(280419, 280419, 0, 0, 515, 0, 0), -- Drifter
(280419, 280420, 3, 270, 515, 8, 17); -- Drifter with cart

-- Waypoints
UPDATE `creature` SET `MovementType` = 2 WHERE `guid` = 280419; -- drifter with cart buddy

-- Hobos event at Sentinel Hill gates
UPDATE `creature` SET `ScriptName` = 'npc_custom_westfall_guard_42407' WHERE `guid` IN (275390,274919);

DELETE FROM `creature_template_difficulty` WHERE `Entry` IN (54373,54371,54372);
INSERT INTO `creature_template_difficulty` (`Entry`,`DifficultyID`,`LevelScalingDeltaMin`,`LevelScalingDeltaMax`,`ContentTuningID`,`HealthScalingExpansion`,`HealthModifier`,`ManaModifier`,`ArmorModifier`,`DamageModifier`,`CreatureDifficultyID`,`TypeFlags`,`TypeFlags2`,`TypeFlags3`,`LootID`,`PickPocketLootID`,`SkinLootID`,`GoldMin`,`GoldMax`,`StaticFlags1`,`StaticFlags2`,`StaticFlags3`,`StaticFlags4`,`StaticFlags5`,`StaticFlags6`,`StaticFlags7`,`StaticFlags8`,`VerifiedBuild`) VALUES 
(54373, 0, 0, 0, 6, 0, 0.7, 1, 1, 0.2, 31357, 0, 0, 0, 54373, 54373, 0, 200, 300, 0, 0, 0, 0, 0, 0, 0, 0, 64978), -- alternate Riverpaw Herbalist
(54372, 0, 0, 0, 6, 0, 1, 1, 1, 0.2, 31359, 0, 0, 0, 54372, 54372, 0, 200, 300, 0, 0, 0, 0, 0, 0, 0, 0, 64978), -- alternate Riverpaw Brute
(54371, 0, 0, 0, 6, 0, 1, 1, 1, 0.2, 31361, 0, 0, 0, 54371, 54371, 0, 200, 300, 0, 0, 0, 0, 0, 0, 0, 0, 64978); -- alternate Riverpaw Bandit

SET @CGUID := 900000;
DELETE FROM `creature` WHERE `guid` IN (@CGUID+818,@CGUID+819,@CGUID+820);
INSERT INTO `creature` VALUES
-- Horatio Laine and Stormwind Investigators for Dialogue with Gryan
(@CGUID+818, '42308', '0', '40', '108', '0', '0', '50006', '0', '-1', '0', '0', '-10507.5', '1049.1', '60.5186', '4.37166', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0'),
(@CGUID+819, '42745', '0', '40', '108', '0', '0', '50006', '0', '-1', '0', '0', '-10504.6', '1049.27', '60.5181', '4.09412', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0'),
(@CGUID+820, '42745', '0', '40', '108', '0', '0', '50006', '0', '-1', '0', '0', '-10508.6', '1051.57', '60.0498', '4.69275', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0');

DELETE FROM `creature_addon` WHERE `guid` = @CGUID+818; -- Horatio Laine addon
INSERT INTO `creature_addon` (`guid`, `VisFlags`, `SheathState`, `auras`) VALUES (@CGUID+818, 0, 1, '');

-- Quest: 26286 In Defense of Westfall
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_gryan_stoutmantle_234' WHERE (`entry` = 234);

-- Ripsnarl
UPDATE `creature_template` SET `ScriptName` = 'npc_ripsnarl' WHERE `entry` IN (42748,42635);