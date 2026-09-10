--
-- Garrison DB2 hotfix tables (char_shipment + garr_*). The garrison reconstruction added
-- the DB2 loaders + prepared statements (HotfixDatabase.cpp) but shipped no table SQL.
--

DROP TABLE IF EXISTS `char_shipment`;
CREATE TABLE `char_shipment` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `ContainerID` smallint unsigned NOT NULL DEFAULT '0',
  `TreasureID` int unsigned NOT NULL DEFAULT '0',
  `DummyItemID` int NOT NULL DEFAULT '0',
  `Duration` int unsigned NOT NULL DEFAULT '0',
  `SpellID` int NOT NULL DEFAULT '0',
  `OnCompleteSpellID` int NOT NULL DEFAULT '0',
  `MaxShipments` tinyint unsigned NOT NULL DEFAULT '0',
  `GarrFollowerID` smallint unsigned NOT NULL DEFAULT '0',
  `Flags` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0'
,  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_ability_category`;
CREATE TABLE `garr_ability_category` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `Name` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` int NOT NULL DEFAULT '0'
,  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_encounter_set_x_encounter`;
CREATE TABLE `garr_encounter_set_x_encounter` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `GarrEncounterID` int unsigned NOT NULL DEFAULT '0',
  `GarrEncounterSetID` int unsigned NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0'
,  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_encounter_x_mechanic`;
CREATE TABLE `garr_encounter_x_mechanic` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `GarrMechanicID` int NOT NULL DEFAULT '0',
  `GarrMechanicSetID` tinyint unsigned NOT NULL DEFAULT '0',
  `GarrEncounterID` int unsigned NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0'
,  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_follower_set_x_follower`;
CREATE TABLE `garr_follower_set_x_follower` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `GarrFollowerID` int NOT NULL DEFAULT '0',
  `GarrFollowerSetID` int unsigned NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0'
,  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_follower_type`;
CREATE TABLE `garr_follower_type` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `GarrTypeID` tinyint NOT NULL DEFAULT '0',
  `MaxFollowers` tinyint unsigned NOT NULL DEFAULT '0',
  `MaxFollowerBuildingType` tinyint unsigned NOT NULL DEFAULT '0',
  `MaxItemLevel` smallint unsigned NOT NULL DEFAULT '0',
  `LevelRangeBias` tinyint unsigned NOT NULL DEFAULT '0',
  `ItemLevelRangeBias` tinyint unsigned NOT NULL DEFAULT '0',
  `Flags` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0'
,  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_mechanic`;
CREATE TABLE `garr_mechanic` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `GarrMechanicTypeID` int NOT NULL DEFAULT '0',
  `Factor` float NOT NULL DEFAULT '0',
  `GarrAbilityID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0'
,  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_mechanic_set_x_mechanic`;
CREATE TABLE `garr_mechanic_set_x_mechanic` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `GarrMechanicID` int NOT NULL DEFAULT '0',
  `GarrMechanicSetID` int unsigned NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0'
,  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_mechanic_type`;
CREATE TABLE `garr_mechanic_type` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `Name` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Description` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `GarrAbilityCategoryID` int NOT NULL DEFAULT '0',
  `Category` tinyint unsigned NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0'
,  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_talent`;
CREATE TABLE `garr_talent` (
  `Name` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Description` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `ID` int unsigned NOT NULL DEFAULT '0',
  `GarrTalentTreeID` int unsigned NOT NULL DEFAULT '0',
  `Tier` tinyint NOT NULL DEFAULT '0',
  `UiOrder` tinyint NOT NULL DEFAULT '0',
  `IconFileDataID` int NOT NULL DEFAULT '0',
  `PlayerConditionID` int unsigned NOT NULL DEFAULT '0',
  `GarrAbilityID` int unsigned NOT NULL DEFAULT '0',
  `Flags` int NOT NULL DEFAULT '0',
  `TalentType` int NOT NULL DEFAULT '0',
  `PrerequisiteTalentID` int NOT NULL DEFAULT '0',
  `ResearchCostSource` int NOT NULL DEFAULT '0',
  `ActiveDurationSecs` int NOT NULL DEFAULT '0',
  `GarrTalentSocketPropertiesID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0'
,  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_talent_cost`;
CREATE TABLE `garr_talent_cost` (
  `MoneyQuantity` bigint unsigned NOT NULL DEFAULT '0',
  `ID` int unsigned NOT NULL DEFAULT '0',
  `GarrTalentTreeID` int unsigned NOT NULL DEFAULT '0',
  `GarrTalentID` int NOT NULL DEFAULT '0',
  `RankIndex` int NOT NULL DEFAULT '0',
  `GarrTalentRankID` int NOT NULL DEFAULT '0',
  `CostType` int NOT NULL DEFAULT '0',
  `CurrencyTypesID` int NOT NULL DEFAULT '0',
  `CurrencyQuantity` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0'
,  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_talent_rank`;
CREATE TABLE `garr_talent_rank` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `Rank` int NOT NULL DEFAULT '0',
  `PerkSpellID` int NOT NULL DEFAULT '0',
  `PerkPlayerConditionID` int NOT NULL DEFAULT '0',
  `Points` float NOT NULL DEFAULT '0',
  `ResearchCost` int NOT NULL DEFAULT '0',
  `ResearchCostCurrencyTypesID` int NOT NULL DEFAULT '0',
  `ResearchGoldCost` int NOT NULL DEFAULT '0',
  `ResearchDurationSecs` int NOT NULL DEFAULT '0',
  `RespecCost` int NOT NULL DEFAULT '0',
  `RespecCostCurrencyTypesID` int NOT NULL DEFAULT '0',
  `RespecGoldCost` int NOT NULL DEFAULT '0',
  `RespecDurationSecs` int NOT NULL DEFAULT '0',
  `AlternateResearchCost` int NOT NULL DEFAULT '0',
  `AlternateResearchCostCurrencyTypesID` int NOT NULL DEFAULT '0',
  `AlternateResearchGoldCost` int NOT NULL DEFAULT '0',
  `AlternateResearchDurationSecs` int NOT NULL DEFAULT '0',
  `GarrTalentID` int unsigned NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0'
,  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_ability_category_locale`;
CREATE TABLE `garr_ability_category_locale` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `locale` varchar(4) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Name_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` int NOT NULL DEFAULT '0'
,  PRIMARY KEY (`ID`,`locale`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_encounter_locale`;
CREATE TABLE `garr_encounter_locale` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `locale` varchar(4) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Name_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` int NOT NULL DEFAULT '0'
,  PRIMARY KEY (`ID`,`locale`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_mechanic_type_locale`;
CREATE TABLE `garr_mechanic_type_locale` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `locale` varchar(4) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Name_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Description_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` int NOT NULL DEFAULT '0'
,  PRIMARY KEY (`ID`,`locale`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_talent_locale`;
CREATE TABLE `garr_talent_locale` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `locale` varchar(4) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Name_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Description_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` int NOT NULL DEFAULT '0'
,  PRIMARY KEY (`ID`,`locale`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

