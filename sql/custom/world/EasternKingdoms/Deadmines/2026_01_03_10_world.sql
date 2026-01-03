-- Defias Blood Wizzard: 48417
-- Defias Enforcer: 48502
-- Defias Shadowguard: 48505

-- Monstrous Parrot: 48447
-- Sunwing Squawker: 48450
-- Defias Pirate: 48522
-- Defias Squallshaper: 48521

-- Admiral Ripsnarl: 47626
-- Ol'Beaky: 48451
-- Brillian Macaw: 48449
-- Albino Eyegouger: 48448

-- Delete difficulty 0 since this wrong for dungeon
DELETE FROM `creature_template_difficulty` WHERE `DifficultyID` = 0 AND `Entry` IN (48417,48502,48505,48447,48450,48522,48521,48451,47626);

-- Fix content tuning for normal (1) difficulty
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48417') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48502') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48505') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48447') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48450') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48522') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48521') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48451') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '47626') and (`DifficultyID` = '1');

UPDATE `creature_template_difficulty` SET `ContentTuningID` = '1199' WHERE (`Entry` = '48448') and (`DifficultyID` = '2');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '1199' WHERE (`Entry` = '48449') and (`DifficultyID` = '2');


UPDATE `creature_template_difficulty` SET `DifficultyID` = '1' WHERE (`Entry` = '48448') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `DifficultyID` = '1' WHERE (`Entry` = '48449') and (`DifficultyID` = '0');

UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48448') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48449') and (`DifficultyID` = '1');

UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin` = '1', `LevelScalingDeltaMax` = '1' WHERE (`Entry` = '47626') and (`DifficultyID` = '1');

-- Defias Blood Wizzard fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48417');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48417;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `target_type`, `comment`) VALUES 
('48417', '0', '0', '0', '1,2', '0', '100', '0', '0', '3400', '4700', '11', '90938', '64', '2', 'Defias Blood Wizzard Cast Bloodbolt'),
('48417', '0', '1', '0', '1,2', '0', '100', '5000', '5000', '35000', '35000', '11', '90932', '0', '1', 'Defias Blood Wizzard Cast Ragezone'),
('48417', '0', '2', '3', '1,2', '2', '100', '0', '15', '1000', '1000', '25', '0', '0', '1', 'Defias Blood Wizzard Flee 15% HP'),
('48417', '0', '3', '0', '1,2', '61', '100', '0', '15', '0', '0', '1', '0', '0', '1', 'Defias Blood Wizzard Say Text at 15% HP'),
('48417', '0', '4', '0', '2', '0', '100', '1000', '2000', '12000', '14000', '11', '90946', '0', '7', 'Defias Blood Wizzard Cast Bloodwash');

DELETE FROM `creature_text` WHERE `CreatureID` = 48417;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `comment`) VALUES 
('48417', '0', '1', '%s attempts to run away in fear!', '16', '100', 'Defias Envoker Flee');

-- Defias Enforcer fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48502');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48502;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `target_type`, `comment`) VALUES 
('48502', '0', '0', '0', '1,2', '4', '100', '0', '0', '0', '0', '11', '90928', '64', '2', 'Defias Enforcer Cast Charge'),
('48502', '0', '1', '0', '1,2', '0', '100', '5000', '5000', '17000', '19000', '11', '90929', '0', '2', 'Defias Enforcer Cast Recklessness'),
('48502', '0', '2', '0', '1,2', '2', '100', '0', '40', '8000', '11000', '11', '90925', '1', '2', 'Defias Enforcer Cast Bloodbath 40% HP');

-- Defias Shadowguard fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48505');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48505;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `target_type`, `comment`) VALUES 
('48505', '0', '0', '0', '1,2', '0', '100', '2000', '3000', '9000', '11000', '11', '90951', '0', '2', 'Defias Shadowguard Cast Sinister Strike'),
('48505', '0', '1', '0', '1,2', '0', '100', '12000', '14000', '24000', '28000', '11', '90960', '0', '1', 'Defias Shadowguard Cast Whirling Blades'),
('48505', '0', '2', '0', '1', '2', '100', '0', '50', '15000', '18000', '11', '90958', '0', '1', 'Defias Shadowguard Cast Evasion 50% HP'),
('48505', '0', '3', '0', '1,2', '0', '100', '8000', '8000', '14000', '15000', '11', '90956', '0', '5', 'Defias Shadowguard Cast Shadowstep');

-- Defias Cannon Event
UPDATE `gameobject_template` SET `ScriptName` = 'go_defias_cannon' WHERE (`entry` = '442661'); -- Defias Cannon (new)
UPDATE `gameobject` SET `id` = '442661' WHERE (`guid` = '235341'); -- Defias Cannon (new) replacing 16398

-- Gunpowder spawn
DELETE FROM `gameobject` WHERE `map` = 36 AND `id` = 17155;
INSERT INTO `gameobject` VALUES
('900043', '17155', '36', '1581', '1581', '1,2', '0', '0', '0', '-1', '-106.115', '-617.24', '13.8818', '3.22747', '-0', '-0', '-0.999078', '0.0429272', '300', '255', '1', '', NULL, '0');

-- Gunpowder fix for loot
UPDATE `gameobject_template` SET `Data1` = '17155' WHERE (`entry` = '17155');

DELETE FROM `gameobject_loot_template` WHERE `Entry` IN (17155); -- Defias Gunpowder
INSERT INTO `gameobject_loot_template` VALUES
('17155', '0', '221485', '100', '0', '1', '0', '1', '1', 'Defias Gunpowder');

-- Rare parrots spawns
DELETE FROM `creature` WHERE `map` = 36 AND `id` IN (48448, 48449);
INSERT INTO `creature` VALUES
('900264', '48448', '36', '1581', '1582', '1', '0', '0', '0', '-1', '0', '0', '-46.8184', '-691.455', '3.82295', '4.42742', '300', '30', '0', '100', '1', NULL, NULL, NULL, NULL, '', NULL, '0'),
('900265', '48449', '36', '1581', '1582', '1', '0', '0', '0', '-1', '0', '0', '-39.5969', '-695.628', '3.97999', '6.19238', '300', '30', '0', '100', '1', NULL, NULL, NULL, NULL, '', NULL, '0');

-- Albino Eyegouger fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48448');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48448;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `target_type`, `comment`) VALUES 
('48448', '0', '0', '0', '1,2', '0', '100', '4000', '4900', '9000', '12000', '11', '90913', '0', '2', 'Albino Eyegouger Cast Gouge'),
('48448', '0', '1', '0', '1,2', '0', '100', '7000', '9000', '16000', '19000', '11', '90920', '0', '2', 'Albino Eyegouger Cast Peck');

-- Brillian Macaw fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48449');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48449;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `target_type`, `comment`) VALUES 
('48449', '0', '0', '0', '1,2', '0', '100', '4000', '4900', '9000', '12000', '11', '90913', '0', '2', 'Brillian Macaw Cast Gouge'),
('48449', '0', '1', '0', '1,2', '0', '100', '7000', '9000', '16000', '19000', '11', '90920', '0', '2', 'Brillian Macaw Cast Peck');

-- Ol Beaky fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48451');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48451;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `target_type`, `comment`) VALUES 
('48451', '0', '0', '0', '1,2', '0', '100', '4000', '4900', '9000', '12000', '11', '90913', '0', '2', 'Ol Beaky Cast Gouge'),
('48451', '0', '1', '0', '1,2', '0', '100', '7000', '9000', '16000', '19000', '11', '90920', '0', '2', 'Ol Beaky Cast Peck');

-- Monstrous Parrot fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48447');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48447;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `target_type`, `comment`) VALUES 
('48447', '0', '0', '0', '1,2', '0', '100', '4000', '4900', '9000', '12000', '11', '90913', '0', '2', 'Monstrous Parrot Cast Gouge'),
('48447', '0', '1', '0', '1,2', '0', '100', '7000', '9000', '16000', '19000', '11', '90920', '0', '2', 'Monstrous Parrot Cast Peck');

-- Sunwing Squawker fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48450');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48450;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `target_type`, `comment`) VALUES 
('48450', '0', '0', '0', '1,2', '0', '100', '4000', '4900', '9000', '12000', '11', '90913', '0', '2', 'Sunwing Squawker Cast Gouge'),
('48450', '0', '1', '0', '1,2', '0', '100', '7000', '9000', '16000', '19000', '11', '90920', '0', '2', 'Sunwing Squawker Cast Peck');

-- Defias Squallshaper fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48521');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48521;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `target_type`, `comment`) VALUES 
('48521', '0', '0', '0', '1,2', '0', '100', '0', '0', '3400', '4700', '11', '90398', '64', '2', 'Defias Squallshaper Cast Seaswell'),
('48521', '0', '1', '0', '2', '14', '100', '200000', '40', '20000', '30000', '11', '90914', '0', '7', 'Defias Squallshaper Cast Riptide'),
('48521', '0', '2', '3', '1,2', '2', '100', '0', '15', '1000', '1000', '25', '0', '0', '1', 'Defias Squallshaper Flee 15% HP'),
('48521', '0', '3', '0', '1,2', '61', '100', '0', '15', '0', '0', '1', '0', '0', '1', 'Defias Squallshaper Say Text at 15% HP');

-- Defias Pirate fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48522');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48522;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `target_type`, `comment`) VALUES 
('48522', '0', '0', '0', '1,2', '4', '100', '0', '0', '0', '0', '11', '90905', '0', '2', 'Defias Pirate Cast Leaping Cleave');

-- Difficulty 1 update for thrash mobs
UPDATE `creature_template_difficulty` SET `HealthModifier` = 1, `ManaModifier` = 1, `ArmorModifier` = 1, `DamageModifier` = 1 WHERE `DifficultyID` = 1 AND `Entry` IN (48417,48502,48505,48447,48450,48522,48521,48451,48449,48448,48418,48419,48421,48420,48338,48229,48230,48262,48279,48441,48445);

-- Loot for thrash mobs
UPDATE `creature_template_difficulty` SET `LootID` = 4822901 WHERE `DifficultyID` = 1 AND `Entry` IN (48417,48502,48505,48447,48450,48522,48521,48451,48449,48448,48418,48419,48421,48420,48338,48229,48230,48262,48279,48441,48445);

DELETE FROM `creature_loot_template` WHERE `Entry` in (48417,48502,48505,48447,48450,48522,48521,48451,48449,48448,48418,48419,48421,48420,48338,48229,48230,48262,48279,48441,48445);
INSERT INTO `creature_loot_template` VALUES
('4822901', '0', '414', '2.6616', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '818', '1.402', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '856', '0.0306', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '858', '2.4691', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1179', '3.7', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1210', '4.3552', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1495', '0.202', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1501', '0.2937', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1513', '3.0191', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1731', '1.9175', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1734', '2.1317', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1735', '2.0705', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2073', '0.1469', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2079', '0.0744', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2287', '3.2922', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2406', '0.0319', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2407', '0.0255', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2408', '0.0301', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2455', '1.4', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2553', '0.0182', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2555', '0.0251', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2589', '22.5', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2592', '29.6', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2632', '0.3787', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2763', '0.3723', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2836', '0.0102', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2969', '0.0246', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2970', '0.1099', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2971', '0.1916', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2972', '0.2281', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2973', '0.0538', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2974', '0.1045', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2975', '0.2176', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2976', '0.1045', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2977', '0.0954', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2978', '0.1145', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2979', '0.1683', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2980', '0.1045', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2983', '0.0579', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '3036', '0.1647', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '3205', '0.0424', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '3207', '0.1725', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '3213', '0.0858', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '3610', '0.036', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '3643', '0.2062', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '3645', '0.0547', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '3651', '0.1401', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '3652', '0.1551', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4293', '0.1938', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4345', '0.0143', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4408', '0.0328', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4541', '4.9383', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4564', '0.2235', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4566', '0.229', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4567', '0.0912', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4569', '0.4206', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4570', '0.1638', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4571', '0.068', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4577', '0.198', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4678', '0.1588', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4683', '0.125', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4684', '0.1191', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4689', '0.0794', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4690', '0.0899', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4701', '0.1154', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '5071', '0.406', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '5212', '0.0757', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '5367', '0.0005', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6266', '0.0858', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6268', '0.1551', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6271', '0.0292', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6336', '0.1405', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6342', '0.0274', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6348', '0.0265', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6378', '0.0648', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6379', '0.0557', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6380', '0.0283', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6512', '0.0972', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6528', '0.0347', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6537', '0.1236', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6539', '0.1588', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6541', '0.1054', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6542', '0.1464', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6543', '0.1163', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6546', '0.047', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6547', '0.0465', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6548', '0.1017', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6549', '0.167', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6550', '0.0881', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6551', '0.0397', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6554', '0.0693', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6555', '0.2121', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6556', '0.1104', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6557', '0.1013', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6558', '0.1182', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6559', '0.0265', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6560', '0.0392', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '7288', '0.0328', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '7997', '1.0192', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9746', '0.1045', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9747', '0.3399', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9748', '0.0675', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9749', '0.0557', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9755', '0.0963', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9756', '0.3038', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9757', '0.1077', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9759', '0.0766', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9762', '0.0739', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9763', '0.2423', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9764', '0.1323', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9765', '0.0967', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9785', '0.0351', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9786', '0.0917', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '15210', '0.1145', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '15268', '0.1305', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '15926', '0.0301', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '15933', '0.1291', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1497', '0.2552', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1499', '0.2933', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1502', '0.2437', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1503', '0.2922', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1506', '0.2945', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1509', '0.2633', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1510', '3.5633', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1511', '0.3546', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1512', '3.4585', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1515', '3.4847', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1730', '0.179', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1732', '0.2529', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2078', '0.0716', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2214', '0.2033', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2409', '0.0157', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2777', '0.3354', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2778', '0.3275', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2984', '0.067', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2987', '0.0474', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '3195', '0.0624', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '3196', '0.082', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '3374', '0.2594', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '3375', '0.2506', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4409', '0.1834', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4695', '0.0612', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4699', '0.0924', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '5573', '0.0462', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '5574', '0.0416', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6347', '0.0157', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6540', '0.0647', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6545', '0.0693', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6553', '0.0716', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6716', '0.1572', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9753', '0.0012', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9779', '0.0658', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9788', '0.0785', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2657', '0.0311', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '723', '0.0007', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1468', '0.0007', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1498', '2.4295', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1504', '0.2797', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1507', '0.2769', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1514', '0.3803', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1516', '0.3456', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1737', '0.2039', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2992', '0.0007', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4605', '0.0021', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '915', '75', '1', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1929', '6.0188', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2881', '0.3701', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4537', '0.7692', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '5368', '13.0769', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '782', '0.0012', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2215', '2.2461', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2598', '0.1005', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1930', '2.775', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2838', '0.4128', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1926', '0.9642', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '6344', '0.0182', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '10400', '3.4423', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '10401', '3.6239', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4346', '0.0194', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '4292', '0.2196', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '5114', '16.2173', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '5115', '5.4791', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2996', '0.0011', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '9783', '0.0245', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '14167', '0.0011', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '14173', '0.0056', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '1951', '6.4414', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '2672', '0.0009', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '5786', '0.6005', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '5787', '0.6871', '0', '1', '0', '1', '1', 'Deadmines Trash'),
('4822901', '0', '8492', '2.619', '0', '1', '0', '1', '1', 'Deadmines Trash');
