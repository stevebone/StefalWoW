--
-- Covenant Callings board (P4.4).
--
-- One row per calling slot. A covenant board holds exactly three slots
-- (CovenantCallingsConstants.Callings.MaxCallings = 3 in the 12.0.7.68275 client); a slot either offers a
-- Bounty.db2 row until expireTime, or is empty and refills at refillTime. Both timestamps are daily-reset
-- boundaries, which is what lets the board roll over exactly at reset and be reconstructed after a restart
-- without any scheduler state.
--
-- Keyed by covenantId as well as guid: the per-covenant currency families (renown 1829-1832, reservoir anima
-- 1859-1862, redeemed souls 1863-1866) are direct evidence that Blizzard keeps all four covenant tracks for a
-- character simultaneously, so a covenant switch must not discard the other covenant's board.
CREATE TABLE IF NOT EXISTS `character_covenant_callings` (
  `guid` BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Character GUID',
  `covenantId` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Covenant.db2 ID this board belongs to',
  `slot` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0-based slot on the board',
  `bountyId` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Bounty.db2 ID currently offered, 0 when the slot is empty',
  `expireTime` BIGINT NOT NULL DEFAULT 0 COMMENT 'Unix time the offer lapses (occupied slots only)',
  `refillTime` BIGINT NOT NULL DEFAULT 0 COMMENT 'Unix time a new calling is issued (empty slots only)',
  PRIMARY KEY (`guid`,`covenantId`,`slot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Covenant Callings - per-character daily bounty board';
