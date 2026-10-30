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
-- Spell: 191466 Illidari Reinforcements

-- Spell: 191466 Illidari Reinforcements -> spell_illidari_reinforcements C++ aura script
DELETE FROM `spell_script_names` WHERE `spell_id` = 191466;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(191466, 'spell_illidari_reinforcements');

-- Spell: 191466 Illidari Reinforcements - active in Mardum (area 7705) while quest 40379 is taken,
-- until quest 39663 On Felbat Wings is complete/rewarded
DELETE FROM `spell_area` WHERE `spell` = 191466 AND `area` = 7705;
INSERT INTO `spell_area` (`spell`, `area`, `quest_start`, `quest_start_status`, `quest_end_status`, `quest_end`, `aura_spell`, `racemask`, `gender`, `flags`) VALUES
(191466, 7705, 40379, 74, 41, 39663, 0, 0, 2, 3);


-- Generic Creature Template fixes
UPDATE `creature_template` SET `faction` = 2843 WHERE `entry` IN (98157,98160,99787,97959);
UPDATE `creature_template` SET `faction` = 4 WHERE `entry` IN (102923);
UPDATE `creature_template` SET `faction` = 73 WHERE `entry` IN (100286);
UPDATE `creature_template` SET `faction` = 35 WHERE `entry` IN (100545,100543,100549);
UPDATE `creature_template` SET `faction` = 1786 WHERE `entry` IN (97769);
UPDATE `creature_template` SET `unit_flags` = 0 WHERE `entry` IN (96877); -- Summoned Ashtongue should not be immune
UPDATE `creature_template` SET `unit_flags` = 0 WHERE `entry` IN (100244); -- Aranasi Maiden should not be immune
UPDATE `creature_template` SET `IconName` = 'questinteract' WHERE `entry` = 102916; -- Well of Souls should have quest interract

-- Generic Creature Template Addon fixes (auras incl. 151597 No NPC Damage Below 45-90%)
DELETE FROM `creature_template_addon` WHERE `entry` IN (97676,93115,94654,94704,95226,94705,93112,96931,99759,101397,98157,96504,96494,96473,102726,97034,97706,96562,99787,96277,98160,108408,98165,96564,96563,98158,99423,96278,97014,99419,102724,101597,101317);
INSERT INTO `creature_template_addon` (`entry`, `auras`, `SheathState`) VALUES
(97676, '151597 198294', 1),
(94654, '151597', 1),
(93115, '151597', 1),
(94704, '222589 151597', 1),
(95226, '151597', 1),
(94705, '222589 151597', 1),
(93112, '151597', 1),
(96931, '222589 151597 42459', 1),
(99759, '151597', 1),
(101397, '222589 151597 42459', 1),
(98157, '222589 151597 42459', 1),
(96504, '151597', 1),
(96494, '151597', 1),
(96473, '151597', 1),
(102726, '151597', 1),
(97034, '151597', 1),
(97706, '151597', 1),
(96562, '151597', 1),
(99787, '222589 151597 42459', 1),
(96277, '151597', 1),
(98160, '222589 151597 42459 198294', 1),
(108408, '222589 151597 42459 198294', 1),
(98165, '222589 151597 42459 198294', 1),
(96564, '151597', 1),
(96563, '151597 198294', 1),
(98158, '222589 151597 42459 198294', 1),
(99423, '222589 151597 42459', 1),
(96278, '151597', 1),
(97014, '151597', 1),
(99419, '222589 151597 42459', 1),
(102724, '151597', 1),
(101597, '222589 151597 42459 198294', 1),
(101317, '222589 151597 42459', 1);


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
-- no longer needed as now spell summon guardian is handled via spell area
UPDATE `quest_template_addon` SET `ScriptName` = '' WHERE `ID` = 40379;

-- Quest: 38765 Enter the Illidari: Shivarra -> quest_enter_the_illidari_shivarra C++ script
-- (fires on status complete after 'Shivarra Forces' completion -> delayed summon cast)
UPDATE `quest_template_addon` SET `ScriptName` = 'quest_enter_the_illidari_shivarra' WHERE `ID` = 38765;

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

-- =====================================================
-- Sniff data updates

UPDATE `quest_template` SET `Flags`=0x2090100, `VerifiedBuild`=70575 WHERE `ID`=39279; -- Assault on Mardum
UPDATE `quest_template` SET `VerifiedBuild`=70575 WHERE `ID` IN (40378, 95043, 94437, 93902, 94431, 93901, 40077);


UPDATE `quest_objectives` SET `VerifiedBuild`=70575 WHERE `ID` IN (279757, 279288, 280768, 279289, 280769, 281030, 279292, 280292, 279741, 281333, 279929, 281335, 279930, 279935, 281158);
UPDATE `quest_objectives` SET `ConditionalAmount`=2004574620, `VerifiedBuild`=70575 WHERE `ID`=280276; -- 280276
UPDATE `quest_objectives` SET `ConditionalAmount`=-889192448, `VerifiedBuild`=70575 WHERE `ID`=279928; -- 279928
UPDATE `quest_objectives` SET `ConditionalAmount`=31360, `VerifiedBuild`=70575 WHERE `ID`=281334; -- 281334
UPDATE `quest_objectives` SET `ConditionalAmount`=3, `VerifiedBuild`=70575 WHERE `ID`=281156; -- 281156
UPDATE `quest_objectives` SET `ConditionalAmount`=135628800, `VerifiedBuild`=70575 WHERE `ID`=279936; -- 279936
UPDATE `quest_objectives` SET `ConditionalAmount`=-2147483648, `VerifiedBuild`=70575 WHERE `ID`=279937; -- 279937


UPDATE `quest_visual_effect` SET `VerifiedBuild`=70575 WHERE (`Index`=0 AND `ID` IN (281030,281156,280768));


UPDATE `creature_template` SET `VerifiedBuild`=70575 WHERE `entry` IN (101288, 93105, 94400, 93112, 93115, 94410, 93117, 98290, 98292, 99160, 93127, 101748, 96159, 100931, 100504, 98354, 100510, 65183, 93221, 23837, 93230, 102714, 105316, 97142, 97592, 96731, 98456, 97594, 98457, 98458, 98459, 94580, 98460, 97624, 99351, 97629, 95046, 97634, 95048, 98497, 95049, 93759, 93762, 98075, 94651, 94654, 93802, 96400, 99003, 97712, 94704, 24021, 94705, 101603, 100745, 96877, 96884, 98611, 93011, 98618, 98621, 98622, 99916, 99918, 99919, 98227, 98228, 98229, 95226, 101696, 101704, 94377, 37490);
UPDATE `creature_template` SET `KillCredit2`=0, `VerifiedBuild`=70575 WHERE `entry`=98482; -- Foul Felstalker
UPDATE `creature_template` SET `KillCredit2`=0, `VerifiedBuild`=70575 WHERE `entry`=98483; -- Hellish Imp
UPDATE `creature_template` SET `KillCredit2`=0, `VerifiedBuild`=70575 WHERE `entry`=98484; -- Mo'arg Brute
UPDATE `creature_template` SET `KillCredit2`=0, `VerifiedBuild`=70575 WHERE `entry`=98486; -- Wrath Warrior


UPDATE `creature_template_model` SET `VerifiedBuild`=70575 WHERE (`Idx`=0 AND `CreatureID` IN (65183,96159,99160,93230,93802,98611,97634,97624,97629,95048,101696,93759,105316,24021,93127,99003,93221,94410,99916,98621,98622,98354,98229,98618,100745,102714,95046,96884,23837,93117,94654,95049,101603,96877,94651,100931,93105,94400,96400,94704,94705,99351,94377,96731,101288,100504,37490,100510,95226,93115,93112,101748,101704,97142,97592,98456,98457,93762,98459,98460,98458,99919,98484,98486,98482,98497,98483,94580,97712,98292,98290,99918,93011,98228,98227,97594,98075)) OR (`Idx`=2 AND `CreatureID` IN (99160,101603,94704,94705)) OR (`Idx`=1 AND `CreatureID` IN (99160,97634,97624,97629,24021,99003,100745,102714,95046,23837,95049,101603,100931,94704,94705,37490,100510,101748,101704)) OR (`Idx`=3 AND `CreatureID` IN (94704,94705));


UPDATE `creature_questitem` SET `VerifiedBuild`=70575 WHERE (`CreatureEntry`=93802 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=93221 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=102714 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=95046 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=94654 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=93105 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=96400 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=101288 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=95226 AND `DifficultyID`=0 AND `Idx`=1) OR (`CreatureEntry`=95226 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=93115 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=93112 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=98484 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=98486 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=98482 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=98497 AND `DifficultyID`=0 AND `Idx`=0);


UPDATE `creature_quest_currency` SET `VerifiedBuild`=70575 WHERE (`CurrencyId`=1155 AND `CreatureId` IN (93802,93221,102714,95046,94654,93105,96400,101288,95226,93115,93112,98484,98486,98482,98497)) OR (`CurrencyId`=3252 AND `CreatureId` IN (93802,93221,102714,95046,94654,94651,93105,96400,101288,95226,93115,93112,98484,98486,98482,98497,98483));

DELETE FROM `gameobject_template` WHERE `entry`=245316;
INSERT INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `size`, `Data0`, `Data1`, `Data2`, `Data3`, `Data4`, `Data5`, `Data6`, `Data7`, `Data8`, `Data9`, `Data10`, `Data11`, `Data12`, `Data13`, `Data14`, `Data15`, `Data16`, `Data17`, `Data18`, `Data19`, `Data20`, `Data21`, `Data22`, `Data23`, `Data24`, `Data25`, `Data26`, `Data27`, `Data28`, `Data29`, `Data30`, `Data31`, `Data32`, `Data33`, `Data34`, `RequiredLevel`, `ContentTuningId`, `VerifiedBuild`) VALUES
(245316, 3, 33263, 'Small Treasure Chest', '', 'Opening', '', 1, 57, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21400, 0, 0, 0, 681, 2, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62804, 0, 0, 0, 0, 8, 699, 70575); -- Small Treasure Chest

UPDATE `gameobject_template` SET `RequiredLevel`=8, `VerifiedBuild`=70575 WHERE `entry` IN (244691, 244689); -- Small Treasure Chest
UPDATE `gameobject_template` SET `Data19`=2, `RequiredLevel`=8, `ContentTuningId`=699, `VerifiedBuild`=70575 WHERE `entry`=246249; -- Small Treasure Chest
UPDATE `gameobject_template` SET `VerifiedBuild`=70575 WHERE `entry` IN (253931, 246486, 246356, 246485, 246280, 259043, 250560, 245045, 246309, 246567, 246566);
UPDATE `gameobject_template` SET `RequiredLevel`=8, `VerifiedBuild`=70575 WHERE `entry`=243059; -- Demon Ward
UPDATE `gameobject_template` SET `RequiredLevel`=8, `VerifiedBuild`=70575 WHERE `entry`=243335; -- Nether Crucible

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (101696);
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES
(101696, 200598, 1, 0);

UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=65183; -- Spirit Healer
UPDATE `creature_template` SET `unit_flags3`=0x40000001 WHERE `entry`=96159; -- Colossal Infernal
UPDATE `creature_template` SET `unit_flags`=0x300 WHERE `entry`=98611; -- Doomguard Eradicator
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=97624; -- Spire of Woe
UPDATE `creature_template` SET `faction`=2028, `npcflag`=16777216, `speed_run`=1.428571462631225585, `BaseAttackTime`=1500, `unit_flags2`=0x4000800, `unit_flags3`=0x40080000 WHERE `entry`=101696; -- Shadow Portal
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=105316; -- Questioner Arev'naal
UPDATE `creature_template` SET `faction`=14, `speed_walk`=2, `speed_run`=0.714285731315612792, `BaseAttackTime`=2000, `unit_flags`=0x2000000, `unit_flags2`=0x800, `unit_flags3`=0x80000 WHERE `entry`=99003; -- Fel Cannon
UPDATE `creature_template` SET `npcflag`=3 WHERE `entry`=94410; -- Allari the Souleater
UPDATE `creature_template` SET `npcflag`=3 WHERE `entry`=98229; -- Kayn Sunfury
UPDATE `creature_template` SET `unit_flags`=0x300 WHERE `entry`=98621; -- Imp Mother
UPDATE `creature_template` SET `unit_flags`=0x300 WHERE `entry`=98622; -- Mo'arg Brute
UPDATE `creature_template` SET `unit_flags`=0x300 WHERE `entry`=98618; -- Hellish Imp
UPDATE `creature_template` SET `faction`=2189, `speed_walk`=2, `speed_run`=0.714285731315612792, `BaseAttackTime`=2000, `unit_flags`=0x2000000, `unit_flags2`=0x800, `unit_flags3`=0x80000 WHERE `entry`=100745; -- Fel Cannon
UPDATE `creature_template` SET `unit_flags3`=0x41000000 WHERE `entry`=95049; -- Demon Ward
UPDATE `creature_template` SET `faction`=1786, `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x80000 WHERE `entry`=101603; -- Lesser Imp
UPDATE `creature_template` SET `unit_flags`=0x300, `unit_flags3`=0x40000000 WHERE `entry`=96877; -- Ashtongue Stalker
UPDATE `creature_template` SET `unit_flags2`=0x4010800, `unit_flags3`=0x41000000 WHERE `entry` IN (96731, 93762); -- Legion Devastator
UPDATE `creature_template` SET `unit_flags3`=0x40000001 WHERE `entry`=100510; -- The Fel Hammer
UPDATE `creature_template` SET `unit_flags3`=0x40000001 WHERE `entry`=101748; -- Fel Geyser
UPDATE `creature_template` SET `unit_flags`=0x300 WHERE `entry`=97592; -- Felguard Sentry
UPDATE `creature_template` SET `unit_flags2`=0x800, `unit_flags3`=0x1000001 WHERE `entry`=97142; -- Fel Spreader
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=98497; -- Imp Mother
UPDATE `creature_template` SET `npcflag`=281474976710659 WHERE `entry`=93011; -- Kayn Sunfury
UPDATE `creature_template` SET `unit_flags`=0x300 WHERE `entry`=97712; -- Wrath Warrior
UPDATE `creature_template` SET `unit_flags`=0x300 WHERE `entry`=97594; -- Foul Felstalker
UPDATE `creature_template` SET `unit_flags2`=0x800 WHERE `entry`=99218; -- Legion Devastator Missile
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=100161; -- Legion Devastator

DELETE FROM `vehicle_template_accessory` WHERE (`entry`=93762 AND `seat_id`=0);
INSERT INTO `vehicle_template_accessory` (`entry`, `accessory_entry`, `seat_id`, `minion`, `description`, `summontype`, `summontimer`) VALUES
(93762, 99218, 0, 1, 'Legion Devastator - Legion Devastator Missile', 5, 0); -- Legion Devastator - Legion Devastator Missile


UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=65183 AND `DifficultyID`=0); -- 65183 (Spirit Healer) - Sessile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=96159 AND `DifficultyID`=0); -- 96159 (Colossal Infernal) - Sessile - CannotSwim, InfiniteAoi
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `VerifiedBuild`=70575 WHERE (`Entry`=99160 AND `DifficultyID`=0); -- 99160 (Beaming Eye) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=93230 AND `DifficultyID`=0); -- 93230 (Mannethrel Darkstar) - CanSwim - CannotTurn
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=70575 WHERE (`DifficultyID`=0 AND `Entry` IN (98611,24021,99003,98621,98622,98618,23837,95049,100931,96400,97592,97712,97594,94744,94492,94655));
UPDATE `creature_template_difficulty` SET `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=97634 AND `DifficultyID`=0); -- 97634 (Spire of Woe) - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=97624 AND `DifficultyID`=0); -- 97624 (Spire of Woe) - Sessile, Floating - CannotTurn - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=97629 AND `DifficultyID`=0); -- 97629 (Spire of Woe) - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x90000000, `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=95048 AND `DifficultyID`=0); -- 95048 (Brood Queen Tyranna) - CanSwim, LargeAoi - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x90000000, `VerifiedBuild`=70575 WHERE (`Entry`=93221 AND `DifficultyID`=0); -- 93221 (Doom Commander Beliash) - CanSwim, LargeAoi
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=101696 AND `DifficultyID`=0); -- 101696 (Shadow Portal) - Sessile, CanSwim - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x90000000, `VerifiedBuild`=70575 WHERE (`Entry`=93759 AND `DifficultyID`=0); -- 93759 (Jace Darkweaver) - CanSwim, LargeAoi - CannotTurn
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=105316 AND `DifficultyID`=0); -- 105316 (Questioner Arev'naal) - Sessile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x90000000, `VerifiedBuild`=70575 WHERE (`Entry`=94410 AND `DifficultyID`=0); -- 94410 (Allari the Souleater) - CanSwim, LargeAoi - CannotTurn
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=99916 AND `DifficultyID`=0); -- 99916 (Sevis Brightflame) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=98354 AND `DifficultyID`=0); -- 98354 (Kor'vas Bloodthorn) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=98229 AND `DifficultyID`=0); -- 98229 (Kayn Sunfury) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `VerifiedBuild`=70575 WHERE (`Entry`=100745 AND `DifficultyID`=0); -- 100745 (Fel Cannon) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=102714 AND `DifficultyID`=0); -- 102714 (Eredar Summoner) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=95046 AND `DifficultyID`=0); -- 95046 (Eredar Summoner) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=93117 AND `DifficultyID`=0); -- 93117 (Izal Whitemoon) - CanSwim - CannotTurn
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=94654 AND `DifficultyID`=0); -- 94654 (Doomguard Eradicator) - CanSwim, Floating - HideInCombatLog
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=101603 AND `DifficultyID`=0); -- 101603 (Lesser Imp) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=96877 AND `DifficultyID`=0); -- 96877 (Ashtongue Stalker) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=94651 AND `DifficultyID`=0); -- 94651 (Hellish Imp) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0xB0000000, `VerifiedBuild`=70575 WHERE (`Entry`=93105 AND `DifficultyID`=0); -- 93105 (Inquisitor Baleful) - CanSwim, Floating, LargeAoi
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=94400 AND `DifficultyID`=0); -- 94400 (Belath Dawnblade) - CanSwim - CannotTurn
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=94704 AND `DifficultyID`=0); -- 94704 (Demon Hunter) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=94377 AND `DifficultyID`=0); -- 94377 (Cyana Nightglaive) - CanSwim - CannotTurn
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000000, `VerifiedBuild`=70575 WHERE (`Entry`=99351 AND `DifficultyID`=0); -- 99351 (Legion Devastator Missile) - Floating - InfiniteAoi
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=94705 AND `DifficultyID`=0); -- 94705 (Demon Hunter) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=96731 AND `DifficultyID`=0); -- 96731 (Legion Devastator) - Sessile - InfiniteAoi - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=101288 AND `DifficultyID`=0); -- 101288 (Abyssal Basilisk) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=100510 AND `DifficultyID`=0); -- 100510 (The Fel Hammer) - Sessile - InfiniteAoi - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000000, `VerifiedBuild`=70575 WHERE (`Entry`=37490 AND `DifficultyID`=0); -- 37490 (ELM General Purpose Bunny Infinite) - Floating - InfiniteAoi
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0xB0000000, `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=101704 AND `DifficultyID`=0); -- 101704 (Empowered Fel Crystal) - CanSwim, Floating, LargeAoi - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x80000100, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=101748 AND `DifficultyID`=0); -- 101748 (Fel Geyser) - Sessile, LargeAoi - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=95226 AND `DifficultyID`=0); -- 95226 (Anguish Jailer) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags5`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=97142 AND `DifficultyID`=0); -- 97142 (Fel Spreader) - InteractWhileHostile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=93115 AND `DifficultyID`=0); -- 93115 (Foul Felstalker) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=93112 AND `DifficultyID`=0); -- 93112 (Felguard Sentry) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=98456 AND `DifficultyID`=0); -- 98456 (Allari the Souleater) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=98457 AND `DifficultyID`=0); -- 98457 (Cyana Nightglaive) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=93762 AND `DifficultyID`=0); -- 93762 (Legion Devastator) - Sessile - InfiniteAoi - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=98459 AND `DifficultyID`=0); -- 98459 (Kayn Sunfury) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=98460 AND `DifficultyID`=0); -- 98460 (Kor'vas Bloodthorn) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=98458 AND `DifficultyID`=0); -- 98458 (Jace Darkweaver) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=99919 AND `DifficultyID`=0); -- 99919 (Sevis Brightflame) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x90000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=98482 AND `DifficultyID`=0); -- 98482 (Foul Felstalker) - CanSwim, LargeAoi - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x90000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=98483 AND `DifficultyID`=0); -- 98483 (Hellish Imp) - CanSwim, LargeAoi - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=98484 AND `DifficultyID`=0); -- 98484 (Mo'arg Brute) - LargeAoi - CannotSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=98497 AND `DifficultyID`=0); -- 98497 (Imp Mother) - Sessile, CanSwim - GiganticAoi - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x90000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=98486 AND `DifficultyID`=0); -- 98486 (Wrath Warrior) - CanSwim, LargeAoi - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=94580 AND `DifficultyID`=0); -- 94580 (Wrath Warrior) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=98228 AND `DifficultyID`=0); -- 98228 (Jace Darkweaver) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=98227 AND `DifficultyID`=0); -- 98227 (Allari the Souleater) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=93011 AND `DifficultyID`=0); -- 93011 (Kayn Sunfury) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=99918 AND `DifficultyID`=0); -- 99918 (Sevis Brightflame) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=98292 AND `DifficultyID`=0); -- 98292 (Kor'vas Bloodthorn) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=98290 AND `DifficultyID`=0); -- 98290 (Cyana Nightglaive) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000000, `VerifiedBuild`=70575 WHERE (`Entry`=99218 AND `DifficultyID`=0); -- 99218 (Legion Devastator Missile) - Floating - InfiniteAoi
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=99656 AND `DifficultyID`=0); -- 99656 (Red Broodling) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=98191 AND `DifficultyID`=0); -- 98191 (Legion Command Center) - Floating - InfiniteAoi - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `StaticFlags4`=0x2000000, `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=100161 AND `DifficultyID`=0); -- 100161 (Legion Devastator) - Sessile - InfiniteAoi - HideInCombatLog - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=99650 AND `DifficultyID`=0); -- 99650 (Thornclaw Broodling) - CanSwim


UPDATE `creature_model_info` SET `VerifiedBuild`=70575 WHERE `DisplayID` IN (32803, 61910, 64029, 38795, 61732, 39810, 65935, 65308, 61911, 39412, 64742, 63526, 21999, 64808, 64743, 67769, 64741, 61906, 61908, 60550, 16982, 13069, 63194, 64416, 68182, 60090, 61909, 61912, 60087, 67022, 59610, 11686, 62552, 65115, 62534, 62593, 66015, 65648, 60791, 60079, 61698, 66396, 66159, 63986, 63969, 62513, 65204, 65203, 22769, 72961, 66210, 65683, 66377, 66213, 64294);
UPDATE `creature_model_info` SET `BoundingRadius`=4.083724021911621093, `VerifiedBuild`=70575 WHERE `DisplayID`=65202;


UPDATE `creature_template_gossip` SET `VerifiedBuild`=70575 WHERE (`CreatureID`=93759 AND `MenuID`=18438) OR (`CreatureID`=98229 AND `MenuID`=18864) OR (`CreatureID`=93011 AND `MenuID`=18761);
UPDATE `gossip_menu` SET `VerifiedBuild`=70575 WHERE (`TextID`=27998 AND `MenuID`=19131) OR (`TextID`=26813 AND `MenuID`=18438) OR (`TextID`=27954 AND `MenuID`=19109) OR (`TextID`=27471 AND `MenuID`=18864) OR (`TextID`=27283 AND `MenuID` IN (18993,18761));
UPDATE `gossip_menu_option` SET `VerifiedBuild`=70575 WHERE (`OptionID`=0 AND `MenuID` IN (18993,18864,18438)) OR (`OptionID`=1 AND `MenuID`=18438);


DELETE FROM `creature_template_addon` WHERE `entry` IN (65183 /*65183 (Spirit Healer) - Ghost, Ghost Visual*/, 97634 /*97634 (Spire of Woe) - Before We're Overrun: Spire of Woe - Area Trigger*/, 97629 /*97629 (Spire of Woe) - Before We're Overrun: Spire of Woe - Area Trigger*/, 95048 /*95048 (Brood Queen Tyranna) - Before We're Overrun: Brood Queen Tyranna's Teleport*/, 93221 /*93221 (Doom Commander Beliash)*/, 101696 /*101696 (Shadow Portal) - Shadow Portal*/, 105316 /*105316 (Questioner Arev'naal)*/, 99003 /*99003 (Fel Cannon)*/, 94410 /*94410 (Allari the Souleater)*/, 99916 /*99916 (Sevis Brightflame)*/, 98354 /*98354 (Kor'vas Bloodthorn)*/, 98229 /*98229 (Kayn Sunfury)*/, 100745 /*100745 (Fel Cannon)*/, 102714 /*102714 (Eredar Summoner) - Eredar Summoner: Fel Channel Omni*/, 95046 /*95046 (Eredar Summoner) - Eredar Summoner: Fel Channel Omni*/, 100931 /*100931 (Fel Cannon)*/, 99351 /*99351 (Legion Devastator Missile)*/, 96731 /*96731 (Legion Devastator)*/, 100510 /*100510 (The Fel Hammer) - The Fel Hammer Beam Event: Bottom Bunny Fel Orb & Grow Aura, The Fel Hammer Beam Event: Shrink Aura*/, 97592 /*97592 (Felguard Sentry) - Permanent Feign Death (NO Stun, Untrackable, Immune)*/, 98456 /*98456 (Allari the Souleater) - Glaive Anim Replacement*/, 98457 /*98457 (Cyana Nightglaive) - Glaive Anim Replacement*/, 93762 /*93762 (Legion Devastator)*/, 98459 /*98459 (Kayn Sunfury) - Glaive Anim Replacement*/, 98460 /*98460 (Kor'vas Bloodthorn) - Glaive Anim Replacement*/, 98458 /*98458 (Jace Darkweaver) - Glaive Anim Replacement*/, 99919 /*99919 (Sevis Brightflame) - Glaive Anim Replacement*/, 99918 /*99918 (Sevis Brightflame) - Demon Hunter Glide State*/, 98290 /*98290 (Cyana Nightglaive) - Demon Hunter Glide State*/, 98227 /*98227 (Allari the Souleater) - Demon Hunter Glide State*/, 98228 /*98228 (Jace Darkweaver)*/, 94744 /*94744 (Dread Felbat)*/, 99218 /*99218 (Legion Devastator Missile)*/, 94492 /*94492 (Colossal Infernal)*/, 98191 /*98191 (Legion Command Center) - Legion Command Center: Engine Plume*/, 100161 /*100161 (Legion Devastator)*/);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(65183, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '9036 9617'), -- 65183 (Spirit Healer) - Ghost, Ghost Visual
(97634, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '192604'), -- 97634 (Spire of Woe) - Before We're Overrun: Spire of Woe - Area Trigger
(97629, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '192604'), -- 97629 (Spire of Woe) - Before We're Overrun: Spire of Woe - Area Trigger
(95048, 0, 0, 0, 0, 0, 1, 0, 0, 9809, 0, 0, 3, '188658'), -- 95048 (Brood Queen Tyranna) - Before We're Overrun: Brood Queen Tyranna's Teleport
(93221, 0, 0, 0, 0, 0, 1, 0, 0, 1615, 0, 0, 3, ''), -- 93221 (Doom Commander Beliash)
(101696, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '200604'), -- 101696 (Shadow Portal) - Shadow Portal
(105316, 0, 0, 0, 3, 0, 1, 0, 0, 6761, 0, 0, 0, ''), -- 105316 (Questioner Arev'naal)
(99003, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 4, ''), -- 99003 (Fel Cannon)
(94410, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 94410 (Allari the Souleater)
(99916, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 99916 (Sevis Brightflame)
(98354, 0, 0, 0, 0, 0, 1, 0, 0, 9125, 0, 0, 0, ''), -- 98354 (Kor'vas Bloodthorn)
(98229, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 98229 (Kayn Sunfury)
(100745, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 5, ''), -- 100745 (Fel Cannon)
(102714, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '188560'), -- 102714 (Eredar Summoner) - Eredar Summoner: Fel Channel Omni
(95046, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '188560'), -- 95046 (Eredar Summoner) - Eredar Summoner: Fel Channel Omni
(100931, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 5, ''), -- 100931 (Fel Cannon)
(99351, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 5, ''), -- 99351 (Legion Devastator Missile)
(96731, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 5, ''), -- 96731 (Legion Devastator)
(100510, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 5, '197836 197843'), -- 100510 (The Fel Hammer) - The Fel Hammer Beam Event: Bottom Bunny Fel Orb & Grow Aura, The Fel Hammer Beam Event: Shrink Aura
(97592, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, '159474'), -- 97592 (Felguard Sentry) - Permanent Feign Death (NO Stun, Untrackable, Immune)
(98456, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 98456 (Allari the Souleater) - Glaive Anim Replacement
(98457, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 98457 (Cyana Nightglaive) - Glaive Anim Replacement
(93762, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 5, ''), -- 93762 (Legion Devastator)
(98459, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 98459 (Kayn Sunfury) - Glaive Anim Replacement
(98460, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 98460 (Kor'vas Bloodthorn) - Glaive Anim Replacement
(98458, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 98458 (Jace Darkweaver) - Glaive Anim Replacement
(99919, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 99919 (Sevis Brightflame) - Glaive Anim Replacement
(99918, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '199303'), -- 99918 (Sevis Brightflame) - Demon Hunter Glide State
(98290, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '199303'), -- 98290 (Cyana Nightglaive) - Demon Hunter Glide State
(98227, 0, 0, 0, 0, 0, 1, 0, 0, 9180, 0, 0, 0, '199303'), -- 98227 (Allari the Souleater) - Demon Hunter Glide State
(98228, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 98228 (Jace Darkweaver)
(94744, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 94744 (Dread Felbat)
(99218, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 5, ''), -- 99218 (Legion Devastator Missile)
(94492, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 5, ''), -- 94492 (Colossal Infernal)
(98191, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 5, '217773'), -- 98191 (Legion Command Center) - Legion Command Center: Engine Plume
(100161, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 5, ''); -- 100161 (Legion Devastator)

UPDATE `creature_template_addon` SET `StandState`=7 WHERE `entry`=96159; -- 96159 (Colossal Infernal)
UPDATE `creature_template_addon` SET `auras`='188485' WHERE `entry`=93759; -- 93759 (Jace Darkweaver) - Fel Channelling
UPDATE `creature_template_addon` SET `AnimTier`=3, `auras`='' WHERE `entry`=94654; -- 94654 (Doomguard Eradicator)
UPDATE `creature_template_addon` SET `auras`='204360' WHERE `entry`=93105; -- 93105 (Inquisitor Baleful) - Cosmetic - Soulgorged
UPDATE `creature_template_addon` SET `auras`='197845 197844 197833' WHERE `entry`=37490; -- 37490 (ELM General Purpose Bunny Infinite) - The Fel Hammer Beam Event: Top Bunny Orb Aura, The Fel Hammer Beam Event: Top Bunny Grow Aura, The Fel Hammer Beam Event: Fel Beam from Top to Bottom Bunny
UPDATE `creature_template_addon` SET `auras`='188485' WHERE `entry`=98497; -- 98497 (Imp Mother) - Fel Channelling

SET @ORD_INDEX := 0;
DELETE FROM `spell_target_position` WHERE `ID` IN (191666,194689);
INSERT INTO `spell_target_position` (`ID`, `EffectIndex`, `OrderIndex`, `MapID`, `PositionX`, `PositionY`, `PositionZ`, `VerifiedBuild`) VALUES
(191666, 0, @ORD_INDEX+0, 1481, 1394.907958984375, 2112.986083984375, 103.6813735961914062, 70575), -- Spell: 191666 (The Gauntlet Event: Devastator Bombardment to Random StringID 01) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(191666, 0, @ORD_INDEX+1, 1481, 1408.6007080078125, 2103.060791015625, 106.203216552734375, 70575), -- Spell: 191666 (The Gauntlet Event: Devastator Bombardment to Random StringID 01) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(191666, 0, @ORD_INDEX+2, 1481, 1384.0086669921875, 2315.4150390625, 66.87717437744140625, 70575), -- Spell: 191666 (The Gauntlet Event: Devastator Bombardment to Random StringID 01) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(191666, 0, @ORD_INDEX+3, 1481, 1375.5399169921875, 2299.088623046875, 70.510009765625, 70575), -- Spell: 191666 (The Gauntlet Event: Devastator Bombardment to Random StringID 01) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(191666, 0, @ORD_INDEX+4, 1481, 1352.5625, 2256.038330078125, 82.77567291259765625, 70575), -- Spell: 191666 (The Gauntlet Event: Devastator Bombardment to Random StringID 01) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(191666, 0, @ORD_INDEX+5, 1481, 1363.3489990234375, 2174.973876953125, 94.10528564453125, 70575), -- Spell: 191666 (The Gauntlet Event: Devastator Bombardment to Random StringID 01) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(191666, 0, @ORD_INDEX+6, 1481, 1352.5521240234375, 2239.732666015625, 86.102569580078125, 70575), -- Spell: 191666 (The Gauntlet Event: Devastator Bombardment to Random StringID 01) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(191666, 0, @ORD_INDEX+7, 1481, 1379.611083984375, 2140.74658203125, 98.66510009765625, 70575), -- Spell: 191666 (The Gauntlet Event: Devastator Bombardment to Random StringID 01) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(191666, 0, @ORD_INDEX+8, 1481, 1358.2708740234375, 2268.921875, 79.73892974853515625, 70575), -- Spell: 191666 (The Gauntlet Event: Devastator Bombardment to Random StringID 01) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(191666, 0, @ORD_INDEX+9, 1481, 1365.9097900390625, 2283.376708984375, 74.903594970703125, 70575), -- Spell: 191666 (The Gauntlet Event: Devastator Bombardment to Random StringID 01) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(191666, 0, @ORD_INDEX+10, 1481, 1353.6771240234375, 2223.854248046875, 88.50164031982421875, 70575), -- Spell: 191666 (The Gauntlet Event: Devastator Bombardment to Random StringID 01) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(191666, 0, @ORD_INDEX+11, 1481, 1371.0399169921875, 2158.6962890625, 96.01561737060546875, 70575), -- Spell: 191666 (The Gauntlet Event: Devastator Bombardment to Random StringID 01) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(191666, 0, @ORD_INDEX+12, 1481, 1359.68408203125, 2189.9619140625, 92.39209747314453125, 70575), -- Spell: 191666 (The Gauntlet Event: Devastator Bombardment to Random StringID 01) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(191666, 0, @ORD_INDEX+13, 1481, 1354.763916015625, 2209.670166015625, 90.08298492431640625, 70575), -- Spell: 191666 (The Gauntlet Event: Devastator Bombardment to Random StringID 01) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(191666, 0, @ORD_INDEX+14, 1481, 1387.0103759765625, 2127.989501953125, 101.0248336791992187, 70575), -- Spell: 191666 (The Gauntlet Event: Devastator Bombardment to Random StringID 01) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+0, 1481, 1513.498291015625, 2455.939208984375, 55.3132476806640625, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+1, 1481, 1562.2899169921875, 2503.30810546875, 59.31312942504882812, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+2, 1481, 1506.2413330078125, 2516.8759765625, 50.83713531494140625, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+3, 1481, 998.54864501953125, 3060.858642578125, -8.29151248931884765, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+4, 1481, 935.34893798828125, 3059.848876953125, -26.3552227020263671, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+5, 1481, 1053.1771240234375, 3095.494873046875, 0.067860893905162811, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+6, 1481, 1109.4583740234375, 2888.80126953125, 4.693957328796386718, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+7, 1481, 1170.78125, 3075.639892578125, -15.8303422927856445, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+8, 1481, 1073.3316650390625, 2853.779541015625, 8.37862396240234375, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+9, 1481, 1553.48095703125, 2606.73779296875, 41.30150985717773437, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+10, 1481, 1247.0989990234375, 2546.873291015625, 2.749742031097412109, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+11, 1481, 1412.845458984375, 2366.703125, 57.92909622192382812, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+12, 1481, 1512.810791015625, 2536.657958984375, 49.51139068603515625, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+13, 1481, 1409.8333740234375, 2351.135498046875, 61.4071044921875, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+14, 1481, 1307.203125, 2389.510498046875, 24.01291847229003906, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+15, 1481, 1274.515625, 2558.704833984375, 8.507220268249511718, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+16, 1481, 987.22052001953125, 2472.3837890625, -78.32171630859375, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+17, 1481, 739.52081298828125, 2434.80029296875, -64.3459625244140625, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+18, 1481, 766.86114501953125, 2513.549560546875, -67.964111328125, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+19, 1481, 839.21875, 2557.779541015625, -55.9799079895019531, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+20, 1481, 738.38543701171875, 2530.279541015625, -69.7242584228515625, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+21, 1481, 784.9461669921875, 2424.28125, -58.9811592102050781, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+22, 1481, 717.71527099609375, 2607.296142578125, -66.4299392700195312, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+23, 1481, 945.83160400390625, 2846.47314453125, 0.637464940547943115, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+24, 1481, 942.109375, 2941.791748046875, -16.612588882446289, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+25, 1481, 881.515625, 2994.1015625, -32.1582145690917968, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+26, 1481, 880.63018798828125, 3109.01904296875, -45.9910507202148437, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+27, 1481, 861.97918701171875, 2907.064208984375, -23.680826187133789, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+28, 1481, 1111.6597900390625, 2994.490478515625, -8.29366588592529296, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+29, 1481, 928.46527099609375, 2917.9619140625, -17.6563987731933593, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+30, 1481, 1553.7222900390625, 2657.18310546875, 25.50534820556640625, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+31, 1481, 1477.3992919921875, 2459.619873046875, 49.789306640625, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+32, 1481, 1530.857666015625, 2515.361083984375, 54.15007400512695312, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+33, 1481, 1358.3385009765625, 2378.37158203125, 42.60752105712890625, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+34, 1481, 1341.6007080078125, 2355.388916015625, 40.18031692504882812, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+35, 1481, 1193.4149169921875, 2567.606689453125, -7.89273309707641601, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+36, 1481, 1342.654541015625, 2577.87060546875, 17.60508537292480468, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+37, 1481, 1078.7291259765625, 2535.4931640625, -31.4035282135009765, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+38, 1481, 801.72052001953125, 2462.982666015625, -58.3619308471679687, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+39, 1481, 783.9375, 2458.078125, -60.1597557067871093, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+40, 1481, 760.15625, 2572.349853515625, -70.6289901733398437, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+41, 1481, 687.7725830078125, 2412.73095703125, -67.128570556640625, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+42, 1481, 709.11981201171875, 2466.053955078125, -67.3068466186523437, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+43, 1481, 887.89239501953125, 3008.6796875, -25.0802249908447265, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+44, 1481, 846.11456298828125, 3000.718017578125, -39.7993736267089843, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+45, 1481, 1113.484375, 2941.296875, -3.664154052734375, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+46, 1481, 1134.329833984375, 2944.600830078125, -10.4341077804565429, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+47, 1481, 1530.1944580078125, 2544.151123046875, 51.57106399536132812, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+48, 1481, 1329.3663330078125, 2386.505126953125, 33.90662002563476562, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+49, 1481, 1343.703125, 2395.819580078125, 37.33985519409179687, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+50, 1481, 1485.236083984375, 2494.810791015625, 47.77035903930664062, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+51, 1481, 846.607666015625, 2462.17529296875, -52.8284912109375, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+52, 1481, 681.1163330078125, 2433.626708984375, -71.4885406494140625, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+53, 1481, 818.27777099609375, 2451.954833984375, -62.8465461730957031, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+54, 1481, 762.23614501953125, 2475.263916015625, -61.6805496215820312, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+55, 1481, 1567.9913330078125, 2643.78466796875, 26.9732666015625, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+56, 1481, 1575.1927490234375, 2612.82373046875, 34.36604690551757812, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+57, 1481, 1562.189208984375, 2483.673583984375, 60.43822479248046875, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+58, 1481, 1372.5208740234375, 2362.25, 48.74998092651367187, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+59, 1481, 1321.4097900390625, 2362.354248046875, 31.21900367736816406, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+60, 1481, 807.04168701171875, 2420.947998046875, -56.5805854797363281, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+61, 1481, 807.76043701171875, 2481.239501953125, -60.5551643371582031, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+62, 1481, 929.59552001953125, 2572.92529296875, -61.1286964416503906, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+63, 1481, 826.73614501953125, 2509.869873046875, -60.7680816650390625, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+64, 1481, 711.19097900390625, 2632.670166015625, -74.7857437133789062, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+65, 1481, 901.04168701171875, 2947.298583984375, -22.5855884552001953, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+66, 1481, 878.1875, 2943.85498046875, -26.1398372650146484, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+67, 1481, 1123.6163330078125, 3032.861083984375, -16.8724784851074218, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+68, 1481, 1014.64410400390625, 3085.3203125, -1.72079682350158691, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+69, 1481, 1328.9635009765625, 2414.994873046875, 30.77369499206542968, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+70, 1481, 1550.8541259765625, 2436.329833984375, 71.89698028564453125, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+71, 1481, 1162.123291015625, 2563.509521484375, -15.5945758819580078, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+72, 1481, 1183.3836669921875, 2558.607666015625, -9.55035400390625, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+73, 1481, 1294.82470703125, 2545.423583984375, 13.12560081481933593, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+74, 1481, 1071.298583984375, 2518.4375, -31.1496143341064453, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+75, 1481, 1032.3507080078125, 2522.5634765625, -41.6789970397949218, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+76, 1481, 629.65625, 2356.302001953125, -72.4207839965820312, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+77, 1481, 718.99652099609375, 2645.7509765625, -63.6153221130371093, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+78, 1481, 867.55206298828125, 2881.643310546875, -24.8716564178466796, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+79, 1481, 924.5225830078125, 2940.91845703125, -18.602304458618164, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+80, 1481, 1021.10418701171875, 3108.67626953125, 3.903302192687988281, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+81, 1481, 1062.638916015625, 3083.2666015625, -3.1349797248840332, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+82, 1481, 1103.12158203125, 2923.8994140625, -1.76480233669281005, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+83, 1481, 1155.203125, 2554.7900390625, -16.5259227752685546, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+84, 1481, 778.94097900390625, 2558.13720703125, -70.4904251098632812, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+85, 1481, 972.59722900390625, 2546.27001953125, -48.3863258361816406, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+86, 1481, 907.1336669921875, 2929.9853515625, -19.7672405242919921, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+87, 1481, 913.98089599609375, 2955.608642578125, -20.6687698364257812, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+88, 1481, 1057.579833984375, 3051.140625, -8.59151554107666015, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+89, 1481, 1207.232666015625, 3134.490478515625, -16.0787925720214843, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+90, 1481, 1374.6961669921875, 2589.236083984375, 20.60826492309570312, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+91, 1481, 1355.7066650390625, 2555.186767578125, 18.32872200012207031, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+92, 1481, 1210.263916015625, 2561.661376953125, -3.19313693046569824, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+93, 1481, 805.08160400390625, 2514.453125, -57.0215644836425781, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+94, 1481, 631.9375, 2372.052001953125, -72.2847061157226562, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+95, 1481, 949.703125, 2916.288330078125, -12.9387788772583007, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+96, 1481, 1532.52783203125, 2537.580810546875, 52.13272857666015625, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+97, 1481, 1544.90283203125, 2558.1796875, 51.4364013671875, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+98, 1481, 1456.1875, 2413.657958984375, 55.54119491577148437, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+99, 1481, 1109.2135009765625, 2542.0400390625, -25.6874580383300781, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+100, 1481, 791.23614501953125, 2440.04345703125, -58.5238227844238281, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+101, 1481, 825.95660400390625, 2535.078125, -58.3251686096191406, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+102, 1481, 730.09722900390625, 2496.069580078125, -67.28521728515625, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+103, 1481, 900.03643798828125, 2893.223876953125, -20.0816287994384765, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+104, 1481, 1017.63018798828125, 3019.80810546875, -8.37801837921142578, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+105, 1481, 1343.920166015625, 2420.85595703125, 36.339996337890625, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+106, 1481, 1424.6510009765625, 2392.3056640625, 55.22384262084960937, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+107, 1481, 1379.920166015625, 2383.78466796875, 49.24224472045898437, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+108, 1481, 961.75, 2496.979248046875, -70.3758621215820312, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+109, 1481, 724.3819580078125, 2456.859375, -65.4786529541015625, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+110, 1481, 713.328125, 2435.786376953125, -67.0481948852539062, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+111, 1481, 882.09893798828125, 2891.2265625, -22.7100772857666015, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+112, 1481, 884.86114501953125, 2957.177978515625, -27.5064411163330078, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+113, 1481, 917.79168701171875, 2997.849853515625, -20.7937831878662109, 70575), -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(194689, 0, @ORD_INDEX+114, 1481, 733.28125, 2580.479248046875, -69.3423385620117187, 70575); -- Spell: 194689 (Fel Bombardment) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)

UPDATE `creature_equip_template` SET `VerifiedBuild`=70575 WHERE (`ID`=1 AND `CreatureID` IN (93112,94705,94704,93105,96400,97592,94654,102714,95046,98622,98611,93221,93759,97712,94410,99916,98354,98229,98484,98486,96877,98456,98457,98459,98460,98458,99919,94580,99918,98292,98290,98228,98227,93011)) OR (`ID`=2 AND `CreatureID`=93112);

UPDATE `quest_offer_reward` SET `VerifiedBuild`=70575 WHERE `ID`=40077;
UPDATE `quest_poi` SET `VerifiedBuild`=70575 WHERE (`QuestID`=39279 AND `BlobIndex`=0 AND `Idx1`=5) OR (`QuestID`=39279 AND `BlobIndex`=0 AND `Idx1`=4) OR (`QuestID`=39279 AND `BlobIndex`=0 AND `Idx1`=3) OR (`QuestID`=39279 AND `BlobIndex`=0 AND `Idx1`=2) OR (`QuestID`=39279 AND `BlobIndex`=1 AND `Idx1`=1) OR (`QuestID`=39279 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=40378 AND `BlobIndex`=0 AND `Idx1`=4) OR (`QuestID`=40378 AND `BlobIndex`=0 AND `Idx1`=3) OR (`QuestID`=40378 AND `BlobIndex`=0 AND `Idx1`=2) OR (`QuestID`=40378 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=40378 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=40077 AND `BlobIndex`=0 AND `Idx1`=3) OR (`QuestID`=40077 AND `BlobIndex`=0 AND `Idx1`=2) OR (`QuestID`=40077 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=40077 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=31927 AND `BlobIndex`=0 AND `Idx1`=4) OR (`QuestID`=31927 AND `BlobIndex`=0 AND `Idx1`=3) OR (`QuestID`=31927 AND `BlobIndex`=0 AND `Idx1`=2) OR (`QuestID`=31927 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=31927 AND `BlobIndex`=0 AND `Idx1`=0);
UPDATE `quest_poi_points` SET `VerifiedBuild`=70575 WHERE (`QuestID`=39279 AND `Idx1`=5 AND `Idx2`=0) OR (`QuestID`=39279 AND `Idx1`=4 AND `Idx2`=0) OR (`QuestID`=39279 AND `Idx1`=3 AND `Idx2`=0) OR (`QuestID`=39279 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=39279 AND `Idx1`=1 AND `Idx2`=11) OR (`QuestID`=39279 AND `Idx1`=1 AND `Idx2`=10) OR (`QuestID`=39279 AND `Idx1`=1 AND `Idx2`=9) OR (`QuestID`=39279 AND `Idx1`=1 AND `Idx2`=8) OR (`QuestID`=39279 AND `Idx1`=1 AND `Idx2`=7) OR (`QuestID`=39279 AND `Idx1`=1 AND `Idx2`=6) OR (`QuestID`=39279 AND `Idx1`=1 AND `Idx2`=5) OR (`QuestID`=39279 AND `Idx1`=1 AND `Idx2`=4) OR (`QuestID`=39279 AND `Idx1`=1 AND `Idx2`=3) OR (`QuestID`=39279 AND `Idx1`=1 AND `Idx2`=2) OR (`QuestID`=39279 AND `Idx1`=1 AND `Idx2`=1) OR (`QuestID`=39279 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=39279 AND `Idx1`=0 AND `Idx2`=11) OR (`QuestID`=39279 AND `Idx1`=0 AND `Idx2`=10) OR (`QuestID`=39279 AND `Idx1`=0 AND `Idx2`=9) OR (`QuestID`=39279 AND `Idx1`=0 AND `Idx2`=8) OR (`QuestID`=39279 AND `Idx1`=0 AND `Idx2`=7) OR (`QuestID`=39279 AND `Idx1`=0 AND `Idx2`=6) OR (`QuestID`=39279 AND `Idx1`=0 AND `Idx2`=5) OR (`QuestID`=39279 AND `Idx1`=0 AND `Idx2`=4) OR (`QuestID`=39279 AND `Idx1`=0 AND `Idx2`=3) OR (`QuestID`=39279 AND `Idx1`=0 AND `Idx2`=2) OR (`QuestID`=39279 AND `Idx1`=0 AND `Idx2`=1) OR (`QuestID`=39279 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=40378 AND `Idx1`=4 AND `Idx2`=0) OR (`QuestID`=40378 AND `Idx1`=3 AND `Idx2`=0) OR (`QuestID`=40378 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=40378 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=40378 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=40077 AND `Idx1`=3 AND `Idx2`=0) OR (`QuestID`=40077 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=40077 AND `Idx1`=1 AND `Idx2`=11) OR (`QuestID`=40077 AND `Idx1`=1 AND `Idx2`=10) OR (`QuestID`=40077 AND `Idx1`=1 AND `Idx2`=9) OR (`QuestID`=40077 AND `Idx1`=1 AND `Idx2`=8) OR (`QuestID`=40077 AND `Idx1`=1 AND `Idx2`=7) OR (`QuestID`=40077 AND `Idx1`=1 AND `Idx2`=6) OR (`QuestID`=40077 AND `Idx1`=1 AND `Idx2`=5) OR (`QuestID`=40077 AND `Idx1`=1 AND `Idx2`=4) OR (`QuestID`=40077 AND `Idx1`=1 AND `Idx2`=3) OR (`QuestID`=40077 AND `Idx1`=1 AND `Idx2`=2) OR (`QuestID`=40077 AND `Idx1`=1 AND `Idx2`=1) OR (`QuestID`=40077 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=40077 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=31927 AND `Idx1`=4 AND `Idx2`=0) OR (`QuestID`=31927 AND `Idx1`=3 AND `Idx2`=0) OR (`QuestID`=31927 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=31927 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=31927 AND `Idx1`=0 AND `Idx2`=0);
UPDATE `quest_details` SET `VerifiedBuild`=70575 WHERE `ID` IN (40378, 40077);

DELETE FROM `quest_request_items` WHERE `ID`=40378;
INSERT INTO `quest_request_items` (`ID`, `EmoteOnComplete`, `EmoteOnIncomplete`, `EmoteOnCompleteDelay`, `EmoteOnIncompleteDelay`, `CompletionText`, `VerifiedBuild`) VALUES
(40378, 0, 0, 0, 0, '$n, I remind you that we need our Ashtongue forces to be brought through the gateway back on Despair Ridge.', 70575); -- Enter the Illidari: Ashtongue

DELETE FROM `ui_map_quest_line` WHERE (`UIMapId`=672 AND `QuestLineId` IN (5891,5890,5889,5888,5887,5866,5839,5821,5819,5814,5813,5795,5691,5673,336));
INSERT INTO `ui_map_quest_line` (`UIMapId`, `QuestLineId`, `VerifiedBuild`) VALUES
(672, 5891, 70575),
(672, 5890, 70575),
(672, 5889, 70575),
(672, 5888, 70575),
(672, 5887, 70575),
(672, 5866, 70575),
(672, 5839, 70575),
(672, 5821, 70575),
(672, 5819, 70575),
(672, 5814, 70575),
(672, 5813, 70575),
(672, 5795, 70575),
(672, 5691, 70575),
(672, 5673, 70575),
(672, 336, 70575);

UPDATE `ui_map_quest_line` SET `VerifiedBuild`=70575 WHERE (`UIMapId`=672 AND `QuestLineId` IN (5519,912,906,788,787,341,339,310,294,287,286,285,284,279,247,211,206,205,202,201,200,161,160,159,154,138,136,135,133,132,90));
UPDATE `creature_queststarter` SET `VerifiedBuild`=70575 WHERE (`id`=98229 AND `quest`=40378) OR (`id`=93011 AND `quest`=40077);
UPDATE `creature_questender` SET `VerifiedBuild`=70575 WHERE (`id`=94410 AND `quest`=40378) OR (`id`=98229 AND `quest`=40077);

DELETE FROM `gameobject_template_addon` WHERE `entry` IN (244691 /*Small Treasure Chest*/, 246249 /*Small Treasure Chest*/, 253931 /*Creature_Demoncrystal_03_fel*/, 245316 /*Small Treasure Chest*/, 246486 /*Spire of Woe*/, 246356 /*7LG_Legion_FloorRune02 (Purple) [Large]*/, 246485 /*Spire of Woe*/, 244689 /*Small Treasure Chest*/, 250560 /*Legion Banner*/, 259043 /*Legion Banner 02*/, 246309 /*Small Treasure Chest*/);
INSERT INTO `gameobject_template_addon` (`entry`, `faction`, `flags`, `WorldEffectID`, `AIAnimKitID`) VALUES
(244691, 0, 0x204000, 2437, 0), -- Small Treasure Chest
(246249, 0, 0x204000, 2437, 0), -- Small Treasure Chest
(253931, 114, 0x0, 0, 0), -- Creature_Demoncrystal_03_fel
(245316, 0, 0x0, 2437, 0), -- Small Treasure Chest
(246486, 114, 0x20, 0, 0), -- Spire of Woe
(246356, 114, 0x2000, 0, 0), -- 7LG_Legion_FloorRune02 (Purple) [Large]
(246485, 114, 0x20, 0, 0), -- Spire of Woe
(244689, 0, 0x204000, 2437, 0), -- Small Treasure Chest
(250560, 0, 0x40001, 0, 0), -- Legion Banner
(259043, 114, 0x0, 0, 0), -- Legion Banner 02
(246309, 0, 0x204000, 2437, 0); -- Small Treasure Chest

UPDATE `gameobject_template_addon` SET `flags`=0x22 WHERE `entry`=246567; -- Spire of Woe B
UPDATE `gameobject_template_addon` SET `flags`=0x22 WHERE `entry`=246566; -- Spire of Woe A

UPDATE `conversation_line_template` SET `VerifiedBuild`=70575 WHERE `Id` IN (1684, 1683, 1511, 2289, 2288, 2287, 2529, 1446, 1445);
UPDATE `conversation_line_template` SET `ChatType`=1, `VerifiedBuild`=70575 WHERE `Id` IN (2938, 1780, 1779);

DELETE FROM `conversation_template` WHERE `Id`=584;
INSERT INTO `conversation_template` (`Id`, `FirstLineID`, `TextureKitId`, `VerifiedBuild`) VALUES
(584, 1516, 0, 70575);

UPDATE `conversation_template` SET `VerifiedBuild`=70575 WHERE `Id`=747;
UPDATE `conversation_template` SET `VerifiedBuild`=70575 WHERE `Id`=581;
UPDATE `conversation_template` SET `VerifiedBuild`=70575 WHERE `Id`=558;
UPDATE `conversation_template` SET `VerifiedBuild`=70575 WHERE `Id`=922;
UPDATE `conversation_template` SET `VerifiedBuild`=70575 WHERE `Id`=705;

DELETE FROM `conversation_actors` WHERE (`ConversationId`=584 AND `Idx` IN (1,0));
INSERT INTO `conversation_actors` (`ConversationId`, `ConversationActorId`, `Idx`, `CreatureId`, `CreatureDisplayInfoId`, `NoActorObject`, `ActivePlayerObject`, `VerifiedBuild`) VALUES
(584, 49947, 1, 93127, 61698, 0, 0, 70575),
(584, 49935, 0, 95048, 65935, 0, 0, 70575);

UPDATE `conversation_actors` SET `VerifiedBuild`=70575 WHERE (`Idx`=0 AND `ConversationId` IN (705,581,922,747,558)) OR (`Idx`=2 AND `ConversationId`=922) OR (`Idx`=1 AND `ConversationId` IN (922,747));

DELETE FROM `conversation_line_template` WHERE `Id` IN (1518, 1517, 1516);
INSERT INTO `conversation_line_template` (`Id`, `UiCameraID`, `ActorIdx`, `Flags`, `ChatType`, `VerifiedBuild`) VALUES
(1518, 118, 1, 0, 0, 70575),
(1517, 262, 0, 0, 0, 70575),
(1516, 262, 0, 0, 0, 70575);

UPDATE `areatrigger_create_properties` SET `VerifiedBuild`=70575 WHERE (`Id`=6659 AND `IsCustom`=0);
UPDATE `areatrigger_create_properties` SET `VerifiedBuild`=70575 WHERE (`Id`=5151 AND `IsCustom`=0); -- Spell: 195051 (Beaming Gaze)
UPDATE `areatrigger_create_properties` SET `VerifiedBuild`=70575 WHERE (`Id`=2104 AND `IsCustom`=0); -- Spell: 192665 (Legion Aegis)
UPDATE `areatrigger_create_properties` SET `VerifiedBuild`=70575 WHERE (`Id`=4920 AND `IsCustom`=0); -- Spell: 192604 (Before We're Overrun: Spire of Woe - Area Trigger)
UPDATE `areatrigger_create_properties` SET `VerifiedBuild`=70575 WHERE (`Id`=3680 AND `IsCustom`=0);

UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=9850 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=11231 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=10350 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=10355 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=9877 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=6669 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=9905 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=9637 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=8352 AND `IsCustom`=0);

DELETE FROM `areatrigger_create_properties` WHERE (`IsCustom`=0 AND `Id` IN (5127,5639,5647,5177));
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`, `ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `SpellForVisuals`, `PositionalSoundKitId`, `TimeToTargetScale`, `Speed`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`, `ShapeData5`, `ShapeData6`, `ShapeData7`, `Roll`, `Pitch`, `Yaw`, `TargetRoll`, `TargetPitch`, `TargetYaw`, `VerifiedBuild`) VALUES
(5127, 0, 9850, 0, 0, 0, 0, 0, 0, -1, 0, 0, NULL, 198306, 5000, 0, 0, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, 70575), -- Spell: 198306 (Fel Beam)
(5639, 0, 10350, 0, 0, 0, 0, 0, 0, -1, 0, 0, NULL, 200417, 6000, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, 70575), -- Spell: 200417 (Foul Fel)
(5647, 0, 10355, 0, 0, 0, 0, 0, 0, -1, 0, 0, NULL, 200510, 10000, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, 70575), -- Spell: 200510 (Anguished Soul)
(5177, 0, 9905, 0, 0, 0, 0, 0, 0, -1, 0, 0, NULL, 195402, 20000, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, 70575); -- Spell: 195402 (Shadow Retreat)

UPDATE `spawn_tracking_template` SET `VerifiedBuild`=70575 WHERE `SpawnTrackingId` IN (1053493, 1053492, 1053491, 1071452, 1027851, 1110294, 1084853, 1029001, 1077247, 1073202, 672538, 722283, 722024, 721274, 721248);
UPDATE `spawn_tracking_quest_objective` SET `VerifiedBuild`=70575 WHERE (`SpawnTrackingId`=1053493 AND `QuestObjectiveId`=279937) OR (`SpawnTrackingId`=1053492 AND `QuestObjectiveId`=279936) OR (`SpawnTrackingId`=1053491 AND `QuestObjectiveId`=279935) OR (`SpawnTrackingId`=1027851 AND `QuestObjectiveId`=281030) OR (`SpawnTrackingId`=1110294 AND `QuestObjectiveId`=281156) OR (`SpawnTrackingId`=1084853 AND `QuestObjectiveId`=280768) OR (`SpawnTrackingId`=1077247 AND `QuestObjectiveId`=280292);

