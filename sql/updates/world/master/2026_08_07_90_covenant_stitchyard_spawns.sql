--
-- Abominable Stitchyard - creature spawns (Necrolord Abomination Factory, content half of P6.2).
--
-- WHY THIS FILE EXISTS
-- -------------------
-- `AbominationFactory.{h,cpp}` is complete and live, and `integ_world.creature_queststarter` already links the
-- Stitchyard NPCs to 42 authored quests (the campaign chain plus 31 dailies, all QuestSortID -593). But NOT ONE of
-- those NPCs is spawned, so the whole feature is unreachable in world: a Necrolord who researches GarrTalentTree 321
-- gets SkillLine 2787 and nothing to do with it. Spawning the questgivers is what switches the authored quests on.
--
-- WHERE THE COORDINATES COME FROM - AND WHY MOST NPCs ARE STILL MISSING
-- --------------------------------------------------------------------
-- Every offline source was worked before anything was written here. Only five rows survived the evidence bar; the
-- other eleven NPCs are deliberately NOT spawned rather than placed at invented positions.
--
--   TIER 1  live world DBs (integ_world, wc_world, wc_test_world, wowc_world, tc_world, playerbot_world, world):
--           ZERO `creature` rows for any of the sixteen entries.
--   TIER 1  filesystem dumps - ten world-DB dumps swept end to end (TDB_full_world_1200.26021 and 1115.25051 and
--           1120.25081, WCDB v1.2t/v1.3/v1.4, LoreWalkerTDB 11.2.7, WoWCommunity DB01/DB02 2026, plus the
--           AshamaneCore / LegionCore / FirestormWoD trees): ZERO spawn rows. The scan was validated with positive
--           controls (map 2222 creature rows do match, ~6.9k Shadowlands-range entry hits), so the negative is real.
--           All sixteen entries DO exist template-side everywhere - only the `creature` rows were never sniffed.
--   TIER 2  client DB2 (12.0.7.68275): publishes the place but never a position. Map 2222; AreaTable 12796
--           "The Stitchyard" is the unrelated open-world one in eastern Maldraxxus; the covenant feature sits in
--           "Seat of the Primus", UiMap 1698, whose UiMapAssignment gives the sanctum footprint on map 2222 as
--           X 1540..2010, Y -2902.5..-2197.5 (five WMO groups, doodad placement 26363831). Every position below
--           falls inside that box.
--   TIER 2  client quest-POI blobs already in `integ_world.quest_poi` / `quest_poi_points` (sniffed, VerifiedBuild
--           66384). Useful but sharp-edged: the point (1928, -2844, 3344) is an AREA blob shared by 27 different
--           QuestSortID -593 quests, so on its own it names no single NPC. Only points that are exclusive to one
--           questgiver were treated as that questgiver's position. Measured against every map-2222 questgiver that
--           IS spawned, an ObjectiveIndex -1 POI lands within 2 yards of the real spawn for 460/1427 quests and
--           within 15 yards for 770/1427 - good enough to confirm a position, not good enough to invent one.
--   TIER 3  sniffs in C:/sniff (87 binaries, ~2 GB, including three genuine 9.2.5/9.2.7 Shadowlands captures):
--           ZERO. The GUID scanner was validated against known entries (entry 143622 -> 406 hits on map 2222), so
--           the misses are real - those captures simply never entered the Necrolord Sanctum.
--   TIER 4  web (LABELLED PER ROW BELOW). Wowhead's map pins come from `g_mapperData` and are UI-map percentages on
--           UiMap 1536 (Maldraxxus), quantised to 0.2% = 8.4 yd in X by 12.6 yd in Y - too coarse on their own.
--           Two higher-precision values exist and are used below. UI% -> world uses the client's own
--           UiMapAssignment row for UiMap 1536 (Region 616.666,-5672.916 .. 4816.666,627.084):
--               worldX = 4816.666 - uiY * 4200      worldY = 627.084 - uiX * 6300
--
-- Z SANITY. The z used below is corroborated by real spawns already in `integ_world` on the same terraces:
--   z 3344 terrace  - Writhing Rachis at (1962.6, -2838.7, 3343.76), (1967.0, -2866.9, 3344.82), (1967.9, -2869.7, 3345.22)
--   z 3334 terrace  - Loyal Recruit at (1928.1, -2802.1, 3334.99), Mantaraganak at (1908.1, -2804.2, 3335.13)
--
-- ORIENTATION IS NOT SOURCED for any of these NPCs by anything, anywhere. It is set to 0 rather than guessed; it is
-- cosmetic only (facing), and none of these NPCs move.
--
-- NOT SPAWNED HERE - and why. Do not "fix" these by interpolating; get a Shadowlands-era sniff of the Stitchyard.
--   158300 Flytrap          - sources CONFLICT: its exclusive POI says (1922,-2860,3344), Wowhead's pin converts to
--                             (1960.7,-2787.5), 82 yd apart. One quest only. Unresolved.
--   159212 Toothpick        - POI (1976,-2839) and (1936,-2848) both carry Z = 0. No usable height.
--   158298 Naxx             - POI (1797,-2937) carries Z = 0 and is 178 yd from Wowhead's pin. Contradictory.
--   158301 Marz, 159199 Iron Phillip, 159214 Guillotine, 159226 Sabrina, 159238 Atticus, 159240 Gas Bag,
--   159241 Roseboil, 167877 Miru Soulblossom
--                           - only the shared 27-quest area blob (1928,-2844,3344). That is the yard, not the NPC.
--                             Wowhead has a pin for each, but at 0.2% quantisation they scatter 19-65 yd from the
--                             blob with no way to choose. Eleven NPCs, ~24 dailies, still dark.
--
-- WHAT THIS FILE TURNS ON: 18 of the 42 authored questgiver links -
--   Rathan 167150         60041 Build-A-Bomination, 60042 May I Take Your Order?, 60048 Stitching Time,
--                         60195 Build One More, 60230 More the Merrier, 61635 Troubled Souls, 61637 Unity,
--                         61638 Iron Solution  (+ 41 creature_questender rows)
--   Chordy 161270         58432 Something Old Something Used, 61509 Shinies of Bastion, 61510 A Bountiful Haul,
--                         61511 Things They Leave Behind
--   Mama Tomalin 161678   56470 Give A Dog A Bone, 58992 Pie Not?, 59043 Baker's Dozen  (+ 3 questender)
--   The Professor 159198  58515 Scrounging for Scrolls, 58525 The Two Sides of History, 62294 One Lich's Trash...
--
-- Idempotent: each spawn is removed by template id before being re-inserted, and guids are taken from MAX+1 the way
-- the sibling covenant spawn migration (2026_08_07_20) does, so no fixed range can collide.
--

DELETE FROM `creature` WHERE `id` IN (167150, 167042, 161270, 161678, 159198);

SET @CGUID := (SELECT IFNULL(MAX(`guid`), 0) + 1 FROM `creature`);

-- (1) Rathan (167150) "Abominable Stitchmaster" - gives the whole Abomination Factory campaign chain.
--     POSITION: client quest-POI (1928, -2844, 3344), the point carried by all six of his POI-bearing quests.
--     CORROBORATION (web, labelled): warcraft.wiki.gg/wiki/Rathan gives "Butchers Block, Maldraxxus; [55.1, 68.75]",
--     which through the UiMap 1536 assignment above converts to (1929.17, -2844.22) - 1.2 yd from the client point.
--     That independent agreement is what makes this Rathan's position rather than merely the yard's.
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `MovementType`) VALUES
(@CGUID + 0, 167150, 2222, 11462, 0, '0', 0, 0, 0, 1928, -2844, 3344, 0, 7200, 0, 0);

-- (2) Abominable Stitching Table (167042) - the crafting station the feature is built around.
--     POSITION x/y: WEB-SOURCED, and the only source there is. Wowhead comment on npc=167042 publishes
--     "TomTom coordinates: /way 54.94 68.81 Abominable Stitching Table"; through the UiMap 1536 assignment that is
--     (1926.65, -2834.14). Wowhead's own map pin (54.8 / 68.8) agrees to within its 0.2% bucket.
--     POSITION z: **ADOPTED, NOT SOURCED** - 3344, Rathan's client-POI height, 10.4 yd away on the same terrace and
--     matching the three Writhing Rachis spawns listed above. This is the one unsourced number in this file. If the
--     table renders sunk or floating, this is the value to correct.
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `MovementType`) VALUES
(@CGUID + 1, 167042, 2222, 11462, 0, '0', 0, 0, 0, 1926.65, -2834.14, 3344, 0, 7200, 0, 0);

-- (3) Chordy (161270) - the rank-1 construct, gives 4 dailies.
--     POSITION: client quest-POI (1929, -2795, 3334), carried by all four of Chordy's quests and by no other
--     questgiver's. Its z is confirmed independently by the Loyal Recruit spawns 7 yd away at z 3334.99.
--     NOTE: Wowhead's pin for Chordy converts to (1943.9, -2837.9), 45 yd away, and Wowhead lists two pins for him.
--     Client data is preferred over the web pin here; flagging the disagreement rather than hiding it.
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `MovementType`) VALUES
(@CGUID + 2, 161270, 2222, 11462, 0, '0', 0, 0, 0, 1929, -2795, 3334, 0, 7200, 0, 0);

-- (4) Mama Tomalin (161678) "Innkeeper" - gives 3 dailies, ends 3.
--     POSITION: client quest-POI (1931, -2856, 3344), exclusive to her three quests.
--     CORROBORATION (web, labelled): Wowhead pin 55.2 / 68.6 converts to (1935.5, -2850.5) - 7.1 yd, inside the
--     0.2% quantisation bucket.
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `MovementType`) VALUES
(@CGUID + 3, 161678, 2222, 11462, 0, '0', 0, 0, 0, 1931, -2856, 3344, 0, 7200, 0, 0);

-- (5) The Professor (159198) - gives 3 dailies.
--     POSITION: client quest-POI (1954, -2848, 3344), exclusive to him (quest 58525).
--     CORROBORATION (web, labelled): Wowhead pin 55.2 / 68.2 converts to (1952.3, -2850.5) - 3.1 yd. This is the
--     closest independent agreement of the whole set.
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `MovementType`) VALUES
(@CGUID + 4, 159198, 2222, 11462, 0, '0', 0, 0, 0, 1954, -2848, 3344, 0, 7200, 0, 0);
