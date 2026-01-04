-- Ripsnarl encounter
-- Admiral Ripsnarl: 47626
-- Adds
-- Ripsnarl Vapor: 47714
-- Ripsnarl Dummy 1: 47242
-- Ripsnarl Dummy 2: 45979


DELETE FROM `creature_template_difficulty` WHERE `Entry` IN (47242,47714);
INSERT INTO `creature_template_difficulty` VALUES
('47714', '1', '0', '0', '202', '0', '0.5', '1', '1', '0.5', '42863', '0', '0', '0', '0', '0', '0', '0', '0', '524288', '0', '0', '0', '0', '0', '0', '0', '45745'),
('47714', '2', '0', '0', '1199', '3', '0.5', '1', '1', '0.5', '42864', '0', '0', '0', '0', '0', '0', '0', '0', '524288', '0', '0', '0', '0', '0', '0', '0', '0'),
('47242', '1', '0', '0', '202', '10', '1', '1', '1', '1', '43815', '1024', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '45338'),
('47242', '2', '0', '0', '1199', '10', '1', '1', '1', '1', '43815', '1024', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '45745');

UPDATE `creature_template_difficulty` SET `DifficultyID` = 1 WHERE `entry` IN (45979) AND `DifficultyID` = 0;

UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '47739') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '45979') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '1199' WHERE (`Entry` = '47739') and (`DifficultyID` = '2');

DELETE FROM `creature_template_difficulty` WHERE `entry` = 45979 AND `DifficultyID` = 2;
INSERT INTO `creature_template_difficulty` VALUES
('45979', '2', '0', '0', '1199', '10', '1', '1', '1', '1', '46416', '1024', '0', '0', '0', '0', '0', '0', '0', '536870912', '0', '0', '0', '0', '0', '0', '0', '48892');


UPDATE `creature_template` SET `ScriptName` = 'boss_admiral_ripsnarl' WHERE (`entry` = '47626');
UPDATE `creature_template` SET `ScriptName` = 'npc_admiral_ripsnarl_vapor' WHERE (`entry` = '47714');
INSERT IGNORE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('92042', 'spell_admiral_ripsnarl_coalesce');

DELETE FROM `creature_text` WHERE `CreatureID` = 47626;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `comment`) VALUES 
('47626', '0', '1', 'You will... NOT find her... until it is too late...', '14', '100', 'Admiral Ripsnarl Death'),
('47626', '1', '2', 'Your blood only increases my hunger!', '14', '100', 'Admiral Ripsnarl kill'),
('47626', '2', '3', 'Do you feel that chill running up your spine?', '14', '100', 'Admiral Ripsnarl Fog 1'),
('47626', '3', '4', 'The fog is rolling in...', '14', '100', 'Admiral Ripsnarl Fog 2'),
('47626', '4', '5', 'I can smell your fear...', '14', '100', 'Admiral Ripsnarl Go for the throat 1'),
('47626', '5', '6', 'Can you still keep up?', '14', '100', 'Admiral Ripsnarl Special Phase'),
('47626', '6', '7', 'I will rip your heart from your chest!', '14', '100', 'Admiral Ripsnarl Go for the throat 2'),
('47626', '7', '8', 'Ah, fresh meat!', '14', '100', 'Admiral Ripsnarl Aggro');

-- Admiral Ripsnarl loot fixes
UPDATE `creature_template_difficulty` SET `LootID` = '4762601' WHERE (`Entry` = '47626') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `LootID` = '4762602' WHERE (`Entry` = '47626') and (`DifficultyID` = '2');
DELETE FROM `creature_loot_template` WHERE `Entry` IN (4762601,4762602); -- Ripsnarl loot normal and heroic
INSERT INTO `creature_loot_template` VALUES
('4762601', '0', '1156', '39', '0', '1', '0', '1', '1', 'Admiral Ripsnarl Lavishly Jeweled Ring'),
('4762601', '0', '872', '31', '0', '1', '0', '1', '1', 'Admiral Ripsnarl Rockslicer'),
('4762601', '0', '5196', '29', '0', '1', '0', '1', '1', 'Admiral Ripsnarl Smite Reaver'),
('4762601', '0', '71638', '1', '0', '1', '0', '1', '1', 'Admiral Ripsnarl Ornate Weapon'),

('4762602', '0', '65170', '34', '0', '1', '0', '1', '1', 'Admiral Ripsnarl Smite Reaver'),
('4762602', '0', '65168', '33', '0', '1', '0', '1', '1', 'Admiral Ripsnarl Rockslicer'),
('4762602', '0', '65169', '32', '0', '1', '0', '1', '1', 'Admiral Ripsnarl Lavishly Jeweled Ring'),
('4762602', '0', '44731', '3', '0', '1', '0', '1', '1', 'Admiral Ripsnarl Bouquet of Ebon Roses'),
('4762602', '0', '22206', '2', '0', '1', '0', '1', '1', 'Admiral Ripsnarl Bouquet of Red Roses'),
('4762602', '0', '21524', '14', '0', '1', '0', '1', '1', 'Admiral Ripsnarl Red Winter Hat');


-- Cookie encounter
-- Captain Cookie: 47739

UPDATE `creature_template` SET `ScriptName` = 'boss_captain_cookie' WHERE (`entry` = '47739');
UPDATE `creature_template` SET `ScriptName` = 'npc_captain_cookie_cauldron' WHERE (`entry` = '47754');
UPDATE `creature_template` SET `ScriptName` = 'npc_captain_cookie_good_food' WHERE `entry` IN (48006,48294,48296,48297,48300,48301);
UPDATE `creature_template` SET `ScriptName` = 'npc_captain_cookie_bad_food' WHERE `entry` IN (48276,48293,48295,48298,48299,48302);

DELETE FROM `spell_script_names` WHERE `spell_id` IN (89732,89267);
INSERT IGNORE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('89732', 'spell_captain_cookie_nauseated');
INSERT IGNORE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('89267', 'spell_captain_cookie_setiated');


DELETE FROM `creature_template_difficulty` WHERE `DifficultyID` = 0 AND `Entry` IN (47739);
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '47739') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin` = '1', `LevelScalingDeltaMax` = '1' WHERE (`Entry` = '47739') and (`DifficultyID` = '1');

-- Cookie spawn
DELETE FROM `creature` WHERE `map` = 36 AND `id` IN (47739);
INSERT INTO `creature` VALUES
('900266', '47739', '36', '1581', '1582', '1,2', '0', '0', '0', '-1', '0', '0', '-88.1319', '-819.33', '39.2345', '0', '3600', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0');


-- Update Cookie Adds
DELETE FROM `creature_template_difficulty` WHERE `entry` IN (48672,47754,48301,48300,48297,48296,48006,
48294,48295,48276,48299,48293,48298,48302);
INSERT INTO `creature_template_difficulty` VALUES
('47754', '1', '0', '0', '338', '0', '1', '1', '1', '1', '42776', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('47754', '2', '0', '0', '338', '0', '1', '1', '1', '1', '42776', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '45745'),
('48006', '1', '0', '0', '338', '0', '1', '1', '1', '1', '42235', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('48006', '2', '1', '1', '338', '0', '1', '1', '1', '1', '42235', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '45745'),
('48276', '1', '0', '0', '338', '0', '1', '1', '1', '1', '41640', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('48276', '2', '1', '1', '338', '0', '1', '1', '1', '1', '41640', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '45745'),
('48293', '1', '0', '0', '338', '0', '1', '1', '1', '1', '41594', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('48293', '2', '1', '1', '338', '0', '1', '1', '1', '1', '41594', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '45745'),
('48294', '1', '0', '0', '338', '0', '1', '1', '1', '1', '41592', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('48294', '2', '1', '1', '338', '0', '1', '1', '1', '1', '41592', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '45745'),
('48295', '1', '0', '0', '338', '0', '1', '1', '1', '1', '41590', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('48295', '2', '1', '1', '338', '0', '1', '1', '1', '1', '41590', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '45745'),
('48296', '1', '0', '0', '338', '0', '1', '1', '1', '1', '41588', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('48296', '2', '1', '1', '338', '0', '1', '1', '1', '1', '41588', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '45745'),
('48297', '1', '0', '0', '338', '0', '1', '1', '1', '1', '41586', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('48297', '2', '1', '1', '338', '0', '1', '1', '1', '1', '41586', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '45745'),
('48298', '1', '0', '0', '338', '0', '1', '1', '1', '1', '41584', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('48298', '2', '1', '1', '338', '0', '1', '1', '1', '1', '41584', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '45745'),
('48299', '1', '0', '0', '338', '0', '1', '1', '1', '1', '41582', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('48299', '2', '1', '1', '338', '0', '1', '1', '1', '1', '41582', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '45745'),
('48300', '1', '0', '0', '338', '0', '1', '1', '1', '1', '41580', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('48300', '2', '1', '1', '338', '0', '1', '1', '1', '1', '41580', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '45745'),
('48301', '1', '0', '0', '338', '0', '1', '1', '1', '1', '41578', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('48301', '2', '1', '1', '338', '0', '1', '1', '1', '1', '41578', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '45745'),
('48302', '1', '0', '0', '338', '0', '1', '1', '1', '1', '41576', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('48302', '2', '1', '1', '338', '0', '1', '1', '1', '1', '41576', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '45745'),
('48672', '1', '0', '0', '338', '0', '1', '1', '1', '1', '40837', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('48672', '2', '0', '0', '338', '0', '1', '1', '1', '1', '40837', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '45745');

-- Captain Cookie loot fixes
UPDATE `creature_template_difficulty` SET `LootID` = '4773901' WHERE (`Entry` = '47739') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `LootID` = '4773902' WHERE (`Entry` = '47739') and (`DifficultyID` = '2');
DELETE FROM `creature_loot_template` WHERE `Entry` IN (4773901,4773902); -- Ripsnarl loot normal and heroic
INSERT INTO `creature_loot_template` VALUES
('4773901', '0', '5193', '28', '0', '1', '0', '1', '1', 'Captain Cookie Cape of the Brotherhood'),
('4773901', '0', '5198', '21', '0', '1', '0', '1', '1', 'Captain Cookie Cookies Stirring Rod'),
('4773901', '0', '5202', '18', '0', '1', '0', '1', '1', 'Captain Cookie Corsair Overshirt'),
('4773901', '0', '5197', '17', '0', '1', '0', '1', '1', 'Captain Cookie Cookies Tenderizer'),
('4773901', '0', '5192', '16', '0', '1', '0', '1', '1', 'Captain Cookie Thiefs Blade'),
('4773901', '0', '248332', '1', '0', '1', '0', '1', '1', 'Captain Cookie Stormwind Footlocker'),

('4773902', '0', '65171', '21', '0', '1', '0', '1', '1', 'Captain Cookie Cookies Tenderizer'),
('4773902', '0', '65174', '21', '0', '1', '0', '1', '1', 'Captain Cookie Corsair Overshirt'),
('4773902', '0', '65173', '21', '0', '1', '0', '1', '1', 'Captain Cookie Thiefs Blade'),
('4773902', '0', '65177', '21', '0', '1', '0', '1', '1', 'Captain Cookie Cape of the Brotherhood'),
('4773902', '0', '65172', '21', '0', '1', '0', '1', '1', 'Captain Cookie Cookies Stirring Rod');

-- Difficulty 1 update for boss mobs
UPDATE `creature_template_difficulty` SET `HealthModifier` = 1, `ManaModifier` = 1, `ArmorModifier` = 1, `DamageModifier` = 1 WHERE `DifficultyID` = 1 AND `Entry` IN (47626,43778,47162,47296,47739,47297);

-- TEMP FIX to remove the invisible mobs (adds) from Glubtok that keep attacking the player
-- Will need to check with heroic version if they are needed to be pre-spawned
UPDATE `creature` SET `spawnDifficulties` = '2' WHERE `map` = 36 AND `id` IN (48975,48976,49039,49040,49041,49042);