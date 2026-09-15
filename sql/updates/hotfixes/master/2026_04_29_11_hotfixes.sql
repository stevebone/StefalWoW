--
-- Table structure for table `garr_foll_support_spell`
--
DROP TABLE IF EXISTS `garr_foll_support_spell`;
CREATE TABLE `garr_foll_support_spell` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `HordeSpellID` int NOT NULL DEFAULT '0',
  `AllianceSpellID` int NOT NULL DEFAULT '0',
  `OrderIndex` tinyint unsigned NOT NULL DEFAULT '0',
  `GarrFollowerID` int unsigned NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
