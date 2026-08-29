-- Redridge Mountains Fixes

-- NPC: 345 Bellygrub
-- NPC: 422 Murloc Flesheater
-- NPC: 423 Redridge Mongrel
-- NPC: 426 Redridge Brute
-- NPC: 428 Dire Condor
-- NPC: 430 Redridge Mystic
-- NPC: 437 Blackrock Renegade
-- NPC: 442 Tarantula
-- NPC: 445 Redridge Alpha
-- NPC: 446 Redridge Basher
-- NPC: 518 Yowler
-- NPC: 545 Murloc Tidecaller
-- NPC: 547 Great Goretusk
-- NPC: 578 Murloc Scout
-- NPC: 580 Redridge Drudger
-- NPC: 584 Kazon
-- NPC: 615 Blackrock Tracker
-- NPC: 711 Ardo Dirtpaw
-- NPC: 712 Redridge Thrasher
-- NPC: 4064 Blackrock Scout
-- NPC: 4463 Blackrock Summoner
-- NPC: 14270 Squiddic
-- NPC: 14271 Ribchaser
-- NPC: 14273 Boulderheart
-- NPC: 43083 Redridge Fox
-- NPC: 43094 Canyon Ettin
-- NPC: 43183 Freshwater Eel
-- NPC: 43185 Blackrock Overseer
-- NPC: 43327 Murdunk
-- NPC: 43329 Homurk
-- NPC: 43340 Blackrock Battle Worg
-- NPC: 43341 Blackrock Worg Captain
-- NPC: 43350 Utroka the Keymistress
-- NPC: 43363 Ritualist Tarak
-- NPC: 43369 Overlord Barbarius
-- NPC: 147222 Gnollfeaster

-- NPC: 43081 Guard Bateman

-- Quest: 26512 Tuning The Gnomecorder
-- Quest: 26545 Yowler Must Die!
-- Quest: 26567 John J. Keeshan
-- Quest: 26568 This Ain't My War
-- Quest: 26571 Weapons of War
-- Quest: 26607 They Drew First Blood
-- Quest: 26616 Its Never Over
-- Quest: 26586 In Search of Bravo Company
-- Quest: 26587 Breaking Out is Hard to Do

-- Spell: 81003 Apply Quest Invis Zone 5
-- Spell: 81004 Detect: Quest Invis Zone 5
-- Spell: 80893 Summon Messner

-- Spell Area
DELETE FROM `spell_area` WHERE `spell` = 81004 AND `area` = 97;
INSERT INTO `spell_area` (`spell`, `area`, `quest_start`, `quest_start_status`, `quest_end_status`, `quest_end`, `aura_spell`, `racemask`, `gender`, `flags`) VALUES
(81004, 97, 0, 0, 43, 26587, 0, 0, 2, 3);

DELETE FROM `spell_area` WHERE `spell` = 80893 AND `area` = 44;
INSERT INTO `spell_area` (`spell`, `area`, `quest_start`, `quest_start_status`, `quest_end_status`, `quest_end`, `aura_spell`, `racemask`, `gender`, `flags`) VALUES
(80893, 44, 26587, 64, 0, 0, 0, 0, 2, 1);

-- Quest Template Addons
UPDATE `quest_template_addon` SET `PrevQuestID` = 26607 WHERE `ID` = 26616;

DELETE FROM `quest_template_addon` WHERE `ID` IN (26567,26571,26586);
INSERT INTO `quest_template_addon` (`ID`, `PrevQuestID`) VALUES 
(26571, 26568),
(26567, 26545),
(26586, 26573);

-- Script Names
UPDATE `creature_template` SET `ScriptName` = 'npc_guard_bateman' WHERE `entry` = 43081;
UPDATE `creature_template` SET `ScriptName` = 'npc_messner' WHERE `entry` = 43270;
UPDATE `creature_template` SET `ScriptName` = 'npc_blackrock_battle_worg' WHERE `entry` = 43340;

-- Area Trigger
DELETE FROM `areatrigger_involvedrelation` WHERE `id` = 6034;
INSERT INTO `areatrigger_involvedrelation` (`id`, `quest`) VALUES 
(6034, 26512);

-- Creature Difficulties
-- Remove incorrect records
DELETE FROM `creature_template_difficulty` WHERE `DifficultyID` = 1 AND `Entry` IN (
	345,422,423,426,428,430,437,442,445,446,518,545,547,578,580,584,615,711,712,4064,4463,14270,14271,14273,43083,43094,
	43183,43185,43327,43329,43340,43341,43350,43363,43369,147222
);

-- Adjust Damage Modifier
UPDATE `creature_template_difficulty` SET `DamageModifier` = 0.2 WHERE `Entry` IN (
	345,422,423,426,428,430,437,442,445,446,518,545,547,578,580,584,615,711,712,4064,4463,14270,14271,14273,43083,43094,
	43183,43185,43327,43329,43340,43341,43350,43363,43369,147222
);

-- Add missing loot ids
UPDATE `creature_template_difficulty` SET `LootID` = 43083 WHERE `Entry` = 43083;
UPDATE `creature_template_difficulty` SET `LootID` = 147222 WHERE `Entry` = 147222;
UPDATE `creature_template_difficulty` SET `LootID` = 43183 WHERE `Entry` = 43183;
UPDATE `creature_template_difficulty` SET `LootID` = 14270 WHERE `Entry` = 14270;
UPDATE `creature_template_difficulty` SET `LootID` = 43185 WHERE `Entry` = 43185;
UPDATE `creature_template_difficulty` SET `StaticFlags1` = 524288, `LootID` = 43094, `GoldMin` = 400, `GoldMax` = 500 WHERE `Entry` = 43094;
UPDATE `creature_template_difficulty` SET `LootID` = 43363, `GoldMin` = 500, `GoldMax` = 600 WHERE `Entry` = 43363;
UPDATE `creature_template_difficulty` SET `LootID` = 43369, `GoldMin` = 500, `GoldMax` = 600 WHERE `Entry` = 43369;

-- Swim flag
UPDATE creature_template_difficulty SET StaticFlags1 = StaticFlags1 | 0x10000000 WHERE Entry IN (43183);

-- SAI
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (147222);
DELETE FROM smart_scripts WHERE entryorguid IN (147222) AND source_type = 0;
DELETE FROM smart_scripts WHERE entryorguid IN (423) AND source_type = 0 AND id = 1;
DELETE FROM smart_scripts WHERE entryorguid IN (426) AND source_type = 0 AND id IN (1,2);
DELETE FROM smart_scripts WHERE entryorguid IN (430) AND source_type = 0 AND id IN (2,3);
DELETE FROM smart_scripts WHERE entryorguid IN (445) AND source_type = 0 AND id = 1;
DELETE FROM smart_scripts WHERE entryorguid IN (446) AND source_type = 0 AND id = 1;
DELETE FROM smart_scripts WHERE entryorguid IN (580) AND source_type = 0 AND id IN (2,3);
DELETE FROM smart_scripts WHERE entryorguid IN (711) AND source_type = 0 AND id = 2;
DELETE FROM smart_scripts WHERE entryorguid IN (712) AND source_type = 0 AND id = 1;
INSERT INTO smart_scripts (entryorguid, source_type, id, link, Difficulties, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string, target_type, target_param1, target_param2, target_param3, target_param4, target_param_string, target_x, target_y, target_z, target_o, comment) VALUES
(147222, 0, 0, 0, '', 0, 0, 100, 0, 0, 1500, 3000, 5000, 0, '', 11, 265725, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Gnollfeaster - In combat (3/5 seconds) - Cast "Leeching Bite"'),
(147222, 0, 1, 0, '', 0, 0, 100, 0, 0, 1500, 5000, 7000, 0, '', 11, 265723, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Gnollfeaster - In combat (5/7 seconds) - Cast "Web"'),
(423, 0, 1, 0, '', 4, 0, 30, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 1, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Redridge Mongrel - On Aggro - Say Line 0 (No Repeat)'),
(426, 0, 1, 0, '', 4, 0, 30, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 1, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Redridge Brute - On Aggro - Say Line 0 (No Repeat)'),
(426, 0, 2, 0, '', 2, 0, 100, 1, 0, 15, 0, 0, 0, '', 1, 1, 0, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Redridge Brute - Flee at 15% HP'),
(430, 0, 2, 0, '', 4, 0, 30, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 1, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Redridge Mystic - On Aggro - Say Line 0 (No Repeat)'),
(430, 0, 3, 0, '', 4, 0, 30, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 1, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Redridge Mystic - On Aggro - Say Line 0 (No Repeat)'),
(445, 0, 1, 0, '', 4, 0, 30, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 1, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Redridge Alpha - On Aggro - Say Line 0 (No Repeat)'),
(446, 0, 1, 0, '', 4, 0, 30, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 1, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Redridge Basher - On Aggro - Say Line 0 (No Repeat)'),
(580, 0, 2, 0, '', 4, 0, 30, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 1, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Redridge Drudger - On Aggro - Say Line 0 (No Repeat)'),
(580, 0, 3, 0, '', 2, 0, 100, 1, 0, 15, 0, 0, 0, '', 1, 1, 0, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Redridge Drudger - Flee at 15% HP'),
(711, 0, 2, 0, '', 4, 0, 30, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 1, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Ardo Dirtpaw - On Aggro - Say Line 0 (No Repeat)'),
(712, 0, 1, 0, '', 4, 0, 30, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 1, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Redridge Thrasher - On Aggro - Say Line 0 (No Repeat)');

-- spell cast on aggro should target invoker not self... lol
UPDATE `smart_scripts` SET `target_type` = '7' WHERE `entryorguid` = 43341 AND `source_type` = 0 AND `id` = 0 AND `link` = 0;


-- Creature Text
DELETE FROM `creature_text` WHERE `creatureID` IN (43081,712,43270);
DELETE FROM `creature_text` WHERE `creatureID` IN (426,430,580) AND `GroupID` = 1;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(43081, 0, 0, 'Outgoing, $c.', 12, 7, 100, 0, 0, 0, 42986, 0, 'Guard Bateman to Player'),
(43081, 0, 1, 'Good hunting, $n!', 12, 7, 100, 0, 0, 0, 42987, 0, 'Guard Bateman to Player'),
(43081, 0, 2, '$R departing Redridge!', 12, 7, 100, 0, 0, 0, 42988, 0, 'Guard Bateman to Player'),
(43081, 0, 3, '$C departing Redridge!', 12, 7, 100, 0, 0, 0, 42989, 0, 'Guard Bateman to Player'),
(43081, 0, 4, 'Good luck out there, $n!', 12, 7, 100, 0, 0, 0, 42990, 0, 'Guard Bateman to Player'),
(712, 0, 0, 'More bones to gnaw on...', 12, 0, 100, 0, 0, 0, 1871, 0, 'Redridge Thrasher - Random Say on Aggro'),
(712, 0, 1, 'Grrrr... fresh meat!', 12, 0, 100, 0, 0, 0, 1870, 0, 'Redridge Thrasher - Random Say on Aggro'),
(426, 1, 0, '%s attempts to run away in fear!', 16, 0, 100, 0, 0, 0, 1150, 0, 'Redridge Brute - Flee Say 15 pct'),
(430, 1, 0, '%s attempts to run away in fear!', 16, 0, 100, 0, 0, 0, 1150, 0, 'Redridge Mystic - Flee Say 15 pct'),
(580, 1, 0, '%s attempts to run away in fear!', 16, 0, 100, 0, 0, 0, 1150, 0, 'Redridge Drudger - Flee Say 15 pct'),
(43270, 0, 0, 'If I ever get out of here I''m gonna crap all over your head, you pig sucking orc.', 12, 7, 100, 5, 0, 0, 43294, 0, 'Messner to Player'),
(43270, 1, 0, 'Where''s the rest of my crew??!', 12, 7, 100, 5, 0, 0, 43295, 0, 'Messner to Player'),
(43270, 2, 0, 'You think these bars can hold me? HAH!', 12, 7, 100, 5, 0, 0, 43296, 0, 'Messner to Player');

-- Creatures
SET @CGUID := 900000;
DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID+3294 AND @CGUID+3294;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `MovementType`) VALUES 
(@CGUID+3294, 147222, 0, 44, 1001, 0, -9732.28, -2104.18, 59.7424, 5.13223, 3600, 0, 0);

-- Creature Spawn Fixes
DELETE FROM `creature_addon` WHERE `guid` IN (334690,334613,334610,334572);
INSERT INTO `creature_addon` (`guid`, `emote`) VALUES 
(334572, 455),
(334690, 455),
(334613, 455),
(334610, 455);

UPDATE creature SET MovementType = 1, wander_distance = 8
WHERE id IN (
    442,423,426,1083,548,422,547,424,7013,43535,
    568,43084,433,429,712,43083,43183,578,545
);

UPDATE creature SET MovementType = 1, wander_distance = 3
WHERE id IN (
    580,43185
);

