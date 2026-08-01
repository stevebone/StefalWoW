-- ===== Mage =====
-- =====================================================================================================================
-- Mage Legion artifact acquisition bindings (artifact_mage.cpp)
-- =====================================================================================================================
--
-- Arcane  "The Nexus Vault" (42011) -> Aluneth      : scenario 1101, map 1583, boss Bilaal 104502, ender Kalec 105081
-- Frost   "The Mage Hunter"  (42479) -> Ebonchill    : scenario 1122, map 1616, boss Balaadur 108168, ender Meryl 102700
-- Fire    Felo'melorn                                : NO acquisition quest exists in this DB -> not bound (see .cpp footer)

-- --- QuestScript bindings ---------------------------------------------------------------------------------------------
UPDATE `quest_template_addon` SET `ScriptName`='quest_the_nexus_vault' WHERE `ID`=42011;
UPDATE `quest_template_addon` SET `ScriptName`='quest_the_mage_hunter' WHERE `ID`=42479;

-- --- Creature (scenario director + boss) bindings --------------------------------------------------------------------
-- Arcane (map 1583)
UPDATE `creature_template` SET `ScriptName`='npc_azuregos_nexus_director' WHERE `entry`=106699; -- Azuregos narrator/director
UPDATE `creature_template` SET `ScriptName`='npc_nexus_prince_bilaal'     WHERE `entry`=104502; -- Nexus-Prince Bilaal boss
-- Frost (map 1616)
UPDATE `creature_template` SET `ScriptName`='npc_meryl_mage_hunter_director' WHERE `entry`=108097; -- Meryl narrator/director
UPDATE `creature_template` SET `ScriptName`='npc_balaadur'                   WHERE `entry`=108168; -- Balaadur boss

-- --- Spawns ----------------------------------------------------------------------------------------------------------
-- Every creature the scripts bind (Azuregos 106699 & Bilaal 104502 on map 1583; Meryl 108097 & Balaadur 108168 on map
-- 1616) and both quest enders (Kalec 105081, Meryl 102700 on map 1220) are ALREADY spawned in creature, so no new
-- spawns are required. The DELETE reserves this task's guid block (50045000..50045199) and keeps re-imports idempotent.
DELETE FROM `creature` WHERE `guid` BETWEEN 50045000 AND 50045199;

-- ===== DemonHunter =====
-- ============================================================================
-- Demon Hunter Legion artifact acquisition scripts (artifact_demonhunter.cpp)
-- ============================================================================

-- Bind the acquisition quests to their QuestScripts.
UPDATE `quest_template_addon` SET `ScriptName`='quest_dh_the_hunt' WHERE `ID`=39247;                    -- Havoc: The Hunt
UPDATE `quest_template_addon` SET `ScriptName`='quest_dh_vengeance_will_be_ours' WHERE `ID`=41863;      -- Vengeance: Vengeance Will Be Ours
UPDATE `quest_template_addon` SET `ScriptName`='quest_dh_vengeance_will_be_ours' WHERE `ID`=40249;      -- Vengeance: phase-duplicate variant (same wire)

-- Bind the scripted creatures.
UPDATE `creature_template` SET `ScriptName`='npc_varedis_felsoul'                WHERE `entry`=94836;   -- Havoc final boss (map 1498)
UPDATE `creature_template` SET `ScriptName`='npc_caria_felsoul'                  WHERE `entry`=99184;   -- Vengeance final boss (map 1500)
UPDATE `creature_template` SET `ScriptName`='npc_dh_artifact_scenario_director'  WHERE `entry`=98882;   -- Vengeance ally/director Allari (map 1500, already spawned)
UPDATE `creature_template` SET `ScriptName`='npc_dh_artifact_scenario_director'  WHERE `entry`=94902;   -- Havoc director Kayn (spawned below on map 1498)

-- Missing spawn: no friendly Illidari director exists on Felsoul Hold (map 1498); place Kayn Sunfury (94902,
-- faction 2838 friendly) at the scenario landing to act as the scenario director. (All other required actors -
-- Varedis 94836, Caria 99184, Allari 98882, Illidari Fel Bats / Kill-Credits, and the ender Kor'vas 102799 - are
-- already spawned or are invisible kill-credits, so no further creature rows are needed.)
DELETE FROM `creature` WHERE `guid` BETWEEN 50051000 AND 50051199;
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(50051000,94902,1498,0,0,'0',0,0,0,-1,0,0,1230.0,5010.0,58.0,3.90,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0);

-- ===== Warrior =====
-- =====================================================================================================================
-- Warrior Legion artifact acquisition (all specs) - script bindings.
-- Arms  41105 "The Sword of Kings"       -> quest_the_sword_of_kings      (scenario 1037, map 1539)
-- Fury  40043 "The Hunter of Heroes"     -> quest_the_hunter_of_heroes    (killcredit, map 1511)
-- Prot  39191 "Legacy of the Icebreaker" -> quest_legacy_of_the_icebreaker(killcredit, map 1495)
-- All three start/end at Odyn (96469) in Skyhold (map 1479).
-- =====================================================================================================================

-- Bind each acquisition quest to its QuestScript.
UPDATE `quest_template_addon` SET `ScriptName`='quest_the_sword_of_kings'       WHERE `ID`=41105;
UPDATE `quest_template_addon` SET `ScriptName`='quest_the_hunter_of_heroes'     WHERE `ID`=40043;
UPDATE `quest_template_addon` SET `ScriptName`='quest_legacy_of_the_icebreaker' WHERE `ID`=39191;

-- Bind each scripted creature.
UPDATE `creature_template` SET `ScriptName`='npc_thoradin_scenario_director' WHERE `entry`=103144; -- Arms director (map 1539)
UPDATE `creature_template` SET `ScriptName`='npc_zakajz_corruptor'           WHERE `entry`=104276; -- Arms final boss
UPDATE `creature_template` SET `ScriptName`='npc_vigfus_bladewind_final'     WHERE `entry`=98602;  -- Fury final boss (map 1511)
UPDATE `creature_template` SET `ScriptName`='npc_magnar_icebreaker'          WHERE `entry`=96034;  -- Prot boss (map 1495)
UPDATE `creature_template` SET `ScriptName`='npc_hruthnir_escort'            WHERE `entry`=96468;  -- Prot ally (kept friendly)

-- No static creature spawns are required: Thoradin(103144)+Zakajz(104276) on 1539, Vigfus(98602) on 1511, and
-- Magnar(96034)+Hruthnir(96468) on 1495 are all already spawned, and the questender Odyn(96469) is spawned in Skyhold.
-- The only research-flagged MISSING creature, Soth'ozz the Guardian (104591), is script-summoned hostile by the Arms
-- director, so no INSERT is needed. Clear the assigned guid block so it stays free/idempotent.
DELETE FROM `creature` WHERE `guid` BETWEEN 50041000 AND 50041199;

-- ===== Druid =====
-- =====================================================================================================================
-- Druid Legion artifact acquisition - script bindings + missing spawns.
-- Guid block assigned to this class: 50048000..50048199 (all spawns below are inside it).
-- =====================================================================================================================

-- --- QuestScript bindings (INSERT-or-UPDATE so a missing quest_template_addon row is created) ------------------------
INSERT INTO `quest_template_addon` (`ID`,`ScriptName`) VALUES
 (40783,'quest_scythe_of_elune'),
 (40784,'quest_its_rightful_place'),
 (42428,'quest_shrine_of_ashamane'),
 (42440,'quest_shrine_in_peril'),
 (42430,'quest_fangs_of_ashamane'),
 (41689,'quest_cleansing_the_mother_tree')
ON DUPLICATE KEY UPDATE `ScriptName`=VALUES(`ScriptName`);

-- --- CreatureAI bindings (templates already exist) ------------------------------------------------------------------
UPDATE `creature_template` SET `ScriptName`='npc_ebonfang'           WHERE `entry`=107729;
UPDATE `creature_template` SET `ScriptName`='npc_eredar_soul_lasher' WHERE `entry`=107535;

-- --- Missing spawns --------------------------------------------------------------------------------------------------
DELETE FROM `creature` WHERE `guid` BETWEEN 50048000 AND 50048199;

-- Feral: Shrine of Ashamane, Val'sharah (map 1220), anchored on Danise Stargazer (4075.33, 7243.44, 52.23).
--   Delandros Shimmermoon 107392 - chain quest-giver/ender for 42428/42439/42440/42430 (npcflag=3 gossip+quest).
--   Verstok Darkbough 107520     - 42428 obj1 kill (faction 2850, already hostile).
--   Ebonfang 107729              - 42430 obj0 boss (faction 35 placeholder -> npc_ebonfang makes it hostile).
--   Eredar Soul Lasher 107535 x6 - 42439 obj0 kill x4 (faction 35 placeholder -> npc_eredar_soul_lasher hostile).
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
 (50048000,107392,1220,0,0,'0',0,0,0,-1,0,0,4080.0,7246.0,52.2,4.20,300,0,0,100,0,3,NULL,NULL,NULL,'',NULL,0),
 (50048001,107520,1220,0,0,'0',0,0,0,-1,0,0,4086.0,7250.0,52.2,4.20,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0),
 (50048002,107729,1220,0,0,'0',0,0,0,-1,0,0,4090.0,7228.0,52.0,3.10,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0),
 (50048003,107535,1220,0,0,'0',0,0,0,-1,0,0,4066.0,7252.0,52.2,4.20,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0),
 (50048004,107535,1220,0,0,'0',0,0,0,-1,0,0,4070.0,7255.0,52.2,4.20,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0),
 (50048005,107535,1220,0,0,'0',0,0,0,-1,0,0,4074.0,7250.0,52.2,4.20,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0),
 (50048006,107535,1220,0,0,'0',0,0,0,-1,0,0,4068.0,7258.0,52.2,4.20,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0),
 (50048007,107535,1220,0,0,'0',0,0,0,-1,0,0,4072.0,7258.0,52.2,4.20,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0),
 (50048008,107535,1220,0,0,'0',0,0,0,-1,0,0,4076.0,7254.0,52.2,4.20,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0),
-- Restoration: Lyessa Bloomwatcher 104577 - giver+ender of 41689, placed at the Dreamgrove (map 1220) beside Rensar
-- Greathoof (3969.63, 7393.94, 24.02) so the standalone quest can be accepted and handed in locally (npcflag=3).
 (50048010,104577,1220,0,0,'0',0,0,0,-1,0,0,3972.0,7396.0,24.02,5.35,300,0,0,100,0,3,NULL,NULL,NULL,'',NULL,0);

-- ===== Paladin =====
-- =====================================================================================================================
-- Paladin Legion artifact acquisition - script bindings + missing spawns (guid block 50042000..50042199).
-- Holy 42120 (scenario 1092/map 1539), Protection 42017 (scenario 1082/map 1495), Retribution 38376 (scenario 775/map 1500).
-- =====================================================================================================================

-- --- QuestScript bindings (acquisition quests) ---------------------------------------------------------------------
UPDATE `quest_template_addon` SET `ScriptName`='quest_the_silver_hand'            WHERE `ID`=42120;
UPDATE `quest_template_addon` SET `ScriptName`='quest_shrine_of_the_truthguard'   WHERE `ID`=42017;
UPDATE `quest_template_addon` SET `ScriptName`='quest_search_for_the_highlord'    WHERE `ID`=38376;

-- --- Creature ScriptName bindings ----------------------------------------------------------------------------------
-- Holy scenario director (Travard copy on map 1539) + boss.
UPDATE `creature_template` SET `ScriptName`='npc_travard_scenario_director' WHERE `entry`=106429;
UPDATE `creature_template` SET `ScriptName`='npc_horrific_aberration'       WHERE `entry`=106669;
-- Protection scenario director (Orik on map 1495) + boss.
UPDATE `creature_template` SET `ScriptName`='npc_orik_scenario_director'    WHERE `entry`=105910;
UPDATE `creature_template` SET `ScriptName`='npc_yrgrim_truthseeker'        WHERE `entry`=105695;
-- Retribution scenario director (Tirion on map 1500), re-factioned demons, and Balnazzar (kept faction 14).
UPDATE `creature_template` SET `ScriptName`='npc_tirion_scenario_director'  WHERE `entry`=92676;
UPDATE `creature_template` SET `ScriptName`='npc_broken_shore_demon'        WHERE `entry`=91672;
UPDATE `creature_template` SET `ScriptName`='npc_broken_shore_demon'        WHERE `entry`=91697;
UPDATE `creature_template` SET `ScriptName`='npc_balnazzar_risen'           WHERE `entry`=90981;

-- --- Missing order-hall spawns (Sanctum of Light, map 1220, zone 7502 / area 7505) ---------------------------------
-- 105813 Orik Trueheart is the Protection queststarter (0 spawns anywhere -> quest 42017 was unacceptable).
-- 90259 Lord Maxwell Tyrosus is the shared quest-ender for all three acquisition quests (only spawned on map 0);
--   a copy in the order hall makes each quest turn-in-able where the player returns after claiming the artifact.
-- npcflag=3 (gossip + quest) on both.
DELETE FROM `creature` WHERE `guid` BETWEEN 50042000 AND 50042199;
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(50042000,105813,1220,7502,7505,'0',0,0,0,-1,0,0,-850.5,4262.0,746.28,1.5,300,0,0,100,0,3,NULL,NULL,NULL,'',NULL,0),
(50042001,90259,1220,7502,7505,'0',0,0,0,-1,0,0,-845.0,4264.0,746.28,3.9,300,0,0,100,0,3,NULL,NULL,NULL,'',NULL,0);

-- ===== Rogue =====
-- =====================================================================================================================
-- Rogue artifact acquisition scripts: quest + creature ScriptName bindings and the missing Kingslayers spawns.
-- Verified against integ_world (2026-08): quests 42504/42627/40849/41924 confirmed; questenders 94141/94159/98102 all
-- present on map 1220; Dreadblades (map 1545) + Devourer (map 1607) actors present; Kingslayers (map 1620) actors ABSENT.
-- =====================================================================================================================

-- --- QuestScript bindings (quest_template_addon.ScriptName) ---
UPDATE `quest_template_addon` SET `ScriptName`='quest_kingslayers'    WHERE `ID` IN (42504,42627); -- Assassination
UPDATE `quest_template_addon` SET `ScriptName`='quest_dreadblades'    WHERE `ID`=40849;            -- Outlaw
UPDATE `quest_template_addon` SET `ScriptName`='quest_fangs_devourer' WHERE `ID`=41924;            -- Subtlety

-- --- Creature ScriptName bindings (creature_template.ScriptName) ---
-- Assassination / The Kingslayers (map 1620) - freshly spawned below.
UPDATE `creature_template` SET `ScriptName`='npc_kingslayers_director'    WHERE `entry`=108218; -- Sister Althea (mini-boss + director)
UPDATE `creature_template` SET `ScriptName`='npc_kingslayers_melris'      WHERE `entry`=107831; -- Melris Malagan (final boss)
-- Outlaw / The Dreadblades (map 1545) - already spawned.
UPDATE `creature_template` SET `ScriptName`='npc_dreadblades_director'    WHERE `entry`=102179; -- Fleet Admiral Tethys scenario-actor (director)
UPDATE `creature_template` SET `ScriptName`='npc_dreadblades_enemy'       WHERE `entry` IN (102185,102239); -- DeGauza, Brinebeard (faction-fix)
UPDATE `creature_template` SET `ScriptName`='npc_dreadblades_eliza'       WHERE `entry`=102293; -- Dread Admiral Eliza (final boss)
-- Subtlety / Fangs of the Devourer (map 1607) - already spawned.
UPDATE `creature_template` SET `ScriptName`='npc_devourer_director'       WHERE `entry`=105843; -- Fangs decorative (director)
UPDATE `creature_template` SET `ScriptName`='npc_devourer_enemy'          WHERE `entry` IN (105536,105542); -- intro Akaari, Xirus (faction-fix)
UPDATE `creature_template` SET `ScriptName`='npc_devourer_akaari_final'   WHERE `entry`=105660; -- final Akaari Shadowgore (final boss)

-- --- Missing Kingslayers spawns on map 1620 (guid block 50049000..50049199) ---
-- Bosses are faction-35 placeholders in the template; the C++ scripts make them hostile at runtime (no template edit).
DELETE FROM `creature` WHERE `guid` BETWEEN 50049000 AND 50049199;
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(50049000,107831,1620,0,0,'0',0,0,0,-1,0,0,1430.8,-1344.8,62.9,4.87,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0), -- Melris Malagan (final boss)
(50049001,108218,1620,0,0,'0',0,0,0,-1,0,0,1432.5,-1319.8,60.5,5.12,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0); -- Sister Althea Ebonlocke (mini-boss/director)

-- ===== Shaman =====
-- =====================================================================================================================
-- Shaman Legion artifact acquisition - script bindings + missing spawn (guid block 50044000..50044199)
-- =====================================================================================================================

-- Acquisition / flag quests -> QuestScripts
UPDATE `quest_template_addon` SET `ScriptName`='quest_voice_of_thunder'              WHERE `ID`=39771; -- Elemental "The Voice of Thunder" -> Fist of Ra-den
UPDATE `quest_template_addon` SET `ScriptName`='quest_restoration_artifact_chosen'   WHERE `ID`=41330; -- Restoration flag quest -> Sharas'dal
UPDATE `quest_template_addon` SET `ScriptName`='quest_enhancement_artifact_chosen'   WHERE `ID`=41328; -- Enhancement flag quest -> Doomhammer (stand-in)

-- Scripted creatures -> ScriptName (faction fixes / directors are all applied in C++, no creature_template.faction edits)
-- Elemental scenario 976 (map 1526)
UPDATE `creature_template` SET `ScriptName`='npc_rehgar_scenario_director' WHERE `entry`=100306; -- Rehgar Earthfury (director)
UPDATE `creature_template` SET `ScriptName`='npc_sigurd_giantslayer'       WHERE `entry`=100363; -- Sigurd the Giantslayer (step 1 boss, made hostile)
UPDATE `creature_template` SET `ScriptName`='npc_lord_kravos'              WHERE `entry`=100546; -- Lord Kra'vos (step 4 boss, made hostile)
-- Restoration scenario 1066 (map 1600)
UPDATE `creature_template` SET `ScriptName`='npc_erunak_scenario_director' WHERE `entry`=102826; -- Erunak Stonespeaker (director)
UPDATE `creature_template` SET `ScriptName`='npc_shaman_scenario_enemy'    WHERE `entry`=104856; -- Lady Zithreen (step 4 boss, made hostile)
UPDATE `creature_template` SET `ScriptName`='npc_shaman_scenario_enemy'    WHERE `entry`=102839; -- Kra'liss (step 2 boss, made hostile)
UPDATE `creature_template` SET `ScriptName`='npc_shaman_scenario_enemy'    WHERE `entry`=102792; -- Zithreenai Naga Brute (made hostile)
UPDATE `creature_template` SET `ScriptName`='npc_shaman_scenario_enemy'    WHERE `entry`=105027; -- Frenzied Deep Sea Crawler (made hostile)
UPDATE `creature_template` SET `ScriptName`='npc_shaman_scenario_enemy'    WHERE `entry`=105028; -- Frenzied Deep Sea Makrura (made hostile)

-- Missing spawn: Lord Kra'vos 100546 (Elemental scenario step-4 final boss) on map 1526 (Temple of the White Tiger).
-- Placed near "Weapons of the Storm" 101415 (3795.68,534.43,639.07) in the same zone/area (5841/7951) as the other
-- scenario spawns. Exact retail coords for Kra'vos on 1526 are not derivable offline (see risks).
DELETE FROM `creature` WHERE `guid` BETWEEN 50044000 AND 50044199;
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(50044000,100546,1526,5841,7951,'0',0,0,0,-1,0,0,3820.0,540.0,642.0,2.9,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0);

-- ===== Monk =====
-- =====================================================================================================================
-- Monk Legion artifact acquisition - script bindings + spawns (assigned guid block 50047000..50047199)
-- Paired with src/server/scripts/BrokenIsles/Orderhalls/artifact_monk.cpp
-- =====================================================================================================================

-- ---------------------------------------------------------------------------------------------------------------------
-- Brewmaster: "The Wanderer's Companion" (42762), scenario 1137 on map 1625 (Trial of the Serpent)
-- ---------------------------------------------------------------------------------------------------------------------
UPDATE `quest_template_addon` SET `ScriptName`='quest_the_wanderers_companion' WHERE `ID`=42762;

UPDATE `creature_template` SET `ScriptName`='npc_yulon_brm_director' WHERE `entry`=109391; -- Yu'lon (director)
UPDATE `creature_template` SET `ScriptName`='npc_lord_korithis'      WHERE `entry`=109821; -- Eredar Lord Korithis (boss/closer)
-- mid bosses made attackable (hostile faction set from script)
UPDATE `creature_template` SET `ScriptName`='npc_monk_artifact_hostile' WHERE `entry` IN (109397,109820);

-- ---------------------------------------------------------------------------------------------------------------------
-- Mistweaver: "The Emperor's Gift" (41003), scenario 1007 on map 1541 (Terrace of Endless Spring)
-- ---------------------------------------------------------------------------------------------------------------------
UPDATE `quest_template_addon` SET `ScriptName`='quest_the_emperors_gift' WHERE `ID`=41003;

UPDATE `creature_template` SET `ScriptName`='npc_taran_zhu_mw_director' WHERE `entry`=101881; -- Taran Zhu (director)
UPDATE `creature_template` SET `ScriptName`='npc_aspersius'             WHERE `entry`=101887; -- Aspersius (boss/closer)
UPDATE `creature_template` SET `ScriptName`='npc_monk_artifact_hostile' WHERE `entry`=101886; -- Hellwarden Xaphan (mini-boss)

-- ---------------------------------------------------------------------------------------------------------------------
-- Windwalker: "The Thundering Heavens" (42790), scenario 983 on map 1528 (Skywall)
-- No acquisition quest exists in the DB, so author a minimal one (cloned from 42762's shape).
-- ---------------------------------------------------------------------------------------------------------------------
DELETE FROM `quest_template`         WHERE `ID`=42790;
DELETE FROM `quest_template_addon`   WHERE `ID`=42790;
DELETE FROM `quest_objectives`       WHERE `QuestID`=42790;
DELETE FROM `creature_queststarter`  WHERE `quest`=42790;
DELETE FROM `creature_questender`    WHERE `quest`=42790;

-- clone 42762's full row, then retarget it to 42790
CREATE TEMPORARY TABLE `_ww_qt` LIKE `quest_template`;
INSERT INTO `_ww_qt` SELECT * FROM `quest_template` WHERE `ID`=42762;
UPDATE `_ww_qt` SET
    `ID`=42790,
    `RewardNextQuest`=0,
    `LogTitle`='The Thundering Heavens',
    `LogDescription`='Travel to Skywall with Li Li Stormstout and claim the Fists of the Heavens from the tyrant Typhinius.',
    `QuestDescription`='The Fists of the Heavens lie in the grasp of Typhinius, the Tyrant of Skywall. Li Li Stormstout will guide you there. Defeat him and the weapon is yours.',
    `VerifiedBuild`=0
WHERE `ID`=42762;
INSERT INTO `quest_template` SELECT * FROM `_ww_qt`;
DROP TEMPORARY TABLE `_ww_qt`;

INSERT INTO `quest_template_addon` (`ID`,`ScriptName`) VALUES (42790,'quest_the_thundering_heavens');

-- single objective: kill Typhinius (100760). His scripted-hostile death auto-credits this objective.
INSERT INTO `quest_objectives`
    (`ID`,`QuestID`,`Type`,`Order`,`StorageIndex`,`ObjectID`,`Amount`,`ConditionalAmount`,`Flags`,`Flags2`,`ProgressBarWeight`,`ParentObjectiveID`,`Visible`,`Description`,`VerifiedBuild`)
VALUES
    (24903000,42790,0,0,0,100760,1,0,0,0,0,0,1,'Defeat Typhinius',0);

-- Li Li Stormstout (101046, order-hall guide) gives and turns in the quest (see spawn below)
INSERT INTO `creature_queststarter` (`id`,`quest`) VALUES (101046,42790);
INSERT INTO `creature_questender`   (`id`,`quest`) VALUES (101046,42790);

-- Skywall boss + adds
UPDATE `creature_template` SET `ScriptName`='npc_lili_ww_director' WHERE `entry`=100740; -- Li Li Stormstout (director)
UPDATE `creature_template` SET `ScriptName`='npc_typhinius'        WHERE `entry`=100760; -- Typhinius (boss/closer)
UPDATE `creature_template` SET `ScriptName`='npc_monk_artifact_hostile' WHERE `entry` IN (100772,100770,100769,100824,100762); -- Skywall adds

-- ---------------------------------------------------------------------------------------------------------------------
-- Spawns (guid block 50047000..50047199)
-- ---------------------------------------------------------------------------------------------------------------------
DELETE FROM `creature` WHERE `guid` BETWEEN 50047000 AND 50047199;

-- Windwalker quest giver/ender: a second Li Li Stormstout (101046) in the Monk order hall (map 1514) with npcflag=3
-- (gossip+quest) so "The Thundering Heavens" can be picked up and handed in.
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(50047000,101046,1514,0,0,'0',0,0,0,-1,0,0,885.41,3607.41,192.42,3.53424,300,0,0,100,0,3,NULL,NULL,NULL,'',NULL,0);

-- ===== artifact_priest =====
-- =====================================================================================================================
-- Priest Legion artifact acquisition (Discipline 41625 / Holy 41957 / Shadow 40710)
-- Script bindings, scenario table rows, and the missing spawns. Guid block: 50050000..50050199.
-- =====================================================================================================================

-- Bind the acquisition quests to their QuestScripts.
UPDATE `quest_template_addon` SET `ScriptName`='quest_lights_wrath'      WHERE `ID`=41625; -- Discipline "The Light's Wrath"
UPDATE `quest_template_addon` SET `ScriptName`='quest_vindicators_plea' WHERE `ID`=41957; -- Holy "The Vindicator's Plea"
UPDATE `quest_template_addon` SET `ScriptName`='quest_blade_in_twilight' WHERE `ID`=40710; -- Shadow "Blade in Twilight"

-- Bind the scripted creatures (directors + bosses). Factions are handled in C++, not creature_template.faction.
UPDATE `creature_template` SET `ScriptName`='npc_azuregos_disc_director'     WHERE `entry`=106699; -- Nexus Vault director (ally)
UPDATE `creature_template` SET `ScriptName`='npc_judgments_flame'            WHERE `entry`=104520; -- Disc step-2 boss (-> hostile in C++)
UPDATE `creature_template` SET `ScriptName`='npc_nexus_prince_bilaal'        WHERE `entry`=104502; -- Disc step-4 boss (-> hostile in C++)
UPDATE `creature_template` SET `ScriptName`='npc_slaghammer_shadow_director' WHERE `entry`=101430; -- Blade in Twilight director (ally)
UPDATE `creature_template` SET `ScriptName`='npc_twilight_deacon_farthing'   WHERE `entry`=101148; -- Shadow step-7 boss (already hostile)
UPDATE `creature_template` SET `ScriptName`='npc_zakajz_corruptor'           WHERE `entry`=104276; -- Shadow step-9 boss (already hostile)

-- Register the two InstanceScenarios so the on-screen step UI runs on those maps (mirrors the Hunter maps 1495/1609).
-- Quest completion does NOT depend on these rows (it is script-driven); they only drive the scenario UI presentation.
DELETE FROM `scenarios` WHERE `map` IN (1583,1539);
INSERT INTO `scenarios` (`map`,`difficulty`,`dungeonID`,`scenario_A`,`scenario_H`) VALUES
(1583,12,0,1065,1065),  -- Nexus Vault (Discipline)
(1539,12,0, 991, 991);  -- Blade in Twilight (Shadow)

-- Missing spawns.
DELETE FROM `creature` WHERE `guid` BETWEEN 50050000 AND 50050199;

-- Shadow: Xal'atath step-8 artifact-claim NPC (111374) - not spawned on map 1539; place it in the Deacon/Zakajz cluster.
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(50050000,111374,1539,0,0,'0',0,0,0,-1,0,0,1878.0,2180.0,36.0,3.0,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0);

-- Holy: Brother Larry (105769, the 41957 TalkTo objective) and Vindicator Boros (105602, the 41957 ender) were not
-- spawned anywhere - place both at Netherlight Temple beside Alonsus Faol. npcflag=3 (gossip+quest) on the ender.
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(50050001,105769,1512,0,0,'0',0,0,0,-1,0,0,1336.0,1338.0,177.2,3.0,300,0,0,100,0,3,NULL,NULL,NULL,'',NULL,0),   -- Brother Larry (TalkTo target)
(50050002,105602,1512,0,0,'0',0,0,0,-1,0,0,1331.0,1333.0,177.2,3.0,300,0,0,100,0,3,NULL,NULL,NULL,'',NULL,0);   -- Vindicator Boros (ender)

-- ===== DeathKnight =====
-- ====================================================================================================================
-- Death Knight Legion artifact acquisition - ScriptName bindings + scenario/scenarios wiring.
-- Guid block assigned to this class: 50043000..50043199 (no creature spawns are required - see note at bottom).
-- ====================================================================================================================

-- --------------------------------------------------------------------------------------------------------------------
-- Unholy: "Apocalypse" chain (40930 -> 40935). Bind each quest to its QuestScript.
-- --------------------------------------------------------------------------------------------------------------------
UPDATE `quest_template_addon` SET `ScriptName`='quest_dk_apocalypse'           WHERE `ID`=40930;
UPDATE `quest_template_addon` SET `ScriptName`='quest_dk_following_the_curse'  WHERE `ID`=40931;
UPDATE `quest_template_addon` SET `ScriptName`='quest_dk_disturbing_the_past'  WHERE `ID`=40932;
UPDATE `quest_template_addon` SET `ScriptName`='quest_dk_a_grisly_task'        WHERE `ID`=40933;
UPDATE `quest_template_addon` SET `ScriptName`='quest_dk_the_dark_riders'      WHERE `ID`=40934;
UPDATE `quest_template_addon` SET `ScriptName`='quest_dk_call_of_vengeance'    WHERE `ID`=40935;

-- Ariden - final boss of 40934. Bound to BOTH the real hostile entry (102532) and the faction-35 duplicate (102200)
-- spawned at the same spot on map 1533; the script forces faction 16 on Reset() (no creature_template.faction edit).
UPDATE `creature_template` SET `ScriptName`='npc_dk_ariden' WHERE `entry` IN (102200,102532);

-- --------------------------------------------------------------------------------------------------------------------
-- Frost: "Blades of the Fallen Prince" - scenario 901 on map 1480.
-- Add the `scenarios` row so InstanceScenario 901 runs on map 1480 (difficulty 12, matching the Hunter artifact
-- scenarios 1068/1099), then bind the scenario-director AI to Highlord Darion Mograine (95193).
-- --------------------------------------------------------------------------------------------------------------------
DELETE FROM `scenarios` WHERE `map`=1480;
INSERT INTO `scenarios` (`map`,`difficulty`,`dungeonID`,`scenario_A`,`scenario_H`) VALUES (1480,12,0,901,901);

UPDATE `creature_template` SET `ScriptName`='npc_dk_blades_scenario_director' WHERE `entry`=95193;

-- --------------------------------------------------------------------------------------------------------------------
-- Creature spawns: NONE required. All needed actors are already spawned in integ_world:
--   Unholy - Ariden 102532 (f2102) + 102200 (f35) on map 1533; Revil Kost 100323/100812/101282; Duke Lankral 101441.
--   Frost  - Highlord Darion 95193, The Lich King 103996, Blades 100577, Scourge Teleporter 95416, fragments on 1480.
--   Blood  - NOT spawned and NOT wired (no quest exists); intentionally left for a future full quest authoring pass.
-- The assigned guid block is cleared for idempotency even though no rows are inserted.
-- --------------------------------------------------------------------------------------------------------------------
DELETE FROM `creature` WHERE `guid` BETWEEN 50043000 AND 50043199;

-- ===== Warlock =====
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
(50046020,109838,1630,0,0,'0',0,0,0,-1,0,0,-1038.6,1151.6,99.6,3.87,300,0,0,100,0,3,NULL,NULL,NULL,'',NULL,0),  -- Calydus, Tol Barad director + 43153 ender
(50046021,106757,1630,0,0,'0',0,0,0,-1,0,0,-1030.0,1145.0,99.6,3.87,300,0,0,100,0,NULL,NULL,NULL,'',NULL,0),   -- Eye of the Beast, scenario 1155 finale boss
(50046022,109698,1107,0,0,'0',0,0,0,-1,0,0,3118.0,1104.0,286.6,4.65,300,0,0,100,0,3,NULL,NULL,NULL,'',NULL,0);   -- Calydus, 43100 quest-ender at the Dreadscar
