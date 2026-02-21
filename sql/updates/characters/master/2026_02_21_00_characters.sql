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

-- Account bank item positions (stores item positions in account bank)
CREATE TABLE IF NOT EXISTS `account_bank_item` (
  `battlenetAccountId` int unsigned NOT NULL,
  `bag` tinyint unsigned NOT NULL COMMENT 'tab index (0-4)',
  `slot` tinyint unsigned NOT NULL COMMENT 'slot within tab (0-97)',
  `item` bigint unsigned NOT NULL,
  PRIMARY KEY (`battlenetAccountId`, `bag`, `slot`),
  UNIQUE KEY `idx_item` (`item`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
