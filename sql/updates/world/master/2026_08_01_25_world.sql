--
-- Titanstrike: run the authored scenario 1068 "Thunder of the Titans" on Shield's Rest (map 1495, a type-5 scenario
-- instance at difficulty 12). Linking the map to the scenario makes MapManager auto-create the InstanceScenario on
-- entry, so its 6 steps drive the flow (intro -> tomb -> assist Prustaga -> search -> Volund fight -> teleport pad).
--
DELETE FROM `scenarios` WHERE `map`=1495 AND `difficulty`=12;
INSERT INTO `scenarios` (`map`,`difficulty`,`dungeonID`,`scenario_A`,`scenario_H`) VALUES (1495,12,0,1068,1068);
