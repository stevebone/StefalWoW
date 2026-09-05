-- Death Knight Spell Fixes
-- Source: StefalWoW

-- Blood Draw (374598): 2 min internal cooldown enforced via ICD aura 374609.
-- 374606 (drain) gates on the ICD and applies ICD + 454871 (buff) synchronously.
-- 454871 rejects the raw talent trigger so it shares the ICD with the drain.
DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dk_blood_draw_drain', 'spell_dk_blood_draw_buff');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(374606, 'spell_dk_blood_draw_drain'),
(454871, 'spell_dk_blood_draw_buff');
