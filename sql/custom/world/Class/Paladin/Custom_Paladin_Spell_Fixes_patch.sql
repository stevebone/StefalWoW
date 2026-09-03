-- Empyrean Legacy (387170): AW grants buff; next ST Holy Power spender casts empowered Divine Storm.
DELETE FROM world.spell_script_names WHERE `ScriptName` IN (
'spell_pal_empyrean_legacy',
'spell_pal_empyrean_legacy_buff',
'spell_pal_empyrean_legacy_spender',
'spell_pal_divine_storm_damage'
);
INSERT INTO world.spell_script_names (`spell_id`, `ScriptName`) VALUES
(387170, 'spell_pal_empyrean_legacy'),
(387178, 'spell_pal_empyrean_legacy_buff'),
(85256, 'spell_pal_empyrean_legacy_spender'),
(383328, 'spell_pal_empyrean_legacy_spender'),
(215661, 'spell_pal_empyrean_legacy_spender'),
(224239, 'spell_pal_divine_storm_damage');

-- Empyreal Ward (387791): only proc from Lay on Hands (633 / 471195).
DELETE FROM world.spell_script_names WHERE `ScriptName` = 'spell_pal_empyreal_ward';
INSERT INTO world.spell_script_names (`spell_id`, `ScriptName`) VALUES
(387791, 'spell_pal_empyreal_ward');



-- Expurgation (383344): restrict DoT application to Blade of Justice only.

DELETE FROM world.spell_script_names WHERE `ScriptName` = 'spell_pal_expurgation' AND `spell_id` = 383344;

INSERT INTO world.spell_script_names (`spell_id`, `ScriptName`) VALUES

(383344, 'spell_pal_expurgation');



-- Hammer of Wrath (1241288): during Avenging Wrath, Judgment is replaced by Hammer of Wrath.

DELETE FROM world.spell_script_names WHERE `ScriptName` IN ('spell_pal_hammer_of_wrath_talent', 'spell_pal_hammer_of_wrath_override', 'spell_pal_hammer_of_wrath_avenging_wrath');

INSERT INTO world.spell_script_names (`spell_id`, `ScriptName`) VALUES

(1241288, 'spell_pal_hammer_of_wrath_talent'),

(31884, 'spell_pal_hammer_of_wrath_avenging_wrath'),

(231895, 'spell_pal_hammer_of_wrath_avenging_wrath'),

(454351, 'spell_pal_hammer_of_wrath_avenging_wrath'),

(454373, 'spell_pal_hammer_of_wrath_avenging_wrath');



-- 384376 is a passive hidden aura, not active Avenging Wrath.

DELETE FROM world.spell_script_names WHERE `spell_id` = 384376 AND `ScriptName` = 'spell_pal_hammer_of_wrath_avenging_wrath';



-- 1277026 override aura is no longer used; action bar swap is server-side only.

DELETE FROM world.spell_script_names WHERE `spell_id` = 1277026 AND `ScriptName` = 'spell_pal_hammer_of_wrath_override';

-- Radiant Glory (462048): Holy Power spenders can proc Avenging Wrath via spell_pal_radiant_glory PlayerScript.

-- Dawnlight (431377): Wake of Ashes / Holy Prism / Divine Toll grant charges; HP spenders apply Dawnlight.
DELETE FROM world.spell_script_names WHERE `ScriptName` IN (
'spell_pal_dawnlight',
'spell_pal_dawnlight_empower',
'spell_pal_dawnlight_radiate_meta',
'spell_pal_dawnlight_charges',
'spell_pal_dawnlight_damage'
);
INSERT INTO world.spell_script_names (`spell_id`, `ScriptName`) VALUES
(431377, 'spell_pal_dawnlight'),
(431460, 'spell_pal_dawnlight_empower'),
(431581, 'spell_pal_dawnlight_radiate_meta'),
(431522, 'spell_pal_dawnlight_charges'),
(431380, 'spell_pal_dawnlight_damage');

-- Consecrated Blade (404834 / 462970): Blade of Justice casts Consecration at target (ICD 407475).
DELETE FROM world.spell_script_names WHERE `ScriptName` IN (
'spell_pal_consecrated_blade',
'spell_pal_consecration_position'
);
INSERT INTO world.spell_script_names (`spell_id`, `ScriptName`) VALUES
(184575, 'spell_pal_consecrated_blade'),
(404358, 'spell_pal_consecrated_blade');

-- Art of War (406064) / Righteous Cause (402912): reset Blade of Justice cooldown on proc.
DELETE FROM world.spell_script_names WHERE `ScriptName` IN ('spell_pal_art_of_war', 'spell_pal_righteous_cause');
INSERT INTO world.spell_script_names (`spell_id`, `ScriptName`) VALUES
(267344, 'spell_pal_art_of_war'),
(406064, 'spell_pal_art_of_war'),
(402912, 'spell_pal_righteous_cause');

-- Second Sunrise (431474): DS/HoW/Holy Shock/Light of Dawn can echo at reduced effectiveness.
DELETE FROM world.spell_script_names WHERE `ScriptName` IN (
'spell_pal_second_sunrise',
'spell_pal_second_sunrise_effectiveness'
);
INSERT INTO world.spell_script_names (`spell_id`, `ScriptName`) VALUES
(431474, 'spell_pal_second_sunrise'),
(53385, 'spell_pal_second_sunrise_effectiveness'),
(224239, 'spell_pal_second_sunrise_effectiveness'),
(1241413, 'spell_pal_second_sunrise_effectiveness'),
(24275, 'spell_pal_second_sunrise_effectiveness'),
(25912, 'spell_pal_second_sunrise_effectiveness'),
(25914, 'spell_pal_second_sunrise_effectiveness'),
(85222, 'spell_pal_second_sunrise_effectiveness');

-- Empyrean Power (326732): CS/Templar Strike 407480/Templar Slash 406647 / Crusading Strikes 404542->408385 all at 15% -> buff 326733.
DELETE FROM world.spell_script_names WHERE `ScriptName` = 'spell_pal_empyrean_power';
INSERT INTO world.spell_script_names (`spell_id`, `ScriptName`) VALUES
(326732, 'spell_pal_empyrean_power');

-- Allow script to roll CS 15% / Crusading Strikes 5% (DBC ProcChance is 100%).
DELETE FROM world.spell_proc WHERE `SpellId` IN (326732, 326733);
INSERT INTO world.spell_proc (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(326732, 0, 0, 0, 0, 0, 0, 0x00000014, 0, 1, 2, 0, 0, 0, 0, 100, 100, 0);

