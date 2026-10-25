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
-- NPC: 96276 Legion Razorwing
-- NPC: 97034 Fury Champion
-- NPC: 96494 Felguard Butcher

-- NPC: 97142 Fel Spreader
-- NPC: 93011 Kayn Sunfury <Illidari>
-- NPC: 98460 Kor'vas Bloodthorn <Illidari>
-- NPC: 96884 Coilskar Sea-Caller <Servant of Illidan>
-- NPC: 93759 Jace Darkweaver <Illidari>
-- NPC: 100161 Legion Devastator
-- NPC: 96499 Jace Darkweaver (Cryptic Hollow Cave)

-- Quest: 39279 Assault On Mardum (Bonus Objectives)
-- Quest: 38759 Set Them Free
-- Quest: 40379 Enter the Illidari: Coilskar
-- Quest: 39049 Eye on the prize
-- Quest: 39050 Meeting With the Queen
-- Quest: 38766 Before we're overrun
-- Quest: 38765 Enter the Illidari: Shivarra
-- Quest: 39262 Give Me Sight Beyond Sight
-- Quest: 39495 Hidden No More

-- Spell: 191827 Destroying Fel Spreader (spell click)
-- Spell: 199617 Assault on Mardum: Fel Spreader Fel Explosion
-- Spell: 191662 Enter the Illidari: Summon Ashtongue Stalker
-- Spell: 191668 Enter the Illidari: Summon Coilskar Sea-Caller
-- Spell: 191671 Summon Shivarra Destroyer
-- Spell: 203369 Summon Matron Mother Malevolence
-- Spell: 194689 Fel Bombardment
-- Spell: 192714 Torrent of Souls
-- Spell: 192715 Their Numbers Are Legion: Torrent of Souls Beam

-- Generic Creature Template fixes
UPDATE `creature_template` SET `faction` = 2843 WHERE `entry` IN (98157,98160,99787,97959);
UPDATE `creature_template` SET `faction` = 4 WHERE `entry` IN (102923);
UPDATE `creature_template` SET `faction` = 73 WHERE `entry` IN (100286);
UPDATE `creature_template` SET `faction` = 35 WHERE `entry` IN (100545,100543,100549);
UPDATE `creature_template` SET `faction` = 1786 WHERE `entry` IN (97769);
UPDATE `creature_template` SET `unit_flags` = 0 WHERE `entry` IN (96877); -- Summoned Ashtongue should not be immune
UPDATE `creature_template` SET `unit_flags` = 0 WHERE `entry` IN (100244); -- Aranasi Maiden should not be immune
UPDATE `creature_template` SET `IconName` = 'questinteract' WHERE `entry` = 102916; -- Well of Souls should have quest interract


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

-- =========================
DELETE FROM `conversation_template` WHERE `Id` IN (569);
INSERT INTO `conversation_template` (`Id`, `FirstLineId`, `VerifiedBuild`) VALUES
(569, 1476, 69875);

DELETE FROM `conversation_line_template` WHERE `Id` IN (1476,1477,1478,1479,1480,1481,1736,1737,1738,1739,1828);
INSERT INTO `conversation_line_template` (`Id`, `UiCameraID`, `ActorIdx`, `VerifiedBuild`) VALUES
(1476, 262, 0, 69875),
(1477, 262, 0, 69875),
(1478, 100, 1, 69875),
(1479, 262, 0, 69875),
(1480, 262, 0, 69875),
(1481, 100, 1, 69875),
(1736, 119, 2, 69875),
(1737, 140, 3, 69875),
(1738, 265, 4, 69875),
(1739, 118, 2, 69875),
(1828, 118, 2, 69875);

DELETE FROM `conversation_actors` WHERE `ConversationId` IN (569);
INSERT  INTO `conversation_actors` (`ConversationId`, `ConversationActorId`, `Idx`, `CreatureId`, `CreatureDisplayInfoId`, `NoActorObject`, `ActivePlayerObject`, `VerifiedBuild`) VALUES
(569, 49935, 0, 93802, 65935, 0, 0, 69875),
(569, 49160, 1, 96420, 63986, 0, 0, 69875),
(569, 47908, 2, 93127, 61698, 0, 0, 69875),
(569, 49496, 3, 97676, 60790, 0, 0, 69875),
(569, 49948, 4, 97425, 64013, 0, 0, 69875);

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

-- ========================= Fixes for Return to the Black Temple
DELETE FROM `creature_queststarter` WHERE `quest` = 38729;
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES
(97303, 38729, 69587);

DELETE FROM `quest_template_addon` WHERE `ID` = 38729;
INSERT INTO `quest_template_addon` (`ID`, `PrevQuestID`, `NextQuestID`) VALUES
(38729, 38728, 0);

-- ========================= Fixes for The Keystone
DELETE FROM `creature_queststarter` WHERE `quest` = 38728;
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES
(97297, 38728, 69587);

DELETE FROM `quest_template_addon` WHERE `ID` = 38728;
INSERT INTO `quest_template_addon` (`ID`, `PrevQuestID`, `NextQuestID`) VALUES
(38728, 39663, 38729);

DELETE FROM `scene_template` WHERE `SceneId` = 1142;
INSERT INTO `scene_template` (`SceneId`, `Flags`, `ScriptPackageID`, `Encrypted`, `ScriptName`) VALUES
(1142, 20, 1512, 0, '');

UPDATE `creature_template` SET `ScriptName` = 'npc_cyana_nightglaive_fel_hammer', `AIName` = '' WHERE `entry` = 97297;
UPDATE `creature_template` SET `ScriptName` = 'npc_brood_queen_tyranna', `AIName` = '' WHERE `entry` = 93802;
UPDATE `creature_template` SET `ScriptName` = 'npc_keystone_downstairs_kill_credit', `AIName` = '' WHERE `entry` = 101760;
UPDATE `creature_template` SET `ScriptName` = 'npc_kayn_sunfury_keystone', `AIName` = '' WHERE `entry` = 97303;
UPDATE `gameobject_template` SET `ScriptName` = 'go_sargerite_keystone' WHERE `entry` = 245728;
UPDATE `gameobject_template` SET `ScriptName` = 'go_fel_portal' WHERE `entry` = 244466;

-- Teleport: Vault of the Wardens destination (TARGET_DEST_DB)
DELETE FROM `spell_target_position` WHERE `ID` = 205395;
INSERT INTO `spell_target_position` (`ID`, `EffectIndex`, `OrderIndex`, `MapID`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `VerifiedBuild`) VALUES
(205395, 0, 0, 1468, 4325.46, -620.53, -281.40, 1.517563, 69587);

-- ========================= Fixes for On Felbat Wings
DELETE FROM `creature_queststarter` WHERE `quest` = 39663;
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES
(93127, 39663, 69587);

DELETE FROM `quest_template_addon` WHERE `ID` = 39663;
INSERT INTO `quest_template_addon` (`ID`, `NextQuestID`, `ScriptName`) VALUES
(39663, 38728, 'quest_on_felbat_wings');

-- Izal Whitemoon (96653) - fel bat gossip + SetData talk (MenuID 18776)
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` = 96653;

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 15 AND `SourceGroup` = 18776 AND `SourceEntry` = 0;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(15, 18776, 0, 0, 0, 47, 0, 39663, 8, 0, 0, 0, 0, '', 'Izal gossip option 0 shown when quest 39663 On Felbat Wings is in progress');

DELETE FROM `smart_scripts` WHERE `entryorguid` = 96653 AND `source_type` = 0;
INSERT INTO `smart_scripts` (entryorguid, source_type, id, link, Difficulties, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string, target_type, target_param1, target_param2, target_param3, target_param4, target_param_string, target_x, target_y, target_z, target_o, comment) VALUES
(96653, 0, 0, 0, '', 62, 0, 100, 0, 18776, 0, 0, 0, 0, '', 134, 192136, 2, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Izal gossip option 0 -> invoker casts 192136 (taxi to Fel Hammer)'),
(96653, 0, 1, 0, '', 38, 0, 100, 0, 1, 1, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 21, 50, 0, 0, 0, '', 0, 0, 0, 0, 'Izal SetData(1,1) -> Talk 0 to closest player');

-- Kayn Sunfury (93127) - path end chain: dismount, SetData on Illidari, mount 65203, fly off, despawn
DELETE FROM `smart_scripts` WHERE `entryorguid` = 93127 AND `source_type` = 0 AND `id` IN (11,12,13,14,15,16,17,18,19);
INSERT INTO `smart_scripts` (entryorguid, source_type, id, link, Difficulties, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string, target_type, target_param1, target_param2, target_param3, target_param4, target_param_string, target_x, target_y, target_z, target_o, comment) VALUES
(93127, 0, 11, 12, '', 58, 0, 100, 0, 1, 9312700, 0, 0, 0, '', 43, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Kayn - waypoint path 9312700 ended - dismount'),
(93127, 0, 12, 13, '', 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 45, 1, 1, 0, 0, 0, 0, 0, '', 19, 96653, 0, 0, 0, '', 0, 0, 0, 0, 'link -> SetData(1,1) Izal Whitemoon 96653'),
(93127, 0, 13, 14, '', 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 45, 1, 1, 0, 0, 0, 0, 0, '', 10, 70033191, 0, 0, 0, '', 0, 0, 0, 0, 'link -> SetData(1,1) Korvas guid 70033191'),
(93127, 0, 14, 15, '', 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 45, 1, 1, 0, 0, 0, 0, 0, '', 19, 96420, 0, 0, 0, '', 0, 0, 0, 0, 'link -> SetData(1,1) Cyana 96420'),
(93127, 0, 15, 16, '', 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 45, 1, 1, 0, 0, 0, 0, 0, '', 19, 96655, 0, 0, 0, '', 0, 0, 0, 0, 'link -> SetData(1,1) Allari 96655'),
(93127, 0, 16, 17, '', 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 43, 0, 65203, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'link -> mount model 65203'),
(93127, 0, 17, 18, '', 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 60, 1, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'link -> disable gravity'),
(93127, 0, 18, 19, '', 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 69, 0, 0, 1, 0, 0, 0, 0, '', 8, 0, 0, 0, 0, '', 1687.663, 1476.996, 253.776, 0, 'link -> move to point (no pathfinding)'),
(93127, 0, 19, 0, '', 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 41, 30000, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'link -> force despawn 30s');

-- Followers - SetData(1,1) chain: dismount, mount 65203, disable gravity, despawn
DELETE FROM `smart_scripts` WHERE `entryorguid` = 96420 AND `source_type` = 0 AND `id` IN (4,5,6,7);
DELETE FROM `smart_scripts` WHERE `entryorguid` = 96655 AND `source_type` = 0 AND `id` IN (4,5,6,7);
DELETE FROM `smart_scripts` WHERE `entryorguid` = 99045 AND `source_type` = 0 AND `id` IN (2,3,4,5);
INSERT INTO `smart_scripts` (entryorguid, source_type, id, link, Difficulties, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string, target_type, target_param1, target_param2, target_param3, target_param4, target_param_string, target_x, target_y, target_z, target_o, comment) VALUES
(96420, 0, 4, 5, '', 38, 0, 100, 0, 1, 1, 0, 0, 0, '', 43, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Cyana - SetData(1,1) - dismount'),
(96420, 0, 5, 6, '', 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 43, 0, 65203, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'link -> mount model 65203'),
(96420, 0, 6, 7, '', 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 60, 1, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'link -> disable gravity'),
(96420, 0, 7, 0, '', 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 41, 30000, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'link -> force despawn 30s'),
(96655, 0, 4, 5, '', 38, 0, 100, 0, 1, 1, 0, 0, 0, '', 43, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Allari - SetData(1,1) - dismount'),
(96655, 0, 5, 6, '', 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 43, 0, 65203, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'link -> mount model 65203'),
(96655, 0, 6, 7, '', 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 60, 1, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'link -> disable gravity'),
(96655, 0, 7, 0, '', 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 41, 30000, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'link -> force despawn 30s'),
(99045, 0, 2, 3, '', 38, 0, 100, 0, 1, 1, 0, 0, 0, '', 43, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Korvas - SetData(1,1) - dismount'),
(99045, 0, 3, 4, '', 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 43, 0, 65203, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'link -> mount model 65203'),
(99045, 0, 4, 5, '', 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 60, 1, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'link -> disable gravity'),
(99045, 0, 5, 0, '', 61, 0, 100, 0, 0, 0, 0, 0, 0, '', 41, 30000, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'link -> force despawn 30s');

-- ========================= Fixes for Fel Secrets
DELETE FROM `creature_queststarter` WHERE `quest` = 40051;
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES
(99045, 40051, 69587);

-- PlayerChoice 231 (DH spec choice) -> playerchoice_fel_secrets
DELETE FROM `playerchoice` WHERE `ChoiceId` = 231;
INSERT INTO `playerchoice` (`ChoiceId`, `Question`, `ScriptName`, `VerifiedBuild`) VALUES
(231, 'Which demon hunter specialization do you want to learn?', 'playerchoice_fel_secrets', 69814);

DELETE FROM `playerchoice_response` WHERE `ChoiceId` = 231 AND `ResponseId` IN (478, 479);
INSERT INTO `playerchoice_response` (`ChoiceId`, `ResponseId`, `Index`, `ChoiceArtFileId`, `Flags`, `WidgetSetID`, `UiTextureAtlasElementID`, `SoundKitID`, `GroupID`, `UiTextureKitID`, `Header`, `SubHeader`, `ButtonTooltip`, `Answer`, `Description`, `Confirmation`, `RewardQuestID`, `VerifiedBuild`) VALUES
(231, 478, 0, 1274664, 0, 0, 0, 0, 0, 0, 'Havoc', 'Havoc', '', 'Havoc', 'Continue your mastery of Havoc.\n\nHavoc specialized demon hunters can use Metamorphosis to turn into winged demons. They focus on Melee DPS.\n\nChoosing Havoc grants |cFFFF0000|Hspell:162264|h[Metamorphosis]|h|r as well as |cFFFF0000|Hspell:185164|h[Mastery: Demonic Presence]|h|r.', '', NULL, 69814),
(231, 479, 1, 1274665, 0, 0, 0, 0, 0, 0, 'Vengeance', 'Vengeance', '', 'Vengeance', 'Change to mastery of Vengeance.\n\nVengeance specialized demon hunters can use Metamorphosis to turn into spiked demons. They focus on Tanking.\n\nChoosing Vengeance grants |cFFFF0000|Hspell:187827|h[Metamorphosis]|h|r as well as |cFFFF0000|Hspell:203747|h[Mastery: Fel Blood]|h|r.\n\nYour |cFFFF0000|Hspell:198013|h[Eye Beam]|h|r will change into |cFFFF0000|Hspell:212084|h[Fel Devastation]|h|r.', '', NULL, 69814);

DELETE FROM `phase_area` WHERE `PhaseId` IN (5343,5357);
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(7712, 5343, 'Mardum - Kor''vas at Illidari Foothold'),
(7712, 5357, 'Mardum - Mannethrel Darkstar at Illidari Foothold');

-- Condition: phase switch 5343 - Kor'vas at Illidari Foothold
-- Condition: phase switch 5357 - Mannethrel Darkstar at Illidari Foothold // not sure when this starts
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` IN (5343,5357);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26, 5343, 0, 0, 0, 47, 0, 40222, 66, 0, 0, 0, 0, '', 'Phase 5343 Kor''vas at Illidari Foothold active when quest 40222 The Imp Mother''s Tome is complete/rewarded'),
(26, 5343, 0, 0, 0, 47, 0, 39663, 66, 0, 1, 0, 0, '', 'Phase 5343 Kor''vas at Illidari Foothold NOT active when quest 39663 On Felbat Wings is complete/rewarded'),
(26, 5357, 0, 0, 0, 47, 0, 40222, 66, 0, 0, 0, 0, '', 'Phase 5357 Mannethrel Darkstar at Illidari Foothold active when quest 40222 The Imp Mother''s Tome is complete/rewarded');

-- Phase 5160 - Illidari at Illidari Foothold (Kayn, Cyana, Allari): drops once On Felbat Wings is done
DELETE FROM `phase_area` WHERE `AreaId` = 7712 AND `PhaseId` = 5160;
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(7712, 5160, 'Mardum - Illidari at Illidari Foothold');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` = 5160 AND `ConditionValue1` = 39663;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26, 5160, 0, 0, 0, 47, 0, 39663, 66, 0, 1, 0, 0, '', 'Phase 5160 Illidari at Illidari Foothold NOT active when quest 39663 On Felbat Wings is complete/rewarded');

-- ========================= Fixes Cry Havoc and Let Slip the Illidari! & Vengeance Will Be Mine!
-- TO-DO: When each npc is "taught" via the gossip option the spell that gives credit triggers also a scripted event
-- TO-DO: Implement said game events for all involved NPCs. They need to cast some spell and say some lines.

DELETE FROM `creature_queststarter` WHERE `quest` IN (39515,39516);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES
(93127, 39515, 69587),
(93127, 39516, 69587);

DELETE FROM `quest_template_addon` WHERE `ID` IN (39515,39516);
INSERT INTO `quest_template_addon` (`ID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`) VALUES
(39515, 39518, 39663, 0),
(39516, 39517, 39663, 0);

-- Allari the Souleater (96655) - secrets gossip kill credit (MenuID 18935)
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 15 AND `SourceGroup` = 18935 AND `SourceEntry` IN (0, 1);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(15, 18935, 0, 0, 0, 47, 0, 39515, 8, 0, 0, 0, 0, '', 'Allari gossip option 0 shown when quest 39515 Vengeance Will Be Mine! is in progress'),
(15, 18935, 1, 0, 0, 47, 0, 39516, 8, 0, 0, 0, 0, '', 'Allari gossip option 1 shown when quest 39516 Cry Havoc and Let Slip the Illidari! is in progress');

DELETE FROM `smart_scripts` WHERE `entryorguid` = 96655 AND `source_type` = 0 AND `id` IN (2,3);
INSERT INTO `smart_scripts` (entryorguid, source_type, id, link, Difficulties, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string, target_type, target_param1, target_param2, target_param3, target_param4, target_param_string, target_x, target_y, target_z, target_o, comment) VALUES
(96655, 0, 2, 0, '', 62, 0, 100, 0, 18935, 0, 0, 0, 0, '', 134, 194996, 2, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Allari gossip option 0 -> invoker casts 194996 (kill credit Allari)'),
(96655, 0, 3, 0, '', 62, 0, 100, 0, 18935, 1, 0, 0, 0, '', 134, 194996, 2, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Allari gossip option 1 -> invoker casts 194996 (kill credit Allari)');

-- Remaining secrets gossip NPCs - Kor'vas (99045) and Mannethrel (96652) need SmartAI
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (99045, 96652);

-- Condition: secrets gossip options shown only while the matching quest is in progress
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 15 AND (
    (`SourceGroup` = 18936 AND `SourceEntry` IN (0, 1)) OR
    (`SourceGroup` = 18435 AND `SourceEntry` IN (1, 2)) OR
    (`SourceGroup` = 18937 AND `SourceEntry` IN (0, 1)) OR
    (`SourceGroup` = 18823 AND `SourceEntry` IN (0, 1)));
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(15, 18936, 0, 0, 0, 47, 0, 39515, 8, 0, 0, 0, 0, '', 'Cyana gossip option 0 shown when quest 39515 Vengeance Will Be Mine! is in progress'),
(15, 18936, 1, 0, 0, 47, 0, 39516, 8, 0, 0, 0, 0, '', 'Cyana gossip option 1 shown when quest 39516 Cry Havoc and Let Slip the Illidari! is in progress'),
(15, 18435, 1, 0, 0, 47, 0, 39515, 8, 0, 0, 0, 0, '', 'Kayn gossip option 1 shown when quest 39515 Vengeance Will Be Mine! is in progress'),
(15, 18435, 2, 0, 0, 47, 0, 39516, 8, 0, 0, 0, 0, '', 'Kayn gossip option 2 shown when quest 39516 Cry Havoc and Let Slip the Illidari! is in progress'),
(15, 18937, 0, 0, 0, 47, 0, 39515, 8, 0, 0, 0, 0, '', 'Kor''vas gossip option 0 shown when quest 39515 Vengeance Will Be Mine! is in progress'),
(15, 18937, 1, 0, 0, 47, 0, 39516, 8, 0, 0, 0, 0, '', 'Kor''vas gossip option 1 shown when quest 39516 Cry Havoc and Let Slip the Illidari! is in progress'),
(15, 18823, 0, 0, 0, 47, 0, 39515, 8, 0, 0, 0, 0, '', 'Mannethrel gossip option 0 shown when quest 39515 Vengeance Will Be Mine! is in progress'),
(15, 18823, 1, 0, 0, 47, 0, 39516, 8, 0, 0, 0, 0, '', 'Mannethrel gossip option 1 shown when quest 39516 Cry Havoc and Let Slip the Illidari! is in progress');

DELETE FROM `smart_scripts` WHERE `entryorguid` = 96420 AND `source_type` = 0 AND `id` IN (2, 3);
DELETE FROM `smart_scripts` WHERE `entryorguid` = 93127 AND `source_type` = 0 AND `id` IN (8, 9);
DELETE FROM `smart_scripts` WHERE `entryorguid` = 99045 AND `source_type` = 0 AND `id` IN (0, 1);
DELETE FROM `smart_scripts` WHERE `entryorguid` = 96652 AND `source_type` = 0 AND `id` IN (0, 1);
INSERT INTO `smart_scripts` (entryorguid, source_type, id, link, Difficulties, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string, target_type, target_param1, target_param2, target_param3, target_param4, target_param_string, target_x, target_y, target_z, target_o, comment) VALUES
(96420, 0, 2, 0, '', 62, 0, 100, 0, 18936, 0, 0, 0, 0, '', 134, 195019, 2, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Cyana gossip option 0 -> invoker casts 195019 (kill credit Cyana)'),
(96420, 0, 3, 0, '', 62, 0, 100, 0, 18936, 1, 0, 0, 0, '', 134, 195019, 2, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Cyana gossip option 1 -> invoker casts 195019 (kill credit Cyana)'),
(93127, 0, 8, 0, '', 62, 0, 100, 0, 18435, 1, 0, 0, 0, '', 134, 195020, 2, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Kayn gossip option 1 -> invoker casts 195020 (kill credit Kayn)'),
(93127, 0, 9, 0, '', 62, 0, 100, 0, 18435, 2, 0, 0, 0, '', 134, 195020, 2, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Kayn gossip option 2 -> invoker casts 195020 (kill credit Kayn)'),
(99045, 0, 0, 0, '', 62, 0, 100, 0, 18937, 0, 0, 0, 0, '', 134, 195021, 2, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Kor''vas gossip option 0 -> invoker casts 195021 (kill credit Kor''vas)'),
(99045, 0, 1, 0, '', 62, 0, 100, 0, 18937, 1, 0, 0, 0, '', 134, 195021, 2, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Kor''vas gossip option 1 -> invoker casts 195021 (kill credit Kor''vas)'),
(96652, 0, 0, 0, '', 62, 0, 100, 0, 18823, 0, 0, 0, 0, '', 134, 195022, 2, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mannethrel gossip option 0 -> invoker casts 195022 (kill credit Mannethrel)'),
(96652, 0, 1, 0, '', 62, 0, 100, 0, 18823, 1, 0, 0, 0, '', 134, 195022, 2, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mannethrel gossip option 1 -> invoker casts 195022 (kill credit Mannethrel)');


-- ========================= Fixes for The Imp Mother's Tome
DELETE FROM `creature_queststarter` WHERE `quest` = 40222;
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES
(98711, 40222, 69587);

-- Kor'vas Bloodthorn (98711) -> npc_korvas_bloodthorn (quest-accept dialog, delayed Shivarra summon)
UPDATE `creature_template` SET `ScriptName` = 'npc_korvas_bloodthorn_foul_creche', `AIName` = '' WHERE `entry` = 98711;

-- Prolifica (98986) -> npc_prolifica (Foul Creche, Secret Tome visibility)
UPDATE `creature_template` SET `ScriptName` = 'npc_prolifica', `AIName` = '' WHERE `entry` = 98986;

-- Fel Geyser (100257) -> SmartAI, passive on spawn
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` = 100257;

DELETE FROM `smart_scripts` WHERE `entryorguid` = 100257 AND `source_type` = 0;
INSERT INTO smart_scripts (entryorguid, source_type, id, link, Difficulties, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string, target_type, target_param1, target_param2, target_param3, target_param4, target_param_string, target_x, target_y, target_z, target_o, comment) VALUES
(100257, 0, 0, 0, '', 11, 0, 100, 0, 0, 0, 0, 0, 0, '', 8, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Fel Geyser - On Respawn - Set React State Passive');

DELETE FROM `conversation_template` WHERE `Id` IN (585);
INSERT INTO `conversation_template` (`Id`, `FirstLineId`, `VerifiedBuild`) VALUES
(585, 1519, 69875);

DELETE FROM `conversation_line_template` WHERE `Id` IN (1519);
INSERT INTO `conversation_line_template` (`Id`, `UiCameraID`, `VerifiedBuild`) VALUES
(1519, 262, 69875);

DELETE FROM `conversation_actors` WHERE `ConversationId` IN (585);
INSERT  INTO `conversation_actors` (`ConversationId`, `ConversationActorId`, `Idx`, `CreatureId`, `CreatureDisplayInfoId`, `NoActorObject`, `ActivePlayerObject`, `VerifiedBuild`) VALUES
(585, 49935, 0, 93802, 65935, 0, 0, 69875);

DELETE FROM `phase_area` WHERE `PhaseId` IN (5344);
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(7912, 5344, 'Mardum - Kor''vas Bloodthorn at Imp Mother cave');

-- Condition: phase switch 5344
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` IN (5344);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26, 5344, 0, 0, 0, 47, 0, 40222, 66, 0, 1, 0, 0, '', 'Phase 5344 Foul Creche active when quest 40222 The Imp Mother''s Tome is NOT complete');

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
UPDATE `creature_template_difficulty` SET `StaticFlags1` = `StaticFlags1` | 0x20000000 WHERE `Entry` IN (94744,96276,100333,100334,98191);

-- SAI
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (98484,98486,98497,98482,95226,93112,93716,94654,97034,102726,96494,102724,
97706,99759,96277,96402,102923,96888,96279,96280,96877,96503);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (98484,98486,98497,98482,95226,93112,93716,94654,97034,102726,96494,102724,
97706,99759,96277,96402,102923,96888,96279,96280,96877,96503) AND source_type = 0;
DELETE FROM `smart_scripts` WHERE `entryorguid` = 93127 AND source_type = 0 AND `id` IN (6,7,10);
INSERT INTO smart_scripts (entryorguid, source_type, id, link, Difficulties, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string, target_type, target_param1, target_param2, target_param3, target_param4, target_param_string, target_x, target_y, target_z, target_o, comment) VALUES
(93127, 0, 6, 0, '', 19, 0, 100, 0, 39515, 0, 0, 0, 0, '', 1, 3, 0, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'On player accepted quest (39515) - Player who accepted quest: Talk 3 to invoker'),
(93127, 0, 7, 0, '', 19, 0, 100, 0, 39516, 0, 0, 0, 0, '', 1, 3, 0, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'On player accepted quest (39516) - Player who accepted quest: Talk 3 to invoker'),
(93127, 0, 10, 0, '', 19, 0, 100, 0, 39663, 0, 0, 0, 0, '', 1, 4, 0, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'On player accepted quest (39663) - Player who accepted quest: Talk 4 to invoker'),

(96503, 0, 0, 0, '', 0, 0, 100, 0, 3000, 3000, 5000, 5000, 0, '', 11, 197745, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Coilskar Sea-Caller - In Combat - Cast ''Lightning Bolt'''),

(96877, 0, 0, 0, '', 11, 0, 100, 0, 0, 0, 0, 0, 0, '', 1, 1, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Ashtongue Stalker - On Respawn - Talk 1 (No Repeat)'),
(96877, 0, 1, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Ashtongue Stalker - On Aggro - Talk 0 (No Repeat)'),

(96280, 0, 0, 0, '', 6, 0, 100, 1, 0, 0, 0, 0, 0, '', 11, 200757, 3, 0, 0, 0, 0, 0, '', 0, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Volatile Minion - On Just Died - Cast ''Explosive Minion on Death'' (No Repeat)'),
(96280, 0, 1, 0, '', 11, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 200756, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Volatile Minion - On Respawn - Cast ''Volatile Minion'' (No Repeat)'),
(96280, 0, 2, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Volatile Minion - On Aggro - Talk (No Repeat)'),

(96279, 0, 0, 0, '', 0, 0, 100, 0, 5000, 8000, 12000, 15000, 0, '', 11, 200827, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mardum Executioner - In Combat - Cast ''Tortured Souls'''),
(96279, 0, 1, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mardum Executioner - On Aggro - Talk (No Repeat)'),

(96888, 0, 0, 0, '', 11, 0, 100, 1, 0, 0, 0, 0, 0, '', 1, 0, 5000, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Shivarra Destroyer - On Spawn - Say Line 0 (No Repeat)'),

(102923, 0, 0, 0, '', 11, 0, 100, 1, 0, 0, 0, 0, 0, '', 1, 0, 5000, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Matron Mother Malevolence - On Spawn - Say Line 0 (No Repeat)'),

(96402, 0, 0, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 5000, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Hulking Forgefiend - On Aggro - Say Line 0 (No Repeat)'),
(96402, 0, 1, 0, '', 0, 0, 100, 0, 5000, 8000, 12000, 15000, 0, '', 11, 200781, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Hulking Forgefiend - In Combat - Cast ''Bad Breath'''),

(96277, 0, 0, 0, '', 0, 0, 100, 0, 5000, 8000, 12000, 15000, 0, '', 11, 200805, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Queen''s Centurion - In Combat - Cast ''Dark Communion'''),
(96277, 0, 1, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Queen''s Centurion - On Aggro - Talk (No Repeat)'),

(99759, 0, 0, 0, '', 0, 0, 100, 0, 5000, 8000, 12000, 15000, 0, '', 11, 200570, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Fiendish Creeper - In Combat - Cast ''Demon Spit'''),

(97706, 0, 0, 0, '', 0, 0, 100, 0, 5000, 8000, 12000, 15000, 0, '', 11, 200748, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Fel Weaver - In Combat - Cast ''Fel Weaving'''),
(97706, 0, 1, 0, '', 0, 0, 100, 0, 0, 0, 2000, 2500, 0, '', 11, 200751, 64, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Fel Weaver - In Combat - Cast ''Lob Poison'''),
(97706, 0, 2, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 5000, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Fel Weaver - On Aggro - Say Line 0 (No Repeat)'),

(102724, 0, 0, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 5000, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Vile Soulmaster - On Aggro - Say Line 0 (No Repeat)'),
(102724, 0, 1, 0, '', 0, 0, 100, 0, 5000, 8000, 12000, 15000, 0, '', 11, 200674, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Vile Soulmaster - In Combat - Cast ''Corrupt Soul'''),
(102724, 0, 2, 0, '', 2, 0, 100, 0, 0, 40, 22000, 25000, 0, '', 11, 200689, 2, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Vile Soulmaster - Between 0-40% Health - Cast ''Shattered Visage'''),
(102724, 0, 3, 0, '', 6, 0, 100, 0, 0, 0, 0, 0, 0, '', 1, 1, 5000, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Vile Soulmaster - On Just Died - Say Line 1'),

(96494, 0, 0, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 5000, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Felguard Butcher - In Combat - Say Line 0 (No Repeat)'),
(96494, 0, 1, 0, '', 0, 0, 100, 0, 5000, 8000, 12000, 15000, 0, '', 11, 200647, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Felguard Butcher - In Combat - Cast ''Shadow Blade'''),
(96494, 0, 2, 0, '', 2, 0, 100, 1, 0, 30, 1000, 1000, 0, '', 11, 200632, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Felguard Butcher - Between 0-30% Health - Cast ''Dark Rage'' (No Repeat)'),

(102726, 0, 0, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 5000, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Eredar Sorcerer - In Combat - Say Line 0 (No Repeat)'),
(102726, 0, 1, 0, '', 0, 0, 100, 0, 5000, 8000, 17000, 20000, 0, '', 11, 200615, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Eredar Sorcerer - In Combat - Cast ''Fel Resonance'''),
(102726, 0, 2, 0, '', 0, 0, 100, 0, 0, 0, 3000, 3000, 0, '', 11, 200582, 64, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Eredar Sorcerer - In Combat - Cast ''Fel Blast'''),

(97034, 0, 0, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 5000, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Fury Champion- On Aggro - Say Line 0 (No Repeat)'),
(97034, 0, 1, 0, '', 0, 0, 100, 0, 5000, 8000, 12000, 15000, 0, '', 11, 200753, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Fury Champion - In Combat - Cast ''Furious Flurry'''),

(93112, 0, 0, 0, '', 0, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Felguard Sentry - In Combat - Talk (No Repeat)'),
(93112, 0, 1, 0, '', 0, 0, 100, 0, 5000, 8000, 12000, 15000, 0, '', 11, 200570, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Felguard Sentry - In Combat - Cast ''Blazing Blade'''),

(94654, 0, 0, 0, '', 4, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 200608, 0, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Doomguard Eradicator - On Aggro - Cast ''Shadowflame'''),
(94654, 0, 1, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Doomguard Eradicator - On Aggro - Talk (No Repeat)'),

(93716, 0, 0, 0, '', 0, 0, 100, 0, 2000, 3000, 3000, 5000, 0, '', 11, 200525, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Doom Slayer - In Combat - Cast ''Inferno Axe'''),
(93716, 0, 1, 0, '', 4, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 200552, 0, 0, 0, 0, 0, 0, '', 7, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Doom Slayer - On Aggro - Cast ''Fel Crash'''),
(93716, 0, 2, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Doom Slayer - On Aggro - Talk (No Repeat)'),

(95226, 0, 0, 0, '', 0, 0, 100, 0, 1000, 2000, 3000, 4000, 0, '', 11, 200502, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Anguish Jailer - In Combat - Cast ''Anguished Soul'''),
(95226, 0, 1, 0, '', 6, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 200521, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Anguish Jailer - On Just Died - Cast ''Well of Souls Soul Visual'''),
(95226, 0, 2, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Anguis Jailer - In Combat - Talk (No Repeat)'),

(98484, 0, 0, 0, '', 0, 0, 100, 0, 1000, 2000, 8000, 9000, 0, '', 11, 200425, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mo''arg Brute - In Combat - Cast ''Brutal Slam'''),
(98484, 0, 1, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mo Arg Brute - In Combat - Talk (No Repeat)'),

(98486, 0, 0, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Wrath Warrior - In Combat - Talk (No Repeat)'),

(98497, 0, 0, 0, '', 1, 0, 100, 1, 1000, 1000, 0, 0, 0, '', 11, 188485, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Time = 1 seconds (OOC) - Self: Cast spell  188485 on Self'),
(98497, 0, 1, 0, '', 0, 0, 100, 0, 2000, 3000, 15000, 18000, 0, '', 11, 200393, 2, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Imp Mother - In Combat - Cast ''Rain of Imp'''),
(98497, 0, 2, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Imp Mother - In Combat - Talk (No Repeat)'),

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

DELETE FROM `areatrigger_create_properties` WHERE `Id` IN (4920,5648,5659,5668,5494,5319) AND `IsCustom` = 0;
INSERT INTO `areatrigger_create_properties` (Id, IsCustom, AreaTriggerId, IsAreatriggerCustom, Flags, MoveCurveId, ScaleCurveId, MorphCurveId, FacingCurveId, AnimId, AnimKitId, DecalPropertiesId, SpellForVisuals, MaxSummons, MaxUnits, PositionalSoundKitId, TimeToTargetScale, Speed, SpeedIsTime, Shape, ShapeData0, ShapeData1, ShapeData2, ShapeData3, ShapeData4, ShapeData5, ShapeData6, ShapeData7, Roll, Pitch, Yaw, TargetRoll, TargetPitch, TargetYaw, ScriptName, VerifiedBuild) VALUES
(4920, 0, 9637,  0, 16, 0, 0, 0, 0, -1, 0, 0, 192604, NULL, NULL, 0,     0, 1, 0, 0,   5,   5, 0, 0, 0,   0,   0,   0, 0, 0, 0, NULL, NULL, NULL, '',                            69814),
(5648, 0, 10356, 0, 0,  0, 0, 0, 0, -1, 0, 0, 200525, NULL, NULL, 0, 15000, 1, 0, 4,   4,   4, 4, 4, 0.3, 0.3, 0,   0, 0, 0, 0, NULL, NULL, NULL, '',                            69814),
(5659, 0, 10366, 0, 0,  0, 0, 0, 0, -1, 0, 0, 200647, NULL, NULL, 0, 10000, 1, 0, 3,   0,   0, 0, 0, 0,   0,   0,   0, 0, 0, 0, NULL, NULL, NULL, '',                            69814),
(5668, 0, 10375, 0, 0,  0, 0, 0, 0, -1, 0, 0, 200753, NULL, NULL, 0,  3000, 1, 0, 0, 3.5, 3.5, 0, 0, 0,   0,   0,   0, 0, 0, 0, NULL, NULL, NULL, '',                            69814),
(5494, 0, 10195, 0, 0,  0, 0, 0, 0, -1, 0, 0, 198947, NULL, NULL, 0,  8000, 1, 0, 0,  35,  35, 0, 0, 0,   0,   0,   0, 0, 0, 0, NULL, NULL, NULL, 'at_nefarious_carrion_storm', 69814),
(5319, 0, 10041, 0, 0,  0, 0, 0, 0, -1, 0, 0, 197245, NULL, NULL, 0, 15000, 1, 0, 0,   7,   7, 0, 0, 0,   0,   0,   0, 0, 0, 0, NULL, NULL, NULL, '',                            69814);


-- smart_scripts (creature entry 93127, id Y = 5)
SET @ENTRY := 93127, @ID := 5;

DELETE FROM `smart_scripts` WHERE `entryorguid`=@ENTRY AND `source_type`=0 AND `id`=@ID;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`target_type`,`target_param1`,`comment`) VALUES
(@ENTRY, 0, @ID, 0, 10, 0, 100, 1, 1, 100, 0, 0, 1, 1, 2, 0, 1, 7, 0, 'NPC - OOC LOS (quest 38765 complete) - Talk 2 to invoker');

-- condition on the invoker: quest 38765 status = complete
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=22 AND `SourceGroup`=@ID+1 AND `SourceEntry`=@ENTRY AND `SourceId`=0;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`Comment`) VALUES
(22, @ID+1, @ENTRY, 0, 0, 47, 0, 38765, 2, 'Invoker has quest 38765 complete');

-- smart_scripts (creature entry 96499, id Y = 0)
SET @ENTRY := 96499, @ID := 0;

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` = @ENTRY;
DELETE FROM `smart_scripts` WHERE `entryorguid`=@ENTRY AND `source_type`=0 AND `id`=@ID;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`target_type`,`target_param1`,`comment`) VALUES
(@ENTRY, 0, @ID, 0, 10, 0, 100, 1, 1, 20, 0, 0, 1, 1, 0, 0, 1, 7, 0, 'NPC - OOC LOS (quest 39495 incomplete) - Talk 0 to invoker');

-- condition on the invoker: quest 39495 status = incomplete
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=22 AND `SourceGroup`=@ID+1 AND `SourceEntry`=@ENTRY AND `SourceId`=0;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`Comment`) VALUES
(22, @ID+1, @ENTRY, 0, 0, 47, 0, 39495, 8, 'Invoker has quest 39495 complete');

-- Conditions needed to decide which Sevis NPC is shown based on whether he was sacrificed OR player
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 32 AND `SourceGroup` = 5 AND `SourceEntry` IN (105945, 99920) AND `ConditionValue1` = 40087;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
-- Sevis Brightflame (105945) - visible only while 40087 NOT rewarded
(32, 5, 105945, 0, 0, 8, 0, 40087, 0, 0, 0, 0, 0, '', 'Sevis Brightflame visible if quest 40087 is rewarded'),
-- Sevis Brightflame (99920) - visible only once 40087 IS rewarded
(32, 5, 99920, 0, 0, 8, 0, 40087, 0, 0, 1, 0, 0, '', 'Sevis Brightflame visible if quest 40087 NOT rewarded');

-- Remove immune flags for 97706 as they should be attackable
UPDATE `creature_template` SET `unit_flags` = `unit_flags` & ~0x300 WHERE `entry` = 97706;

-- NPC: 97059 King Voras -> npc_king_voras C++ script
UPDATE `creature_template` SET `ScriptName` = 'npc_king_voras', `AIName` = '' WHERE `entry` = 97059;

DELETE FROM `conversation_template` WHERE `Id` IN (567);
INSERT INTO `conversation_template` (`Id`, `FirstLineId`, `VerifiedBuild`) VALUES
(567, 1469, 69875);

DELETE FROM `conversation_line_template` WHERE `Id` IN (1469);
INSERT INTO `conversation_line_template` (`Id`, `UiCameraID`, `VerifiedBuild`) VALUES
(1469, 262, 69875);

DELETE FROM `conversation_actors` WHERE `ConversationId` IN (567);
INSERT  INTO `conversation_actors` (`ConversationId`, `ConversationActorId`, `Idx`, `CreatureId`, `CreatureDisplayInfoId`, `NoActorObject`, `ActivePlayerObject`, `VerifiedBuild`) VALUES
(567, 49935, 0, 93802, 65935, 0, 0, 69875);

-- "Stop the Bombardment" (38727) - Illidari Banners -> go_mardum_illidari_banner
-- 243968 Doom Fortress / 243967 Forge of Corruption / 243965 Soul Engine
UPDATE `gameobject_template` SET `ScriptName` = 'go_mardum_illidari_banner' WHERE `entry` IN (243965,243967,243968);

-- Sabotage trigger npcs -> npc_illidari_banner_trigger
-- 96562 Ashtongue Stalker / 96564 Priestess of Dementia / 96561 Coilskar Harpooner
UPDATE `creature_template` SET `ScriptName` = 'npc_illidari_banner_trigger', `AIName` = '' WHERE `entry` IN (96561,96562,96564);

-- "Their Numbers Are Legion" (38819) Stabilizer objective
DELETE FROM `conversation_template` WHERE `Id` IN (573,576);
INSERT INTO `conversation_template` (`Id`, `FirstLineId`, `VerifiedBuild`) VALUES
(573, 1491, 69875),
(576, 1496, 69875);

DELETE FROM `conversation_line_template` WHERE `Id` IN (1491,1492,1496);
INSERT INTO `conversation_line_template` (`Id`, `UiCameraID`, `VerifiedBuild`) VALUES
(1491, 99, 69875),
(1492, 99, 69875),
(1496, 120, 69875);

DELETE FROM `conversation_actors` WHERE `ConversationId` IN (573,576);
INSERT  INTO `conversation_actors` (`ConversationId`, `ConversationActorId`, `Idx`, `CreatureId`, `CreatureDisplayInfoId`, `NoActorObject`, `ActivePlayerObject`, `VerifiedBuild`) VALUES
(573, 49821, 0, 93759, 60791, 0, 0, 69875),
(576, 49945, 0, 96655, 60079, 0, 0, 69875);

-- "Their Numbers Are Legion" (38819) - Doom Fortress Stabilizer spellclick
-- clicker casts 201112 Destroying Stabilizer on it (cast_flags 1 = caster is clicker)
DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (101947,100545,100543,100549);
INSERT INTO `npc_spellclick_spells` (`npc_entry`,`spell_id`,`cast_flags`,`user_type`) VALUES
(101947,201112,1,0),
(100549,192262,1,0),
(100545,192262,1,0),
(100543,192262,1,0);

-- Only clickable while Their Numbers Are Legion (38819) is in the quest log
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 18 AND `SourceGroup` = 101947 AND `SourceEntry` = 201112;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(18,101947,201112,0,0,9,0,38819,0,0,0,0,0,'','Doom Fortress Stabilizer - spellclick requires quest 38819 Their Numbers Are Legion in log');

UPDATE `creature_template` SET `ScriptName` = 'npc_doom_fortress_stabilizer' WHERE `entry` = 101947;

-- Spire Stabilizer (245995) -> go_spire_stabilizer (Spire of Woe beams + use-despawn)
UPDATE `gameobject_template` SET `ScriptName` = 'go_spire_stabilizer' WHERE `entry` IN (245995,246012);

-- Floating flag
UPDATE `creature_template_difficulty` SET `StaticFlags1` = `StaticFlags1` | 0x20000000 WHERE `Entry` IN (101320);
-- Overseer Brutarg (97057) -> npc_overseer_brutarg
UPDATE `creature_template` SET `ScriptName` = 'npc_overseer_brutarg', `AIName` = '' WHERE `entry` = 97057;

-- Wounded Demon Hunter (100549) spellclick 192262 -> npc_wounded_demon_hunter
UPDATE `creature_template` SET `ScriptName` = 'npc_wounded_demon_hunter', `AIName` = '' WHERE `entry` IN (100545,100543,100549);

-- Count Nefarious (97058) -> npc_count_nefarious
UPDATE `creature_template` SET `ScriptName` = 'npc_count_nefarious', `AIName` = '' WHERE `entry` = 97058;

-- Cylinder Collision (246280) -> go_cylinder_collision (summons the Well of Souls)
UPDATE `gameobject_template` SET `ScriptName` = 'go_cylinder_collision' WHERE `entry` = 246280;

-- Well of Souls (102916) spellclick 198185 -> npc_well_of_souls
UPDATE `creature_template` SET `ScriptName` = 'npc_well_of_souls', `AIName` = '' WHERE `entry` = 102916;
DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` = 102916;
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES
(102916, 198185, 1, 0); -- clicker casts Opening on the Well

-- General Volroth (97370) -> npc_general_volroth (spawns captives, companion summon on engage)
UPDATE `creature_template` SET `ScriptName` = 'npc_general_volroth', `AIName` = '' WHERE `entry` = 97370;

-- Lady S'theno (97676) -> npc_lady_stheno (greeting, attacker pack, combat spells)
UPDATE `creature_template` SET `ScriptName` = 'npc_lady_stheno', `AIName` = '' WHERE `entry` = 97676;

-- ========================= The Fel Hammer phasing (area 7749)
-- Phase 101: active once On Felbat Wings (39663) is done, until The Keystone
--            (38728) completes - the boss fight / keystone window
-- Phase 102: active once The Keystone (38728) is done, until Return to the
--            Black Temple (38729) is rewarded
DELETE FROM `phase_area` WHERE `AreaId` = 7749 AND `PhaseId` IN (101, 102);
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(7749, 101, 'Mardum - The Fel Hammer: after On Felbat Wings until The Keystone'),
(7749, 102, 'Mardum - The Fel Hammer: after The Keystone until Return to the Black Temple');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` IN (101, 102);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26, 101, 0, 0, 0, 47, 0, 39663, 66, 0, 0, 0, 0, '', 'Phase 101 Fel Hammer active when quest 39663 On Felbat Wings is complete/rewarded'),
(26, 101, 0, 0, 0, 47, 0, 38728, 66, 0, 1, 0, 0, '', 'Phase 101 Fel Hammer NOT active when quest 38728 The Keystone is complete/rewarded'),
(26, 102, 0, 0, 0, 47, 0, 38728, 66, 0, 0, 0, 0, '', 'Phase 102 Fel Hammer active when quest 38728 The Keystone is complete/rewarded'),
(26, 102, 0, 0, 0, 47, 0, 38729, 64, 0, 1, 0, 0, '', 'Phase 102 Fel Hammer NOT active when quest 38729 Return to the Black Temple is rewarded');
