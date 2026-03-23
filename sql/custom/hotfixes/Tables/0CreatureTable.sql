--
-- Table structure for table `creature`
--

DROP TABLE IF EXISTS `creature`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `creature` (
  `ID` mediumint unsigned NOT NULL DEFAULT '0',
  `ItemID1` mediumint unsigned NOT NULL DEFAULT '0',
  `ItemID2` mediumint unsigned NOT NULL DEFAULT '0',
  `ItemID3` mediumint unsigned NOT NULL DEFAULT '0',
  `Mount` mediumint unsigned NOT NULL DEFAULT '0',
  `DisplayID1` mediumint unsigned NOT NULL DEFAULT '0',
  `DisplayID2` mediumint unsigned NOT NULL DEFAULT '0',
  `DisplayID3` mediumint unsigned NOT NULL DEFAULT '0',
  `DisplayID4` mediumint unsigned NOT NULL DEFAULT '0',
  `DisplayIDProbability1` float NOT NULL DEFAULT '0',
  `DisplayIDProbability2` float NOT NULL DEFAULT '0',
  `DisplayIDProbability3` float NOT NULL DEFAULT '0',
  `DisplayIDProbability4` float NOT NULL DEFAULT '0',
  `Name` mediumtext CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `FemaleName` mediumtext CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `SubName` mediumtext CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `FemaleSubName` mediumtext CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Type` mediumint unsigned NOT NULL DEFAULT '0',
  `Family` tinyint unsigned NOT NULL DEFAULT '0',
  `Classification` tinyint unsigned NOT NULL DEFAULT '0',
  `InhabitType` tinyint unsigned NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;