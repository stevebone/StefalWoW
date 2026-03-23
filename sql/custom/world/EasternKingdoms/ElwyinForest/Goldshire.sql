-- Goldshire changes and updates

-- NPC: 30 Forest Spider
-- NPC: 40 Kobold Miner
-- NPC: 94 Cutpurse
-- NPC: 116 Bandit
-- NPC: 240 Marshal Dughan
-- NPC: 244 Ma Stonefield
-- NPC: 246 Bernice Stonefield
-- NPC: 247 Billy Maclure
-- NPC: 248 Gramma Stonefield
-- NPC: 250 Pa Maclure
-- NPC: 251 Maybell Maclure
-- NPC: 252 Joe Stonefield
-- NPC: 253 William Pestle
-- NPC: 255 Gerard Tiller
-- NPC: 258 Joshua Maclure
-- NPC: 261 Guard Thomas
-- NPC: 279 Morgan Pestle
-- NPC: 285 Murloc
-- NPC: 295 Innkeeper Farley
-- NPC: 327 Goldtooth
-- NPC: 330 Princess
-- NPC: 383 Jason Mathers
-- NPC: 448 Hogger
-- NPC: 473 Morgan The Collector
-- NPC: 475 Kobold Tunneler
-- NPC: 525 Mangy Wolf
-- NPC: 735 Murloc Streamrunner
-- NPC: 794 Matt
-- NPC: 795 Mark
-- NPC: 796 Joshua
-- NPC: 797 Bo
-- NPC: 804 Dana
-- NPC: 805 Cameron
-- NPC: 806 John
-- NPC: 807 Lisa
-- NPC: 810 Aaron
-- NPC: 811 Jose
-- NPC: 880 Erlan Drudgemoor
-- NPC: 881 Surena Caledon
-- NPC: 894 Homer Stonefield
-- NPC: 896 Veldan Lightfoot
-- NPC: 955 Sergeant de Vries
-- NPC: 963 Deputy Rainer
-- NPC: 5403 Stallion
-- NPC: 5406 Palomino
-- NPC: 6121 Remen Marcot
-- NPC: 6122 Gakin (Stormwind)
-- NPC: 6774 Falkhaan
-- NPC: 6846 Dockmaster
-- NPC: 6866 Bodyguard
-- NPC: 6927 Dock Worker
-- NPC: 7381 Cat
-- NPC: 7382 Cat
-- NPC: 7384 Cat
-- NPC: 7385 Cat
-- NPC: 11940 Merissa Stilwell
-- NPC: 42260 Stormwind Charger
-- NPC: 46932 Hogger Minion
-- NPC: 63014 Marcus Jensen

-- Quest: 61 Shipment to Stormwind
-- Quest: 123 The Collector
-- Quest: 176 WANTED: "Hogger"
-- Quest: 1685 Gakin's Summons
-- Quest: 1860 Speak with Jennea
-- Quest: 2205 Seek out SI7

-- Reactivating old/deprecated quests in Goldshire
DELETE FROM disables where sourceType = 1 AND entry = 61;
DELETE FROM disables where sourceType = 1 AND entry = 123;
DELETE FROM disables where sourceType = 1 AND entry = 1685;
DELETE FROM disables where sourceType = 1 AND entry = 1860;
DELETE FROM disables where sourceType = 1 AND entry = 2205;


-- Quest Starters / Enders
DELETE FROM `creature_queststarter` WHERE `quest` IN (61, 1685, 1860, 2205);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('253', '61', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('6121', '1685', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('6121', '1860', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('6121', '2205', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (61, 1685, 1860, 2205);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('279', '61', '0');
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('6122', '1685', '0');
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('5497', '1860', '0');
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('332', '2205', '0');

-- Quest Templates
UPDATE `quest_template` SET `ContentTuningID` = '73', `QuestSortID` = '12', `AllowableRaces` = '18446744073709551615', `Expansion` = '0' WHERE `ID` IN (61, 1860, 2205);
UPDATE `quest_template` SET `ContentTuningID` = '73', `QuestSortID` = '12', `RewardFactionID1` = '72', `RewardFactionValue1` = '1', `RewardFactionFlags` = '1', `AllowableRaces` = '18446744073709551615', `Expansion` = '0' WHERE (`ID` = '1685');
UPDATE `quest_template_addon` SET `AllowableClasses` = '0' WHERE `ID` IN (1685);
UPDATE `quest_template_addon` SET `AllowableClasses` = '0', `NextQuestID` = 1861, `BreadcrumbForQuestId` = 0 WHERE `ID` IN (1860);
UPDATE `quest_template_addon` SET `AllowableClasses` = '0', `NextQuestID` = 2206, `BreadcrumbForQuestId` = 0 WHERE `ID` IN (2205);

-- Creature Templates
UPDATE `creature_template` SET `faction` = '12', `npcflag` = '0', `AIName` = '' WHERE (`entry` = '383'); -- Jason Mathers updates
UPDATE `creature_template` SET `faction` = '12' WHERE `entry` IN (244, 246, 247, 248, 250, 251, 252, 255, 258, 794, 795, 796, 797, 804, 805, 806, 807, 810, 811, 894, 896, 955, 963); -- incorrect faction
UPDATE `creature_template` SET `faction` = '35' WHERE `entry` IN (5403, 5406); -- incorrect faction
UPDATE `creature_template` SET `npcflag` = '179' WHERE (`entry` = '63014'); -- Marcus Jensen npc flag
UPDATE `creature_template` SET `AIName` = '' WHERE `entry` IN (5406, 6121); -- remove AI Name for creatures without scripts
UPDATE `creature_template_difficulty` SET `DifficultyID` = '0', `ContentTuningID` = '2' WHERE (`Entry` = '6866') and (`DifficultyID` = '2'); -- difficulty fix for bodyguard
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (6866);
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_hogger', `AIName` = '' WHERE `entry` IN (448);
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_hogger_minion' WHERE `entry` IN (46932);

-- Creature Texts
DELETE FROM `creature_text` WHERE (`CreatureID` = '40') and (`GroupID` = '2') and (`ID` = '0');
DELETE FROM `creature_text` WHERE `CreatureID` IN (94, 116, 6866);

INSERT INTO `creature_text` VALUES
('94', '0', '0', 'You\'re in the wrong place at the wrong time, $g pal:missy;!', '12', '0', '100', '0', '0', '0', '0', '42883', '0', 'combat Say'),
('94', '0', '1', 'Give me all your gold!', '12', '0', '100', '0', '0', '0', '0', '0', '0', 'combat Say'),
('94', '0', '2', 'No one here gets out alive!', '12', '0', '100', '0', '0', '0', '0', '0', '0', 'combat Say');

INSERT INTO `creature_text` VALUES
('116', '0', '0', 'You\'re in the wrong place at the wrong time, $g pal:missy;!', '12', '0', '100', '0', '0', '0', '0', '42883', '0', 'combat Say'),
('116', '0', '1', 'Give me all your gold!', '12', '0', '100', '0', '0', '0', '0', '0', '0', 'combat Say'),
('116', '0', '2', 'No one here gets out alive!', '12', '0', '100', '0', '0', '0', '0', '0', '0', 'combat Say');

INSERT INTO `creature_text` VALUES
('6866', '0', '0', 'You\'re in the wrong place at the wrong time, $g pal:missy;!', '12', '0', '100', '0', '0', '0', '0', '42883', '0', 'combat Say'),
('6866', '0', '1', 'Give me all your gold!', '12', '0', '100', '0', '0', '0', '0', '0', '0', 'combat Say'),
('6866', '0', '2', 'No one here gets out alive!', '12', '0', '100', '0', '0', '0', '0', '0', '0', 'combat Say');

-- Hogger Event text fixes
DELETE FROM `creature_text` WHERE (`CreatureID` = '448') and (`GroupID` = '7') and (`ID` = '0');
UPDATE `creature_text` SET `comment` = 'Hogger' WHERE (`CreatureID` = '448') and (`GroupID` = '3') and (`ID` = '0');
UPDATE `creature_text` SET `comment` = 'Hogger' WHERE (`CreatureID` = '448') and (`GroupID` = '4') and (`ID` = '0');
UPDATE `creature_text` SET `comment` = 'Hogger' WHERE (`CreatureID` = '448') and (`GroupID` = '5') and (`ID` = '0');
DELETE FROM `creature_text` WHERE (`CreatureID` = '46943') and (`GroupID` = '0') and (`ID` = '1');
DELETE FROM `creature_text` WHERE (`CreatureID` = '65153') and (`GroupID` = '10') and (`ID` = '0');
UPDATE `creature_text` SET `comment` = 'Hogger' WHERE (`CreatureID` = '448') and (`GroupID` = '6') and (`ID` = '0');
UPDATE `creature_text` SET `Probability` = '100' WHERE (`CreatureID` = '46943') and (`GroupID` = '0') and (`ID` = '0');
UPDATE `creature_text` SET `Probability` = '100' WHERE (`CreatureID` = '46943') and (`GroupID` = '1') and (`ID` = '0');
UPDATE `creature_text` SET `Type` = '14', `Language` = '0', `Probability` = '100', `Emote` = '396' WHERE (`CreatureID` = '65153') and (`GroupID` = '0') and (`ID` = '0');
UPDATE `creature_text` SET `Type` = '12', `Language` = '0', `Probability` = '100', `Emote` = '396', `Duration` = '0' WHERE (`CreatureID` = '65153') and (`GroupID` = '13') and (`ID` = '0');

-- Creature Spawns
-- Remove no longer needed Hogger quest spawns
DELETE FROM `creature` WHERE `guid` IN (280327, 280328, 280329, 280330, 280331);

-- Scripts Cleanup
DELETE FROM `smart_scripts` WHERE  `entryorguid` IN (30, 94, 383, 525, 795, 796, 797, 804, 805, 806, 807, 810, 811, 7381, 7382, 7384, 7385, 11940, 63014) AND `id` IN (11000, 11001, 11002, 11003, 11004);
DELETE FROM `smart_scripts` WHERE  `entryorguid` IN (40, 244, 246, 247, 248, 250, 251, 252, 253, 255, 258, 261, 279, 295, 330, 475, 894, 5403, 5406, 6121, 6774) AND `id` IN (11000, 11001, 11002, 11003, 11004);
DELETE FROM `smart_scripts` WHERE  `entryorguid` IN (116, 240, 285, 327, 473, 735, 794, 79400, 79401, 881, 896, 955, 963, 6846, 42260) AND `id` IN (11000, 11001, 11002, 11003, 11004);

DELETE FROM `smart_scripts` WHERE `entryorguid` = 448; -- Remove Hogger SAI as now handled via scripting
DELETE FROM `smart_scripts` WHERE `entryorguid` = 6866;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
(6866,0,1,0,'',4,0,100,0,0,0,0,0,0,'',1,0,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Bodyguard - On Aggro - Say Line 0 (No Repeat)');

UPDATE `smart_scripts` SET `link` = '4' WHERE (`entryorguid` = '6846') and (`source_type` = '0') and (`id` = '3') and (`link` = '0');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 6846 AND `id` = 4;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `event_type`, `event_chance`, `action_type`, `action_param1`, `action_param2`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `comment`) VALUES 
('6846', '0', '4', '61', '100', '39', '20', '1', '9', '6866', '10', '20', '0', 'Defias Dockmaster - On Aggro Call for help');

-- Trainers
INSERT IGNORE INTO `trainer_spell` VALUES
('580', '125610', '1000', '0', '0', '0', '0', '0', '1', '41079'); -- Battle Pet Training for Marcus Jensen

-- Quest 62 The Fargodeep Mine should not be shown if player is holding quest 54 Report to Goldshire (which is autocompleted)
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` =19  AND `SourceEntry` = 62 AND `ConditionTypeOrReference` = 28;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES 
(19, 0, 62, 0, 0, 28, 0, 54, 0, 0, '', 1, 0, 0, '', 'The Fargodeep Mine quest not visible if Report to Goldshire is Complete');

-- Add missing areatrigger_involvedrelation for quest credit in upper Fargodeep Mine
DELETE FROM `areatrigger_involvedrelation` WHERE `id` = 197 AND `quest` = 62;
INSERT INTO `areatrigger_involvedrelation` (`id`, `quest`) VALUES 
(197, 62);

-- Stormwind Charger fix
DELETE FROM `smart_scripts` WHERE `entryorguid` = 240 AND `id` IN (1);
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `action_type`, `action_param1`, `target_type`, `comment`) VALUES 
('240', '0', '1', '62', '100', '11611', '0', '134', '78852', '7', 'Marshal Dughan - On Gossip Select - Invoker Cast Stormwind Charger');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` = 42260;

DELETE FROM `smart_scripts` WHERE `entryorguid` = 42260 AND `id` IN (1,2,3);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param7`,`action_param_string`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
(42260,0,1,0,'',11,0,100,0,1,0,12,0,0,'',53,1,1,0,0,0,0,NULL,0,1,0,0,0,0,NULL,0,0,0,0,'Stormwind Charger - On Spawn - Start WP'),
(42260,0,2,0,'',40,0,100,0,58,1,0,0,0,'',41,0,0,0,0,0,0,NULL,0,1,0,0,0,0,NULL,0,0,0,0,'Stormwind Charger - On WP Reached - Despawn'),
(42260,0,3,0,'',11,0,100,0,1,0,12,0,0,'',145,1,0,0,0,0,0,NULL,0,1,0,0,0,0,NULL,0,0,0,0,'Stormwind Charger - On Spawn - Set Immune NPC');

-- using quest status not taken condition 9 DOES NOT WORK so am using 14
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=15 AND `SourceGroup`=11611 AND `SourceEntry`=0 AND `SourceId`=0;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(15, 11611, 0, 0, 0, 14, 0, 35, 0, 0, 1, 0, 0, '', 'Show gossip menu 11611 option id 0 if quest Further Concerns has been taken.'),
(15, 11611, 0, 0, 0, 8, 0, 35, 0, 0, 1, 0, 0, '', 'Show gossip menu 11611 option id 0 if quest Further Concerns has not been rewarded.');

DELETE FROM waypoint_path WHERE `PathId` = 1;
insert into waypoint_path values (1, 1, 0, null, 'Stormwind Charger - Further Concerns');

DELETE FROM waypoint_path_node WHERE `PathId` = 1;
insert into waypoint_path_node values
( '1', '1', '-9492.8', '64.3598', '56.0275', NULL, '0' ),
( '1', '2', '-9497.88', '48.8162', '55.9479', NULL, '0' ),
( '1', '3', '-9503.12', '36.3532', '56.4226', NULL, '0' ),
( '1', '4', '-9509.77', '23.9451', '56.5319', NULL, '0' ),
( '1', '5', '-9517.83', '6.74794', '56.1109', NULL, '0' ),
( '1', '6', '-9529.74', '-15.7274', '55.8871', NULL, '0' ),
( '1', '7', '-9541.36', '-44.913', '56.7677', NULL, '0' ),
( '1', '8', '-9547.37', '-71.9513', '57.5196', NULL, '0' ),
( '1', '9', '-9549.51', '-109.446', '57.5067', NULL, '0' ),
( '1', '10', '-9556.68', '-136.553', '57.5594', NULL, '0' ),
( '1', '11', '-9567.81', '-151.018', '57.1723', NULL, '0' ),
( '1', '12', '-9578.88', '-170.288', '57.3785', NULL, '0' ),
( '1', '13', '-9586.78', '-195.139', '57.6285', NULL, '0' ),
( '1', '14', '-9598.22', '-224.65', '57.3034', NULL, '0' ),
( '1', '15', '-9608.65', '-258.085', '57.3082', NULL, '0' ),
( '1', '16', '-9614.82', '-289.097', '57.5905', NULL, '0' ),
( '1', '17', '-9618.11', '-312.991', '57.5834', NULL, '0' ),
( '1', '18', '-9620.26', '-340.564', '57.5246', NULL, '0' ),
( '1', '19', '-9619.76', '-370.955', '57.5807', NULL, '0' ),
( '1', '20', '-9618.12', '-393.672', '57.8307', NULL, '0' ),
( '1', '21', '-9610.83', '-427.736', '57.5228', NULL, '0' ),
( '1', '22', '-9602.77', '-445.216', '57.624', NULL, '0' ),
( '1', '23', '-9596.11', '-461.798', '57.7484', NULL, '0' ),
( '1', '24', '-9591.11', '-476.258', '57.7685', NULL, '0' ),
( '1', '25', '-9591.47', '-488.467', '57.84', NULL, '0' ),
( '1', '26', '-9593.78', '-501.44', '57.3818', NULL, '0' ),
( '1', '27', '-9598.51', '-512.459', '57.4743', NULL, '0' ),
( '1', '28', '-9609.81', '-533.646', '54.865', NULL, '0' ),
( '1', '29', '-9615.97', '-550.82', '54.49', NULL, '0' ),
( '1', '30', '-9618.97', '-571.015', '54.3746', NULL, '0' ),
( '1', '31', '-9620.51', '-590.842', '53.786', NULL, '0' ),
( '1', '32', '-9623.52', '-620.694', '51.9486', NULL, '0' ),
( '1', '33', '-9629', '-638.227', '51.1792', NULL, '0' ),
( '1', '34', '-9636.7', '-653.112', '49.6941', NULL, '0' ),
( '1', '35', '-9641.28', '-663.944', '49.3969', NULL, '0' ),
( '1', '36', '-9645.32', '-681.183', '48.0089', NULL, '0' ),
( '1', '37', '-9650.68', '-701.941', '45.9937', NULL, '0' ),
( '1', '38', '-9653.72', '-722.322', '44.6729', NULL, '0' ),
( '1', '39', '-9654.57', '-742.368', '44.5512', NULL, '0' ),
( '1', '40', '-9652.61', '-766.483', '44.3022', NULL, '0' ),
( '1', '41', '-9648.01', '-792.975', '43.6243', NULL, '0' ),
( '1', '42', '-9638.62', '-808.644', '43.7188', NULL, '0' ),
( '1', '43', '-9626.77', '-822.281', '43.8849', NULL, '0' ),
( '1', '44', '-9613.57', '-836.709', '43.6231', NULL, '0' ),
( '1', '45', '-9600.54', '-851.837', '43.6819', NULL, '0' ),
( '1', '46', '-9589.79', '-865.696', '43.831', NULL, '0' ),
( '1', '47', '-9585.49', '-875.554', '43.9037', NULL, '0' ),
( '1', '48', '-9584.77', '-898.893', '43.9037', NULL, '0' ),
( '1', '49', '-9586.06', '-915.418', '43.9035', NULL, '0' ),
( '1', '50', '-9592.03', '-931.484', '43.7444', NULL, '0' ),
( '1', '51', '-9598.08', '-942.441', '43.9232', NULL, '0' ),
( '1', '52', '-9607.78', '-959.011', '43.4947', NULL, '0' ),
( '1', '53', '-9615.45', '-974.169', '43.8519', NULL, '0' ),
( '1', '54', '-9620.89', '-993.472', '42.6139', NULL, '0' ),
( '1', '55', '-9620.6', '-1003.66', '42.2562', NULL, '0' ),
( '1', '56', '-9620.34', '-1014.11', '41.0678', NULL, '0' ),
( '1', '57', '-9619.95', '-1024.69', '40.4931', NULL, '0' ),
( '1', '58', '-9619.24', '-1036.47', '39.8739', NULL, '0' );