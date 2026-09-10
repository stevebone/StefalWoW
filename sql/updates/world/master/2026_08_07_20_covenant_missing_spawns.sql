--
-- Covenant remediation P0.5 - missing sanctum spawns.
--
-- Three covenant sanctum features have a complete creature_template / gameobject_template but ZERO spawns, so the
-- feature is unreachable in game:
--   * Night Fae Command Table (172400)      - Heart of the Forest. Template + gossip menu 26161 (OptionNpc 31) ready.
--                                             The other three covenants' Command Tables (154527 / 166143 / 175136)
--                                             are already spawned.
--   * Necrolord Renown Quartermaster (172176 "Su Zettai", npcflag 128 VENDOR) - Seat of the Primus.
--   * Anima Conductor gameobjects 350776 and 350777.
--
-- COVENANT ASSIGNMENT OF THE TWO ANIMA CONDUCTORS (derived, not assumed):
--   gameobject_template.type 58 carries the GarrTalentTreeID in Data1.
--     350776 -> Data1 348 -> GarrTalentTree 348: GarrTypeID 111, FeatureTypeIndex 7 (Channel Anima),
--                            FeatureSubtypeIndex 2 => VENTHYR   -> Sinfall.
--     350777 -> Data1 346 -> GarrTalentTree 346: GarrTypeID 111, FeatureTypeIndex 7 (Channel Anima),
--                            FeatureSubtypeIndex 3 => NIGHT FAE -> Heart of the Forest.
--   The other two covenants' conductors are already spawned and corroborate the mapping:
--     328302 -> tree 345 -> FeatureSubtypeIndex 1 (Kyrian, spawned in area 11012)
--     348675 -> tree 347 -> FeatureSubtypeIndex 4 (Necrolord, spawned in area 12876)
--   So 350776 and 350777 belong to DIFFERENT covenants and must NOT both be placed in Sinfall.
--
-- COORDINATES ARE APPROXIMATE. There is no positional dump or Shadowlands-era sniff for these four objects in the
-- workspace, so each is anchored a few yards from an already-spawned NPC of the SAME covenant's sanctum (guaranteed
-- valid ground, correct area/phase). Replace with sniffed positions when one becomes available.
--   Night Fae  -> Zayhad, The Builder (165702) @ (-6870.43, 1069.99, 5671.26)
--   Necrolord  -> Elspeth Larink (175998, Keeper of Renown) @ (1821.12, -2524.59, 3385.07)
--   Venthyr    -> Foreman Flatfinger (172605) @ (-1860.89, 7634.70, 4193.81)
--
-- zoneId/areaId follow whatever the anchor row uses; the core recomputes them from the position when
-- CONFIG_CALCULATE_CREATURE_ZONE_AREA_DATA is on, so the position is the authoritative field.
--
-- Idempotent: each spawn is removed by template id before being re-inserted.
--

-- ---------------------------------------------------------------------------------------------------------------
-- Creatures
-- ---------------------------------------------------------------------------------------------------------------
DELETE FROM `creature` WHERE `id` IN (172400, 172176);

SET @CGUID := (SELECT IFNULL(MAX(`guid`), 0) + 1 FROM `creature`);

-- (1) Night Fae Command Table (172400) - Heart of the Forest, map 2222. Opens Adventures via gossip menu 26161.
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `MovementType`) VALUES
(@CGUID + 0, 172400, 2222, 0, 0, '0', 0, 0, 0, -6873.43, 1069.99, 5671.26, 1.85944, 7200, 0, 0);

-- (2) Necrolord Renown Quartermaster "Su Zettai" (172176) - Seat of the Primus, area 12876. Vendor (npcflag 128),
--     placed beside the Necrolord Keeper of Renown he trades alongside.
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `MovementType`) VALUES
(@CGUID + 1, 172176, 2222, 11462, 12876, '0', 0, 0, 0, 1824.12, -2524.59, 3385.07, 4.79229, 7200, 0, 0);

-- ---------------------------------------------------------------------------------------------------------------
-- Gameobjects
-- ---------------------------------------------------------------------------------------------------------------
DELETE FROM `gameobject` WHERE `id` IN (350776, 350777);

SET @OGUID := (SELECT IFNULL(MAX(`guid`), 0) + 1 FROM `gameobject`);

-- (3) Venthyr Anima Conductor (350776, Channel Anima tree 348) - Sinfall, zone 10413 / area 12917.
--     spawntimesecs / animprogress copied from the already-spawned Kyrian and Necrolord conductors.
INSERT INTO `gameobject` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) VALUES
(@OGUID + 0, 350776, 2222, 10413, 12917, '0', 0, 0, 0, -1864.89, 7634.70, 4193.81, 2.62036, 0, 0, 0.966231, 0.257676, 7200, 255, 1);

-- (4) Night Fae Anima Conductor (350777, Channel Anima tree 346) - Heart of the Forest.
INSERT INTO `gameobject` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) VALUES
(@OGUID + 1, 350777, 2222, 0, 0, '0', 0, 0, 0, -6867.43, 1069.99, 5671.26, 1.85944, 0, 0, 0.801453, 0.598058, 7200, 255, 1);
