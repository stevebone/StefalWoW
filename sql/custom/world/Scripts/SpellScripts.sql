DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dru_thorns_of_iron_damage');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(400223, 'spell_dru_thorns_of_iron_damage');

DELETE FROM `spell_proc` WHERE `SpellId` IN (400222);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(400222,0x00,7,0x00000000,0x00000000,0x00004000,0x00000000,0x0,0x0,0x0,0x4,0x0,0x0,0x0,0,0,0,0); -- Thorns of Iron

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_dh_wave_of_debilitation';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(179057, 'spell_dh_wave_of_debilitation');

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dru_thrash');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(77758, 'spell_dru_thrash'),
(106830, 'spell_dru_thrash');

UPDATE `spell_script_names` SET `ScriptName` = 'spell_dru_thrash_bear_bleed' WHERE `spell_id` = 192090 AND `ScriptName` = 'spell_dru_thrash_aura';

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dru_rake');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1822, 'spell_dru_rake');

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_dh_shattered_restoration';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(202644, 'spell_dh_shattered_restoration'),
(228532, 'spell_dh_shattered_restoration'),
(178963, 'spell_dh_shattered_restoration'),
(203794, 'spell_dh_shattered_restoration'),
(210042, 'spell_dh_shattered_restoration');

DELETE FROM `spell_proc` WHERE `SpellId` IN (428607);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(428607,0x00,107,0x00000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x2,0x20,0x2,0x0,0,0,0,0); -- Live by the Glaive

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dru_matted_fur_absorb');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(385787, 'spell_dru_matted_fur_absorb');

DELETE FROM `spell_proc` WHERE `SpellId` IN (385786);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(385786,0x00,7,0x00000000,0x00040000,0x00000080,0x00000000,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0,0,0,0); -- Matted Fur

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dru_twin_moonfire');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(8921, 'spell_dru_twin_moonfire');

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dru_pulverize', 'spell_dru_pulverize_thrash', 'spell_dru_pulverize_trigger_periodic');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(80313, 'spell_dru_pulverize'),
(77758, 'spell_dru_pulverize_thrash'),
(158790, 'spell_dru_pulverize_trigger_periodic');

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dru_moonless_night');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(400278, 'spell_dru_moonless_night');

DELETE FROM `spell_proc` WHERE `SpellId` IN (400278);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(400278,0x01,7,0x00000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x1,0x2,0x0,0x0,0x0,0,0,0,0); -- Moonless Night

DELETE FROM `spell_proc` WHERE `SpellId` IN (392792);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(392792,0x00,4,0x02400400,0x00000000,0x00000000,0x08040000,0x0,0x0,0x0,0x1,0x0,0x4,0x0,0,0,0,0); -- Frothing Berserker

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_warr_frothing_berserker';
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(392792,'spell_warr_frothing_berserker');

DELETE FROM `spell_script_names` WHERE `spell_id` IN (315584, 8679, 2823, 381664) AND `ScriptName`='spell_rog_leeching_poison';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(315584, 'spell_rog_leeching_poison'),
(8679, 'spell_rog_leeching_poison'),
(2823, 'spell_rog_leeching_poison'),
(381664, 'spell_rog_leeching_poison');

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dru_maim');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(22570, 'spell_dru_maim');

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dru_guardian_of_elune_healing');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(22842, 'spell_dru_guardian_of_elune_healing');

DELETE FROM `spell_proc` WHERE `SpellId` IN (155578);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(155578,0x00,7,0x00000000,0x00000040,0x00000000,0x00000000,0x0,0x0,0x0,0x2,0x403,0x0,0x0,0,0,0,0); -- Guardian of Elune

DELETE FROM `spell_proc` WHERE `SpellId` IN (213680);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(213680,0x00,7,0x00000000,0x40000000,0x00004000,0x00000000,0x0,0x0,0x0,0x2,0x0,0x0,0x0,0,0,0,0); -- Guardian of Elune

DELETE FROM `spell_proc` WHERE `SpellId` IN (395446);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(395446,0x00,107,0x00000000,0x00000002,0x00000000,0x00000000,0x0,0x0,0x5,0x2,0x403,0x0,0x0,0,0,0,0); -- Soul Sigils

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_dh_soul_sigils';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(395446, 'spell_dh_soul_sigils');

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_dk_suppression';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(374049, 'spell_dk_suppression');

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dru_flower_walk', 'spell_dru_flower_walk_heal');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(22812, 'spell_dru_flower_walk'),
(439902, 'spell_dru_flower_walk_heal');

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_pri_atonement_effect' AND `spell_id` IN (139, 200829, 47750);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(200829, 'spell_pri_atonement_effect'),
(47750, 'spell_pri_atonement_effect');

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dh_enduring_torment','spell_dh_enduring_torment_buff');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(452410, 'spell_dh_enduring_torment'),
(453314, 'spell_dh_enduring_torment_buff');

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_warr_thunder_clap';
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_warr_thunder_clap_rend';
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(6343,'spell_warr_thunder_clap'),
(6343,'spell_warr_thunder_clap_rend');

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dru_elunes_favored', 'spell_dru_elunes_favored_proc');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(5487, 'spell_dru_elunes_favored'),
(370588, 'spell_dru_elunes_favored_proc');

DELETE FROM `spell_proc` WHERE `SpellId` IN (370588);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(370588,0x40,7,0x00000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x1,0x2,0x0,0x0,0x0,0,0,0,0); -- Elune's Favored

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_warr_pain_and_gain_heal';
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(382551,'spell_warr_pain_and_gain_heal');

DELETE FROM `spell_proc` WHERE `SpellId`=382549;
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(382549,0x00,4,0x00000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x1,0x0,0x0,0x0,0x0,0,0,0,0); -- Pain and Gain

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_intervene','spell_warr_intervene_charge');
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(3411,'spell_warr_intervene'),
(316531,'spell_warr_intervene_charge');

DELETE FROM `spell_proc` WHERE `SpellId` IN (147833);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(147833,0x00,4,0x00000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x0,0x0,0x2,0x0,0,0,0,0); -- Intervene

DELETE FROM `spell_proc` WHERE `SpellId` IN (383115);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(383115,0x00,4,0x00000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x2,0x1000,0x0,0x0,0,0,0,0); -- Concussive Blows

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_bloodsurge');
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(384361, 'spell_warr_bloodsurge');

UPDATE `spell_script_names` SET `ScriptName` = 'spell_warr_improved_whirlwind' WHERE `ScriptName` = 'spell_improved_whirlwind';

DELETE FROM `spell_proc` WHERE `SpellId` IN (388807);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(388807,0x00,4,0x00000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x0,0x20,0x0,0x0,0,0,0,0); -- Storm Wall

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_sudden_death','spell_warr_sudden_death_proc');
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(29725,'spell_warr_sudden_death');

DELETE FROM `spell_proc` WHERE `SpellId` IN (29725,52437);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(29725,0x00,4,0x00000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x1,0x2,0x0,0x0,0x0,0,0,0,0), -- Sudden Death
(52437,0x00,4,0x00000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x2,0x0,0x18,0x0,0,0,0,0); -- Sudden Death

DELETE FROM `spell_proc` WHERE `SpellId` IN (184783,199854);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(184783,0x00,4,0x00000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x1,0x0,0x0,0x0,0,0,0,0), -- Tactician
(199854,0x00,4,0x00000004,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x1,0x0,0x0,0x0,0,0,0,0); -- Tactician

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_warr_tactician';
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(184783,'spell_warr_tactician');

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_warr_overpowering_finish';
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(7384,'spell_warr_overpowering_finish');

DELETE FROM `spell_proc` WHERE `SpellId` IN (383219,391962);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(383219,0x00,4,0x02000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x1,0x0,0x0,0x0,0,0,0,0), -- Exhilarating Blows, Mortal Strike
(391962,0x00,4,0x00400000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x1,0x0,0x0,0x0,0,0,0,0); -- Exhilarating Blows, Cleave

UPDATE `creature_template` SET `RegenHealth`=0, `ScriptName`='npc_pet_dk_risen_ghoul' WHERE `entry`=26125;
UPDATE `creature_template` SET `ScriptName`='npc_pet_dk_bloodworm' WHERE `entry`=28017;

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dk_birth', 'spell_dk_blood_bond_periodic');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1217740, 'spell_dk_birth'),
(1267044, 'spell_dk_blood_bond_periodic');


DELETE FROM `spell_proc` WHERE `SpellId` IN (1265570);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(1265570,0x01,4,0x00000000,0x08000000,0x00000000,0x00000000,0x0,0x0,0x0,0x2,0x403,0x0,0x0,0,0,0,0); -- Executioner's Wrath


DELETE FROM `spell_proc` WHERE `SpellId` IN (385703);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(385703,0x01,4,0x00000000,0x00000400,0x00000000,0x00000000,0x0,0x0,0x1,0x2,0x403,0x0,0x0,0,0,0,0); -- Bloodborne


DELETE FROM `spell_proc` WHERE `SpellId` IN (335077);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(335077,0x00,4,0x00000000,0x00000000,0x00000000,0x08000000,0x0,0x0,0x0,0x4,0x0,0x0,0x0,0,0,0,0); -- Frenzy

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_frenzy_rampage', 'spell_warr_frenzy');


DELETE FROM `spell_proc` WHERE `SpellId` IN (1265356);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(1265356,0x01,4,0x00000000,0x00000400,0x00000000,0x00000000,0x0,0x0,0x1,0x2,0x402,0x0,0x0,0,0,0,0); -- Ragedrinker


DELETE FROM `spell_proc` WHERE `SpellId` IN (1265355);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(1265355,0x00,4,0x00000000,0x00000000,0x00000000,0x08000000,0x0,0x0,0x0,0x2,0x0,0x0,0x0,0,0,0,0); -- Scent of Blood

DELETE FROM `spell_proc` WHERE `SpellId` IN (1265399);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(1265399,0x01,4,0x00000000,0x00000400,0x00000000,0x00000000,0x0,0x0,0x1,0x2,0x403,0x0,0x0,0,0,0,1); -- Scent of Blood


DELETE FROM `spell_proc` WHERE `SpellId` IN (1261060);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(1261060,0x01,4,0x20000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x1,0x2,0x403,0x0,0x0,0,0,0,0); -- Deep Wounds


DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_dh_consume_energize';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(473662, 'spell_dh_consume_energize');


DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dh_voidblade', 'spell_dh_voidblade_charge');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1245412, 'spell_dh_voidblade'),
(1241285, 'spell_dh_voidblade_charge');


DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_dh_shift';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1234796, 'spell_dh_shift');

DELETE FROM `jump_charge_params` WHERE `id`=1466;
INSERT INTO `jump_charge_params` (`id`, `speed`, `treatSpeedAsMoveTimeSeconds`, `unlimitedSpeed`, `minHeight`, `maxHeight`, `spellVisualId`, `progressCurveId`, `parabolicCurveId`, `triggerSpellId`) VALUES
(1466, 0.1, 1, 1, NULL, NULL, 164063, 88051, 88052, NULL);


DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_execute', 'spell_warr_execute_refund_rage');
DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_execute_damage') AND `spell_id` IN (317483);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(163201, 'spell_warr_execute'),
(281000, 'spell_warr_execute'),
(330334, 'spell_warr_execute'),
(317349, 'spell_warr_execute'),
(317483, 'spell_warr_execute_damage'),
(260798, 'spell_warr_execute_refund_rage');


DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_dh_void_ray';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(473728, 'spell_dh_void_ray');


DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_dh_voidglare_boon';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(473728, 'spell_dh_voidglare_boon');


DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_pri_evangelism','spell_pri_power_word_radiance_evangelism');
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(472433,'spell_pri_evangelism'),
(194509,'spell_pri_power_word_radiance_evangelism');

DELETE FROM `spell_proc` WHERE `SpellId` IN (472433);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(472433,0x00,6,0x00400000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x0,0x0,0x10,0x0,0,0,0,0); -- Evangelism


DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_warr_surge_of_adrenaline';
INSERT INTO `spell_script_names`(`spell_id`, `ScriptName`) VALUES
(280270, 'spell_warr_surge_of_adrenaline');

DELETE FROM `spell_proc` WHERE `SpellId` IN (280270);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(280270,0x00,4,0x00000000,0x00000000,0x00000000,0x00000400,0x0,0x0,0x0,0x1,0x0,0x0,0x0,0,0,0,0); -- Always Angry


DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_meat_cleaver_damage_bonus', 'spell_warr_meat_cleaver_damage_bonus_thunder_clap');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(199667, 'spell_warr_meat_cleaver_damage_bonus'),
(44949, 'spell_warr_meat_cleaver_damage_bonus'),
(199852, 'spell_warr_meat_cleaver_damage_bonus'),
(199851, 'spell_warr_meat_cleaver_damage_bonus'),
(6343, 'spell_warr_meat_cleaver_damage_bonus_thunder_clap'),
(435222, 'spell_warr_meat_cleaver_damage_bonus_thunder_clap');

DELETE FROM `areatrigger_create_properties` WHERE `IsCustom`=0 AND `Id`=36671;
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`, `ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `SpellForVisuals`, `TimeToTargetScale`, `Speed`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`, `ShapeData5`, `ShapeData6`, `ShapeData7`, `ScriptName`, `VerifiedBuild`) VALUES
(36671, 0, 38618, 0, 0, 0, 0, 0, 0, -1, 0, 0, NULL, 20000, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 'at_dh_shattered_souls_devourer', 65727); -- Spell: 1223412 (Soul Fragment)

DELETE FROM `areatrigger_template` WHERE `IsCustom`=0 AND `Id`=38618;
INSERT INTO `areatrigger_template` (`Id`, `IsCustom`, `VerifiedBuild`) VALUES
(38618, 0, 65727);

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dh_shattered_souls_devourer', 'spell_dh_shattered_souls_devourer_missile', 'spell_dh_shattered_souls_devourer_dummy');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1227619, 'spell_dh_shattered_souls_devourer'),
(1223445, 'spell_dh_shattered_souls_devourer_trigger'),
(1223448, 'spell_dh_shattered_souls_devourer_trigger'),
(1223450, 'spell_dh_shattered_souls_devourer_dummy');


DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_kill_or_be_killed', 'spell_warr_kill_or_be_killed_target', 'spell_warr_kill_or_be_killed_warrior');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1265361, 'spell_warr_kill_or_be_killed'),
(1265641, 'spell_warr_kill_or_be_killed_target'),
(1265600, 'spell_warr_kill_or_be_killed_warrior');

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_rampaging_ruin');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(184367, 'spell_warr_rampaging_ruin');

DELETE FROM `spell_proc` WHERE `SpellId` IN (386633,386634);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(386633,0x00,4,0x02000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x4,0x0,0x0,0x0,0,0,0,1), -- Executioner's Precision
(386634,0x00,4,0x00000000,0x00000000,0x00040000,0x00000000,0x0,0x0,0x0,0x2,0x0,0x0,0x0,0,0,0,0); -- Executioner's Precision

DELETE FROM `spell_proc` WHERE `SpellId` IN (85739);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(85739,0x00,4,0x00000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x1,0x0,0x18,0x0,0,0,0,0); -- Whirlwind

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_improved_whirlwind_cleave');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1464, 'spell_warr_improved_whirlwind_cleave'),  -- Slam
(1715, 'spell_warr_improved_whirlwind_cleave'),  -- Hamstring
(23881, 'spell_warr_improved_whirlwind_cleave'),  -- Bloodthirst
(23922, 'spell_warr_improved_whirlwind_cleave'),  -- Shield Slam
(34428, 'spell_warr_improved_whirlwind_cleave'),  -- Victory Rush
(85384, 'spell_warr_improved_whirlwind_cleave'),  -- Raging Blow
(96103, 'spell_warr_improved_whirlwind_cleave'),  -- Raging Blow
(100130, 'spell_warr_improved_whirlwind_cleave'),  -- Furious Slash
(163558, 'spell_warr_improved_whirlwind_cleave'),  -- Execute Off-Hand
(184707, 'spell_warr_improved_whirlwind_cleave'),  -- Rampage
(184709, 'spell_warr_improved_whirlwind_cleave'),  -- Rampage
(201363, 'spell_warr_improved_whirlwind_cleave'),  -- Rampage
(201364, 'spell_warr_improved_whirlwind_cleave'),  -- Rampage
(202168, 'spell_warr_improved_whirlwind_cleave'),  -- Impending Victory
(218617, 'spell_warr_improved_whirlwind_cleave'),  -- Rampage
(260798, 'spell_warr_improved_whirlwind_cleave'),  -- Execute
(280772, 'spell_warr_improved_whirlwind_cleave'),  -- Siegebreaker
(280849, 'spell_warr_improved_whirlwind_cleave'),  -- Execute
(317483, 'spell_warr_improved_whirlwind_cleave'),  -- Condemn (Venthyr)
(317488, 'spell_warr_improved_whirlwind_cleave'),  -- Condemn (Venthyr)
(317489, 'spell_warr_improved_whirlwind_cleave'),  -- Condemn Off-Hand (Venthyr)
(335096, 'spell_warr_improved_whirlwind_cleave'),  -- Bloodbath
(335098, 'spell_warr_improved_whirlwind_cleave'),  -- Crushing Blow
(335100, 'spell_warr_improved_whirlwind_cleave'),  -- Crushing Blow
(394062, 'spell_warr_improved_whirlwind_cleave'),  -- Rend
(394063, 'spell_warr_improved_whirlwind_cleave'),  -- Rend
(396718, 'spell_warr_improved_whirlwind_cleave'),  -- Onslaught
(463815, 'spell_warr_improved_whirlwind_cleave'),  -- Arms Execute FX Test
(463816, 'spell_warr_improved_whirlwind_cleave'),  -- Fury Execute FX Test
(463817, 'spell_warr_improved_whirlwind_cleave'),  -- Fury Execute Off-Hand FX Test
(1269383, 'spell_warr_improved_whirlwind_cleave');  -- Heroic Strike

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_pri_searing_light');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1280131, 'spell_pri_searing_light');

DELETE FROM `spell_proc` WHERE `SpellId` IN (1280131);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(1280131,0x00,6,0x00000000,0x00008000,0x00000000,0x00000000,0x0,0x0,0x1,0x2,0x0,0x0,0x0,0,0,0,0); -- Searing Light

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ("spell_pri_archangel");
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(472433, 'spell_pri_archangel');

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dru_galactic_guardian_moonfire','spell_dru_lunar_wrath');
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(164812,'spell_dru_galactic_guardian_moonfire'),
(1253600,'spell_dru_lunar_wrath');

DELETE FROM `spell_proc` WHERE `SpellId` IN (203964,1253600);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(203964,0x00,7,0x00000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x1,0x2,0x0,0x0,0x0,0,0,0,0), -- Galactic Guardian
(1253600,0x00,7,0x00000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x2,0x0,0x10,0x0,0,0,0,0); -- Lunar Wrath

DELETE FROM `spell_proc` WHERE `SpellId` IN (1253724);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(1253724,0x00,6,0x00000080,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x2,0x0,0x0,0x0,0,0,0,0); -- Greater Smite

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_brutal_finish');
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(227847,'spell_warr_brutal_finish'), -- Brutal Finish (Bladestorm)
(389774,'spell_warr_brutal_finish'), -- Brutal Finish (Bladestorm, Hurricane)
(446035,'spell_warr_brutal_finish'); -- Brutal Finish (Bladestorm, Unrelenting Onslaught)

DELETE FROM `spell_proc` WHERE `SpellId` IN (446918);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(446918,0x00,4,0x02000000,0x00000000,0x00000000,0x00800000,0x0,0x0,0x0,0x2,0x0,0x0,0x0,0,0,0,1); -- Brutal Finish

DELETE FROM `spell_proc` WHERE `SpellId` IN (85739);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(85739,0x00,4,0x00300002,0x00202700,0x00000120,0x00900000,0x0,0x0,0x0,0x4,0x0,0x10,0x0,0,0,0,0); -- Whirlwind

DELETE FROM `spell_proc` WHERE `SpellId` IN (435607,435615);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(435607,0x01,4,0x00000000,0x00000600,0x00000000,0x00000000,0x0,0x0,0x1,0x4,0x403,0x0,0x0,0,0,0,0), -- Thunder Blast
(435615,0x00,4,0x00000080,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x4,0x0,0x10,0x0,0,0,0,0); -- Thunder Blast

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_thunder_blast', 'spell_warr_thunder_clap_rend', 'spell_warr_thunder_clap');
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(6343, 'spell_warr_thunder_clap'),
(6343, 'spell_warr_thunder_clap_rend'),
(435222, 'spell_warr_thunder_clap'),
(435222, 'spell_warr_thunder_clap_rend'),
(435607,'spell_warr_thunder_blast');

DELETE FROM `spell_proc` WHERE `SpellId` IN (438590);

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_keep_your_feet_on_the_ground');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(438590, 'spell_warr_keep_your_feet_on_the_ground');