CREATE TABLE IF NOT EXISTS `warband_reputation` (
  `battlenetAccountId` int unsigned NOT NULL,
  `faction` smallint unsigned NOT NULL,
  `standing` int NOT NULL DEFAULT '0',
  `renownLevel` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`battlenetAccountId`, `faction`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

--
-- Phase 10C - renown reward grant tracking tables
--
-- Per-character grants: rewards bound to the character that earned them
-- (typically RR rows with Flags & 0x8 AccountUnlock == 0, e.g. Crafter's
-- Knowledge, character-bound spells).
CREATE TABLE IF NOT EXISTS `character_renown_rewards_granted` (
  `characterId`    bigint unsigned NOT NULL,
  `renownRewardId` int unsigned    NOT NULL,
  PRIMARY KEY (`characterId`, `renownRewardId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Per-Battle.net-account grants: rewards collected once per account
-- (Flags & 0x8 AccountUnlock set; mounts, pets, toys, transmog
-- appearances always go through the per-bnet collection path).
CREATE TABLE IF NOT EXISTS `warband_renown_rewards_granted` (
  `battlenetAccountId` int unsigned NOT NULL,
  `renownRewardId`     int unsigned NOT NULL,
  PRIMARY KEY (`battlenetAccountId`, `renownRewardId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

