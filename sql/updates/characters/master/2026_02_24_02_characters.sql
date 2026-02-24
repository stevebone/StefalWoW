CREATE TABLE IF NOT EXISTS `character_garrison_talents` (
  `guid` bigint unsigned NOT NULL,
  `garrTalentId` int unsigned NOT NULL DEFAULT '0' COMMENT 'GarrTalent.ID',
  `rank` int NOT NULL DEFAULT '0',
  `researchStartTime` bigint NOT NULL DEFAULT '0',
  `flags` int NOT NULL DEFAULT '0',
  `soulbindConduitId` int NOT NULL DEFAULT '0',
  `soulbindConduitRank` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`, `garrTalentId`),
  KEY `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
