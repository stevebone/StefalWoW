--
-- Shadowlands Adventures: correct the hotfix column layout of the auto-combat DB2 mirrors.
--
-- GarrAutoCombatant.db2 (layout 0x6ADAF487), GarrEncounter.db2 (layout 0x90365AF7) and
-- GarrMissionXEncounter.db2 (layout 0x08428AE4) as shipped by client 12.0.7.68275 do not have the
-- columns these tables were created with. The names below are taken from the WoWDBDefs definition
-- for the layout hash that DB2Metadata.h already pins for this build, and they match the schema the
-- current TDB hotfixes dump ships. The old names were positionally aligned but semantically wrong:
--   garr_auto_combatant       Attack/Health/MaxHealth/AutoAttackSpellID/Role/BoardIndex/
--                             GarrEncounterID/GarrAutoSpellID/Flags
--                          -> HealthBase/HealthGainPerLevel/AttackBase/AttackGainPerLevel/
--                             AttackSpellID/AbilitySpellID/AbilitySpellID2/PassiveSpellID/Role
--   garr_encounter            CreatureDisplayInfoID/UiAnimHeight/UiAnimScale/UiTextureScale/
--                             EnvGarrMechanicTypeID/GarrEncounterSetID
--                          -> PortraitFileDataID/UiTextureKitID/UiAnimScale/UiAnimHeight/
--                             Flags/AutoCombatantID
--   garr_mission_x_encounter  GarrMissionSetEncounterID/CombatWeightBase/CombatWeightMax
--                          -> GarrEncounterSetID/OrderIndex/BoardIndex
--
-- All three tables are hotfix mirrors that hold no rows on this realm (the data is read from the
-- client DB2 files), so recreating them loses nothing.
--

DROP TABLE IF EXISTS `garr_auto_combatant`;
CREATE TABLE `garr_auto_combatant` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `HealthBase` INT NOT NULL DEFAULT '0',
  `HealthGainPerLevel` INT NOT NULL DEFAULT '0',
  `AttackBase` INT NOT NULL DEFAULT '0',
  `AttackGainPerLevel` INT NOT NULL DEFAULT '0',
  `AttackSpellID` INT NOT NULL DEFAULT '0',
  `AbilitySpellID` INT NOT NULL DEFAULT '0',
  `AbilitySpellID2` INT NOT NULL DEFAULT '0',
  `PassiveSpellID` INT NOT NULL DEFAULT '0',
  `Role` INT NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_encounter`;
CREATE TABLE `garr_encounter` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Name` TEXT DEFAULT NULL,
  `CreatureID` INT NOT NULL DEFAULT '0',
  `PortraitFileDataID` INT NOT NULL DEFAULT '0',
  `UiTextureKitID` INT UNSIGNED NOT NULL DEFAULT '0',
  `UiAnimScale` FLOAT NOT NULL DEFAULT '0',
  `UiAnimHeight` FLOAT NOT NULL DEFAULT '0',
  `Flags` INT NOT NULL DEFAULT '0',
  `AutoCombatantID` INT NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_mission_x_encounter`;
CREATE TABLE `garr_mission_x_encounter` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrEncounterID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrEncounterSetID` INT UNSIGNED NOT NULL DEFAULT '0',
  `OrderIndex` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `BoardIndex` TINYINT NOT NULL DEFAULT '0',
  `GarrMissionID` INT NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
