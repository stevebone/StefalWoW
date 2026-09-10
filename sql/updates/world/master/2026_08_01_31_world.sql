-- Beast Mastery Hunter artifact - Leg 3 "Never Hunt Alone" (42185): the Temple of Storms (map 1609), scenario 1099.
-- See zone_orderhall_hunter.cpp.

-- Link map 1609 (difficulty 12) to InstanceScenario 1099 "Never Hunt Alone" so it auto-starts on entry, mirroring the
-- Shield's Rest (1495 -> 1068) link.
DELETE FROM `scenarios` WHERE `map` = 1609 AND `difficulty` = 12;
INSERT INTO `scenarios` (`map`, `difficulty`, `dungeonID`, `scenario_A`, `scenario_H`) VALUES
(1609, 12, 0, 1099, 1099);

-- Scenario 1099 director (temple Grif) + the traitor Prustaga's temple encounter.
UPDATE `creature_template` SET `ScriptName` = 'npc_grif_temple_director' WHERE `entry` = 106715;
UPDATE `creature_template` SET `ScriptName` = 'npc_prustaga_temple'      WHERE `entry` = 106744;

-- Leg 3 quest: grant "See Mimiron's Head" (106671) on accept, fly to the temple (106672), recover Titanstrike (114509).
UPDATE `quest_template_addon` SET `ScriptName` = 'quest_never_hunt_alone' WHERE `ID` = 42185;

-- Spawn the missing antagonists on map 1609 (only the allies Grif/Thorim/Hati ship spawned). Prustaga 106744 is
-- repurposed here (it spawns nowhere else); the Restless Tombguards 106302 are the vrykul horde of scenario step 1.
-- The director/AI make them hostile and drive the scenario steps.
DELETE FROM `creature` WHERE `guid` BETWEEN 50040001 AND 50040004;
INSERT INTO `creature`
  (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`)
VALUES
  (50040001, 106744, 1609, 67, 8176, '12', 0, 0, 0, -1, 0, 0, 7444.0, -528.0, 1896.9, 3.00, 300, 0, 0, 100, 0, NULL, NULL, NULL, NULL, '', NULL, 0),
  (50040002, 106302, 1609, 67, 8176, '12', 0, 0, 0, -1, 0, 0, 7435.0, -525.0, 1896.9, 3.00, 300, 0, 0, 100, 0, NULL, NULL, NULL, NULL, '', NULL, 0),
  (50040003, 106302, 1609, 67, 8176, '12', 0, 0, 0, -1, 0, 0, 7448.0, -524.0, 1896.9, 3.00, 300, 0, 0, 100, 0, NULL, NULL, NULL, NULL, '', NULL, 0),
  (50040004, 106302, 1609, 67, 8176, '12', 0, 0, 0, -1, 0, 0, 7440.0, -518.0, 1896.9, 3.00, 300, 0, 0, 100, 0, NULL, NULL, NULL, NULL, '', NULL, 0);
