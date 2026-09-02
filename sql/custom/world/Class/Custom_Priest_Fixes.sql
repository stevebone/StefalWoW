-- Power Word: Fortitude (21562) — remove script link
-- Effects are now handled by the core in Midnight. The spell_pri_power_word_fortitude
-- registration in spell_generic.cpp becomes dead code.
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_pri_power_word_fortitude';
