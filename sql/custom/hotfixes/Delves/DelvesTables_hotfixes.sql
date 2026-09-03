-- ============================================================================
-- HOTFIX DATABASE TABLES - Perks and Delves (DB2 Overrides)
-- ============================================================================

-- Delves Season
DROP TABLE IF EXISTS `delves_season`;
CREATE TABLE IF NOT EXISTS `delves_season` (
    `ID` INT(10) UNSIGNED NOT NULL DEFAULT 0,
    `FactionID` INT(11) NOT NULL DEFAULT 0,
    `VerifiedBuild` INT(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`, `VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4;

DELETE FROM `delves_season`;
INSERT INTO `delves_season` (`ID`, `FactionID`, `VerifiedBuild`) VALUES
(1, 2644, 66527),
(2, 2683, 66527),
(3, 2722, 66527),
(4, 2742, 66527);  -- Season 4 = Midnight S1 (Delver's of the Deep)

-- Delves Season X Spell
DROP TABLE IF EXISTS `delves_season_x_spell`;
CREATE TABLE IF NOT EXISTS `delves_season_x_spell` (
    `ID` INT(10) UNSIGNED NOT NULL DEFAULT 0,
    `SpellID` INT(11) NOT NULL DEFAULT 0,
    `DelvesSeasonID` INT(11) NOT NULL DEFAULT 0,
    `VerifiedBuild` INT(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`, `VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4;

-- Player Companion Info
DROP TABLE IF EXISTS `player_companion_info`;
CREATE TABLE IF NOT EXISTS `player_companion_info` (
    `ID` INT(10) UNSIGNED NOT NULL DEFAULT 0,
    `DelvesSeasonID` INT(11) NOT NULL DEFAULT 0,
    `TraitTreeID` INT(11) NOT NULL DEFAULT 0,
    `TraitNodeID_DPS` INT(11) NOT NULL DEFAULT 0,
    `TraitNodeID_Heal` INT(11) NOT NULL DEFAULT 0,
    `TraitSubTreeID_DPS` INT(11) NOT NULL DEFAULT 0,
    `TraitSubTreeID_Heal` INT(11) NOT NULL DEFAULT 0,
    `TraitSubTreeID_Tank` INT(11) NOT NULL DEFAULT 0,
    `FactionID` INT(11) NOT NULL DEFAULT 0,
    `CreatureDisplayInfoID` INT(11) NOT NULL DEFAULT 0,
    `UiModelSceneID` INT(11) NOT NULL DEFAULT 0,
    `Field_011` INT(11) NOT NULL DEFAULT 0,
    `Field_012` INT(11) NOT NULL DEFAULT 0,
    `FlavorNodeID` INT(11) NOT NULL DEFAULT 0,
    `Field_014` INT(11) NOT NULL DEFAULT 0,
    `UnlockDescription` TEXT,
    `Field_001` TEXT,
    `VerifiedBuild` INT(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`, `VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4;

DELETE FROM `player_companion_info`;
INSERT INTO `player_companion_info` (`ID`, `DelvesSeasonID`, `TraitTreeID`, `TraitNodeID_DPS`, `TraitNodeID_Heal`, `TraitSubTreeID_DPS`, `TraitSubTreeID_Heal`, `TraitSubTreeID_Tank`, `FactionID`, `CreatureDisplayInfoID`, `UiModelSceneID`, `Field_011`, `Field_012`, `FlavorNodeID`, `Field_014`, `UnlockDescription`, `Field_001`, `VerifiedBuild`) VALUES
(1,  1, 874,  99855,  99854,  29,  30,  81,  2640, 115505, 898,  395, 0, 0, 1, 'Unlocks with the quest "Delve into the Earth" in the Isle of Dorn campaign.', NULL, 66527),
(9,  2, 1060, 104031, 104036, 84,  83,  82,  2640, 115505, 898,  395, 0, 0, 1, 'Unlocks with the quest "Delve into the Earth" in the Isle of Dorn campaign.', NULL, 66527),
(10, 3, 1151, 107570, 107571, 100, 98,  99,  2640, 115505, 898,  395, 1, 0, 1, 'Unlocks with the quest "Delve into the Earth" in the Isle of Dorn campaign.', NULL, 66527),
(11, 4, 1168, 109951, 109950, 108, 109, 110, 2744, 67214,  1135, 396, 1, 0, 2, 'Unlocks with the quest "Void Walk With Me" in the Eversong Woods campaign.', NULL, 66527),
(12, 5, 1223, 110786, 110785, 141, 142, 140, 2744, 67214,  1135, 396, 1, 110784, 2, 'Unlocks with the quest "Void Walk With Me" in the Eversong Woods campaign.', 'Poisons', 66527);

-- Player Companion Info Locale
DROP TABLE IF EXISTS `player_companion_info_locale`;
CREATE TABLE IF NOT EXISTS `player_companion_info_locale` (
    `ID` INT(10) UNSIGNED NOT NULL DEFAULT 0,
    `locale` VARCHAR(4) NOT NULL,
    `UnlockDescription_lang` TEXT,
    `Field_001_lang` TEXT,
    `VerifiedBuild` INT(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`, `locale`, `VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4;


-- Perks Activity Condition
DROP TABLE IF EXISTS `perks_activity_condition`;
CREATE TABLE IF NOT EXISTS `perks_activity_condition` (
    `ID` INT(10) UNSIGNED NOT NULL DEFAULT 0,
    `PlayerConditionID` INT(11) NOT NULL DEFAULT 0,
    `Field_002` INT(11) NOT NULL DEFAULT 0,
    `PerksActivityID` INT(11) NOT NULL DEFAULT 0,
    `VerifiedBuild` INT(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`, `VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4;

-- Perks Activity Tag
DROP TABLE IF EXISTS `perks_activity_tag`;
CREATE TABLE IF NOT EXISTS `perks_activity_tag` (
    `ID` INT(10) UNSIGNED NOT NULL DEFAULT 0,
    `TagName` TEXT,
    `VerifiedBuild` INT(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`, `VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4;

-- Perks Activity Tag Locale
DROP TABLE IF EXISTS `perks_activity_tag_locale`;
CREATE TABLE IF NOT EXISTS `perks_activity_tag_locale` (
    `ID` INT(10) UNSIGNED NOT NULL DEFAULT 0,
    `locale` VARCHAR(4) NOT NULL,
    `TagName_lang` TEXT,
    `VerifiedBuild` INT(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`, `locale`, `VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4;

-- Perks Activity X Holidays
DROP TABLE IF EXISTS `perks_activity_x_holidays`;
CREATE TABLE IF NOT EXISTS `perks_activity_x_holidays` (
    `ID` INT(10) UNSIGNED NOT NULL DEFAULT 0,
    `HolidayID` INT(11) NOT NULL DEFAULT 0,
    `PerksActivityID` INT(11) NOT NULL DEFAULT 0,
    `VerifiedBuild` INT(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`, `VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4;

-- Perks Activity X Tag
DROP TABLE IF EXISTS `perks_activity_x_tag`;
CREATE TABLE IF NOT EXISTS `perks_activity_x_tag` (
    `ID` INT(10) UNSIGNED NOT NULL DEFAULT 0,
    `PerksActivityTagID` INT(11) NOT NULL DEFAULT 0,
    `PerksActivityID` INT(11) NOT NULL DEFAULT 0,
    `VerifiedBuild` INT(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`, `VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4;

-- Perks UI Theme
DROP TABLE IF EXISTS `perks_ui_theme`;
CREATE TABLE IF NOT EXISTS `perks_ui_theme` (
    `ID` INT(10) UNSIGNED NOT NULL DEFAULT 0,
    `UiTextureKitID` INT(11) NOT NULL DEFAULT 0,
    `Field_002` INT(11) NOT NULL DEFAULT 0,
    `VerifiedBuild` INT(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`, `VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4;

-- Perks Vendor Category
DROP TABLE IF EXISTS `perks_vendor_category`;
CREATE TABLE IF NOT EXISTS `perks_vendor_category` (
    `ID` INT(10) UNSIGNED NOT NULL DEFAULT 0,
    `DisplayName` TEXT,
    `PerksVendorType` INT(11) NOT NULL DEFAULT 0,
    `DefaultUIModelSceneID` INT(11) NOT NULL DEFAULT 0,
    `VerifiedBuild` INT(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`, `VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4;

-- Perks Vendor Category Locale
DROP TABLE IF EXISTS `perks_vendor_category_locale`;
CREATE TABLE IF NOT EXISTS `perks_vendor_category_locale` (
    `ID` INT(10) UNSIGNED NOT NULL DEFAULT 0,
    `locale` VARCHAR(4) NOT NULL,
    `DisplayName_lang` TEXT,
    `VerifiedBuild` INT(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`, `locale`, `VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4;

-- Perks Vendor Item (DB2)
DROP TABLE IF EXISTS `perks_vendor_item`;
CREATE TABLE IF NOT EXISTS `perks_vendor_item` (
    `ID` INT(10) UNSIGNED NOT NULL DEFAULT 0,
    `PerksVendorCategoryID` TINYINT(4) NOT NULL DEFAULT 0,
    `Field_002` INT(11) NOT NULL DEFAULT 0,
    `ItemID` INT(11) NOT NULL DEFAULT 0,
    `Field_004` INT(11) NOT NULL DEFAULT 0,
    `CreatureDisplayInfoID` INT(11) NOT NULL DEFAULT 0,
    `Cost` INT(11) NOT NULL DEFAULT 0,
    `UiModelSceneID` INT(11) NOT NULL DEFAULT 0,
    `UiGroupInfo` INT(11) NOT NULL DEFAULT 0,
    `VerifiedBuild` INT(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`, `VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4;

-- Perks Vendor Item UI Group
DROP TABLE IF EXISTS `perks_vendor_item_ui_group`;
CREATE TABLE IF NOT EXISTS `perks_vendor_item_ui_group` (
    `ID` INT(10) UNSIGNED NOT NULL DEFAULT 0,
    `Name` TEXT,
    `Priority` INT(11) NOT NULL DEFAULT 0,
    `VerifiedBuild` INT(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`, `VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4;

-- Perks Vendor Item UI Group Locale
DROP TABLE IF EXISTS `perks_vendor_item_ui_group_locale`;
CREATE TABLE IF NOT EXISTS `perks_vendor_item_ui_group_locale` (
    `ID` INT(10) UNSIGNED NOT NULL DEFAULT 0,
    `locale` VARCHAR(4) NOT NULL,
    `Name_lang` TEXT,
    `VerifiedBuild` INT(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`, `locale`, `VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4;

-- Perks Vendor Item UI Info
DROP TABLE IF EXISTS `perks_vendor_item_ui_info`;
CREATE TABLE IF NOT EXISTS `perks_vendor_item_ui_info` (
    `ID` INT(10) UNSIGNED NOT NULL DEFAULT 0,
    `Field_001` INT(11) NOT NULL DEFAULT 0,
    `CreatureDisplayInfoID` INT(11) NOT NULL DEFAULT 0,
    `Field_003` INT(11) NOT NULL DEFAULT 0,
    `VerifiedBuild` INT(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`, `VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4;

-- Perks Vendor Item X Interval
DROP TABLE IF EXISTS `perks_vendor_item_x_interval`;
CREATE TABLE IF NOT EXISTS `perks_vendor_item_x_interval` (
    `ID` INT(10) UNSIGNED NOT NULL DEFAULT 0,
    `PerksVendorItemID` INT(11) NOT NULL DEFAULT 0,
    `PerksActivityThresholdID` INT(11) NOT NULL DEFAULT 0,
    `VerifiedBuild` INT(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`, `VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4;

-- Perks Activity Threshold
DROP TABLE IF EXISTS `perks_activity_threshold`;
CREATE TABLE IF NOT EXISTS `perks_activity_threshold` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `PerksActivityCount` INT NOT NULL DEFAULT '0',
  `CurrencyAmount` INT NOT NULL DEFAULT '0',
  `Unknown` INT NOT NULL DEFAULT '0',
  `ThresholdGroupID` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` INT NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Perks Activity Threshold Group
DROP TABLE IF EXISTS `perks_activity_threshold_group`;
CREATE TABLE IF NOT EXISTS `perks_activity_threshold_group` (
  `Name` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Unknown` INT NOT NULL DEFAULT '0',
  `VerifiedBuild` INT NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Perks Activity Threshold Group Locale
DROP TABLE IF EXISTS `perks_activity_threshold_group_locale`;
CREATE TABLE IF NOT EXISTS `perks_activity_threshold_group_locale` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `locale` VARCHAR(4) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Name_lang` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` INT NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`locale`,`VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Perks Activity X Interval
DROP TABLE IF EXISTS `perks_activity_x_interval`;
CREATE TABLE IF NOT EXISTS `perks_activity_x_interval` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `PerksActivityID` INT NOT NULL DEFAULT '0',
  `IntervalID` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` INT NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
