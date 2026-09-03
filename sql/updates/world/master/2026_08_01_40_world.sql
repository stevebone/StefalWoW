-- Hunter Marksmanship + Survival artifact acquisitions. See zone_orderhall_hunter.cpp.

-- Marksmanship "Call of the Marksman" (40392): scripted flight to the Broken Shore + Vereesa credits.
UPDATE `quest_template_addon` SET `ScriptName` = 'quest_call_of_the_marksman'   WHERE `ID` = 40392;

-- Survival "The Eagle Spirit's Blessing" (39427): flight to Spiritwatch Point + slay Degar Bloodtotem + blessing.
UPDATE `quest_template_addon` SET `ScriptName` = 'quest_eagle_spirits_blessing' WHERE `ID` = 39427;
UPDATE `creature_template`     SET `ScriptName` = 'npc_degar_bloodtotem'         WHERE `entry` = 110685;

-- Spawn the missing Survival encounter at Spiritwatch Point / Skyhorn (Highmountain, map 1220): the obj-2 boss Degar
-- Bloodtotem (110685, made hostile by its script) and the questender Apata Highmountain (110821, questgiver flag).
DELETE FROM `creature` WHERE `guid` BETWEEN 50040010 AND 50040011;
INSERT INTO `creature`
  (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`)
VALUES
  (50040010, 110685, 1220, 7503, 8335, '0', 0, 0, 0, -1, 0, 0, 4757.0, 3928.0, 809.0, 3.00, 300, 0, 0, 100, 0, NULL, NULL, NULL, NULL, '', NULL, 0),
  (50040011, 110821, 1220, 7503, 8335, '0', 0, 0, 0, -1, 0, 0, 4762.0, 3924.0, 809.0, 1.50, 300, 0, 0, 100, 3,    NULL, NULL, NULL, NULL, '', NULL, 0);
