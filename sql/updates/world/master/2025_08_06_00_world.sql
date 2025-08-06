-- Quest: 29772 Raucous Rousing
-- Quest: 29774 Not In the Face!
-- Quest: 29768 Missing Mallet
-- Quest: 29775 The Spirit and Body of Shen-zin Su
-- NPC: 55539 Wugou
-- NPC: 60916 Wugou spawn
-- NPC: 55556 Shu at the Farmstead
-- NPC: 55558 Shu spawn at the Farmstead
-- Object 209626 Break Gong


DELETE FROM `creature_queststarter` WHERE `quest` IN (29772,29774,29775);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55477', '29772', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55477', '29774', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55477', '29775', '0');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29772,29774,29775);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29772', '0', '0', '0', '29768', '29774', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29774', '0', '0', '0', '29772', '29775', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29775', '0', '0', '0', '29774', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `creature_text` WHERE `creatureID` IN (55539);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('55539', '0', '0', '%s snorts loudly, and continues sleeping.', '16', '0', '100', '0', '0', '0', '54991', '0', 'Wugou to Player');

DELETE FROM `creature_template_addon` WHERE `entry` IN (55558,60916);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('55558', '8', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '105890'),
('60916', '8', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '105889 42386');


UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55539,55556,55558);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55539,55556,55558,5555800);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55477) AND `event_type` IN (38,61);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('55539', '0', '0', '0', '38', '0', '100', '0', '1', '1', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Wugou - On Data Set - Talk'),
('55477', '0', '2', '3', '38', '0', '100', '0', '2', '2', '5000', '5000', '0', '', '1', '11', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('55477', '0', '3', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '33', '55546', '0', '0', '0', '0', '0', '18', '20', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Quest Credit'),
('55556', '0', '0', '0', '1', '0', '100', '0', '0', '5000', '8000', '12000', '0', '', '11', '118032', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - Update OOC - Cast Water Spout'),
('55556', '0', '1', '2', '62', '0', '100', '0', '13140', '0', '0', '0', '0', '', '72', '0', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Gossip Select - Close Gossip'),
('55556', '0', '2', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '85', '104017', '2', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Gossip Select - Invoker Cast Summon Spirit of Water'),
('55558', '0', '0', '1', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '53', '1', '8', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - Just Spawned - Start WP'),
('55558', '0', '1', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '54', '2000', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - Just Spawned - Pause WP'),
('55558', '0', '2', '3', '58', '0', '100', '0', '5', '8', '0', '0', '0', '', '66', '0', '0', '0', '0', '0', '0', '8', '0', '0', '0', '0', '0', '0', '6.21337', 'Shu - On WP Ended - Set Orientation'),
('55558', '0', '3', '4', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '11', '118027', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - On WP Ended - Cast Shu\'s Water Splash'),
('55558', '0', '4', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '11', '118034', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - On WP Ended - Cast Water Splash'),
('55558', '0', '5', '6', '8', '0', '100', '0', '118034', '0', '0', '0', '0', '', '11', '104023', '0', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Spellhit - Cast Water Splash Credit'),
('55558', '0', '6', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5555800', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Spellhit - Run Script'),
('55558', '0', '7', '8', '31', '0', '100', '0', '118027', '0', '1000', '1000', '0', '', '28', '42386', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Spellhit Target - Remove Aura'),
('55558', '0', '8', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '64', '1', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Spellhit Target - Store Target'),
('55558', '0', '9', '10', '58', '0', '100', '0', '7', '9', '0', '0', '0', '', '11', '118035', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - On WP Ended - Cast Cast Water Spirit Laugh'),
('55558', '0', '10', '11', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '45', '1', '1', '0', '0', '0', '0', '12', '1', '0', '0', '0', '0', '0', '0', 'Shu - On WP Ended - Set Data'),
('55558', '0', '11', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '29', '0', '1', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Shu - On WP Ended - Follow Summoner'),
('55558', '0', '12', '13', '8', '0', '100', '1', '106683', '0', '0', '0', '0', '', '59', '1', '15', '1', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Spellhit - Set Run'),
('55558', '0', '13', '14', '61', '0', '100', '1', '0', '0', '0', '0', '0', '', '53', '1', '9', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Spellhit - Start WP'),
('55558', '0', '14', '0', '61', '0', '100', '1', '0', '0', '0', '0', '0', '', '28', '104017', '0', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Spellhit - Remove Aura'),
('55558', '0', '15', '0', '58', '0', '100', '0', '7', '9', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - On WP Ended - Despawn'),
('5555800', '9', '0', '0', '0', '0', '100', '0', '2000', '2000', '1000', '1000', '0', '', '11', '118035', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - Cast Water Spirit Laugh'),
('5555800', '9', '1', '0', '0', '0', '100', '0', '2000', '2000', '1000', '1000', '0', '', '53', '1', '9', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - Start WP');


UPDATE `gameobject_template` SET `Data1` = '29772' WHERE (`entry` = '209626');

-- Condition does not really work and target is not found, not sure why
DELETE FROM `conditions` WHERE (`SourceTypeOrReferenceId` = 13 AND `SourceGroup` = 8 AND `SourceEntry` = 104012);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `Comment`) VALUES 
('13', '8', '104012', '0', '0', '51', '0', '10', '209626', '0', '', '0', '0', '0', 'Break Gong Credit target Break Gong');

UPDATE `gameobject_template` SET `AIName` = 'SmartGameObjectAI' WHERE `Entry` = 209626;
UPDATE `gameobject_template` SET `Data10` = '0' WHERE (`entry` = '209626'); -- was 104012

-- Removed spell from object as i can't get it to work
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (209626);
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_chance`, `event_param1`, `action_type`, `action_param1`, `action_param2`, `target_type`, `target_param1`, `target_param2`, `comment`) VALUES 
('209626', '1', '0', '1', '70', '100', '2', '134', '118026', 0, '7', 0,0,'On Use Break Gong - Cast Gong Song'),
('209626', '1', '1', '2', '61', '100', '0', '45', '1', 1, '11', 55539, 20, 'Break Gong - Set Data Wugou'),
('209626', '1', '2', '0', '61', '100', '0', '45', '2', 2, '11', 55477, 20, 'Break Gong - Set Data Ji');

DELETE FROM `spell_target_position` WHERE `ID` IN (108827,104450,108847,108858,102499,116191,116191,108786,115426,115426,115334,115336,115338,
116190,118500,126040,104571,131974,117615,117597,118499,115435,115495,115494,115493);
INSERT INTO spell_target_position VALUES
('108827', '0', '0', '860', '601.62', '3132.89', '88.0976', NULL, '0'),
('104450', '0', '0', '860', '909.137', '3610.38', '252.092', NULL, '0'),
('108847', '0', '0', '860', '1075.6', '4177.97', '204.63', NULL, '0'),
('108858', '0', '0', '860', '1075.54', '4177.9', '205.583', NULL, '0'),
('102499', '0', '0', '860', '1351.33', '3939.03', '109.324', NULL, '0'),
('116191', '0', '0', '860', '949.37', '3510', '187.715', NULL, '0'),
('108786', '0', '0', '860', '1038.55', '3286.39', '128.176', NULL, '0'),
('115426', '0', '0', '860', '695.26', '3600.99', '142.381', NULL, '20886'),
('115334', '0', '0', '860', '571.201', '3584.59', '94.8437', NULL, '0'),
('115336', '0', '0', '860', '573.84', '3577.94', '95.0413', NULL, '0'),
('115338', '0', '0', '860', '576.42', '3579.7', '94.9249', NULL, '0'),
('116190', '0', '0', '860', '933.509', '3511.01', '187.717', NULL, '0'),
('118500', '0', '0', '860', '992.108', '3604.55', '193.032', NULL, '0'),
('126040', '0', '0', '860', '756.201', '4168.14', '202.262', NULL, '0'),
('104571', '0', '0', '860', '666.514', '4218.27', '200.851', NULL, '0'),
('131974', '0', '0', '860', '248.988', '3941.08', '65.3511', NULL, '20886'),
('117615', '1', '0', '860', '250.757', '3932.67', '66.408', NULL, '20886'),
('117597', '0', '0', '860', '230.314', '4006.68', '87.2803', NULL, '0'),
('118499', '0', '0', '860', '992.003', '3600.76', '193.031', NULL, '0'),
('115435', '0', '0', '860', '566.524', '3583.47', '92.1576', NULL, '24742'),
('115495', '0', '0', '860', '418.16', '3630.88', '92.9404', NULL, '0'),
('115494', '0', '0', '860', '424.858', '3635.56', '92.6789', NULL, '0'),
('115493', '0', '0', '860', '422.292', '3633.7', '92.8522', NULL, '0');

DELETE FROM `waypoint_path` WHERE `PathId` IN (8,9);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(8,1,0,'Spawned Shu at the Farmstead quest'),
(9,1,0,'Spawned Shu at the Farmstead after quest');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (8,9);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
('8', '0', '648.675', '3124.17', '90.2095'),
('8', '1', '639.615', '3120.11', '89.8218'),
('8', '2', '628.448', '3121.48', '88.5602'),
('8', '3', '622.969', '3125.62', '88.0585'),
('8', '4', '620.156', '3134.65', '87.837'),
('8', '5', '621.747', '3140.77', '87.837'),
('9', '0', '614.498', '3126.25', '87.8195'),
('9', '1', '624.028', '3121.75', '87.9445'),
('9', '2', '634.573', '3125.93', '88.6816'),
('9', '3', '638.785', '3133.88', '88.1648'),
('9', '4', '639.109', '3140.55', '88.3733'),
('9', '5', '636.264', '3146.2', '88.5406'),
('9', '6', '630.757', '3147.61', '88.2409'),
('9', '7', '625.675', '3143.88', '87.837');

DELETE FROM `gossip_menu` WHERE `MenuID` = 13140;
INSERT INTO `gossip_menu` (`MenuID`, `TextID`, `VerifiedBuild`) VALUES ('13140', '18503', '0');

DELETE FROM `gossip_menu_option` WHERE `MenuID` = 13140 AND `GossipOptionID` = 18503;
INSERT INTO `gossip_menu_option` (`MenuID`, `GossipOptionID`, `OptionID`, `OptionNpc`, `OptionText`, `OptionBroadcastTextID`, `Language`, `Flags`) VALUES ('13140', '18503', '0', '0', 'Shu, can you wake up Wuguo for me?', '54025', '0', '1');

