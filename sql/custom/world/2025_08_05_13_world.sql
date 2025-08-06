-- Quest: 29768 Missing Mallet
-- Quest: 29771 Stronger Than Wood
-- NPC: 55477 Ji Firepaw at the farm
-- NPC: 55021 Jojo Ironbrow at first house
-- NPC: 57638 Jojo Ironbrow (spell spawn first house)
-- NPC: 55478 Jojo Ironbrow at farmstead
-- NPC: 57669 Jojo Ironbrow (spell spawn farmstead)
-- NPC: 64939 Lamplighter Sunny
-- NPC: 55506 Raggis
-- NPC: 65467 Mesmerized Onlooker
-- NPC: 56394 Mesmerized Child
-- NPC: 56393 Mesmerized Onlooker

DELETE FROM `creature_queststarter` WHERE `quest` IN (29768,29771);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55477', '29768', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55478', '29771', '0');

DELETE FROM `creature_text` WHERE `creatureID` IN (57669,57638);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('57669', '0', '0', 'Our crafters build the finest wooden planks.  They would weather the most brutal of storms.  But they are as water before the storm of my crushing skull.', '12', '0', '100', '1', '0', '0', '56343', '0', 'Jojo Ironbrow to Player'),
('57669', '1', '0', 'Nothing made by pandaren hands can withstand me.', '12', '0', '100', '0', '0', '0', '56344', '0', 'Jojo Ironbrow to Player'),
('57638', '0', '0', 'The reeds of the Singing Pools are the hardest in all of the land, but they are as air before my mighty brow.', '12', '0', '100', '1', '0', '0', '56333', '0', 'Jojo Ironbrow to Player'),
('57638', '1', '0', 'Many have tested my claim, and I yet stand proven.', '12', '0', '100', '2', '0', '0', '56334', '0', 'Jojo Ironbrow to Player');


DELETE FROM `quest_template_addon` WHERE `ID` IN (29768,29771);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29768', '0', '0', '0', '29769', '29772', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29771', '0', '0', '0', '29769', '29772', '-29768', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `creature_template_addon` WHERE `entry` IN (64939,55478,57669);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('64939', '5', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '126986'),
('55478', '0', '0', '0', '1', '0', '1', '1', '0', '461', '0', '0', '0', '0', '60921'),
('57669', '6', '0', '0', '0', '0', '0', '1', '0', '0', '2935', '0', '0', '0', '');


-- Lamp Lighter needs waypoint movement
UPDATE `creature` SET `zoneId` = '5736', `areaId` = '5881', `wander_distance` = '0', `MovementType` = '2' WHERE (`guid` = '451131');

-- Visibility addons need to be on spawn level
DELETE FROM `creature_template_addon` WHERE (`entry` = '56394');
DELETE FROM `creature_template_addon` WHERE (`entry` = '65467');

DELETE FROM `creature_addon` WHERE `guid` IN (450741,450742,450744,450747,450752,450759,450760,
451158,451160,451170,451171,451173,451174,451178,
451461,451457,451466,451456,451467,451463,451458);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('450741', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '78718'), -- first jojo power quest
('450742', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '78718'),
('450744', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '78718'),
('450747', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '78718'),
('450752', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '78718'),
('450759', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '78718'),
('450760', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '78718'),
('451158', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'), -- farmstead jojo power quest
('451160', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451170', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451171', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451173', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451174', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451178', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451461', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80852'), -- morning breeze jojo power quest
('451457', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80852'),
('451466', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80852'),
('451456', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80852'),
('451467', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80852'),
('451463', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80852'),
('451458', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80852');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55506,57669,57638);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55506,57669,5766900,57638,5763800);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('55506', '0', '0', '0', '0', '0', '100', '0', '8000', '12000', '35000', '40000', '0', '', '11', '125383', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Raggis - Update IC - Cast Burrow'),
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
('5766900', '9', '8', '0', '0', '0', '100', '0', '8000', '8000', '1000', '1000', '0', '', '53', '0', '6', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Start WP'),
('57638', '0', '0', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5763800', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Just Spawned - Run Script'),
('57638', '0', '1', '0', '58', '0', '100', '0', '39', '7', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - WP Ended - Despawn'),
('5763800', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Talk'),
('5763800', '9', '1', '0', '0', '0', '100', '0', '2000', '2000', '1000', '1000', '0', '', '128', '1', '1', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Set AI AnimKit'),
('5763800', '9', '2', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '69', '1', '0', '0', '0', '0', '0', '8', '0', '0', '0', '1039.19', '3284.26', '129.397', '0', 'Jojo Ironbrow - Move to Pos'),
('5763800', '9', '3', '0', '0', '0', '100', '0', '3200', '3200', '1000', '1000', '0', '', '11', '129272', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Cast Jojo Headbash, Reeds Cast'),
('5763800', '9', '4', '0', '0', '0', '100', '0', '2500', '2500', '1000', '1000', '0', '', '1', '1', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Talk'),
('5763800', '9', '5', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '45', '1', '1', '0', '0', '0', '0', '11', '0', '10', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Set Data'),
('5763800', '9', '6', '0', '0', '0', '100', '0', '8000', '8000', '1000', '1000', '0', '', '53', '0', '7', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Start WP');

DELETE FROM `waypoint_path` WHERE `PathId` IN (5,6);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(5,0,0,'Lamplighter Sunny'),
(6,0,0,'Jojo Ironbrow (spell spawn farmstead)'),
(7,0,0,'Jojo Ironbrow (spell spawn first house)');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (5,6);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ, Orientation,Delay) values
('5', '0', '612.982', '3096.65', '86.6167', '0', '0'),
('5', '1', '612.482', '3088.15', '85.3667', '0', '0'),
('5', '2', '610.982', '3079.9', '84.1167', '0', '0'),
('5', '3', '611.232', '3067.15', '81.6167', '0', '0'),
('5', '4', '611.482', '3050.65', '78.1167', '0', '0'),
('5', '5', '612.732', '3041.15', '76.8667', '0', '0'),
('5', '6', '619.232', '3025.4', '75.8667', '0', '0'),
('5', '7', '624.71', '3015.89', '75.1487', '0', '7000'),
('5', '8', '622.442', '3021.02', '75.5442', '0', '0'),
('5', '9', '617.692', '3028.02', '76.0442', '0', '0'),
('5', '10', '613.942', '3037.02', '76.7942', '0', '0'),
('5', '11', '611.692', '3045.27', '77.5442', '0', '0'),
('5', '12', '611.442', '3053.52', '79.0442', '0', '0'),
('5', '13', '611.692', '3062.27', '80.5442', '0', '0'),
('5', '14', '611.942', '3071.02', '82.2942', '0', '0'),
('5', '15', '614.692', '3080.27', '84.0442', '0', '0'),
('5', '16', '620.674', '3085.16', '85.9397', '0', '7000'),
('5', '17', '619.254', '3091.48', '86.6491', '0', '0'),
('5', '18', '618.004', '3101.98', '87.1491', '0', '0'),
('5', '19', '616.504', '3112.48', '87.6491', '0', '0'),
('5', '20', '615.004', '3123.23', '87.8991', '0', '0'),
('5', '21', '617.004', '3137.73', '87.8991', '0', '0'),
('5', '22', '618.335', '3147.79', '87.8586', '0', '7000'),
('5', '23', '611.154', '3148.1', '88.101', '0', '0'),
('5', '24', '601.904', '3149.35', '87.851', '0', '0'),
('5', '25', '597.154', '3148.85', '87.851', '0', '0'),
('5', '26', '592.472', '3143.41', '87.8434', '0', '7000'),
('5', '27', '587.675', '3146.72', '88.0353', '0', '0'),
('5', '28', '580.925', '3146.47', '87.7853', '0', '0'),
('5', '29', '572.175', '3147.72', '86.5353', '0', '0'),
('5', '30', '564.175', '3150.47', '83.0353', '0', '0'),
('5', '31', '555.925', '3155.72', '78.7853', '0', '0'),
('5', '32', '549.175', '3161.97', '77.5353', '0', '0'),
('5', '33', '545.675', '3169.72', '77.2853', '0', '0'),
('5', '34', '544.425', '3177.97', '77.2853', '0', '0'),
('5', '35', '543.175', '3186.72', '77.2853', '0', '0'),
('5', '36', '542.425', '3194.22', '77.5353', '0', '0'),
('5', '37', '539.675', '3202.47', '77.0353', '0', '0'),
('5', '38', '535.878', '3207.53', '76.2272', '0', '7000'),
('5', '39', '522.16', '3220.36', '74.4621', '0', '0'),
('5', '40', '514.915', '3231.21', '74.093', '0', '0'),
('5', '41', '512.849', '3240.09', '75.0834', '0', '0'),
('5', '42', '511.016', '3248.97', '76.8458', '0', '0'),
('5', '43', '511.754', '3261.91', '77.5846', '2.98451', '12000'),
('5', '44', '508.482', '3259.15', '77.6167', '0', '0'),
('5', '45', '505.232', '3251.15', '77.3667', '0', '0'),
('5', '46', '504.982', '3244.65', '76.3667', '0', '0'),
('5', '47', '507.232', '3236.65', '75.1167', '0', '0'),
('5', '48', '511.232', '3228.65', '74.3667', '0', '0'),
('5', '49', '518.482', '3220.4', '74.6167', '0', '0'),
('5', '50', '528.982', '3213.65', '75.8667', '0', '0'),
('5', '51', '536.232', '3209.4', '76.3667', '0', '0'),
('5', '52', '540.482', '3201.4', '76.8667', '0', '0'),
('5', '53', '543.732', '3190.15', '77.1167', '0', '0'),
('5', '54', '544.732', '3177.4', '77.1167', '0', '0'),
('5', '55', '547.982', '3165.4', '77.1167', '0', '0'),
('5', '56', '554.232', '3157.15', '77.8667', '0', '0'),
('5', '57', '561.482', '3152.65', '81.1167', '0', '0'),
('5', '58', '567.232', '3149.4', '84.3667', '0', '0'),
('5', '59', '574.732', '3147.15', '87.3667', '0', '0'),
('5', '60', '583.982', '3145.65', '87.8667', '0', '0'),
('5', '61', '593.232', '3144.4', '87.8667', '0', '0'),
('5', '62', '599.982', '3142.9', '88.1167', '0', '0'),
('5', '63', '606.482', '3141.4', '87.8667', '0', '0'),
('5', '64', '615.232', '3137.4', '87.8667', '0', '0'),
('5', '65', '619.982', '3130.4', '87.8667', '0', '0'),
('5', '66', '621.482', '3123.4', '87.8667', '0', '0'),
('5', '67', '619.732', '3112.4', '87.8667', '0', '0'),
('5', '68', '616.982', '3104.65', '87.3667', '0', '0'),
('6', '0', '599.929', '3140.35', '88.2049', '0', '0'),
('6', '1', '594.019', '3144.98', '87.8352', '0', '0'),
('6', '2', '585.708', '3147.41', '87.8352', '0', '0'),
('6', '3', '578.502', '3150.31', '87.6758', '0', '0'),
('6', '4', '568.241', '3155.38', '84.0477', '0', '0'),
('6', '5', '556.214', '3163.61', '77.2933', '0', '0'),
('6', '6', '550.847', '3172.42', '77.0957', '0', '0'),
('6', '7', '549.276', '3180.51', '77.0957', '0', '0'),
('6', '8', '548.031', '3191.97', '77.0957', '0', '0'),
('6', '9', '546.122', '3202.98', '76.5399', '0', '0'),
('6', '10', '537.189', '3213.91', '75.7559', '0', '0'),
('6', '11', '527.2', '3219.97', '74.6265', '0', '0'),
('6', '12', '516.071', '3230.63', '74.093', '0', '0'),
('6', '13', '511.005', '3245.36', '76.2209', '0', '0'),
('6', '14', '510.042', '3258.1', '77.5846', '0', '0'),
('6', '15', '511.365', '3274.88', '76.3656', '0', '0'),
('6', '16', '515.174', '3293.1', '74.0391', '0', '0'),
('6', '17', '517.49', '3302.16', '73.4592', '0', '0'),
('6', '18', '522.252', '3313.48', '73.2886', '0', '0'),
('6', '19', '524.229', '3319.17', '74.429', '0', '0'),
('6', '20', '527.083', '3323.94', '77.9471', '0', '0'),
('6', '21', '529.615', '3328.26', '77.2828', '0', '0'),
('6', '22', '532.233', '3332.62', '73.9326', '0', '0'),
('6', '23', '536.986', '3339.77', '74.2679', '0', '0'),
('6', '24', '541.793', '3346.9', '75.5099', '0', '0'),
('6', '25', '545.04', '3354.28', '76.3512', '0', '0'),
('6', '26', '551.722', '3360.92', '77.6802', '0', '0'),
('6', '27', '562.785', '3369.31', '79.3041', '0', '0'),
('6', '28', '573.752', '3374.6', '81.1686', '0', '0'),
('6', '29', '584.516', '3376.84', '83.0058', '0', '0'),
('6', '30', '602.658', '3380.54', '86.0039', '0', '0'),
('6', '31', '616.135', '3383.62', '88.2383', '0', '0'),
('6', '32', '629.408', '3388.19', '91.4238', '0', '0'),
('6', '33', '641.904', '3394.52', '94.8907', '0', '0'),
('6', '34', '656.509', '3405.72', '99.4795', '0', '0'),
('6', '35', '669.83', '3419.28', '104.06', '0', '0'),
('6', '36', '680.092', '3431.23', '108.153', '0', '0'),
('6', '37', '685.416', '3441.67', '111.014', '0', '0'),
('6', '38', '689.628', '3453.92', '115.057', '0', '0'),
('6', '39', '694.601', '3465.64', '117.579', '0', '0'),
('6', '40', '705.93', '3473.97', '119.103', '0', '0'),
('6', '41', '719.997', '3481.66', '123.177', '0', '0'),
('6', '42', '730.43', '3485.82', '128.295', '0', '0'),
('6', '43', '740.32', '3491.29', '132.969', '0', '0'),
('6', '44', '747.703', '3497.73', '136.24', '0', '0'),
('6', '45', '754.712', '3506.09', '137.961', '0', '0'),
('6', '46', '762.233', '3509.7', '140.319', '0', '0'),
('6', '47', '770.802', '3509.02', '140.395', '0', '0'),
('6', '48', '781.087', '3506.4', '141.267', '0', '0'),
('6', '49', '783.833', '3502.92', '141.293', '0', '0'),
('6', '50', '782.533', '3499.91', '141.293', '0', '0'),
('7', '0', '1027.38', '3287.42', '126.294', '0', '0'),
('7', '1', '1017.19', '3284.9', '122.151', '0', '0'),
('7', '2', '1008.78', '3281.77', '126.096', '0', '0'),
('7', '3', '1002.94', '3280.24', '129.183', '0', '0'),
('7', '4', '995.347', '3277.77', '131.925', '0', '0'),
('7', '5', '987.771', '3274.97', '132.621', '0', '0'),
('7', '6', '980.602', '3271.6', '132.311', '0', '0'),
('7', '7', '972.566', '3266.96', '129.922', '0', '0'),
('7', '8', '965.878', '3262.29', '126.923', '0', '0'),
('7', '9', '957.291', '3258.33', '124.272', '0', '0'),
('7', '10', '954.688', '3250.52', '123.022', '0', '0'),
('7', '11', '956.017', '3239.25', '119.817', '0', '0'),
('7', '12', '958.292', '3227.13', '119.183', '0', '0'),
('7', '13', '961.021', '3215.98', '118.817', '0', '0'),
('7', '14', '965.293', '3203.27', '119.105', '0', '0'),
('7', '15', '968.996', '3193.56', '119.713', '0', '0'),
('7', '16', '965.625', '3186.46', '117.808', '0', '0'),
('7', '17', '960.597', '3180.62', '117.558', '0', '0'),
('7', '18', '952.604', '3169.27', '115.568', '0', '0'),
('7', '19', '945.833', '3161.46', '115.308', '0', '0'),
('7', '20', '938.726', '3150.68', '116.118', '0', '0'),
('7', '21', '930.743', '3137.84', '113.399', '0', '0'),
('7', '22', '925.177', '3127.7', '110.467', '0', '0'),
('7', '23', '921.023', '3119.7', '110.343', '0', '0'),
('7', '24', '917.917', '3111.25', '110.045', '0', '0'),
('7', '25', '916.821', '3100.02', '108.697', '0', '0'),
('7', '26', '918', '3090.03', '105.801', '0', '0'),
('7', '27', '919.632', '3082.33', '102.748', '0', '0'),
('7', '28', '922.191', '3071.39', '102.677', '0', '0'),
('7', '29', '925.359', '3059.75', '102.743', '0', '0'),
('7', '30', '928.519', '3048.86', '102.779', '0', '0'),
('7', '31', '931.036', '3042.57', '102.727', '0', '0'),
('7', '32', '934.375', '3034.38', '102.855', '0', '0'),
('7', '33', '937.5', '3030.21', '103.73', '0', '0'),
('7', '34', '941.05', '3025.7', '105.176', '0', '0'),
('7', '35', '944.791', '3022.92', '105.73', '0', '0'),
('7', '36', '958.542', '3019.45', '105.591', '0', '0'),
('7', '37', '965.668', '3015.71', '105.698', '0', '0'),
('7', '38', '968.58', '3009.25', '105.693', '0', '0'),
('7', '39', '966.87', '3002.41', '105.04', '0', '0');


UPDATE `creature_template_difficulty` SET `LootID` = '55506', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '55506') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '55506', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '55506') and (`DifficultyID` = '1');

DELETE FROM `creature_loot_template` WHERE `entry` IN (55506);
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,118,2.15699,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,159,2.49055,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,766,0.422504,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,767,0.244607,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,768,0.222371,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,774,2.22371,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,805,0.177896,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,858,0.022237,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1411,0.355793,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1412,0.400267,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1413,0.37803,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1414,0.555926,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1415,0.845008,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1416,0.422504,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1417,0.311319,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1418,0.333556,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1419,0.778297,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1420,0.400267,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1422,0.489215,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1423,0.444741,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1425,0.355793,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1427,0.289082,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1429,0.289082,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1430,0.555926,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1431,0.37803,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,1433,0.266845,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,2138,0.177896,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,2212,0.422504,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,2213,0.333556,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,2589,77.3627,0,1,0,1,2,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,2635,0.422504,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,2642,0.200133,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,2643,0.355793,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,2645,0.444741,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,2646,0.555926,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,2648,0.311319,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,2773,0.489215,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,2774,0.75606,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,2962,0.022237,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,2965,0.222371,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,3189,0.133422,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,3190,0.133422,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,3214,0.489215,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,3370,0.244607,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,3373,0.466978,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,3609,0.222371,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,4496,0.0667111,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,4536,5.38136,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,4537,0.022237,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,4560,0.155659,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,4562,0.022237,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,4565,0.37803,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,5572,0.133422,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,6515,0.222371,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,8178,0.111185,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,8179,0.177896,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,8181,0.75606,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,8182,0.489215,0,1,0,1,1,'');
INSERT INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questRequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55506,0,9759,0.222371,0,1,0,1,1,'');
