-- NPC: 60858 Deepscale Fleshripper (phase 544)
-- NPC: 60780 Deepscale Ravager (phase 544)
-- NPC: 56360 Deepscale Tormentor (phase 1835)
-- NPC: 60722 Deepscale Tormentor
-- NPC: 56007 Darkened Horror (phase 1835)
-- NPC: 56008 Darkened Terror (phase 1835)
-- NPC: 60685 Deepscale Aggressor
-- NPC: 56009 Vordraka
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
-- NPC: 60729 Aysa at the explosion
-- NPC: 60741 Ji at the explosion
-- NPC: 56417 Aysa after explosion?
-- NPC: 56418 Ji after Vordraka
-- NPC: 57739 Ji After healed Shenzinsu
-- NPC: 56013 Spirit of Master Shang
-- NPC: 60877 Alliance Priest
-- NPC: 60878 Alliance Priest
-- NPC: 60770 Horde Druid
-- NPC: 60834 Horde Druid
-- NPC: 60851 Delora Lionheart (during healing)
-- NPC: 60852 Korga (during healing)
-- NPC: 57317 Skyseeker Sailor (during healing)
-- NPC: 60854 Escaped Horde Crewman (during healing)
-- NPC: 60853 Makael Bay (during healing)
-- NPC: 60873 Skyseeker Sailor (during healing)
-- NPC: 56419 Jojo (during healing)
-- NPC: 60889 Delora (after healing)
-- NPC: 60888 Korga (after healing)
-- NPC: 56195 Escaped Horde engineer
-- NPC: 56000 Escaped Horde Crewman

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
('29796', '0', '0', '89521', '29793', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29794', '0', '0', '0', '29796', '29798', '-29665', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29797', '0', '0', '0', '29796', '29798', '-29665', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('30590', '0', '0', '0', '30589', '29796', '-29793', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29665', '0', '0', '0', '29796', '29798', '-29665', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29798', '0', '0', '0', '29665', '30767', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('30767', '0', '0', '0', '29798', '29799', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29799', '0', '0', '0', '30767', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29800', '0', '0', '0', '29799', '31450', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'quest_29800_new_allies');

DELETE FROM `world_state` WHERE `ID` IN (6488, 6489);
INSERT INTO `world_state` (`ID`, `DefaultValue`, `MapIDs`, `AreaIDs`, `Comment`) VALUES 
('6488', '0', '860', '5833', 'The Wandering Isle - Healing Shen-zin Su healers active'),
('6489', '1', '860', '5833', 'The Wandering Isle - Healing Shen-zin Su healers enabled');

-- Creature Templates
UPDATE `creature_template` SET `ScriptName` = 'npc_korga_hut' WHERE `entry` = 60042;
UPDATE `creature_template` SET `ScriptName` = 'npc_injured_sailor_55999', `RegenHealth` = '0' WHERE `entry` = 55999;
UPDATE `creature_template` SET `ScriptName` = 'npc_aysa_vordraka_fight' WHERE `entry` = 56416;
UPDATE `creature_template` SET `ScriptName` = 'npc_vordraka' WHERE `entry` = 56009;
UPDATE `creature_template` SET `ScriptName` = 'npc_aysa_explosion' WHERE `entry` = 60729;
UPDATE `creature_template` SET `ScriptName` = 'npc_shenzin_su_healer' WHERE `entry` IN (60877, 60878, 60770, 60834);


UPDATE `creature` SET `PhaseId` = 543, `spawntimesecs` = '360' WHERE `id` IN (56009);
UPDATE `creature` SET `PhaseId` = 0, `PhaseGroup` = 680 WHERE `id` IN (56416);
UPDATE `creature` SET `PhaseId` = '903' WHERE (`guid` = '451895');
UPDATE `creature` SET `PhaseId` = 1835 WHERE `id` IN (56360, 55999, 56362, 56476);
UPDATE `creature` SET `PhaseId` = '1835' WHERE (`guid` = '451988');
UPDATE `creature` SET `PhaseId` = '0', `PhaseGroup` = '641' WHERE (`guid` = '451723');
UPDATE `creature` SET `PhaseId` = 544, `terrainSwapMap` = 975 WHERE id IN (60780, 60858, 57317, 56417, 56418, 56419, 60851, 60853, 60854, 60877, 60878, 60873, 60770, 60834, 60852, 60874);
UPDATE `creature` SET `PhaseId` = 545, `terrainSwapMap` = 976 WHERE id IN (60895, 60888, 60896, 60898, 60894, 60893, 60892, 60891, 60890, 60889, 60722, 57743, 57741, 57739);
UPDATE `creature` SET `PhaseId` = 545, `terrainSwapMap` = 976 WHERE guid IN (452340);
UPDATE `creature` SET `PhaseId` = 0, `PhaseGroup` = 641, `terrainSwapMap` = -1 WHERE `guid` IN (451725, 451731, 4001703, 451746);
UPDATE `creature` SET `PhaseId` = 544, `ScriptName` = 'npc_healers_active_bunny' WHERE (`guid` = '452176');
UPDATE `creature` SET `PhaseId` = 543, `terrainSwapMap` = -1 WHERE `guid` IN (452131, 452134, 452133, 452127,452130, 452136, 452135, 452129, 452124); -- Deepscale Tormentors
UPDATE `creature` SET `PhaseId` = 169, `terrainSwapMap` = -1 WHERE `guid` IN (451858, 451860, 451965, 451979, 451972,451978,451975,451969,451851,451856);
UPDATE `creature` SET `PhaseId` = 0, `PhaseGroup` = 680 WHERE `guid` IN (451733,451734,452119,452121,452123,452126);

DELETE FROM `creature` WHERE `map` = 860 AND `guid` IN (452061, 452110, 452062, 452099, 452114, 452105, 452117, 452351, 452355, 452101, 452111, 452112, 452248, 452229, 452265, 452250,
452252,452254, 452196, 452210, 452268, 452283, 452273, 452253, 452224, 452195, 452266, 452267, 452219, 452237, 452263, 452277, 452211, 452225, 452244, 452262, 452279, 452264); -- duplicate spawns
DELETE FROM `creature` WHERE `map` = 860 AND `id` IN (56236); -- wrong spawns as the sailors are spawned from quest
DELETE FROM `creature` WHERE `map` = 860 AND `guid` IN (452202, 452245, 452189, 452201, 452149, 452143, 452151, 452141, 452158, 452235, 452208); -- event adds are now spawned from script

-- Duplicate Tormentor Spawns
DELETE FROM `creature` WHERE `map` = 860 AND `guid` IN (452059,452115,451758,452011);
-- Duplicate Darkened Horror/Terror
DELETE FROM `creature` WHERE `map` = 860 AND `guid` IN (451966,451967,451959,451952);

UPDATE `gameobject` SET `PhaseId` = 1835 WHERE `id` IN (209793);
UPDATE `gameobject` SET `PhaseId` = 993 WHERE `guid` IN (300834);

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

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (55999, 60770, 60877, 60878, 60834);
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES
(55999, 129340, 1, 0),
(55999, 56685, 1, 0),
(60770, 56685, 1, 0),
(60877, 56685, 1, 0),
(60834, 56685, 1, 0),
(60878, 56685, 1, 0);


-- Scripts
DELETE FROM `areatrigger_scripts` WHERE `entry` IN (7714, 7087);
INSERT INTO `areatrigger_scripts` VALUES
(7714, 'at_forlorn_hut_7714'),
(7087, 'at_wreck_of_the_skyseeker_injured_sailor');

DELETE FROM `spell_script_names` WHERE `spell_id` IN (117973, 108806, 129341, 117597, 118233, 117783, 117400);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES 
(117400, 'spell_summon_deep_sea_aggressor'),
(117973, 'spell_summon_ji_forlorn_hut'),
(129341, 'spell_rescue_injured_sailor'),
(108806, 'aura_injured_sailor_feign_death'),
(117597, 'spell_summon_ji_wreck_explosion'),
(118233, 'spell_turtle_healed_phase_timer'),
(117783, 'spell_healing_shenzin_su');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=13 AND `SourceEntry` IN (108932,108933);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(13, 1, 108933, 0, 0, 51, 0, 5, 57741, 0, 0, 0, 0, '', 'Summon Ox Cart, Skyfire Crash > Temple target Delivery Cart'),
(13, 1, 108932, 0, 0, 51, 0, 5, 57743, 0, 0, 0, 0, '', 'Summon Ox, Skyfire Crash > Temple target Nourished Yak');


UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (60900, 60851, 57317, 60852, 60854, 60858, 56417, 55940, 60853, 60873, 56419, 60780, 56007, 56008,
56174, 55946, 53705, 56172, 60685);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (60900, 6090000, 60851, 57317, 60852, 60854, 60858, 56417, 55940, 60853, 60873, 56419, 60780, 57739,
56007, 56008, 56174, 55946, 53705, 56172, 60685) AND `source_type` IN (0, 9);
INSERT INTO `smart_scripts` (entryorguid, source_type, id, link, event_type, event_phase_mask, event_chance, event_flags,  event_param1, event_param2, 
event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, 
action_param6, target_type, target_param1, target_param2, target_param3, target_x, target_y, target_z, target_o, comment) VALUES
(60685, 0, 0, 1, 54, 0, 100, 0, 0, 0, 0, 0, 0, '', 117, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Deepscale Agressor - Just Spawned - Disable Evade'),
(60685, 0, 1, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 117407, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Deepscale Agressor - Just Spawned - Cast Aggresive Leap'),
(60685, 0, 2, 0, 31, 0, 100, 0, 117407, 0, 0, 0, 0, '', 49, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Deepscale Agressor - On Spellhit Target - Start Attack'),
(56172, 0, 0, 0, 0, 0, 100, 0, 5000, 8000, 8000, 12000, 0, '', 11, 128429, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Ash Moth - Update IC - Cast Horn'),
(53705, 0, 0, 0, 0, 0, 100, 0, 5000, 8000, 8000, 12000, 0, '', 11, 128406, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Pangolin - Update IC - Cast Hardened Shell'),
(55946, 0, 0, 0, 0, 0, 100, 0, 5000, 8000, 8000, 12000, 0, '', 11, 128272, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Pei-Wu Tiger - Update IC - Cast Tiger Pounce'),
(56174, 0, 0, 0, 0, 0, 100, 0, 5000, 8000, 8000, 12000, 0, '', 11, 128277, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Pei-Wu Tiger Cub - Update IC - Cast Cub Pounce'),
(56007, 0, 0, 0, 0, 0, 100, 0, 3000, 5000, 8000, 12000, 0, '', 11, 128417, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Darkened Horror - Update IC - Cast Shadow Geyser'),
(56008, 0, 0, 0, 0, 0, 100, 0, 3000, 5000, 8000, 12000, 0, '', 11, 128424, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Darkened Terror - Update IC - Cast Backhanded Swipes'),
('57739', '0', '0', '1', '20', '0', '100', '0', '29799', '0', '0', '0', '0', '', '44', '545', '1', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - On Quest Complete - Add Phase'),
('57739', '0', '1', '2', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '44', '544', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - On Quest Complete - Remove Phase'),
('55940', '0', '0', '1', '19', '0', '100', '0', '29798', '0', '0', '0', '0', '', '44', '543', '1', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - On Quest Accepted - Add Phase'),
('55940', '0', '1', '2', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '44', '993', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - On Quest Accepted - Remove Phase'),
('55940', '0', '2', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '44', '1835', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - On Quest Accepted - Remove Phase'),
('60900', '0', '0', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '6090000', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Just Spawned - Run Script'),
('60900', '0', '1', '2', '40', '0', '100', '0', '2', '6090000', '0', '0', '0', '', '54', '3000', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - On WP Reached - Pause WP'),
('60900', '0', '2', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '97', '15', '15', '0', '0', '0', '0', '8', '0', '0', '0', '424.019', '3676.25', '78.6968', '0', 'Ji Firepaw - On WP Reached - Jump to Pos'),
('60900', '0', '3', '4', '58', '0', '100', '0', '22', '6090000', '0', '0', '0', '', '66', '0', '0', '0', '0', '0', '0', '8', '0', '0', '0', '0', '0', '0', '0.401426', 'Ji Firepaw - On WP Ended - Set Orientation'),
('60900', '0', '4', '5', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '28', '117973', '0', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - On WP Ended - Remove Aura'),
('60900', '0', '5', '6', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '44', '1835', '1', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - On WP Ended - Add Phase'),
('60900', '0', '6', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - On WP Ended - Despawn'),
('6090000', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('6090000', '9', '1', '0', '0', '0', '100', '0', '5000', '5000', '1000', '1000', '0', '', '53', '1', '6090000', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Start WP'),
('60851', '0', '0', '0', '10', '0', '100', '0', '1', '10', '240000', '300000', '0', '', '84', '0', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Delora Lionheart - OOC LOS - Talk'),
('60851', '0', '1', '0', '0', '0', '100', '0', '5000', '8000', '5000', '8000', '0', '', '11', '128440', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Delora Lionheart - Update IC - Cast Piercing Strikes'),
('60851', '0', '2', '0', '0', '0', '100', '0', '2000', '5000', '2000', '5000', '0', '', '11', '128513', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Delora Lionheart - Update IC - Cast Strike'),
('57317', '0', '0', '0', '0', '0', '100', '0', '3000', '5000', '3000', '5000', '0', '', '11', '128440', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Skyseeker Sailor - Update IC - Cast Piercing Strikes'),
('60852', '0', '0', '0', '10', '0', '100', '0', '1', '10', '240000', '300000', '0', '', '84', '0', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Korga Strongmane - OOC LOS - Talk'),
('60852', '0', '1', '0', '0', '0', '100', '0', '5000', '8000', '5000', '8000', '0', '', '11', '128515', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Korga Strongmane - Update IC - Cast Lightning Bolt'),
('60854', '0', '0', '0', '0', '0', '100', '0', '5000', '8000', '5000', '8000', '0', '', '11', '128510', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Escaped Horde Crewman - Update IC - Cast Double Thrust'),
('60854', '0', '1', '0', '1', '0', '100', '0', '2000', '2000', '2000', '2000', '0', '', '49', '0', '0', '0', '0', '0', '0', '19', '0', '10', '0', '0', '0', '0', '0', 'Escaped Horde Crewman - Update OOC - Start Attack'),
('60858', '0', '0', '0', '0', '0', '100', '0', '5000', '8000', '5000', '8000', '0', '', '11', '128533', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Deepscale Fleshripper - Update IC - Cast Rip Flesh'),
('60858', '0', '1', '0', '1', '0', '100', '0', '2000', '2000', '2000', '2000', '0', '', '49', '0', '0', '0', '0', '0', '0', '19', '0', '10', '0', '0', '0', '0', '0', 'Deepscale Fleshripper - Update OOC - Start Attack'),
('56417', '0', '0', '0', '11', '0', '100', '0', '0', '0', '0', '0', '0', '', '8', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Aysa Cloudsinger - On Respawn - Set React State'),
('56417', '0', '1', '0', '1', '0', '100', '0', '2000', '2000', '2000', '2000', '0', '', '49', '0', '0', '0', '0', '0', '0', '19', '0', '5', '0', '0', '0', '0', '0', 'Aysa Cloudsinger - Update OOC - Start Attack'),
('56417', '0', '2', '0', '0', '0', '100', '0', '2000', '4000', '2000', '4000', '0', '', '11', '117275', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Aysa Cloudsinger - Update IC - Cast Tempered Fury'),
('56417', '0', '3', '0', '0', '0', '100', '0', '5000', '8000', '10000', '15000', '0', '', '11', '117401', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Aysa Cloudsinger - Update IC - Cast Spinning Crane Kick'),
('60853', '0', '0', '0', '11', '0', '100', '0', '0', '0', '0', '0', '0', '', '21', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Makael Bay - On Respawn - Disable Combat Movement'),
('60853', '0', '1', '0', '0', '0', '100', '0', '1000', '1200', '1000', '1200', '0', '', '11', '107115', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Makael Bay - Update IC - Cast Throw Dynamite'),
('60853', '0', '2', '0', '9', '0', '100', '0', '0', '5', '8000', '12000', '0', '', '11', '128526', '1', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Makael Bay - On Target In Range - Cast Saw Blade'),
('60873', '0', '0', '0', '1', '0', '100', '0', '2000', '2000', '2000', '2000', '0', '', '49', '0', '0', '0', '0', '0', '0', '19', '0', '10', '0', '0', '0', '0', '0', 'Skyseeker Sailor - Update OOC - Start Attack'),
('60780', '0', '0', '0', '1', '0', '100', '0', '2000', '2000', '2000', '2000', '0', '', '49', '0', '0', '0', '0', '0', '0', '19', '0', '10', '0', '0', '0', '0', '0', 'Deepscale Ravager - Update OOC - Start Attack'),
('56419', '0', '0', '0', '11', '0', '100', '0', '0', '0', '0', '0', '0', '', '8', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - On Respawn - Set React State'),
('56419', '0', '1', '0', '9', '0', '50', '0', '5', '30', '5000', '10000', '0', '', '11', '81574', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - On Target In Range - Cast Charge'),
('56419', '0', '2', '0', '0', '0', '100', '0', '2000', '2000', '2000', '2000', '0', '', '11', '120383', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Update IC - Cast Pillar Strike'),
('56419', '0', '3', '0', '0', '0', '100', '0', '5000', '8000', '8000', '12000', '0', '', '11', '120372', '1', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Update IC - Cast Pillar Sweep');

UPDATE `creature_template` SET `ScriptName` = '', `AIName` = 'SmartAI' WHERE `Entry` IN (56007,56008,56360);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (56007,56008,56360,-452012,-452006,-452103,-452104,-452054) AND `source_type` IN (0,9);
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
-- Darkened Horror
(56007, 0, 0, 0, 0, 0, 100, 0, 3000, 5000, 8000, 12000, 0, 11, 128417, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Darkened Horror - Update IC - Cast Shadow Geyser'),
-- Darkened Horror
(-452012, 0, 0, 0, 11, 0, 100, 0, 0, 0, 0, 0, 0, 53, 1, 5600700, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Darkened Horror - On Respawn - Start WP'),
(-452012, 0, 1, 2, 58, 0, 100, 0, 9, 5600700, 0, 0, 0, 17, 27, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Darkened Horror - On WP Ended - Set Emote State'),
(-452012, 0, 2, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 49, 1, 0, 0, 0, 0, 0, 11, 60554, 5, 0, 0, 0, 0, 0, 'Darkened Horror - On WP Ended - Start Attack'),
(-452012, 0, 3, 0, 0, 0, 100, 0, 3000, 5000, 8000, 12000, 0, 11, 128417, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Darkened Horror - Update IC - Cast Shadow Geyser'),
-- Darkened Horror
(-452006, 0, 0, 0, 11, 0, 100, 0, 0, 0, 0, 0, 0, 53, 1, 5600701, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Darkened Horror - On Respawn - Start WP'),
(-452006, 0, 1, 2, 58, 0, 100, 0, 8, 5600701, 0, 0, 0, 17, 27, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Darkened Horror - On WP Ended - Set Emote State'),
(-452006, 0, 2, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 49, 1, 0, 0, 0, 0, 0, 11, 60554, 5, 0, 0, 0, 0, 0, 'Darkened Horror - On WP Ended - Start Attack'),
(-452006, 0, 3, 0, 0, 0, 100, 0, 3000, 5000, 8000, 12000, 0, 11, 128417, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Darkened Horror - Update IC - Cast Shadow Geyser'),
-- Darkened Horror
(-452103, 0, 0, 0, 11, 0, 100, 0, 0, 0, 0, 0, 0, 53, 1, 5600702, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Darkened Horror - On Respawn - Start WP'),
(-452103, 0, 1, 2, 58, 0, 100, 0, 12, 5600702, 0, 0, 0, 17, 27, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Darkened Horror - On WP Ended - Set Emote State'),
(-452103, 0, 2, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 49, 1, 0, 0, 0, 0, 0, 11, 60554, 5, 0, 0, 0, 0, 0, 'Darkened Horror - On WP Ended - Start Attack'),
(-452103, 0, 3, 0, 0, 0, 100, 0, 3000, 5000, 8000, 12000, 0, 11, 128417, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Darkened Horror - Update IC - Cast Shadow Geyser'),
-- Darkened Terror
(56008, 0, 0, 0, 0, 0, 100, 0, 3000, 5000, 8000, 12000, 0, 11, 128424, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Darkened Terror - Update IC - Cast Backhanded Swipes'),
-- Darkened Terror
(-452104, 0, 0, 0, 11, 0, 100, 0, 0, 0, 0, 0, 0, 53, 1, 5600800, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Darkened Terror - On Respawn - Start WP'),
(-452104, 0, 1, 2, 58, 0, 100, 0, 12, 5600800, 0, 0, 0, 17, 27, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Darkened Terror - On WP Ended - Set Emote State'),
(-452104, 0, 2, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 49, 1, 0, 0, 0, 0, 0, 11, 60554, 5, 0, 0, 0, 0, 0, 'Darkened Terror - On WP Ended - Start Attack'),
(-452104, 0, 3, 0, 0, 0, 100, 0, 3000, 5000, 8000, 12000, 0, 11, 128424, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Darkened Terror - Update IC - Cast Backhanded Swipes'),
-- Darkened Terror
(-452054, 0, 0, 0, 11, 0, 100, 0, 0, 0, 0, 0, 0, 53, 1, 5600801, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Darkened Terror - On Respawn - Start WP'),
(-452054, 0, 1, 2, 58, 0, 100, 0, 9, 5600801, 0, 0, 0, 17, 27, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Darkened Terror - On WP Ended - Set Emote State'),
(-452054, 0, 2, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 49, 1, 0, 0, 0, 0, 0, 11, 60554, 5, 0, 0, 0, 0, 0, 'Darkened Terror - On WP Ended - Start Attack'),
(-452054, 0, 3, 0, 0, 0, 100, 0, 3000, 5000, 8000, 12000, 0, 11, 128424, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Darkened Terror - Update IC - Cast Backhanded Swipes'),
-- Deepscale Tormentor
(56360, 0, 0, 0, 11, 0, 100, 0, 0, 0, 0, 0, 0, 49, 0, 0, 0, 0, 0, 0, 9, 56362, 0, 5, 0, 0, 0, 0, 'Deepscale Tormentor - Update OOC - Start Attack'),
(56360, 0, 1, 0, 0, 0, 100, 0, 3000, 5000, 8000, 12000, 0, 11, 128270, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Deepscale Tormentor - Update IC - Cast Smash');


DELETE FROM `creature_template_difficulty` WHERE `entry` IN (60888, 60889, 60685, 60554, 65742, 55946, 56195, 56174);
INSERT INTO `creature_template_difficulty` VALUES
(65742, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 59502, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60554, 0, 0, 0, 80, 4, 1, 1, 1, 1, 21169, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60685, 0, 0, 0, 80, 4, 0.5, 1, 1, 1, 20951, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 256, 0, 0, 0, 0, 0, 56647),
(60888, 0, 0, 0, 80, 4, 5, 1, 1, 0.01, 20592, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60889, 0, 0, 0, 80, 4, 5, 1, 1, 0.2, 20590, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(56174, 0, 0, 0, 80, 4, 0.5, 1, 1, 0.2, 28481, 0, 0, 0, 56174, 0, 1, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55946, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28860, 1, 0, 0, 55946, 0, 1, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(56195, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28424, 0, 0, 0, 0, 0, 0, 0, 0, 268435712, 0, 0, 0, 0, 0, 0, 0, 56647);

DELETE FROM `creature_template_addon` WHERE `entry` IN (60888, 60889, 60685, 60554, 65742, 55946, 56195, 56174);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureId`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
(60554, 0, 0, 0, 0, 0, 0, 1, 0, 425, 0, 0, 0, 0, ''),
(65742, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(60685, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(60888, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(60889, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(56174, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- tiger cub
('55946', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''), -- tiger
(56195, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '96733'); -- Escaped horde engineer


DELETE FROM `creature_addon` WHERE `guid` IN (451865, 451813, 451815, 451817, 451821, 451836, 451844, 451847, 451900, 451905, 451916, 451923, 451929, 451932, 451339, 451352, 452010, 452034, 452038, 452041, 
451350, 451840, 451867, 451901, 452033, 452040, 452042,451944,452052,452098,451984,451763,451751,451747,451953,452005,452051,452009,451965,451979,451858);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
-- tiger cub spawns with zzz aura
(451350, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451840, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451867, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451901, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(452033, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(452040, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(452042, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
-- tiger spawns with zzz aura
(451339, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451352, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451813, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451815, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451817, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451821, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451836, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451844, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451847, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451865, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451900, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451905, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451916, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451923, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451929, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(451932, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(452010, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(452034, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(452038, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
(452041, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '120703'),
-- Horde Druid
(451965, 6089600, 0, 0, 8, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451979, 0, 0, 0, 8, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451858, 6089601, 0, 0, 8, 0, 1, 0, 0, 0, 0, 0, 0, ''),
-- Terror/Horror with path
(451944, 5600802, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(452052, 5600803, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(452098, 5600703, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451984, 5600704, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451763, 5600705, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451751, 5600706, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451747, 5600707, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451953, 5600708, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(452005, 5600709, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(452051, 5600710, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(452009, 5600711, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '');



DELETE FROM `creature_addon` WHERE `guid` IN (451348, 451352, 451756, 451788, 451835, 451839, 451845, 451849, 451878, 451880, 451883, 451886, 
451888, 451893, 451912, 451918, 451926, 451930, 451933, 451937, 451941, 451942, 451983, 452015, 452019, 452025, 451928, 451810, 451875, 451857);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, 
`aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
-- Escaped Horde Crewman with path
(451928, 6055400, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451810, 6055401, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451875, 6574200, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '129315'),
(451857, 6574201, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '129315'),


-- tiger spawns with path
(451348, 5594600, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451352, 5594601, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451756, 5594602, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451788, 5594603, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451835, 5594604, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451839, 5594605, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451845, 5594606, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451849, 5594607, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451878, 5594608, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451880, 5594609, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451883, 5594610, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451886, 5594611, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451888, 5594612, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451893, 5594613, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451912, 5594614, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451918, 5594615, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451926, 5594616, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451930, 5594617, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451933, 5594618, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451937, 5594619, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451941, 5594620, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451942, 5594621, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451983, 5594622, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(452015, 5594623, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(452019, 5594624, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(452025, 5594625, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '');

UPDATE `creature` SET `MovementType` = 2 WHERE `guid` IN (451857, 451875, 451810, 451928, 451348, 451352, 451756, 451788, 451835, 451839, 451845, 451849, 451878, 
451880, 451883, 451886, 451888, 451893, 451912, 451918, 451926, 451930, 451933, 451937, 451941, 451942, 451983, 452015, 452019, 452025,451944,452052,452098,451984,
451763,451751,451747,451953,452005,452051,452009,451965,451858);
UPDATE `creature` SET `MovementType` = 1, `wander_distance` = 5 WHERE `id` IN (55946) and `guid` not in 
(451348, 451352, 451756, 451788, 451835, 451839, 451845, 451849, 451878, 451880, 451883, 451886, 451888, 451893, 451912, 451918, 451926, 451930, 451933, 451937, 451941, 451942, 451983, 452015, 452019, 452025, 451865, 451813, 451815, 451817, 451821, 451836, 451844, 451847, 451900, 451905, 451916, 451923, 451929, 451932, 451339, 451352, 452010, 452034, 452038, 452041);

UPDATE `creature` SET `position_x` = '548.625', `position_y` = '3492.5', `position_z` = '95.0985', `orientation` = '1.64148' WHERE `guid` = 451840; -- tiger cub pos fix

-- Loot tables

-- Delete only reference loot items (ItemType = 1) for specific NPCs
DELETE FROM creature_loot_template 
WHERE Entry IN (53705, 55946, 56172, 56174) 
AND ItemType = 1;

-- NPC: 53705 Pangolin (Beast)
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, COMMENT) VALUES
(53705, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(53705, 1, 2, 100, 0, 1, 1, 1, 'Beasts');

-- NPC: 55946 Pei-wu tiger (Beast) 
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, COMMENT) VALUES
(55946, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(55946, 1, 2, 100, 0, 1, 1, 1, 'Beasts');

-- NPC: 56172 Ash moth (Beast)
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, COMMENT) VALUES
(56172, 1, 1, 100, 0, 1, 1, 1, 'General loot');

-- NPC: 56174 Tiger cub (Beast)
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, COMMENT) VALUES
(56174, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(56174, 1, 2, 100, 0, 1, 1, 1, 'Beasts');

-- Delete existing reference loot for humanoid NPCs with scales
DELETE FROM creature_loot_template 
WHERE Entry IN (60858, 60780, 56360, 60722, 56007, 56008, 60685) 
AND ItemType = 1;

-- NPC: 60858 Deepscale Fleshripper (Humanoid with scales)
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(60858, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(60858, 1, 3, 100, 0, 1, 1, 1, 'Water related beasts'),
(60858, 1, 4, 100, 0, 1, 1, 1, 'Humanoids');

-- NPC: 60780 Deepscale Ravager (Humanoid with scales)
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(60780, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(60780, 1, 3, 100, 0, 1, 1, 1, 'Water related beasts'),
(60780, 1, 4, 100, 0, 1, 1, 1, 'Humanoids');

-- NPC: 56360 Deepscale Tormentor (Humanoid with scales)
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(56360, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(56360, 1, 3, 100, 0, 1, 1, 1, 'Water related beasts'),
(56360, 1, 4, 100, 0, 1, 1, 1, 'Humanoids');

-- NPC: 60722 Deepscale Tormentor (Humanoid with scales)
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(60722, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(60722, 1, 3, 100, 0, 1, 1, 1, 'Water related beasts'),
(60722, 1, 4, 100, 0, 1, 1, 1, 'Humanoids');

-- NPC: 56007 Darkened Horror (Humanoid with scales)
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(56007, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(56007, 1, 3, 100, 0, 1, 1, 1, 'Water related beasts'),
(56007, 1, 4, 100, 0, 1, 1, 1, 'Humanoids');

-- NPC: 56008 Darkened Terror (Humanoid with scales)
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(56008, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(56008, 1, 3, 100, 0, 1, 1, 1, 'Water related beasts'),
(56008, 1, 4, 100, 0, 1, 1, 1, 'Humanoids');

-- NPC: 60685 Deepscale Aggressor (Humanoid with scales)
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(60685, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(60685, 1, 3, 100, 0, 1, 1, 1, 'Water related beasts'),
(60685, 1, 4, 100, 0, 1, 1, 1, 'Humanoids');