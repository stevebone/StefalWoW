-- Demon Hunter spell script bindings

-- 1226019 - Reap: fires single-target damage (1225823) at target and grants Fury (1261679)
DELETE FROM `spell_script_names` WHERE `spell_id` = 1226019 AND `ScriptName` = 'spell_dh_reap';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1226019, 'spell_dh_reap');

-- 473662 - Consume: native damage + grants Fury via 1261710
DELETE FROM `spell_script_names` WHERE `spell_id` = 473662 AND `ScriptName` = 'spell_dh_consume_energize';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (473662, 'spell_dh_consume_energize');

-- 471306 - Void Metamorphosis passive talent
DELETE FROM `spell_script_names` WHERE `spell_id` = 471306 AND `ScriptName` = 'spell_dh_void_metamorphosis_passive';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (471306, 'spell_dh_void_metamorphosis_passive');

-- 1225789 - Void Metamorphosis stack counter
DELETE FROM `spell_script_names` WHERE `spell_id` = 1225789 AND `ScriptName` = 'spell_dh_void_metamorphosis_counter';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1225789, 'spell_dh_void_metamorphosis_counter');

-- 1217607 - Void Metamorphosis active buff (Fury drain + cleanup)
DELETE FROM `spell_script_names` WHERE `spell_id` = 1217607 AND `ScriptName` = 'spell_dh_void_metamorphosis_buff';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1217607, 'spell_dh_void_metamorphosis_buff');

-- 1217610 - Devour: 2 soul fragments + reduces Reap CD by 1s
DELETE FROM `spell_script_names` WHERE `spell_id` = 1217610 AND `ScriptName` = 'spell_dh_devour';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1217610, 'spell_dh_devour');

-- 1245453 - Cull: damage + collects up to 4 soul fragments (Reap replacement during Void Metamorphosis)
DELETE FROM `spell_script_names` WHERE `spell_id` = 1245453 AND `ScriptName` = 'spell_dh_cull';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1245453, 'spell_dh_cull');

-- 1225826 - Eradicate
DELETE FROM `spell_script_names` WHERE `spell_id` = 1225826 AND `ScriptName` = 'spell_dh_eradicate';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1225826, 'spell_dh_eradicate');

-- 1261684 - Entropy: spawn fragment every 12s in combat; increment Void Meta counter out of combat (up to 25)
DELETE FROM `spell_script_names` WHERE `spell_id` = 1261684 AND `ScriptName` = 'spell_dh_entropy';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1261684, 'spell_dh_entropy');

-- 1237270 - Feast of Souls: independent 10s stack per soul fragment via m_Events timer
DELETE FROM `spell_script_names` WHERE `spell_id` = 1237270 AND `ScriptName` = 'spell_dh_feast_of_souls_tracker';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1237270, 'spell_dh_feast_of_souls_tracker');

-- 1221167 - Collapsing Star passive tracker (30 fragments during Void Meta grants 1221171 override)
DELETE FROM `spell_script_names` WHERE `spell_id` = 1221167 AND `ScriptName` = 'spell_dh_collapsing_star_passive';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1221167, 'spell_dh_collapsing_star_passive');

-- 1227702 - Collapsing Star counter (stacked manually per fragment collected during meta)
DELETE FROM `spell_script_names` WHERE `spell_id` = 1227702 AND `ScriptName` = 'spell_dh_collapsing_star_counter';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1227702, 'spell_dh_collapsing_star_counter');

-- 1221150 - Collapsing Star (castable): fires 1221162 damage + optional soul fragments
DELETE FROM `spell_script_names` WHERE `spell_id` = 1221150 AND `ScriptName` = 'spell_dh_collapsing_star';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1221150, 'spell_dh_collapsing_star');

-- 1246169 - The Hunt damage: if player has Hungering Slash passive, grant override
DELETE FROM `spell_script_names` WHERE `spell_id` = 1246169 AND `ScriptName` = 'spell_dh_the_hunt_damage';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1246169, 'spell_dh_the_hunt_damage');

-- Cleanup old proc-based passive binding (no longer used)
DELETE FROM `spell_script_names` WHERE `spell_id` = 1239519 AND `ScriptName` = 'spell_dh_hungering_slash_passive';

-- 1239123 - Hungering Slash: damage, Fury, fragment shatter, VR charge, Voidstep
DELETE FROM `spell_script_names` WHERE `spell_id` = 1239123 AND `ScriptName` = 'spell_dh_hungering_slash';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1239123, 'spell_dh_hungering_slash');

-- 1225827/1279200 - Eradicate damage: target filtering (25yd cone)
DELETE FROM `spell_script_names` WHERE `spell_id` IN (1225827, 1279200) AND `ScriptName` = 'spell_dh_eradicate_damage';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1225827, 'spell_dh_eradicate_damage'),
(1279200, 'spell_dh_eradicate_damage');

-- 1256305/1256306/1256617/1256619 - Voidfall/World Killer meteor damage: Catastrophe DoT
DELETE FROM `spell_script_names` WHERE `spell_id` IN (1256305, 1256306, 1256617, 1256619) AND `ScriptName` = 'spell_dh_voidfall_meteor_damage';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1256305, 'spell_dh_voidfall_meteor_damage'),
(1256306, 'spell_dh_voidfall_meteor_damage'),
(1256617, 'spell_dh_voidfall_meteor_damage'),
(1256619, 'spell_dh_voidfall_meteor_damage');

-- 1253676 - Doomsayer passive: tracks combat entry; 1265768 - Doomsayer buff: fires meteors
DELETE FROM `spell_script_names` WHERE `spell_id` IN (1253676, 1265768) AND `ScriptName` IN ('spell_dh_doomsayer_passive', 'spell_dh_doomsayer_buff');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1253676, 'spell_dh_doomsayer_passive'),
(1265768, 'spell_dh_doomsayer_buff');

DELETE FROM `spell_proc` WHERE `SpellId` IN (1253676, 1265768);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(1253676, 0, 0, 0, 0, 0, 0, 0x00011110, 0, 1, 2, 0, 0, 0, 0, 100, 0, 0),
(1265768, 0, 0, 0, 0, 0, 0, 0x00010000, 0, 1, 2, 0, 0, 0, 0, 100, 0, 1);

-- 1253304 - Voidfall passive: 35% chance on Consume hit to grant Voidfall stack
DELETE FROM `spell_script_names` WHERE `spell_id` = 1253304 AND `ScriptName` = 'spell_dh_voidfall_passive';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1253304, 'spell_dh_voidfall_passive');

DELETE FROM `spell_proc` WHERE `SpellId` = 1253304;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES (1253304, 0, 0, 0, 0, 0, 0, 0x00011010, 0, 1, 2, 0, 0, 0, 0, 100, 0, 0);

-- 1246556 - Spontaneous Immolation: 5% proc chance to cast Soul Immolation
DELETE FROM `spell_script_names` WHERE `spell_id` = 1246556 AND `ScriptName` = 'spell_dh_spontaneous_immolation';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1246556, 'spell_dh_spontaneous_immolation');

DELETE FROM `spell_proc` WHERE `SpellId` = 1246556;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES (1246556, 0, 0, 0, 0, 0, 0, 0x00011110, 0, 1, 2, 0, 0, 0, 0, 100, 500, 0);

-- 1241937/1266696 - Soul Immolation: periodic dummy spawns fragments + Fury
DELETE FROM `spell_script_names` WHERE `spell_id` IN (1241937, 1266696) AND `ScriptName` = 'spell_dh_soul_immolation';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1241937, 'spell_dh_soul_immolation'),
(1266696, 'spell_dh_soul_immolation');

-- 1250088 - Midnight: scales Collapsing Star crit damage by SpellCrit% via periodic dummy
DELETE FROM `spell_script_names` WHERE `spell_id` = 1250088 AND `ScriptName` = 'spell_dh_midnight_passive';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1250088, 'spell_dh_midnight_passive');

-- 473728 - Void Ray channel aura: Doomsayer window + Meteoric Rise on full channel
DELETE FROM `spell_script_names` WHERE `spell_id` = 473728 AND `ScriptName` = 'spell_dh_void_ray';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (473728, 'spell_dh_void_ray');

-- 1242504 - Emptiness buff: tracks DR stacks from soul fragments
DELETE FROM `spell_script_names` WHERE `spell_id` = 1242504 AND `ScriptName` = 'spell_dh_emptiness_buff';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1242504, 'spell_dh_emptiness_buff');

DELETE FROM `areatrigger_create_properties` WHERE `Id`=36671 AND `IsCustom`=0;
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`, `ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `SpellForVisuals`, `TimeToTargetScale`, `Speed`, `SpeedIsTime`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`, `ShapeData5`, `ShapeData6`, `ShapeData7`, `ScriptName`, `VerifiedBuild`) VALUES (36671, 0, 38618, 0, 0, 0, 0, 0, 0, -1, 0, 0, 1223412, 20000, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 'at_dh_shattered_souls_devourer', 65727);

-- 195072 - Fel rush
DELETE FROM `spell_script_names` WHERE `spell_id` = 195072 AND `ScriptName` = 'spell_dh_fel_rush';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (195072, 'spell_dh_fel_rush');

-- 223107 - Fel rush damage
DELETE FROM `spell_script_names` WHERE `spell_id` = 223107 AND `ScriptName` = 'spell_dh_fel_rush_damage';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (223107, 'spell_dh_fel_rush_damage');

-- 197923 - Fel rush dash
DELETE FROM `spell_script_names` WHERE `spell_id` = 197923 AND `ScriptName` = 'spell_dh_fel_rush_dash';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (197923, 'spell_dh_fel_rush_dash');

-- 197922 - Fel rush dash ground
DELETE FROM `spell_script_names` WHERE `spell_id` = 197922 AND `ScriptName` = 'spell_dh_fel_rush_dash_ground';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (197922, 'spell_dh_fel_rush_dash_ground');

-- 203556 - Master of the glaive
DELETE FROM `spell_script_names` WHERE `spell_id` = 203556 AND `ScriptName` = 'spell_dh_master_of_the_glaive';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (203556, 'spell_dh_master_of_the_glaive');

-- 211881 - Fel Eruption
DELETE FROM `spell_script_names` WHERE `spell_id` = 211881 AND `ScriptName` = 'spell_dh_fel_eruption';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (211881, 'spell_dh_fel_eruption');

-- 191427 - Metamorphosis
DELETE FROM `spell_script_names` WHERE `spell_id` = 191427 AND `ScriptName` = 'spell_dh_metamorphosis';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (191427, 'spell_dh_metamorphosis');

-- 206473 - Bloodlet
DELETE FROM `spell_script_names` WHERE `spell_id` = 206473 AND `ScriptName` = 'spell_dh_bloodlet';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (206473, 'spell_dh_bloodlet');

-- 211053 - Fel Barrage
DELETE FROM `spell_script_names` WHERE `spell_id` = 211053 AND `ScriptName` = 'spell_dh_fel_barrage';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (211053, 'spell_dh_fel_barrage');

-- 222703 - Fel Barrage Aura
DELETE FROM `spell_script_names` WHERE `spell_id` = 222703 AND `ScriptName` = 'spell_dh_fel_barrage_aura';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (222703, 'spell_dh_fel_barrage_aura');

-- 222703 - Fel Barrage Damage
DELETE FROM `spell_script_names` WHERE `spell_id` = 222703 AND `ScriptName` = 'spell_dh_fel_barrage_damage';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (222703, 'spell_dh_fel_barrage_damage');

-- 206491 - Nemesis
DELETE FROM `spell_script_names` WHERE `spell_id` = 206491 AND `ScriptName` = 'spell_dh_nemesis';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (206491, 'spell_dh_nemesis');

-- 189110 - Infernal Strike
DELETE FROM `spell_script_names` WHERE `spell_id` = 189110 AND `ScriptName` = 'spell_dh_infernal_strike';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (189110, 'spell_dh_infernal_strike');

-- 228477 - Soul Cleave
DELETE FROM `spell_script_names` WHERE `spell_id` = 228477 AND `ScriptName` = 'spell_dh_soul_cleave';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (228477, 'spell_dh_soul_cleave');

-- 228478 - Soul Cleave Damage
DELETE FROM `spell_script_names` WHERE `spell_id` = 228478 AND `ScriptName` = 'spell_dh_soul_cleave_damage';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (228478, 'spell_dh_soul_cleave_damage');

-- 263648 - Soul Barrier
DELETE FROM `spell_script_names` WHERE `spell_id` = 263648 AND `ScriptName` = 'spell_dh_soul_barrier';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (263648, 'spell_dh_soul_barrier');

-- 209400 - Razor Spikes
DELETE FROM `spell_script_names` WHERE `spell_id` = 209400 AND `ScriptName` = 'spell_dh_razor_spikes';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (209400, 'spell_dh_razor_spikes');

-- 207810 - Nether Bond
DELETE FROM `spell_script_names` WHERE `spell_id` = 207810 AND `ScriptName` = 'spell_dh_nether_bond';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (207810, 'spell_dh_nether_bond');

-- 207811 - Nether Bond Periodic
DELETE FROM `spell_script_names` WHERE `spell_id` = 207811 AND `ScriptName` = 'spell_dh_nether_bond_periodic';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (207811, 'spell_dh_nether_bond_periodic');

-- 258920 - Immolation Aura
DELETE FROM `spell_script_names` WHERE `spell_id` = 258920 AND `ScriptName` = 'spell_dh_immolation_aura';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (258920, 'spell_dh_immolation_aura');

-- 258922 - Immolation Aura Damage
DELETE FROM `spell_script_names` WHERE `spell_id` = 258922 AND `ScriptName` = 'spell_dh_immolation_aura_damage';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (258922, 'spell_dh_immolation_aura_damage');

-- 211509 - Solitude
DELETE FROM `spell_script_names` WHERE `spell_id` = 211509 AND `ScriptName` = 'spell_dh_solitude';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (211509, 'spell_dh_solitude');

-- 205598 - Awaken the demon
DELETE FROM `spell_script_names` WHERE `spell_id` = 205598 AND `ScriptName` = 'spell_dh_awaken_the_demon';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (205598, 'spell_dh_awaken_the_demon');

-- 205604 - Reverse Magic
DELETE FROM `spell_script_names` WHERE `spell_id` = 205604 AND `ScriptName` = 'spell_dh_reverse_magic';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (205604, 'spell_dh_reverse_magic');

-- 206649 - Eye of Leotheras
DELETE FROM `spell_script_names` WHERE `spell_id` = 206649 AND `ScriptName` = 'spell_dh_eye_of_leotheras';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (206649, 'spell_dh_eye_of_leotheras');

-- 208796 - Jagged Spikes
DELETE FROM `spell_script_names` WHERE `spell_id` = 208796 AND `ScriptName` = 'spell_dh_jagged_spikes';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (208796, 'spell_dh_jagged_spikes');

-- 203753 - Blade Turning
DELETE FROM `spell_script_names` WHERE `spell_id` = 203753 AND `ScriptName` = 'spell_dh_blade_turning';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (203753, 'spell_dh_blade_turning');

-- 206891 - Intimidated
DELETE FROM `spell_script_names` WHERE `spell_id` = 206891 AND `ScriptName` = 'spell_dh_intimidated';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (206891, 'spell_dh_intimidated');

-- 206966 - Fel Lance
DELETE FROM `spell_script_names` WHERE `spell_id` = 206966 AND `ScriptName` = 'spell_dh_fel_lance';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (206966, 'spell_dh_fel_lance');

-- 206475 - Chaos Cleave
DELETE FROM `spell_script_names` WHERE `spell_id` = 206475 AND `ScriptName` = 'spell_dh_chaos_cleave';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (206475, 'spell_dh_chaos_cleave');

-- 203783 - Shear
DELETE FROM `spell_script_names` WHERE `spell_id` = 203783 AND `ScriptName` = 'spell_dh_shear_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (203783, 'spell_dh_shear_proc');

-- 210047 - Consume Soul Missile
DELETE FROM `spell_script_names` WHERE `spell_id` = 210047 AND `ScriptName` = 'spell_dh_consume_soul_missile';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (210047, 'spell_dh_consume_soul_missile');

-- 209426 - Darkness Absorb
DELETE FROM `spell_script_names` WHERE `spell_id` = 209426 AND `ScriptName` = 'spell_dh_darkness_absorb';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (209426, 'spell_dh_darkness_absorb');

-- 203650 - Prepared
DELETE FROM `spell_script_names` WHERE `spell_id` = 203650 AND `ScriptName` = 'spell_dh_prepared';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (203650, 'spell_dh_prepared');

-- 228532 - Soul Fragment Heals
DELETE FROM `spell_script_names` WHERE `spell_id` = 228532 AND `ScriptName` = 'spell_dh_soul_fragment_heals';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (228532, 'spell_dh_soul_fragment_heals');

-- 205411 - Desperate Instincts
DELETE FROM `spell_script_names` WHERE `spell_id` = 205411 AND `ScriptName` = 'spell_dh_desperate_instincts';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (205411, 'spell_dh_desperate_instincts');

-- 209795 - Fracture
DELETE FROM `spell_script_names` WHERE `spell_id` = 209795 AND `ScriptName` = 'spell_dh_fracture';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (209795, 'spell_dh_fracture');

-- 224509 - Frailty
DELETE FROM `spell_script_names` WHERE `spell_id` = 224509 AND `ScriptName` = 'spell_dh_frailty';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (224509, 'spell_dh_frailty');

-- 247454 - Spirit Bomb
DELETE FROM `spell_script_names` WHERE `spell_id` = 247454 AND `ScriptName` = 'spell_dh_spirit_bomb';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (247454, 'spell_dh_spirit_bomb');

-- 236189 - Demonic Infusion
DELETE FROM `spell_script_names` WHERE `spell_id` = 236189 AND `ScriptName` = 'spell_dh_demonic_infusion';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (236189, 'spell_dh_demonic_infusion');

-- 206803 - Rain from above
DELETE FROM `spell_script_names` WHERE `spell_id` = 206803 AND `ScriptName` = 'spell_dh_rain_from_above';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (206803, 'spell_dh_rain_from_above');

-- 162264 - Metamorphosis buffs
DELETE FROM `spell_script_names` WHERE `spell_id` = 162264 AND `ScriptName` = 'spell_dh_metamorphosis_buffs';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (162264, 'spell_dh_metamorphosis_buffs');

-- 185244 - Pain
DELETE FROM `spell_script_names` WHERE `spell_id` = 185244 AND `ScriptName` = 'spell_dh_pain';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (185244, 'spell_dh_pain');

-- 203704 - Mana Break
DELETE FROM `spell_script_names` WHERE `spell_id` = 203704 AND `ScriptName` = 'spell_dh_mana_break';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (203704, 'spell_dh_mana_break');

-- 258881 - Trail of ruin
DELETE FROM `spell_script_names` WHERE `spell_id` = 258881 AND `ScriptName` = 'spell_dh_trail_of_ruin';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (258881, 'spell_dh_trail_of_ruin');

-- 213480 - Unending Hatred
DELETE FROM `spell_script_names` WHERE `spell_id` = 213480 AND `ScriptName` = 'spell_dh_unending_hatred';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (213480, 'spell_dh_unending_hatred');

-- 162794 - Chaos Strike Energized
DELETE FROM `spell_script_names` WHERE `spell_id` = 162794 AND `ScriptName` = 'spell_dh_chaos_strike_energize';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (162794, 'spell_dh_chaos_strike_energize');

-- 344865 - Fel rush no spec
DELETE FROM `spell_script_names` WHERE `spell_id` = 344865 AND `ScriptName` = 'spell_dh_fel_rush_specless';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (344865, 'spell_dh_fel_rush_specless');

-- 212105 - Fel Devastation dmg
DELETE FROM `spell_script_names` WHERE `spell_id` = 212105 AND `ScriptName` = 'spell_dh_fel_devastation_damage';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (212105, 'spell_dh_fel_devastation_damage');

-- 201427 - Annihilation
DELETE FROM `spell_script_names` WHERE `spell_id` = 201427 AND `ScriptName` = 'spell_dh_annihilation';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (201427, 'spell_dh_annihilation');

REPLACE INTO `areatrigger_scripts` VALUES (9695, 'at_dh_mana_rift');
REPLACE INTO `areatrigger_scripts` VALUES (6482, 'at_dh_demonic_trample');

DELETE FROM `areatrigger_create_properties` WHERE `ScriptName` IN ('at_dh_demonic_trample');
INSERT INTO `areatrigger_create_properties` VALUES (6482, 1, 11107, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'at_dh_demonic_trample', 40120);