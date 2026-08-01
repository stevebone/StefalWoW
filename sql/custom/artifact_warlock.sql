-- =====================================================================================================================
-- Warlock Legion artifact acquisition - script bindings + missing scenario spawns
-- Assigned guid block: 50046000..50046199
-- =====================================================================================================================

-- ---------------------------------------------------------------------------------------------------------------------
-- Quest -> QuestScript bindings (INSERT IGNORE guarantees the addon row exists, then bind the ScriptName)
-- ---------------------------------------------------------------------------------------------------------------------
INSERT IGNORE INTO `quest_template_addon` (`ID`) VALUES
(40495),(40623),(42128),(42168),(42125),(43100),(43153),(43254);

UPDATE `quest_template_addon` SET `ScriptName`='quest_warlock_ulthalesh'        WHERE `ID`=40495; -- Affliction open-world lead-in
UPDATE `quest_template_addon` SET `ScriptName`='quest_warlock_dark_riders'      WHERE `ID`=40623; -- Affliction Karazhan Catacombs scenario 988
UPDATE `quest_template_addon` SET `ScriptName`='quest_warlock_ritual_reagents'  WHERE `ID`=42128; -- Demonology reagent gather
UPDATE `quest_template_addon` SET `ScriptName`='quest_warlock_looking_darkness' WHERE `ID`=42168; -- Demonology Felsoul Hold scenario 1097
UPDATE `quest_template_addon` SET `ScriptName`='quest_warlock_dark_whispers'    WHERE `ID`=42125; -- Demonology skull obtained
UPDATE `quest_template_addon` SET `ScriptName`='quest_warlock_finding_scepter'  WHERE `ID`=43100; -- Destruction open-world Dalaran Crater
UPDATE `quest_template_addon` SET `ScriptName`='quest_warlock_eye_for_a_scepter' WHERE `ID`=43153; -- Destruction Tol Barad scenario 1155
UPDATE `quest_template_addon` SET `ScriptName`='quest_warlock_ritual_ruination' WHERE `ID`=43254; -- Destruction finale

-- ---------------------------------------------------------------------------------------------------------------------
-- Creature -> ScriptName bindings (faction of the placeholder/hostile bosses is set in C++ Reset(), NOT here)
-- ---------------------------------------------------------------------------------------------------------------------
UPDATE `creature_template` SET `ScriptName`='npc_warlock_afflic_director'        WHERE `entry`=100323; -- Revil Kost (director on 1533; also 40495 ender on map 0)
UPDATE `creature_template` SET `ScriptName`='npc_warlock_demo_director'          WHERE `entry`=106610; -- Calydus (Felsoul Hold director/ender on 1498)
UPDATE `creature_template` SET `ScriptName`='npc_warlock_destro_director'        WHERE `entry`=109838; -- Calydus (Tol Barad director/ender on 1630)
UPDATE `creature_template` SET `ScriptName`='npc_warlock_artifact_hostile_boss'  WHERE `entry`=102200; -- Ariden placeholder (faction 35 -> hostile)
UPDATE `creature_template` SET `ScriptName`='npc_warlock_artifact_hostile_boss'  WHERE `entry`=106644; -- Felborn Overfiend (scenario 1097 step-1 boss)
UPDATE `creature_template` SET `ScriptName`='npc_warlock_artifact_hostile_boss'  WHERE `entry`=106757; -- Eye of the Beast (scenario 1155 finale boss)

-- ---------------------------------------------------------------------------------------------------------------------
-- Missing spawns: scenario directors / quest-enders and the two un-spawned encounter bosses
-- ---------------------------------------------------------------------------------------------------------------------
DELETE FROM `creature` WHERE `guid` BETWEEN 50046000 AND 50046199;

-- Affliction (Karazhan Catacombs scenario 988, map 1533; Dreadscar return map 1107)
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(50046000,100323,1533,0,0,'0',0,0,0,-1,0,0,-10865.3,-1961.7,-41.0,3.29,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0), -- Revil Kost, scenario director on 1533
(50046001,100812,1107,0,0,'0',0,0,0,-1,0,0,3124.0,1108.0,286.6,4.65,300,0,0,100,0,3,NULL,NULL,NULL,'',NULL,0);    -- Revil Kost, 40623 quest-ender at the Dreadscar

-- Demonology (Felsoul Hold scenario 1097, map 1498)
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(50046010,106610,1498,0,0,'0',0,0,0,-1,0,0,999.0,4920.0,36.0,2.20,300,0,0,100,0,3,NULL,NULL,NULL,'',NULL,0),   -- Calydus, Felsoul Hold director + 42128/42168 ender
(50046011,106644,1498,0,0,'0',0,0,0,-1,0,0,1010.0,4930.0,36.0,2.20,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0); -- Felborn Overfiend, scenario 1097 step-1 boss

-- Destruction (Tol Barad scenario 1155, map 1630; Dreadscar return map 1107)
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(50046020,109838,1630,0,0,'0',0,0,0,-1,0,0,-1038.6,1151.6,99.6,3.87,300,0,0,100,0,3,NULL,NULL,NULL,NULL,'',NULL,0),  -- Calydus, Tol Barad director + 43153 ender
(50046021,106757,1630,0,0,'0',0,0,0,-1,0,0,-1030.0,1145.0,99.6,3.87,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0),   -- Eye of the Beast, scenario 1155 finale boss
(50046022,109698,1107,0,0,'0',0,0,0,-1,0,0,3118.0,1104.0,286.6,4.65,300,0,0,100,0,3,NULL,NULL,NULL,NULL,'',NULL,0);   -- Calydus, 43100 quest-ender at the Dreadscar
