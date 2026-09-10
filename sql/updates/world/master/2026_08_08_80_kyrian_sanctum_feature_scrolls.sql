--
-- Kyrian sanctum (Elysian Hold) feature-activation scrolls - missing `gameobject` spawns.
--
-- REPORTED BUG
-- ------------
-- Quest 57899 "More Work?" ("Activate the Command Table.") never advances at the Eternal Watch.
--
-- WHAT THE OBJECTIVE ACTUALLY POINTS AT
-- -------------------------------------
-- `integ_world`.`quest_objectives` id 409051 (QuestID 57899, VerifiedBuild 66384):
--     Type 2 (QUEST_OBJECTIVE_GAMEOBJECT), ObjectID 364917, Amount 1, Description "Command Table activated".
-- `gameobject_template` 364917 (VerifiedBuild 43345) is "Elysian Scroll", type 10 GAMEOBJECT_TYPE_GOOBER,
-- displayId 58968, IconName 'questinteract', castBarCaption 'Activating'. Mapped against the goober union in
-- `GameObjectData.h` its fields are:
--     Data0  open                = 93     (Lock)
--     Data1  questID             = 0      -> the goober's own quest gate is DISABLED
--     Data10 spell               = 0
--     Data12 linkedTrap          = 0
--     Data14 openTextID          = 27700  (BroadcastText)
--     Data20 AllowMultiInteract  = 1
--     Data23 playerCast          = 1
--     everything else 0.
-- `GameObject::Use()` case GAMEOBJECT_TYPE_GOOBER (GameObject.cpp) skips the quest gate because goober.questID is 0
-- and then calls `player->KillCreditGO(info->entry, GetGUID())` unconditionally - i.e. using 364917 credits 364917,
-- which is exactly what objective 409051 wants. No script, no SmartAI, no C++ change is needed.
--
-- SO WHY DOESN'T IT WORK: 364917 HAS ZERO SPAWNS. There is nothing in the world to click.
-- Verified across every world DB on this box - integ_world, wowc_world, wc_world, wc_test_world, tc_world, world,
-- playerbot_world: `SELECT COUNT(*) FROM gameobject WHERE id=364917` = 0 in all seven. Positive control: the same
-- query for 352582 returns 4 in integ_world, so the method resolves rows that exist.
--
-- THE "COMMAND TABLE" GAMEOBJECT IS A RED HERRING
-- -----------------------------------------------
-- GO 352582 "Command Table" (type 5 GENERIC, 4 spawns) is NOT at the Eternal Watch. Its spawns are at
-- (2862.92, -1654.51, 3253.59) on map 2222; quest POIs at that coordinate resolve to UiMap 1536 = "Maldraxxus"
-- (UiMap.db2, client 12.0.7). The Eternal Watch is at UiMap 1707 = "Elysian Hold" (parent 1533 "Bastion"),
-- ~4500 yards away. 352582 must NOT be retyped to UI_LINK: it is a different table in a different zone, and doing
-- so would put a Kyrian mission UI on a Maldraxxus prop.
--
-- The thing a player actually clicks at the Eternal Watch is CREATURE 154527 "Command Table" (guid 611652 @
-- -1594.32, -5752.79, 6842.61), npcflag 137438953473 = GOSSIP | (1<<37) i.e. NPCFlags2 0x20
-- UNIT_NPC_FLAG_2_GARRISON_MISSION_NPC, gossip menu 25583. That NPC is already spawned and already opens the
-- Adventures UI - it is not, and never was, the quest objective. The scroll is.
--
-- WHERE THE POSITIONS COME FROM (client-derived, no invented values)
-- ------------------------------------------------------------------
-- `integ_world`.`quest_poi` / `quest_poi_points` are sniffed client quest-POI blobs (VerifiedBuild 66384). Each of
-- the four Kyrian sanctum feature quests has exactly three single-point blobs: Idx1 0 (giver), Idx1 1
-- (ObjectiveIndex 0, carrying QuestObjectiveID + QuestObjectID = the scroll), Idx1 2 (ObjectiveIndex 32, turn-in).
-- The Idx1 1 rows are the authority used below - MapID 2222, UiMapID 1707 "Elysian Hold" for all four:
--
--   QuestID  QuestObjectiveID  QuestObjectID  SpawnTrackingID   POI point (X, Y, Z)
--   57899    409051            364917         2038241           -1596, -5748, 6843
--   57901    409109            364934         2039439           -1753, -5700, 6826
--   60489    409118            364941         2039548           -1597, -5623, 6850
--   63052    409119            364942         2039573           -1715, -5787, 6825
--
-- ACCURACY CONTROL for single-point objective blobs on map 2222, measured against gameobjects that ARE spawned:
--   quest 63616 -> GO 368194: POI (4273, 5824, 4793) vs spawn (4273.48, 5825.87, 4792.46)  = 1.93 yd
--   quest 57689 -> GO 356397: five separate point blobs, each within 0.54 - 0.70 yd of its own spawn
-- So a single-point objective POI is the object's position to ~2 yards. That is the precision of the rows below.
--
-- INDEPENDENT CORROBORATION of each point against something already spawned in Elysian Hold:
--   57899 (-1596,-5748,6843) : creature 154527 "Command Table" guid 611652 @ (-1594.32,-5752.79,6842.61)  ~5.1 yd
--   57901 (-1753,-5700,6826) : GO 356590 "Anima Conductor" guid 186077 @ (-1758.82,-5693.86,6825.84)      ~8.4 yd
--   60489 (-1597,-5623,6850) : GO 353806 "Forge" guid 186086 @ (-1598.55,-5644.64,6853.09)               ~21.7 yd
--   63052 (-1715,-5787,6825) : GO 357942 "Mailbox" guid 186080 @ (-1694.37,-5792.69,6824.69)             ~21.6 yd
-- Each scroll lands on the terrace of the sanctum feature its quest unlocks, at that terrace's floor height.
--
-- ORIENTATION IS NOT SOURCED by anything in the workspace (no Shadowlands-era sniff of Elysian Hold exists in
-- C:/sniff, and GameObjects.db2 does not carry these entries - see below). It is set to 0 with the matching unit
-- quaternion (0,0,0,1) rather than guessed, following the policy already used in
-- 2026_08_07_90_covenant_stitchyard_spawns.sql. It is cosmetic only; the scroll does not move and Use() ignores it.
-- A non-unit quaternion would be silently replaced by ObjectMgr::LoadGameObjects with an sql.sql error, so it is
-- written out explicitly.
--
-- CLIENT GameObjects.db2 (M:/WorldofWarcraft/dbc/enUS, schema WOWSTATIC_12_0_7_67808, 31730 rows, single
-- non-sparse section, copy_table_count 0) CONTAINS NEITHER 352582 NOR 364917. That table only holds client-placed
-- static world objects; both of these entries reach us from sniffs instead, so unlike the Oribos covenant map
-- (357095) there is no client type/PropValue row to contradict `gameobject_template`. The reader was control-checked
-- against the same file (ids 352080/352435.../364239/364448... all resolve, and the id neighbourhood around both
-- misses is dense), and against AreaTable.db2 / UiMap.db2 / Lock.db2, so the negative is real and not a parse
-- failure. Consequently `gameobject_template`.`type` is left ALONE for both entries - 364917 is already type 10,
-- which is the type that grants the credit.
--
-- SPAWN TRACKING. `spawn_tracking_template` already has all four tracking ids (map 2222, PhaseId/PhaseGroup/
-- PhaseUseFlags 0, VerifiedBuild 60822) and `spawn_tracking_quest_objective` already links each one to its
-- objective (VerifiedBuild 54630) - both sniffed. What is missing is the `spawn_tracking` membership row, because
-- there was no spawn to bind. Those rows are added below so the scrolls appear only while the quest is being done,
-- as the client expects. The per-state `Visible` values are NOT client-sourced - no SpawnTracking DB2 ships in
-- M:/WorldofWarcraft/dbc/enUS - they follow the dominant convention of our own `spawn_tracking_state` table
-- (39 of 50 existing spawns use exactly 0:0, 1:1, 2:0; the other patterns are 0:0,1:1 (3) and 0:0,1:1,2:1 (8)):
--   State 0 None     -> hidden (quest not taken)
--   State 1 Active   -> visible (objective outstanding)  <- the state a player doing the quest is in
--   State 2 Complete -> hidden (objective done / quest rewarded)
-- Failure mode is safe: if ObjectMgr rejects any of these rows the spawn simply stays visible in every state.
--
-- LOCK 93 DOES NOT BLOCK THE INTERACT. GameObject::Use() has no lock check on the GOOBER path, and integ_world
-- already contains 159 distinct spawned type-10 goobers with Data0 = 93 that serve as Type-2 quest objectives
-- (e.g. 357346 "Soul Cage" 70 spawns, 339319 "Enchanted Cage" 29 spawns).
--
-- spawntimesecs 180 / animprogress 255 / state 1 match every other gameobject spawn in the Elysian Hold bounding
-- box (map 2222, x -1800..-1400, y -6000..-5600) except the Anima Conductor.
--
-- SCOPE. 57899 is the reported bug; 57901 "All That Remains" (Anima Conductor), 60489 "The Path of Ascension" and
-- 63052 "Step of Faith" (Transport Network) are the same defect on the same feature with the same class of
-- evidence, and are fixed here too.
--
-- Idempotent.
--

-- ---------------------------------------------------------------------------------------------------------------
-- Clean up (states first - they are keyed by gameobject guid)
-- ---------------------------------------------------------------------------------------------------------------
DELETE `sts` FROM `spawn_tracking_state` `sts`
  INNER JOIN `gameobject` `g` ON `g`.`guid` = `sts`.`SpawnId`
  WHERE `sts`.`SpawnType` = 1 AND `g`.`id` IN (364917, 364934, 364941, 364942);

DELETE FROM `spawn_tracking` WHERE `SpawnTrackingId` IN (2038241, 2039439, 2039548, 2039573);

DELETE FROM `gameobject` WHERE `id` IN (364917, 364934, 364941, 364942);

-- ---------------------------------------------------------------------------------------------------------------
-- Spawns
-- ---------------------------------------------------------------------------------------------------------------
SET @OGUID := (SELECT IFNULL(MAX(`guid`), 0) + 1 FROM `gameobject`);

INSERT INTO `gameobject`
  (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`,
   `terrainSwapMap`, `position_x`, `position_y`, `position_z`, `orientation`,
   `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`, `isActive`) VALUES
-- Elysian Scroll -> quest 57899 "More Work?"            objective 409051, Command Table terrace (next to Koros)
(@OGUID + 0, 364917, 2222, 0, 0, '0', 0, 0, 0, -1, -1596, -5748, 6843, 0, 0, 0, 0, 1, 180, 255, 1, 0),
-- Elysian Scroll -> quest 57901 "All That Remains"      objective 409109, Anima Conductor terrace
(@OGUID + 1, 364934, 2222, 0, 0, '0', 0, 0, 0, -1, -1753, -5700, 6826, 0, 0, 0, 0, 1, 180, 255, 1, 0),
-- Elysian Scroll -> quest 60489 "The Path of Ascension" objective 409118, Path of Ascension terrace
(@OGUID + 2, 364941, 2222, 0, 0, '0', 0, 0, 0, -1, -1597, -5623, 6850, 0, 0, 0, 0, 1, 180, 255, 1, 0),
-- Elysian Scroll -> quest 63052 "Step of Faith"         objective 409119, Transport Network terrace
(@OGUID + 3, 364942, 2222, 0, 0, '0', 0, 0, 0, -1, -1715, -5787, 6825, 0, 0, 0, 0, 1, 180, 255, 1, 0);

-- ---------------------------------------------------------------------------------------------------------------
-- Spawn tracking membership (SpawnType 1 = SPAWN_TYPE_GAMEOBJECT)
-- ---------------------------------------------------------------------------------------------------------------
INSERT INTO `spawn_tracking` (`SpawnTrackingId`, `SpawnType`, `SpawnId`, `QuestObjectiveIds`) VALUES
(2038241, 1, @OGUID + 0, '409051'),
(2039439, 1, @OGUID + 1, '409109'),
(2039548, 1, @OGUID + 2, '409118'),
(2039573, 1, @OGUID + 3, '409119');

INSERT INTO `spawn_tracking_state` (`SpawnType`, `SpawnId`, `State`, `Visible`) VALUES
(1, @OGUID + 0, 0, 0), (1, @OGUID + 0, 1, 1), (1, @OGUID + 0, 2, 0),
(1, @OGUID + 1, 0, 0), (1, @OGUID + 1, 1, 1), (1, @OGUID + 1, 2, 0),
(1, @OGUID + 2, 0, 0), (1, @OGUID + 2, 1, 1), (1, @OGUID + 2, 2, 0),
(1, @OGUID + 3, 0, 0), (1, @OGUID + 3, 1, 1), (1, @OGUID + 3, 2, 0);
