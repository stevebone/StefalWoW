-- Quest: 29789 Small but significant
-- Quest: 29788 Unwelcome nature
-- Quest: 29790 Passing Wisdom
-- NPC: 55672 Master Shang Xi in the woods
-- NPC: 56686 Another Shang in the forest (spawn)
-- NPC: 55640 Thornbranch Scamp

DELETE FROM `quest_template_addon` WHERE `ID` IN (29788, 29789, 29790);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29788', '0', '0', '0', '29787', '29790', '-29788', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29789', '0', '0', '0', '29787', '29790', '-29788', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29790', '0', '0', '0', '29788', '29791', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `creature_queststarter` WHERE `quest` IN (29788, 29789,29790);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55672', '29788', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55672', '29789', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55672', '29790', '0');

DELETE FROM `creature_text` WHERE `creatureID` IN (56686);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('56686', '0', '0', 'For 3,000 years, we have passed the knowledge of our people down. Elder to youth. Master to student.', '12', '0', '100', '1', '0', '27807', '55528', '0', 'Master Shang Xi to Player'),
('56686', '1', '0', 'Every elder reaches the day where he must pass on and plant his stave with the staves of his ancestors. Today is the day when my staff joins these woods.', '12', '0', '100', '1', '0', '27808', '55529', '0', 'Master Shang Xi to Player'),
('56686', '2', '0', '$p, our people have lived the wholes of their lives on this great turtle, Shen-zin Su, but not in hundreds of years has anyone spoken to him.', '12', '0', '100', '1', '0', '27809', '55530', '0', 'Master Shang Xi to Player'),
('56686', '3', '0', 'Now Shen-zin Su is ill, and we are all in danger. With the help of the elements, you will break the silence. You will speak to him.', '12', '0', '100', '1', '0', '27810', '55531', '0', 'Master Shang Xi to Player'),
('56686', '4', '0', 'Aysa and Ji have retrieved the spirits and brought them here. You are to go with them, speak to the great Shen-zin Su, and do what must be done to save our people.', '12', '0', '100', '1', '0', '27811', '55532', '0', 'Master Shang Xi to Player'),
('56686', '5', '0', 'You\'ve come far, my young student. I see within you a great hero. I leave the fate of this land to you.', '12', '0', '100', '396', '0', '27812', '55533', '0', 'Master Shang Xi to Player');


UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55640,56686);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55640,56686,5668600,5668601,5668602,5668603);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('55640', '0', '0', '0', '0', '0', '100', '0', '3000', '6000', '15000', '20000', '0', '', '11', '109126', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Thornbranch Scamp - Update IC - Cast Mirror Images'),
('56686', '0', '0', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5668600', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Just Spawned - Run Script'),
('56686', '0', '1', '2', '40', '0', '100', '0', '0', '22', '0', '0', '0', '', '54', '10000', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - WP Reached - Pause WP'),
('56686', '0', '2', '3', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '66', '0', '0', '0', '0', '0', '0', '8', '0', '0', '0', '0', '0', '0', '0.699036', 'Master Shang - WP Reached - Set Orientation'),
('56686', '0', '3', '4', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '5', '396', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - WP Reached - Play Emote'),
('56686', '0', '4', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5668601', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - WP Reached - Run Script'),
('56686', '0', '5', '6', '40', '0', '100', '0', '4', '22', '0', '0', '0', '', '54', '15000', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - WP Reached - Pause WP'),
('56686', '0', '6', '7', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '66', '0', '0', '0', '0', '0', '0', '8', '0', '0', '0', '0', '0', '0', '4.8168', 'Master Shang - WP Reached - Set Orientation'),
('56686', '0', '7', '8', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '4', '33098', '0', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - WP Reached - Play Sound'),
('56686', '0', '8', '9', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '11', '56913', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - WP Reached - Cast CSA Dummy Effect Self'),
('56686', '0', '9', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '1', '3', '3500', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - WP Reached - Talk'),
('56686', '0', '10', '11', '52', '0', '100', '0', '3', '56686', '0', '0', '0', '', '28', '126160', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On Text Over - Remove Aura'),
('56686', '0', '11', '12', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '11', '128850', '0', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On Text Over - Cast Forcecast Summon Walking Stick, Blossoming'),
('56686', '0', '12', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5668602', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On Text Over - Run Script'),
('56686', '0', '13', '14', '58', '0', '100', '0', '5', '22', '0', '0', '0', '', '66', '0', '0', '0', '0', '0', '0', '8', '0', '0', '0', '0', '0', '0', '4.29266', 'Master Shang - WP Ended - Set Orientation'),
('56686', '0', '14', '15', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '1', '4', '0', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - WP Ended - Talk'),
('56686', '0', '15', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5668603', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - WP Ended - Run Script'),
('5668600', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '4', '33097', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Play Sound'),
('5668600', '9', '1', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Talk'),
('5668600', '9', '2', '0', '0', '0', '100', '0', '6500', '6500', '1000', '1000', '0', '', '5', '396', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Play Emote'),
('5668600', '9', '3', '0', '0', '0', '100', '0', '5500', '5500', '1000', '1000', '0', '', '1', '1', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Talk'),
('5668600', '9', '4', '0', '0', '0', '100', '0', '6500', '6500', '1000', '1000', '0', '', '136', '0', '1', '5', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Set Movement Speed'),
('5668600', '9', '5', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '53', '0', '22', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Start WP'),
('5668601', '9', '0', '0', '0', '0', '100', '0', '6000', '6000', '1000', '1000', '0', '', '1', '2', '0', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Talk'),
('5668602', '9', '0', '0', '0', '0', '100', '0', '4000', '4000', '1000', '1000', '0', '', '5', '25', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Play Emote'),
('5668602', '9', '1', '0', '0', '0', '100', '0', '2500', '2500', '1000', '1000', '0', '', '5', '25', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Play Emote'),
('5668603', '9', '0', '0', '0', '0', '100', '0', '8000', '8000', '1000', '1000', '0', '', '5', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Play Emote'),
('5668603', '9', '1', '0', '0', '0', '100', '0', '7000', '7000', '1000', '1000', '0', '', '1', '5', '0', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Talk'),
('5668603', '9', '2', '0', '0', '0', '100', '0', '5500', '5500', '1000', '1000', '0', '', '90', '8', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Set Bytes1'),
('5668603', '9', '3', '0', '0', '0', '100', '0', '2500', '2500', '1000', '1000', '0', '', '11', '128851', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Cast Master Shang Spirit Transform'),
('5668603', '9', '4', '0', '0', '0', '100', '0', '1500', '1500', '1000', '1000', '0', '', '11', '109336', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Cast Trigger Walking Stick Blossom'),
('5668603', '9', '5', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '11', '106625', '0', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Cast Planting Stave Credit'),
('5668603', '9', '6', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '41', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Despawn');

DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55672);
INSERT INTO `smart_scripts` VALUES
('55672', '0', '0', '1', '', '19', '0', '100', '0', '29790', '0', '0', '0', '0', '', '85', '106623', '2', '0', '0', '0', '0', '0', NULL, '7', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Master Shang - On Accepted Quest - Invoker Cast Summon Master Shang Xi'),
('55672', '0', '1', '0', '', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '41', '1000', '30000', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Master Shang - On Accepted Quest - Invoker Cast Summon Master Shang Xi'),
('55672', '0', '2', '0', '', '20', '0', '100', '0', '29787', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Master Shang - On Reward Quest - Master Shang Xi Talk');


DELETE FROM `waypoint_path` WHERE `PathId` IN (22);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(22,0,0,'Master Shang In forest spawn - Quest accept 29790');


DELETE FROM `waypoint_path_node` WHERE `PathId` IN (22);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
('22', '0', '871.057', '4460.55', '241.45'),
('22', '1', '868.007', '4464.84', '241.665'),
('22', '2', '869.679', '4467.75', '241.748'),
('22', '3', '872.245', '4467.27', '241.643'),
('22', '4', '872.793', '4465.13', '241.417'),
('22', '5', '874.205', '4464.75', '241.382');


UPDATE `creature_template_difficulty` SET `LootID` = '55640', `GoldMin` = '9', `GoldMax` = '9' WHERE (`Entry` = '55640') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '55640', `GoldMin` = '9', `GoldMax` = '9' WHERE (`Entry` = '55640') and (`DifficultyID` = '1');

DELETE FROM `creature_loot_template` WHERE `entry` = 55640;
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,117,0.014668,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,159,0.0091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,727,0.0669227,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,766,0.247523,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,767,0.271358,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,768,0.263107,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,774,0.796656,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,805,0.03667,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,818,0.275025,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,828,0.0412537,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,856,0.00091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1411,0.21452,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1412,0.194351,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1413,0.178766,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1414,0.21727,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1415,0.1806,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1416,0.223687,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1417,0.162265,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1418,0.121928,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1419,0.14668,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1420,0.130178,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1422,0.219103,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1423,0.168682,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1425,0.179683,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1427,0.167765,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1429,0.139346,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1430,0.170515,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1431,0.153097,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,1433,0.161348,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2138,0.198935,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2140,0.0375867,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2212,0.113677,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2213,0.143013,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2553,0.007334,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2555,0.00091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2589,0.322696,0,1,0,1,2,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2598,0.0751735,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2635,0.125595,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2642,0.123761,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2643,0.11001,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2645,0.109093,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2646,0.145763,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2648,0.118261,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2773,0.189767,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2774,0.204435,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2959,0.0504212,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2961,0.0018335,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2962,0.0018335,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2963,0.00825075,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2964,0.00091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2966,0.0641725,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2967,0.00091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,2968,0.00091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,3189,0.297027,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,3190,0.224604,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,3370,0.255773,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,3373,0.137513,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,3609,0.084341,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,3641,0.0155848,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,4496,0.0449207,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,4536,0.0889247,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,4560,0.120094,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,4561,0.11001,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,4562,0.14943,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,4563,0.208102,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,4565,0.00275025,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,4659,0.014668,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,4666,0.0311695,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,5069,0.00091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,5571,0.033003,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,5572,0.0339198,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,6336,0.00091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,6337,0.0091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,6507,0.0155848,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,6513,0.00091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,6514,0.00091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,6515,0.051338,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,6517,0.00091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,7108,0.00091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,7109,0.00091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,7288,0.0238355,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,8177,0.25119,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,8178,0.194351,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,8179,0.0687563,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,8180,0.0485878,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,8181,0.091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,8182,0.161348,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,9746,0.00091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,9752,0.0091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,9753,0.0018335,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,9761,0.00091675,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,15925,0.00458375,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,55973,0.14668,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,55974,22,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,55982,0.153097,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,55983,78,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55640,0,74615,0.240188,1,1,0,1,1,'');
