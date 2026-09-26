-- ---------------------------------------------------------------------------
-- Warband bank (battle.net account-wide)
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS `account_bank_tab_settings` (
  `battlenetAccountId` int unsigned NOT NULL,
  `tabId` tinyint unsigned NOT NULL,
  `name` varchar(16) NOT NULL DEFAULT '',
  `icon` varchar(64) NOT NULL DEFAULT '',
  `description` varchar(2048) NOT NULL DEFAULT '',
  `depositFlags` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`battlenetAccountId`, `tabId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `account_bank_item` (
  `battlenetAccountId` int unsigned NOT NULL,
  `bag` tinyint unsigned NOT NULL COMMENT 'tab index (0-4)',
  `slot` tinyint unsigned NOT NULL COMMENT 'slot within tab (0-97)',
  `item` bigint unsigned NOT NULL,
  `sourceRealm` int unsigned NOT NULL COMMENT 'realm whose characters database holds the item_instance row',
  PRIMARY KEY (`battlenetAccountId`, `bag`, `slot`),
  -- NOT unique: sibling realms issue the same numeric item guids
  KEY `idx_item` (`item`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `account_bank_coinage` (
  `battlenetAccountId` int unsigned NOT NULL,
  `coinage` bigint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`battlenetAccountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- migrate existing per-realm bank data before dropping the old tables - the item
-- rows stay in this realm's characters database, so sourceRealm = this realm's id
-- (edit the schema name / realm id if this is not `characters` / realm 1)
INSERT INTO `account_bank_tab_settings` (battlenetAccountId, tabId, name, icon, description, depositFlags)
  SELECT battlenetAccountId, tabId, name, icon, description, depositFlags
  FROM `characters`.`account_bank_tab_settings`;

INSERT INTO `account_bank_item` (battlenetAccountId, bag, slot, item, sourceRealm)
  SELECT battlenetAccountId, bag, slot, item, 1
  FROM `characters`.`account_bank_item`;

INSERT INTO `account_bank_coinage` (battlenetAccountId, coinage)
  SELECT battlenetAccountId, coinage
  FROM `characters`.`account_bank_coinage`;

-- drop the obsolete per-realm bank tables (edit schema name if the characters
-- database is not named `characters`; repeat for each sibling characters schema)
DROP TABLE IF EXISTS `characters`.`account_bank_item`;
DROP TABLE IF EXISTS `characters`.`account_bank_tab_settings`;
DROP TABLE IF EXISTS `characters`.`account_bank_coinage`;
