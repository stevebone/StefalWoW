-- temp fixes for heroic version only mobs
UPDATE `creature` SET `spawnDifficulties` = '2' WHERE `map` = 36 AND `id` IN (48351); -- 48338

-- Delete difficulty 0 since this wrong for dungeon
DELETE FROM `creature_template_difficulty` WHERE `DifficultyID` = 0 AND `Entry` IN (48229,48230,48266,48262,47162,48284,48279,48441,48445,47296,47297,47314);

-- Fix content tuning for normal (1) difficulty
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '47162') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '47296') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '47297') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48229') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48230') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48262') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48266') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48284') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48279') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48441') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '48445') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '202' WHERE (`Entry` = '47314') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '1199' WHERE (`Entry` = '47314') and (`DifficultyID` = '2');

UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin` = '0', `LevelScalingDeltaMax` = '0' WHERE (`Entry` = '48229') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin` = '0', `LevelScalingDeltaMax` = '0' WHERE (`Entry` = '48284') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin` = '0', `LevelScalingDeltaMax` = '0' WHERE (`Entry` = '48441') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin` = '1', `LevelScalingDeltaMax` = '1' WHERE (`Entry` = '47162') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin` = '1', `LevelScalingDeltaMax` = '1' WHERE (`Entry` = '47296') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin` = '1', `LevelScalingDeltaMax` = '1' WHERE (`Entry` = '47297') and (`DifficultyID` = '0');

-- Glubtok encounter
UPDATE `creature_template` SET `ScriptName` = 'boss_glubtok' WHERE (`entry` = '47162');

DELETE FROM `creature_text` WHERE `CreatureID` = 47162;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `comment`) VALUES 
('47162', '0', '1', 'TOO... MUCH... POWER!!!', '14', '100', 'Glubtok Death'),
('47162', '1', '2', 'ARCANE POWER!!!!', '14', '100', 'Glubtok Arcane attack'),
('47162', '2', '3', 'Glubtok show you da power of de arcane!', '14', '100', 'Glubtok Aggro'),
('47162', '3', '4', '\'Sploded dat one!', '14', '100', 'Glubtok kill'),
('47162', '4', '5', 'Fists of flame!', '14', '100', 'Glubtok Fire attack'),
('47162', '5', '6', 'Glubtok ready?', '14', '100', 'Glubtok Head 1'),
('47162', '6', '7', 'Fists of frost!', '14', '100', 'Glubtok Frost attack'),
('47162', '7', '8', 'Let\'s do it!', '14', '100', 'Glubtok Head 2');

-- Glubtok loot fixes
UPDATE `creature_template_difficulty` SET `LootID` = '4716201' WHERE (`Entry` = '47162') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `LootID` = '4716202' WHERE (`Entry` = '47162') and (`DifficultyID` = '2');
DELETE FROM `creature_loot_template` WHERE `Entry` IN (4716201,4716202); -- Glubtok loot normal and heroic
INSERT INTO `creature_loot_template` VALUES
('4716201', '0', '5444', '37', '0', '1', '0', '1', '1', 'Glubtok Miner Cape'),
('4716201', '0', '2169', '32', '0', '1', '0', '1', '1', 'Glubtok Buzzer Blade'),
('4716201', '0', '5195', '32', '0', '1', '0', '1', '1', 'Glubtok Gold-Flecked Gloves'),
('4716202', '0', '65163', '21', '0', '1', '0', '1', '1', 'Glubtok Heroic Buzzer Blade'),
('4716202', '0', '63468', '20', '0', '1', '0', '1', '1', 'Glubtok Heroic Defias Brotherhood Vest'),
('4716202', '0', '63470', '20', '0', '1', '0', '1', '1', 'Glubtok Heroic Missing Diplomat Pauldrons'),
('4716202', '0', '63471', '20', '0', '1', '0', '1', '1', 'Glubtok Heroic Vest of the curious visitor'),
('4716202', '0', '63467', '20', '0', '1', '0', '1', '1', 'Glubtok Heroic Shadow of the past'),
('4716202', '0', '157628', '19', '0', '1', '0', '1', '1', 'Glubtok Heroic Gold-Flecked Gloves');

-- Helix encounter
UPDATE `creature_template` SET `ScriptName` = 'boss_helix_gearbreaker' WHERE (`entry` = '47296');
UPDATE `creature_template` SET `ScriptName` = 'npc_lumbering_oaf' WHERE (`entry` = '47297');
UPDATE `creature_template` SET `ScriptName` = 'npc_helix_crew' WHERE (`entry` = '49136'); -- only heroic
UPDATE `creature_template` SET `ScriptName` = 'npc_sticky_bomb' WHERE (`entry` = '47314');
INSERT IGNORE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('88278', 'spell_helix_force_player_to_ride_oaf');

DELETE FROM `creature_text` WHERE `CreatureID` = 47296; -- Helix
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `comment`) VALUES 
('47296', '0', '1', 'The scales...have...tipped...', '14', '100', 'Helix Death'),
('47296', '1', '2', 'Only ten copper? You\'re not even worth killing!', '14', '100', 'Helix Kill'),
('47296', '2', '3', 'I didn\'t need him! Not when I\'ve got YOU oafs!', '14', '100', 'Helix Oaf Dead'),
('47296', '3', '4', 'Bombs away!', '14', '50', 'Helix Bombs Attack'),
('47296', '4', '5', 'Ready Oafie? Throw!', '14', '50', 'Helix Throw Attack'),
('47296', '5', '6', 'The mistress will pay me handsomely for your heads!', '14', '100', 'Helix Aggro');

DELETE FROM `creature_text` WHERE `CreatureID` = 47297; -- Lumbering Oaf
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `comment`) VALUES 
('47297', '0', '1', 'No...NO!', '14', '100', 'Lumbering Oaf charge'),
('47297', '1', '2', 'OAF SMASH!!', '14', '100', 'Lumbering Oaf smash');

-- Helix loot fixes
UPDATE `creature_template_difficulty` SET `LootID` = '4729601' WHERE (`Entry` = '47296') and (`DifficultyID` = '1');
UPDATE `creature_template_difficulty` SET `LootID` = '4729602' WHERE (`Entry` = '47296') and (`DifficultyID` = '2');
DELETE FROM `creature_loot_template` WHERE `Entry` IN (4729601,4729602); -- Helix loot normal and heroic
INSERT INTO `creature_loot_template` VALUES
('4729601', '0', '5199', '25', '0', '1', '0', '1', '1', 'Helix Smelting Pants'),
('4729601', '0', '5443', '22', '0', '1', '0', '1', '1', 'Helix Gold-Plated Buckler'),
('4729601', '0', '5191', '20', '0', '1', '0', '1', '1', 'Helix Cruel Barb'),
('4729601', '0', '5200', '19', '0', '1', '0', '1', '1', 'Helix Impaling Harpoon'),
('4729601', '0', '151062', '6', '0', '1', '0', '1', '1', 'Helix Armbands of Exiled Architects'),
('4729601', '0', '132556', '6', '0', '1', '0', '1', '1', 'Helix Smelters Britches'),
('4729601', '0', '151063', '4', '0', '1', '0', '1', '1', 'Helix Gear-Marked Gauntlets'),

('4729602', '0', '65164', '21', '0', '1', '0', '1', '1', 'Helix Cruel Barb'),
('4729602', '0', '63474', '20', '0', '1', '0', '1', '1', 'Helix Gear-Marked Gauntlets'),
('4729602', '0', '63476', '20', '0', '1', '0', '1', '1', 'Helix Gearbreaker Bindings'),
('4729602', '0', '63475', '20', '0', '1', '0', '1', '1', 'Helix Old Friends Gloves'),
('4729602', '0', '63473', '19', '0', '1', '0', '1', '1', 'Helix Cloak of Thredd'),
('4729602', '0', '157752', '1.4', '0', '1', '0', '1', '1', 'Helix Armbands of Exiled Architects');

-- Kobold Digger fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48229');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48229;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `target_type`, `comment`) VALUES 
('48229', '0', '0', '0', '1', '0', '100', '5000', '5000', '9000', '12000', '11', '317898', '2', 'Kobold Digger Cast Blinding Sleet'),
('48229', '0', '1', '0', '1,2', '0', '100', '5000', '5000', '9000', '12000', '11', '89663', '2', 'Kobold Digger Cast Candle Blast');

-- Ogre Henchmen fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48230');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48230;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `target_type`, `comment`) VALUES 
('48230', '0', '0', '0', '1,2', '0', '100', '5000', '5000', '12000', '14000', '11', '91045', '2', 'Ogre Henchman Cast Uppercut'),
('48230', '0', '1', '0', '1,2', '2', '100', '0', '25', '1000', '1000', '11', '89652', '1', 'Ogre Henchman Cast Ogrish Motivation at 25% HP');

-- Ogre Bodyguard fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48262');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48262;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `target_type`, `comment`) VALUES 
('48262', '0', '0', '0', '1,2', '0', '100', '5000', '5000', '9000', '12000', '11', '91050', '2', 'Ogre Bodyguard Cast Bonk'),
('48262', '0', '1', '0', '1,2', '0', '100', '5000', '5000', '9000', '12000', '11', '91053', '1', 'Ogre Bodyguard Cast Frenzy');

-- Mining Powder fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48284');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48284;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `target_type`, `comment`) VALUES 
('48284', '0', '0', '0', '1,2', '4', '100', '0', '0', '0', '0', '11', '89769', '1', 'Mining Powder Cast Explosion');

-- Goblin Overseer fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48279');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48279;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `target_type`, `comment`) VALUES 
('48279', '0', '0', '0', '1,2', '4', '100', '0', '0', '0', '0', '11', '91034', '1', 'Goblin Overseer Cast Motivate'),
('48279', '0', '1', '0', '1,2', '0', '100', '3000', '4000', '8000', '12000', '11', '91036', '5', 'Goblin Overseer Cast Threatening Shout');

-- Mining Monkey fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48441');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48441;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `target_type`, `comment`) VALUES 
('48441', '0', '0', '0', '1,2', '0', '100', '0', '0', '2300', '3900', '11', '91038', '2', 'Mining Monkey Cast Throw'),
('48441', '0', '1', '2', '1,2', '2', '100', '0', '15', '1000', '1000', '25', '0', '1', 'Mining Monkey Flee 15% HP'),
('48441', '0', '2', '0', '1,2', '61', '100', '0', '15', '0', '0', '1', '0', '1', 'Mining Monkey Say Text at 15% HP');

DELETE FROM `creature_text` WHERE `CreatureID` = 48441;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `comment`) VALUES 
('48441', '0', '1', '%s attempts to run away in fear!', '16', '100', 'Mining Monkey Flee');

-- Oaf Lackey fixes
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '48445');
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48445;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `target_type`, `comment`) VALUES 
('48445', '0', '0', '0', '1,2', '4', '100', '0', '0', '0', '0', '11', '90098', '2', 'Oaf Lackey Cast Axe to the Head on Aggro'),
('48445', '0', '1', '0', '1,2', '2', '100', '0', '30', '1000', '1000', '11', '8599', '1', 'Oaf Lackey Cast Enrage at 30% HP'),
('48445', '0', '2', '0', '1,2', '2', '100', '0', '30', '0', '0', '1', '0', '1', 'Oaf Lackey Say Text at 30% HP');

DELETE FROM `creature_text` WHERE `CreatureID` = 48445;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `comment`) VALUES 
('48445', '0', '1', '%s becomes enraged!', '16', '100', 'Oaf Lackey Enraged');