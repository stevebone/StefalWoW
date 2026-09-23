-- Mardum (Demon Hunter) Fixes

-- NPC: 93112 Felguard Sentry
-- NPC: 93115 Foul Felstalker
-- NPC: 93716 Doom Slayer
-- NPC: 98484 Mo'arg Brute
-- NPC: 98483 Hellish Imp
-- NPC: 98482 Foul Felstalker
-- NPC: 98497 Imp Mother
-- NPC: 98486 Wrath Warrior
-- NPC: 95226 Anguish Jailer
-- NPC: 94654 Doomguard Eradicator

-- NPC: 97142 Fel Spreader
-- NPC: 93011 Kayn Sunfury <Illidari>
-- NPC: 98460 Kor'vas Bloodthorn <Illidari>
-- NPC: 96884 Coilskar Sea-Caller <Servant of Illidan>
-- NPC: 93759 Jace Darkweaver <Illidari>
-- NPC: 100161 Legion Devastator

-- Quest: 39279 Assault On Mardum (Bonus Objectives)
-- Quest: 38759 Set Them Free
-- Quest: 40379 Enter the Illidari: Coilskar
-- Quest: 39049 Eye on the prize
-- Quest: 39050 Meeting With the Queen
-- Quest: 38766 Before we're overrun
-- Quest: 38765 Enter the Illidari: Shivarra

-- Spell: 191827 Destroying Fel Spreader (spell click)
-- Spell: 199617 Assault on Mardum: Fel Spreader Fel Explosion
-- Spell: 191668 Enter the Illidari: Summon Coilskar Sea-Caller
-- Spell: 194689 Fel Bombardment

-- ========================= Fixes for the quest 38765 Enter the Illidari: Shivarra / Fel Bombardments
DELETE FROM `conversation_template` WHERE `Id` = 747;
INSERT INTO `conversation_template` (`Id`, `FirstLineId`, `VerifiedBuild`) VALUES
(747, 1779, 69875);

DELETE FROM `conversation_line_template` WHERE `Id` IN (1779,1780,2938);
INSERT INTO `conversation_line_template` (`Id`, `UiCameraID`, `ActorIdx`, `VerifiedBuild`) VALUES
(1779, 119, 0, 69875),
(1780, 119, 0, 69875),
(2938, 263, 1, 69875);

DELETE FROM `conversation_actors` WHERE `ConversationId` = 747;
INSERT  INTO `conversation_actors` (`ConversationId`, `ConversationActorId`, `Idx`, `CreatureId`, `CreatureDisplayInfoId`, `NoActorObject`, `ActivePlayerObject`, `VerifiedBuild`) VALUES
(747, 49947, 0, 93127, 61698, 0, 0, 69875),
(747, 49935, 1, 93802, 65935, 0, 0, 69875);

-- ========================= Fixes for the Bonus quest
-- Update Kill Credits
UPDATE `creature_template` SET `KillCredit2` = 94651 WHERE `entry` = 98483; -- imps are min mobs
UPDATE `creature_template` SET `KillCredit2` = 95226 WHERE `entry` IN (98482,98486); -- Foul Felstalker & Wrath Warrior are normal mobs
UPDATE `creature_template` SET `KillCredit2` = 96400 WHERE `entry` = 98484; -- Brutes have their own kill credit

-- Add flag for auto reward
UPDATE `quest_template` SET `Flags` = `Flags` | 0x400 WHERE `ID` = 39279;

DELETE FROM `conversation_template` WHERE `Id` = 581;
INSERT INTO `conversation_template` (`Id`, `FirstLineId`, `VerifiedBuild`) VALUES
(581, 1511, 69875);

DELETE FROM `conversation_line_template` WHERE `Id` = 1511;
INSERT INTO `conversation_line_template` (`Id`, `UiCameraID`, `VerifiedBuild`) VALUES
(1511, 119, 69875);

DELETE FROM `conversation_actors` WHERE `ConversationId` = 581;
INSERT  INTO `conversation_actors` (`ConversationId`, `ConversationActorId`, `Idx`, `CreatureId`, `CreatureDisplayInfoId`, `NoActorObject`, `ActivePlayerObject`, `VerifiedBuild`) VALUES
(581, 49947, 0, 93127, 61698, 0, 0, 69875);

-- Clicker casts 191827 (2s cast) on the spreader (cast_flags 1 = caster is clicker)
DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` = 97142;
INSERT INTO `npc_spellclick_spells` (`npc_entry`,`spell_id`,`cast_flags`,`user_type`) VALUES
(97142,191827,1,0);

-- Fel Spreader (97142) spellclick -> npc_fel_spreader script
UPDATE `creature_template` SET `ScriptName` = 'npc_fel_spreader', `npcflag` = `npcflag` | 0x01000000 WHERE `entry` = 97142;

-- Only clickable while Assault On Mardum (39279) is in the quest log
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 18 AND `SourceGroup` = 97142 AND `SourceEntry` = 191827;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(18,97142,191827,0,0,9,0,39279,0,0,0,0,0,'','Fel Spreader - spellclick requires quest 39279 Assault On Mardum in log');

-- Ashtongue Mystic (99914) sacrifice scene -> npc_ashtongue_mystic C++ script
-- (replaces SAI timed action list 9991400: the soul-missile casts silently died on
--  the guid-target lookup / OOC gating; ScriptName replaces SmartAI for the entry)
UPDATE `creature_template` SET `ScriptName` = 'npc_ashtongue_mystic', `AIName` = '' WHERE `entry` = 99914;
DELETE FROM `smart_scripts` WHERE `entryorguid` = 99914 AND `source_type` = 0;
DELETE FROM `smart_scripts` WHERE `entryorguid` = 9991400 AND `source_type` = 9;

-- GO: 244439 Legion Communicator -> conversation 558 (Doom Commander Beliash)
-- GO: 244440 Legion Communicator -> conversation 558 (Doom Commander Beliash)
UPDATE `gameobject_template` SET `ScriptName` = 'go_legion_communicator' WHERE `entry` IN (244439,244440);

DELETE FROM `conversation_template` WHERE `Id` IN (558,583);
INSERT INTO `conversation_template` (`Id`, `FirstLineId`, `VerifiedBuild`) VALUES
(558, 1445, 69875),
(583, 1514, 69875);

DELETE FROM `conversation_line_template` WHERE `Id` IN (1445,1446,1514,1515);
INSERT INTO `conversation_line_template` (`Id`, `UiCameraID`, `VerifiedBuild`) VALUES
(1445, 254, 69875),
(1446, 254, 69875),
(1514, 254, 69875),
(1515, 254, 69875);

DELETE FROM `conversation_actors` WHERE `ConversationId` IN (558,583);
INSERT  INTO `conversation_actors` (`ConversationId`, `ConversationActorId`, `Idx`, `CreatureId`, `CreatureDisplayInfoId`, `NoActorObject`, `ActivePlayerObject`, `VerifiedBuild`) VALUES
(558, 49825, 0, 93221, 65308, 0, 0, 69875),
(583, 49825, 0, 93221, 65308, 0, 0, 69875);

-- ========================= Fixes for the Invasion Begins
-- Scene: 1116 The Invasion Begins (banner planted) -> Kayn Sunfury (98229) dialogue on complete
UPDATE `scene_template` SET `ScriptName` = 'scene_the_invasion_begins_banner_planted' WHERE `SceneId` = 1116;

-- Add missing gossip menus
DELETE FROM `creature_template_gossip` WHERE `CreatureID` IN (93011);
INSERT INTO `creature_template_gossip` (`CreatureID`, `MenuID`, `VerifiedBuild`) VALUES
(93011, 18761, 69587);

-- Creature Difficulties
-- Remove incorrect records
DELETE FROM `creature_template_difficulty` WHERE `DifficultyID` = 1 AND `Entry` IN (
	98486, 98482, 98484, 98483, 93115, 101288, 95226, 93105, 93112, 94651,
 96400, 94654, 95046, 102714, 93221, 93716, 99759, 96493, 96441, 96494,
 96473, 102726, 97034, 97706, 97059, 96277, 96402, 96280, 96278, 97014,
 103432, 96279, 97370, 102724, 97058, 97057, 100243, 100244, 93802, 98497, 98986
);

-- ========================= Fixes for Before We're Overrun
DELETE FROM `creature_queststarter` WHERE `quest` = 38766;
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES
(93759, 38766, 69587);

UPDATE `creature_template` SET `ScriptName` = 'npc_jace_darkweaver' WHERE `entry` = 93759;

DELETE FROM `conversation_template` WHERE `Id` IN (531);
INSERT INTO `conversation_template` (`Id`, `FirstLineId`, `VerifiedBuild`) VALUES
(531, 1383, 69875);

DELETE FROM `conversation_line_template` WHERE `Id` IN (1383);
INSERT INTO `conversation_line_template` (`Id`, `UiCameraID`, `VerifiedBuild`) VALUES
(1383, 99, 69875);

DELETE FROM `conversation_actors` WHERE `ConversationId` IN (531);
INSERT  INTO `conversation_actors` (`ConversationId`, `ConversationActorId`, `Idx`, `CreatureId`, `CreatureDisplayInfoId`, `NoActorObject`, `ActivePlayerObject`, `VerifiedBuild`) VALUES
(531, 49821, 0, 93759, 60791, 0, 0, 69875);

-- NPC: 93221 Doom Commander Beliash -> npc_doom_commander_beliash C++ script
UPDATE `creature_template` SET `ScriptName` = 'npc_doom_commander_beliash', `AIName` = '' WHERE `entry` = 93221;
DELETE FROM `smart_scripts` WHERE `entryorguid` = 93221 AND `source_type` = 0;

-- remove prior script
UPDATE creature_template SET ScriptName='' WHERE entry=96159;

-- Floating flag
UPDATE `creature_template_difficulty` SET `StaticFlags1` = `StaticFlags1` | 0x20000000 WHERE `Entry` IN (94744);

-- SAI
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (98484,98486,98497,98482,95226,93112,93716,94654);
DELETE FROM smart_scripts WHERE entryorguid IN (98484,98486,98497,98482,95226,93112,93716,94654) AND source_type = 0;
INSERT INTO smart_scripts (entryorguid, source_type, id, link, Difficulties, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string, target_type, target_param1, target_param2, target_param3, target_param4, target_param_string, target_x, target_y, target_z, target_o, comment) VALUES
(93112, 0, 0, 0, '', 0, 0, 100, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Felguard Sentry - In Combat - Talk (No Repeat)'),
(93112, 0, 2, 0, '', 0, 0, 100, 0, 5000, 8000, 12000, 15000, 0, '', 11, 200570, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Felguard Sentry - In Combat - Cast ''Blazing Blade'''),

(94654, 0, 0, 0, '', 4, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 200608, 0, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Doomguard Eradicator - On Aggro - Cast ''Shadowflame'''),
(94654, 0, 1, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Doomguard Eradicator - On Aggro - Talk (No Repeat)'),

(93716, 0, 0, 0, '', 0, 0, 100, 0, 2000, 3000, 3000, 5000, 0, '', 11, 200525, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Doom Slayer - In Combat - Cast ''Inferno Axe'''),
(93716, 0, 1, 0, '', 4, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 200552, 0, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Doom Slayer - On Aggro - Cast ''Fel Crash'''),
(93716, 0, 2, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Doom Slayer - On Aggro - Talk (No Repeat)'),

(95226, 0, 0, 0, '', 0, 0, 100, 0, 1000, 2000, 3000, 4000, 0, '', 11, 200502, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Anguish Jailer - In Combat - Cast ''Anguished Soul'''),
(95226, 0, 1, 0, '', 6, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 200521, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Anguish Jailer - On Just Died - Cast ''Well of Souls Soul Visual'''),
(95226, 0, 2, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Anguis Jailer - In Combat - Talk (No Repeat)'),


(98484, 0, 0, 0, '', 0, 0, 100, 0, 1000, 2000, 8000, 9000, 0, '', 11, 200425, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mo''arg Brute - In Combat - Cast ''Brutal Slam'''),
(98484, 0, 1, 4, '', 0, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mo Arg Brute - In Combat - Talk (No Repeat)'),

(98486, 0, 0, 4, '', 0, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Wrath Warrior - In Combat - Talk (No Repeat)'),

(98497, 0, 0, 0, '', 1, 0, 100, 1, 1000, 1000, 0, 0, 0, '', 11, 188485, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Time = 1 seconds (OOC) - Self: Cast spell  188485 on Self'),
(98497, 0, 1, 0, '', 0, 0, 100, 0, 2000, 3000, 15000, 18000, 0, '', 11, 200393, 2, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Imp Mother - In Combat - Cast ''Rain of Imp'''),
(98497, 0, 2, 4, '', 0, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Imp Mother - In Combat - Talk (No Repeat)'),

(98482, 0, 0, 0, '', 0, 0, 100, 0, 5000, 8000, 12000, 15000, 0, '', 11, 200417, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Foul Felstalker - In Combat - Cast ''Foul Fel''');

DELETE FROM `graveyard_zone` WHERE `GhostZone` = 7705;
INSERT INTO `graveyard_zone` (`ID`, `GhostZone`, `Comment`) VALUES
(5082, 7705, 'DH-Mardum - (01) Start'),
(5284, 7705, 'DH-Mardum - (02) Molten Shore'),
(5083, 7705, 'DH-Mardum - (03) Seat of Command'),
(5119, 7705, 'DH-Mardum - (04) Illidari Foothold'),
(5140, 7705, 'DH-Mardum - (05) Volcano'),
(5188, 7705, 'DH-Mardum - (06) The Fel Hammer');

-- ========================= Coilskar Sea-Caller (quest 40379) =========================
-- NPC: 96884 Coilskar Sea-Caller -> npc_coilskar_sea_caller C++ script
UPDATE `creature_template` SET `ScriptName` = 'npc_coilskar_sea_caller', `AIName` = '' WHERE `entry` = 96884;
DELETE FROM `smart_scripts` WHERE `entryorguid` = 96884 AND `source_type` = 0;

-- Quest: 40379 Enter the Illidari: Coilskar -> quest_enter_the_illidari_coilskar C++ script
-- (fires on objective 280771 'Coilskar Forces' completion -> delayed summon cast)
UPDATE `quest_template_addon` SET `ScriptName` = 'quest_enter_the_illidari_coilskar' WHERE `ID` = 40379;

-- Quest: 38765 Enter the Illidari: Shivarra -> quest_enter_the_illidari_shivarra C++ script
-- (fires on status complete after 'Shivarra Forces' completion -> delayed summon cast)
UPDATE `quest_template_addon` SET `ScriptName` = 'quest_enter_the_illidari_shivarra' WHERE `ID` = 38765;

-- Spell Positions for Fel Bombardment
DELETE FROM `spell_target_position` WHERE `ID` = 194689;
INSERT INTO `spell_target_position` (`ID`, `EffectIndex`, `OrderIndex`, `MapID`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `VerifiedBuild`) VALUES
(194689, 0, 0, 1481, 1343.7, 2395.82, 37.3399, 0, 69814),
(194689, 0, 1, 1481, 1307.2, 2389.51, 24.0129, 0, 69814),
(194689, 0, 2, 1481, 972.597, 2546.27, -48.3863, 0, 69814),
(194689, 0, 3, 1481, 839.219, 2557.78, -55.9799, 0, 69814),
(194689, 0, 4, 1481, 629.656, 2356.3, -72.4208, 0, 69814),
(194689, 0, 5, 1481, 718.997, 2645.75, -63.6153, 0, 69814),
(194689, 0, 6, 1481, 791.236, 2440.04, -58.5238, 0, 69814),
(194689, 0, 7, 1481, 1553.72, 2657.18, 25.5053, 0, 69814),
(194689, 0, 8, 1481, 1343.92, 2420.86, 36.34, 0, 69814),
(194689, 0, 9, 1481, 1562.19, 2483.67, 60.4382, 0, 69814),
(194689, 0, 10, 1481, 1477.4, 2459.62, 49.7893, 0, 69814),
(194689, 0, 11, 1481, 1485.24, 2494.81, 47.7704, 0, 69814),
(194689, 0, 12, 1481, 1071.3, 2518.44, -31.1496, 0, 69814),
(194689, 0, 13, 1481, 1328.96, 2414.99, 30.7737, 0, 69814),
(194689, 0, 14, 1481, 987.221, 2472.38, -78.3217, 0, 69814),
(194689, 0, 15, 1481, 751.707, 2451.73, -62.4183, 0, 69814),
(194689, 0, 16, 1481, 687.773, 2412.73, -67.1286, 0, 69814),
(194689, 0, 17, 1481, 775.446, 2480.36, -63.5775, 0, 69814),
(194689, 0, 18, 1481, 980.747, 2870.43, 2.35552, 0, 69814),
(194689, 0, 19, 1481, 882.099, 2891.23, -22.7101, 0, 69814),
(194689, 0, 20, 1481, 985.601, 3037.96, -12.8181, 0, 69814),
(194689, 0, 21, 1481, 881.516, 2994.1, -32.1582, 0, 69814),
(194689, 0, 22, 1481, 1036.43, 3066.39, -5.33064, 0, 69814),
(194689, 0, 23, 1481, 1031.06, 2870.49, 3.68664, 0, 69814),
(194689, 0, 24, 1481, 1170.78, 3075.64, -15.8303, 0, 69814),
(194689, 0, 25, 1481, 1374.7, 2589.24, 20.6083, 0, 69814),
(194689, 0, 26, 1481, 1342.65, 2577.87, 17.6051, 0, 69814),
(194689, 0, 27, 1481, 1294.82, 2545.42, 13.1256, 0, 69814),
(194689, 0, 28, 1481, 1512.81, 2536.66, 49.5114, 0, 69814),
(194689, 0, 29, 1481, 1506.24, 2516.88, 50.8371, 0, 69814),
(194689, 0, 30, 1481, 1329.37, 2386.51, 33.9066, 0, 69814),
(194689, 0, 31, 1481, 1142.96, 2560.93, -20.4551, 0, 69814),
(194689, 0, 32, 1481, 709.372, 2398.98, -60.1543, 0, 69814),
(194689, 0, 33, 1481, 713.328, 2435.79, -67.0482, 0, 69814),
(194689, 0, 34, 1481, 760.156, 2572.35, -70.629, 0, 69814),
(194689, 0, 35, 1481, 846.608, 2462.18, -52.8285, 0, 69814),
(194689, 0, 36, 1481, 888.95, 2838.16, -6.23767, 0, 69814),
(194689, 0, 37, 1481, 737.292, 2639.74, -59.5846, 0, 69814),
(194689, 0, 38, 1481, 711.191, 2632.67, -74.7857, 0, 69814),
(194689, 0, 39, 1481, 926.262, 2894.84, -16.7389, 0, 69814),
(194689, 0, 40, 1481, 1109.46, 2888.8, 4.69396, 0, 69814),
(194689, 0, 41, 1481, 859.865, 3008.98, -33.0248, 0, 69814),
(194689, 0, 42, 1481, 1530.19, 2544.15, 51.5711, 0, 69814),
(194689, 0, 43, 1481, 1274.52, 2558.7, 8.50722, 0, 69814),
(194689, 0, 44, 1481, 1456.19, 2413.66, 55.5412, 0, 69814),
(194689, 0, 45, 1481, 1247.1, 2546.87, 2.74974, 0, 69814),
(194689, 0, 46, 1481, 1575.19, 2612.82, 34.366, 0, 69814),
(194689, 0, 47, 1481, 1358.34, 2378.37, 42.6075, 0, 69814),
(194689, 0, 48, 1481, 1372.52, 2362.25, 48.75, 0, 69814),
(194689, 0, 49, 1481, 1321.41, 2362.35, 31.219, 0, 69814),
(194689, 0, 50, 1481, 1032.35, 2522.56, -41.679, 0, 69814),
(194689, 0, 51, 1481, 724.382, 2456.86, -65.4787, 0, 69814),
(194689, 0, 52, 1481, 778.941, 2558.14, -70.4904, 0, 69814),
(194689, 0, 53, 1481, 766.861, 2513.55, -67.9641, 0, 69814),
(194689, 0, 54, 1481, 807.76, 2481.24, -60.5552, 0, 69814),
(194689, 0, 55, 1481, 681.116, 2433.63, -71.4885, 0, 69814),
(194689, 0, 56, 1481, 949.703, 2916.29, -12.9388, 0, 69814),
(194689, 0, 57, 1481, 1113.48, 2941.3, -3.66415, 0, 69814),
(194689, 0, 58, 1481, 1014.64, 3085.32, -1.7208, 0, 69814),
(194689, 0, 59, 1481, 1053.18, 3095.49, 0.0678609, 0, 69814),
(194689, 0, 60, 1481, 1111.66, 2994.49, -8.29367, 0, 69814),
(194689, 0, 61, 1481, 801.721, 2462.98, -58.3619, 0, 69814),
(194689, 0, 62, 1481, 749.648, 2629.87, -49.0207, 0, 69814),
(194689, 0, 63, 1481, 960.625, 3058.87, -22.6355, 0, 69814),
(194689, 0, 64, 1481, 1343.7, 2395.82, 37.3399, 0, 69814),
(194689, 0, 65, 1481, 1567.99, 2643.78, 26.9733, 0, 69814),
(194689, 0, 66, 1481, 1307.2, 2389.51, 24.0129, 0, 69814),
(194689, 0, 67, 1481, 1513.5, 2455.94, 55.3132, 0, 69814),
(194689, 0, 68, 1481, 1355.71, 2555.19, 18.3287, 0, 69814),
(194689, 0, 69, 1481, 1132.88, 2548.75, -21.5645, 0, 69814),
(194689, 0, 70, 1481, 1109.21, 2542.04, -25.6875, 0, 69814),
(194689, 0, 71, 1481, 700.736, 2427.94, -68.544, 0, 69814),
(194689, 0, 72, 1481, 807.042, 2420.95, -56.5806, 0, 69814),
(194689, 0, 73, 1481, 1073.33, 2853.78, 8.37862, 0, 69814),
(194689, 0, 74, 1481, 1530.86, 2515.36, 54.1501, 0, 69814),
(194689, 0, 75, 1481, 1315.75, 2560.01, 15.0239, 0, 69814),
(194689, 0, 76, 1481, 1409.83, 2351.14, 61.4071, 0, 69814),
(194689, 0, 77, 1481, 1210.26, 2561.66, -3.19314, 0, 69814),
(194689, 0, 78, 1481, 729.2, 2427.37, -64.609, 0, 69814),
(194689, 0, 79, 1481, 762.236, 2475.26, -61.6805, 0, 69814),
(194689, 0, 80, 1481, 784.946, 2424.28, -58.9812, 0, 69814),
(194689, 0, 81, 1481, 818.278, 2451.95, -62.8465, 0, 69814),
(194689, 0, 82, 1481, 1532.53, 2537.58, 52.1327, 0, 69814),
(194689, 0, 83, 1481, 1250.82, 2572.66, 3.93576, 0, 69814),
(194689, 0, 84, 1481, 1155.2, 2554.79, -16.5259, 0, 69814),
(194689, 0, 85, 1481, 1183.38, 2558.61, -9.55035, 0, 69814),
(194689, 0, 86, 1481, 1341.6, 2355.39, 40.1803, 0, 69814),
(194689, 0, 87, 1481, 1032.35, 2522.56, -41.679, 0, 69814),
(194689, 0, 88, 1481, 744.299, 2549.16, -68.98, 0, 69814),
(194689, 0, 89, 1481, 718.997, 2645.75, -63.6153, 0, 69814),
(194689, 0, 90, 1481, 749.648, 2629.87, -49.0207, 0, 69814),
(194689, 0, 91, 1481, 1477.4, 2459.62, 49.7893, 0, 69814),
(194689, 0, 92, 1481, 1567.99, 2643.78, 26.9733, 0, 69814),
(194689, 0, 93, 1481, 1343.7, 2395.82, 37.3399, 0, 69814),
(194689, 0, 94, 1481, 673.521, 2332.26, -65.9305, 0, 69814),
(194689, 0, 95, 1481, 724.382, 2456.86, -65.4787, 0, 69814),
(194689, 0, 96, 1481, 805.082, 2514.45, -57.0216, 0, 69814),
(194689, 0, 97, 1481, 1380.84, 2576.01, 22.4598, 0, 69814),
(194689, 0, 98, 1481, 1485.24, 2494.81, 47.7704, 0, 69814),
(194689, 0, 99, 1481, 681.116, 2433.63, -71.4885, 0, 69814),
(194689, 0, 100, 1481, 818.278, 2451.95, -62.8465, 0, 69814),
(194689, 0, 101, 1481, 961.75, 2496.98, -70.3759, 0, 69814),
(194689, 0, 102, 1481, 778.941, 2558.14, -70.4904, 0, 69814),
(194689, 0, 103, 1481, 942.109, 2941.79, -16.6126, 0, 69814),
(194689, 0, 104, 1481, 1011.61, 2889.24, 0.918915, 0, 69814),
(194689, 0, 105, 1481, 987.243, 2895.9, -2.7283, 0, 69814),
(194689, 0, 106, 1481, 907.134, 2929.99, -19.7672, 0, 69814),
(194689, 0, 107, 1481, 1409.83, 2351.14, 61.4071, 0, 69814),
(194689, 0, 108, 1481, 1506.24, 2516.88, 50.8371, 0, 69814),
(194689, 0, 109, 1481, 826.736, 2509.87, -60.7681, 0, 69814),
(194689, 0, 110, 1481, 629.656, 2356.3, -72.4208, 0, 69814),
(194689, 0, 111, 1481, 699.826, 2653.49, -73.8398, 0, 69814),
(194689, 0, 112, 1481, 1036.43, 3066.39, -5.33064, 0, 69814),
(194689, 0, 113, 1481, 1372.52, 2362.25, 48.75, 0, 69814),
(194689, 0, 114, 1481, 700.736, 2427.94, -68.544, 0, 69814),
(194689, 0, 115, 1481, 1380.769, 2307.047, 68.79, 0, 69814),
(194689, 0, 116, 1481, 1359.921, 2272.701, 78.418, 0, 69814),
(194689, 0, 117, 1481, 1352.14, 2228.301, 87.94, 0, 69814),
(194689, 0, 118, 1481, 1360.766, 2178.386, 93.686, 0, 69814),
(194689, 0, 119, 1481, 1381.394, 2134.039, 99.793, 0, 69814),
(194689, 0, 120, 1481, 1407.384, 2096.695, 106.86, 0, 69814);

-- NPC: 100161 Legion Devastator -> npc_legion_devastator C++ script
UPDATE `creature_template` SET `ScriptName` = 'npc_legion_devastator', `AIName` = '' WHERE `entry` = 100161;
DELETE FROM `smart_scripts` WHERE `entryorguid` = 100161 AND `source_type` = 0;

DELETE FROM `areatrigger_create_properties` WHERE `Id` = 4920 AND `IsCustom` = 0;
INSERT INTO `areatrigger_create_properties` VALUES
(4920, 0, 9637, 0, 16, 0, 0, 0, 0, -1, 0, 0, NULL, 0, 0, 1, 0, 0, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, '', 69814);