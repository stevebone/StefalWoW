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
