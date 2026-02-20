--
-- Add missing battle pet hotfix tables for pet battle system
--

-- Table structure for table `battle_pet_ability_effect`
DROP TABLE IF EXISTS `battle_pet_ability_effect`;
CREATE TABLE `battle_pet_ability_effect` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `BattlePetAbilityTurnID` smallint unsigned NOT NULL DEFAULT '0',
  `OrderIndex` tinyint unsigned NOT NULL DEFAULT '0',
  `Aura` smallint unsigned NOT NULL DEFAULT '0',
  `BattlePetEffectPropertiesID` smallint unsigned NOT NULL DEFAULT '0',
  `VisualID` smallint unsigned NOT NULL DEFAULT '0',
  `Param1` smallint NOT NULL DEFAULT '0',
  `Param2` smallint NOT NULL DEFAULT '0',
  `Param3` smallint NOT NULL DEFAULT '0',
  `Param4` smallint NOT NULL DEFAULT '0',
  `Param5` smallint NOT NULL DEFAULT '0',
  `Param6` smallint NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Table structure for table `battle_pet_ability_state`
DROP TABLE IF EXISTS `battle_pet_ability_state`;
CREATE TABLE `battle_pet_ability_state` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `BattlePetStateID` int NOT NULL DEFAULT '0',
  `BattlePetAbilityID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Table structure for table `battle_pet_ability_turn`
DROP TABLE IF EXISTS `battle_pet_ability_turn`;
CREATE TABLE `battle_pet_ability_turn` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `BattlePetAbilityID` smallint unsigned NOT NULL DEFAULT '0',
  `OrderIndex` tinyint unsigned NOT NULL DEFAULT '0',
  `TurnTypeEnum` tinyint unsigned NOT NULL DEFAULT '0',
  `EventTypeEnum` tinyint unsigned NOT NULL DEFAULT '0',
  `BattlePetVisualID` smallint unsigned NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Table structure for table `battle_pet_effect_properties`
DROP TABLE IF EXISTS `battle_pet_effect_properties`;
CREATE TABLE `battle_pet_effect_properties` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `ParamLabel1` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `ParamLabel2` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `ParamLabel3` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `ParamLabel4` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `ParamLabel5` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `ParamLabel6` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `BattlePetVisualID` smallint unsigned NOT NULL DEFAULT '0',
  `ParamTypeEnum1` tinyint unsigned NOT NULL DEFAULT '0',
  `ParamTypeEnum2` tinyint unsigned NOT NULL DEFAULT '0',
  `ParamTypeEnum3` tinyint unsigned NOT NULL DEFAULT '0',
  `ParamTypeEnum4` tinyint unsigned NOT NULL DEFAULT '0',
  `ParamTypeEnum5` tinyint unsigned NOT NULL DEFAULT '0',
  `ParamTypeEnum6` tinyint unsigned NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Table structure for table `battle_pet_species_x_ability`
DROP TABLE IF EXISTS `battle_pet_species_x_ability`;
CREATE TABLE `battle_pet_species_x_ability` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `BattlePetAbilityID` smallint unsigned NOT NULL DEFAULT '0',
  `RequiredLevel` tinyint unsigned NOT NULL DEFAULT '0',
  `SlotEnum` tinyint NOT NULL DEFAULT '0',
  `BattlePetSpeciesID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
