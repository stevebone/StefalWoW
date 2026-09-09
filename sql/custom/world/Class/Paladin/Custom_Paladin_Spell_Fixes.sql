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

-- =========================================================================
-- Holy Prism (114852/114871) - fixed EFFECT_2 target mismatch
-- =========================================================================
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_holy_prism_selector';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(114852, 'spell_pal_holy_prism_selector'),
(114871, 'spell_pal_holy_prism_selector');

-- =========================================================================
-- Sun's Avatar (431425): beam linking to Dawnlights
-- =========================================================================
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_suns_avatar_link';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(431907, 'spell_pal_suns_avatar_link');

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_suns_avatar_self_link';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(463073, 'spell_pal_suns_avatar_self_link');

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_suns_avatar_damage';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(431911, 'spell_pal_suns_avatar_damage');

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_suns_avatar_heal';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(431939, 'spell_pal_suns_avatar_heal');

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_suns_avatar_self_damage';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(463075, 'spell_pal_suns_avatar_self_damage');

-- =========================================================================
-- Judge, Jury and Executioner (406157)
-- =========================================================================
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_judge_jury_executioner';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(406157, 'spell_pal_judge_jury_executioner');

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_judge_jury_execution_sentence';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(343527, 'spell_pal_judge_jury_execution_sentence');

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_judge_jury_executioner_buff';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1253174, 'spell_pal_judge_jury_executioner_buff');

DELETE FROM `spell_proc` WHERE `SpellId` = 1253174;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(1253174, 0, 0, 0, 0, 0, 0, 0, 0x4, 0, 1, 0, 0, 0, 0, 100, 0, 0);

-- =========================================================================
-- Righteous Protector (204074) - fixed aura hook mismatch
-- =========================================================================
DELETE FROM `spell_proc` WHERE `SpellId` = 204074;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(204074, 0, 0, 0, 0, 0, 0, 0, 0x4, 0, 1, 0, 0x4, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_righteous_protector';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(204074, 'spell_pal_righteous_protector');

-- Fix Native Scripts Registration
-- Heartfire heal has incorrect spell id in script registration
UPDATE `spell_script_names` SET `spell_id` = 408461 WHERE `ScriptName` = 'spell_pal_t30_2p_protection_bonus_heal' AND `spell_id` = 410530;

-- =========================================================================
-- Execution Sentence (343527) — moved from core, fixed proc filters
-- 1260251 - Radiate tracking aura on blast-affected enemies
-- =========================================================================
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_execution_sentence_radiate';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1260251, 'spell_pal_execution_sentence_radiate');

DELETE FROM `spell_proc` WHERE `SpellId` = 1260251;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(1260251, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 100, 0, 0);

-- =========================================================================
-- Walk Into Light (1263782) — Herald of the Sun talent
-- Ret: AW → Blessing of An'she + 2 HP; HoW → BoJ during wings.
-- Holy: Infusion of Light procs 2x during AW.
-- =========================================================================
DELETE FROM `spell_script_names` WHERE `ScriptName` IN (
    'spell_pal_walk_into_light',
    'spell_pal_walk_into_light_avenging_wrath',
    'spell_pal_walk_into_light_hammer_of_wrath',
    'spell_pal_walk_into_light_infusion'
);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1263782, 'spell_pal_walk_into_light'),
(31884, 'spell_pal_walk_into_light_avenging_wrath'),
(231895, 'spell_pal_walk_into_light_avenging_wrath'),
(454351, 'spell_pal_walk_into_light_avenging_wrath'),
(454373, 'spell_pal_walk_into_light_avenging_wrath'),
(1241413, 'spell_pal_walk_into_light_hammer_of_wrath'),
(24275, 'spell_pal_walk_into_light_hammer_of_wrath'),
(53576, 'spell_pal_walk_into_light_infusion');

-- =========================================================================
-- Divine Toll (375576)
-- Holy: Holy Shock on up to 5 targets; Prot: Avenger's Shield on up to 5 enemies;
-- Ret: Judgment on up to 5 enemies with +50% damage.
-- =========================================================================
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_divine_toll';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(375576, 'spell_pal_divine_toll');

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_divine_toll_judgment';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(20271, 'spell_pal_divine_toll_judgment'),
(275779, 'spell_pal_divine_toll_judgment'),
(275773, 'spell_pal_divine_toll_judgment');

-- =========================================================================
-- Crusading Strikes (408385) - EFFECT_1 DUMMY: generate 1 Holy Power every other attack
-- =========================================================================
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_crusading_strikes_damage';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(408385, 'spell_pal_crusading_strikes_damage');

-- =========================================================================
-- Light of the Titans (378405) - procs on Word of Glory, heals target as HoT
-- =========================================================================
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_light_of_the_titans';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(378405, 'spell_pal_light_of_the_titans');

-- Light of the Titans HoT (378412) - dynamic amount (canBeRecalculated = false)
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_light_of_the_titans_hot';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(378412, 'spell_pal_light_of_the_titans_hot');

-- =========================================================================
-- Holy Ritual (199422) - attached to Blessing of Sacrifice (6940) and Blessing of Protection (1022)
-- =========================================================================
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_holy_ritual';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(6940, 'spell_pal_holy_ritual'),
(1022, 'spell_pal_holy_ritual');

-- =========================================================================
-- Judgment of Justice (403495) - attached to Judgment (20271)
-- =========================================================================
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_judgment_of_justice';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(20271, 'spell_pal_judgment_of_justice');

-- =========================================================================
-- Punishment (403530) - procs on Rebuke/Avenger's Shield interrupt
-- EFFECT_0 DUMMY: casts extra Blessed Hammer/HoTR/Holy Shock/Crusader Strike
-- EFFECT_1 PROC_TRIGGER_SPELL: TriggerSpell=0, suppressed by PreventDefaultAction
-- =========================================================================
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_punishment';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(403530, 'spell_pal_punishment');

-- =========================================================================
-- Guided Prayer (404357) - procs when health drops below 25%
-- EFFECT_0 PROC_TRIGGER_SPELL: TriggerSpell=0, suppressed by PreventDefaultAction
-- EFFECT_1 DUMMY: casts Word of Glory at 60% effectiveness on self
-- =========================================================================
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pal_guided_prayer';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(404357, 'spell_pal_guided_prayer');

-- =========================================================================
-- Crusading Strikes marker aura (406833) - suppress proc warning
-- EFFECT_0 PROC_TRIGGER_SPELL: TriggerSpell=0, used only as marker by script
-- DisableEffectsMask=1 prevents the proc from firing, suppressing the warning
-- =========================================================================
DELETE FROM `spell_proc` WHERE `SpellId` = 406833;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(406833, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0);

-- =========================================================================
-- Empyreal Ward (387791) - restrict proc to Lay on Hands only
-- EFFECT_0 PROC_TRIGGER_SPELL: TriggerSpell=387792 (armor buff)
-- Auto-generated proc has SpellFamilyName=0 (no filter), so it procs on any
-- spell including Avenging Wrath. Restrict to Paladin SpellFamilyMask0=0x8000
-- (Lay on Hands) with ProcFlags2=CAST_SUCCESSFUL, SpellPhaseMask=CAST.
-- =========================================================================
DELETE FROM `spell_proc` WHERE `SpellId` = 387791;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(387791, 0, 10, 0x8000, 0, 0, 0, 0, 0x4, 0, 1, 0, 0, 0, 0, 100, 0, 0);
