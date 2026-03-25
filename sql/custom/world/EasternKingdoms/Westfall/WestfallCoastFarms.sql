-- Westfall coast and farm areas changes and updates

-- NPC: 117 Riverpaw Gnoll
-- NPC: 199 Young Fleshripper
-- NPC: 233 Farmer Saldean
-- NPC: 234 Gryan Stoutmantle
-- NPC: 235 Salma Saldean
-- NPC: 237 Farmer Furlbrow
-- NPC: 238 Verna Furlbrow
-- NPC: 454 Young Goretusk
-- NPC: 462 Vultros
-- NPC: 500 Riverpaw Scout
-- NPC: 582 Old Blanchy
-- NPC: 834 Coyote
-- NPC: 1109 Fleshripper
-- NPC: 42383 Transient
-- NPC: 42384 Homeless Stormwind Citizen
-- NPC: 42386 Homeless Stormwind Citizen (Female)
-- NPC: 42391 West Plains Drifter
-- NPC: 900000 copy of Farmer Furlbrow for phasing
-- NPC: 900001 copy of Verna Furlbrow
-- NPC: 900002 copy of Old Blanchy

-- Quest: 36 Westfall Stew
-- Quest: 64 The Forgotten Heirloom
-- Quest: 109 Report to Gryan Stoutmantle
-- Quest: 151 Poor Old Blanchy
-- Quest: 184 Furlbrows Deed
-- Quest: 26209 Murder Was The Case That They Gave Me

-- Reactivating old/deprecated quests in Stormwind
DELETE FROM `disables` WHERE `entry` IN (36, 64, 109, 151) AND `sourceType` = 1; -- quest source
DELETE FROM `creature_queststarter` WHERE `quest` IN (36, 109, 64, 151);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES 
('900000', '64', '0'),
('900001', '36', '0'),
('900001', '151', '0'),
('900000', '109', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (36, 109, 64, 151, 184);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES 
('900000', '64', '0'),
('900000', '184', '0'),
('235', '36', '0'),
('900001', '151', '0'),
('234', '109', '0');


-- custom phases for Westfall
DELETE FROM `phase_area` WHERE `PhaseId` IN (50004, 50005);
DELETE FROM `phase_area` WHERE `PhaseId` = 169 AND `AreaId` IN (916);
INSERT INTO `phase_area` VALUES
(916, 169, 'Westfall The Jansen Stead - Furlbrows'),
(916, 50004, 'Westfall The Jansen Stead - Furlbrows 1'),
(916, 50005, 'Westfall The Jansen Stead - Furlbrows 2');

-- Phase Conditions
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` IN (50004, 50005);
INSERT INTO `conditions` VALUES
('26', '50004', '916', '0', '0', '47', '0', '109', '64', '0', '', '1', '0', '0', '', 'Westfall - Add phase 50004 if 109 IS NOT rewarded'),
('26', '50005', '916', '0', '0', '47', '0', '109', '64', '0', '', '0', '0', '0', '', 'Westfall - Add phase 50005 if 109 IS rewarded');

-- Quest Templates
UPDATE `quest_template` SET `ContentTuningID` = '6', `Expansion` = '0' WHERE `ID` IN (36, 109, 64, 151);
DELETE FROM `quest_template_addon` WHERE `ID` IN (109);
INSERT INTO `quest_template_addon` (`ID`, `PrevQuestID`) VALUES
(109, 64);

-- Creature templates
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
INSERT INTO `creature` (guid, id, map, zoneId, areaId, spawnDifficulties, phaseUseFlags, PhaseId, PhaseGroup, terrainSwapMap, modelid, equipment_id, position_x, position_y, position_z, orientation, spawntimesecs, wander_distance, currentwaypoint, curHealthPct, MovementType, npcflag, unit_flags, unit_flags2, unit_flags3, ScriptName, StringId, VerifiedBuild) VALUES
(@CGUID+128, '900000', '0', '40', '916', '0', '0', '50004', '0', '-1', '0', '0', '-9845.67', '908.615', '30.2339', '1.25925', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0'),
(@CGUID+129, '900001', '0', '40', '916', '0', '0', '50004', '0', '-1', '0', '0', '-9846.88', '909.397', '30.2058', '0.529411', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0'),
(@CGUID+130, '900002', '0', '40', '916', '0', '0', '50004', '0', '-1', '0', '0', '-9848.07', '912.605', '30.3397', '5.49706', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0');


-- Update phase for existing creatures
UPDATE `creature` SET `PhaseId` = 50005 WHERE `guid` IN (276241, 276256, 276238, 276261, 276262, 276239, 276257, 276258, 276259, 276260);