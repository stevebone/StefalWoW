-- Warband account tables (auth database).
-- Ported from RolePlay_master: group ids are scoped to the game account - the client
-- picks new ids counting from the highest id it was sent. realmId is provenance only
-- (the realm that created the row); groups are region-wide and shared by all realms.

DROP TABLE IF EXISTS `account_warband_group_members`;
DROP TABLE IF EXISTS `account_warband_groups`;

CREATE TABLE `account_warband_groups` (
  `id` bigint(20) unsigned NOT NULL,
  `accountId` int(10) unsigned NOT NULL,
  `realmId` int(10) unsigned NOT NULL DEFAULT '1',
  `orderIndex` tinyint(3) unsigned NOT NULL,
  `name` varchar(257) NOT NULL,
  `warbandSceneId` int(10) unsigned NOT NULL DEFAULT '0',
  `flags` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`accountId`, `id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `account_warband_group_members` (
  `accountId` int(10) unsigned NOT NULL,
  `realmId` int(10) unsigned NOT NULL DEFAULT '1',
  `groupId` bigint(20) unsigned NOT NULL,
  `characterGuid` bigint(20) unsigned NOT NULL,
  `placementId` int(10) unsigned NOT NULL,
  `type` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`accountId`, `groupId`, `characterGuid`),
  CONSTRAINT `fk_warband_group` FOREIGN KEY (`accountId`, `groupId`)
    REFERENCES `account_warband_groups` (`accountId`, `id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ---------------------------------------------------------------------------
-- Multi-realm character list (CharacterSelect.ExtraRealms)
-- ---------------------------------------------------------------------------

-- Global account cache types live here so every realm of the account sees the
-- same values; the characters db still holds per-character types.
CREATE TABLE IF NOT EXISTS `account_data_global` (
  `accountId` int unsigned NOT NULL,
  `type` tinyint unsigned NOT NULL DEFAULT 0,
  `time` bigint NOT NULL DEFAULT 0,
  `data` longblob NULL,
  PRIMARY KEY (`accountId`, `type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- One-shot realm-transfer handoff: the realm redirecting a login writes the row,
-- the home realm consumes it in HandleAuthContinuedSessionCallback.
CREATE TABLE IF NOT EXISTS `account_realm_transfer` (
  `accountId` int unsigned NOT NULL,
  `connectKey` int unsigned NOT NULL,
  `characterGuid` bigint unsigned NOT NULL,
  `createTime` bigint NOT NULL,
  PRIMARY KEY (`accountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- realm -> characters schema registry; worldserver registers itself when
-- CharacterSelect.ExtraRealms = "auto".
CREATE TABLE IF NOT EXISTS `realm_character_schemas` (
  `realmId` int unsigned NOT NULL,
  `schemaName` varchar(64) NOT NULL,
  PRIMARY KEY (`realmId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- preserve existing client UI settings / macros when account data moves to auth
INSERT IGNORE INTO `account_data_global` (`accountId`, `type`, `time`, `data`)
  SELECT `accountId`, `type`, `time`, `data` FROM `characters`.`account_data`;

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

-- drop the obsolete per-realm bank tables (edit schema name if the characters
-- database is not named `characters`; repeat for each sibling characters schema)
DROP TABLE IF EXISTS `characters`.`account_bank_item`;
DROP TABLE IF EXISTS `characters`.`account_bank_tab_settings`;
DROP TABLE IF EXISTS `characters`.`account_bank_coinage`;
