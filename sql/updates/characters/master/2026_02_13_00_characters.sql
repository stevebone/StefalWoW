CREATE TABLE IF NOT EXISTS `character_garrison_missions` (
  `dbId` bigint unsigned NOT NULL DEFAULT '0',
  `guid` bigint unsigned NOT NULL DEFAULT '0',
  `missionRecID` int unsigned NOT NULL DEFAULT '0',
  `offerTime` bigint NOT NULL DEFAULT '0',
  `offerDuration` int NOT NULL DEFAULT '0',
  `startTime` bigint NOT NULL DEFAULT '0',
  `travelDuration` int NOT NULL DEFAULT '0',
  `missionDuration` int NOT NULL DEFAULT '0',
  `missionState` int NOT NULL DEFAULT '0',
  `successChance` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`dbId`),
  KEY `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
