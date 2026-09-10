CREATE TABLE IF NOT EXISTS `character_garrison_trophies` (
  `guid` bigint unsigned NOT NULL,
  `trophyId` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`, `trophyId`),
  KEY `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
