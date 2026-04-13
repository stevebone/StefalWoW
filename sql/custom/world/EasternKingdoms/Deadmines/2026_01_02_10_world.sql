-- Defias Envoker: 48418
-- Defias Miner: 48419
-- Defias Overseer: 48421
-- Defias Digger: 48420
-- Mine Bunny: 48338

-- Foe Reaper 5000: 43778

-- temp fixes for heroic version only mobs
UPDATE `creature` SET `spawnDifficulties` = '2' WHERE `map` = 36 AND `id` IN (48338);

-- temp fixes for horde version only mobs
UPDATE `creature` SET `phaseId` = 170 WHERE `map` = 36 AND `id` IN (46890, 46902, 46889, 46906,46903);

-- temp fixes for alliance version only mobs
UPDATE `creature` SET `phaseId` = 226 WHERE `map` = 36 AND `id` IN (46612, 46614, 50595, 46613, 491);

-- Delete difficulty 0 since this wrong for dungeon
DELETE FROM `creature_template_difficulty` WHERE `DifficultyID` = 0 AND `Entry` IN (48418,48419,48421,48420,43778);
DELETE FROM `creature_template_difficulty` WHERE `DifficultyID` = 0 AND `Entry` IN (47468,47403,49229,49208); -- Adds for Foereaper

-- Fix content tuning for normal (1) difficulty
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48418') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48419') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48421') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48420') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '47403') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '43778') and (`DifficultyID` = '1');

UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin` = '1', `LevelScalingDeltaMax` = '1' WHERE (`Entry` = '43778') and (`DifficultyID` = '1');

-- Defias Envoker fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48418');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48418;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `target_type`, `comment`) VALUES 
('48418', '0', '0', '0', '1,2', '0', '100', '0', '0', '3400', '4700', '11', '91004', '64', '2', 'Defias Envoker Cast Holy Fire'),
('48418', '0', '1', '0', '1,2', '11', '100', '0', '0', '0', '0', '11', '92001', '0', '1', 'Defias Envoker Cast Shield on Spawn'),
('48418', '0', '2', '3', '1,2', '2', '100', '0', '15', '1000', '1000', '25', '0', '0', '1', 'Defias Envoker Flee 15% HP'),
('48418', '0', '3', '0', '1,2', '61', '100', '0', '15', '0', '0', '1', '0', '0', '1', 'Defias Envoker Say Text at 15% HP'),
('48418', '0', '4', '0', '1,2', '0', '100', '0', '0', '9000', '12000', '11', '90047', '0', '1', 'Defias Envoker Cast Renegade Strength');

DELETE FROM `creature_text` WHERE `CreatureID` = 48418;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `comment`) VALUES 
('48418', '0', '1', '%s attempts to run away in fear!', '16', '100', 'Defias Envoker Flee');

-- Defias Miner fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48419');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48419;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `target_type`, `comment`) VALUES 
('48419', '0', '0', '0', '1,2', '2', '100', '0', '30', '1000', '1000', '11', '90994', '0', '1', 'Defias Miner Cast Tunnel 30% HP'),
('48419', '0', '1', '0', '2', '2', '100', '0', '15', '1000', '1000', '11', '91009', '0', '1', 'Defias Miner Cast Renegade Strength 15% HP');

-- Defias Digger fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48420');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48420;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `target_type`, `comment`) VALUES 
('48420', '0', '0', '0', '1,2', '2', '100', '0', '30', '1000', '1000', '11', '90994', '0', '1', 'Defias Digger Cast Tunnel 30% HP'),
('48420', '0', '1', '0', '2', '2', '100', '0', '15', '1000', '1000', '11', '91009', '0', '1', 'Defias Digger Cast Renegade Strength 15% HP');

-- Defias Reaper fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '47403');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 47403;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `target_type`, `comment`) VALUES 
('47403', '0', '0', '0', '1,2', '9', '100', '0', '5', '7000', '9000', '11', '90980', '0', '2', 'Defias Reaper Cast Cleave'),
('47403', '0', '1', '0', '1,2', '2', '100', '0', '60', '1000', '1000', '11', '90978', '0', '1', 'Defias Reaper Energize! 60% HP'),
('47403', '0', '2', '0', '1,2', '2', '100', '0', '25', '1000', '1000', '11', '91737', '0', '1', 'Defias Reaper On Fire! 25% HP');

-- Foereaper 5000 encounter
UPDATE `creature_template` SET `ScriptName` = 'boss_foereaper5000' WHERE (`entry` = '43778');
UPDATE `creature_template` SET `ScriptName` = 'npc_foereaper_targeting_bunny' WHERE (`entry` = '47468');

-- Foereaper loot fixes
UPDATE `creature_template_difficulty` SET `LootID` = '4377801' WHERE (`Entry` = '43778') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `LootID` = '4377802' WHERE (`Entry` = '43778') and (`DifficultyID` = '2');
DELETE FROM `creature_loot_template` WHERE `Entry` IN (4377801,4377802); -- Helix loot normal and heroic
INSERT INTO `creature_loot_template` VALUES
('4377801', '0', '5201', '31', '0', '1', '0', '1', '1', 'Foereaper Emberstone Staff'),
('4377801', '0', '1937', '28', '0', '1', '0', '1', '1', 'Foereaper Buzz Saw'),
('4377801', '0', '5187', '28', '0', '1', '0', '1', '1', 'Foereaper Foe Reaper'),
('4377801', '0', '151066', '4', '0', '1', '0', '1', '1', 'Foereaper Missing Diplomat Pauldrons'),
('4377801', '0', '151064', '4', '0', '1', '0', '1', '1', 'Foereaper Vest of the Curious Visitor'),
('4377801', '0', '151065', '3', '0', '1', '0', '1', '1', 'Foereaper Old Friends Gloves'),

('4377801', '0', '65167', '32', '0', '1', '0', '1', '1', 'Foereaper Emberstone Staff'),
('4377801', '0', '65166', '31', '0', '1', '0', '1', '1', 'Foereaper Buzz Saw'),
('4377801', '0', '65165', '31', '0', '1', '0', '1', '1', 'Foereaper Foe Reaper'),
('4377801', '0', '157755', '1', '0', '1', '0', '1', '1', 'Foereaper Missing Diplomat Pauldrons'),
('4377801', '0', '157753', '1', '0', '1', '0', '1', '1', 'Foereaper Vest of the Curious Visitor'),
('4377801', '0', '157754', '1', '0', '1', '0', '1', '1', 'Foereaper Old Friends Gloves');

DELETE FROM `creature_text` WHERE `CreatureID` = 43778;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `comment`) VALUES 
('43778', '0', '1', 'Foe Reaper 5000 on-line. All systems nominal.', '14', '100', 'Foereaper Aggro'),
('43778', '1', '2', 'Overheat threshold exceeded. System failure. Wheat clog in port two. Shutting down.', '14', '100', 'Foereaper Death'),
('43778', '2', '3', 'Target terminated', '14', '100', 'Foereaper kill'),
('43778', '3', '4', 'Target acquired. Harvesting servos engaged.', '14', '100', 'Foereaper Spell Harvesting'),
('43778', '4', '5', 'Acquiring target...', '14', '100', 'Foereaper Harvest Targetting'),
('43778', '5', '6', 'Overdrive engine activated', '14', '100', 'Foereaper Spell Overdrive'),
('43778', '6', '7', 'Safety restrictions off-line. Catastrophic system failure imminent.', '14', '100', 'Foereaper Spell Safety');