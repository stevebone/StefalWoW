-- BfA Warfronts P3/P4: bind the battle-map scenarios, instance controllers and final-boss AI.
--
-- Battle maps (Map.db2 InstanceType 5 = scenario): 1876/1943 (Arathi), 2105/2111 (Darkshore). They resolve to
-- difficulty 147 (Warfront Normal) / 149 (Warfront Heroic). Each map is single-faction, so scenario_A/scenario_H
-- both point at that map's faction scenario.

-- ---------------------------------------------------------------------------------------------------------------
-- Scenario binding: (map, difficulty, dungeonID, scenario_A, scenario_H).
--   Stromgarde: 1568 (Alliance, view 1568) / 1346 (Horde) - 3 steps, reward quests 52212 / 51342.
--   Darkshore : 1697 (Alliance) / 1701 (Horde) - 11 steps, reward quests 53800 / 53801.
-- ---------------------------------------------------------------------------------------------------------------
DELETE FROM `scenarios` WHERE `map` IN (1876,1943,2105,2111);
INSERT INTO `scenarios` (`map`,`difficulty`,`dungeonID`,`scenario_A`,`scenario_H`) VALUES
(1943,147,0,1568,1568),  -- Battle for Stromgarde (Alliance assault), Normal
(1876,147,0,1346,1346),  -- Battle for Stromgarde (Horde assault),    Normal
(2105,147,0,1697,1697),  -- Battle for Darkshore  (Alliance assault), Normal
(2111,147,0,1701,1701),  -- Battle for Darkshore  (Horde assault),    Normal
(1943,149,0,1568,1568),  -- Heroic mirrors (same scenario chain; boss tuning is P6)
(1876,149,0,1346,1346),
(2105,149,0,1697,1697),
(2111,149,0,1701,1701);

-- ---------------------------------------------------------------------------------------------------------------
-- Instance controller binding: instance_template.script must match the C++ InstanceMapScript names registered in
-- scripts/Warfronts/instance_warfront_arathi.cpp / instance_warfront_darkshore.cpp (one unique name per map id).
-- ---------------------------------------------------------------------------------------------------------------
DELETE FROM `instance_template` WHERE `map` IN (1876,1943,2105,2111);
INSERT INTO `instance_template` (`map`,`parent`,`script`,`insideResurrection`) VALUES
(1876,0,'instance_warfront_arathi_horde',0),
(1943,0,'instance_warfront_arathi_alliance',0),
(2105,0,'instance_warfront_darkshore_alliance',0),
(2111,0,'instance_warfront_darkshore_horde',0);

-- ---------------------------------------------------------------------------------------------------------------
-- Final-boss AI (npc_warfront_final_boss, scripts/Warfronts/warfront_bosses.cpp). The boss death completes the
-- scenario's final step and calls WarfrontMgr::OnScenarioComplete, which flips zone control and spawns the world
-- boss. Applied by template so both instance-mustered and ambient (Darkshore) spawns share the win-condition hook.
--   82877  High Warlord Volrath   (Stromgarde)
--   149098 Maiev Shadowsong       (Darkshore Horde path)
--   146628 Sira Moonwarden        (Darkshore Alliance path)
-- ---------------------------------------------------------------------------------------------------------------
UPDATE `creature_template` SET `ScriptName`='npc_warfront_final_boss' WHERE `entry` IN (82877,149098,146628);
