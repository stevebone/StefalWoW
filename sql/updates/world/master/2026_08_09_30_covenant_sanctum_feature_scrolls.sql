--
-- Venthyr / Night Fae / Necrolord sanctum feature-activation scrolls
-- ==================================================================
--
-- This is the exact defect already fixed for Kyrian in 2026_08_08_80_kyrian_sanctum_feature_scrolls.sql,
-- enumerated for the other three covenants instead of being found by chance. Two mechanisms are in play:
--   (M3) the quest objective points at a gameobject that has ZERO spawns -> nothing to click;
--   (M1) five of those gameobjects are `type` 5 GAMEOBJECT_TYPE_GENERIC, which `GameObject::Use()` has no
--        case for, so even once spawned a click would do nothing.
--
-- HOW THE SET WAS ENUMERATED (not by chance)
-- ------------------------------------------
-- integ_world: every `quest_objectives` row of Type 2 (QUEST_OBJECTIVE_GAMEOBJECT) whose Description matches
-- the sanctum-feature wording, joined to `gameobject` spawn counts. The Kyrian four (364917/364934/364941/
-- 364942) now have 1 spawn each; every other covenant's equivalent has 0:
--
--   entry   covenant   template name                 type  quest(s)        objective(s)      tracking id
--   364950  Venthyr    Scroll of Dark Empowerment     10   63056, 63063    409136, 409153    2039827
--   364983  Venthyr    Scroll of Dark Empowerment     10   63064           409155            2039852
--   364984  Venthyr    Scroll of Dark Empowerment     10   63066           409158            2039855
--   364985  Venthyr    Scroll of Dark Empowerment      5   63065           409157            2039853
--   364971  Night Fae  Fae Scroll                     10   61552           409138            2039758
--   364972  Night Fae  Fae Scroll                      5   63073           409169            2040151
--   364973  Night Fae  Fae Scroll                     10   62624           409166            2039983
--   364974  Night Fae  Fae Scroll                      5   63067           409160            2039857
--   364975  Necrolord  (name empty)                   10   63054           409130            2039754
--   364976  Necrolord  (name empty)                   10   63055           409132            2039757
--   364979  Necrolord  Prime Scroll                    5   63057           409140            2039768
--   364980  Necrolord  Prime Scroll                    5   63058, 63061    409142, 409149    2039778
--
-- All 14 quests exist in `quest_template` (verified: 61552, 62624, 63054-63058, 63061, 63063-63067, 63073).
-- Control for the zero-spawn claim: the identical `SELECT COUNT(*) FROM gameobject WHERE id=?` returns 1 for
-- each of the four Kyrian scrolls and 4 for 352582, so the method resolves rows that exist.
--
-- WHY THE TYPE-5 ROWS MUST BECOME TYPE 10
-- ---------------------------------------
-- A Type-2 quest objective is credited only by `Player::KillCreditGO`, and in `GameObject::Use()` that is
-- reached from GAMEOBJECT_TYPE_GOOBER (and CHEST / GATHERING_NODE). There is NO `case GAMEOBJECT_TYPE_GENERIC`
-- in the Use() switch at all (GameObject.cpp, switch at `void GameObject::Use`), so a type-5 object is inert
-- by construction - this is the same mechanism that made the Oribos covenant map (357095) dead until it became
-- type 48 UI_LINK.
--
-- The five type-5 rows are not a different object: each shares its covenant sibling's `displayId`
-- (61306 Venthyr / 61418 Night Fae / 60154 Necrolord) and its quest family, and each carries the
-- signature of a partial capture - `VerifiedBuild` 41079, no `IconName`, no `castBarCaption`,
-- Data0 = 1 / Data1 = 1 (the GENERIC union's floatingTooltip / highlight) and every other Data 0.
-- Their type-10 siblings, captured at 38556 / 39427 / 40966 / 41323 / 43345 / 46597, all carry the identical
-- GOOBER field set: Data0 (open/Lock) 93, Data14 (openTextID) 27700, Data20 (AllowMultiInteract) 1,
-- Data23 (playerCast) 1, IconName 'questinteract'. The values written below are copied verbatim from the
-- SAME-COVENANT, SAME-displayId sibling - nothing is invented.
--
-- Client control for the type: GameObjects.db2 @12.0.7 (M:/WorldofWarcraft/dbc/enUS,
-- schema WOWSTATIC_12_0_7_67808, 31730 rows, all 31730 parsed, single section) contains NONE of these
-- 16 entries - the same negative the Kyrian file recorded for 364917/352582. The negative is real and not a
-- parse failure: the id neighbourhood is dense (364967, 364968, 364969, 364970 and 365077 all resolve).
-- So no client row contradicts the retype; the authority is the sibling parity plus the KillCreditGO path.
--
-- Lock 93 does not block the interact: `GameObject::Use()`'s GOOBER path has no lock check, and integ_world
-- already contains 159 distinct spawned type-10 goobers with Data0 = 93 serving as Type-2 objectives.
--
-- WHERE THE POSITIONS COME FROM (client-derived, nothing invented)
-- ----------------------------------------------------------------
-- `quest_poi` / `quest_poi_points` (sniffed client POI blobs, VerifiedBuild 66384). Every one of the twelve
-- has exactly ONE single-point objective blob carrying both QuestObjectiveID and QuestObjectID, MapID 2222 -
-- the same shape the Kyrian file used:
--
--   entry   POI (X, Y, Z)          UiMapID
--   364950  -1914,  7698, 4192     1699 Sinfall
--   364983  -1950,  7633, 4192     1699 Sinfall
--   364984  -1951,  7779, 4125     1700 Sinfall (lower)
--   364985  -1842,  7657, 4194     1699 Sinfall
--   364971  -7037,  1126, 5687     1565 Ardenweald
--   364972  -6807,  1013, 5694     1701 Heart of the Forest
--   364973  -6897,  1010, 5674     1702 Heart of the Forest
--   364974  -6895,  1041, 5673     1703 Heart of the Forest
--   364975   1792, -2477, 3394     1698 Seat of the Primus
--   364976   1849, -2615, 3394     1698 Seat of the Primus
--   364979   1837, -2548, 3384     1536 Maldraxxus
--   364980   1934, -2834, 3344     1536 Maldraxxus
-- (UiMap names read from UiMap.db2 @12.0.7; reader control: 1533 = 'Bastion', 1565 = 'Ardenweald',
--  1707 = 'Elysian Hold' - the last matches the value the Kyrian file derived independently.)
--
-- ACCURACY CONTROL, re-measured in this session against gameobjects that ARE spawned on map 2222:
--   quest 63616 -> GO 368194: POI (4273, 5824, 4793) vs spawn (4273.48, 5825.87, 4792.46) = 2.00 yd
--   quest 57689 -> GO 356397: five point blobs, each 0.58 / 0.64 / 0.72 / 0.73 yd from its OWN spawn
-- So a single-point objective POI is the object's position to ~2 yards.
--
-- INDEPENDENT CORROBORATION - nearest already-spawned object to each POI (measured, not assumed):
--   364950 -> creature 172649 Sinfall Surface Flyer          11.8 yd
--   364983 -> creature 175458 Sinfall Recruit                 6.3 yd  / GO 364867 Scouting Map      8.3 yd
--   364984 -> creature 161735 Kael'thas Sunstrider           15.0 yd  (lower Sinfall, z 4125)
--   364985 -> creature 161979 Theotar                         3.5 yd  (the Ember Court host)
--   364971 -> creature 164023 Watcher Vesperbloom             3.7 yd  / GO 358820 Scouting Map      5.3 yd
--   364972 -> GO 355851 Mushroom Ring                        14.0 yd
--   364973 -> GO 353524 9ARD_FaeMushroomCircle_B01            8.9 yd
--   364974 -> GO 328303 Anima Conductor                      10.8 yd
--   364975 -> creature 175136 Command Table                   3.6 yd  (the Necrolord command table)
--   364976 -> creature 162934 Hand of Vashj                   5.9 yd
--   364979 -> GO 190942 Death Gate                            9.0 yd  / creature 164667 Prime Shieldguard 6.8 yd
--   364980 -> creature 167042 Abominable Stitching Table      7.4 yd  (the Abomination Factory)
-- Every scroll lands on the terrace of the sanctum feature its quest unlocks. Three of them land on the exact
-- feature named in the objective text, which is the strongest possible corroboration short of a sniff.
--
-- ORIENTATION IS NOT SOURCED (no Shadowlands-era sniff of these sanctums exists in C:/sniff and
-- GameObjects.db2 does not carry the entries). It is written as 0 with the matching unit quaternion
-- (0, 0, 0, 1) rather than guessed - the policy already used by the Kyrian file and by
-- 2026_08_07_90_covenant_stitchyard_spawns.sql. It is cosmetic; Use() ignores it.
--
-- SPAWN TRACKING. All twelve `spawn_tracking_template` rows already exist (map 2222, VerifiedBuild 60822) and
-- every `spawn_tracking_quest_objective` link already exists (VerifiedBuild 54630). Only the `spawn_tracking`
-- membership row was missing, because there was no spawn to bind. Per-state `Visible` follows the same
-- convention the Kyrian file established (0 None -> hidden, 1 Active -> visible, 2 Complete -> hidden).
-- Re-measured this session over `spawn_tracking_state` SpawnType 1: 18 of the 29 tracked gameobject spawns use
-- exactly 0:0, 1:1, 2:0; the alternatives are 0:0,1:1,2:1 (8) and 0:0,1:1 (3). Failure mode is safe - if
-- ObjectMgr rejects any of these rows the scroll simply stays visible in every state.
--
-- PHASE - the one place this file deliberately diverges from the client data, and why.
-- Ten of the twelve `spawn_tracking_template` rows carry PhaseId 0. Two do not: 2039852 (Venthyr Command
-- Table, GO 364983) and 2039855 (Venthyr Anima Conductor, GO 364984) carry PhaseId 16938.
-- Phase 16938 is REAL - it is one of Phase.db2's 25,967 copy-table rows (base section holds only 37 ids;
-- expanding the copy table gives 26,004 ids, min 50 / max 29683). Control: the same expansion also resolves
-- 169 (a base row) and 25571 (an integ_hotfixes.`phase` row). An earlier pass that ignored the copy table
-- wrongly called it absent - noted here so nobody repeats it.
-- BUT nothing on this server can put a player INTO phase 16938: `phase_area` has 546 rows and none for 16938,
-- and `conditions` has no CONDITION_SOURCE_TYPE_PHASE (26) row for it. Control: phases 1001/10040/172/169 are
-- all used by hundreds of existing spawns; 16938 is used by zero creature and zero gameobject rows.
-- So the two possible choices are:
--   (a) spawn them with PhaseId 16938 - data-faithful, and INVISIBLE to every player, i.e. the reported bug
--       is not actually fixed; or
--   (b) spawn them with PhaseId 0 and omit their `spawn_tracking` membership - the scroll is visible and
--       clickable and credits its objective, at the cost of being visible outside the quest window.
-- (b) is chosen. Omitting the membership rows is deliberate: `ObjectMgr::LoadSpawnTrackings` requires
-- template phase == spawn phase and would otherwise drop them with an sql.sql error anyway, so this avoids a
-- spurious startup error while keeping the object functional. The missing phase source for 16938 is recorded
-- in SANCTUM_INERT_SWEEP_68275.md as a judgement item - do NOT invent a `phase_area` row for it.
--
-- COSMETIC: 364975 and 364976 have an EMPTY `name`, which the client shows in the object tooltip
-- (SMSG_QUERY_GAME_OBJECT_RESPONSE carries it). They are given 'Prime Scroll' - the name their same-covenant,
-- same-displayId (60154) siblings 364979 / 364980 already carry. Not invented, but flagged as cosmetic.
--
-- Idempotent. Does not touch anything the Kyrian file, the Command Table / Anima Conductor fix, the trophy
-- work or the Transport Network taxi work created.
--

-- ---------------------------------------------------------------------------------------------------------
-- 1. The five GENERIC rows become GOOBER, copying the same-covenant sibling's field set verbatim
-- ---------------------------------------------------------------------------------------------------------

-- Venthyr: sibling 364950 / 364983 / 364984 (displayId 61306)
UPDATE `gameobject_template` SET
  `type` = 10, `IconName` = 'questinteract', `castBarCaption` = 'Activating',
  `Data0` = 93, `Data1` = 0, `Data14` = 27700, `Data20` = 1, `Data23` = 1
WHERE `entry` = 364985;

-- Night Fae: sibling 364971 / 364973 (displayId 61418)
UPDATE `gameobject_template` SET
  `type` = 10, `IconName` = 'questinteract', `castBarCaption` = 'Activating',
  `Data0` = 93, `Data1` = 0, `Data14` = 27700, `Data20` = 1, `Data23` = 1
WHERE `entry` IN (364972, 364974);

-- Necrolord: sibling 364975 / 364976 (displayId 60154) - those two carry an EMPTY castBarCaption
UPDATE `gameobject_template` SET
  `type` = 10, `IconName` = 'questinteract', `castBarCaption` = '',
  `Data0` = 93, `Data1` = 0, `Data14` = 27700, `Data20` = 1, `Data23` = 1
WHERE `entry` IN (364979, 364980);

-- Necrolord: give the two nameless rows their siblings' name (cosmetic, tooltip only)
UPDATE `gameobject_template` SET `name` = 'Prime Scroll' WHERE `entry` IN (364975, 364976) AND `name` = '';

-- ---------------------------------------------------------------------------------------------------------
-- 2. Clean up (states first - they are keyed by gameobject guid)
-- ---------------------------------------------------------------------------------------------------------
DELETE `sts` FROM `spawn_tracking_state` `sts`
  INNER JOIN `gameobject` `g` ON `g`.`guid` = `sts`.`SpawnId`
  WHERE `sts`.`SpawnType` = 1
    AND `g`.`id` IN (364950, 364971, 364972, 364973, 364974, 364975, 364976, 364979, 364980, 364983, 364984, 364985);

DELETE FROM `spawn_tracking` WHERE `SpawnTrackingId` IN
  (2039754, 2039757, 2039758, 2039768, 2039778, 2039827, 2039852, 2039853, 2039855, 2039857, 2039983, 2040151);

DELETE FROM `gameobject` WHERE `id` IN
  (364950, 364971, 364972, 364973, 364974, 364975, 364976, 364979, 364980, 364983, 364984, 364985);

-- ---------------------------------------------------------------------------------------------------------
-- 3. Spawns
-- ---------------------------------------------------------------------------------------------------------
SET @OGUID := (SELECT IFNULL(MAX(`guid`), 0) + 1 FROM `gameobject`);

INSERT INTO `gameobject`
  (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`,
   `terrainSwapMap`, `position_x`, `position_y`, `position_z`, `orientation`,
   `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`, `isActive`) VALUES
-- Venthyr / Sinfall
(@OGUID + 0,  364950, 2222, 0, 0, '0', 0, 0, 0, -1, -1914,  7698, 4192, 0, 0, 0, 0, 1, 180, 255, 1, 0), -- 63056/63063 Mirror Network
(@OGUID + 1,  364983, 2222, 0, 0, '0', 0, 0, 0, -1, -1950,  7633, 4192, 0, 0, 0, 0, 1, 180, 255, 1, 0), -- 63064 Command Table
(@OGUID + 2,  364984, 2222, 0, 0, '0', 0, 0, 0, -1, -1951,  7779, 4125, 0, 0, 0, 0, 1, 180, 255, 1, 0), -- 63066 Anima Conductor
(@OGUID + 3,  364985, 2222, 0, 0, '0', 0, 0, 0, -1, -1842,  7657, 4194, 0, 0, 0, 0, 1, 180, 255, 1, 0), -- 63065 The Ember Court
-- Night Fae / Heart of the Forest
(@OGUID + 4,  364971, 2222, 0, 0, '0', 0, 0, 0, -1, -7037,  1126, 5687, 0, 0, 0, 0, 1, 180, 255, 1, 0), -- 61552 Adventures
(@OGUID + 5,  364972, 2222, 0, 0, '0', 0, 0, 0, -1, -6807,  1013, 5694, 0, 0, 0, 0, 1, 180, 255, 1, 0), -- 63073 Mycelial Network
(@OGUID + 6,  364973, 2222, 0, 0, '0', 0, 0, 0, -1, -6897,  1010, 5674, 0, 0, 0, 0, 1, 180, 255, 1, 0), -- 62624 Queen's Conservatory
(@OGUID + 7,  364974, 2222, 0, 0, '0', 0, 0, 0, -1, -6895,  1041, 5673, 0, 0, 0, 0, 1, 180, 255, 1, 0), -- 63067 Anima Conductor
-- Necrolord / Seat of the Primus
(@OGUID + 8,  364975, 2222, 0, 0, '0', 0, 0, 0, -1,  1792, -2477, 3394, 0, 0, 0, 0, 1, 180, 255, 1, 0), -- 63054 Command Table
(@OGUID + 9,  364976, 2222, 0, 0, '0', 0, 0, 0, -1,  1849, -2615, 3394, 0, 0, 0, 0, 1, 180, 255, 1, 0), -- 63055 Transport Network
(@OGUID + 10, 364979, 2222, 0, 0, '0', 0, 0, 0, -1,  1837, -2548, 3384, 0, 0, 0, 0, 1, 180, 255, 1, 0), -- 63057 Anima Conductor
(@OGUID + 11, 364980, 2222, 0, 0, '0', 0, 0, 0, -1,  1934, -2834, 3344, 0, 0, 0, 0, 1, 180, 255, 1, 0); -- 63058/63061 Abomination Factory

-- ---------------------------------------------------------------------------------------------------------
-- 4. Spawn tracking membership (SpawnType 1 = SPAWN_TYPE_GAMEOBJECT). QuestObjectiveIds is comma-tokenised
--    by ObjectMgr::LoadSpawnTrackings, so the two shared scrolls list both of their objectives.
-- ---------------------------------------------------------------------------------------------------------
--    2039852 (@OGUID+1 / GO 364983) and 2039855 (@OGUID+2 / GO 364984) are intentionally ABSENT - see the
--    PHASE note above. Their scrolls are spawned unphased and simply stay visible.
INSERT INTO `spawn_tracking` (`SpawnTrackingId`, `SpawnType`, `SpawnId`, `QuestObjectiveIds`) VALUES
(2039827, 1, @OGUID + 0,  '409136,409153'),
(2039853, 1, @OGUID + 3,  '409157'),
(2039758, 1, @OGUID + 4,  '409138'),
(2040151, 1, @OGUID + 5,  '409169'),
(2039983, 1, @OGUID + 6,  '409166'),
(2039857, 1, @OGUID + 7,  '409160'),
(2039754, 1, @OGUID + 8,  '409130'),
(2039757, 1, @OGUID + 9,  '409132'),
(2039768, 1, @OGUID + 10, '409140'),
(2039778, 1, @OGUID + 11, '409142,409149');

INSERT INTO `spawn_tracking_state` (`SpawnType`, `SpawnId`, `State`, `Visible`) VALUES
(1, @OGUID + 0,  0, 0), (1, @OGUID + 0,  1, 1), (1, @OGUID + 0,  2, 0),
(1, @OGUID + 3,  0, 0), (1, @OGUID + 3,  1, 1), (1, @OGUID + 3,  2, 0),
(1, @OGUID + 4,  0, 0), (1, @OGUID + 4,  1, 1), (1, @OGUID + 4,  2, 0),
(1, @OGUID + 5,  0, 0), (1, @OGUID + 5,  1, 1), (1, @OGUID + 5,  2, 0),
(1, @OGUID + 6,  0, 0), (1, @OGUID + 6,  1, 1), (1, @OGUID + 6,  2, 0),
(1, @OGUID + 7,  0, 0), (1, @OGUID + 7,  1, 1), (1, @OGUID + 7,  2, 0),
(1, @OGUID + 8,  0, 0), (1, @OGUID + 8,  1, 1), (1, @OGUID + 8,  2, 0),
(1, @OGUID + 9,  0, 0), (1, @OGUID + 9,  1, 1), (1, @OGUID + 9,  2, 0),
(1, @OGUID + 10, 0, 0), (1, @OGUID + 10, 1, 1), (1, @OGUID + 10, 2, 0),
(1, @OGUID + 11, 0, 0), (1, @OGUID + 11, 1, 1), (1, @OGUID + 11, 2, 0);
