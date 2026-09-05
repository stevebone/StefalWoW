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

-- =========================================================================
-- Divine Storm / Empyrean Power / Tempest / Empyrean Legacy / Second Sunrise
-- =========================================================================

-- Divine Storm (53385): override native spell_pal_divine_storm
DELETE FROM `spell_script_names` WHERE `spell_id`=53385 AND `ScriptName`='spell_pal_divine_storm';
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_divine_storm_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(53385, 'spell_pal_divine_storm_custom');

-- Divine Storm Damage (224239): applies Tempest wave effectiveness + Empyrean Legacy bonus
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_divine_storm_damage_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(224239, 'spell_pal_divine_storm_damage_custom');

-- Empyrean Power (326732): custom AuraScript for proc handling
-- Set Chance=100 so script handles the per-cast roll (old entry had Chance=15 causing double-roll)
DELETE FROM `spell_proc` WHERE `SpellId` = 326732;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(326732, 0, 10, 0x0, 0x00008000, 0x0, 0x0, 0x10, 0, 1, 2, 0x403, 0, 0, 0, 100, 0, 0);

-- Empyrean Power buff (326733): remove old spell_proc (buff consumption handled in script)
DELETE FROM `spell_proc` WHERE `SpellId` = 326733;

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_empyrean_power_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(326732, 'spell_pal_empyrean_power_custom');

-- Empyrean Legacy (387170): proc on Avenging Wrath cast, Chance=100 (script does filtering)
DELETE FROM `spell_proc` WHERE `SpellId` = 387170;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(387170, 0, 0, 0, 0, 0, 0, 0, 0x4, 0, 1, 0, 0, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_empyrean_legacy_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(387170, 'spell_pal_empyrean_legacy_custom');

-- Empyrean Legacy buff (387178): consumed by ST HP spender, triggers empowered DS
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_empyrean_legacy_buff_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(387178, 'spell_pal_empyrean_legacy_buff_custom');

-- Empyrean Legacy spender fallback (Templar's Verdict, Justicar's Vengeance, Final Verdict)
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_empyrean_legacy_spender_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(85256, 'spell_pal_empyrean_legacy_spender_custom'),
(215661, 'spell_pal_empyrean_legacy_spender_custom'),
(383328, 'spell_pal_empyrean_legacy_spender_custom');

-- Second Sunrise (431474): echo proc, Chance=100 (script does filtering + roll)
DELETE FROM `spell_proc` WHERE `SpellId` = 431474;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(431474, 0, 0, 0, 0, 0, 0, 0, 0x4, 0, 1, 0, 0, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_second_sunrise_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(431474, 'spell_pal_second_sunrise_custom');

-- Second Sunrise effectiveness: applies echo modifier to damage/healing on all echoable spells
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_second_sunrise_effectiveness_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(53385, 'spell_pal_second_sunrise_effectiveness_custom'),
(224239, 'spell_pal_second_sunrise_effectiveness_custom'),
(24275, 'spell_pal_second_sunrise_effectiveness_custom'),
(1241413, 'spell_pal_second_sunrise_effectiveness_custom'),
(20473, 'spell_pal_second_sunrise_effectiveness_custom'),
(25912, 'spell_pal_second_sunrise_effectiveness_custom'),
(25914, 'spell_pal_second_sunrise_effectiveness_custom'),
(85222, 'spell_pal_second_sunrise_effectiveness_custom');

-- Holy Shock (20473): override native spell_pal_holy_shock to forward m_customArg
DELETE FROM `spell_script_names` WHERE `spell_id`=20473 AND `ScriptName`='spell_pal_holy_shock';
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_holy_shock_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(20473, 'spell_pal_holy_shock_custom');
