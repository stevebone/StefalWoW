-- Westfall coast and farm areas changes and updates

-- NPC: 95 Defias Smuggler
-- NPC: 98 Riverpaw Taskmaster
-- NPC: 114 Harvest Watchers
-- NPC: 115 Harvest Reaper
-- NPC: 117 Riverpaw Gnoll
-- NPC: 121 Defias Pathstalker
-- NPC: 122 Defias Highwayman
-- NPC: 123 Riverpaw Mongrel
-- NPC: 124 Riverpaw Brute
-- NPC: 126 Murloc Coastrunner
-- NPC: 127 Murloc Tidehunter
-- NPC: 154 Greater Fleshripper
-- NPC: 157 Goretusk
-- NPC: 171 Murloc Warrior
-- NPC: 199 Young Fleshripper
-- NPC: 233 Farmer Saldean
-- NPC: 235 Salma Saldean
-- NPC: 237 Farmer Furlbrow
-- NPC: 238 Verna Furlbrow
-- NPC: 391 Murloc Old Murk
-- NPC: 392 Captain Grayson
-- NPC: 449 Defias Knuckleduster
-- NPC: 452 Riverpaw Bandit
-- NPC: 453 Riverpaw Mystic
-- NPC: 454 Young Goretusk
-- NPC: 456 Murloc Minor Oracle
-- NPC: 458 Murloc Hunter
-- NPC: 462 Vultros
-- NPC: 480 Rusty Harvest Golem
-- NPC: 481 Defias Footpad
-- NPC: 500 Riverpaw Scout
-- NPC: 501 Riverpaw Herbalist
-- NPC: 504 Defias Trapper
-- NPC: 513 Murloc Netter
-- NPC: 515 Murloc Raider
-- NPC: 517 Murloc Oracle
-- NPC: 519 Murloc Slark
-- NPC: 582 Old Blanchy
-- NPC: 589 Defias Pillager
-- NPC: 590 Defias Looter
-- NPC: 594 Defias Hencheman
-- NPC: 830 Sand Crawler
-- NPC: 831 Sea Crawler
-- NPC: 832 Unbound Cyclone
-- NPC: 834 Coyote
-- NPC: 1065 Riverpaw Shaman
-- NPC: 1109 Fleshripper
-- NPC: 1236 Kobold Digger
-- NPC: 1424 Master Digger
-- NPC: 6250 Crawler
-- NPC: 7024 Agent Kearnen
-- NPC: 42342 Energized Harvest Reaper
-- NPC: 42357 Hulking Goretusk
-- NPC: 42383 Transient
-- NPC: 42384 Homeless Stormwind Citizen
-- NPC: 42386 Homeless Stormwind Citizen (Female)
-- NPC: 42390 Small-Time Hustler
-- NPC: 42391 West Plains Drifter
-- NPC: 42653 Riverpaw Jango
-- NPC: 42656 Mercenary
-- NPC: 42669 Chasm Slime
-- NPC: 42677 Moonbrook Thug
-- NPC: 900000 copy of Farmer Furlbrow for phasing
-- NPC: 900001 copy of Verna Furlbrow
-- NPC: 900002 copy of Old Blanchy

-- Quest: 9 The killing fields
-- Quest: 22 Goretusk Liver Pie
-- Quest: 36 Westfall Stew
-- Quest: 64 The Forgotten Heirloom
-- Quest: 103 Keeper of the flame
-- Quest: 109 Report to Gryan Stoutmantle
-- Quest: 151 Poor Old Blanchy
-- Quest: 184 Furlbrows Deed
-- Quest: 26209 Murder Was The Case That They Gave Me
-- Quest: 26213 Hot On the Trail: The Riverpaw Clan
-- Quest: 26214 Hot On the Trail: Murlocs

-- Reactivating old/deprecated quests in Stormwind
DELETE FROM `disables` WHERE `entry` IN (9, 22, 36, 64, 109, 151, 103) AND `sourceType` = 1; -- quest source
DELETE FROM `creature_queststarter` WHERE `quest` IN (9, 22, 36, 109, 64, 151, 103);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES 
('900000', '64', '0'),
('900001', '36', '0'),
('900001', '151', '0'),
('900000', '109', '0'),
('392', '103', '0'),
('235', '22', '0'),
('233', '9', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (9, 22, 36, 109, 64, 151, 184, 103);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES 
('900000', '64', '0'),
('900000', '184', '0'),
('235', '36', '0'),
('900001', '151', '0'),
('234', '109', '0'),
('392', '103', '0'),
('235', '22', '0'),
('233', '9', '0');


-- custom phases for Westfall
DELETE FROM `phase_area` WHERE `PhaseId` IN (50004, 50005);
DELETE FROM `phase_area` WHERE `PhaseId` = 169 AND `AreaId` IN (916, 40);
INSERT INTO `phase_area` VALUES
(916, 169, 'Westfall The Jansen Stead - Furlbrows'),
(40, 169, 'Westfall The Jansen Stead - Furlbrows'),
(916, 50004, 'Westfall The Jansen Stead - Furlbrows 1'),
(40, 50004, 'Westfall - Furlbrows 1'),
(40, 50005, 'Westfall - Furlbrows 2'),
(916, 50005, 'Westfall The Jansen Stead - Furlbrows 2');

-- Phase Conditions
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` IN (50004, 50005);
INSERT INTO `conditions` VALUES
('26', '50004', '916', '0', '0', '47', '0', '109', '64', '0', '', '1', '0', '0', '', 'Westfall - Add phase 50004 if 109 IS NOT rewarded'),
('26', '50005', '916', '0', '0', '47', '0', '109', '64', '0', '', '0', '0', '0', '', 'Westfall - Add phase 50005 if 109 IS rewarded'),
('26', '50004', '40', '0', '0', '47', '0', '109', '64', '0', '', '1', '0', '0', '', 'Westfall - Add phase 50004 if 109 IS NOT rewarded'),
('26', '50005', '40', '0', '0', '47', '0', '109', '64', '0', '', '0', '0', '0', '', 'Westfall - Add phase 50005 if 109 IS rewarded');

-- Quest Templates
UPDATE `quest_template` SET `ContentTuningID` = '6', `Expansion` = '0' WHERE `ID` IN (9, 22, 36, 109, 64, 103, 151);
DELETE FROM `quest_template_addon` WHERE `ID` IN (109);
INSERT INTO `quest_template_addon` (`ID`, `PrevQuestID`) VALUES
(109, 64);

-- Creature templates
UPDATE `creature` SET `spawn_distance` = 10, `MovementType` = 1 WHERE `id` IN (1236); -- Kobold Digger
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (504); -- Defias Trapper


DELETE FROM `creature_template` WHERE `entry` IN (900000, 900001, 900002);
INSERT INTO creature_template (entry, KillCredit1, KillCredit2, name, femaleName, subname, TitleAlt, IconName, RequiredExpansion, VignetteID, faction, npcflag, speed_walk, speed_run, scale, Classification, dmgschool, BaseAttackTime, RangeAttackTime, BaseVariance, RangeVariance, unit_class, unit_flags, unit_flags2, unit_flags3, family, trainer_class, type, VehicleId, AIName, MovementType, ExperienceModifier, RacialLeader, movementId, WidgetSetID, WidgetSetUnitConditionID, RegenHealth, CreatureImmunitiesId, flags_extra, ScriptName, StringId, VerifiedBuild) VALUES
('900000', '0', '0', 'Farmer Furlbrow', '', '', NULL, NULL, '0', '0', '12', '2', '1', '1.14286', '1', '0', '0', '1500', '2000', '1', '1', '1', '537133824', '2048', '0', '0', '0', '7', '0', 'SmartAI', '0', '1', '0', '0', '0', '0', '1', '0', '66', '', NULL, '66384'),
('900001', '0', '0', 'Verna Furlbrow', '', '', NULL, NULL, '0', '0', '12', '2', '1', '1.14286', '1', '0', '0', '1500', '2000', '1', '1', '1', '537133824', '2048', '0', '0', '0', '7', '0', 'SmartAI', '0', '1', '0', '0', '0', '0', '1', '0', '66', '', NULL, '66384'),
('900002', '0', '0', 'Old Blanchy', '', '', NULL, NULL, '0', '0', '35', '0', '1', '1.28571', '1', '0', '0', '2000', '2000', '1', '1', '1', '768', '2048', '0', '299', '0', '1', '0', '', '0', '1', '0', '0', '0', '0', '1', '0', '64', '', NULL, '66384');

DELETE FROM `creature_template_difficulty` WHERE `entry` IN (900000, 900001, 900002);
INSERT INTO `creature_template_difficulty` (`Entry`,`DifficultyID`,`LevelScalingDeltaMin`,`LevelScalingDeltaMax`,`ContentTuningID`,`HealthScalingExpansion`,`HealthModifier`,`ManaModifier`,`ArmorModifier`,`DamageModifier`,`CreatureDifficultyID`,`TypeFlags`,`TypeFlags2`,`TypeFlags3`,`LootID`,`PickPocketLootID`,`SkinLootID`,`GoldMin`,`GoldMax`,`StaticFlags1`,`StaticFlags2`,`StaticFlags3`,`StaticFlags4`,`StaticFlags5`,`StaticFlags6`,`StaticFlags7`,`StaticFlags8`,`VerifiedBuild`) VALUES 
('900000', '0', '0', '0', '6', '0', '1', '1', '1', '1', '124', '0', '0', '0', '0', '0', '0', '0', '0', '256', '0', '0', '0', '0', '0', '0', '0', '63906'),
('900001', '0', '0', '0', '6', '0', '1', '1', '1', '1', '125', '0', '0', '0', '0', '0', '0', '0', '0', '256', '0', '0', '0', '0', '0', '0', '0', '63906'),
('900002', '0', '0', '0', '6', '0', '1', '1', '1', '1', '396', '1', '0', '0', '0', '0', '0', '0', '0', '256', '0', '0', '0', '0', '0', '0', '0', '63906');

-- duplicate models for the new Furlbrows
DELETE FROM `creature_template_model` WHERE `CreatureID` IN (900000, 900001, 900002);
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`) VALUES
(900000, 0, 1944, 1, 1, 66384),
(900001, 0, 1692, 1, 1, 66384),
(900002, 0, 236, 1, 1, 66384);

-- creature texts for the new Furlbrows
DELETE FROM `creature_text` WHERE `CreatureID` IN (900000, 900001, 900002);
INSERT INTO creature_text (CreatureID, GroupID, ID, Text, Type, Language, Probability, Emote, Duration, Sound, SoundPlayType, BroadcastTextId, TextRange, comment) VALUES
(900001, 0, 0, 'Don\'t worry, Old Blanchy… we\'ll find a way to keep going.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Verna ambient'),
(900001, 0, 1, 'Some days I wonder if Westfall will ever be the same again.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Verna ambient'),
(900001, 0, 2, 'If only the harvest had been kinder to us this year…', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Verna ambient'),
(900001, 0, 3, 'I miss the sound of children playing in these fields.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Verna ambient'),
(900001, 0, 4, 'Stormwind forgets about folk like us… but we keep on trying.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Verna ambient'),
(900001, 0, 5, 'If only the Defias would leave honest people alone.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Verna ambient'),
(900001, 0, 6, 'I never thought we’d see such hard times in our own homeland.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Verna ambient'),
(900001, 0, 7, 'Old Blanchy deserves better than this… we all do.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Verna ambient'),
(900001, 0, 8, 'I pray the fields will bloom again someday.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Verna ambient'),
(900001, 0, 9, 'We’ll rebuild… somehow. We always do.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Verna ambient'),
(900000, 0, 0, 'If this wagon breaks again, I swear I’ll toss it in the river.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Farmer ambient'),
(900000, 0, 1, 'Ain’t been a lucky day in Westfall for years now.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Farmer ambient'),
(900000, 0, 2, 'One good harvest… that’s all I’m askin’ for.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Farmer ambient'),
(900000, 0, 3, 'If the Defias show their faces again, they’ll get a pitchfork for their trouble.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Farmer ambient'),
(900000, 0, 4, 'Stormwind taxes us dry, then leaves us to fend for ourselves.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Farmer ambient'),
(900000, 0, 5, 'Blasted dust gets everywhere… can’t keep a thing clean.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Farmer ambient'),
(900000, 0, 6, 'If I can fix this wheel, maybe we can finally move on.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Farmer ambient'),
(900000, 0, 7, 'Used to be a proud farm… now look at it.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Farmer ambient'),
(900000, 0, 8, 'Verna keeps my spirits up… Light knows I need it.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Farmer ambient'),
(900000, 0, 9, 'Someday, Westfall will rise again. Mark my words.', 12, 0, 100, 0, 0, 0, 0, 0, 0, 'Farmer ambient');

-- Smart Scripts
DELETE FROM `smart_scripts` WHERE `source_Type` = 0 AND `entryorguid` IN (900000, 900001);
INSERT INTO smart_scripts (entryorguid, source_type, id, link, Difficulties,
event_type, event_phase_mask, event_chance, event_flags,
event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string,
action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string,
target_type, target_param1, target_param2, target_param3, target_param4, target_param_string,
target_x, target_y, target_z, target_o,
comment)
VALUES
(900000, 0, 0, 0, '', 1, 0, 100, 0, 35000, 45000, 35000, 45000, 0, '',
 1, 0, 0, 0, 0, 0, 0, 0, '',
 1, 0, 0, 0, 0, '',
 0, 0, 0, 0,
 'Farmer Furlbrow - OOC Timer - Say Group 0 every 45s'),
 (900001, 0, 0, 0, '', 1, 0, 100, 0, 45000, 45000, 45000, 45000, 0, '',
 1, 0, 0, 0, 0, 0, 0, 0, '',
 1, 0, 0, 0, 0, '',
 0, 0, 0, 0,
 'Verna Furlbrow - OOC Timer - Say Group 0 every 45s');

-- Creature spawns
SET @CGUID := 900000;
DELETE FROM `creature` WHERE `guid` in (@CGUID+128, @CGUID+129, @CGUID+130) OR `id` IN (900000, 900001, 900002);
DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID+131 AND @CGUID+190;
INSERT INTO `creature` (guid, id, map, zoneId, areaId, spawnDifficulties, phaseUseFlags, PhaseId, PhaseGroup, terrainSwapMap, modelid, equipment_id, position_x, position_y, position_z, orientation, spawntimesecs, wander_distance, currentwaypoint, curHealthPct, MovementType, npcflag, unit_flags, unit_flags2, unit_flags3, ScriptName, StringId, VerifiedBuild) VALUES
(@CGUID+128, '900000', '0', '40', '916', '0', '0', '50004', '0', '-1', '0', '0', '-9845.67', '908.615', '30.2339', '1.25925', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0'),
(@CGUID+129, '900001', '0', '40', '916', '0', '0', '50004', '0', '-1', '0', '0', '-9846.88', '909.397', '30.2058', '0.529411', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0'),
(@CGUID+130, '900002', '0', '40', '916', '0', '0', '50004', '0', '-1', '0', '0', '-9848.07', '912.605', '30.3397', '5.49706', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0'),
(@CGUID+131, '121', '0', '40', '917', '0', '0', '0', '0', '-1', '0', '1', '-10760.6', '887.112', '35.9499', '1.8592', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+132, '121', '0', '40', '917', '0', '0', '0', '0', '-1', '0', '1', '-10778.4', '900.557', '34.2905', '3.63155', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+133, '121', '0', '40', '922', '0', '0', '0', '0', '-1', '0', '1', '-11286.6', '1003.59', '93.8531', '5.4755', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+134, '121', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11249.3', '1160.31', '89.0338', '6.05507', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+135, '121', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11227.6', '1186.13', '90.4088', '1.23839', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+136, '121', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11277.9', '1520.15', '80.2267', '1.9987', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+137, '121', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11314.6', '1720.49', '36.9659', '2.99404', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+138, '121', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11322.3', '1729.38', '37.0876', '4.48294', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+139, '122', '0', '40', '917', '0', '0', '0', '0', '-1', '0', '1', '-10764.5', '894.108', '35.9581', '4.65798', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+140, '122', '0', '40', '917', '0', '0', '0', '0', '-1', '0', '1', '-10755.9', '893.287', '35.9569', '3.42053', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+141, '122', '0', '40', '917', '0', '0', '0', '0', '-1', '0', '1', '-10767.1', '898.213', '34.767', '3.95724', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+142, '122', '0', '40', '917', '0', '0', '0', '0', '-1', '0', '1', '-10756.9', '870.277', '35.4261', '0.0904293', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+143, '122', '0', '40', '922', '0', '0', '0', '0', '-1', '0', '1', '-11275.1', '1021.64', '94.3743', '2.74624', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+144, '122', '0', '40', '922', '0', '0', '0', '0', '-1', '0', '1', '-11279.4', '1024.1', '95.5327', '5.83067', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+145, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11238.7', '1153.14', '89.0472', '2.10047', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+146, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11245.6', '1153.78', '89.0967', '0.548273', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+147, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11191.6', '1197.71', '86.6465', '0.146682', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+148, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11168.5', '1205.76', '78.8108', '1.57565', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+149, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11140', '1189.16', '68.3135', '2.78897', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+150, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11136.4', '1245.01', '77.4862', '3.3101', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+151, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11174.3', '1304.97', '95.8018', '3.24405', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+152, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11177.2', '1323.61', '93.2492', '3.38542', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+153, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11207.9', '1343.03', '96.2819', '4.51465', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+154, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11213.5', '1314.9', '89.6881', '4.8451', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+155, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11286.3', '1365.58', '94.4671', '2.51144', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+156, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11291.7', '1390.67', '93.5157', '1.90552', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+157, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11255.4', '1390.16', '89.5377', '5.40389', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+158, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11262.5', '1544.64', '74.1953', '0.684066', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+159, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11248.2', '1560.05', '70.6895', '1.67966', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+160, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11281.2', '1580.46', '67.3042', '2.82415', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+161, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11295.9', '1565.61', '71.1469', '5.05699', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+162, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11266.6', '1613.04', '59.1569', '1.43701', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+163, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11329.7', '1717.26', '37.7311', '0.202397', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+164, '122', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '1', '-11321.7', '1712.37', '37.8317', '0.89297', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+165, '449', '0', '40', '917', '0', '0', '0', '0', '-1', '0', '0', '-10783.6', '914.14', '34.135', '4.0791', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+166, '449', '0', '40', '917', '0', '0', '0', '0', '-1', '0', '0', '-10752.9', '880.94', '35.7814', '1.72567', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+167, '449', '0', '40', '922', '0', '0', '0', '0', '-1', '0', '0', '-11269.2', '1004.1', '88.7102', '5.90251', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+168, '449', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '0', '-11191.9', '1232.61', '87.9491', '1.30399', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+169, '449', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '0', '-11241.7', '1265.15', '89.0532', '0.844075', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+170, '449', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '0', '-11289.1', '1365.1', '94.906', '0.890615', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+171, '449', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '0', '-11288.8', '1369.07', '95.2292', '4.85134', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+172, '449', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '0', '-11283.7', '1650.94', '52.0799', '1.5525', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+173, '449', '0', '40', '920', '0', '0', '0', '0', '-1', '0', '0', '-11252.3', '1673.7', '45.5236', '0.85383', '300', '10', '0', '100', '1', NULL, '32768', NULL, NULL, '', NULL, '0');

INSERT INTO `creature`
(guid, id, map, zoneId, areaId, PhaseId, position_x, position_y, position_z, orientation, spawntimesecs, wander_distance, curHealthPct) VALUES
(@CGUID+174, 480, 0, 40, 916, 50004, -9899.81, 1014.58, 33.1021, 2.4945, 120, 10, 100),
(@CGUID+175, 480, 0, 40, 916, 50004, -9888.45, 1029.92, 33.2239, 1.29987, 120, 10, 100),
(@CGUID+176, 480, 0, 40, 916, 50004, -9884.91, 1012.66, 32.4921, 5.65349, 120, 10, 100),
(@CGUID+177, 480, 0, 40, 916, 50004, -9869.65, 1016.56, 32.7775, 0.61599, 120, 10, 100),
(@CGUID+178, 480, 0, 40, 40, 50004, -9971.62, 1080.07, 42.0848, 2.55812, 120, 10, 100),
(@CGUID+179, 480, 0, 40, 40, 50004, -9986.91, 1090.26, 40.8899, 5.70351, 120, 10, 100),
(@CGUID+180, 480, 0, 40, 40, 50004, -9956.94, 1075.69, 39.8649, 1.84501, 120, 10, 100),
(@CGUID+181, 480, 0, 40, 109, 50004, -9986.07, 1134.79, 43.5638, 1.87089, 120, 10, 100),
(@CGUID+182, 480, 0, 40, 109, 50004, -9996.21, 1153.53, 43.2372, 3.80586, 120, 10, 100),
(@CGUID+183, 480, 0, 40, 109, 50004, -9971.11, 1138.98, 43.2571, 5.31846, 120, 10, 100),
(@CGUID+184, 480, 0, 40, 109, 50004, -9999.38, 1119.2, 43.526, 6.21426, 120, 10, 100),
(@CGUID+185, 480, 0, 40, 916, 50004, -9811.83, 985.927, 29.1312, 3.25388, 120, 10, 100),
(@CGUID+186, 480, 0, 40, 916, 50004, -9828.17, 951.856, 29.1312, 3.25388, 120, 10, 100),
(@CGUID+187, 480, 0, 40, 916, 50004, -9846.42, 970.621, 29.1312, 1.27616, 120, 10, 100),
(@CGUID+188, 480, 0, 40, 916, 50004, -9821.65, 1003.68, 29.1312, 1.03315, 120, 10, 100),
(@CGUID+189, 480, 0, 40, 916, 50004, -9780.87, 979.251, 29.1312, 3.56651, 120, 10, 100),
(@CGUID+190, 480, 0, 40, 916, 50004, -9806.41, 967.694, 29.1312, 3.56651, 120, 10, 100);

-- Update phase for existing creatures
UPDATE `creature` SET `PhaseId` = 50005 WHERE `guid` IN (276241, 276256, 276238, 276261, 276262, 276239, 276257, 276258, 276259, 276260);
UPDATE `creature` SET `PhaseId` = 50005 WHERE `guid` IN (224151,224156,224161,224164,224170,224171,224177,224180,224181,224152,224153,224158,224160,224166,224173,224182,224185,224154,224155,224157,224159,224162,224167,224169,224174,224176,224178,224179,224163,224165,224168,224172,224175,224183,224184);

-- fix for missing loot for clues
INSERT IGNORE INTO `creature_loot_template` VALUES 
('117', '0', '57755', '30', '1', '1', '0', '1', '1', 'Riverpaw Gnoll Clue'),
('500', '0', '57755', '30', '1', '1', '0', '1', '1', 'Riverpaw Gnoll Clue'),
('126', '0', '57756', '20', '1', '1', '0', '1', '1', 'Murloc Clue'),
('515', '0', '57756', '20', '1', '1', '0', '1', '1', 'Murloc Clue');

-- Creature Text
DELETE FROM creature_text
WHERE CreatureID IN (95, 121, 122, 449, 481, 504, 589, 590, 594); -- Defias
INSERT INTO creature_text 
(CreatureID, GroupID, ID, Text, Type, Language, Probability, Emote, Duration, Sound, SoundPlayType, BroadcastTextId, TextRange, comment) VALUES
(95, 0, 0, "Another fool wandering into Brotherhood territory.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 1, "You look lost… and soon you’ll be dead.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 2, "That purse better be heavier than your brain.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 3, "I sharpened this blade just for punks like you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 4, "You think you’re tough? Cute.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 5, "Westfall belongs to us now — crawl back to Stormwind.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 6, "I’ll gut you before you finish that heroic pose.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 7, "Look at this one… all confidence and no clue.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 8, "The Brotherhood grows stronger while you grow dumber.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 9, "You’re about to regret every life choice that led you here.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 10, "I’ve robbed nobles with more spine than you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 11, "Try not to bleed on my boots — they’re new.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 12, "You’re not worth the bounty on your own head.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 13, "Another hero trying to ‘save’ Westfall. Adorable.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 14, "I’ll make this quick. I’ve got real work to do.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 15, "You walk like someone who’s never been stabbed before.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 16, "I’ll carve my initials into your armor.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 17, "The Brotherhood doesn’t forgive… or forget.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 18, "You’re about to be another missing person in Westfall.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 19, "I’ve mugged murlocs with more fight in them.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 20, "You smell like Stormwind privilege.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 21, "I’ll send your gear to the nearest pawn shop.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 22, "You’re not the first idiot I’ve dropped today.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 23, "Hope you said goodbye to your loved ones.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 24, "You’re in the wrong place at the wrong time, genius.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 25, "I’ll enjoy watching you panic.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 26, "You think justice is coming? It’s already here — in my hands.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 27, "I’ll take your coin, your pride, and maybe your boots.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 28, "You’re about to learn why we run this place.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 29, "I’ve seen scarecrows with more courage.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 30, "You’re not even worth the effort… but I’m bored.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 31, "I’ll make you famous — as a corpse.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 32, "You’re slower than a harvest golem with a broken cog.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 33, "I’ll enjoy telling the others how easily you fell.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 34, "You’re about to be a footnote in Brotherhood history.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 35, "I hope you brought a healer. You’ll need one.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 36, "You’re not walking away from this, friend.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 37, "I’ll take your coin and your dignity in one swing.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 38, "You’re brave… or stupid. Mostly stupid.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 39, "I’ll make sure your last words are embarrassing.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 40, "You’re trespassing. Payment is due in blood.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 41, "I’ve robbed caravans tougher than you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 42, "You’re about to be another stain on the dirt.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 43, "I’ll carve a lesson into you for the others to see.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 44, "You’re not even worth the bounty paperwork.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 45, "I’ll enjoy breaking that smug look off your face.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 46, "You’re in Brotherhood land — start praying.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 47, "I’ll take your coin, your gear, and your hope.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 48, "You’re about to learn why people fear the red bandana.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(95, 0, 49, "I’ll make sure your end is quick… I’m feeling generous.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler");

INSERT INTO creature_text 
(CreatureID, GroupID, ID, Text, Type, Language, Probability, Emote, Duration, Sound, SoundPlayType, BroadcastTextId, TextRange, comment) VALUES
(121, 0, 0, "Another fool wandering into Brotherhood territory.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 1, "You look lost… and soon you’ll be dead.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 2, "That purse better be heavier than your brain.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 3, "I sharpened this blade just for punks like you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 4, "You think you’re tough? Cute.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 5, "Westfall belongs to us now — crawl back to Stormwind.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 6, "I’ll gut you before you finish that heroic pose.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 7, "Look at this one… all confidence and no clue.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 8, "The Brotherhood grows stronger while you grow dumber.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 9, "You’re about to regret every life choice that led you here.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 10, "I’ve robbed nobles with more spine than you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 11, "Try not to bleed on my boots — they’re new.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 12, "You’re not worth the bounty on your own head.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 13, "Another hero trying to ‘save’ Westfall. Adorable.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 14, "I’ll make this quick. I’ve got real work to do.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 15, "You walk like someone who’s never been stabbed before.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 16, "I’ll carve my initials into your armor.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 17, "The Brotherhood doesn’t forgive… or forget.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 18, "You’re about to be another missing person in Westfall.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 19, "I’ve mugged murlocs with more fight in them.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 20, "You smell like Stormwind privilege.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 21, "I’ll send your gear to the nearest pawn shop.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 22, "You’re not the first idiot I’ve dropped today.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 23, "Hope you said goodbye to your loved ones.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 24, "You’re in the wrong place at the wrong time, genius.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 25, "I’ll enjoy watching you panic.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 26, "You think justice is coming? It’s already here — in my hands.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 27, "I’ll take your coin, your pride, and maybe your boots.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 28, "You’re about to learn why we run this place.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 29, "I’ve seen scarecrows with more courage.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 30, "You’re not even worth the effort… but I’m bored.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 31, "I’ll make you famous — as a corpse.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 32, "You’re slower than a harvest golem with a broken cog.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 33, "I’ll enjoy telling the others how easily you fell.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 34, "You’re about to be a footnote in Brotherhood history.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 35, "I hope you brought a healer. You’ll need one.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 36, "You’re not walking away from this, friend.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 37, "I’ll take your coin and your dignity in one swing.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 38, "You’re brave… or stupid. Mostly stupid.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 39, "I’ll make sure your last words are embarrassing.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 40, "You’re trespassing. Payment is due in blood.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 41, "I’ve robbed caravans tougher than you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 42, "You’re about to be another stain on the dirt.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 43, "I’ll carve a lesson into you for the others to see.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 44, "You’re not even worth the bounty paperwork.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 45, "I’ll enjoy breaking that smug look off your face.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 46, "You’re in Brotherhood land — start praying.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 47, "I’ll take your coin, your gear, and your hope.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 48, "You’re about to learn why people fear the red bandana.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(121, 0, 49, "I’ll make sure your end is quick… I’m feeling generous.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler");

INSERT INTO creature_text 
(CreatureID, GroupID, ID, Text, Type, Language, Probability, Emote, Duration, Sound, SoundPlayType, BroadcastTextId, TextRange, comment) VALUES
(122, 0, 0, "Another fool wandering into Brotherhood territory.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 1, "You look lost… and soon you’ll be dead.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 2, "That purse better be heavier than your brain.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 3, "I sharpened this blade just for punks like you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 4, "You think you’re tough? Cute.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 5, "Westfall belongs to us now — crawl back to Stormwind.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 6, "I’ll gut you before you finish that heroic pose.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 7, "Look at this one… all confidence and no clue.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 8, "The Brotherhood grows stronger while you grow dumber.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 9, "You’re about to regret every life choice that led you here.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 10, "I’ve robbed nobles with more spine than you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 11, "Try not to bleed on my boots — they’re new.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 12, "You’re not worth the bounty on your own head.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 13, "Another hero trying to ‘save’ Westfall. Adorable.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 14, "I’ll make this quick. I’ve got real work to do.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 15, "You walk like someone who’s never been stabbed before.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 16, "I’ll carve my initials into your armor.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 17, "The Brotherhood doesn’t forgive… or forget.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 18, "You’re about to be another missing person in Westfall.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 19, "I’ve mugged murlocs with more fight in them.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 20, "You smell like Stormwind privilege.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 21, "I’ll send your gear to the nearest pawn shop.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 22, "You’re not the first idiot I’ve dropped today.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 23, "Hope you said goodbye to your loved ones.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 24, "You’re in the wrong place at the wrong time, genius.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 25, "I’ll enjoy watching you panic.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 26, "You think justice is coming? It’s already here — in my hands.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 27, "I’ll take your coin, your pride, and maybe your boots.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 28, "You’re about to learn why we run this place.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 29, "I’ve seen scarecrows with more courage.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 30, "You’re not even worth the effort… but I’m bored.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 31, "I’ll make you famous — as a corpse.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 32, "You’re slower than a harvest golem with a broken cog.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 33, "I’ll enjoy telling the others how easily you fell.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 34, "You’re about to be a footnote in Brotherhood history.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 35, "I hope you brought a healer. You’ll need one.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 36, "You’re not walking away from this, friend.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 37, "I’ll take your coin and your dignity in one swing.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 38, "You’re brave… or stupid. Mostly stupid.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 39, "I’ll make sure your last words are embarrassing.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 40, "You’re trespassing. Payment is due in blood.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 41, "I’ve robbed caravans tougher than you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 42, "You’re about to be another stain on the dirt.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 43, "I’ll carve a lesson into you for the others to see.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 44, "You’re not even worth the bounty paperwork.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 45, "I’ll enjoy breaking that smug look off your face.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 46, "You’re in Brotherhood land — start praying.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 47, "I’ll take your coin, your gear, and your hope.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 48, "You’re about to learn why people fear the red bandana.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(122, 0, 49, "I’ll make sure your end is quick… I’m feeling generous.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler");

INSERT INTO creature_text 
(CreatureID, GroupID, ID, Text, Type, Language, Probability, Emote, Duration, Sound, SoundPlayType, BroadcastTextId, TextRange, comment) VALUES
(449, 0, 0, "Another fool wandering into Brotherhood territory.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 1, "You look lost… and soon you’ll be dead.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 2, "That purse better be heavier than your brain.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 3, "I sharpened this blade just for punks like you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 4, "You think you’re tough? Cute.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 5, "Westfall belongs to us now — crawl back to Stormwind.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 6, "I’ll gut you before you finish that heroic pose.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 7, "Look at this one… all confidence and no clue.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 8, "The Brotherhood grows stronger while you grow dumber.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 9, "You’re about to regret every life choice that led you here.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 10, "I’ve robbed nobles with more spine than you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 11, "Try not to bleed on my boots — they’re new.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 12, "You’re not worth the bounty on your own head.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 13, "Another hero trying to ‘save’ Westfall. Adorable.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 14, "I’ll make this quick. I’ve got real work to do.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 15, "You walk like someone who’s never been stabbed before.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 16, "I’ll carve my initials into your armor.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 17, "The Brotherhood doesn’t forgive… or forget.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 18, "You’re about to be another missing person in Westfall.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 19, "I’ve mugged murlocs with more fight in them.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 20, "You smell like Stormwind privilege.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 21, "I’ll send your gear to the nearest pawn shop.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 22, "You’re not the first idiot I’ve dropped today.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 23, "Hope you said goodbye to your loved ones.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 24, "You’re in the wrong place at the wrong time, genius.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 25, "I’ll enjoy watching you panic.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 26, "You think justice is coming? It’s already here — in my hands.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 27, "I’ll take your coin, your pride, and maybe your boots.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 28, "You’re about to learn why we run this place.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 29, "I’ve seen scarecrows with more courage.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 30, "You’re not even worth the effort… but I’m bored.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 31, "I’ll make you famous — as a corpse.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 32, "You’re slower than a harvest golem with a broken cog.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 33, "I’ll enjoy telling the others how easily you fell.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 34, "You’re about to be a footnote in Brotherhood history.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 35, "I hope you brought a healer. You’ll need one.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 36, "You’re not walking away from this, friend.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 37, "I’ll take your coin and your dignity in one swing.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 38, "You’re brave… or stupid. Mostly stupid.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 39, "I’ll make sure your last words are embarrassing.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 40, "You’re trespassing. Payment is due in blood.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 41, "I’ve robbed caravans tougher than you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 42, "You’re about to be another stain on the dirt.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 43, "I’ll carve a lesson into you for the others to see.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 44, "You’re not even worth the bounty paperwork.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 45, "I’ll enjoy breaking that smug look off your face.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 46, "You’re in Brotherhood land — start praying.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 47, "I’ll take your coin, your gear, and your hope.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 48, "You’re about to learn why people fear the red bandana.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(449, 0, 49, "I’ll make sure your end is quick… I’m feeling generous.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler");

INSERT INTO creature_text 
(CreatureID, GroupID, ID, Text, Type, Language, Probability, Emote, Duration, Sound, SoundPlayType, BroadcastTextId, TextRange, comment) VALUES
(481, 0, 0, "Another fool wandering into Brotherhood territory.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 1, "You look lost… and soon you’ll be dead.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 2, "That purse better be heavier than your brain.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 3, "I sharpened this blade just for punks like you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 4, "You think you’re tough? Cute.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 5, "Westfall belongs to us now — crawl back to Stormwind.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 6, "I’ll gut you before you finish that heroic pose.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 7, "Look at this one… all confidence and no clue.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 8, "The Brotherhood grows stronger while you grow dumber.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 9, "You’re about to regret every life choice that led you here.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 10, "I’ve robbed nobles with more spine than you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 11, "Try not to bleed on my boots — they’re new.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 12, "You’re not worth the bounty on your own head.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 13, "Another hero trying to ‘save’ Westfall. Adorable.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 14, "I’ll make this quick. I’ve got real work to do.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 15, "You walk like someone who’s never been stabbed before.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 16, "I’ll carve my initials into your armor.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 17, "The Brotherhood doesn’t forgive… or forget.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 18, "You’re about to be another missing person in Westfall.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 19, "I’ve mugged murlocs with more fight in them.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 20, "You smell like Stormwind privilege.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 21, "I’ll send your gear to the nearest pawn shop.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 22, "You’re not the first idiot I’ve dropped today.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 23, "Hope you said goodbye to your loved ones.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 24, "You’re in the wrong place at the wrong time, genius.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 25, "I’ll enjoy watching you panic.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 26, "You think justice is coming? It’s already here — in my hands.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 27, "I’ll take your coin, your pride, and maybe your boots.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 28, "You’re about to learn why we run this place.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 29, "I’ve seen scarecrows with more courage.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 30, "You’re not even worth the effort… but I’m bored.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 31, "I’ll make you famous — as a corpse.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 32, "You’re slower than a harvest golem with a broken cog.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 33, "I’ll enjoy telling the others how easily you fell.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 34, "You’re about to be a footnote in Brotherhood history.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 35, "I hope you brought a healer. You’ll need one.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 36, "You’re not walking away from this, friend.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 37, "I’ll take your coin and your dignity in one swing.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 38, "You’re brave… or stupid. Mostly stupid.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 39, "I’ll make sure your last words are embarrassing.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 40, "You’re trespassing. Payment is due in blood.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 41, "I’ve robbed caravans tougher than you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 42, "You’re about to be another stain on the dirt.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 43, "I’ll carve a lesson into you for the others to see.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 44, "You’re not even worth the bounty paperwork.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 45, "I’ll enjoy breaking that smug look off your face.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 46, "You’re in Brotherhood land — start praying.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 47, "I’ll take your coin, your gear, and your hope.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 48, "You’re about to learn why people fear the red bandana.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(481, 0, 49, "I’ll make sure your end is quick… I’m feeling generous.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler");

INSERT INTO creature_text 
(CreatureID, GroupID, ID, Text, Type, Language, Probability, Emote, Duration, Sound, SoundPlayType, BroadcastTextId, TextRange, comment) VALUES
(504, 0, 0, "Another fool wandering into Brotherhood territory.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 1, "You look lost… and soon you’ll be dead.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 2, "That purse better be heavier than your brain.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 3, "I sharpened this blade just for punks like you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 4, "You think you’re tough? Cute.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 5, "Westfall belongs to us now — crawl back to Stormwind.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 6, "I’ll gut you before you finish that heroic pose.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 7, "Look at this one… all confidence and no clue.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 8, "The Brotherhood grows stronger while you grow dumber.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 9, "You’re about to regret every life choice that led you here.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 10, "I’ve robbed nobles with more spine than you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 11, "Try not to bleed on my boots — they’re new.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 12, "You’re not worth the bounty on your own head.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 13, "Another hero trying to ‘save’ Westfall. Adorable.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 14, "I’ll make this quick. I’ve got real work to do.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 15, "You walk like someone who’s never been stabbed before.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 16, "I’ll carve my initials into your armor.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 17, "The Brotherhood doesn’t forgive… or forget.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 18, "You’re about to be another missing person in Westfall.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 19, "I’ve mugged murlocs with more fight in them.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 20, "You smell like Stormwind privilege.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 21, "I’ll send your gear to the nearest pawn shop.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 22, "You’re not the first idiot I’ve dropped today.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 23, "Hope you said goodbye to your loved ones.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 24, "You’re in the wrong place at the wrong time, genius.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 25, "I’ll enjoy watching you panic.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 26, "You think justice is coming? It’s already here — in my hands.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 27, "I’ll take your coin, your pride, and maybe your boots.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 28, "You’re about to learn why we run this place.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 29, "I’ve seen scarecrows with more courage.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 30, "You’re not even worth the effort… but I’m bored.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 31, "I’ll make you famous — as a corpse.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 32, "You’re slower than a harvest golem with a broken cog.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 33, "I’ll enjoy telling the others how easily you fell.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 34, "You’re about to be a footnote in Brotherhood history.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 35, "I hope you brought a healer. You’ll need one.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 36, "You’re not walking away from this, friend.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 37, "I’ll take your coin and your dignity in one swing.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 38, "You’re brave… or stupid. Mostly stupid.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 39, "I’ll make sure your last words are embarrassing.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 40, "You’re trespassing. Payment is due in blood.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 41, "I’ve robbed caravans tougher than you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 42, "You’re about to be another stain on the dirt.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 43, "I’ll carve a lesson into you for the others to see.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 44, "You’re not even worth the bounty paperwork.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 45, "I’ll enjoy breaking that smug look off your face.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 46, "You’re in Brotherhood land — start praying.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 47, "I’ll take your coin, your gear, and your hope.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 48, "You’re about to learn why people fear the red bandana.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(504, 0, 49, "I’ll make sure your end is quick… I’m feeling generous.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler");

INSERT INTO creature_text 
(CreatureID, GroupID, ID, Text, Type, Language, Probability, Emote, Duration, Sound, SoundPlayType, BroadcastTextId, TextRange, comment) VALUES
(589, 0, 0, "Another fool wandering into Brotherhood territory.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 1, "You look lost… and soon you’ll be dead.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 2, "That purse better be heavier than your brain.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 3, "I sharpened this blade just for punks like you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 4, "You think you’re tough? Cute.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 5, "Westfall belongs to us now — crawl back to Stormwind.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 6, "I’ll gut you before you finish that heroic pose.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 7, "Look at this one… all confidence and no clue.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 8, "The Brotherhood grows stronger while you grow dumber.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 9, "You’re about to regret every life choice that led you here.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 10, "I’ve robbed nobles with more spine than you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 11, "Try not to bleed on my boots — they’re new.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 12, "You’re not worth the bounty on your own head.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 13, "Another hero trying to ‘save’ Westfall. Adorable.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 14, "I’ll make this quick. I’ve got real work to do.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 15, "You walk like someone who’s never been stabbed before.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 16, "I’ll carve my initials into your armor.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 17, "The Brotherhood doesn’t forgive… or forget.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 18, "You’re about to be another missing person in Westfall.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 19, "I’ve mugged murlocs with more fight in them.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 20, "You smell like Stormwind privilege.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 21, "I’ll send your gear to the nearest pawn shop.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 22, "You’re not the first idiot I’ve dropped today.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 23, "Hope you said goodbye to your loved ones.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 24, "You’re in the wrong place at the wrong time, genius.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 25, "I’ll enjoy watching you panic.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 26, "You think justice is coming? It’s already here — in my hands.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 27, "I’ll take your coin, your pride, and maybe your boots.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 28, "You’re about to learn why we run this place.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 29, "I’ve seen scarecrows with more courage.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 30, "You’re not even worth the effort… but I’m bored.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 31, "I’ll make you famous — as a corpse.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 32, "You’re slower than a harvest golem with a broken cog.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 33, "I’ll enjoy telling the others how easily you fell.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 34, "You’re about to be a footnote in Brotherhood history.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 35, "I hope you brought a healer. You’ll need one.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 36, "You’re not walking away from this, friend.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 37, "I’ll take your coin and your dignity in one swing.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 38, "You’re brave… or stupid. Mostly stupid.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 39, "I’ll make sure your last words are embarrassing.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 40, "You’re trespassing. Payment is due in blood.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 41, "I’ve robbed caravans tougher than you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 42, "You’re about to be another stain on the dirt.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 43, "I’ll carve a lesson into you for the others to see.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 44, "You’re not even worth the bounty paperwork.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 45, "I’ll enjoy breaking that smug look off your face.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 46, "You’re in Brotherhood land — start praying.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 47, "I’ll take your coin, your gear, and your hope.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 48, "You’re about to learn why people fear the red bandana.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(589, 0, 49, "I’ll make sure your end is quick… I’m feeling generous.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler");

INSERT INTO creature_text 
(CreatureID, GroupID, ID, Text, Type, Language, Probability, Emote, Duration, Sound, SoundPlayType, BroadcastTextId, TextRange, comment) VALUES
(594, 0, 0, "Another fool wandering into Brotherhood territory.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 1, "You look lost… and soon you’ll be dead.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 2, "That purse better be heavier than your brain.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 3, "I sharpened this blade just for punks like you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 4, "You think you’re tough? Cute.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 5, "Westfall belongs to us now — crawl back to Stormwind.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 6, "I’ll gut you before you finish that heroic pose.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 7, "Look at this one… all confidence and no clue.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 8, "The Brotherhood grows stronger while you grow dumber.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 9, "You’re about to regret every life choice that led you here.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 10, "I’ve robbed nobles with more spine than you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 11, "Try not to bleed on my boots — they’re new.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 12, "You’re not worth the bounty on your own head.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 13, "Another hero trying to ‘save’ Westfall. Adorable.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 14, "I’ll make this quick. I’ve got real work to do.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 15, "You walk like someone who’s never been stabbed before.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 16, "I’ll carve my initials into your armor.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 17, "The Brotherhood doesn’t forgive… or forget.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 18, "You’re about to be another missing person in Westfall.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 19, "I’ve mugged murlocs with more fight in them.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 20, "You smell like Stormwind privilege.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 21, "I’ll send your gear to the nearest pawn shop.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 22, "You’re not the first idiot I’ve dropped today.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 23, "Hope you said goodbye to your loved ones.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 24, "You’re in the wrong place at the wrong time, genius.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 25, "I’ll enjoy watching you panic.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 26, "You think justice is coming? It’s already here — in my hands.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 27, "I’ll take your coin, your pride, and maybe your boots.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 28, "You’re about to learn why we run this place.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 29, "I’ve seen scarecrows with more courage.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 30, "You’re not even worth the effort… but I’m bored.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 31, "I’ll make you famous — as a corpse.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 32, "You’re slower than a harvest golem with a broken cog.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 33, "I’ll enjoy telling the others how easily you fell.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 34, "You’re about to be a footnote in Brotherhood history.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 35, "I hope you brought a healer. You’ll need one.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 36, "You’re not walking away from this, friend.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 37, "I’ll take your coin and your dignity in one swing.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 38, "You’re brave… or stupid. Mostly stupid.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 39, "I’ll make sure your last words are embarrassing.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 40, "You’re trespassing. Payment is due in blood.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 41, "I’ve robbed caravans tougher than you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 42, "You’re about to be another stain on the dirt.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 43, "I’ll carve a lesson into you for the others to see.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 44, "You’re not even worth the bounty paperwork.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 45, "I’ll enjoy breaking that smug look off your face.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 46, "You’re in Brotherhood land — start praying.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 47, "I’ll take your coin, your gear, and your hope.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 48, "You’re about to learn why people fear the red bandana.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler"),
(594, 0, 49, "I’ll make sure your end is quick… I’m feeling generous.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Defias Smuggler");

DELETE FROM creature_text
WHERE CreatureID IN (98, 117, 123, 124, 452, 453, 500, 501, 1065, 42653); -- Riverpaw

INSERT INTO creature_text 
(CreatureID, GroupID, ID, Text, Type, Language, Probability, Emote, Duration, Sound, SoundPlayType, BroadcastTextId, TextRange, comment) VALUES
(42653, 0, 0, "Your life’s worth more to me on the ground than standing in my way.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Riverpaw Jango"),
(42653, 0, 1, "My pack? Hah! They’re tools. You’re just another broken one.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Riverpaw Jango"),
(42653, 0, 2, "Coins, power, fear… that’s all that matters. You’re nothing in that list.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Riverpaw Jango"),
(42653, 0, 3, "I don’t need loyalty. I need victims — starting with you.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Riverpaw Jango"),
(42653, 0, 4, "Move aside, whelp. Jango takes what he wants, and today I want you gone.", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Riverpaw Jango");

INSERT INTO creature_text 
(CreatureID, GroupID, ID, Text, Type, Language, Probability, Emote, Duration, Sound, SoundPlayType, BroadcastTextId, TextRange, comment) VALUES
(42653, 1, 0, "No… this can’t be! I was meant to rule… not fall to you!", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Riverpaw Jango - Death"),
(42653, 1, 1, "My strength… failing? Impossible… I am Jango!", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Riverpaw Jango - Death"),
(42653, 1, 2, "Gnolls… worthless! None of you were worth my power…", 12, 0, 100, 0, 0, 0, 0, 0, 0, "Riverpaw Jango - Death");

INSERT INTO creature_text 
(CreatureID, GroupID, ID, Text, Type, Language, Probability, Emote, Duration, Sound, SoundPlayType, BroadcastTextId, TextRange, comment) VALUES
(98, 0, 0, "Work harder, bleed faster! The Taskmaster demands it!", 12,0,100,0,0,0,0,0,0,"Riverpaw Taskmaster"),
(98, 0, 1, "Lazy whelp! I’ll beat strength into your bones!", 12,0,100,0,0,0,0,0,0,"Riverpaw Taskmaster"),
(98, 0, 2, "You dare interrupt my laborers? I’ll break you!", 12,0,100,0,0,0,0,0,0,"Riverpaw Taskmaster"),
(98, 0, 3, "Another weakling to whip into the dirt!", 12,0,100,0,0,0,0,0,0,"Riverpaw Taskmaster"),
(98, 0, 4, "I’ll make you dig your own grave!", 12,0,100,0,0,0,0,0,0,"Riverpaw Taskmaster"),
(98, 0, 5, "You look soft… perfect for breaking!", 12,0,100,0,0,0,0,0,0,"Riverpaw Taskmaster"),
(98, 0, 6, "My workers die for me. You will too!", 12,0,100,0,0,0,0,0,0,"Riverpaw Taskmaster"),
(98, 0, 7, "I’ll whip the courage out of you!", 12,0,100,0,0,0,0,0,0,"Riverpaw Taskmaster"),
(98, 0, 8, "You’re nothing but another body for the pits!", 12,0,100,0,0,0,0,0,0,"Riverpaw Taskmaster"),
(98, 0, 9, "Fall in line or fall in pieces!", 12,0,100,0,0,0,0,0,0,"Riverpaw Taskmaster");

INSERT INTO creature_text VALUES
(117,0,0,"Gnoll hungry! You look tasty!",12,0,100,0,0,0,0,0,0,"Riverpaw Gnoll"),
(117,0,1,"Fresh meat! Fresh screams!",12,0,100,0,0,0,0,0,0,"Riverpaw Gnoll"),
(117,0,2,"You smell weak! Weak dies first!",12,0,100,0,0,0,0,0,0,"Riverpaw Gnoll"),
(117,0,3,"Riverpaw bite! Riverpaw kill!",12,0,100,0,0,0,0,0,0,"Riverpaw Gnoll"),
(117,0,4,"You fight? Good! Gnoll bored!",12,0,100,0,0,0,0,0,0,"Riverpaw Gnoll"),
(117,0,5,"Your bones will crack nice!",12,0,100,0,0,0,0,0,0,"Riverpaw Gnoll"),
(117,0,6,"You bleed now!",12,0,100,0,0,0,0,0,0,"Riverpaw Gnoll"),
(117,0,7,"Gnoll smash little hero!",12,0,100,0,0,0,0,0,0,"Riverpaw Gnoll"),
(117,0,8,"You slow! Easy kill!",12,0,100,0,0,0,0,0,0,"Riverpaw Gnoll"),
(117,0,9,"Riverpaw strong! You not!",12,0,100,0,0,0,0,0,0,"Riverpaw Gnoll");

INSERT INTO creature_text VALUES
(123,0,0,"Mongrel bite first, think later!",12,0,100,0,0,0,0,0,0,"Riverpaw Mongrel"),
(123,0,1,"You look like easy prey!",12,0,100,0,0,0,0,0,0,"Riverpaw Mongrel"),
(123,0,2,"Mongrel tear you apart!",12,0,100,0,0,0,0,0,0,"Riverpaw Mongrel"),
(123,0,3,"You squeal good for Mongrel!",12,0,100,0,0,0,0,0,0,"Riverpaw Mongrel"),
(123,0,4,"Fight Mongrel? Bad idea!",12,0,100,0,0,0,0,0,0,"Riverpaw Mongrel"),
(123,0,5,"Your fear smells sweet!",12,0,100,0,0,0,0,0,0,"Riverpaw Mongrel"),
(123,0,6,"Mongrel chew your bones!",12,0,100,0,0,0,0,0,0,"Riverpaw Mongrel"),
(123,0,7,"You run? Mongrel chase!",12,0,100,0,0,0,0,0,0,"Riverpaw Mongrel"),
(123,0,8,"Mongrel smash puny thing!",12,0,100,0,0,0,0,0,0,"Riverpaw Mongrel"),
(123,0,9,"You die fast or slow? Mongrel choose!",12,0,100,0,0,0,0,0,0,"Riverpaw Mongrel");

INSERT INTO creature_text VALUES
(124,0,0,"Brute crush little hero!",12,0,100,0,0,0,0,0,0,"Riverpaw Brute"),
(124,0,1,"You tiny. Brute strong!",12,0,100,0,0,0,0,0,0,"Riverpaw Brute"),
(124,0,2,"Brute make paste out of you!",12,0,100,0,0,0,0,0,0,"Riverpaw Brute"),
(124,0,3,"You break easy!",12,0,100,0,0,0,0,0,0,"Riverpaw Brute"),
(124,0,4,"Brute hungry for smashing!",12,0,100,0,0,0,0,0,0,"Riverpaw Brute"),
(124,0,5,"Your skull look smashable!",12,0,100,0,0,0,0,0,0,"Riverpaw Brute"),
(124,0,6,"Brute stomp you flat!",12,0,100,0,0,0,0,0,0,"Riverpaw Brute"),
(124,0,7,"You fight Brute? Bad choice!",12,0,100,0,0,0,0,0,0,"Riverpaw Brute"),
(124,0,8,"Brute crush bones for fun!",12,0,100,0,0,0,0,0,0,"Riverpaw Brute"),
(124,0,9,"Brute show you real pain!",12,0,100,0,0,0,0,0,0,"Riverpaw Brute");

INSERT INTO creature_text VALUES
(452,0,0,"Hand over coin or hand over life!",12,0,100,0,0,0,0,0,0,"Riverpaw Bandit"),
(452,0,1,"Bandit take everything you got!",12,0,100,0,0,0,0,0,0,"Riverpaw Bandit"),
(452,0,2,"Your pockets look heavy!",12,0,100,0,0,0,0,0,0,"Riverpaw Bandit"),
(452,0,3,"Give loot or give blood!",12,0,100,0,0,0,0,0,0,"Riverpaw Bandit"),
(452,0,4,"Bandit love easy targets!",12,0,100,0,0,0,0,0,0,"Riverpaw Bandit"),
(452,0,5,"You walk alone? Bad idea!",12,0,100,0,0,0,0,0,0,"Riverpaw Bandit"),
(452,0,6,"Bandit take what Bandit wants!",12,0,100,0,0,0,0,0,0,"Riverpaw Bandit"),
(452,0,7,"Your coin mine now!",12,0,100,0,0,0,0,0,0,"Riverpaw Bandit"),
(452,0,8,"You fight? Good! More loot!",12,0,100,0,0,0,0,0,0,"Riverpaw Bandit"),
(452,0,9,"Bandit cut you open for shiny things!",12,0,100,0,0,0,0,0,0,"Riverpaw Bandit");

INSERT INTO creature_text VALUES
(453,0,0,"Spirits whisper your doom!",12,0,100,0,0,0,0,0,0,"Riverpaw Mystic"),
(453,0,1,"Mystic curse your bones!",12,0,100,0,0,0,0,0,0,"Riverpaw Mystic"),
(453,0,2,"You face gnoll magic now!",12,0,100,0,0,0,0,0,0,"Riverpaw Mystic"),
(453,0,3,"Mystic burn your soul!",12,0,100,0,0,0,0,0,0,"Riverpaw Mystic"),
(453,0,4,"Your fate sealed in blood!",12,0,100,0,0,0,0,0,0,"Riverpaw Mystic"),
(453,0,5,"Mystic show you true pain!",12,0,100,0,0,0,0,0,0,"Riverpaw Mystic"),
(453,0,6,"Spirits hungry for you!",12,0,100,0,0,0,0,0,0,"Riverpaw Mystic"),
(453,0,7,"You die under gnoll magic!",12,0,100,0,0,0,0,0,0,"Riverpaw Mystic"),
(453,0,8,"Mystic power crush you!",12,0,100,0,0,0,0,0,0,"Riverpaw Mystic"),
(453,0,9,"Your screams feed the spirits!",12,0,100,0,0,0,0,0,0,"Riverpaw Mystic");

INSERT INTO creature_text VALUES
(500,0,0,"Scout find prey! Scout kill prey!",12,0,100,0,0,0,0,0,0,"Riverpaw Scout"),
(500,0,1,"You not hide from Scout!",12,0,100,0,0,0,0,0,0,"Riverpaw Scout"),
(500,0,2,"Scout track you easy!",12,0,100,0,0,0,0,0,0,"Riverpaw Scout"),
(500,0,3,"You step loud. Scout hear!",12,0,100,0,0,0,0,0,0,"Riverpaw Scout"),
(500,0,4,"Scout bring your head back!",12,0,100,0,0,0,0,0,0,"Riverpaw Scout"),
(500,0,5,"You slow! Scout fast!",12,0,100,0,0,0,0,0,0,"Riverpaw Scout"),
(500,0,6,"Scout hunt you now!",12,0,100,0,0,0,0,0,0,"Riverpaw Scout"),
(500,0,7,"You smell fear!",12,0,100,0,0,0,0,0,0,"Riverpaw Scout"),
(500,0,8,"Scout catch you easy!",12,0,100,0,0,0,0,0,0,"Riverpaw Scout"),
(500,0,9,"You run? Scout chase!",12,0,100,0,0,0,0,0,0,"Riverpaw Scout");

INSERT INTO creature_text VALUES
(501,0,0,"Herbalist mix poison just for you!",12,0,100,0,0,0,0,0,0,"Riverpaw Herbalist"),
(501,0,1,"You interrupt herbs? You die!",12,0,100,0,0,0,0,0,0,"Riverpaw Herbalist"),
(501,0,2,"Herbalist brew your doom!",12,0,100,0,0,0,0,0,0,"Riverpaw Herbalist"),
(501,0,3,"You smell like good ingredient!",12,0,100,0,0,0,0,0,0,"Riverpaw Herbalist"),
(501,0,4,"Herbalist test new toxins on you!",12,0,100,0,0,0,0,0,0,"Riverpaw Herbalist"),
(501,0,5,"You make good fertilizer!",12,0,100,0,0,0,0,0,0,"Riverpaw Herbalist"),
(501,0,6,"Herbalist brew pain!",12,0,100,0,0,0,0,0,0,"Riverpaw Herbalist"),
(501,0,7,"You ruin herbs! Herbalist ruin you!",12,0,100,0,0,0,0,0,0,"Riverpaw Herbalist"),
(501,0,8,"Herbalist cut you for ingredients!",12,0,100,0,0,0,0,0,0,"Riverpaw Herbalist"),
(501,0,9,"You bleed good for potions!",12,0,100,0,0,0,0,0,0,"Riverpaw Herbalist");

INSERT INTO creature_text VALUES
(1065,0,0,"Shaman call storm to crush you!",12,0,100,0,0,0,0,0,0,"Riverpaw Shaman"),
(1065,0,1,"Elements obey Shaman, not you!",12,0,100,0,0,0,0,0,0,"Riverpaw Shaman"),
(1065,0,2,"Shaman burn your flesh!",12,0,100,0,0,0,0,0,0,"Riverpaw Shaman"),
(1065,0,3,"You face gnoll fury and storm!",12,0,100,0,0,0,0,0,0,"Riverpaw Shaman"),
(1065,0,4,"Shaman tear spirit from body!",12,0,100,0,0,0,0,0,0,"Riverpaw Shaman"),
(1065,0,5,"Storm hungry for your bones!",12,0,100,0,0,0,0,0,0,"Riverpaw Shaman"),
(1065,0,6,"Shaman crush you with earth and flame!",12,0,100,0,0,0,0,0,0,"Riverpaw Shaman"),
(1065,0,7,"You die under storm power!",12,0,100,0,0,0,0,0,0,"Riverpaw Shaman"),
(1065,0,8,"Shaman show you true gnoll magic!",12,0,100,0,0,0,0,0,0,"Riverpaw Shaman"),
(1065,0,9,"Your spirit weak! Shaman take it!",12,0,100,0,0,0,0,0,0,"Riverpaw Shaman");