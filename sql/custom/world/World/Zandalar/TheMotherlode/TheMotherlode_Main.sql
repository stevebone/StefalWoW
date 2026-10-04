-- ============================================================
-- The MOTHERLODE!! - Database Setup (Map 1594)
-- ============================================================
-- Registers the native instance script, binds the four boss
-- creature entries to their BossAI scripts and installs the
-- custom telegraph areatriggers used by the encounter scripts.
--
-- Boss/add spawns are expected to come from the world database
-- (TDB). Required creature_template entries:
--   Bosses : 129214, 129227, 129231, 129232
--   Adds   : 129246 (Footbomb), 140636 (Coin Pile),
--            129802 (Earthrager), 141303 (B.O.O.M.B.A.),
--            131247 (Skyscorcher)
-- Verify with: SELECT entry, guid FROM creature WHERE map = 1594;
-- If any creature row carries its own ScriptName it overrides the
-- template value - clear stale per-spawn bindings if present.
-- ============================================================

-- instance_template: Register Map 1594 with our InstanceScript
INSERT INTO instance_template (map, parent, script)
VALUES (1594, 0, 'custom_instance_the_motherlode')
ON DUPLICATE KEY UPDATE script = 'custom_instance_the_motherlode';

-- creature_template: Boss script bindings
UPDATE creature_template SET ScriptName = 'boss_coin_operated_crowd_pummeler' WHERE entry = 129214;
UPDATE creature_template SET ScriptName = 'boss_azerokk'                      WHERE entry = 129227;
UPDATE creature_template SET ScriptName = 'boss_rixxa_fluxflame'              WHERE entry = 129231;
UPDATE creature_template SET ScriptName = 'boss_mogul_razdunk'                WHERE entry = 129232;

-- World States
DELETE FROM `world_state` WHERE `ID` IN (14432, 14435, 14437, 14439);
INSERT INTO `world_state` (`ID`, `DefaultValue`, `MapIDs`, `AreaIDs`, `ScriptName`, `Comment`) VALUES
(14432, 0, 1594, NULL, '', 'The MOTHERLODE!! - Coin-Operated Crowd Pummeler - Encounter completed'),
(14435, 0, 1594, NULL, '', 'The MOTHERLODE!! - Azerokk - Encounter completed'),
(14437, 0, 1594, NULL, '', 'The MOTHERLODE!! - Rixxa Fluxfume - Encounter completed'),
(14439, 0, 1594, NULL, '', 'The MOTHERLODE!! - Mogul Razdunk - Encounter completed');


-- areatrigger_template + areatrigger_create_properties:
-- circular warning decals (4y radius, SpellForVisuals-driven) used by the
-- MOTHERLODE telegraphs. Create-properties ids 91400201-91400211, IsCustom = 1.
DELETE FROM `areatrigger_template` WHERE `IsCustom` = 1 AND `Id` IN (15623,15262,14739,12307,12803,15055,12208);
DELETE FROM `areatrigger_template` WHERE `IsCustom` = 0 AND `Id` IN (10202,16446);
INSERT INTO `areatrigger_template` (`Id`,`IsCustom`,`Flags`,`ActionSetId`,`ActionSetFlags`,`VerifiedBuild`) VALUES
(15623,1,0,0,0,0), -- Coin Impact (Pummeler) -- 16261 for Mithic
(14739,1,0,0,0,0), -- Footbomb Blast (Pummeler)
(15262,1,0,0,0,0), -- Shocking Claw (Pummeler)
(16446,0,0,0,0,69814), -- Tectonic Smash (Azerokk)
(12208,1,0,0,0,0), -- Catalyst Fire (Rixxa)
(12307,1,0,0,0,0), -- Propellant Jet (Rixxa)
(10202,0,0,0,0,69814), -- Missile Blast (Razdunk)
(12803,1,0,0,0,0), -- Big Red Rocket (Razdunk)
(15055,1,0,0,0,0); -- Drill Smash (Razdunk)

DELETE FROM `areatrigger_create_properties` WHERE `IsCustom` = 1 AND `Id` IN (15623,15262,14739,1409,12208,12307,5500,12803,15055);
DELETE FROM `areatrigger_create_properties` WHERE `IsCustom` = 0 AND `Id` IN (1409,5500);
INSERT INTO `areatrigger_create_properties` (`Id`,`IsCustom`,`AreaTriggerId`,`IsAreatriggerCustom`,`Flags`,`MoveCurveId`,`ScaleCurveId`,`MorphCurveId`,`FacingCurveId`,`AnimId`,`AnimKitId`,`DecalPropertiesId`,`SpellForVisuals`,`TimeToTargetScale`,`Speed`,`SpeedIsTime`,`Shape`,`ShapeData0`,`ShapeData1`,`ShapeData2`,`ShapeData3`,`ShapeData4`,`ShapeData5`,`ShapeData6`,`ShapeData7`,`ScriptName`,`VerifiedBuild`) VALUES
(15623,1,15623,1,0,0,0,0,0,-1,0,0,287073,0,1.0,0,0,4.0,4.0,0.0,0.0,0.0,0.0,0.0,0.0,'',0),
(14739,1,14739,1,0,0,0,0,0,-1,0,0,277289,0,1.0,0,0,4.0,4.0,0.0,0.0,0.0,0.0,0.0,0.0,'',0),
(15262,1,15262,1,0,0,0,0,0,-1,0,0,284285,0,1.0,0,0,4.0,4.0,0.0,0.0,0.0,0.0,0.0,0.0,'',0),
(1409, 0,16446,0,0,0,0,0,0,-1,0,0,268078,0,8000, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, '', 69814),
-- (91400204,1,91400204,1,0,0,0,0,0,-1,0,0,268078,0,1.0,0,0,4.0,4.0,0.0,0.0,0.0,0.0,0.0,0.0,'',0),
(12208,1,12208,1,0,0,0,0,0,-1,0,0,259787,0,1.0,0,0,4.0,4.0,0.0,0.0,0.0,0.0,0.0,0.0,'',0),
(12307,1,12307,1,0,0,0,0,0,-1,0,0,260669,0,1.0,0,0,4.0,4.0,0.0,0.0,0.0,0.0,0.0,0.0,'',0),
(5500, 0,10202,0,0,0,0,0,0,-1,0,0,263521,0,2000, 6, 0, 0, 1, 1, 0, 0, 0, 0, 0, '', 69814),
-- (91400207,1,91400207,1,0,0,0,0,0,-1,0,0,263521,0,1.0,0,0,4.0,4.0,0.0,0.0,0.0,0.0,0.0,0.0,'',0),
(12803,1,12803,1,0,0,0,0,0,-1,0,0,264456,0,1.0,0,0,4.0,4.0,0.0,0.0,0.0,0.0,0.0,0.0,'',0),
(15055,1,15055,1,0,0,0,0,0,-1,0,0,282463,0,1.0,0,0,4.0,4.0,0.0,0.0,0.0,0.0,0.0,0.0,'',0);
