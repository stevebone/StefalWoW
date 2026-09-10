--
-- War-effort Contribution system DB2 hotfix tables (Contribution, CreatureXContribution,
-- ManagedWorldState, ManagedWorldStateInput). Field order matches the DB2LoadInfo / *Meta schemas.
--
DROP TABLE IF EXISTS `contribution`;
CREATE TABLE `contribution` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `Description` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Name` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `ManagedWorldStateInputID` int NOT NULL DEFAULT '0',
  `OrderIndex` int NOT NULL DEFAULT '0',
  `ContributionStyleContainer` int NOT NULL DEFAULT '0',
  `UiTextureAtlasMemberID1` int NOT NULL DEFAULT '0',
  `UiTextureAtlasMemberID2` int NOT NULL DEFAULT '0',
  `UiTextureAtlasMemberID3` int NOT NULL DEFAULT '0',
  `UiTextureAtlasMemberID4` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `contribution_locale`;
CREATE TABLE `contribution_locale` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `locale` varchar(4) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Description_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Name_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`locale`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `creature_x_contribution`;
CREATE TABLE `creature_x_contribution` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `ContributionID` int NOT NULL DEFAULT '0',
  `CreatureID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `managed_world_state`;
CREATE TABLE `managed_world_state` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `CurrentStageWorldStateID` int NOT NULL DEFAULT '0',
  `ProgressWorldStateID` int NOT NULL DEFAULT '0',
  `UpTimeSecs` int unsigned NOT NULL DEFAULT '0',
  `DownTimeSecs` int unsigned NOT NULL DEFAULT '0',
  `AccumulationStateTargetValue` int NOT NULL DEFAULT '0',
  `DepletionStateTargetValue` int NOT NULL DEFAULT '0',
  `AccumulationAmountPerMinute` int NOT NULL DEFAULT '0',
  `DepletionAmountPerMinute` int NOT NULL DEFAULT '0',
  `Field_8_1_5_29418_009` tinyint NOT NULL DEFAULT '0',
  `OccurrencesWorldStateID1` int NOT NULL DEFAULT '0',
  `OccurrencesWorldStateID2` int NOT NULL DEFAULT '0',
  `OccurrencesWorldStateID3` int NOT NULL DEFAULT '0',
  `OccurrencesWorldStateID4` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `managed_world_state_input`;
CREATE TABLE `managed_world_state_input` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `ManagedWorldStateID` int NOT NULL DEFAULT '0',
  `QuestID` int NOT NULL DEFAULT '0',
  `ValidInputConditionID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `managed_world_state_buff`;
CREATE TABLE `managed_world_state_buff` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `BuffSpellID` int NOT NULL DEFAULT '0',
  `PlayerConditionID` int unsigned NOT NULL DEFAULT '0',
  `OccurrenceValue` int unsigned NOT NULL DEFAULT '0',
  `ManagedWorldStateID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
