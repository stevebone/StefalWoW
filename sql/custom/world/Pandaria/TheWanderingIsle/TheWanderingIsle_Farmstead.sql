-- Quest Templates
DELETE FROM `quest_template_addon` WHERE `ID` IN (29768, 29771, 29769, 29772, 29774, 29775);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29768', '0', '0', '0', '29769', '29772', '-29768', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''), -- Quest: 29768 Missing Mallet
('29771', '0', '0', '0', '29769', '29772', '-29768', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''), -- Quest: 29771 Stronger Than Wood
('29769', '0', '0', '0', '29680', '29768', '-29769', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''), -- Quest: 29769 Rascals
('29772', '0', '0', '0', '29768', '29774', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''), -- Quest: 29772 Raucous Rousing
('29774', '0', '0', '0', '29772', '29775', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''), -- Quest: 29774 Not In the Face!
('29775', '0', '0', '104017', '29774', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''); -- Quest: 29775 The Spirit and Body of Shen-zin Su

DELETE FROM `creature_queststarter` WHERE `quest` IN (29768, 29771);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55477', '29768', '0'); -- Quest: 29768 Missing Mallet
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55478', '29771', '0'); -- Quest: 29771 Stronger Than Wood
DELETE FROM `creature_queststarter` WHERE `quest` IN (29772,29774,29775);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55477', '29772', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55477', '29774', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55477', '29775', '0');

DELETE FROM `gossip_menu` WHERE `MenuID` = 13140;
INSERT INTO `gossip_menu` (`MenuID`, `TextID`, `VerifiedBuild`) VALUES ('13140', '18503', '0');

DELETE FROM `gossip_menu_option` WHERE `MenuID` = 13140 AND `GossipOptionID` = 18503;
INSERT INTO `gossip_menu_option` (`MenuID`, `GossipOptionID`, `OptionID`, `OptionNpc`, `OptionText`, `OptionBroadcastTextID`, `Language`, `Flags`, `SpellID`) VALUES 
('13140', '18503', '0', '0', 'Shu, can you wake up Wuguo for me?', '54025', '0', '1', 104017);

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 15 AND `ConditionValue1` = 29774;
INSERT INTO `conditions` VALUES
(15, 13140, 0, 0, 0, 47, 0, 29774, 8, 0, '', 0, 0, 0, '', 'Player for which gossip text is shown has quest 29774 in progress');

DELETE FROM `creature_template_addon` WHERE `entry` IN (55539, 55213, 55556, 64939,55478,57669,55506,56241,55483, 55504,55477,55558,60916);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
(55556, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '49414'), -- Shu at farmstead
(55558, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '105890'), -- Wugou spawn
(60916, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '105889'), -- Shu spawn
(55477, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, ''),
(55483, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), 
(55504, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), 
(55506, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), 
(56241, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1904, 0, 0, 0, ''),
(55213, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '103245'),
(55539, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '80797 42386'),
('64939', '6493900', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '126986'), -- Lamplighter Sunny
('55478', '0', '0', '0', '1', '0', '1', '1', '0', '461', '0', '0', '0', '0', '60921'), -- Jojo at farmstead
('57669', '5766900', '0', '0', '0', '0', '0', '1', '0', '0', '2935', '0', '0', '0', ''); -- Jojo spawned at farmstead

DELETE FROM `creature_template_difficulty` WHERE `entry` IN (55556, 64939,55478,57669,55506,56241,55483, 55504,55477, 55213, 55539, 55558,60916);
INSERT INTO `creature_template_difficulty` VALUES
(55556, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 29560, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647), -- Shu at farmstead
(55558, 0, 0, 0, 80, 4, 1, 1, 1, 1, 29555, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60916, 0, 0, 0, 80, 4, 1, 1, 1, 1, 20539, 0, 0, 0, 0, 0, 0, 0, 0, 268435712, 0, 0, 0, 0, 0, 0, 0, 56647),
(55213, 0, 0, 0, 80, 4, 1, 1, 1, 1, 30184, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647), -- Shu follower
(55539, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 29594, 0, 0, 0, 0, 0, 0, 0, 0, 268435712, 0, 0, 0, 0, 0, 0, 0, 56647), -- Wugou at Farmstead
(55477, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 29691, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647), -- Ji at farmstead
(55483, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 29679, 0, 0, 0, 55483, 0, 0, 7, 7, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647), -- Plump Virmen
(55504, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 29636, 0, 0, 0, 55504, 0, 0, 7, 7, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647), -- Plump Carrotcruncher
(55506, 0, 0, 0, 80, 4, 3, 1, 1, 0.2, 29633, 2147483648, 0, 0, 55506, 0, 0, 7, 7, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647), -- Raggis
(56241, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28345, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647), -- Dai-Lo Farmer
(55478, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 29689, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(57669, 0, 0, 0, 80, 4, 1, 1, 1, 1, 26310, 2097160, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(64939, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 58335, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647); -- Lamplighter Sunny

-- Ji second spawn
SET @CGUID := 900000;
DELETE FROM `creature` WHERE `guid` IN (@CGUID+750);
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `PhaseGroup`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, 
`currentwaypoint`, `MovementType`, `npcflag`, `unit_flags`, `unit_flags2`, `unit_flags3`, `VerifiedBuild`) VALUES
(@CGUID+750, 55477, 860, 0, 0, 0, 169, 0, 0, 0, 629.354, 3139.15, 87.837, 0.558505, 120, 5, 0, 1, NULL, NULL, NULL, NULL, 0); -- Ji at farmstead as there are 2 spawns with different invis aura


-- Lamp Lighter needs waypoint movement
UPDATE `creature` SET `zoneId` = '5736', `areaId` = '5881', `wander_distance` = '0', `MovementType` = '2' WHERE (`guid` = '451131');

DELETE FROM `creature_addon` WHERE `guid` IN (451158,451160,451170,451171,451173,451174,451178, 451166, @CGUID+750);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(451166, '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '49415'),
(@CGUID+750, '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '85096'),
-- Jojo quest crowd
('451158', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'), -- farmstead jojo power quest
('451160', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451170', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451171', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451173', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451174', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451178', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (57669);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (57669,5766900) AND `source_type` IN (0,9);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('57669', '0', '0', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5766900', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Just Spawned - Run Script'),
('57669', '0', '1', '0', '58', '0', '100', '0', '50', '6', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - On WP Ended - Despawn'),
('5766900', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Talk'),
('5766900', '9', '1', '0', '0', '0', '100', '0', '7000', '7000', '1000', '1000', '0', '', '128', '1', '1', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Set AI AnimKit'),
('5766900', '9', '2', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '69', '1', '0', '0', '0', '0', '0', '8', '0', '0', '0', '599.215', '3132.27', '89.0657', '0', 'Jojo Ironbrow - Move to Pos'),
('5766900', '9', '3', '0', '0', '0', '100', '0', '5000', '5000', '1000', '1000', '0', '', '11', '129293', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Cast Jojo Headbash, Planks Cast'),
('5766900', '9', '4', '0', '0', '0', '100', '0', '2500', '2500', '1000', '1000', '0', '', '1', '1', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Talk'),
('5766900', '9', '5', '0', '0', '0', '100', '0', '100', '100', '0', '0', '0', '', '28', '108831', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Remove Auras'),
('5766900', '9', '6', '0', '0', '0', '100', '0', '100', '100', '0', '0', '0', '', '28', '108831', '0', '0', '0', '0', '0', '18', '50', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Remove Player Auras'),
('5766900', '9', '7', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '45', '1', '1', '0', '0', '0', '0', '11', '0', '10', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Set Data'),
('5766900', '9', '8', '0', '0', '0', '100', '0', '8000', '8000', '1000', '1000', '0', '', '53', '0', '5766900', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Start WP');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55506);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55506) AND `source_type` = 0;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('55506', '0', '0', '0', '0', '0', '100', '0', '8000', '12000', '35000', '40000', '0', '', '11', '125383', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Raggis - Update IC - Cast Burrow');

-- Quest: 29769 Rascals
-- Quest: 29770 Still Good!
DELETE FROM `creature_queststarter` WHERE `quest` IN (29769,29770);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55477', '29769', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55479', '29770', '0');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55477,55483,55504);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55477,5547700,55483,55504) AND `source_type` IN (0,9);
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

-- Condition for Ji at the farmstead SAY lines
DELETE FROM `conditions`
WHERE SourceTypeOrReferenceId = 22
  AND SourceGroup = 5
  AND SourceEntry = 55477
  AND ConditionTypeOrReference = 9
  AND ConditionValue1 = 29774;
INSERT INTO `conditions` (SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ElseGroup, ConditionTypeOrReference, ConditionTarget, ConditionValue1, ConditionValue2, ConditionValue3, NegativeCondition, COMMENT
) VALUES
(22, 5, 55477, 0, 0, 9, 0, 29774, 0, 0, 1, 'Ji: run SAI only if quest 29774 is NOT taken');

UPDATE `creature_template` SET `ScriptName` = 'npc_shu_at_farmstead_pool', `AIName` = '' WHERE `Entry` = 55556;
UPDATE `creature_template` SET `ScriptName` = 'npc_shu_at_farmstead_play', `AIName` = '' WHERE `Entry` = 55558;

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55539,55556,55558);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55539,55556,55558,5555800) AND `source_type` IN (0,9);
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

-- Water spout bunny
UPDATE `creature_template` SET `AIName` = 'SmartAI', `unit_flags` = '33554944', `unit_flags2` = '2048' WHERE (`entry` = '66941');

DELETE FROM `creature_template_addon` WHERE `entry` IN (66941);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('66941', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80797 116695'); -- Water Spout Bunny at farmstead -- not sure of the invis aura ????? 

-- these flags disable gravity for the bunny so it can stay above water
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '66941') and (`DifficultyID` = '0');

DELETE FROM `smart_scripts` WHERE `entryorguid` IN (66941,6694100); -- 15880 was the original model but not working
INSERT INTO `smart_scripts` VALUES -- we use model 21072 now taken from wowhead
('66941', '0', '0', '1', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '3', '0', '21072', '0', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '', '0', '0', '0', '0', 'Water Spout - Just Spawned - Morph'),
('66941', '0', '1', '0', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '6694100', '2', '0', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '', '0', '0', '0', '0', 'Water Spout - Just Spawned - Run Script'),
('6694100', '9', '0', '0', '', '0', '0', '100', '0', '5000', '5000', '1000', '1000', '0', '', '11', '117057', '2', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Cast Water Spout Geyser Aura'),
('6694100', '9', '1', '0', '', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '11', '116696', '2', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Cast Water Spout Burst'),
('6694100', '9', '2', '0', '', '0', '0', '100', '0', '0', '0', '3000', '3000', '0', '', '28', '116695', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Remove Aura'),
('6694100', '9', '3', '0', '', '0', '0', '100', '0', '3000', '3000', '1000', '1000', '0', '', '41', '0', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Despawn');


UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (56241);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (56241) AND `source_type` IN (0,9);
INSERT INTO `smart_scripts` (entryorguid,    source_type,    id,    link,    event_type,    event_phase_mask,    event_chance,    event_flags,    event_param1,    event_param2,
    event_param3,    event_param4,    event_param5,    event_param_string,    action_type,    action_param1,    action_param2,    action_param3,
    action_param4,    action_param5,    action_param6,    target_type,    target_param1,    target_param2,    target_param3,    target_x,    target_y,
    target_z,    target_o,    comment) VALUES
(56241, 0, 0, 0, 0, 0, 100, 0, 2000, 5000, 5000, 8000, 0, '', 11, 128435, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Dai-Lo Farmer - Update IC - Cast Palm Strike'),
(56241, 0, 1, 0, 0, 0, 100, 0, 5000, 8000, 8000, 12000, 0, '', 11, 128436, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Dai-Lo Farmer - Update IC - Cast Round Kick');

DELETE FROM `waypoint_path` WHERE `PathId` IN (5521300);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(5521300,1,0,'The Wandering Isle - Spawned Shu quest 29680');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (5521300);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
('5521300', '0', '634.3', '3126.2', '88.7482'),
('5521300', '1', '642.205', '3124.58', '89.3339'),
('5521300', '2', '649.877', '3126.44', '89.808'),
('5521300', '3', '655.521', '3131.44', '89.3411'),
('5521300', '4', '660.047', '3135.83', '88.807'),
('5521300', '5', '665.788', '3136.77', '87.8995');

DELETE FROM `waypoint_path` WHERE `PathId` IN (5555800, 5555801,5555802);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(5555802,1,0,'The Wandering Isle - Spawned Shu at the Farmstead after Wugou wakes up'),
(5555800,1,0,'The Wandering Isle - Spawned Shu at the Farmstead quest'),
(5555801,1,0,'The Wandering Isle - Spawned Shu at the Farmstead after quest');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (5555800, 5555801, 5555802);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
('5555802', '0', '815.315', '3608.55', '173.987'),
('5555802', '1', '831.068', '3607.74', '175.247'),
('5555802', '2', '841.146', '3606.77', '175.188'),
('5555802', '3', '855.831', '3607.75', '173.619'),
('5555802', '4', '880.269', '3605.99', '192.267'),
('5555802', '5', '892.776', '3605.94', '192.93'),
('5555802', '6', '900.755', '3603.12', '193.08'),
('5555802', '7', '906.239', '3600.17', '193.1');
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
('5555800', '0', '648.675', '3124.17', '90.2095'),
('5555800', '1', '639.615', '3120.11', '89.8218'),
('5555800', '2', '628.448', '3121.48', '88.5602'),
('5555800', '3', '622.969', '3125.62', '88.0585'),
('5555800', '4', '620.156', '3134.65', '87.837'),
('5555800', '5', '621.747', '3140.77', '87.837');
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
('5555801', '0', '614.498', '3126.25', '87.8195'),
('5555801', '1', '624.028', '3121.75', '87.9445'),
('5555801', '2', '634.573', '3125.93', '88.6816'),
('5555801', '3', '638.785', '3133.88', '88.1648'),
('5555801', '4', '639.109', '3140.55', '88.3733'),
('5555801', '5', '636.264', '3146.2', '88.5406'),
('5555801', '6', '630.757', '3147.61', '88.2409'),
('5555801', '7', '625.675', '3143.88', '87.837');

UPDATE `waypoint_path_node` SET `Orientation` = '6.21337' WHERE (`PathId` = 5555800 AND `NodeId` = 5);

-- Loot tables
-- Delete only reference loot items (ItemType = 1) for specific NPCs
DELETE FROM creature_loot_template 
WHERE Entry IN (55504,55483,55506) 
AND ItemType = 1;

INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(55504, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(55504, 1, 4, 100, 0, 1, 1, 1, 'Humanoids'),
(55483, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(55483, 1, 4, 100, 0, 1, 1, 1, 'Humanoids'),
(55506, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(55506, 1, 4, 100, 0, 1, 1, 1, 'Humanoids');

DELETE FROM `npc_vendor` WHERE `entry` IN (57618);
INSERT INTO `npc_vendor` (entry, slot, item, maxcount, incrtime, type, VerifiedBuild) VALUES
(57618, 1, 2880, 0, 0, 1, 63906),
(57618, 2, 2901, 0, 0, 1, 63906),
(57618, 3, 3466, 0, 0, 1, 63906),
(57618, 4, 3857, 0, 0, 1, 63906),
(57618, 5, 5956, 0, 0, 1, 63906),
(57618, 6, 18567, 0, 0, 1, 63906),
(57618, 7, 160298, 0, 0, 1, 63906),
(57618, 8, 180733, 0, 0, 1, 63906);
