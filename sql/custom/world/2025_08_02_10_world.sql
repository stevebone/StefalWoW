-- Quest: 29678 Shu, the Spirit of Water
-- NPC: 65493 Shu
-- NPC: 60488 Bunny water spout summon
-- Spell: 116810 Summon Bunny Water Spout -- I can't get this one to work so it is not used for now
-- Spell: 117033 Shu Jump to front right
-- Spell: 117034 Shu Jump to front left
-- Spell: 117035 Shu Jump to back right
-- Spell: 117036 Shu Jump to back left

UPDATE `creature_template` SET `AIName` = 'SmartAI', `unit_flags` = '33554944', `unit_flags2` = '2048' WHERE (`entry` = '60488');
UPDATE `creature_template` SET `ScriptName` = 'npc_shu_playing' WHERE (`entry` = '65493');

DELETE FROM `creature_template_addon` WHERE `entry` IN (60488);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('60488', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '85096 116695'); -- Water Spout Bunny at pool2 -- 80797 ????? 

-- these flags disable gravity for the bunny so it can stay above water
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '60488') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '60488') and (`DifficultyID` = '1');

DELETE FROM `creature_addon` WHERE `guid` IN (451090);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('451090', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '85096'); -- Shu at pool2 -- 80797 ????? 

DELETE FROM `spell_script_names` WHERE `spell_id` IN (117033,117034,117035,117036);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('117033', 'spell_jump_to_front_right');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('117034', 'spell_jump_to_front_left');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('117035', 'spell_jump_to_back_right');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('117036', 'spell_jump_to_back_left');

DELETE FROM `smart_scripts` WHERE `entryorguid` IN (60488,6048800); -- 15880 was the original model but not working
INSERT INTO `smart_scripts` VALUES -- we use model 21072 now taken from wowhead
('60488', '0', '0', '1', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '3', '0', '21072', '0', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '', '0', '0', '0', '0', 'Water Spout - Just Spawned - Morph'),
('60488', '0', '1', '0', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '6048800', '2', '0', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '', '0', '0', '0', '0', 'Water Spout - Just Spawned - Run Script'),
('6048800', '9', '0', '0', '', '0', '0', '100', '0', '5000', '5000', '1000', '1000', '0', '', '11', '117057', '2', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Cast Water Spout Geyser Aura'),
('6048800', '9', '1', '0', '', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '11', '116696', '2', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Cast Water Spout Burst'),
('6048800', '9', '2', '0', '', '0', '0', '100', '0', '0', '0', '3000', '3000', '0', '', '28', '116695', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Remove Aura'),
('6048800', '9', '3', '0', '', '0', '0', '100', '0', '3000', '3000', '1000', '1000', '0', '', '41', '0', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Despawn');
