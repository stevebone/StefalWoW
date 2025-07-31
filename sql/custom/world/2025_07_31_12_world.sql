-- Quest: 29678 Shu, the Spirit of Water
-- Spell: 103071 Forcecast Rock Jump A
-- Spell: 103069 Rock Jump A -- this gets the scriptname
-- Spell: 103070 Rock Jump B -- this gets the scriptname
-- Spell: 103072 Forcecast Rock Jump B
-- Spell: 103077 Rock Jump C -- this gets the scriptname
-- Spell: 103078 Forcecast Rock Jump C

DELETE FROM `spell_script_names` WHERE `spell_id` IN (103069,103077,103070);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('103069', 'spell_rock_jump_a');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('103070', 'spell_rock_jump_b');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('103077', 'spell_rock_jump_c');

DELETE FROM `areatrigger_scripts` WHERE `entry` IN (7783);
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES ('7783', 'at_pools_of_reflection');

-- Object required for the pool of reflection
DELETE FROM `gameobject` WHERE `guid` = 900041;
INSERT INTO `gameobject` VALUES
('900041', '209585', '860', '5736', '5862', '0', '0', '0', '0', '-1', '1106.2', '2860.34', '92.189', '0.918553', '-0', '-0', '-0.4433', '-0.896374', '300', '255', '1', '', NULL, '0');
