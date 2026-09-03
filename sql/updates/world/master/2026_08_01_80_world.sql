-- Druid Guardian artifact "Claws of Ursoc" (quest 40647 "When Dreams Become Nightmares", Ursoc's Lair map 1536).
-- The quest existed with 3 objectives but had no quest-giver spawned and no boss on the lair map, so it was
-- unobtainable and had no encounter. Spawn the giver/ender Lea in the Dreamgrove + Malithar at the Claws, and bind the
-- acquisition scripts.

-- Bind the QuestScript (teleports into the lair on accept) and the Malithar boss AI.
UPDATE `quest_template_addon` SET `ScriptName` = 'quest_when_dreams_become_nightmares' WHERE `ID` = 40647;
UPDATE `creature_template` SET `ScriptName` = 'npc_malithar' WHERE `entry` = 101390;

-- Spawns: giver Lea (104535) + ender Lea (105354) in the Dreamgrove beside the other Druid artifact givers
-- (Rensar 101195 / Lyessa 104577 stand at ~3969,7394,24 on map 1220); Malithar (101390) at the Claws of Ursoc on the
-- lair map 1536 (the retail fight spot - he claims the Claws and fights there). Malithar is hostile via his AI Reset().
DELETE FROM `creature` WHERE `guid` IN (50052000,50052001,50052002);
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(50052000,104535,1220,0,0,'0',0,0,0,-1,0,0,3968.0,7392.0,24.0,5.35,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0),
(50052001,105354,1220,0,0,'0',0,0,0,-1,0,0,3971.0,7398.0,24.0,5.35,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0),
(50052002,101390,1536,0,0,'0',0,0,0,-1,0,0,-12174.6,-13118.8,333.62,1.06,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0);
