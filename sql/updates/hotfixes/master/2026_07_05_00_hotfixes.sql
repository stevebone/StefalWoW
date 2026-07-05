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

DROP TABLE IF EXISTS `renown_rewards`;
CREATE TABLE `renown_rewards` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `Name` text,
  `Description` text,
  `ToastDescription` text,
  `CovenantID` int NOT NULL DEFAULT '0',
  `Level` int NOT NULL DEFAULT '0',
  `Icon` int NOT NULL DEFAULT '0',
  `Flags` int NOT NULL DEFAULT '0',
  `UiOrder` int NOT NULL DEFAULT '0',
  `ItemID` int NOT NULL DEFAULT '0',
  `SpellID` int NOT NULL DEFAULT '0',
  `MountID` int NOT NULL DEFAULT '0',
  `TransmogID` int NOT NULL DEFAULT '0',
  `TransmogSetID` int NOT NULL DEFAULT '0',
  `CharTitlesID` int NOT NULL DEFAULT '0',
  `GarrFollowerID` int NOT NULL DEFAULT '0',
  `TransmogIllusionID` int NOT NULL DEFAULT '0',
  `Field_12_0_0_63534_016` int NOT NULL DEFAULT '0',
  `QuestID` int NOT NULL DEFAULT '0',
  `PlayerConditionID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
