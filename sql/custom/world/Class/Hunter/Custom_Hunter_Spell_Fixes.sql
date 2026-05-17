-- Hunter spell script bindings

-- Call Pet Spells
DELETE FROM `spell_script_names` WHERE `spell_id` IN (883,83242,83243,83244,83245);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(883, 'spell_hun_call_pet'),
(83242, 'spell_hun_call_pet'),
(83243, 'spell_hun_call_pet'),
(83244, 'spell_hun_call_pet'),
(83246, 'spell_hun_call_pet');

-- Kill Command
DELETE FROM `spell_script_names` WHERE `spell_id` IN (34026,259489,83381);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(34026, 'spell_hun_kill_command'),
(259489, 'spell_hun_kill_command'),
(83381, 'spell_hun_kill_command_proc');