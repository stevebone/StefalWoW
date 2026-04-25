-- Quest Templates
DELETE FROM `quest_template_addon` WHERE `ID` IN (29423);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
(29423, 0, 0, 0, 0, 29521, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'quest_29423_the_passion_of_shen_zin_su'); 	-- Quest: 29423 The Passion of Shen-zin Su

UPDATE `quest_template_addon` SET `ScriptName`= 'quest_29422_huo_the_spirit_of_fire', `NextQuestID`=29423 WHERE `ID` IN (29422);

-- Misc mobs fixes

-- Added smart scripts missing for 54131
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (54131) AND `source_type` = 0;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('54131', '0', '0', '0', '11', '0', '100', '0', '0', '0', '0', '0', '0', '', '8', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Hozen - On Respawn - Set React State'),
('54131', '0', '1', '0', '4', '0', '100', '0', '0', '0', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '11', '56739', '1', '0', '0', '0', '0', '0', 'Fe-Feng Hozen - On Aggro - Despawn Vehicle'),
('54131', '0', '2', '0', '7', '0', '100', '0', '0', '0', '0', '0', '0', '', '41', '2000', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Hozen - On Evade - Despawn'),
('54131', '0', '3', '0', '0', '0', '100', '0', '2000', '8000', '4000', '8000', '0', '', '11', '121484', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Hozen - IC - Cast Ook\'em'),
('54131', '0', '4', '0', '0', '0', '50', '0', '5000', '10000', '5000', '10000', '0', '', '11', '115006', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Hozen - IC - Cast Hozen Rage');

UPDATE `creature_template` SET `AIName` = '' WHERE `Entry` IN (53714); -- Training target no longer having scripts
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (56730,53704,57205,57164,54130, 57797);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (56730,53704,57205,57164,54130, 57797) AND `source_type` = 0;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('56730', '0', '0', '1', '38', '0', '100', '0', '1', '1', '0', '0', '0', '', '97', '15', '15', '0', '0', '0', '0', '1', '0', '0', '0', '1390.44', '3846', '91.4894', '0', 'Fe-Feng Brewthief - On Data Set - Jump to Pos'),
('56730', '0', '1', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5673000', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Brewthief - On Data Set - Run Script'),
('56730', '0', '2', '3', '38', '0', '100', '0', '2', '2', '0', '0', '0', '', '97', '15', '15', '0', '0', '0', '0', '1', '0', '0', '0', '1380.98', '3846.78', '93.4798', '0', 'Fe-Feng Brewthief - Just Spawned - Jump to Pos'),
('56730', '0', '3', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5673001', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Brewthief - On Data Set - Run Script'),
('56730', '0', '4', '5', '58', '0', '100', '0', '5', '5673006', '0', '0', '0', '', '97', '15', '10', '0', '0', '0', '0', '1', '0', '0', '0', '1402.4', '3737.49', '87.1755', '0', 'Fe-Feng Brewthief - On WP Ended - Jump to Pos'),
('56730', '0', '5', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5673002', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Brewthief - On WP Ended - Run Script'),
('56730', '0', '6', '7', '58', '0', '100', '0', '4', '5673004', '0', '0', '0', '', '97', '15', '10', '0', '0', '0', '0', '1', '0', '0', '0', '1401.19', '3736.55', '87.4988', '0', 'Fe-Feng Brewthief - On WP Ended - Jump to Pos'),
('56730', '0', '7', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5673003', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Brewthief - On WP Ended - Run Script'),
('56730', '0', '8', '9', '58', '0', '100', '0', '5', '5673007', '0', '0', '0', '', '97', '15', '15', '0', '0', '0', '0', '1', '0', '0', '0', '1399.77', '3859.49', '100.569', '0', 'Fe-Feng Brewthief - On WP Ended - Jump to Pos'),
('56730', '0', '9', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '41', '2000', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Brewthief - On WP Ended - Despawn'),
('56730', '0', '10', '11', '58', '0', '100', '0', '4', '5673005', '0', '0', '0', '', '97', '15', '15', '0', '0', '0', '0', '1', '0', '0', '0', '1380.9', '3863.48', '100.405', '0', 'Fe-Feng Brewthief - On WP Ended - Jump to Pos'),
('56730', '0', '11', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '41', '2000', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Brewthief - On WP Ended - Despawn'),
('53704', '0', '0', '0', '0', '0', '100', '0', '3000', '8000', '15000', '20000', '0', '', '11', '75533', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Corsac Fox - Update IC - Cast Wily Wits'),
('57797', '0', '0', '0', '0', '0', '100', '0', '3000', '8000', '5000', '8000', '0', '', '11', '75529', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Corsac Fox - Update IC - Cast Agile Focus'),
('57205', '0', '0', '0', '0', '0', '100', '0', '3000', '8000', '15000', '20000', '0', '', '11', '114974', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Hozen - Update IC - Impale'),
('54130', '0', '0', '0', '0', '0', '100', '0', '3000', '8000', '15000', '20000', '0', '', '11', '109081', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Amberleaf Scamp - Update IC - Take This'),
('57164', '0', '0', '0', '4', '0', '100', '0', '0', '0', '0', '0', '0', '', '11', '107526', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Leaper - On Aggro - Cast Reckless Leap');

DELETE FROM `creature_template_addon` WHERE `entry` IN (54130, 54131, 57164, 56730, 53704, 57205, 57797, 54787, 54958, 57779);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureId`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, 
`movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(54787, 0, 0, 0, 0, 3, 1, 1, 0, 0, 0, 0, 0, 0, '83305'),
(54958, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0, 3300, 0, 0, '102630'),
(57779, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(53704, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Corsac Fox
(57797, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '106562'), -- Corsac Fox
(54130, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Amberleaf Scamp
(54131, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Fe-Feng Hozen
(56730, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '109084'), -- Fe-Feng Brewthief
(57164, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Fe-Feng Leaper
(57205, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''); -- Fe-Feng Hozen

DELETE FROM `creature_template_difficulty` WHERE `entry` IN (54734,54130, 54131, 57164, 56730, 53704, 57205, 57797, 54787,57779, 54958, 65472,57132);
INSERT INTO `creature_template_difficulty` VALUES
(54734, 0, 0, 0, 80, 4, 60, 1, 1, 0.2, 30831, 4, 0, 0, 0, 0, 0, 0, 0, 524296, 0, 0, 0, 0, 0, 0, 0, 56647),
(57132, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 26851, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647), -- Wu-song Villager
(65472, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 59132, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647), -- Wu-song Villager
(54958, 0, 0, 0, 80, 4, 1, 1, 1, 1, 30560, 0, 0, 0, 0, 0, 0, 0, 0, 805306368, 0, 0, 0, 0, 0, 0, 0, 64978), -- Huo
(54787, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 30792, 0, 0, 0, 0, 0, 0, 0, 0, 805306624, 0, 0, 0, 0, 0, 0, 0, 56647), -- Huo
(57779, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 26153, 0, 0, 0, 0, 0, 0, 0, 0, 805306624, 0, 0, 0, 0, 0, 0, 0, 56647), -- Huo
(57797, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 26137, 1, 0, 0, 57797, 0, 1, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(53704, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 32105, 1, 0, 0, 53704, 0, 1, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(54130, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 31575, 0, 0, 0, 54130, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(54131, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 31574, 0, 0, 0, 54131, 0, 0, 3, 3, 268435712, 0, 0, 0, 0, 0, 0, 0, 56647),
(56730, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 27399, 0, 0, 0, 56730, 0, 0, 3, 3, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(57164, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 26816, 0, 0, 0, 57164, 0, 0, 3, 3, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(57205, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 26781, 0, 0, 0, 57205, 0, 0, 3, 3, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647);

-- Fixes for Huo, the spirit of fire
UPDATE `creature` SET `PhaseId`= 632 WHERE `guid` IN (450608); 
UPDATE `creature` SET `PhaseId`= 631, `StringId` = 'Huo_Pre_Ignition' WHERE `guid` IN (450607); 


-- 29423 The Passion of Shen-zin Su
UPDATE `creature_template` SET `speed_run`=1.714285731315612792, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800, `ScriptName`= 'npc_huo_follower' WHERE `entry`=54958; -- Huo
UPDATE `creature` SET `ScriptName`= 'npc_chia_hui_autumnleaf', `StringId`= 'Chia_Hui_Talk_Event_Starter' WHERE `guid`=450386; 
UPDATE `creature` SET `StringId`= 'Brewer_Lin_Talk_Event' WHERE `guid`=450387; 
UPDATE `creature` SET `StringId`= 'Brewer_Zhen_Talk_Event' WHERE `guid`=450385; 
UPDATE `creature` SET `ScriptName`= 'npc_shanxi_quest', `StringId`= 'ShanXi_Talk' WHERE `guid`=450020; 
UPDATE `creature` SET `StringId`= 'Huo_Pre_Ignition' WHERE `guid`=450608;

-- Loot tables
-- Delete only reference loot items (ItemType = 1) for specific NPCs
DELETE FROM creature_loot_template 
WHERE Entry IN (54130, 54131, 57164, 56730, 53704, 57205, 57797) 
AND ItemType = 1;

-- NPC: 54130 Amberleaf Scamp
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(54130, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(54130, 1, 4, 100, 0, 1, 1, 1, 'Humanoids'),

-- NPC: 53704 Corsac Fox
(53704, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(53704, 1, 2, 100, 0, 1, 1, 1, 'Beasts'),
-- NPC: 57797 Corsac Fox
(57797, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(57797, 1, 2, 100, 0, 1, 1, 1, 'Beasts'),

-- Hozen Monkeys
(54131, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(54131, 1, 4, 100, 0, 1, 1, 1, 'Humanoids'),
(56730, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(56730, 1, 4, 100, 0, 1, 1, 1, 'Humanoids'),
(57164, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(57164, 1, 4, 100, 0, 1, 1, 1, 'Humanoids'),
(57205, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(57205, 1, 4, 100, 0, 1, 1, 1, 'Humanoids');