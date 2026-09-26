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
  