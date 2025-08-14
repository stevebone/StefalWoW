-- Quest: 29787 Worthy Of Passing
-- NPC: 55586 Master Shang Xi
-- NPC: 56159 Master Shang (spawned for quest 29787)
-- NPC: 56274 Guardian of the Elders

DELETE FROM `spell_script_names` WHERE `spell_id` = 105333;
INSERT INTO `spell_script_names` VALUES (105333, 'spell_summon_worthy_of_passing');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29787);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29787', '0', '0', '0', '29786', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `creature_queststarter` WHERE `quest` IN (29787);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55586', '29787', '0');

DELETE FROM `creature_text` WHERE `creatureID` IN (56159,55672);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('56159', '0', '0', 'Come child. We have one final journey to take together before your training is complete.', '12', '0', '100', '1', '0', '27797', '55520', '0', 'Master Shang Xi to Player'),
('56159', '1', '0', 'Beyond the Elders\' Path lies the Wood of Staves, a sacred place that only the worthy may enter.', '12', '0', '100', '1', '0', '27800', '55521', '0', 'Master Shang Xi to Player'),
('56159', '2', '0', 'Of the many ways to prove your worth, I require the simplest of you now. I must know that you will fight for our people. I must know that you can keep them safe.', '12', '0', '100', '1', '0', '27801', '55522', '0', 'Master Shang Xi to Player'),
('56159', '3', '0', 'Defeat the Guardian of the Elders, and we may pass.', '12', '0', '100', '1', '0', '27802', '55523', '0', 'Master Shang Xi to Player'),
('56159', '4', '0', 'You\'ve become strong indeed, child. This is good. You will need that strength soon.', '12', '0', '100', '1', '0', '27803', '56484', '0', 'Master Shang Xi to Player'),
('55672', '0', '0', 'And here we are. Help me with a couple small tasks while I prepare, if you would.', '12', '0', '100', '1', '0', '27805', '56485', '0', 'Master Shang Xi to Player');


UPDATE `creature_template_difficulty` SET `ContentTuningID` = '80' WHERE (`Entry` = '56274') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '80' WHERE (`Entry` = '55466') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '80' WHERE (`Entry` = '55672') and (`DifficultyID` = '0');



UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55586,56159,56274,55672);
UPDATE `gameobject_template` SET `AIName` = 'SmartGameObjectAI' WHERE `Entry` IN (209922);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55586,56159,5615900,56274,209922,55672);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('55586', '0', '0', '0', '19', '0', '100', '0', '29787', '0', '0', '0', '0', '', '85', '105333', '2', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Master Shang Xi - On Accepted Quest - Invoker Cast Summon Master Shang'),
('56159', '0', '0', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5615900', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Just Spawned - Run Script'),
('56159', '0', '1', '2', '40', '0', '100', '0', '7', '20', '0', '0', '0', '', '59', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On WP Reached - Set Walk'),
('56159', '0', '2', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '1', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On WP Reached - Talk'),
('56159', '0', '3', '0', '40', '0', '100', '0', '8', '20', '0', '0', '0', '', '1', '2', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On WP Reached - Talk'),
('56159', '0', '4', '0', '40', '0', '100', '0', '9', '20', '0', '0', '0', '', '1', '3', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On WP Reached - Talk'),
('56159', '0', '5', '0', '58', '0', '100', '0', '12', '20', '0', '0', '0', '', '66', '0', '0', '0', '0', '0', '0', '8', '0', '0', '0', '0', '0', '0', '5.48033', 'Master Shang - On WP Ended - Set Orientation'),
('56159', '0', '6', '7', '38', '0', '100', '0', '1', '1', '0', '0', '0', '', '55', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On Data Set - Stop WP'),
('56159', '0', '7', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '1', '4', '2000', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On Data Set - Talk'),
('56159', '0', '8', '0', '52', '0', '100', '0', '4', '56159', '0', '0', '0', '', '53', '1', '21', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On Text Over - Start WP'),
('56159', '0', '9', '10', '58', '0', '100', '0', '6', '21', '0', '0', '0', '', '11', '109144', '2', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On WP Ended - Cast Force Trigger Master Shang Xi Final Escort Say'),
('56159', '0', '10', '11', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '66', '0', '0', '0', '0', '0', '0', '8', '0', '0', '0', '0', '0', '0', '4.53786', 'Master Shang - On WP Ended - Set Orientation'),
('56159', '0', '11', '12', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '44', '1527', '1', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On WP Ended - Add Phase'),
('56159', '0', '12', '13', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '28', '105333', '0', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On WP Ended - Remove Aura'),
('56159', '0', '13', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On WP Ended - Despawn'),
('5615900', '9', '0', '0', '0', '0', '100', '0', '2000', '2000', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Talk'),
('5615900', '9', '1', '0', '0', '0', '100', '0', '3000', '3000', '1000', '1000', '0', '', '53', '1', '20', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - Start WP'),
('56274', '0', '0', '0', '0', '0', '100', '0', '8000', '12000', '8000', '12000', '0', '', '11', '125218', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Guardian of the Elders - Update IC - Cast Pounce'),
('56274', '0', '1', '2', '6', '0', '100', '0', '0', '0', '0', '0', '0', '', '45', '1', '1', '0', '0', '0', '0', '9', '56159', '0', '50', '0', '0', '0', '0', 'Guardian of the Elders - On Death - Set Data'),
('56274', '0', '2', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '45', '1', '1', '0', '0', '0', '0', '15', '209922', '0', '50', '0', '0', '0', '0', 'Guardian of the Elders - On Death - Set Data'),
('209922', '1', '0', '0', '38', '0', '100', '0', '1', '1', '0', '0', '0', '', '41', '1000', '20000', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Spirit Wall - On Data Set - Despawn'),
('55672', '0', '0', '0', '19', '0', '100', '0', '29790', '0', '0', '0', '0', '', '85', '106623', '2', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On Accepted Quest - Invoker Cast Summon Master Shang Xi'),
('55672', '0', '1', '0', '20', '0', '100', '0', '29787', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Master Shang - On Reward Quest - Master Shang Xi Talk');



DELETE FROM `waypoint_path` WHERE `PathId` IN (20,21);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(20,1,0,'Master Shang spawn - Quest accept 29787'),
(21,0,0,'Master Shang - Quest 29787');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (20,21);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
('20', '0', '776.526', '4178.65', '207.064'),
('20', '1', '792.852', '4185.14', '208.447'),
('20', '2', '808.821', '4193.28', '207.043'),
('20', '3', '827.309', '4205.26', '199.727'),
('20', '4', '839.753', '4215.79', '197.826'),
('20', '5', '840.438', '4230.83', '198.077'),
('20', '6', '842.536', '4245.48', '196.849'),
('20', '7', '845.007', '4267.59', '196.825'),
('20', '8', '843.82', '4301.17', '210.984'),
('20', '9', '843.262', '4339.1', '223.981'),
('20', '10', '828.653', '4353.31', '224.09'),
('20', '11', '828.582', '4364.45', '223.973'),
('20', '12', '830.259', '4368.55', '223.988'),
('21', '0', '846.183', '4382.34', '224.339'),
('21', '1', '845.933', '4391.09', '231.589'),
('21', '2', '844.683', '4402.34', '237.339'),
('21', '3', '853.433', '4415.59', '237.589'),
('21', '4', '867.433', '4436.09', '236.839'),
('21', '5', '873.683', '4448.59', '239.839'),
('21', '6', '874.108', '4459.63', '241.189');

