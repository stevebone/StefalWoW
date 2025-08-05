-- NPC: 55477 Ji Firepaw at the farm
-- NPC: 55483 Plump Virmen
-- NPC: 55504 Plump Carrotcruncher
-- NPC: 55479 Gao Summerdraft
-- Quest: 29769 Rascals
-- Quest 29768 Missing mallet
-- Quest 29770 Still Good!

DELETE FROM `creature_queststarter` WHERE `quest` IN (29769,29770);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55477', '29769', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55479', '29770', '0');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29769);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29769', '0', '0', '0', '29680', '29768', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');


DELETE FROM `creature_text` WHERE `creatureID` IN (55477,55483,55504);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('55477', '0', '0', 'Wake... up... DANGIT!', '12', '0', '100', '15', '0', '27345', '60447', '0', 'Ji Firepaw'),
('55477', '1', '0', 'This is ridiculous...', '12', '0', '100', '274', '0', '27346', '60448', '0', 'Ji Firepaw'),
('55477', '2', '0', 'You sleep like a rock!', '12', '0', '100', '5', '0', '27347', '60449', '0', 'Ji Firepaw'),
('55477', '3', '0', 'There\'s no way you\'ll sleep through THIS.', '12', '0', '100', '274', '0', '27348', '60450', '0', 'Ji Firepaw'),
('55477', '4', '0', 'Seriously?', '12', '0', '100', '18', '0', '27349', '60451', '0', 'Ji Firepaw'),
('55477', '5', '0', 'Why...', '12', '0', '100', '507', '0', '27350', '60452', '0', 'Ji Firepaw'),
('55477', '6', '0', 'won\'t...', '12', '0', '100', '509', '0', '27351', '60453', '0', 'Ji Firepaw'),
('55477', '7', '0', 'you...', '12', '0', '100', '507', '0', '27352', '60454', '0', 'Ji Firepaw'),
('55477', '8', '0', 'just...', '12', '0', '100', '509', '0', '27353', '60455', '0', 'Ji Firepaw'),
('55477', '9', '0', 'WAKE UP?!', '12', '0', '100', '22', '0', '27354', '60456', '0', 'Ji Firepaw'),
('55477', '10', '0', 'I will break you, little rock man!', '12', '0', '100', '25', '0', '27355', '60457', '0', 'Ji Firepaw'),
('55477', '11', '0', '%s sighs.', '16', '0', '100', '0', '0', '0', '55942', '0', 'Ji Firepaw to Player'),
('55483', '0', '0', 'Gimme all your vegetables!', '12', '0', '100', '0', '0', '0', '54873', '0', 'Plump Virmen to Player'),
('55483', '0', '1', 'AIIIIEEEEEEE!', '12', '0', '100', '0', '0', '0', '54874', '0', 'Plump Virmen to Player'),
('55483', '0', '2', 'This virmen land!', '12', '0', '100', '0', '0', '0', '54876', '0', 'Plump Virmen to Player'),
('55483', '0', '3', 'You no take carrot! You take turnip instead!', '12', '0', '100', '0', '0', '0', '54877', '0', 'Plump Virmen to Player'),
('55483', '0', '4', 'We rowdy!', '12', '0', '100', '0', '0', '0', '54875', '0', 'Plump Virmen to Player'),
('55504', '0', '0', 'Gimme all your vegetables!', '12', '0', '100', '0', '0', '0', '54873', '0', 'Plump Carrotcruncher to Player'),
('55504', '0', '1', 'AIIIIEEEEEEE!', '12', '0', '100', '0', '0', '0', '54874', '0', 'Plump Carrotcruncher to Player'),
('55504', '0', '2', 'This virmen land!', '12', '0', '100', '0', '0', '0', '54876', '0', 'Plump Carrotcruncher to Player'),
('55504', '0', '3', 'You no take carrot! You take turnip instead!', '12', '0', '100', '0', '0', '0', '54877', '0', 'Plump Carrotcruncher to Player'),
('55504', '0', '4', 'We rowdy!', '12', '0', '100', '0', '0', '0', '54875', '0', 'Plump Carrotcruncher to Player');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55477,55483,55504);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55477,5547700,55483,55504);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('55483', '0', '0', '0', '4', '0', '25', '0', '0', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Plump Virmen - On Aggro - Talk'),
('55483', '0', '1', '0', '4', '0', '100', '0', '0', '0', '0', '0', '0', '', '30', '1', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Plump Virmen - On Aggro - Set Random Phase'),
('55483', '0', '2', '0', '0', '0', '100', '0', '2000', '5000', '5000', '8000', '0', '', '11', '107936', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Plump Virmen - Update IC - Cast Back Kick'),
('55483', '0', '3', '0', '0', '1', '100', '0', '5000', '8000', '8000', '12000', '0', '', '11', '107938', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Plump Virmen - Update IC - Cast Tail Thrash'),
('55483', '0', '4', '0', '0', '2', '100', '0', '5000', '8000', '15000', '18000', '0', '', '11', '107944', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Plump Virmen - Update IC - Cast Carrot Breath'),
('55504', '0', '0', '0', '4', '0', '25', '0', '0', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Plump Carrotcruncher - On Aggro - Talk'),
('55504', '0', '1', '0', '4', '0', '100', '0', '0', '0', '0', '0', '0', '', '30', '1', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Plump Carrotcruncher - On Aggro - Set Random Phase'),
('55504', '0', '2', '0', '0', '0', '100', '0', '2000', '5000', '5000', '8000', '0', '', '11', '107936', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Plump Carrotcruncher - Update IC - Cast Back Kick'),
('55504', '0', '3', '0', '0', '1', '100', '0', '5000', '8000', '8000', '12000', '0', '', '11', '107938', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Plump Carrotcruncher - Update IC - Cast Tail Thrash'),
('55504', '0', '4', '0', '0', '2', '100', '0', '5000', '8000', '15000', '20000', '0', '', '11', '107944', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Plump Carrotcruncher - Update IC - Cast Carrot Breath'),
('55477', '0', '0', '0', '11', '0', '100', '0', '0', '0', '0', '0', '0', '', '59', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - On Respawn - Set Run'),
('55477', '0', '1', '0', '1', '0', '100', '0', '1000', '1000', '116000', '116000', '0', '', '80', '5547700', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Update OOC - Run Script'),
('5547700', '9', '0', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '1', '0', '0', '0', '100', '0', '16000', '16000', '1000', '1000', '0', '', '1', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '2', '0', '0', '0', '100', '0', '17000', '17000', '1000', '1000', '0', '', '1', '2', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '3', '0', '0', '0', '100', '0', '18000', '18000', '1000', '1000', '0', '', '69', '1', '0', '0', '0', '0', '0', '8', '0', '0', '0', '628.734', '3138.05', '87.837', '0', 'Ji Firepaw - Move to Pos'),
('5547700', '9', '4', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '66', '0', '0', '0', '0', '0', '0', '8', '0', '0', '0', '0', '0', '0', '6.21337', 'Ji Firepaw - Set Orientation'),
('5547700', '9', '5', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '1', '3', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '6', '0', '0', '0', '100', '0', '4000', '4000', '1000', '1000', '0', '', '11', '118026', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Cast SpecialUnarmed w/ Gong Song'),
('5547700', '9', '7', '0', '0', '0', '100', '0', '3500', '3500', '1000', '1000', '0', '', '1', '4', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '8', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '69', '2', '0', '0', '0', '0', '0', '8', '0', '0', '0', '629.354', '3139.15', '87.837', '0', 'Ji Firepaw - Move to Pos'),
('5547700', '9', '9', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '66', '0', '0', '0', '0', '0', '0', '8', '0', '0', '0', '0', '0', '0', '0.558505', 'Ji Firepaw - Set Orientation'),
('5547700', '9', '10', '0', '0', '0', '100', '0', '4000', '4000', '1000', '1000', '0', '', '1', '5', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '11', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '17', '510', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Set Emote State'),
('5547700', '9', '12', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '1', '6', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '13', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '1', '7', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '14', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '1', '8', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '15', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '17', '30', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Set Emote State'),
('5547700', '9', '16', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '1', '9', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '17', '0', '0', '0', '100', '0', '20000', '20000', '1000', '1000', '0', '', '1', '10', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '18', '0', '0', '0', '100', '0', '13000', '13000', '1000', '1000', '0', '', '5', '20', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Play Emote');


UPDATE `creature_template_difficulty` SET `LootID` = '55483', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '55483') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '55483', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '55483') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `LootID` = '55504', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '55504') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '55504', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '55504') and (`DifficultyID` = '1');


DELETE FROM `creature_loot_template` WHERE `entry` IN (55483,55504);
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,117,0.0280956,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,118,1.65452,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,159,3.15919,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,727,0.115504,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,766,0.290321,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,767,0.223204,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,768,0.0421434,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,774,1.76846,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,805,0.1077,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,818,0.668051,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,828,0.0967737,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,858,0.0046826,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1179,0.0187304,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1411,0.629029,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1412,0.55879,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1413,0.660246,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1414,0.561912,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1415,0.588446,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1416,0.658685,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1417,0.616542,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1418,0.404264,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1419,0.362121,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1420,0.580642,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1422,0.569716,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1423,0.408947,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1425,0.416751,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1427,0.368364,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1429,0.344951,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1430,0.474503,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1431,0.527573,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1433,0.708633,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,1438,0.00156087,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2075,0.0109261,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2138,0.530694,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2140,0.00312173,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2212,0.494794,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2213,0.732046,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2407,0.0093652,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2455,0.00312173,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2555,0.0124869,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2589,75.7426,0,1,0,1,2,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2598,0.212278,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2635,0.313734,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2642,0.443286,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2643,0.335586,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2645,0.279395,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2646,0.647759,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2648,0.307491,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2773,0.536938,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2774,0.552547,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2958,0.0202913,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2959,0.00624346,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2960,0.00312173,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2961,0.00156087,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2962,0.0156087,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2963,0.0874085,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2966,0.0343391,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2967,0.168573,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,2968,0.223204,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,3189,0.412069,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,3190,0.156087,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,3214,0.0561912,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,3370,0.340269,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,3373,0.401143,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,3609,0.16233,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,3610,0.00156087,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,4408,0.00156087,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,4496,0.142039,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,4536,6.53066,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,4537,0.0296564,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,4560,0.112382,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,4561,0.0187304,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,4562,0.0171695,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,4563,0.1436,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,4565,0.1077,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,4658,0.00624346,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,4662,0.00780433,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,4663,0.00312173,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,4666,0.0561912,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,4677,0.00156087,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,4689,0.0046826,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,5069,0.00624346,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,5571,0.0920911,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,5572,0.159208,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,6271,0.0093652,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,6337,0.0156087,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,6342,0.0093652,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,6506,0.0327782,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,6507,0.00156087,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,6508,0.00156087,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,6511,0.0343391,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,6513,0.0358999,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,6514,0.0140478,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,6519,0.00156087,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,6520,0.0202913,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,6521,0.00624346,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,7109,0.00156087,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,8177,0.280956,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,8178,0.112382,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,8179,0.0280956,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,8180,0.101456,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,8181,0.140478,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,8182,0.358999,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,9745,0.00156087,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,9754,0.023413,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,15932,0.00312173,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,55973,0.190426,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,55983,0.174817,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,56234,0.0156087,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55483,0,62391,0.00312173,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,118,0.630951,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,159,1.79534,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,727,0.00573592,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,766,0.0344155,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,767,0.0516233,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,768,0.0114718,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,774,0.831708,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,805,0.0458874,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,818,0.412986,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,828,0.0860388,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,858,0.0172077,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1411,0.240909,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1412,0.177814,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1413,0.206493,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1414,0.25238,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1415,0.28106,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1416,0.659631,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1417,0.15487,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1418,0.212229,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1419,0.235173,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1420,0.200757,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1422,0.206493,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1423,0.217965,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1425,0.131926,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1427,0.212229,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1429,0.321211,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1430,0.172078,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1431,0.189285,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1433,0.143398,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,1438,0.0975106,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2138,0.321211,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2140,0.0114718,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2212,0.177814,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2213,0.166342,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2455,0.00573592,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2589,39.2222,0,1,0,1,2,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2598,0.0344155,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2635,0.636687,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2642,0.143398,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2643,0.149134,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2645,0.269588,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2646,0.0917747,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2648,0.15487,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2773,0.28106,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2774,0.344155,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2958,0.00573592,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2965,0.00573592,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,2967,0.143398,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,3190,0.195021,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,3370,0.206493,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,3373,0.200757,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,3609,0.0917747,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,4496,0.0458874,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,4536,3.43008,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,4537,0.0458874,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,4538,0.00573592,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,4560,0.00573592,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,4561,0.0573592,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,4563,0.189285,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,4565,0.860388,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,4663,0.00573592,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,5571,0.0458874,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,5572,0.0458874,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,6337,0.12619,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,6507,0.00573592,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,6513,0.0344155,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,6520,0.00573592,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,7108,0.00573592,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,8178,0.0114718,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,8180,0.149134,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,8181,0.0803029,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,9743,0.00573592,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,9758,0.0630951,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55504,0,74296,77,1,1,0,1,1,'');
