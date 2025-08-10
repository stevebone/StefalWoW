-- Quest: 29777 Tools of the enemy
-- Quest: 29778 Rewritten wisdoms
-- Quest: 29783 Stronger than stone
-- NPC: 55588 Elder Shaopai at Morning Breeze
-- NPC: 55583 Ji Firepaw at Morning Breeze
-- NPC: 55585 Jojo at Morning Breeze
-- NPC: 57670 Jojo spawn at Morning Breeze
-- NPC: 55601 Fe-feng Wiseman

DELETE FROM `spell_script_names` WHERE `spell_id` = 104126;
INSERT INTO `spell_script_names` VALUES (104126, 'spell_monkey_wisdom_text');

DELETE FROM `creature_queststarter` WHERE `quest` IN (29777,29778,29783);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55588', '29777', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55583', '29778', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55585', '29783', '0');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29777,29778,29783);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29777', '0', '0', '0', '29776', '0', '-29778', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29778', '0', '0', '0', '29776', '0', '-29778', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29783', '0', '0', '0', '29776', '0', '-29778', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `creature_text` WHERE `creatureID` IN (57670);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('57670', '0', '0', 'These are the densest stones across all of Shen-zin Su, yet they are as nothing before my mighty head.', '12', '0', '100', '1', '0', '0', '56355', '0', 'Jojo Ironbrow to Player'),
('57670', '1', '0', 'Any who yet doubt my strength are simply foolish.', '12', '0', '100', '2', '0', '0', '56356', '0', 'Jojo Ironbrow to Player');


UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55601,57670,65467,56394,56393);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55601,57670,5767000,65467,56394,56393,6546700,5639400,5639300);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('55601', '0', '0', '0', '0', '0', '100', '0', '3000', '6000', '18000', '22000', '0', '', '11', '128751', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
('55601', '0', '1', '0', '0', '0', '100', '0', '2000', '4000', '8000', '12000', '0', '', '11', '128413', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
('57670', '0', '0', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5767000', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Just Spawned - Run Script'),
('5767000', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Talk'),
('5767000', '9', '1', '0', '0', '0', '100', '0', '2500', '2500', '1000', '1000', '0', '', '128', '1', '1', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Set AI AnimKit'),
('5767000', '9', '2', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '69', '1', '0', '0', '0', '0', '0', '8', '0', '0', '0', '1077.31', '4179.94', '205.774', '0', 'Jojo Ironbrow - Move to Pos'),
('5767000', '9', '3', '0', '0', '0', '100', '0', '3000', '3000', '1000', '1000', '0', '', '11', '129294', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Cast Jojo Headbash, Blocks Cast'),
('5767000', '9', '4', '0', '0', '0', '100', '0', '2500', '2500', '1000', '1000', '0', '', '1', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Talk'),
('5767000', '9', '5', '0', '0', '0', '100', '0', '100', '100', '0', '0', '0', '', '28', '108846', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Remove Auras'),
('5767000', '9', '6', '0', '0', '0', '100', '0', '100', '100', '0', '0', '0', '', '28', '108846', '0', '0', '0', '0', '0', '18', '50', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Remove Player Auras'),
('5767000', '9', '7', '0', '0', '0', '100', '0', '100', '100', '0', '0', '0', '', '28', '108846', '0', '0', '0', '0', '0', '11', '0', '10', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Remove Creatures Auras'),
('5767000', '9', '8', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '45', '1', '1', '0', '0', '0', '0', '11', '0', '10', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Set Data'),
('65467', '0', '0', '0', '38', '0', '100', '0', '1', '1', '0', '0', '0', '', '80', '6546700', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Mesmerized Onlooker - On Data Set - Run Script'),
('56394', '0', '0', '0', '38', '0', '100', '0', '1', '1', '0', '0', '0', '', '80', '5639400', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Mesmerized Child - On Data Set - Run Script'),
('56393', '0', '0', '0', '38', '0', '100', '0', '1', '1', '0', '0', '0', '', '80', '5639300', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Mesmerized Onlooker - On Data Set - Run Script'),
('5639300', '9', '0', '0', '0', '0', '100', '0', '2000', '2000', '1000', '1000', '0', '', '5', '4', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Excited Onlooker - Play Emote'),
('5639300', '9', '1', '0', '0', '0', '100', '0', '3500', '3500', '1000', '1000', '0', '', '5', '21', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Excited Onlooker - Play Emote'),
('5639400', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '5', '18', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Mesmerized Child - Play Emote'),
('6546700', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '5', '4', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Excited Onlooker - Play Emote'),
('6546700', '9', '1', '0', '0', '0', '100', '0', '4000', '4000', '1000', '1000', '0', '', '5', '21', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Excited Onlooker - Play Emote'),
('6546700', '9', '2', '0', '0', '0', '100', '0', '5000', '5000', '1000', '1000', '0', '', '5', '4', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Excited Onlooker - Play Emote');



UPDATE `gameobject_template` SET `Data6` = '120000' WHERE (`entry` = '209656');
UPDATE `gameobject_template` SET `Data6` = '120000' WHERE (`entry` = '209660');
UPDATE `gameobject_template` SET `Data6` = '120000' WHERE (`entry` = '209661');
UPDATE `gameobject_template` SET `Data6` = '120000' WHERE (`entry` = '209663');

UPDATE `gameobject_template_addon` SET `flags` = '0' WHERE (`entry` = '209660'); -- was 36
UPDATE `gameobject_template_addon` SET `flags` = '0' WHERE (`entry` = '209661'); -- was 36

DELETE FROM `gameobject` WHERE (`guid` = '300646');
DELETE FROM `gameobject` WHERE (`guid` = '300791');
DELETE FROM `gameobject` WHERE (`guid` = '300643');
DELETE FROM `gameobject` WHERE (`guid` = '300789');
DELETE FROM `gameobject` WHERE (`guid` = '300540');
DELETE FROM `gameobject` WHERE (`guid` = '300642');
DELETE FROM `gameobject` WHERE (`guid` = '300648');
DELETE FROM `gameobject` WHERE (`guid` = '300539');
DELETE FROM `gameobject` WHERE (`guid` = '300644');
DELETE FROM `gameobject` WHERE (`guid` = '300787');
DELETE FROM `gameobject` WHERE (`guid` = '300788');
DELETE FROM `gameobject` WHERE (`guid` = '300759');
DELETE FROM `gameobject` WHERE (`guid` = '300751');
DELETE FROM `gameobject` WHERE (`guid` = '300753');
DELETE FROM `gameobject` WHERE (`guid` = '300466');
DELETE FROM `gameobject` WHERE (`guid` = '300560');
DELETE FROM `gameobject` WHERE (`guid` = '300793');
DELETE FROM `gameobject` WHERE (`guid` = '300488');
DELETE FROM `gameobject` WHERE (`guid` = '300650');
DELETE FROM `gameobject` WHERE (`guid` = '300792');
DELETE FROM `gameobject` WHERE (`guid` = '300490');
DELETE FROM `gameobject` WHERE (`guid` = '300562');
DELETE FROM `gameobject` WHERE (`guid` = '300790');
DELETE FROM `gameobject` WHERE (`guid` = '300489');
DELETE FROM `gameobject` WHERE (`guid` = '300561');
DELETE FROM `gameobject` WHERE (`guid` = '300649');











UPDATE `creature_template_difficulty` SET `LootID` = '55601', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '55601') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '55601', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '55601') and (`DifficultyID` = '1');

DELETE FROM `creature_loot_template` WHERE `entry` IN (55601);
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,117,2.57307,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,118,1,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,159,1.63384,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,727,0.187846,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,766,0.293122,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,767,0.180621,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,768,0.069152,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,774,1,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,805,0.0350921,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,818,0.343696,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,828,0.0536702,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,856,0.00103212,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,858,0.069152,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1179,0.109405,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1411,0.215713,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1412,0.304475,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1413,0.282801,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1414,0.196103,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1415,0.263191,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1416,0.263191,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1417,0.233259,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1418,0.152754,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1419,0.135208,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1420,0.183717,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1422,0.205392,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1423,0.248741,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1425,0.157914,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1427,0.145529,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1429,0.157914,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1430,0.193006,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1431,0.167203,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,1433,0.17546,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2138,0.220874,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2140,0.025803,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2212,0.157914,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2213,0.163075,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2287,0.206424,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2406,0.0051606,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2455,0.017546,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2553,0.0123854,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2589,0.0650235,0,1,0,1,2,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2598,0.0712162,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2635,0.356081,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2642,0.493353,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2643,0.229131,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2645,0.158946,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2646,0.163075,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2648,0.144497,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2773,0.252869,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2774,0.221906,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2959,0.0237387,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2960,0.00206424,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,2962,0.00103212,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,3189,0.242548,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,3190,0.0897944,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,3200,0.00103212,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,3214,0.0051606,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,3370,0.167203,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,3373,0.197135,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,3609,0.0722484,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,3641,0.00103212,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,4496,0.0495417,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,4536,0.0051606,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,4560,0.127983,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,4561,0.0144497,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,4562,0.115597,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,4563,0.280737,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,4565,0.129015,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,4659,0.00103212,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,4663,0.00103212,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,4665,0.00103212,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,5069,0.0557345,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,5571,0.069152,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,5572,0.0464454,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,5574,0.0330278,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,6271,0.00619272,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,6337,0.0051606,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,6507,0.00309636,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,6508,0.0051606,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,6513,0.0577987,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,6514,0.00928908,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,6515,0.0567666,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,6517,0.00103212,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,6518,0.0051606,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,6519,0.00619272,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,6520,0.00722484,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,7350,0.0381884,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,8177,0.274544,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,8178,0.1414,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,8179,0.108373,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,8180,0.0103212,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,8181,0.151722,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,8182,0.335439,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,9743,0.00619272,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,9753,0.00103212,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,9760,0.0216745,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,15895,0.00206424,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,15932,0.0185781,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,55973,0.0350921,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,55983,0.0278672,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55601,0,74615,96.2761,1,1,0,1,1,'');
