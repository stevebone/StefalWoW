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
