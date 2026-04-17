-- NPC: 60858 Deepscale Fleshripper (phase 544)
-- NPC: 60780 Deepscale Ravager (phase 544)
-- NPC: 56360 Deepscale Tormentor (phase 1835)
-- NPC: 60722 Deepscale Tormentor
-- NPC: 56007 Darkened Horror (phase 1835)
-- NPC: 56008 Darkened Terror (phase 1835)
-- NPC: 60685 Deepscale Aggressor
-- NPC: 56009 Vordraka
-- (60858, 60780, 56360, 60722, 56007, 56008, 60685, 56009)
-- NPC: 53705 Pangolin
-- NPC: 55946 Pei-wu tiger
-- NPC: 56172 Ash moth
-- NPC: 56174 Tiger cub

-- NPC: 55940 Jojo at the Alliance camp
-- NPC: 55942 Ji at the Hut
-- NPC: 55999 Injured Sailor (phase 1835)
-- NPC: 56362 Skyseeker Sailor (phase 1835)
-- NPC: 56476 Injured Sailor Rescue controller (phase 1835)
-- NPC: 56236 Injured Sailor (spawn)
-- NPC: 60897 Injured Sailor
-- NPC: 60900 Ji summoned at the Hut
-- NPC: 56416 Aysa fighting Vordraka
-- NPC: 55943 Wei Palerage
-- NPC: 55944 Delora Lionheart
-- NPC: 60042 Korga
-- NPC: 60055 Makael Bay
-- NPC: 67186 Provisioner Drog
-- NPC: 67185 Provisioner Phelps
-- NPC: 60895 Alliance Priest
-- (55940, 55942, 55999, 56362, 56476, 56236, 60897, 60900, 56416, 55943, 55944, 60042, 60055, 67186, 67185, 60895)
-- NPC: 60729 Aysa at the explosion
-- NPC: 60741 Ji at the explosion
-- NPC: 56418 Ji after Vordraka
-- NPC: 57739 Ji After healed Shenzinsu
-- NPC: 56013 Spirit of Master Shang
-- NPC: 60877 Alliance Priest
-- NPC: 60878 Alliance Priest
-- NPC: 60770 Horde Druid
-- NPC: 60834 Horde Druid

-- Spells: 117600 Summon Ji Cinematic Intro
-- Spells: 117499 Summon Aysa Cinematic Intro

-- Quest: 29665 From bad to worse
-- Quest: 29793 Evil from the seas
-- Quest: 29794 None left behind
-- Quest: 29795 Stocking Stalks
-- Quest: 29796 Urgent News
-- Quest: 29797 Medical Supplies
-- Quest: 29798 An ancient evil
-- Quest: 29799 The Healing of Shen-zin Su
-- Quest: 29800 New Allies
-- Quest: 30589 Wrecking the Wreck
-- Quest: 30590 Handle with care
-- Quest: 30591 Praying on the predators
-- Quest: 30767 Risking it all


DELETE FROM `creature_queststarter` WHERE `quest` IN (30591, 29795, 30589, 29793, 29796, 30590, 29794, 29797, 29665, 29798, 30767, 29799, 29800);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES
('57739', '29800', '0'), 
('56418', '29799', '0'), 
('56416', '30767', '0'),
('55940', '29665', '0'),
('55940', '29798', '0'),
('55942', '29793', '0'),
('55942', '29796', '0'),
('55943', '30591', '0'),
('55944', '29794', '0'),
('55944', '29797', '0'),
('60055', '30590', '0'),
('60042', '29795', '0'),
('60042', '30589', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (30591, 29795, 30589, 29793, 29796, 30590, 29794, 29797, 29665, 29798, 30767, 29799, 29800);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES 
('56013', '29800', '0'), 
('56418', '29799', '0'),
('56418', '30767', '0'),
('56416', '29798', '0'),
('55940', '29665', '0'),
('55942', '29793', '0'),
('55944', '29796', '0'),
('60055', '30590', '0'),
('55943', '30591', '0'),
('55944', '29794', '0'),
('55944', '29797', '0'),
('60042', '29795', '0'),
('60055', '30589', '0');

DELETE FROM `quest_template_addon` WHERE `ID` IN (30591, 29795, 30589, 29793, 29796, 30590, 29794, 29797, 29665, 29798, 30767, 29799, 29800);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('30591', '0', '0', '0', '29792', '30589', '-30591', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29795', '0', '0', '0', '29792', '30589', '-30591', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('30589', '0', '0', '117973', '30591', '30590', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29793', '0', '0', '0', '30589', '29796', '-29793', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29796', '0', '0', '0', '89521', '29793', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29794', '0', '0', '0', '29796', '29798', '-29665', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29797', '0', '0', '0', '29796', '29798', '-29665', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('30590', '0', '0', '0', '30589', '29796', '-29793', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29665', '0', '0', '0', '29796', '29798', '-29665', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29798', '0', '0', '0', '29665', '30767', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('30767', '0', '0', '0', '29798', '29799', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29799', '0', '0', '0', '30767', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29800', '0', '0', '0', '29799', '31450', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `world_state` WHERE `ID` = 6488;
INSERT INTO `world_state` (`ID`, `DefaultValue`, `MapIDs`, `AreaIDs`, `Comment`) VALUES ('6488', '0', '860', '5833', 'The Wandering Isle - Healing Shen-zin Su healers active');

DELETE FROM `phase_area` WHERE `PhaseId` IN (903, 993, 1835, 543, 544, 545);
INSERT INTO `phase_area` VALUES
(5736, 543, 'The Wandering Isle - Vordraka boss fight'),
(5736, 544, 'The Wandering Isle - Before healing Shenzinsu'),
(5736, 545, 'The Wandering Isle - After healing Shenzinsu'),
(5736, 903, 'The Wandering Isle - The Wandering Isle'), -- Forlorn Hut see Ji until quest reward 30589
(5736, 993, 'The Wandering Isle - After Vordraka boss fight'),
(5736, 1835, 'The Wandering Isle - The Wandering Isle'); -- See Ji at Makael Bay

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` IN (543, 903, 993, 1835, 545, 544);
INSERT INTO `conditions` VALUES
('26', '543', '0', '0', '0', '47', '0', '29665', '64', '0', '', '0', '0', '0', '', 'The Wandering Isle Forbidden Forest - Add phase 543 if 29665 IS rewarded'),
('26', '543', '0', '0', '0', '47', '0', '29798', '66', '0', '', '1', '0', '0', '', 'The Wandering Isle Forbidden Forest - Add phase 543 if 29798 IS NOT complete or rewarded'),
('26', '903', '0', '0', '0', '47', '0', '29792', '64', '0', '', '0', '0', '0', '', 'The Wandering Isle Forbidden Forest - Add phase 903 if 29792 IS rewarded'),
('26', '903', '0', '0', '0', '47', '0', '30589', '74', '0', '', '1', '0', '0', '', 'The Wandering Isle Forbidden Forest - Add phase 903 if 30589 IS NOT in progress or complete or rewarded'),
('26', '993', '0', '0', '0', '47', '0', '29798', '66', '0', '', '0', '0', '0', '', 'The Wandering Isle Forbidden Forest - Add phase 993 if 29798 IS complete or rewarded'),
('26', '993', '0', '0', '0', '47', '0', '30767', '66', '0', '', '1', '0', '0', '', 'The Wandering Isle Forbidden Forest - Add phase 993 if 30767 IS NOT complete or rewarded'),
('26', '544', '0', '0', '0', '47', '0', '29798', '66', '0', '', '0', '0', '0', '', 'The Wandering Isle Forbidden Forest - Add phase 544 if 29798 IS in complete or rewarded'),
('26', '544', '0', '0', '0', '47', '0', '29799', '64', '0', '', '1', '0', '0', '', 'The Wandering Isle Forbidden Forest - Add phase 544 if 29799 IS NOT rewarded'),
('26', '545', '0', '0', '0', '47', '0', '29799', '64', '0', '', '0', '0', '0', '', 'The Wandering Isle Forbidden Forest - Add phase 545 if 29799 IS rewarded'),
('26', '1835', '0', '0', '0', '47', '0', '30589', '64', '0', '', '0', '0', '0', '', 'The Wandering Isle Forbidden Forest - Add phase 1835 if 30589 IS rewarded'),
('26', '1835', '0', '0', '0', '47', '0', '29798', '64', '0', '', '1', '0', '0', '', 'The Wandering Isle Forbidden Forest - Add phase 1835 if 29798 IS NOT rewarded');

-- Creature Templates
UPDATE `creature_template` SET `ScriptName` = 'npc_korga_hut' WHERE `entry` = 60042;
UPDATE `creature_template` SET `ScriptName` = 'npc_injured_sailor_55999', `RegenHealth` = '0' WHERE `entry` = 55999;
UPDATE `creature_template` SET `ScriptName` = 'npc_aysa_vordraka_fight' WHERE `entry` = 56416;
UPDATE `creature_template` SET `ScriptName` = 'npc_vordraka' WHERE `entry` = 56009;
UPDATE `creature_template` SET `ScriptName` = 'npc_aysa_explosion' WHERE `entry` = 60729;

DELETE FROM `creature_template_addon` WHERE `entry` IN (55943, 60042, 67186, 55942, 55944, 60055, 60900, 67185, 55999, 56236, 60897, 56362, 56416, 56476, 60895, 55940,
60858, 60780, 56360, 60722, 56007, 56008, 60685, 56009);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureId`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, 
`movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('55943', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '110846'),
('60042', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('67186', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('55942', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('55944', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80797'),
('60055', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('60900', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('67185', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('55999', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '108806'),
('56236', '0', '0', '0', '3', '0', '0', '1', '0', '0', '0', '0', '0', '0', '46598'),
('60897', '0', '0', '0', '3', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('55940', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '115672'),
('56362', '0', '0', '0', '0', '0', '0', '1', '0', '333', '0', '0', '0', '0', ''),
('56416', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '83305'),
('56476', '0', '0', '0', '0', '3', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('60895', '0', '0', '0', '1', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('56007', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('56008', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('56009', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '3', ''),
('56360', '0', '0', '0', '0', '0', '0', '1', '0', '27', '0', '0', '0', '0', ''),
('60685', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('60722', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '96733'),
('60780', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('60858', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '');

UPDATE `creature` SET `PhaseId` = 543, `spawntimesecs` = '360' WHERE `id` IN (56009);
UPDATE `creature` SET `PhaseId` = 0, `PhaseGroup` = 680 WHERE `id` IN (56416);
UPDATE `creature` SET `PhaseId` = '903' WHERE (`guid` = '451895');
UPDATE `creature` SET `PhaseId` = 1835 WHERE `id` IN (56360, 55999, 56362, 56476, 56007, 56008);
UPDATE `creature` SET `PhaseId` = '1835' WHERE (`guid` = '451988');
UPDATE `creature` SET `PhaseId` = '0', `PhaseGroup` = '641' WHERE (`guid` = '451723');
UPDATE `creature` SET `PhaseId` = 544, `terrainSwapMap` = 975 WHERE id IN (60780, 60858, 57317, 56417, 56418, 56419, 60851, 60853, 60854, 60877, 60878, 60873, 60770, 60834, 60852, 60874);
UPDATE `creature` SET `PhaseId` = 545, `terrainSwapMap` = 976 WHERE id IN (60895, 60888, 60896, 60898, 60894, 60893, 60892, 60891, 60890, 60889, 60722, 57743, 57741, 57739);
UPDATE `creature` SET `PhaseId` = 545, `terrainSwapMap` = 976 WHERE guid IN (452340);
UPDATE `creature` SET `PhaseId` = 0, `PhaseGroup` = 641, `terrainSwapMap` = -1 WHERE `guid` IN (451725, 451731, 4001703);
UPDATE `creature` SET `PhaseId` = 169, `terrainSwapMap` = -1 WHERE `guid` IN (451858, 451860, 451965, 451979);
UPDATE `creature` SET `PhaseId` = 544, `ScriptName` = 'npc_healers_active_bunny' WHERE (`guid` = '452176');

DELETE FROM `creature` WHERE `map` = 860 AND `guid` IN (452061, 452110, 452062, 452099, 452114, 452105, 452117, 452351, 452355, 452101, 452111, 452112, 452248, 452229, 452265, 452250,
452252,452254, 452196, 452210, 452268, 452283, 452273, 452253, 452224, 452195, 452266, 452267, 452219, 452237, 452263, 452277, 452211, 452225, 452244, 452262, 452279, 452264); -- duplicate spawns
DELETE FROM `creature` WHERE `map` = 860 AND `id` IN (56236); -- wrong spawns as the sailors are spawned from quest

UPDATE `gameobject` SET `PhaseId` = 1835 WHERE `id` IN (209793);

DELETE FROM `npc_vendor` WHERE `entry` IN (67185, 67186);
INSERT INTO `npc_vendor` (entry, slot, item, maxcount, incrtime, ExtendedCost, `type`, BonusListIDs, PlayerConditionID, IgnoreFiltering, VerifiedBuild) VALUES
('67186', '1', '117', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '7', '159', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '4', '787', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '2', '2287', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '3', '3770', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '5', '4592', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '6', '4593', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '8', '90659', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67186', '9', '90660', '0', '0', '0', '1', NULL, '0', '0', '64154'),
('67185', '1', '159', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('67185', '2', '4536', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('67185', '3', '4538', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('67185', '4', '4540', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('67185', '5', '4541', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('67185', '6', '4542', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('67185', '7', '90659', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('67185', '8', '90660', '0', '0', '0', '1', NULL, '0', '0', '63906');

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (55999);
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES
('55999', '129340', '1', '0');

-- Scripts
DELETE FROM `areatrigger_scripts` WHERE `entry` IN (7714, 7087);
INSERT INTO `areatrigger_scripts` VALUES
(7714, 'at_forlorn_hut_7714'),
(7087, 'at_wreck_of_the_skyseeker_injured_sailor');

DELETE FROM `spell_script_names` WHERE `spell_id` IN (117973, 108806, 129341, 117597, 118233, 117783);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES 
(117973, 'spell_summon_ji_forlorn_hut'),
(129341, 'spell_rescue_injured_sailor'),
(108806, 'aura_injured_sailor_feign_death'),
(117597, 'spell_summon_ji_wreck_explosion'),
(118233, 'spell_turtle_healed_phase_timer'),
(117783, 'spell_healing_shenzin_su');


UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (60900);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (60900, 6090000) AND `source_type` IN (0, 9);
INSERT INTO `smart_scripts` (entryorguid, source_type, id, link, event_type, event_phase_mask, event_chance, event_flags,  event_param1, event_param2, 
event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, 
action_param6, target_type, target_param1, target_param2, target_param3, target_x, target_y, target_z, target_o, comment) VALUES
('60900', '0', '0', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '6090000', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Just Spawned - Run Script'),
('60900', '0', '1', '2', '40', '0', '100', '0', '2', '6090000', '0', '0', '0', '', '54', '3000', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - On WP Reached - Pause WP'),
('60900', '0', '2', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '97', '15', '15', '0', '0', '0', '0', '1', '0', '0', '0', '424.019', '3676.25', '78.6968', '0', 'Ji Firepaw - On WP Reached - Jump to Pos'),
('60900', '0', '3', '4', '58', '0', '100', '0', '22', '6090000', '0', '0', '0', '', '66', '0', '0', '0', '0', '0', '0', '8', '0', '0', '0', '0', '0', '0', '0.401426', 'Ji Firepaw - On WP Ended - Set Orientation'),
('60900', '0', '4', '5', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '28', '117973', '0', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - On WP Ended - Remove Aura'),
('60900', '0', '5', '6', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '44', '1835', '1', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - On WP Ended - Add Phase'),
('60900', '0', '6', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - On WP Ended - Despawn'),
('6090000', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('6090000', '9', '1', '0', '0', '0', '100', '0', '5000', '5000', '1000', '1000', '0', '', '53', '1', '6090000', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Start WP');