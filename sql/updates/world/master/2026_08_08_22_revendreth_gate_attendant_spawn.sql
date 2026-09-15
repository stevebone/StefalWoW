--
-- Ring of Transference: spawn the Revendreth gate attendant (Pathscribe Roh-Avonavi 175134).
--
-- 2026_08_08_20 wired the gossip -> taxi transport for all four Oribos zone gates, but 175134 had
-- NO spawn in any of the seven world databases on this machine and no sniffed position, so the
-- Revendreth option was scripted-but-unreachable. Wowhead publishes only "This NPC can be found in
-- Oribos" for 175134 - no coordinates, no mapper data - and the client ships no Oribos-side TaxiNode
-- for Revendreth (Bastion 2393 / Maldraxxus 2394 / Ardenweald 2396 each sit on their attendant;
-- Revendreth's path 7574 departs the shared Oribos node 2395), so there is no client coordinate to
-- read either.
--
-- The position below is therefore DERIVED FROM THE THREE EXISTING ATTENDANT SPAWNS, not sourced and
-- not guessed. They are exactly cocircular about (-1832.77, 1287.64) at R = 98.75, and each stands a
-- constant +0.0844 rad (+4.83 deg) around that circle from its own gate GO:
--     175133 Bastion    +0.0836 rad     gate 364419
--     175132 Maldraxxus +0.0826 rad     gate 364424
--     175131 Ardenweald +0.0870 rad     gate 364423
-- spread 0.0043 rad (0.25 deg). Leave-one-out check - refit R and the offset from the other two, then
-- predict the held-out attendant - reproduces all three to 0.12 / 0.26 / 0.38 yards.
-- Applying the same rule to the spawned Revendreth gate GO 364422 (-1834.27, 1396.27, 5455.15),
-- bearing +1.5846, gives (-1842.45, 1385.91). Z is the mean attendant floor height (5450.90; the three
-- sit at 5450.70-5451.01). Orientation faces the gate, as 175131 does.
--
-- If it renders off the walkway, this position is the thing to correct - everything else about the
-- Revendreth option (gossip, SmartAI, TaxiPath 7574) is already client-sourced and unaffected.
--
DELETE FROM `creature` WHERE `guid` = 50052013;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseUseFlags`, `PhaseId`, `PhaseGroup`, `terrainSwapMap`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curHealthPct`, `MovementType`, `npcflag`, `unit_flags`, `unit_flags2`, `unit_flags3`, `ScriptName`, `StringId`, `VerifiedBuild`) VALUES
(50052013, 175134, 2222, 10565, 13499, '0', 0, 0, 0, -1, 0, 0, -1842.45, 1385.91, 5450.9, 0.902, 180, 0, 0, 100, 0, NULL, NULL, NULL, NULL, '', NULL, 0);
