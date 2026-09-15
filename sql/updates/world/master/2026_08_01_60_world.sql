-- Hunter BM scenario: spawn the Image of Mimiron (106557) at the Titan Chest in Volund's hoard (map 1495), so the
-- hoard-room set-piece from the reference video plays. Non-combat spectral NPC; its line is triggered by the escort
-- director (zone_orderhall_hunter.cpp) when the player reaches the hoard.
DELETE FROM `creature` WHERE `guid` = 50040020;
INSERT INTO `creature`
  (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`)
VALUES
  (50040020, 106557, 1495, 7541, 0, '12', 0, 0, 0, -1, 0, 0, 4988.0, 296.0, -27.0, 3.10, 300, 0, 0, 100, 0, NULL, NULL, NULL, NULL, '', NULL, 0);

-- Stage-2 mini-boss Stormweaver Ingrida (105122): scripted hostile + video lines.
UPDATE `creature_template` SET `ScriptName` = 'npc_stormweaver_ingrida' WHERE `entry` = 105122;
