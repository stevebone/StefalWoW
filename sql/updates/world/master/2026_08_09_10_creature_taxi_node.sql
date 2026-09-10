--
-- Covenant Transport Network flight points ("The Eternal Gateway")
-- ===============================================================
--
-- NEW TABLE `creature_taxi_node` (CreatureID, TaxiNodeID) - binds a flight master to a TaxiNodes.db2 row
-- explicitly instead of resolving it by proximity.
--
-- WHY THE TABLE IS NEEDED
-- -----------------------
-- The Kyrian sanctum transport network lives in TaxiNodes.db2 rows 2625-2637 (build 68275,
-- M:/WorldofWarcraft/dbc/enUS/TaxiNodes.db2). Every one of them carries Flags = 99, i.e.
--   ShowOnAllianceMap | ShowOnHordeMap | EndPointOnly | IgnoreForFindNearest
-- (TaxiNodeFlags in src/server/game/DataStores/DBCEnums.h). `IgnoreForFindNearest` makes
-- ObjectMgr::GetNearestTaxiNode skip them outright, so no flight master can ever resolve to one by
-- distance. Blizzard can afford that flag precisely because their flight master is bound to its node
-- explicitly - which is what this table restores.
--
-- Measured against the live world DB, the Eternal Gateway resolves today to the *legacy* Bastion flight
-- point 2528 "Elysian Hold, Bastion" 86.4 yd away, whose reachable set shares no TaxiPath with the
-- 2625-network - hence "the ordinary flight map with none of the sanctum destinations on it".
--
-- THE ONE SOURCED BINDING
-- -----------------------
--   creature_template 171037 "Eternal Gateway", npcflag 8192 (UNIT_NPC_FLAG_FLIGHTMASTER, and that is its
--   only flag). Exactly one `creature` row: guid 464327, map 2222, (-1714.37, -5790.77, 6823.57).
--   Nearest TaxiNodes row on map 2222: 2625 "Elysian Hold, Bastion" (-1714.82, -5786.21, 6823.57) at
--   4.58 yd. Second nearest is 77.58 yd away (node 2683, a quest path) - a ~17x margin.
--   2625 is also the only node of the network with outbound TaxiPath rows (8302 -> 2628, 8317 -> 2627),
--   i.e. the network's entry point. Control for the method: the same computation returns node 2 for
--   Dungar Longdrink (352, Stormwind) at 5.82 yd and node 23 for Doras (3310, Orgrimmar) at 7.07 yd.
--
-- THE OTHER THREE COVENANTS ARE DELIBERATELY NOT SEEDED
-- -----------------------------------------------------
-- Two independent reasons, both from the client data:
--   1. There is no transport network to bind to. Filtering TaxiNodes.db2 by
--      Flags & TaxiNodeFlags::IgnoreForFindNearest (64) on map 2222 returns exactly
--      2625, 2626, 2630, 2631, 2632, 2633, 2634, 2635, 2636, 2637 and 2682 - all of them in Bastion.
--      Revendreth, Maldraxxus and Ardenweald have no equivalent cluster anywhere in the DB2.
--   2. Their sanctum flight points do not need a binding. 2548 "Sinfall, Revendreth",
--      2398 "Bleak Redoubt, Maldraxxus" and 2587 "Heart of the Forest, Ardenweald" are ordinary nodes
--      (Flags = 3, no IgnoreForFindNearest), so their flight masters - 162702 Courier Snaggle (4.8 yd),
--      157514 Wing Guard Buurkin (13.3 yd), 165701 Ceridwyn (14.4 yd) - already resolve by proximity.
--      They are gated by TaxiNodes.ConditionID (84603 / 87386 / 87905), which the server change shipped
--      alongside this file now evaluates. Binding them here would be redundant, so it is not done.
--
-- NOT TOUCHED: node 2528 keeps its own flight master, creature 159421 "Cassius" at 3.8 yd, which still
-- resolves by proximity. Binding 171037 therefore does not strip the legacy Bastion flight points from
-- anybody - they simply move to the flight master that was always standing next to them.
--
-- NOTE: a missing table aborts worldserver startup for tables read through prepared statements. This one
-- is read with a plain WorldDatabase.Query in ObjectMgr::LoadCreatureTaxiNodes, so an unapplied update
-- degrades to "0 bindings loaded" rather than an ABORT - but it must still be applied for the fix to work.
--

CREATE TABLE IF NOT EXISTS `creature_taxi_node` (
  `CreatureID` int unsigned NOT NULL COMMENT 'creature_template.entry of the flight master',
  `TaxiNodeID` int unsigned NOT NULL COMMENT 'TaxiNodes.db2 ID this flight master serves',
  `Comment` varchar(255) NOT NULL DEFAULT '' COMMENT 'sourcing note',
  PRIMARY KEY (`CreatureID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci
  COMMENT='Explicit flight master -> TaxiNodes binding for nodes flagged IgnoreForFindNearest';

DELETE FROM `creature_taxi_node` WHERE `CreatureID` IN (171037);
INSERT INTO `creature_taxi_node` (`CreatureID`, `TaxiNodeID`, `Comment`) VALUES
(171037, 2625, 'Eternal Gateway -> Elysian Hold (Kyrian Transport Network entry node); sole spawn 4.58yd, 2nd nearest 77.58yd');
