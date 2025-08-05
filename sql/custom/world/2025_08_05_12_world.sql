-- NPC: 56730 Fe-Feng Brewthief
-- NPC: 57205 Fe-Feng Hozen
-- NPC: 53704 Corsac Fox
-- NPC: 57164 Fe-Feng Leaper
-- NPC: 53714 Training Target
-- NPC: 54130 Amberleaf Scamp
-- NPC: 57132 Wu-song Villager

-- Invalid scripts
DELETE FROM `smart_scripts` WHERE (`entryorguid` = '0') and (`source_type` = '9') and (`id` = '0') and (`link` = '0');
DELETE FROM `smart_scripts` WHERE (`entryorguid` = '0') and (`source_type` = '9') and (`id` = '1') and (`link` = '0');


UPDATE `creature_template` SET `AIName` = '' WHERE `Entry` IN (53714); -- no longer having scripts

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (56730,53704,57205,57164,54130);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (56730,53704,57205,57164,54130);
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
('56730', '0', '12', '0', '0', '0', '100', '0', '3000', '8000', '15000', '20000', '0', '', '11', '109084', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Brewthief - Update IC - Drunken Boxing'),
('53704', '0', '0', '0', '0', '0', '100', '0', '3000', '8000', '15000', '20000', '0', '', '11', '75533', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Corsac Fox - Update IC - Cast Wily Wits'),
('57205', '0', '0', '0', '0', '0', '100', '0', '3000', '8000', '15000', '20000', '0', '', '11', '114974', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Hozen - Update IC - Impale'),
('54130', '0', '0', '0', '0', '0', '100', '0', '3000', '8000', '15000', '20000', '0', '', '11', '109081', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Amberleaf Scamp - Update IC - Take This'),
('57164', '0', '0', '0', '4', '0', '100', '0', '0', '0', '0', '0', '0', '', '11', '107526', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Leaper - On Aggro - Cast Reckless Leap');

-- fixing incorrect parameters
UPDATE `smart_scripts` SET `action_param6` = '0', `target_type` = '1', `target_param1` = '0' WHERE (`entryorguid` = '5673000') and (`source_type` = '9') and (`id` = '1') and (`link` = '0');
UPDATE `smart_scripts` SET `action_param6` = '0', `target_type` = '1', `target_param1` = '0' WHERE (`entryorguid` = '5673001') and (`source_type` = '9') and (`id` = '3') and (`link` = '0');
UPDATE `smart_scripts` SET `action_param6` = '0', `target_type` = '1', `target_param1` = '0' WHERE (`entryorguid` = '5673003') and (`source_type` = '9') and (`id` = '1') and (`link` = '0');
UPDATE `smart_scripts` SET `action_param6` = '0', `target_type` = '1', `target_param1` = '0' WHERE (`entryorguid` = '5673004') and (`source_type` = '9') and (`id` = '3') and (`link` = '0');
UPDATE `smart_scripts` SET `target_type` = '1', `target_param1` = '0' WHERE (`entryorguid` = '5413000') and (`source_type` = '9') and (`id` = '1') and (`link` = '0');
UPDATE `smart_scripts` SET `target_type` = '1', `target_param1` = '0' WHERE (`entryorguid` = '5413001') and (`source_type` = '9') and (`id` = '1') and (`link` = '0');
UPDATE `smart_scripts` SET `target_type` = '1', `target_param1` = '0' WHERE (`entryorguid` = '5413002') and (`source_type` = '9') and (`id` = '1') and (`link` = '0');
UPDATE `smart_scripts` SET `target_type` = '1', `target_param1` = '0' WHERE (`entryorguid` = '5413003') and (`source_type` = '9') and (`id` = '1') and (`link` = '0');
UPDATE `smart_scripts` SET `action_param6` = '0' WHERE (`entryorguid` = '-450456') and (`source_type` = '0') and (`id` = '0') and (`link` = '0');
UPDATE `smart_scripts` SET `action_param6` = '0' WHERE (`entryorguid` = '-450451') and (`source_type` = '0') and (`id` = '0') and (`link` = '0');
UPDATE `smart_scripts` SET `action_param6` = '0' WHERE (`entryorguid` = '-450448') and (`source_type` = '0') and (`id` = '0') and (`link` = '0');
UPDATE `smart_scripts` SET `action_param6` = '0' WHERE (`entryorguid` = '-450436') and (`source_type` = '0') and (`id` = '0') and (`link` = '0');
UPDATE `smart_scripts` SET `action_param6` = '0' WHERE (`entryorguid` = '-450402') and (`source_type` = '0') and (`id` = '0') and (`link` = '0');
UPDATE `smart_scripts` SET `action_param6` = '0' WHERE (`entryorguid` = '-450326') and (`source_type` = '0') and (`id` = '0') and (`link` = '0');
UPDATE `smart_scripts` SET `action_param6` = '0' WHERE (`entryorguid` = '-450305') and (`source_type` = '0') and (`id` = '0') and (`link` = '0');
UPDATE `smart_scripts` SET `action_param6` = '0' WHERE (`entryorguid` = '34033') and (`source_type` = '0') and (`id` = '0') and (`link` = '0');
UPDATE `smart_scripts` SET `action_param6` = '0' WHERE (`entryorguid` = '34103') and (`source_type` = '0') and (`id` = '0') and (`link` = '0');
UPDATE `smart_scripts` SET `action_param6` = '0' WHERE (`entryorguid` = '962300') and (`source_type` = '9') and (`id` = '4') and (`link` = '0');


UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (57132);
DELETE FROM `smart_scripts` WHERE `entryorguid` = 57132;
INSERT INTO `smart_scripts` VALUES
('57132', '5', '11000', '0', '', '47', '0', '100', '0', '0', '0', '0', '0', '0', '', '85', '82238', '0', '0', '0', '0', '0', 0, NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'On Quest accept - Self: Cast spell 82238 on Self'),
('57132', '5', '11001', '0', '', '48', '0', '100', '0', '0', '0', '0', '0', '0', '', '85', '82238', '0', '0', '0', '0', '0', 0, NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'On Quest accept - Self: Cast spell 82238 on Self'),
('57132', '5', '11002', '0', '', '49', '0', '100', '0', '0', '0', '0', '0', '0', '', '85', '82238', '0', '0', '0', '0', '0', 0, NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'On Quest accept - Self: Cast spell 82238 on Self'),
('57132', '5', '11003', '0', '', '50', '0', '100', '0', '0', '0', '0', '0', '0', '', '85', '82238', '0', '0', '0', '0', '0', 0, NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'On Quest accept - Self: Cast spell 82238 on Self'),
('57132', '5', '11004', '0', '', '51', '0', '100', '0', '0', '0', '0', '0', '0', '', '85', '82238', '0', '0', '0', '0', '0', 0, NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'On Quest accept - Self: Cast spell 82238 on Self');



UPDATE `creature_template_difficulty` SET `LootID` = '53704' WHERE (`Entry` = '53704') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '53704' WHERE (`Entry` = '53704') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `LootID` = '57164', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '57164') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '57164', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '57164') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `LootID` = '57205', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '57205') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '57205', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '57205') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `LootID` = '56730', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '56730') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '56730', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '56730') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `LootID` = '54130', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '54130') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '54130', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '54130') and (`DifficultyID` = '1');

DELETE FROM `creature_loot_template` WHERE `entry` IN (53704,56730,57164,57205,54130);
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (53704,0,805,0.0666349,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (53704,0,1374,0.114231,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (53704,0,3300,23.6554,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (53704,0,3609,0.00951928,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (53704,0,5572,0.0475964,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (53704,0,55973,0.780581,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (53704,0,55983,0.52356,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (53704,0,62328,76.5635,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,117,32,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,159,17.7645,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,774,8.58283,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,805,2.39521,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,828,1.72987,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,1368,0.0665336,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,1374,0.0665336,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,1417,0.0665336,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,1429,0.0665336,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,2589,0.665336,0,1,0,2,2,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,2598,0.133067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,2649,0.0665336,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,3370,0.0665336,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,3609,0.0665336,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,4496,2.86094,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,4561,0.0665336,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,4562,0.0665336,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,4563,0.0665336,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,5571,3.12708,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,5572,2.12908,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,6514,0.0665336,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,8182,0.199601,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,55973,5.85496,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,55983,4.52428,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (56730,0,62328,0.399202,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57164,0,117,47.7901,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57164,0,159,11.6022,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57164,0,828,2.76243,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57164,0,1378,2.20994,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57164,0,2598,0.276243,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57164,0,3370,0.276243,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57164,0,4496,2,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57164,0,4560,0.276243,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57164,0,4562,0.276243,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57164,0,5571,1.65746,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57164,0,5572,0.828729,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57164,0,55973,13.2597,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57164,0,55983,2,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,117,28.3338,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,159,13.7587,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,766,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,767,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,774,6,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,805,3.47747,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,828,2.23768,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,1367,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,1368,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,1370,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,1377,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,1378,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,1380,0.332628,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,1413,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,1419,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,1430,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,1468,0.120955,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,2555,0.302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,2589,0.604778,0,1,0,1,2,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,2598,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,2635,0.0604778,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,2656,0.302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,2773,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,2968,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,3190,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,3300,0.514061,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,3365,0.27215,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,3609,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,4408,0.302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,4496,2.35863,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,4563,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,4663,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,5571,3.50771,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,5572,2.87269,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,6271,0.0604778,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,6342,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,8179,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,8181,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,8182,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,15895,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,15925,0.0302389,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,55973,10.3719,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,55983,7,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (57205,0,62328,1.4817,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,727,0.00107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,767,0.00107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,774,0.315849,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,805,0.0728059,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,828,0.152036,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,1364,0.00749473,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,1366,0.0107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,1367,0.0107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,1370,0.0042827,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,1376,0.00107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,1419,0.00107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,1430,0.00107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,1431,0.00214135,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,2589,0.00535338,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,2598,0.00107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,2645,0.00107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,2649,0.00214135,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,2651,0.00107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,2653,0.00214135,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,2654,0.00107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,2774,0.00107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,2968,0.00107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,3214,0.00107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,3300,0.00963608,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,3363,0.0738766,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,3609,0.0406857,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,4496,0.088866,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,4562,0.00107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,5571,0.0738766,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,5572,0.105997,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,6271,0.00107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,6342,0.00107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,8182,0.00214135,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,55973,62.2437,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,55974,0.00107067,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,55982,0.00214135,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,55983,32.5057,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,62328,0.0877954,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54130,0,72071,84.1797,1,1,0,1,1,'');
