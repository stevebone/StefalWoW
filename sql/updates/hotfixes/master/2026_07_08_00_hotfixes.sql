--
-- Bounty.db2 hotfix table (covenant P4 callings)
--
DROP TABLE IF EXISTS `bounty`;
CREATE TABLE `bounty` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `QuestID` int NOT NULL DEFAULT '0',
  `FactionID` smallint unsigned NOT NULL DEFAULT '0',
  `IconFileDataID` int unsigned NOT NULL DEFAULT '0',
  `TurninPlayerConditionID` int unsigned NOT NULL DEFAULT '0',
  `BountySetID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

--
-- BountySet.db2 hotfix table (covenant P4 callings)
--
DROP TABLE IF EXISTS `bounty_set`;
CREATE TABLE `bounty_set` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `VisiblePlayerConditionID` int unsigned NOT NULL DEFAULT '0',
  `LockedQuestID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
