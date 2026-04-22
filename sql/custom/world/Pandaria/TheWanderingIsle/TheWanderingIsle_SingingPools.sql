
DELETE FROM `quest_template_addon` WHERE `ID` IN (29521, 29661, 29662, 29663, 29676, 29666, 29677, 29678, 29679, 29680);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29521', '0', '0', '0', '0', '29661', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''), 		-- Quest: 29521 The Singing Pools
(29662, 0, 0, 0, 0, 29676, -29661521, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'SmartQuest'), 						-- Quest: 29662 Stronger Than Reeds
(29661, 0, 0, 0, 29521, 29676, -29661521, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), 							-- Quest: 29661 The Lesson of the Dry Fur
(29663, 0, 0, 0, 29521, 29676, -29661521, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''),							 	-- Quest: 29663 The Lesson of the Balanced Rock
('29676', '0', '0', '0', '29663', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''), 		-- Quest: 29676 Finding an Old Friend
('29666', '0', '0', '0', '29676', '29677', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),		-- Quest: 29666 The Sting of Learning
('29677', '0', '0', '0', '29676', '29678', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),		-- Quest: 29677 The Sun Pearl 
('29678', '0', '0', '0', '29677', '29679', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '1', '0', ''),		-- Quest: 29678 Shu, the Spirit of Water
('29679', '0', '0', '0', '29678', '29680', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'quest_29679_a_new_friend'), -- Quest: 29679 A new friend
('29680', '0', '0', '0', '29679', '29769', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''); -- Quest: 29680 The Source of Our Livelihood

-- temp fix until quest accept works from auto completed quests and new offered quests
DELETE FROM `creature_queststarter` WHERE `quest` IN (29679, 29680);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES 
('54975', '29679', '0'),
('54975', '29680', '0');

-- Creature Templates
DELETE FROM `creature_template_addon` WHERE `entry` IN (60249, 60250, 57620);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureId`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, 
`movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
(57620, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Whittler Dewei
(60249, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Deng
(60250, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''); -- Cai

DELETE FROM `creature_template_difficulty` WHERE `entry` IN (60250, 60249, 54975, 55021, 56393, 56394, 57620, 65467);
INSERT INTO `creature_template_difficulty` VALUES
(54975, 0, 0, 0, 80, 4, 1, 1, 1, 1, 30538, 0, 0, 0, 0, 0, 0, 0, 0, 805306624, 0, 0, 0, 0, 0, 0, 0, 56647), 
(60249, 0, -3, -3, 1723, 11, 1, 1, 1, 1, 21699, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60250, 0, -3, -3, 1723, 11, 1, 1, 1, 1, 21697, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55021, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 30457, 2097160, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(56393, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28066, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(56394, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28064, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(57620, 0, 0, 0, 80, 4, 1.1, 1, 1, 0.2, 26364, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(65467, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 59127, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647);

DELETE FROM `creature_addon` WHERE `guid`=450772;
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(450772, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, '49414'); -- Aysa Cloudsinger - 49414 - Generic Quest Invisibility 1 - !!! already present in database !!!

-- Creature Spawns Addons Jojo at the Pools
-- Unnecessary spawn
DELETE FROM `gameobject` WHERE `guid` IN (300228, 300206, 300217, 300229, 300231, 300234, 300232, 300224, 300215, 300218, 300230);
DELETE FROM `creature` WHERE `guid` IN (450955, 450999, 450970, 450962, 450959, 450961, 450963, 450958, 450978, 451002, 450787, 450786, 450728, 451003);

-- Respawn creatures with the same guid
DELETE FROM `creature` WHERE `guid` IN (450755, 450745, 450759, 450741, 450752, 450747, 450742, 450760, 450744);
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `PhaseGroup`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `MovementType`, `npcflag`, `unit_flags`, `unit_flags2`, `unit_flags3`, `VerifiedBuild`) VALUES
(450755, 57620, 860, 5736, 5826, '0', 0, 0, 0, 1, 1047.8802490234375, 3293.57470703125, 130.4282684326171875, 2.894826173782348632, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Whittler Dewei (Area: Поющие пруды - Difficulty: 0) CreateObject1 - !!! already present in database !!!
(450745, 55021, 860, 5736, 5826, '0', 0, 0, 0, 0, 1039.4913330078125, 3283.111083984375, 129.5230712890625, 1.815142393112182617, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Jojo Ironbrow (Area: Поющие пруды - Difficulty: 0) CreateObject1 (Auras: 82343 - Generic Quest Invisibility 4) - !!! already present in database !!!

(450759, 65467, 860, 5736, 5826, '0', 0, 0, 0, 0, 1039.51220703125, 3293.630126953125, 129.3238372802734375, 4.677482128143310546, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Excited Onlooker (Area: Поющие пруды - Difficulty: 0) CreateObject1 (Auras: 78718 - Generic Quest Invisibility 8) - !!! already present in database !!!
(450741, 65467, 860, 5736, 5826, '0', 0, 0, 0, 0, 1041.9478759765625, 3291.673583984375, 129.422149658203125, 4.450589656829833984, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Excited Onlooker (Area: Поющие пруды - Difficulty: 0) CreateObject1 (Auras: 78718 - Generic Quest Invisibility 8)

(450752, 56394, 860, 5736, 5826, '0', 0, 0, 0, 0, 1038.0711669921875, 3291.072998046875, 129.069671630859375, 4.904375076293945312, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Mesmerized Child (Area: Поющие пруды - Difficulty: 0) CreateObject1 (Auras: 78718 - Generic Quest Invisibility 8)
(450747, 56394, 860, 5736, 5826, '0', 0, 0, 0, 0, 1040.79345703125, 3288.7734375, 129.3875732421875, 4.164384365081787109, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Mesmerized Child (Area: Поющие пруды - Difficulty: 0) CreateObject1 (Auras: 78718 - Generic Quest Invisibility 8) - !!! already present in database !!!
(450742, 56394, 860, 5736, 5826, '0', 0, 0, 0, 0, 1034.7535400390625, 3286.971435546875, 128.67218017578125, 5.917473793029785156, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Mesmerized Child (Area: Поющие пруды - Difficulty: 0) CreateObject1 (Auras: 78718 - Generic Quest Invisibility 8)
(450760, 56394, 860, 5736, 5826, '0', 0, 0, 0, 0, 1035.873291015625, 3287.873291015625, 128.843170166015625, 5.644624233245849609, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Mesmerized Child (Area: Поющие пруды - Difficulty: 0) CreateObject1 (Auras: 78718 - Generic Quest Invisibility 8) - !!! already present in database !!!

(450744, 56393, 860, 5736, 5826, '0', 0, 0, 0, 0, 1034.357666015625, 3290.73095703125, 128.7551116943359375, 5.323254108428955078, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978); -- Excited Onlooker (Area: Поющие пруды - Difficulty: 0) CreateObject1 (Auras: 78718 - Generic Quest Invisibility 8)


DELETE FROM `creature_addon` WHERE `guid` IN (450745, 450759, 450741, 450752, 450747, 450742, 450760, 450744);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(450745, 0, 0, 0, 0, 1, 1, 0, 461, 0, 0, 0, 0, '82343'), -- Jojo Ironbrow - 82343 - Generic Quest Invisibility 4 - !!! already present in database !!!
(450759, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '78718'), -- Excited Onlooker - 78718 - Generic Quest Invisibility 8 - !!! already present in database !!!
(450741, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '78718'), -- Excited Onlooker - 78718 - Generic Quest Invisibility 8
(450752, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '78718'), -- Mesmerized Child - 78718 - Generic Quest Invisibility 8
(450747, 0, 0, 0, 0, 1, 1, 0, 0, 1507, 0, 0, 0, '78718'), -- Mesmerized Child - 78718 - Generic Quest Invisibility 8 - !!! already present in database !!!
(450742, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '78718'), -- Mesmerized Child - 78718 - Generic Quest Invisibility 8
(450760, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, '78718'), -- Mesmerized Child - 78718 - Generic Quest Invisibility 8 - !!! already present in database !!!
(450744, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '78718'); -- Excited Onlooker - 78718 - Generic Quest Invisibility 8

UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=54975; -- Aysa Cloudsinger
UPDATE `creature_template` SET `AIName`='', `ScriptName`='npc_cai', `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=60250; 
UPDATE `creature_template` SET `AIName`='', `ScriptName`='npc_deng', `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=60249; 

-- Quest: 29662 Stronger Than Reeds
DELETE FROM `smart_scripts` WHERE `entryorguid`=29662 AND `source_type`=5;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `event_param_string`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `action_param_string`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `target_param_string`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(29662, 5, 0, 0, '', 50, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 108786, 0, 0, 0, 0, 0, 0, NULL, 7, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'On Quest 29662 Reward - Cast on Player - Summon Stack of Reeds');

-- Quest: 29663 The Lesson of the Balanced Rock
-- Quest: 29661 The Lesson of the Dry Fur

UPDATE `creature_template` SET `ScriptName` = 'npc_tushui_monk_on_pole' WHERE (`entry` = '55019');
UPDATE `creature_template` SET `ScriptName` = 'npc_tushui_monk_on_pole', `KillCredit1` = '55019' WHERE (`entry` = '65468');
UPDATE `creature_template` SET `ScriptName` = 'npc_balance_pole' WHERE `Entry` IN (54993, 57431, 55083);

DELETE FROM `vehicle_template` WHERE `creatureId` IN (54993, 57431, 55083, 56869);
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`, `CustomFlags`) VALUES ('54993', '0', '0');
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`, `CustomFlags`) VALUES ('57431', '0', '0');
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`, `CustomFlags`) VALUES ('55083', '0', '0');
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`, `CustomFlags`) VALUES ('56869', '0', '0');

UPDATE `creature_template` SET `unit_flags3` = '524288', `npcflag` = '50331648' WHERE `entry` IN (54993, 55083, 57431);

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (55083, 57431,54993);
INSERT INTO `npc_spellclick_spells` VALUES
(54993, 102717, 1, 0),
(55083, 102717, 1, 0),
(57431, 102717, 1, 0);

DELETE FROM `conditions` WHERE `SourceEntry` = 107049;
insert  into `conditions`(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) values 
(13,1,107049,0,0,51,0,5,56869,0,0,0,0,'','Ride Vehicle target Balance Pole Landing Bunny'),
(17,0,107049,0,0,1,0,133381,0,0,1,30,0,'','Ride Vehicle when player has not aura'),
(17,0,107049,0,0,29,0,56869,8,0,0,30,0,'','Ride Vehicle when bunny within 8y'),
(13,1,107049,0,0,31,0,5,56869,0,0,0,0,'','Ride Vehicle target Balance Pole Landing Bunny');

SET @CGUID := 900000;
DELETE FROM `creature` WHERE `guid` IN (@CGUID+743, @CGUID+744, @CGUID+745, @CGUID+746);
INSERT INTO `creature` (guid, id, map, zoneId, areaId, spawnDifficulties, phaseUseFlags, PhaseId, PhaseGroup, terrainSwapMap, modelid, equipment_id, position_x, position_y, position_z, orientation, spawntimesecs, wander_distance, currentwaypoint, curHealthPct, MovementType, npcflag, unit_flags, unit_flags2, unit_flags3, ScriptName, StringId, VerifiedBuild) VALUES
(@CGUID+743, '54993', '860', '5736', '5826', '0', '0', '169', '0', '-1', '0', '0', '968.982', '3293.41', '117.685', '-0.50883', '300', '0', '0', '100', '0', NULL, '262144', NULL, NULL, '', NULL, '0'),
(@CGUID+744, '54993', '860', '5736', '5826', '0', '0', '169', '0', '-1', '0', '0', '941.046', '3299.72', '117.024', '-0.24147', '300', '0', '0', '100', '0', NULL, '262144', NULL, NULL, '', NULL, '0'),
(@CGUID+745, '54993', '860', '5736', '5826', '0', '0', '169', '0', '-1', '0', '0', '929.348', '3299.2', '117.429', '0.55515', '300', '0', '0', '100', '0', NULL, '262144', NULL, NULL, '', NULL, '0'),
(@CGUID+746, '54993', '860', '5736', '5826', '0', '0', '169', '0', '-1', '0', '0', '1011.65', '3299.14', '116.784', '3.13979', '300', '0', '0', '100', '0', NULL, '262144', NULL, NULL, '', NULL, '0');

-- Quest: 29677 The Sun Pearl 

UPDATE `creature_template` SET `ScriptName` = 'npc_fang_she' WHERE `entry` = 55292;
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` = 54976; -- Barbed Ray

DELETE FROM `creature` WHERE `guid` IN ('451096', '451050', '451043'); -- remove duplicate Fang-She as not sure they are for different phases
UPDATE `creature` SET `wander_distance` = 5 AND `MovementType` = 1 WHERE id IN (54976); -- Barbed Rays should be moving
UPDATE `gameobject_template` SET `ContentTuningId` = 80 WHERE (`entry` = '209584');

DELETE FROM `smart_scripts` WHERE `entryorguid` = 54976 AND `source_type` = 0;
INSERT INTO `smart_scripts` (entryorguid, source_type, id, link, Difficulties,
    event_type, event_phase_mask, event_chance, event_flags,
    event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string,
    action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string,
    target_type, target_param1, target_param2, target_param3, target_param4, target_param_string,
    target_x, target_y, target_z, target_o,
    comment) VALUES 
('54976', '0', '0', '0', '', '0', '0', '100', '0', '3000', '5000', '6000', '8000', '0', '', '11', '128407', '0', '0', '0', '0', '0', '0', NULL, '2', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Every 6 - 8 seconds (3 - 5s initially) (IC) - Self: Cast spell  128407 on Victim');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '60411'); -- Water Pincer
DELETE FROM smart_scripts WHERE entryorguid IN (60411) AND source_type = 0;
INSERT INTO smart_scripts (
    entryorguid, source_type, id, link,
    event_type, event_phase_mask, event_chance, event_flags,
    event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string,
    action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string,
    target_type, target_param1, target_param2, target_param3, target_param4, target_param_string,
    target_x, target_y, target_z, target_o,
    comment
) VALUES
(
    60411, 0, 0, 0,
    0, 0, 100, 0,               -- EVENT_TYPE = 0 (In Combat)
    5000, 7000, 5000, 7000, 0, '',
    11, 128448, 0, 0, 0, 0, 0, 0, '',   -- ACTION_TYPE = 11 (Cast Spell)
    2, 0, 0, 0, 0, '',           -- TARGET_TYPE = 2 (Current Target)
    0, 0, 0, 0,
    'Water Pincer casts Phlogiston every 5-7 seconds'
);

UPDATE `creature` SET `PhaseId` = 964 WHERE `id` IN (60454, 60457);
UPDATE `creature` SET `MovementType` = 2 WHERE `id` = 60457;

DELETE FROM `creature_template_addon` WHERE `entry` IN (55015, 60411, 54976, 55292, 60454, 60457);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureId`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, 
`movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
(60454, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(60457, 6045700, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(55015, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(60411, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '116552'),
(54976, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(55292, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '');


DELETE FROM `creature_template_difficulty` WHERE `entry` IN (55015, 60411, 54976, 55292, 60454, 60457);
INSERT INTO `creature_template_difficulty` VALUES
(60454, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 21333, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60457, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 21327, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55015, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 30467, 1, 0, 0, 55015, 0, 0, 6, 6, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60411, 0, 0, 0, 80, 4, 0.7, 1, 1, 0.2, 21410, 65537, 0, 0, 60411, 0, 0, 9, 9, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(54976, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 30536, 0, 0, 0, 54976, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55292, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 30033, 0, 0, 0, 55292, 0, 1, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647);

-- Loot tables
-- Delete only reference loot items (ItemType = 1) for specific NPCs
DELETE FROM creature_loot_template 
WHERE Entry IN (54976, 55292, 60411, 55015) 
AND ItemType = 1;

-- NPC: 54976 Barbed Ray
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(54976, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(54976, 1, 3, 100, 0, 1, 1, 1, 'Water related beasts');

-- NPC: 55292 Fang-She
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(55292, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(55292, 1, 3, 100, 0, 1, 1, 1, 'Water related beasts');

-- NPC: 60411 Water Pincer
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(60411, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(60411, 1, 3, 100, 0, 1, 1, 1, 'Water related beasts');

-- NPC: 55015 Whitefeather Crane
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(55015, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(55015, 1, 7, 100, 0, 1, 1, 1, 'Bird or Flying beasts');

DELETE FROM `waypoint_path` WHERE `PathId` IN (6045700);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(6045700,0,0,'The Wandering Isle - Vision of Young Liang');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (6045700);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ, Orientation,Delay) values
(6045700, 1, 1033.42, 3215.86, 114.141, 0, 0),
(6045700, 2, 1022.4, 3217.83, 114.141, 0, 0),
(6045700, 3, 1018.58, 3212.31, 114.141, 0, 0),
(6045700, 4, 1023.44, 3204.75, 114.141, 0, 0),
(6045700, 5, 1031.87, 3201.36, 114.141, 0, 0),
(6045700, 6, 1045.64, 3202.29, 114.141, 0, 0),
(6045700, 7, 1051.84, 3209.9, 114.141, 0, 0),
(6045700, 8, 1045.16, 3219.34, 114.141, 0, 0),
(6045700, 9, 1040.06, 3227.04, 114.141, 0, 0),
(6045700, 10, 1038.56, 3232.97, 114.141, 0, 0),
(6045700, 11, 1051.79, 3231.9, 114.141, 0, 0),
(6045700, 12, 1056.31, 3225.55, 114.141, 0, 0),
(6045700, 13, 1059.53, 3217.61, 114.141, 0, 0),
(6045700, 14, 1051.41, 3210.45, 114.141, 0, 0),
(6045700, 15, 1039.15, 3198.65, 114.109, 0, 0),
(6045700, 16, 1027.84, 3194.99, 114.109, 0, 0),
(6045700, 17, 1021.41, 3199.74, 114.109, 0, 0),
(6045700, 18, 1020.83, 3207.86, 114.141, 0, 0),
(6045700, 19, 1036.36, 3210.61, 114.141, 0, 0),
(6045700, 20, 1045.86, 3214.15, 114.141, 0, 0),
(6045700, 21, 1055.54, 3224.68, 114.141, 0, 0),
(6045700, 22, 1052.03, 3227.78, 114.141, 0, 0),
(6045700, 23, 1040.15, 3223.93, 114.141, 0, 0),
(6045700, 24, 1035.53, 3219.7, 114.141, 0, 0),
(6045700, 25, 1027.34, 3213.58, 114.141, 0, 0),
(6045700, 26, 1024.65, 3203.78, 114.141, 0, 0),
(6045700, 27, 1029.32, 3195.56, 114.109, 0, 0),
(6045700, 28, 1038.38, 3195.97, 114.109, 0, 0),
(6045700, 29, 1041.18, 3208.39, 114.141, 0, 0),
(6045700, 30, 1039.5, 3221.29, 114.141, 0, 0),
(6045700, 31, 1038.4, 3228.55, 114.141, 0, 0),
(6045700, 32, 1041.76, 3233.66, 114.14, 0, 0),
(6045700, 33, 1051.43, 3234.98, 114.14, 0, 0),
(6045700, 34, 1060.51, 3229.98, 114.141, 0, 0),
(6045700, 35, 1059.11, 3220.5, 114.141, 0, 0),
(6045700, 36, 1046.72, 3214.42, 114.141, 0, 0);
