-- Paladin Spell Fixes
-- Source: StefalWoW

-- Art of War (406064): custom AuraScript handling both effects (base chance + crit bonus)
-- and restoring Blade of Justice charge (ChargeCategory 2128) instead of ResetCooldown.
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_art_of_war_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(406064, 'spell_pal_art_of_war_custom');

-- Righteous Cause (402912): Chance=100 so script handles the per-Holy-Power roll.
-- ProcFlags2=0x4 (CAST_SUCCESSFUL), SpellPhaseMask=1 (CAST), AttributesMask=0x4 (REQ_POWER_COST)
DELETE FROM `spell_proc` WHERE `SpellId` = 402912;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(402912, 0, 0, 0, 0, 0, 0, 0, 0x4, 0, 1, 0, 0x4, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_righteous_cause_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(402912, 'spell_pal_righteous_cause_custom');

-- Consecrated Blade (404834): cast Consecration at target's location on Blade of Justice cast.
-- Also handles 382275 (Art of War variant) which bypasses the 10s ICD.
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_consecrated_blade_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(184575, 'spell_pal_consecrated_blade_custom');
