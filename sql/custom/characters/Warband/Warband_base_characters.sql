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

DROP TABLE IF EXISTS `account_bank_tab_settings`;
-- Account bank tab settings (persists tab name/icon/description/flags per bnet account)
CREATE TABLE IF NOT EXISTS `account_bank_tab_settings` (
  `battlenetAccountId` int unsigned NOT NULL,
  `tabId` tinyint unsigned NOT NULL,
  `name` varchar(16) NOT NULL DEFAULT '',
  `icon` varchar(64) NOT NULL DEFAULT '',
  `description` varchar(2048) NOT NULL DEFAULT '',
  `depositFlags` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`battlenetAccountId`, `tabId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `account_bank_item`;
-- Account bank item positions (stores item positions in account bank)
CREATE TABLE IF NOT EXISTS `account_bank_item` (
  `battlenetAccountId` int unsigned NOT NULL,
  `bag` tinyint unsigned NOT NULL COMMENT 'tab index (0-4)',
  `slot` tinyint unsigned NOT NULL COMMENT 'slot within tab (0-97)',
  `item` bigint unsigned NOT NULL,
  PRIMARY KEY (`battlenetAccountId`, `bag`, `slot`),
  UNIQUE KEY `idx_item` (`item`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
