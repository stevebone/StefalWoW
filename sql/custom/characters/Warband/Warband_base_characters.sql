DROP TABLE IF EXISTS `character_warband_groups`;
CREATE TABLE IF NOT EXISTS `character_warband_groups` (
  `groupId` bigint unsigned NOT NULL AUTO_INCREMENT,
  `battlenetAccountId` int unsigned NOT NULL,
  `orderIndex` tinyint unsigned NOT NULL DEFAULT '0',
  `warbandSceneId` int unsigned NOT NULL DEFAULT '0',
  `flags` int unsigned NOT NULL DEFAULT '0',
  `contentSetId` int NOT NULL DEFAULT '0',
  `name` varchar(128) NOT NULL DEFAULT '',
  PRIMARY KEY (`groupId`),
  KEY `idx_account` (`battlenetAccountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `character_warband_groups_members`;
CREATE TABLE IF NOT EXISTS `character_warband_group_members` (
  `groupId` bigint unsigned NOT NULL,
  `memberIndex` tinyint unsigned NOT NULL,
  `guid` bigint unsigned NOT NULL DEFAULT '0',
  `warbandScenePlacementId` int unsigned NOT NULL DEFAULT '0',
  `memberType` int NOT NULL DEFAULT '0',
  `contentSetId` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`groupId`, `memberIndex`),
  KEY `idx_guid` (`guid`),
  CONSTRAINT `fk_warband_group_members_group` FOREIGN KEY (`groupId`) REFERENCES `character_warband_groups` (`groupId`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
