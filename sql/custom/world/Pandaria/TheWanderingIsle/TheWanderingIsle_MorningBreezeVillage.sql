-- Quest: 29777 Tools of the enemy
-- Quest: 29778 Rewritten wisdoms
-- Quest: 29783 Stronger than stone
-- Quest: 29779 The direct solution
-- Quest: 29780 Do no evil
-- Quest: 29781 Monkey Advisory warning
-- Quest: 29784 Balanced perspective
-- Quest: 29785 Dafeng-the-spirit-of-air

-- NPC: 57623 Shen Stonecarver
-- NPC: 55588 Elder Shaopai at Morning Breeze
-- NPC: 55583 Ji Firepaw at Morning Breeze
-- NPC: 55585 Jojo at Morning Breeze
-- NPC: 57670 Jojo spawn at Morning Breeze
-- NPC: 55601 Fe-feng Wiseman
-- NPC: 55632 Fe-Feng Ruffian
-- NPC: 57465 Fe-Feng Ruffian
-- NPC: 57466 Fe-Feng Firethief
-- NPC: 55633 Fe-Feng Firethief
-- NPC: 55634 Ruk-Ruk
-- NPC: 64322 Ruk-Ruk's rocket
-- NPC: 65558 Ji Firepaw (with player)

DELETE FROM `creature_queststarter` WHERE `quest` IN (29777,29778,29783);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES 
('55588', '29777', '0'),
('55583', '29778', '0'),
('55585', '29783', '0');

DELETE FROM `creature_queststarter` WHERE `quest` IN (29779,29780,29781);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55583', '29779', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55583', '29780', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55583', '29781', '0');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29777,29778,29783);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29777', '0', '0', '0', '29776', '0', '-29778', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29778', '0', '0', '0', '29776', '0', '-29778', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29783', '0', '0', '0', '29776', '0', '-29778', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29779,29780,29781);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29779', '0', '0', '0', '29778', '29784', '-29779', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29780', '0', '0', '0', '29778', '29784', '-29779', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29781', '0', '0', '0', '29778', '29784', '-29779', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `creature_queststarter` WHERE `quest` IN (29785);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55595', '29785', '0');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29784,29785);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29784', '0', '0', '116992', '29780', '29785', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29785', '0', '0', '0', '29784', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

-- Scripts for Jojo and monkeys
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55601,57670,65467,56394,56393);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55601,57670,5767000,65467,56394,56393,6546700,5639400,5639300) AND `source_type` IN (0,9);
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

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (57465,57466,55633,55632);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (57465,57466,55633,55632) AND `source_type` = 0;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('57466', '0', '0', '0', '0', '0', '100', '0', '4000', '8000', '15000', '20000', '0', '', '11', '109098', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Firethief - Update IC - Cast Set Ablaze'),
('55633', '0', '0', '0', '0', '0', '100', '0', '4000', '8000', '8000', '12000', '0', '', '11', '127940', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Firethief - Update IC - Cast Shoot'),
('55633', '0', '1', '0', '0', '0', '100', '0', '10000', '15000', '10000', '15000', '0', '', '11', '109104', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Firethief - Update IC - Cast Shoot ALL the fireworks!'),
('57465', '0', '0', '0', '0', '0', '100', '0', '4000', '8000', '8000', '12000', '0', '', '11', '128414', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Ruffian - Update IC - Cast Display of Fury'),
('55632', '0', '0', '0', '0', '0', '100', '0', '4000', '8000', '8000', '12000', '0', '', '11', '128414', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Ruffian - Update IC - Cast Display of Fury');

DELETE FROM `npc_vendor` WHERE `entry` IN (57623);
INSERT INTO `npc_vendor` (entry, slot, item, maxcount, incrtime, ExtendedCost, type, BonusListIDs, PlayerConditionID, IgnoreFiltering, VerifiedBuild) VALUES
('57623', '1', '2880', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '2', '2901', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '3', '3466', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '4', '3857', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '5', '5956', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '6', '18567', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '7', '160298', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '8', '180733', '0', '0', '0', '1', NULL, '0', '0', '63906');

UPDATE `creature_template` SET `ScriptName` = 'npc_ruk_ruk' WHERE `Entry` = 55634;
UPDATE `creature_template` SET `ScriptName` = 'npc_ruk_ruk_rocket' WHERE `Entry` = 64322;

UPDATE `creature_template` SET `faction` = '16' WHERE (`entry` = '57465'); -- was 14
UPDATE `creature_template` SET `unit_flags` = '0' WHERE (`entry` = '55632'); -- was 537133824

DELETE FROM `creature_template_difficulty` WHERE `entry` IN (55601);
INSERT INTO `creature_template_difficulty` VALUES
('55601', '0', '0', '0', '80', '4', '1', '1', '1', '0.2', '29467', '0', '0', '0', '55601', '55601', '0', '5', '5', '268435456', '0', '0', '0', '0', '0', '0', '0', '56647');

-- Fe-Feng Firethief stolen cannons aura
DELETE FROM `creature_template_addon` WHERE `entry` = 55633;
INSERT INTO `creature_template_addon` (`entry`, `SheathState`, `auras`) VALUES ('55633', '1', '127932');

DELETE FROM `creature_addon` WHERE `guid` IN (451477,451621);
INSERT INTO `creature_addon` (`guid`, `AnimTier`, `VisFlags`, `SheathState`, `auras`) VALUES ('451477', '2', '1', '1', '78718 121801 81312'); -- NPC: 55595 Aysa at Morning Breeze
INSERT INTO `creature_addon` (`guid`, `StandState`, `SheathState`) VALUES ('451621', '8', '1'); -- NPC: 55595 Aysa at ZhaoRen

DELETE FROM `creature_loot_template` WHERE `entry` IN (55601);
INSERT INTO `creature_loot_template` (Entry, ItemType, Item, Chance, QuestRequired, LootMode, GroupId, MinCount, MaxCount, Comment) VALUES
('55601', '0', '117', '2.55494', '0', '1', '0', '1', '1', 'Tough Jerky'),
('55601', '0', '118', '0.885809', '0', '1', '0', '1', '1', 'Minor Healing Potion'),
('55601', '0', '159', '1.62347', '0', '1', '0', '1', '1', 'Refreshing Spring Water'),
('55601', '0', '727', '0.185685', '0', '1', '0', '1', '1', 'Notched Shortsword'),
('55601', '0', '766', '0.29324', '0', '1', '0', '1', '1', 'Flanged Mace'),
('55601', '0', '767', '0.185685', '0', '1', '0', '1', '1', 'Long Bo Staff'),
('55601', '0', '774', '1', '0', '1', '0', '1', '1', 'Malachite'),
('55601', '0', '818', '0.341944', '0', '1', '0', '1', '1', 'Tigerseye'),
('55601', '0', '1179', '0.10857', '0', '1', '0', '1', '1', 'Ice Cold Milk'),
('55601', '0', '1411', '0.214096', '0', '1', '0', '1', '1', 'Withered Staff'),
('55601', '0', '1412', '0.299328', '0', '1', '0', '1', '1', 'Crude Bastard Sword'),
('55601', '0', '1413', '0.28005', '0', '1', '0', '1', '1', 'Feeble Sword'),
('55601', '0', '1414', '0.194817', '0', '1', '0', '1', '1', 'Cracked Sledge'),
('55601', '0', '1415', '0.261785', '0', '1', '0', '1', '1', 'Carpenter\'s Mallet'),
('55601', '0', '1416', '0.260771', '0', '1', '0', '1', '1', 'Rusty Hatchet'),
('55601', '0', '1417', '0.233375', '0', '1', '0', '1', '1', 'Beaten Battle Axe'),
('55601', '0', '1418', '0.150172', '0', '1', '0', '1', '1', 'Worn Leather Belt'),
('55601', '0', '1419', '0.133937', '0', '1', '0', '1', '1', 'Worn Leather Boots'),
('55601', '0', '1420', '0.180612', '0', '1', '0', '1', '1', 'Worn Leather Bracers'),
('55601', '0', '1422', '0.202934', '0', '1', '0', '1', '1', 'Worn Leather Gloves'),
('55601', '0', '1423', '0.245551', '0', '1', '0', '1', '1', 'Worn Leather Pants'),
('55601', '0', '1425', '0.157274', '0', '1', '0', '1', '1', 'Worn Leather Vest'),
('55601', '0', '1427', '0.144083', '0', '1', '0', '1', '1', 'Patchwork Shoes'),
('55601', '0', '1429', '0.15626', '0', '1', '0', '1', '1', 'Patchwork Cloak'),
('55601', '0', '1430', '0.191773', '0', '1', '0', '1', '1', 'Patchwork Gloves'),
('55601', '0', '1431', '0.164377', '0', '1', '0', '1', '1', 'Patchwork Pants'),
('55601', '0', '1433', '0.172494', '0', '1', '0', '1', '1', 'Patchwork Armor'),
('55601', '0', '2138', '0.218155', '0', '1', '0', '1', '1', 'Sharpened Letter Opener'),
('55601', '0', '2212', '0.15626', '0', '1', '0', '1', '1', 'Cracked Buckler'),
('55601', '0', '2213', '0.161333', '0', '1', '0', '1', '1', 'Worn Large Shield'),
('55601', '0', '2287', '0.203949', '0', '1', '0', '1', '1', 'Haunch of Meat'),
('55601', '0', '2635', '0.350062', '0', '1', '0', '1', '1', 'Loose Chain Belt'),
('55601', '0', '2642', '0.485013', '0', '1', '0', '1', '1', 'Loose Chain Boots'),
('55601', '0', '2643', '0.228301', '0', '1', '0', '1', '1', 'Loose Chain Bracers'),
('55601', '0', '2645', '0.158289', '0', '1', '0', '1', '1', 'Loose Chain Gloves'),
('55601', '0', '2646', '0.160318', '0', '1', '0', '1', '1', 'Loose Chain Pants'),
('55601', '0', '2648', '0.143069', '0', '1', '0', '1', '1', 'Loose Chain Vest'),
('55601', '0', '2773', '0.250624', '0', '1', '0', '1', '1', 'Cracked Shortbow'),
('55601', '0', '2774', '0.222213', '0', '1', '0', '1', '1', 'Rust-Covered Blunderbuss'),
('55601', '0', '3189', '0.244536', '0', '1', '0', '1', '1', 'Wood Chopper'),
('55601', '0', '3370', '0.165392', '0', '1', '0', '1', '1', 'Patchwork Belt'),
('55601', '0', '3373', '0.194817', '0', '1', '0', '1', '1', 'Patchwork Bracers'),
('55601', '0', '4560', '0.127849', '0', '1', '0', '1', '1', 'Fine Scimitar'),
('55601', '0', '4562', '0.115673', '0', '1', '0', '1', '1', 'Severing Axe'),
('55601', '0', '4563', '0.277005', '0', '1', '0', '1', '1', 'Billy Club'),
('55601', '0', '4565', '0.126834', '0', '1', '0', '1', '1', 'Simple Dagger'),
('55601', '0', '8177', '0.27802', '0', '1', '0', '1', '1', 'Practice Sword'),
('55601', '0', '8178', '0.141039', '0', '1', '0', '1', '1', 'Training Sword'),
('55601', '0', '8179', '0.107555', '0', '1', '0', '1', '1', 'Cadet\'s Bow'),
('55601', '0', '8181', '0.152201', '0', '1', '0', '1', '1', 'Hunting Rifle'),
('55601', '0', '8182', '0.334842', '0', '1', '0', '1', '1', 'Pellet Rifle'),
('55601', '0', '74615', '100', '1', '1', '0', '1', '1', 'Paint Soaked Brush');

UPDATE `gameobject_template` SET `Data6` = 120000, `Data23` = 1 WHERE `entry` IN (209656, 209660, 209661, 209663);

UPDATE `gameobject_template_addon` SET `flags` = 0 WHERE `entry` IN (209660, 209661); -- was 36

DELETE FROM `gameobject`
WHERE `guid` IN (
    300646, 300791, 300643, 300789, 300540, 300642, 300648, 300539,
    300644, 300787, 300788, 300759, 300751, 300753, 300466, 300560,
    300793, 300488, 300650, 300792, 300490, 300562, 300790, 300489,
    300561, 300649
); -- duplicate spawns or maybe for another phase????