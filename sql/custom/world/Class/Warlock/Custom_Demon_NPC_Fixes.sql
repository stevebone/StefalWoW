-- Warlock Spell Fixes
-- Source: RetailCore && DoomCore

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warl_diabolic_ritual_passive', 'spell_warl_diabolic_ritual', 'spell_warl_demonic_art', 'spell_warl_pit_lord_felseeker', 'spell_warl_pit_lord_felseeker_at', 'spell_warl_mother_chaos_missile', 'spell_warl_overlord_wicked_cleave', 'spell_warl_infernal_bolt_empower_aura', 'spell_warl_overfiend_chaos_bolt', 'spell_warl_avatar_of_destruction', 'spell_warl_dimensional_rift_talent', 'spell_warl_ruination', 'spell_warl_ruination_damage', 'spell_warl_ruination_entry_aura', 'spell_warl_diabolic_oculi');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(428514, 'spell_warl_diabolic_ritual_passive'),
(431944, 'spell_warl_diabolic_ritual'),
(432815, 'spell_warl_diabolic_ritual'),
(432816, 'spell_warl_diabolic_ritual'),
(428524, 'spell_warl_demonic_art'),
(432794, 'spell_warl_demonic_art'),
(432795, 'spell_warl_demonic_art'),
(438973, 'spell_warl_pit_lord_felseeker'),
(434404, 'spell_warl_pit_lord_felseeker_at'),
(432596, 'spell_warl_mother_chaos_missile'),
(432113, 'spell_warl_overlord_wicked_cleave'),
(432120, 'spell_warl_overlord_wicked_cleave'),
(433891, 'spell_warl_infernal_bolt_empower_aura'),
(434589, 'spell_warl_overfiend_chaos_bolt'),
(1245089, 'spell_warl_avatar_of_destruction'),
(1280868, 'spell_warl_dimensional_rift_talent'),
(433885, 'spell_warl_ruination_entry_aura'),
(434635, 'spell_warl_ruination'),
(434636, 'spell_warl_ruination_damage'),
(1269643, 'spell_warl_diabolic_oculi');

DELETE FROM `spell_proc` WHERE `SpellId` IN (428514, 428524, 432794, 432795, 1245089, 1280868);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(428514, 0, 5, 0, 0, 0, 0, 0x10000, 0, 1, 2, 0, 0, 0, 0, 100, 0, 0),
(428524, 0, 0, 0, 0, 0, 0, 0, 0x4, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(432794, 0, 0, 0, 0, 0, 0, 0, 0x4, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(432795, 0, 0, 0, 0, 0, 0, 0, 0x4, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(1245089, 0, 5, 0, 0, 0, 0, 0x10000, 0, 1, 2, 0, 0, 0, 0, 100, 0, 0),
(1280868, 0, 5, 0, 0, 0, 0, 0x10000, 0, 1, 2, 0, 0, 0, 0, 10, 0, 0);

UPDATE `creature_template` SET `ScriptName` = 'npc_warlock_dreadstalker' WHERE `Entry` = 98035; -- Dreadstalker
UPDATE `creature_template` SET `ScriptName` = 'npc_pet_warlock_demonic_tyrant' WHERE `Entry` = 135002; -- // Demonic Tyrant - 135002
UPDATE `creature_template` SET `ScriptName` = 'npc_warl_diabolic_imp' WHERE `Entry` = 219161; -- // 219161 - Diabolic Imp
UPDATE `creature_template` SET `ScriptName` = 'npc_pet_warlock_wild_imp' WHERE `Entry` = 55659; -- // 55659 - Wild Imp
UPDATE `creature_template` SET `ScriptName` = 'npc_warl_dimensional_rift_shadowy_tear' WHERE `Entry` = 198547; -- 198547 - Shadowy Tear
UPDATE `creature_template` SET `ScriptName` = 'npc_warl_dimensional_rift_unstable_tear' WHERE `Entry` = 196280; -- 196280 - Unstable Tear
UPDATE `creature_template` SET `ScriptName` = 'npc_warl_dimensional_rift_chaos_tear' WHERE `Entry` = 108493; -- 108493 - Chaos Tear
UPDATE `creature_template` SET `ScriptName` = 'npc_warl_diabolist_overlord' WHERE `Entry` = 228575; -- // 228575 - Overlord
UPDATE `creature_template` SET `ScriptName` = 'npc_warl_diabolist_mother_of_chaos' WHERE `Entry` = 228576; -- // 228576 - Mother of Chaos
UPDATE `creature_template` SET `ScriptName` = 'npc_warl_avatar_of_destruction_overfiend' WHERE `Entry` = 217429; -- // 217429 - Overfiend
UPDATE `creature_template` SET `ScriptName` = 'npc_warl_diabolist_pit_lord' WHERE `Entry` = 228574; -- // 228574 - Pit Lord
UPDATE `creature_template` SET `ScriptName` = 'npc_pet_warlock_darkglare' WHERE `Entry` = 103673; -- // 103673 - Darkglare
UPDATE `creature_template` SET `ScriptName` = 'npc_warl_demonic_gateway' WHERE `Entry` = 59262; -- // 59262 - Demonic Gateway
UPDATE `creature_template` SET `ScriptName` = 'npc_warl_demonic_gateway' WHERE `Entry` = 59271; -- // 59271 - Demonic Gateway

DELETE FROM `areatrigger_template`
WHERE `IsCustom` = 0
  AND `Id` IN (
    3921, 36160, 3282, 3319, 3678, 5300, 5302, 5972, 7020,
    9397, 10133, 10466, 11457, 12740, 16552, 30719, 35067,
    36050, 37662, 38618, 39672, 39871, 41475, 41750
);

INSERT INTO `areatrigger_template`
(`Id`, `IsCustom`, `Flags`, `ActionSetId`, `ActionSetFlags`, `VerifiedBuild`)
VALUES
    (3921, 0, 0, 0, 0, 65299),
    (36160, 0, 0, 0, 0, 65299),
    (3282, 0, 0, 0, 0, 66263),
    (3319, 0, 0, 0, 0, 66263),
    (3678, 0, 0, 0, 0, 66263),
    (5300, 0, 0, 0, 0, 66263),
    (5302, 0, 0, 0, 0, 66263),
    (5972, 0, 0, 0, 0, 66263),
    (7020, 0, 0, 0, 0, 66263),
    (9397, 0, 0, 0, 0, 66263),
    (10133, 0, 0, 0, 0, 66263),
    (10466, 0, 0, 0, 0, 66263),
    (11457, 0, 0, 0, 0, 66263),
    (12740, 0, 0, 0, 0, 66263),
    (16552, 0, 0, 0, 0, 66263),
    (30719, 0, 0, 0, 0, 66263),
    (35067, 0, 0, 0, 0, 66263),
    (36050, 0, 0, 0, 0, 66263),
    (37662, 0, 0, 0, 0, 66263),
    (38618, 0, 0, 0, 0, 66263),
    (39672, 0, 0, 0, 0, 66263),
    (39871, 0, 0, 0, 0, 66263),
    (41475, 0, 0, 0, 0, 66263),
    (41750, 0, 0, 0, 0, 66263);
	
DELETE FROM `areatrigger_create_properties_spline_point`
WHERE `AreaTriggerCreatePropertiesId` = 900274
  AND `IsCustom` = 1
  AND `Idx` IN (
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15, 16, 17, 18
);

INSERT INTO `areatrigger_create_properties_spline_point`
(`AreaTriggerCreatePropertiesId`, `IsCustom`, `Idx`, `X`, `Y`, `Z`, `VerifiedBuild`)
VALUES
    (900274, 1, 0, 0, 0, 0, 66263),
    (900274, 1, 1, 0, 0, 0, 66263),
    (900274, 1, 2, 2.40611, -0.0313201, -0.148239, 66263),
    (900274, 1, 3, 4.81222, -0.0626402, -0.00296021, 66263),
    (900274, 1, 4, 7.21833, -0.0939603, -0.131355, 66263),
    (900274, 1, 5, 9.62444, -0.12528, -0.154251, 66263),
    (900274, 1, 6, 12.0306, -0.1566, -1.43166, 66263),
    (900274, 1, 7, 14.6347, -0.755443, -2.81906, 66263),
    (900274, 1, 8, 17.0275, -0.729853, -2.81906, 66263),
    (900274, 1, 9, 19.4167, -0.704479, -2.81906, 66263),
    (900274, 1, 10, 21.8017, -0.679022, -2.81906, 66263),
    (900274, 1, 11, 24.18, -0.653373, -2.81906, 66263),
    (900274, 1, 12, 26.5485, -0.628046, -2.81906, 66263),
    (900274, 1, 13, 28.901, -0.602687, -2.81906, 66263),
    (900274, 1, 14, 31.2257, -0.577806, -2.35486, 66263),
    (900274, 1, 15, 33.5374, -0.552839, -1.63754, 66263),
    (900274, 1, 16, 35.8548, -0.527929, -0.257195, 66263),
    (900274, 1, 17, 38.261, -0.502262, 1.17984, 66263),
    (900274, 1, 18, 38.261, -0.502262, 1.17984, 66263);
	
DELETE FROM `areatrigger_create_properties`
WHERE `IsCustom` = 1
  AND `Id` IN (
    900060, 900155, 900158, 900159, 900160, 900161, 900162,
    900165, 900183, 900185, 900196, 900197, 900198, 900200,
    900207, 900270, 900271, 900272
);

INSERT INTO `areatrigger_create_properties`
(`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`, `ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `SpellForVisuals`, `TimeToTargetScale`, `Speed`, `SpeedIsTime`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`, `ShapeData5`, `ShapeData6`, `ShapeData7`, `ScriptName`, `VerifiedBuild`)
VALUES
    (900060, 1, 16552, 0, 0, 0, 0, 0, 0, -1, 0, 0, 255545, 2000, 0, 0, 4, 8, 8, 2, 2, 0.3, 0.3, 0, 0, '', 66263),
    (900155, 1, 3678, 0, 0, 0, 0, 0, 0, -1, 0, 0, 132950, 20000, 0, 0, 4, 10, 10, 7, 7, 3, 3, 0, 0, '', 66263),
    (900158, 1, 9397, 0, 5, 0, 0, 0, 0, -1, 0, 0, 190356, 0, 0, 0, 4, 12, 12, 4, 4, 0.3, 0.3, 0, 0, '', 66263),
    (900159, 1, 5972, 0, 0, 0, 0, 0, 0, -1, 0, 0, 109248, 10000, 0, 0, 4, 5, 5, 2, 2, 0.3, 0.3, 0, 0, '', 66263),
    (900160, 1, 12740, 0, 24, 0, 0, 0, 0, -1, 0, 0, 84714, 15000, 3.00034, 0, 0, 9.5, 9.5, 0, 0, 0, 0, 0, 0, '', 66263),
    (900161, 1, 41750, 0, 4, 0, 0, 0, 0, -1, 0, 0, 1266082, 10000, 24.9887, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, '', 66263),
    (900162, 1, 3678, 0, 0, 0, 0, 0, 0, -1, 0, 0, 132950, 20000, 0, 0, 4, 10, 10, 7, 7, 3, 3, 0, 0, '', 66263),
    (900165, 1, 10466, 0, 24, 0, 0, 0, 0, -1, 0, 0, 201591, 2500, 19.2243, 0, 4, 6, 6, 4, 4, 0.3, 0.3, 0, 0, '', 66263),
    (900183, 1, 37662, 0, 24, 0, 0, 0, 0, -1, 0, 0, 471947, 3000, 25.0119, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, '', 66263),
    (900185, 1, 37662, 0, 24, 0, 0, 0, 0, -1, 0, 0, 471947, 3000, 25.0029, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, '', 66263),
    (900196, 1, 39672, 0, 2, 0, 0, 0, 0, -1, 0, 0, 198034, 8000, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, '', 66263),
    (900197, 1, 41475, 0, 0, 0, 0, 0, 0, -1, 0, 0, 1259298, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, '', 66263),
    (900198, 1, 35067, 0, 0, 0, 0, 0, 0, -1, 0, 0, 414114, 12000, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, '', 66263),
    (900200, 1, 11457, 0, 0, 0, 0, 0, 0, -1, 0, 0, 212269, 6000, 1, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, '', 66263),
    (900207, 1, 39871, 0, 0, 0, 0, 0, 0, -1, 0, 0, 1239621, 0, 0, 0, 0, 5, 5, 0, 0, 0, 0, 0, 0, '', 66263),
    (900270, 1, 10133, 0, 0, 0, 0, 0, 0, -1, 0, 0, 1214467, 6677, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, '', 66263),
    (900271, 1, 3282, 0, 0, 0, 0, 0, 0, -1, 0, 0, 124503, 30000, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '', 66263),
    (900272, 1, 36050, 0, 0, 0, 0, 0, 0, -1, 0, 0, 452225, 2000, 0, 0, 1, 5, 1.5, 5, 5, 1.5, 5, 0, 0, '', 66263);
	
DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warl_darkglare_eye_laser', 'spell_warl_hand_of_guldan', 'spell_warl_fear', 'spell_warl_fear_buff', 'spell_warl_corruption_effect', 'spell_warl_drain_life', 
'aura_warl_phantomatic_singularity', 'aura_warl_haunt', 'spell_warlock_summon_darkglare', 'spell_warlock_unending_breath', 'spell_warl_demonic_gateway', 'spell_warl_hand_of_guldan_damage', 'spell_warlock_call_dreadstalkers' 
'spell_warlock_demonbolt_new', 'spell_warl_demonic_calling', 'spell_warl_implosion', 'spell_warlock_doom', 'spell_warlock_soul_fire', 'spell_warl_soul_conduit', 'spell_warr_shadowbolt_affliction', 
'spell_warlock_fel_firebolt_wild_imp', 'spell_warlock_inquisitors_gaze', 'spell_warl_incinerate', 'spell_warlock_agony', 'spell_warlock_imp_firebolt');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(205231, 'spell_warl_darkglare_eye_laser'),
(5782, 'spell_warl_fear'),
(204730, 'spell_warl_fear_buff'),
(146739, 'spell_warl_corruption_effect'),
(234153, 'spell_warl_drain_life'),
(205246, 'aura_warl_phantomatic_singularity'),
(48181, 'aura_warl_haunt'),
(205180, 'spell_warlock_summon_darkglare'),
(5697, 'spell_warlock_unending_breath'),
(111771, 'spell_warl_demonic_gateway'),
(104316, 'spell_warlock_call_dreadstalkers'),
(264178, 'spell_warlock_demonbolt_new'),
(205145, 'spell_warl_demonic_calling'),
(196277, 'spell_warl_implosion'),
(603, 'spell_warlock_doom'),
(6353, 'spell_warlock_soul_fire'),
(215941, 'spell_warl_soul_conduit'),
(232670, 'spell_warr_shadowbolt_affliction'),
(104318, 'spell_warlock_fel_firebolt_wild_imp'),
(386344, 'spell_warlock_inquisitors_gaze'),
(29722, 'spell_warl_incinerate'),
(980, 'spell_warlock_agony'),
(3110, 'spell_warlock_imp_firebolt'),
(86040, 'spell_warl_hand_of_guldan_damage'),
(105174, 'spell_warl_hand_of_guldan');

REPLACE INTO `npc_spellclick_spells` VALUES (59262, 113902, 0, 0);
REPLACE INTO `npc_spellclick_spells` VALUES (59271, 113902, 0, 0);