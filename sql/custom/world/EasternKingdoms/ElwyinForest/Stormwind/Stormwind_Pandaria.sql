-- Phase: 1135 Cosmetic - Stormwind/Elwynn - Aysa
-- Phase: 1136 Cosmetic - Stormwind/Elwynn - Jojo
-- Phase: 1137 Cosmetic - Stormwind/Elwynn - Balloon
-- Phase: 1138 Cosmetic - Ling Completion - See Scroll
-- Phase: 1139 Cosmetic - Stormwind Keep - See Varian

-- Spells: 120344 Summon Aysa (Stormwind Gate)
-- Spells: 120345 Summon Jojo (Stormwind Gate)
-- Spells: 120352 Summon Varian (also triggers the spawn of Jojo/Aysa see above)
-- Spells: 83773 Conversation Trigger 01 (Instant No Delay)
-- Spells: 88811 CSA AT Dummy Timer (30s)
-- Spells: 116601 Timer Aura (5s)

-- AT: 7995 Stormwind Bridge (middle)
-- AT: 5828 Stormwind Bridge (Marshall)
-- AT: 7989 Stormwind City Entrance (Josie & Marty)
-- AT: 7996 Stormwind City (Trade District Center)
-- AT: 7990 Stormwind City (Trade District Moni npc)
-- AT: 7991 Stormwind City (Trade District > Canal)
-- AT: 7992 Stormwind City (Castle 1st stairs)
-- AT: 7993 Stormwind City (Canals Bridge)
-- AT: 7994 Stormwind City (Canals Brunn Goldenmug)

-- Quest: 30987 Joining the Alliance
-- Quest: 30988 The Alliance Way

-- NPC: 62419 Aysa Stormwind Monk Trainer
-- NPC: 60565 Shang Xi's Hot Air Balloon
-- NPC: 61792 Aysa spawned at Stormwind Gate
-- NPC: 61793 Jojo spawned at Stormwind Gate
-- NPC: 60566 Aysa Stormwind Gate
-- NPC: 60567 Jojo Stormwind Gate
-- NPC: 61834 Alyn Black
-- NPC: 61836 Moni Widdlesprock
-- NPC: 61837 Leria Nightwind
-- NPC: 61838 Gavin Marlsbury
-- NPC: 61839 Lucas Severing
-- NPC: 61840 Naanae
-- NPC: 61841 Brunn Goldenmug
-- NPC: 61895 Marty
-- NPC: 61896 Josie
-- NPC: 29611 King Varian
-- NPC: 61796 King Varian (spawned)

-- Phases
DELETE FROM `phase_area` WHERE `PhaseId` IN (1135,1136,1137,1139);
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(1617, 1135, 'Stormwind - Valley of Heroes see Aysa'),
(7486, 1135, 'Elwynn Forest - Stormwind Gate see Aysa'),
(1617, 1136, 'Stormwind - Valley of Heroes see Jojo'),
(7486, 1136, 'Elwynn Forest - Stormwind Gate see Jojo'),
(1617, 1137, 'Stormwind - Valley of Heroes see Balloon'),
(7486, 1137, 'Elwynn Forest - Stormwind Gate see Balloon'),
(6292, 1139, 'Stormwind Keep - see King Varian');

-- Phase Conditions
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` IN (1135,1136,1137,1139);
INSERT INTO `conditions` VALUES
-- Stormwind Gate
(26, 1135, 0, 0, 0, 47, 0, 31450, 74, 0, '', 0, 0, 0, '', 'Stormwind Gate - Phase 1135 active if 31450 taken, in progress, complete, rewarded'),
(26, 1136, 0, 0, 0, 47, 0, 31450, 74, 0, '', 0, 0, 0, '', 'Stormwind Gate - Phase 1136 active if 31450 taken, in progress, complete, rewarded'),
(26, 1137, 0, 0, 0, 47, 0, 31450, 74, 0, '', 0, 0, 0, '', 'Stormwind Gate - Phase 1135 active if 31450 taken, in progress, complete, rewarded'),
(26, 1135, 0, 0, 0, 47, 0, 30987, 1, 0, '', 0, 0, 0, '', 'Stormwind Gate - Phase 1135 active if 30987 NOT taken'),
(26, 1136, 0, 0, 0, 47, 0, 30987, 1, 0, '', 0, 0, 0, '', 'Stormwind Gate - Phase 1136 active if 30987 NOT taken'),
(26, 1137, 0, 0, 0, 47, 0, 30987, 64, 0, '', 1, 0, 0, '', 'Stormwind Gate - Phase 1137 active if 30987 NOT rewarded'),
(26, 1139, 0, 0, 0, 47, 0, 30987, 66, 0, '', 0, 0, 0, '', 'Stormwind Gate - Phase 1139 active if 30987 complete or rewarded'),
(26, 1139, 0, 0, 0, 47, 0, 30988, 66, 0, '', 1, 0, 0, '', 'Stormwind Gate - Phase 1139 active if 30987 complete or rewarded');

-- On quest accept cast Summon Aysa spell on player
UPDATE `quest_template_addon` SET `SourceSpellID` = 120344 WHERE `ID` = 30987;
-- Quest Template Addons
DELETE FROM `quest_template_addon` WHERE `ID` IN (30988);
INSERT INTO `quest_template_addon` VALUES
(30988, 0, 0, 0, 30987, 30989, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'quest_30988_the_alliance_way');

DELETE FROM `creature_queststarter` WHERE `quest` IN (30988);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES 
('29611', '30988', '0');


-- Spell Conditions
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=13 AND `SourceEntry` IN (120344,120345);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(13, 1, 120344, 0, 0, 51, 0, 5, 60566, 0, 0, 0, 0, '', 'Summon Aysa at Stormwind Gate > Aysa'),
(13, 1, 120345, 0, 0, 51, 0, 5, 60567, 0, 0, 0, 0, '', 'Summon Jojo at Stormwind Gate > Jojo');

-- Update teleport spell location for Stormwind Gate
DELETE FROM `spell_target_position` WHERE `ID` IN (116957);
INSERT INTO `spell_target_position` VALUES
('116957', '0', 0, '0', '-9063.70', '434.73', '93.055', '0.6744', 0),
('116957', '1', 0, '0', '-9063.70', '434.73', '93.055', '0.6744', 0);

-- AT scripts
DELETE FROM `areatrigger_scripts` WHERE `entry` IN (5828,7989, 7990,7991,7992,7993,7994,7995,7996);
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES 
(5828, 'at_stormwind_valley_of_heroes_5828'),
(7989, 'at_stormwind_entrance_7989'),
(7990, 'at_stormwind_trade_district_7990'),
(7991, 'at_stormwind_canals_7991'),
(7992, 'at_stormwind_keep_7992'),
(7993, 'at_stormwind_canals_7993'),
(7994, 'at_stormwind_canals_7994'),
(7995, 'at_stormwind_valley_of_heroes_7995'),
(7996, 'at_stormwind_trade_district_center_7996');

-- Creature Templates
UPDATE `creature_template` SET `ScriptName` = 'npc_king_varian_61796' WHERE `entry` = 61796;

DELETE FROM `creature_template_difficulty` WHERE `entry` IN (466,60567,60566,60565,61792,61793);
INSERT INTO `creature_template_difficulty` VALUES
(466, 0, 0, 0, 864, 11, 10, 1, 1, 4.6, 313, 0, 0, 0, 466, 0, 0, 451, 594, 0, 0, 0, 0, 0, 0, 0, 0, -1),
(61792, 0, 0, 0, 864, 11, 1, 1, 1, 1, 19012, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(61793, 0, 0, 0, 864, 11, 1, 1, 1, 1, 19010, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60565, 0, 0, 0, 864, 11, 30, 1, 1, 0.3, 21146, 1073741824, 0, 0, 0, 0, 0, 0, 0, 805306368, 0, 262144, 0, 0, 0, 0, 0, 56647),
(60566, 0, 0, 0, 864, 11, 1, 1, 1, 1, 21145, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60567, 0, 0, 0, 864, 11, 1, 1, 1, 1, 21141, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647);

DELETE FROM `creature_template_addon` WHERE `entry` IN (60567,60566,60565,61792,61793);
INSERT INTO `creature_template_addon` VALUES
(61792, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Aysa Spawned at Stormwind Gate
(61793, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '115672'), -- Jojo Spawned at Stormwind Gate
(60565, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- Shang Xi's Hot Air Balloon
(60566, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Aysa
(60567, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '115672'); -- Jojo

-- SmartAI
 -- Aysa Cloudsinger smart ai
SET @ENTRY := 61792;
UPDATE `creature_template` SET `AIName` = 'SmartAI', `ScriptName` = '' WHERE `entry` = @ENTRY;
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` = @ENTRY;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`, `Difficulties`) VALUES
(@ENTRY, 0, 0, 1, 11, 0, 100, 0, 0, 0, 0, 0, 0, 29, 0, 0, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'On respawn - Self: Follow Owner by distance 0, angle 0', ''),
(@ENTRY, 0, 1, 2, 61, 0, 100, 0, 0, 0, 0, 0, 0, 44, 1135, 0, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'On respawn - Owner/Summoner: Remove phase id Cosmetic - Stormwind/Elwynn - Aysa (1135)', ''),
(@ENTRY, 0, 2, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'On respawn - Self: Talk 0 to Owner/Summoner', '');

 -- Jojo smart ai
SET @ENTRY := 61793;
UPDATE `creature_template` SET `AIName` = 'SmartAI', `ScriptName` = '' WHERE `entry` = @ENTRY;
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` = @ENTRY;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`, `Difficulties`) VALUES
(@ENTRY, 0, 0, 1, 11, 0, 100, 0, 0, 0, 0, 0, 0, 29, 3, 0, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'On respawn - Self: Follow Owner by distance 3, angle 0', ''),
(@ENTRY, 0, 1, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 44, 1136, 0, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'On respawn - Owner/Summoner: Remove phase id Cosmetic - Stormwind/Elwynn - Jojo (1136)', '');

-- Creature Text
DELETE FROM creature_text WHERE `CreatureID` IN (61792,61793,61836,61834,61837,61841,61838, 61839,61840,61895,61896,61796);
DELETE FROM creature_text WHERE `CreatureID` = 68 AND `GroupID` = 6;
DELETE FROM creature_text WHERE `CreatureID` = 466 AND `GroupID` IN (1,2,3,4);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES 
-- Aysa spawned at Stormwind Gate
(61792, 0, 0, 'Let''s get going, $p. If we are going to be of any use out here, we are going to need allies.', 12, 0, 100, 0, 0, 27389, 0, 61421, 0, 'Aysa Spawned at Stormwind Gate'),
(61792, 1, 0, 'We''re here to join the Alliance. We seek audience with your Emperor.', 12, 0, 100, 2, 5000, 0, 0, 61417, 0, 'Aysa Spawned at Stormwind Gate'),
(61792, 2, 0, 'Look at this!', 14, 0, 100, 5, 5000, 0, 0, 61422, 0, 'Aysa Spawned at Stormwind Gate'),
(61792, 3, 0, 'Well, that''s good news. Once we join the Alliance, we certainly won''t run out of things to do.', 12, 0, 100, 1, 5000, 0, 0, 61423, 0, 'Aysa Spawned at Stormwind Gate'),
(61792, 4, 0, 'Let''s keep moving.', 12, 0, 100, 1, 0, 0, 0, 61415, 0, 'Aysa Spawned at Stormwind Gate'),
(61792, 5, 0, 'Yes. Go on.', 12, 0, 100, 273, 0, 27390, 0, 61428, 0, 'Aysa Spawned at Stormwind Gate'),
(61792, 6, 0, 'Certainly.', 12, 0, 100, 273, 0, 27391, 0, 61433, 0, 'Aysa Spawned at Stormwind Gate'),
(61792, 7, 0, 'Yes... of course.', 12, 0, 100, 24, 0, 27392, 0, 61441, 0, 'Aysa Spawned at Stormwind Gate'),
-- Jojo spawned at Stormwind Gate
(61793, 0, 0, 'Let''s... just keep moving.', 12, 0, 100, 1, 0, 0, 0, 61409, 0, 'Jojo Spawned at Stormwind Gate'),
(61793, 1, 0, 'What''s the Light?', 12, 0, 100, 6, 0, 0, 0, 61416, 0, 'Jojo Spawned at Stormwind Gate'),
-- Stormwind Guard at Stormwind Bridge
(68, 6, 0, 'Never thought I''d see one of your kind walk through here.', 12, 0, 100, 66, 0, 0, 0, 61418, 0, 'Stormwind Guard at Stormwind Bridge'),
-- General Marcus Jonathan
(466, 1, 0, 'Pandaren? What business do you have in Stormwind?', 12, 0, 100, 1, 0, 0, 0, 61397, 0, 'General Marcus Jonathan at Stormwind Bridge'),
(466, 2, 0, 'Emperor? You''ll find KING Anduin inside Stormwind Keep.', 12, 0, 100, 1, 0, 0, 0, 61398, 0, 'General Marcus Jonathan at Stormwind Bridge'),
(466, 3, 0, '<%s eyes Jojo''s pillar.>', 16, 0, 100, 1, 3000, 0, 0, 61399, 0, 'General Marcus Jonathan at Stormwind Bridge'),
(466, 4, 0, 'Behave yourselves inside Stormwind, pandaren. You''re in Alliance territory now.', 12, 0, 100, 397, 0, 0, 0, 61400, 0, 'General Marcus Jonathan at Stormwind Bridge'),
-- Moni
(61836, 0, 0, 'Teacher... look!', 12, 0, 100, 25, 5000, 0, 0, 61404, 0, 'Moni Widdlesprock at Stormwind Trade District'),
(61836, 1, 0, 'Pandaren? Here, in Stormwind?', 12, 0, 100, 6, 5000, 0, 0, 61406, 0, 'Moni Widdlesprock at Stormwind Trade District'),
-- Alyn Black
(61834, 0, 0, 'Yes. Those are pandaren, Moni.', 12, 0, 100, 1, 5000, 0, 0, 61405, 0, 'Alyn Black at Stormwind Trade District'),
-- Leria Nightwind
(61837, 0, 0, 'Welcome to Stormwind!', 12, 0, 100, 3, 0, 0, 0, 61410, 0, 'Leria Nightwind at Stormwind Canals Bridge'),
-- Brunn Goldenmug
(61841, 0, 0, 'Pandaren? I mussht be seein'' thingssh...', 12, 0, 100, 0, 0, 0, 0, 61411, 0, 'Brunn Goldenmug at Stormwind Canals'),
-- Gavin Marlsbury
(61838, 0, 0, 'Heh... pandaren. You must be here to join the Alliance.', 12, 0, 100, 0, 0, 0, 0, 61407, 0, 'Gaving Marlsbury at Stormwind Canals'),
(61838, 1, 0, 'Good move. It worked out well for me.', 12, 0, 100, 0, 0, 0, 0, 61408, 0, 'Gaving Marlsbury at Stormwind Canals'),
-- Lucas Severing
(61839, 0, 0, 'Quiet, Naanae. Outsiders approach.', 12, 0, 100, 1, 0, 0, 0, 61412, 0, 'Lucas Severing at Stormwind Keep'),
-- Naanae
(61840, 0, 0, 'Outsiders, yes - but their intentions are good.', 12, 0, 100, 1, 0, 0, 0, 61413, 0, 'Naanae at Stormwind Keep'),
(61840, 1, 0, 'Look carefully, Lucas. The Light is strong with them. Particularly that one.', 12, 0, 100, 1, 0, 0, 0, 61414, 0, 'Naanae at Stormwind Keep'),
-- Marty
(61895, 0, 0, 'Hey look! Gnolls!', 12, 0, 100, 1, 0, 0, 0, 61401, 0, 'Marty at Stormwind City entrance'),
-- Josie
(61896, 0, 0, 'Those aren''t gnolls, silly. Use your eyes.', 12, 0, 100, 1, 0, 0, 0, 61402, 0, 'Josie at Stormwind City entrance'),
(61896, 1, 0, 'Those are FURBOLGS.', 12, 0, 100, 1, 0, 0, 0, 61403, 0, 'Josie at Stormwind City entrance'),
-- King Varian
(61796, 0, 0, 'Walk with me. All three of you.', 12, 0, 100, 1, 0, 29574, 0, 61424, 0, 'King Varian Spawned in Stormwind Keep'),
(61796, 1, 0, 'I understand you want to join the Alliance.', 12, 0, 100, 0, 0, 29575, 0, 61425, 0, 'King Varian Spawned in Stormwind Keep'),
(61796, 2, 0, 'We always have need of allies. I''d be a fool to turn you away, especially at a time of war.', 12, 0, 100, 1, 0, 29576, 0, 61426, 0, 'King Varian Spawned in Stormwind Keep'),
(61796, 3, 0, 'But there are a number of things I need to make clear to you.', 12, 0, 100, 0, 0, 29577, 0, 61427, 0, 'King Varian Spawned in Stormwind Keep'),
(61796, 4, 0, 'First: the races of the Alliance look out for one another.', 12, 0, 100, 0, 0, 29578, 0, 61429, 0, 'King Varian Spawned in Stormwind Keep'),
(61796, 5, 0, 'You are expected to provide aid to Alliance members in need, whether they be human, gnome, draenei... or pandaren like yourselves.', 12, 0, 100, 1, 0, 29579, 0, 61430, 0, 'King Varian Spawned in Stormwind Keep'),
(61796, 6, 0, 'It is a simple guideline, but a meaningful one. Do you understand?', 12, 0, 100, 0, 0, 29580, 0, 61432, 0, 'King Varian Spawned in Stormwind Keep'),
(61796, 7, 0, 'Good. Because that brings me to my second point.', 12, 0, 100, 273, 0, 29581, 0, 61434, 0, 'King Varian Spawned in Stormwind Keep'),
(61796, 8, 0, 'Not all pandaren have chosen the same path as you three.', 12, 0, 100, 0, 0, 29582, 0, 61435, 0, 'King Varian Spawned in Stormwind Keep'),
(61796, 9, 0, 'Some have sided with the Horde.', 12, 0, 100, 0, 0, 29583, 0, 61436, 0, 'King Varian Spawned in Stormwind Keep'),
(61796, 10, 0, 'They, along with the other barbarian clans of the Horde, are your new enemies.', 12, 0, 100, 1, 0, 29584, 0, 61437, 0, 'King Varian Spawned in Stormwind Keep'),
(61796, 11, 0, 'Those who you once considered friends, or even those you might have loved, are now your sworn adversaries.', 12, 0, 100, 274, 0, 29585, 0, 61438, 0, 'King Varian Spawned in Stormwind Keep'),
(61796, 12, 0, 'I am deeply sorry, but the battle lines have been drawn. I will not tolerate any fraternizing with the enemy, as you could expose our Alliance to danger.', 12, 0, 100, 5, 0, 29586, 0, 61439, 0, 'King Varian Spawned in Stormwind Keep'),
(61796, 13, 0, 'Do I make myself clear?', 12, 0, 100, 0, 0, 29587, 0, 61440, 0, 'King Varian Spawned in Stormwind Keep'),
(61796, 14, 0, 'Very well. We are nearly finished, $p.', 12, 0, 100, 0, 0, 29588, 0, 61442, 0, 'King Varian Spawned in Stormwind Keep');

-- Spawns
SET @CGUID := 900810;

DELETE FROM `creature` WHERE `id` IN (60567,60566,60565,61839,61840,29611);
INSERT INTO `creature` VALUES
(@CGUID+0, '60565', '0', '12', '7486', '0', '0', '1137', '0', '-1', '0', '0', '-9084.53', '427.842', '92.5858', '0.521299', '300', '0', '0', '100', '0', NULL, NULL, NULL, '1048576', '', NULL, '0'),
(@CGUID+1, '60566', '0', '1519', '1617', '0', '0', '1135', '0', '-1', '0', '0', '-9061.02', '433.229', '93.0557', '0.705292', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0'),
(@CGUID+2, '60567', '0', '1519', '1617', '0', '0', '1136', '0', '-1', '0', '0', '-9065.02', '437.349', '93.0558', '0.550979', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0'),
(@CGUID+3, '61839', '0', '1519', '5390', '0', '0', '0', '0', '-1', '0', '1', '-8479', '392.182', '115.942', '5.96828', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '37474'),
(@CGUID+4, '61840', '0', '1519', '5390', '0', '0', '0', '0', '-1', '0', '1', '-8476.99', '391.53', '115.942', '2.82668', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '37474'),
(@CGUID+5, '29611', '0', '1519', '6292', '0', '0', '1139', '0', '-1', '0', '1', '-8362.34', '233.782', '156.991', '2.79159', '300', '0', '0', '100', '0', NULL, NULL, '2099200', NULL, '', NULL, '0');

-- Waypoint Data
DELETE FROM `waypoint_path` WHERE `PathId` IN (6179600);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Velocity`, `Comment`) VALUES 
('6179600', '0', '0', '0', 'Stormwind City - King Varian - The Alliance Way');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (6179600);
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `Delay`) VALUES 
(6179600, 1, -8369.53, 241.893, 155.328, 1.99772, 0),
(6179600, 2, -8368.91, 244.505, 155.347, 1.0287, 0),
(6179600, 3, -8364, 252.275, 155.347, 0.904774, 0),
(6179600, 4, -8354.36, 261.153, 155.347, 0.650098, 0),
(6179600, 5, -8344.61, 268.933, 155.347, 5.75023, 0),
(6179600, 6, -8328.42, 249.585, 155.347, 5.42983, 0),
(6179600, 7, -8321.03, 249.644, 155.347, 0.101599, 0),
(6179600, 8, -8312.62, 256.125, 155.347, 0.631743, 0),
(6179600, 9, -8302.02, 264.74, 155.347, 0.6329, 0),
(6179600, 10, -8284.91, 278.514, 155.347, 0.681181, 0),
(6179600, 11, -8279.14, 282.917, 155.347, 0.778777, 0),
(6179600, 12, -8279.43, 286.499, 155.347, 1.82567, 0),
(6179600, 13, -8283.64, 291.271, 155.347, 2.2662, 0),
(6179600, 14, -8291.48, 301.105, 155.347, 2.2297, 0),
(6179600, 15, -8297.76, 308.91, 155.347, 2.22416, 0),
(6179600, 16, -8309.79, 324.38, 155.347, 2.2051, 0),
(6179600, 17, -8314.17, 329.66, 155.347, 2.26909, 0),
(6179600, 18, -8318.16, 329.228, 155.347, 3.39176, 0),
(6179600, 19, -8326.47, 322.829, 155.347, 3.76887, 0),
(6179600, 20, -8339.91, 312.315, 155.347, 4.21609, 0),
(6179600, 21, -8334.28, 306.164, 157.347, 5.38865, 0),
(6179600, 22, -8327.68, 297.537, 156.832, 5.4263, 0),
(6179600, 23, -8318.99, 287.084, 156.832, 3.89098, 0);
