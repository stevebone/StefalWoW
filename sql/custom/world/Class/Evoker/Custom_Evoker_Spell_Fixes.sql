-- Evoker spell script bindings

-- 359073 - Eternity Surge (3-stage base version)
DELETE FROM `spell_script_names` WHERE `spell_id` = 359073 AND `ScriptName` IN ('spell_evo_eternity_surge', 'spell_evo_eternity_surge_base');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (359073, 'spell_evo_eternity_surge_base');

-- 382411 - Eternity Surge (4-stage active version)
DELETE FROM `spell_script_names` WHERE `spell_id` = 382411 AND `ScriptName` = 'spell_evo_eternity_surge';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (382411, 'spell_evo_eternity_surge');

-- 359077 - Eternity Surge damage: chain target count controlled by spell_evo_eternity_surge_damage script
DELETE FROM `spell_script_names` WHERE `spell_id` = 359077 AND `ScriptName` = 'spell_evo_eternity_surge_damage';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (359077, 'spell_evo_eternity_surge_damage');

-- 444088/444089 - Consume Flame: consumes Fire Breath DoT and detonates for 115%
-- 356995 - Disintegrate (AuraScript): consumes 2000ms of Fire Breath per tick
DELETE FROM `spell_script_names` WHERE `spell_id` = 356995 AND `ScriptName` = 'spell_evo_disintegrate';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (356995, 'spell_evo_disintegrate');
-- 357212 - Pyre damage: consumes 1000ms of Fire Breath per target hit
DELETE FROM `spell_script_names` WHERE `spell_id` = 357212 AND `ScriptName` = 'spell_evo_pyre_damage';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (357212, 'spell_evo_pyre_damage');
-- 444089 - Consume Flame damage: flat damage from CustomArg
DELETE FROM `spell_script_names` WHERE `spell_id` = 444089 AND `ScriptName` = 'spell_evo_consume_flame_damage';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (444089, 'spell_evo_consume_flame_damage');

-- 1271783 - Rising Fury: periodic tick grants stack while Dragonrage active; Dragonrage apply/remove manages it
DELETE FROM `spell_script_names` WHERE `spell_id` = 1271783 AND `ScriptName` = 'spell_evo_rising_fury';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1271783, 'spell_evo_rising_fury');

-- 1271799 - Risen Fury: periodic dummy generates Essence Burst every 4s
DELETE FROM `spell_script_names` WHERE `spell_id` = 1271799 AND `ScriptName` = 'spell_evo_risen_fury';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1271799, 'spell_evo_risen_fury');

-- 370821 - Scintillation: procs on Disintegrate periodic, fires 431192 at 40% power
DELETE FROM `spell_script_names` WHERE `spell_id` = 370821 AND `ScriptName` = 'spell_evo_scintillation';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (370821, 'spell_evo_scintillation');

-- 431192 - Eternity Surge (Scintillation proc missile): applies power pct from CustomArg
DELETE FROM `spell_script_names` WHERE `spell_id` = 431192 AND `ScriptName` = 'spell_evo_scintillation_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (431192, 'spell_evo_scintillation_proc');

-- 359618 - Essence Burst: fires Twin Flame when charge is consumed
DELETE FROM `spell_script_names` WHERE `spell_id` = 359618 AND `ScriptName` = 'spell_evo_essence_burst_consumed';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (359618, 'spell_evo_essence_burst_consumed');

-- 1265872 - Azure Sweep (Blue): target cap handled by spell_evo_azure_sweep
DELETE FROM `spell_script_names` WHERE `spell_id` = 1265872 AND `ScriptName` = 'spell_evo_azure_sweep';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (1265872, 'spell_evo_azure_sweep');

-- 358385 - Landslide (Black): casts 355689 at dest (root + AT visual)
DELETE FROM `spell_script_names` WHERE `spell_id` = 358385 AND `ScriptName` = 'spell_evo_landslide';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (358385, 'spell_evo_landslide');

-- AT 25144 (IsCustom=0, template 29625) — root zone AT spawned by 355689 EFFECT_1 at dest
DELETE FROM `areatrigger_create_properties` WHERE `Id`=25144 AND `IsCustom`=0;
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`, `ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `SpellForVisuals`, `TimeToTargetScale`, `Speed`, `SpeedIsTime`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`, `ShapeData5`, `ShapeData6`, `ShapeData7`, `ScriptName`, `VerifiedBuild`) VALUES (25144, 0, 29625, 0, 0, 0, 0, 0, 0, -1, 0, 0, 355689, 0, 0, 0, 0, 6, 6, 0, 0, 0, 0, 0, 0, '', 66562);

-- AT 900466 (template 29625) — original custom entry, fix radius
UPDATE `areatrigger_create_properties` SET `ShapeData0`=6, `ShapeData1`=6, `ScriptName`='' WHERE `Id`=900466 AND `IsCustom`=1;

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_evo_deep_breath');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(357210, 'spell_evo_deep_breath');

DELETE FROM `areatrigger_create_properties` WHERE `Id`=900527 AND `IsCustom`=1;
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`, `ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `SpellForVisuals`, `TimeToTargetScale`, `Speed`, `SpeedIsTime`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`, `ShapeData5`, `ShapeData6`, `ShapeData7`, `ScriptName`, `VerifiedBuild`) VALUES (900527, 1, 35600, 0, 2, 0, 0, 0, 0, -1, 0, 0, 446136, 1000, 0, 0, 4, 5, 5, 5, 5, 0.3, 0.3, 0, 0, '', 66562);
DELETE FROM `areatrigger_template` WHERE `Id`=35600 AND `IsCustom`=0;
INSERT INTO `areatrigger_template` (`Id`, `IsCustom`, `Flags`, `ActionSetId`, `ActionSetFlags`, `VerifiedBuild`) VALUES (35600, 0, 0, 0, 0, 66562);

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_evo_soar', 'spell_evo_cosmic_visage');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(369536, 'spell_evo_soar'),
(351239 , 'spell_evo_cosmic_visage');

-- Remove no longer valid scripts
-- Call of Ysera spell itself no longer exists, the talent now adds passive auras and no longer related to 361195
-- Snapfire no longer exists and no longer needed for Firestorm.
-- Permeating Chill talent got removed and proc added in passive aura.
DELETE FROM `spell_script_names` WHERE (`spell_id` = '361195') and (`ScriptName` = 'spell_evo_call_of_ysera');
DELETE FROM `spell_script_names` WHERE (`spell_id` = '368847') and (`ScriptName` = 'spell_evo_snapfire');
DELETE FROM `spell_script_names` WHERE (`spell_id` = '369374') and (`ScriptName` = 'spell_evo_snapfire_bonus_damage');
DELETE FROM `spell_script_names` WHERE (`spell_id` = '381773') and (`ScriptName` = 'spell_evo_permeating_chill');

