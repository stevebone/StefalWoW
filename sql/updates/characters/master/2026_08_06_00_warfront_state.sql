-- BfA Warfronts (P0): coarse per-zone cycle state owned by WarfrontMgr. The contribution *bar counter* persists
-- separately through ManagedWorldStateMgr -> world_state; this table only holds the cycle spine (which phase, who
-- controls the zone, and the current phase-end timestamp) so the cycle survives a restart. See WARFRONTS_DESIGN.md §1.3.
CREATE TABLE IF NOT EXISTS `warfront_state` (
  `WarfrontId`   TINYINT UNSIGNED NOT NULL,                -- our WarfrontId enum (1=Stromgarde, 2=Darkshore)
  `State`        TINYINT UNSIGNED NOT NULL DEFAULT 0,      -- WarfrontState enum (0=CONTRIBUTION,1=SIEGE,2=FLIP)
  `Controlling`  TINYINT UNSIGNED NOT NULL DEFAULT 0,      -- TeamId of the controlling faction (0=Alliance,1=Horde)
  `PhaseEndTime` INT UNSIGNED     NOT NULL DEFAULT 0,      -- unix time the current phase ends (0 = no active timer)
  PRIMARY KEY (`WarfrontId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='BfA Warfront cycle state (WarfrontMgr)';
