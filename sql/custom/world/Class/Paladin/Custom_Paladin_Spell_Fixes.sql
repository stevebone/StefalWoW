-- Paladin Spell Fixes
-- Source: StefalWoW

-- Art of War (406064): custom AuraScript handling both effects (base chance + crit bonus)
-- and restoring Blade of Justice charge (ChargeCategory 2128) instead of ResetCooldown.
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_art_of_war_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(406064, 'spell_pal_art_of_war_custom');
