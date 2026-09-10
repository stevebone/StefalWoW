ALTER TABLE `character_garrison`
  ADD COLUMN `missionsStartedToday` int unsigned NOT NULL DEFAULT '0' AFTER `type`,
  ADD COLUMN `lastMissionStartDay` int unsigned NOT NULL DEFAULT '0' AFTER `missionsStartedToday`;

CREATE TABLE IF NOT EXISTS `character_garrison_archived_missions` (
  `guid` bigint unsigned NOT NULL,
  `garrType` int unsigned NOT NULL DEFAULT '0',
  `missionRecID` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`, `garrType`, `missionRecID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
