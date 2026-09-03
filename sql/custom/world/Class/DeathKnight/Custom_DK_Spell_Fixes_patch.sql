-- Blood Draw (374598): register proc script on 374606.
DELETE FROM world.spell_script_names WHERE `ScriptName` = 'spell_dk_blood_draw' AND `spell_id` = 374606;
INSERT INTO world.spell_script_names (`spell_id`, `ScriptName`) VALUES
(374606, 'spell_dk_blood_draw');
