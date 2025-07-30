-- Quest: 20666 The Sting of Learning
-- Quest: 29677 The Sun Pearl
-- NPC: 60411 Water Pincer
-- NPC: 55292 Fang-she
-- Quest: 29678 Shu, the Spirit of Water

-- NPC: 55015 Whitefeather Crane
-- NPC: 60411 Water Pincer
-- NPC: 54976 Barbed Ray

DELETE FROM `quest_template_addon` WHERE `ID` IN (20666, 29677, 29678);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`) VALUES
('20666', '0', '0', '0', '29676', '29677', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('29677', '0', '0', '0', '29676', '29678', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('29678', '0', '0', '0', '29677', '29679', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '1', '0');

UPDATE `creature_template_difficulty` SET `LootID` = '60411' WHERE (`Entry` = '60411') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '60411' WHERE (`Entry` = '60411') and (`DifficultyID` = '1');

DELETE FROM `creature` WHERE `id` = 55292;
INSERT INTO `creature` VALUES
('451089', '55292', '860', '0', '0', '0', '0', '169', '0', '-1', '0', '0', '952.151', '2941.5', '82.1681', '5.70723', '120', '5', '0', '100', '1', NULL, NULL, NULL, NULL, '', NULL, '0');

-- static flags1 values
-- 268435456 SWIM ENABLED
-- 268959744 SWIM ENABLED + AQUATIC
UPDATE `creature_template_difficulty` SET `LootID` = '55292', `SkinLootID` = 1, `StaticFlags1` = '268959744' WHERE (`Entry` = '55292') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '55292', `SkinLootID` = 1, `StaticFlags1` = '268959744' WHERE (`Entry` = '55292') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `LootID` = '54976', `StaticFlags1` = '268959744' WHERE (`Entry` = '54976') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '54976', `StaticFlags1` = '268959744' WHERE (`Entry` = '54976') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `LootID` = '55015' WHERE (`Entry` = '55015') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '55015' WHERE (`Entry` = '55015') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `LootID` = '60411' WHERE (`Entry` = '60411') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '60411' WHERE (`Entry` = '60411') and (`DifficultyID` = '1');

UPDATE `creature_template_difficulty` SET `StaticFlags1` = '268435456' WHERE (`Entry` = '55015') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '268435456' WHERE (`Entry` = '55015') and (`DifficultyID` = '1');

DELETE FROM `skinning_loot_template` WHERE `Entry` = 1;
INSERT INTO `skinning_loot_template` VALUES
('1', '0', '2318', '70', '0', '1', '1', '1', '3', 'Starting Zones'),
('1', '0', '2934', '75', '0', '1', '1', '1', '4', 'Starting Zones');

-- unit flags values
-- 33792 LOOT ENABLED + SWIM
-- 67142656 LOOT ENABLED + SKINNING + SWIM
UPDATE `creature_template` SET `unit_flags` = '33792', `AIName` = 'SmartAI' WHERE (`entry` = '54976');
UPDATE `creature_template` SET `unit_flags` = '33792', `AIName` = 'SmartAI' WHERE (`entry` = '55015');
UPDATE `creature_template` SET `unit_flags` = '67142656', `AIName` = 'SmartAI' WHERE (`entry` = '55292');
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '60411');

DELETE FROM smart_scripts WHERE entryorguid IN (55292,55015,54976,60411) AND source_type = 0;
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
    55292, 0, 0, 0,
    0, 0, 100, 0,               -- EVENT_TYPE = 0 (In Combat)
    5000, 7000, 5000, 7000, 0, '',
    11, 128409, 0, 0, 0, 0, 0, 0, '',   -- ACTION_TYPE = 11 (Cast Spell)
    2, 0, 0, 0, 0, '',           -- TARGET_TYPE = 2 (Current Target)
    0, 0, 0, 0,
    'Fang-she casts Serpent sting every 5-7 seconds'
),
(
    55015, 0, 0, 0,
    0, 0, 100, 0,               -- EVENT_TYPE = 0 (In Combat)
    5000, 7000, 5000, 7000, 0, '',
    11, 109088, 0, 0, 0, 0, 0, 0, '',   -- ACTION_TYPE = 11 (Cast Spell)
    2, 0, 0, 0, 0, '',           -- TARGET_TYPE = 2 (Current Target)
    0, 0, 0, 0,
    'Whitefeather Crane casts Razor Beak every 5-7 seconds'
),
(
    54976, 0, 0, 0,
    0, 0, 100, 0,               -- EVENT_TYPE = 0 (In Combat)
    5000, 7000, 5000, 7000, 0, '',
    11, 128407, 0, 0, 0, 0, 0, 0, '',   -- ACTION_TYPE = 11 (Cast Spell)
    2, 0, 0, 0, 0, '',           -- TARGET_TYPE = 2 (Current Target)
    0, 0, 0, 0,
    'Barbed Ray casts Poisoned Barb every 5-7 seconds'
),
(
    60411, 0, 0, 0,
    0, 0, 100, 0,               -- EVENT_TYPE = 0 (In Combat)
    5000, 7000, 5000, 7000, 0, '',
    11, 128448, 0, 0, 0, 0, 0, 0, '',   -- ACTION_TYPE = 11 (Cast Spell)
    2, 0, 0, 0, 0, '',           -- TARGET_TYPE = 2 (Current Target)
    0, 0, 0, 0,
    'Water Pincer casts Phlogiston every 5-7 seconds'
);

-- Loot tables for
-- Barbed Ray
-- Whitefeather crane
-- Water Pincer
-- Fang-she
DELETE FROM `creature_loot_template` WHERE `Entry` IN (55292,54976,55015,60411);
INSERT INTO `creature_loot_template` VALUES
(60411, 1, 11111, 1, 0, 1, 0, 1, 1, 'Water Pincer'),
(60411, 0, 56195, 100, 0, 1, 0, 1, 1, 'Water Pincer'),
(55292, 1, 10101, 2, 0, 1, 0, 1, 1, 'Fang-she'),
(55292, 0, 56234, 50, 0, 1, 0, 1, 1, 'Fang-she'),
(55292, 0, 56236, 50, 0, 1, 0, 1, 1, 'Fang-she');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54976,0,727,0.165975,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54976,0,768,0.0165975,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54976,0,805,0.0995851,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54976,0,828,0.0497925,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54976,0,1415,0.0165975,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54976,0,2140,0.0165975,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54976,0,2959,0.0829875,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54976,0,3189,0.033195,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54976,0,3190,0.0165975,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54976,0,5572,0.0165975,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54976,0,8181,0.165975,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54976,0,8182,0.0165975,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54976,0,9755,0.0165975,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54976,0,54632,0.0165975,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54976,0,54633,27.8008,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54976,0,54634,72.3485,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (54976,0,56234,0.116183,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,117,0.00779788,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,766,0.00779788,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,768,0.0311915,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,774,0.226139,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,805,0.062383,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,828,0.066282,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,1411,0.00389894,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,1413,0.00389894,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,1430,0.00389894,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,2589,0.0194947,0,1,0,2,2,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,2646,0.00389894,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,2959,0.00389894,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,3189,0.163756,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,3190,0.0194947,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,3214,0.00389894,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,4496,0.0467873,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,4560,0.0350905,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,4562,0.0428883,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,4563,0.00389894,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,4665,0.0389894,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,4757,10,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,4775,8.24236,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,5114,0.0311915,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,5115,0.00389894,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,5571,0.0740798,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,5572,0.0428883,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,6520,0.00389894,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,7096,0.0155958,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,7097,80.8328,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,7350,0.0389894,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,8179,0.00389894,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,8182,0.00779788,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,17056,1,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,55973,0.0857767,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55015,0,55983,0.120867,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,767,0.0888099,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,768,0.00986777,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,774,0.53286,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1411,0.187488,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1412,0.345372,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1413,0.236826,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1414,0.226959,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1415,0.44405,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1416,0.217091,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1417,0.256562,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1418,0.167752,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1419,0.197355,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1420,0.157884,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1422,0.118413,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1423,0.157884,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1425,0.236826,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1427,0.128281,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1429,0.17762,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1430,0.157884,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1431,0.207223,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,1433,0.0986777,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,2138,0.296033,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,2212,0.138149,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,2213,0.157884,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,2635,0.207223,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,2642,0.148017,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,2643,0.138149,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,2645,0.0986777,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,2646,0.0296033,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,2648,0.118413,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,2773,0.256562,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,2774,0.374975,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,2960,0.00986777,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,3189,0.0197355,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,3370,0.226959,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,3373,0.187488,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,4496,0.0986777,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,4560,0.00986777,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,4563,0.00986777,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,4662,0.00986777,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,4666,0.00986777,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,5571,0.0493389,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,7109,0.00986777,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,8178,0.0789422,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,8180,0.0197355,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,8181,0.0197355,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,8182,0.00986777,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,54634,0.276298,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,55973,0.0197355,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,56234,54.332,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,56236,45.6286,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (55292,0,56237,0.00986777,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,727,0.0268588,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,766,0.146933,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,767,0.123234,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,768,0.0916359,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,774,0.241729,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,805,0.118495,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,818,0.109015,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,828,0.0521376,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,1411,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,1417,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,1418,0.00315986,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,1420,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,1425,0.00315986,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,1429,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,1431,0.0189591,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,1438,0.00315986,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2075,0.00315986,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2140,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2211,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2212,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2589,0.0189591,0,1,0,1,2,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2598,0.00315986,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2642,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2643,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2645,0.0521376,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2773,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2774,0.00315986,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2958,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2959,0.0236989,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2960,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2961,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2963,0.00315986,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2967,0.00473979,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,2968,0.0900559,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,3189,0.0458179,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,3190,0.257528,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,3200,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,3213,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,3609,0.00473979,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,4496,0.127974,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,4560,0.0821563,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,4561,0.167472,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,4562,0.0363384,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,4563,0.0631971,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,4565,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,4658,0.0394982,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,4659,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,4662,0.00789964,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,4663,0.0189591,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,4666,0.00631971,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,5571,0.0789964,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,5572,0.115335,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,6269,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,6508,0.0347584,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,6510,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,6519,0.00315986,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,6520,0.0868961,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,8178,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,8179,0.112175,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,8180,0.0900559,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,8181,0.00473979,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,8182,0.139034,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,9745,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,9754,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,15895,0.00157993,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,55973,0.107435,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,55983,0.146933,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,56195,84.2418,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,56196,15.7393,0,1,0,1,1,'');
INSERT IGNORE INTO `creature_loot_template` (`entry`,`itemType`,`item`,`chance`,`questrequired`,`lootmode`,`groupid`,`mincount`,`maxcount`,`comment`) VALUES (60411,0,56197,0.00947957,0,1,0,1,1,'');
