--
-- SoulbindConduit.db2 hotfix table (covenant P2 conduit sub-system)
--
DROP TABLE IF EXISTS `soulbind_conduit`;
CREATE TABLE `soulbind_conduit` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `ConduitType` tinyint unsigned NOT NULL DEFAULT '0',
  `CovenantID` int NOT NULL DEFAULT '0',
  `SpecSetID` int NOT NULL DEFAULT '0',
  `Flags` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `soulbind_conduit_item`;
CREATE TABLE `soulbind_conduit_item` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `ItemID` int NOT NULL DEFAULT '0',
  `ConduitID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `soulbind_conduit_rank_properties`;
CREATE TABLE `soulbind_conduit_rank_properties` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `Rank` int NOT NULL DEFAULT '0',
  `ItemLevel` int NOT NULL DEFAULT '0',
  `QualityID` tinyint NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
