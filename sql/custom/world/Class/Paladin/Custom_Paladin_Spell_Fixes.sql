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

-- Second Sunrise effectiveness: applies echo modifier to damage/healing on echoable spells
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_second_sunrise_effectiveness_custom';
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_second_sunrise_effectiveness_damage';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(53385, 'spell_pal_second_sunrise_effectiveness_damage'),
(224239, 'spell_pal_second_sunrise_effectiveness_damage'),
(24275, 'spell_pal_second_sunrise_effectiveness_damage'),
(1241413, 'spell_pal_second_sunrise_effectiveness_damage'),
(25912, 'spell_pal_second_sunrise_effectiveness_damage');

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_second_sunrise_effectiveness_healing';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(85222, 'spell_pal_second_sunrise_effectiveness_healing'),
(25914, 'spell_pal_second_sunrise_effectiveness_healing');

-- Holy Shock (20473): override native spell_pal_holy_shock to forward m_customArg
DELETE FROM `spell_script_names` WHERE `spell_id`=20473 AND `ScriptName`='spell_pal_holy_shock';
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_holy_shock_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(20473, 'spell_pal_holy_shock_custom');

-- =========================================================================
-- Dawnlight (Herald of the Sun hero talent)
-- =========================================================================

-- Dawnlight charges (431522): procs on HP spender hit, applies Dawnlight DoT/HoT
-- ProcFlags=0x15410 (DEAL_MELEE_ABILITY | DEAL_HELPFUL_ABILITY | DEAL_HARMFUL_ABILITY | DEAL_HELPFUL_SPELL | DEAL_HARMFUL_SPELL)
-- SpellPhaseMask=2 (HIT) so GetActionTarget() is non-null
-- AttributesMask=0x4 (REQ_POWER_COST) to only proc on HP spenders
DELETE FROM `spell_proc` WHERE `SpellId` = 431377;
DELETE FROM `spell_proc` WHERE `SpellId` = 431522;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(431522, 0, 0, 0, 0, 0, 0, 0x15410, 0, 0, 2, 0, 0x4, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_dawnlight_custom';
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_dawnlight_trigger_custom';
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_dawnlight_charges';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(431522, 'spell_pal_dawnlight_charges');

-- Morning Star (431482): periodic driver that stacks damage/healing buff
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_morning_star';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(431482, 'spell_pal_morning_star');

-- Will of the Dawn (431406): 5% speed above 80% HP, 40% burst below 35% HP with 1min ICD
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_will_of_the_dawn';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(431406, 'spell_pal_will_of_the_dawn');

-- Dawnlight damage DoT (431380): radiates damage on tick
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_dawnlight_damage_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(431380, 'spell_pal_dawnlight_damage_custom');

-- Dawnlight healing HoT (431381): radiates healing on tick
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_dawnlight_heal_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(431381, 'spell_pal_dawnlight_heal_custom');

-- Dawnlight damage radiation (431399): AoE damage with target cap
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_dawnlight_radiation_damage_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(431399, 'spell_pal_dawnlight_radiation_damage_custom');

-- Dawnlight heal radiation (431382): AoE heal with target cap + Beacon exclusion
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_dawnlight_radiation_heal_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(431382, 'spell_pal_dawnlight_radiation_heal_custom');

-- Light's Beacon (53651): override native to exclude Dawnlight healing from Beacon transfer
DELETE FROM `spell_script_names` WHERE `spell_id`=53651 AND `ScriptName`='spell_pal_light_s_beacon';
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_light_s_beacon_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(53651, 'spell_pal_light_s_beacon_custom');

-- Expurgation (383344): should only proc from Blade of Justice (184575) and its AOE (404358).
-- DB2 EffectSpellClassMask is 0, so auto-generated proc has no family filter (procs from everything).
-- spell_proc Chance=100 + AuraScript CheckProc filters by exact spell ID.
DELETE FROM `spell_proc` WHERE `SpellId` = 383344;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(383344, 0, 0, 0, 0, 0, 0, 0x10, 0, 0, 2, 0, 0, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_expurgation_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(383344, 'spell_pal_expurgation_custom');

-- Hammer of Wrath Talent (1241288): replaces Judgment with Hammer of Wrath during Avenging Wrath.
-- spell_pal_hammer_of_wrath_avenging_wrath hooks all Avenging Wrath variants.
-- spell_pal_hammer_of_wrath_talent hooks the talent passive aura.
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_hammer_of_wrath_avenging_wrath';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(31884, 'spell_pal_hammer_of_wrath_avenging_wrath'),
(231895, 'spell_pal_hammer_of_wrath_avenging_wrath'),
(454351, 'spell_pal_hammer_of_wrath_avenging_wrath'),
(454373, 'spell_pal_hammer_of_wrath_avenging_wrath');

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_hammer_of_wrath_talent';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1241288, 'spell_pal_hammer_of_wrath_talent');

-- =========================================================================
-- Radiant Glory (458359): Wake of Ashes activates Avenging Wrath
-- =========================================================================

-- spell_proc: proc on successful cast (ProcFlags2=0x4), Chance=100 (script filters to Wake of Ashes only)
DELETE FROM `spell_proc` WHERE `SpellId` = 458359;
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(458359, 0, 0, 0, 0, 0, 0, 0, 0x4, 0, 1, 0, 0, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_radiant_glory';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(458359, 'spell_pal_radiant_glory');

-- =========================================================================
-- Grand Crusader (85043)
-- =========================================================================

-- Remove any existing native script mapping for spell 85043
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_grand_crusader';
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_grand_crusader_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(85043, 'spell_pal_grand_crusader_custom');

-- spell_proc: add DEAL_MELEE_ABILITY (0x10) to the DBC's TAKE_MELEE_SWING (0x08) + TAKE_MELEE_ABILITY (0x20)
-- HitMask includes dodge/parry/miss/block/full_block (for avoids) + normal/critical (for CS/HoTR hits)
-- Chance=100 so the AuraScript handles the 15% roll
-- SpellFamilyName=0 so taken melee from enemies isn't filtered by family (C++ handles deal-melee filtering)
DELETE FROM `spell_proc` WHERE `SpellId` = 85043;
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(85043, 0, 0, 0, 0, 0, 0, 0x38, 0, 0, 0, 0x2077, 0, 0, 0, 100, 0, 0);
