--
-- Covenant / Soulbind: per-character active covenant + soulbind
--
CREATE TABLE IF NOT EXISTS `character_covenant` (
  `guid` bigint unsigned NOT NULL DEFAULT '0',
  `covenantId` int unsigned NOT NULL DEFAULT '0',
  `soulbindId` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Player active covenant/soulbind';
