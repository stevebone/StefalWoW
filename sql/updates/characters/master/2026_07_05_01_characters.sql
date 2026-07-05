--
-- Soulbind conduit collection + socketed conduits (covenant P2 conduit sub-system)
--
DROP TABLE IF EXISTS `character_soulbind_conduits`;
CREATE TABLE `character_soulbind_conduits` (
  `guid` bigint unsigned NOT NULL DEFAULT '0',
  `conduitId` int unsigned NOT NULL DEFAULT '0',
  `rankIndex` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`conduitId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Player soulbind conduit collection (owned conduit -> rank)';

DROP TABLE IF EXISTS `character_soulbind_conduit_sockets`;
CREATE TABLE `character_soulbind_conduit_sockets` (
  `guid` bigint unsigned NOT NULL DEFAULT '0',
  `garrTalentId` int unsigned NOT NULL DEFAULT '0',
  `conduitId` int unsigned NOT NULL DEFAULT '0',
  `garrTalentTreeId` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`garrTalentId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Conduits socketed into soulbind tree nodes';
