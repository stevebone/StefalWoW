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
-- NPC: 548 Murloc Minor Tidecaller
-- NPC: 578 Murloc Scout
-- NPC: 580 Redridge Drudger
-- NPC: 584 Kazon
-- NPC: 615 Blackrock Tracker
-- NPC: 711 Ardo Dirtpaw
-- NPC: 712 Redridge Thrasher
-- NPC: 1083 Murlock Shorestriker
-- NPC: 4064 Blackrock Scout
-- NPC: 4462 Blackrock Hunter
-- NPC: 4463 Blackrock Summoner
-- NPC: 7013 Blackrock Guard
-- NPC: 14270 Squiddic
-- NPC: 14271 Ribchaser
-- NPC: 14273 Boulderheart
-- NPC: 43041 Ol' Gummers
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
-- NPC: 43532 Muckdweller
-- NPC: 43533 Blackrock Drake Rider
-- NPC: 43535 Blackrock Warden
-- NPC: 147222 Gnollfeaster

-- NPC: 43081 Guard Bateman
-- NPC: 43270 Messner (in cage)
-- NPC: 43272 Jorgensen (in cage)
-- NPC: 43274 Krakauer Captured
-- NPC: 43275 Danforth Captured
-- NPC: 43300 Messner spawned
-- NPC: 43305 Jorgensen spawned
-- NPC: 43303 Krakauer spawned
-- NPC: 43302 Danforth spawned
-- NPC: 43450 Keeshan Riverboat
-- NPC: 43448 Messner (boat)
-- NPC: 43447 Jorgensen (boat)
-- NPC: 43446 Krakauer (boat)
-- NPC: 43445 Danforth (boat)
-- NPC: 43449 Keeshan (boat)
-- NPC: 43458 Keeshan (camp)
-- NPC: 43462 Danforth (camp)
-- NPC: 43461 Krakauer (camp)
-- NPC: 43460 Jorgensen (camp)
-- NPC: 43459 Messner (camp)
-- NPC: 43508 Brubaker

-- Quest: 26512 Tuning The Gnomecorder
-- Quest: 26545 Yowler Must Die!
-- Quest: 26567 John J. Keeshan
-- Quest: 26568 This Ain't My War
-- Quest: 26571 Weapons of War
-- Quest: 26607 They Drew First Blood
-- Quest: 26616 Its Never Over
-- Quest: 26586 In Search of Bravo Company
-- Quest: 26587 Breaking Out is Hard to Do
-- Quest: 26560 Jorgensen
-- Quest: 26561 Krakauer
-- Quest: 26562 And Last But Not Least... Danforth
-- Quest: 26616 It's Never Over
-- Quest: 26639 Point of Contact: Brubaker
-- Quest: 26636 Bravo Company Field Kit: Camouflage
-- Quest: 26637 Bravo Company Field Kit: Chloroform
-- Quest: 26638 Hunting the Hunters

-- Spell: 81003 Apply Quest Invis Zone 5
-- Spell: 81004 Detect: Quest Invis Zone 5
-- Spell: 80893 Summon Messner
-- Spell: 81009 Apply Quest Invis Zone 6
-- Spell: 81010 Detect: Quest Invis Zone 6
-- Spell: 80940 Summon Jorgensen
-- Spell: 81018 Apply Quest Invis Zone 7
-- Spell: 81019 Detect: Quest Invis Zone 7
-- Spell: 80941 Summon Krakauer
-- Spell: 81079 Apply Quest Invis Zone 8
-- Spell: 81080 Detect: Quest Invis Zone 8
-- Spell: 80943 Summon Danforth
-- Spell: 81243 Summon Keeshan Riverboat
-- Spell: 81265 Riverboat Quest Credit
-- Spell: 81201 Apply Quest Invis Zone 9
-- Spell: 81202 Detect: Quest Invis Zone 9
-- Spell: 75038 Freeze Anim

-- Spell Area
DELETE FROM `spell_area` WHERE `spell` = 81004 AND `area` = 97;
DELETE FROM `spell_area` WHERE `spell` = 81010 AND `area` = 996;
DELETE FROM `spell_area` WHERE `spell` = 81019 AND `area` = 998;
DELETE FROM `spell_area` WHERE `spell` = 81080 AND `area` = 998;
DELETE FROM `spell_area` WHERE `spell` = 81202 AND `area` = 5326;
INSERT INTO `spell_area` (`spell`, `area`, `quest_start`, `quest_start_status`, `quest_end_status`, `quest_end`, `aura_spell`, `racemask`, `gender`, `flags`) VALUES
(81004, 97, 0, 0, 43, 26587, 0, 0, 2, 3),
(81010, 996, 26560, 10, 43, 26560, 0, 0, 2, 3),
(81019, 998, 26561, 10, 43, 26561, 0, 0, 2, 3),
(81080, 998, 26562, 10, 43, 26562, 0, 0, 2, 3),
(81202, 5326, 26616, 66, 0, 0, 0, 0, 2, 1); -- Quest 26616 complete and onwards

DELETE FROM `spell_area` WHERE `spell` IN (80893,80940,80941,80943) AND `area` = 44;
INSERT INTO `spell_area` (`spell`, `area`, `quest_start`, `quest_start_status`, `quest_end_status`, `quest_end`, `aura_spell`, `racemask`, `gender`, `flags`) VALUES
(80893, 44, 26587, 64, 43, 26563, 0, 0, 2, 3),
(80940, 44, 26560, 64, 43, 26563, 0, 0, 2, 3),
(80941, 44, 26561, 64, 43, 26563, 0, 0, 2, 3),
(80943, 44, 26562, 64, 43, 26563, 0, 0, 2, 3);

-- Spell Position
DELETE FROM `spell_target_position` WHERE `id` = 81243;
INSERT INTO `spell_target_position` (`ID`, `EffectIndex`, `MapID`, `PositionX`, `PositionY`, `PositionZ`, `VerifiedBuild`) VALUES
(81243, 0, 0, -9305.59, -2369.9, 56.1616, 24015);

-- NPC Spell Click
DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` = 43450;
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES
(43450, 46598, 1, 0);

-- Vehicles
DELETE FROM `vehicle_template_accessory` WHERE `entry` = 43450;
INSERT INTO `vehicle_template_accessory` (`entry`, `accessory_entry`, `seat_id`, `minion`, `summontype`, `summontimer`, `RideSpellID`, `description`) VALUES
(43450, 43448, 1, 1, 8, 0, NULL, 'Keeshan Riverboat - Messner'),
(43450, 43449, 2, 1, 8, 0, NULL, 'Keeshan Riverboat - Keeshan'),
(43450, 43447, 3, 1, 8, 0, NULL, 'Keeshan Riverboat - Jorgensen'),
(43450, 43446, 4, 1, 8, 0, NULL, 'Keeshan Riverboat - Krakauer'),
(43450, 43445, 5, 1, 8, 0, NULL, 'Keeshan Riverboat - Danforth');

DELETE FROM `vehicle_seat_addon` WHERE `SeatEntry` IN (8216,8217,8218);
INSERT INTO `vehicle_seat_addon` (`SeatEntry`, `SeatOrientation`, `ExitParamX`, `ExitParamY`, `ExitParamZ`, `ExitParamO`, `ExitParamValue`) VALUES
(8216, 3.14159265, 0, 0, 0, 0, 0),
(8217, 3.14159265, 0, 0, 0, 0, 0),
(8218, 3.14159265, 0, 0, 0, 0, 0);

-- Quest Template Addons
UPDATE `quest_template_addon` SET `PrevQuestID` = 26607 WHERE `ID` = 26616;
UPDATE `quest_template_addon` SET `ExclusiveGroup` = 0 WHERE `ID` = 26563;
UPDATE `quest_template_addon` SET `ScriptName` = 'quest_26563_return_of_the_bravo_company' WHERE `ID` = 26563;
UPDATE `quest_template_addon` SET `SourceSpellID` = 82005 WHERE `ID` = 26646;


DELETE FROM `quest_template_addon` WHERE `ID` IN (26567,26571,26586,26636,26637,26638);
INSERT INTO `quest_template_addon` (`ID`, `PrevQuestID`) VALUES 
(26571, 26568),
(26567, 26545),
(26586, 26573),
(26636, 26616),
(26637, 26616),
(26638, 26616);

-- Script Names
UPDATE `creature_template` SET `ScriptName` = 'npc_guard_bateman' WHERE `entry` = 43081;
UPDATE `creature_template` SET `ScriptName` = 'npc_messner' WHERE `entry` = 43270;
UPDATE `creature_template` SET `ScriptName` = 'npc_blackrock_battle_worg' WHERE `entry` = 43340;
UPDATE `creature_template` SET `ScriptName` = 'npc_spawned_messner' WHERE `entry` = 43300;
UPDATE `creature_template` SET `ScriptName` = 'npc_jorgensen' WHERE `entry` = 43272;
UPDATE `creature_template` SET `ScriptName` = 'npc_spawned_jorgensen' WHERE `entry` = 43305;
UPDATE `creature_template` SET `ScriptName` = 'npc_danforth_captured' WHERE `entry` = 43275;
UPDATE `creature_template` SET `ScriptName` = 'npc_spawned_krakauer' WHERE `entry` = 43303;
UPDATE `creature_template` SET `ScriptName` = 'npc_spawned_danforth' WHERE `entry` = 43302;
UPDATE `creature_template` SET `ScriptName` = 'npc_keeshan_riverboat' WHERE `entry` = 43450;
UPDATE `gameobject_template` SET `ScriptName` = 'go_chain_lever' WHERE `entry` = 204403;

-- Template Fixes
UPDATE `creature_template` SET `VehicleId` = 964, `IconName` = 'vehichleCursor', `npcflag` = 16777216 WHERE `Entry` = 43450;

-- Area Trigger
DELETE FROM `areatrigger_involvedrelation` WHERE `id` = 6034;
INSERT INTO `areatrigger_involvedrelation` (`id`, `quest`) VALUES 
(6034, 26512);

DELETE FROM `areatrigger_scripts` WHERE `entry` = 682;
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES
(682, 'at_lakeshire_inn_682');

DELETE FROM `areatrigger_scripts` WHERE `entry` = 6079;
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES
(6079, 'at_camp_everstill_6079');

-- Spell Scripts
DELETE FROM `spell_script_names` WHERE `spell_id` = 82580 AND `ScriptName` = 'spell_bravo_company_field_kit';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(82580, 'spell_bravo_company_field_kit');

-- Creature Difficulties
-- Remove incorrect records
DELETE FROM `creature_template_difficulty` WHERE `DifficultyID` = 1 AND `Entry` IN (
	345,422,423,426,428,430,437,442,445,446,518,545,547,548,578,580,584,615,711,712,1083,
	4064,4462,4463,7013,14270,14271,14273,43041,43083,43094,43183,43185,43327,43329,43340,43341,
	43350,43363,43369,43532,43533,43535,147222
);

-- Adjust Damage Modifier
UPDATE `creature_template_difficulty` SET `DamageModifier` = 0.2 WHERE `Entry` IN (
	345,422,423,426,428,430,437,442,445,446,518,545,547,548,578,580,584,615,711,712,1083,
	4064,4462,4463,7013,14270,14271,14273,43041,43083,43094,43183,43185,43327,43329,43340,43341,
	43350,43363,43369,43532,43533,43535,147222
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
UPDATE `creature_template_difficulty` SET `LootID` = 43350, `GoldMin` = 600, `GoldMax` = 700 WHERE `Entry` = 43350;
UPDATE `creature_template_difficulty` SET `LootID` = 7013, `GoldMin` = 2000, `GoldMax` = 2500 WHERE `Entry` = 7013;
UPDATE `creature_template_difficulty` SET `LootID` = 43535, `GoldMin` = 3000, `GoldMax` = 4000 WHERE `Entry` = 43535;
UPDATE `creature_template_difficulty` SET `LootID` = 43041 WHERE `Entry` = 43041;
UPDATE `creature_template_difficulty` SET `StaticFlags4` = 0 WHERE `Entry` = 43532;
UPDATE `creature_template_difficulty` SET `StaticFlags1` = `StaticFlags1` | 0x20000100 WHERE `Entry` = 43508;

-- Swim flag
UPDATE `creature_template_difficulty` SET `StaticFlags1` = `StaticFlags1` | 0x10000000 WHERE `Entry` IN (43183,43041,43532);

-- Floating flag
UPDATE `creature_template_difficulty` SET `StaticFlags1` = `StaticFlags1` | 0x20000000 WHERE `Entry` IN (43450,43533);

-- Creature Template Addons
UPDATE `creature_template_addon` SET `auras` = '80815 393433' WHERE `Entry` = 43248;
UPDATE `creature_template_addon` SET `auras` = '81201' WHERE `Entry` IN (43458,43459,43460,43461,43462);
UPDATE `creature_template_addon` SET `StandState` = 8 WHERE `Entry` = 43458;
-- this is not the right spell id/emote for Brubaker but close enough. May be done via an anim kit
UPDATE `creature_template_addon` SET `auras` = '75038', `emote` = 420 WHERE `Entry` = 43508;

-- SAI
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (147222);
DELETE FROM smart_scripts WHERE entryorguid IN (147222) AND source_type = 0;
DELETE FROM smart_scripts WHERE entryorguid IN (426) AND source_type = 0 AND id IN (1,2);
DELETE FROM smart_scripts WHERE entryorguid IN (430,580) AND source_type = 0 AND id IN (2,3);
DELETE FROM smart_scripts WHERE entryorguid IN (445,446,423,712) AND source_type = 0 AND id = 1;
DELETE FROM smart_scripts WHERE entryorguid IN (711) AND source_type = 0 AND id = 2;
DELETE FROM smart_scripts WHERE entryorguid IN (7013) AND source_type = 0 AND id = 4;
DELETE FROM smart_scripts WHERE entryorguid IN (43535) AND source_type = 0 AND id = 5;
INSERT INTO smart_scripts (entryorguid, source_type, id, link, Difficulties, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string, target_type, target_param1, target_param2, target_param3, target_param4, target_param_string, target_x, target_y, target_z, target_o, comment) VALUES
(7013, 0, 4, 0, '', 61, 0, 30, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 1, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Blackrock Guard - On Aggro - Say Line 0 (No Repeat)'),
(43535, 0, 5, 0, '', 4, 0, 30, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 1, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Blackrock Warden - On Aggro - Say Line 0 (No Repeat)'),


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

UPDATE `smart_scripts` SET `link` = 4 WHERE `entryorguid` = 7013 AND `source_type` = 0 AND `id` = 3;



-- Creatures
SET @CGUID := 900000;
DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID+3294 AND @CGUID+3300;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `MovementType`) VALUES 
(@CGUID+3294, 147222, 0, 44, 1001, 0, -9732.28, -2104.18, 59.7424, 5.13223, 3600, 0, 0),

(@CGUID+3295, 43458, 0, 44, 5326, 1, -9465.2, -2827.6, 65.2785, 0.65487, 180, 0, 0),
(@CGUID+3296, 43459, 0, 44, 5326, 1, -9463.28, -2829.31, 65.2789, 2.10001, 180, 0, 0),
(@CGUID+3297, 43460, 0, 44, 5326, 1, -9462.12, -2826.4, 65.2759, 3.32837, 180, 0, 0),
(@CGUID+3298, 43461, 0, 44, 5326, 1, -9463.3, -2824.85, 65.2785, 4.32189, 180, 0, 0),
(@CGUID+3299, 43462, 0, 44, 5326, 1, -9465.57, -2825.34, 65.2785, 5.5047, 180, 0, 0),

(@CGUID+3300, 43508, 0, 44, 44, 0, -9676.09, -2842, 53.5381, 0.070987, 180, 0, 0);

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
    568,43084,433,429,712,43083,43183,578,545,43532
);

UPDATE creature SET MovementType = 1, wander_distance = 3
WHERE id IN (
    580,43185
);

