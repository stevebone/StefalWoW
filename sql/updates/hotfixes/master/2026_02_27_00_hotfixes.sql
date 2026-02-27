-- =============================================
-- GarrBuildingDoodadSet.db2
-- =============================================
DROP TABLE IF EXISTS `garr_building_doodad_set`;
CREATE TABLE `garr_building_doodad_set` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrBuildingID` INT UNSIGNED NOT NULL DEFAULT '0',
  `HordeDoodadSetID` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `AllianceDoodadSetID` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `HordeAltDoodadSetID` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `AllianceAltDoodadSetID` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrFollItemSetMember.db2
-- =============================================
DROP TABLE IF EXISTS `garr_foll_item_set_member`;
CREATE TABLE `garr_foll_item_set_member` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrFollItemSetID` INT UNSIGNED NOT NULL DEFAULT '0',
  `ItemSlot` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `ItemModifiedAppearanceID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrFollowerID` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrFollowerLevelXP.db2
-- =============================================
DROP TABLE IF EXISTS `garr_follower_level_xp`;
CREATE TABLE `garr_follower_level_xp` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `FollowerLevel` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `GarrFollowerTypeID` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `XpToNextLevel` INT UNSIGNED NOT NULL DEFAULT '0',
  `ShipmentXP` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrItemLevelUpgradeData.db2
-- =============================================
DROP TABLE IF EXISTS `garr_item_level_upgrade_data`;
CREATE TABLE `garr_item_level_upgrade_data` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Operation` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `MinItemLevel` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  `MaxItemLevel` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  `FollowerTypeID` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrMissionSet.db2
-- =============================================
DROP TABLE IF EXISTS `garr_mission_set`;
CREATE TABLE `garr_mission_set` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrTypeID` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `MissionCostCurrencyTypesID` INT UNSIGNED NOT NULL DEFAULT '0',
  `ContentTuningID` INT UNSIGNED NOT NULL DEFAULT '0',
  `MapID` INT NOT NULL DEFAULT '0',
  `Flags` INT UNSIGNED NOT NULL DEFAULT '0',
  `Priority` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrMissionType.db2
-- =============================================
DROP TABLE IF EXISTS `garr_mission_type`;
CREATE TABLE `garr_mission_type` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Name` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `UiTextureAtlasMemberID` INT UNSIGNED NOT NULL DEFAULT '0',
  `UiTextureKitID` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_mission_type_locale`;
CREATE TABLE `garr_mission_type_locale` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `locale` VARCHAR(4) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL DEFAULT '',
  `Name_lang` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`, `locale`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrMssnBonusAbility.db2
-- =============================================
DROP TABLE IF EXISTS `garr_mssn_bonus_ability`;
CREATE TABLE `garr_mssn_bonus_ability` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrMssnBonusAbilityType` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `MssnSort` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `GarrAbilityID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Modifier` FLOAT NOT NULL DEFAULT '0',
  `MissionSetID` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrType.db2
-- =============================================
DROP TABLE IF EXISTS `garr_type`;
CREATE TABLE `garr_type` (
  `ID` TINYINT NOT NULL DEFAULT '0',
  `PrimaryCurrencyTypeID` INT UNSIGNED NOT NULL DEFAULT '0',
  `SecondaryCurrencyTypeID` INT UNSIGNED NOT NULL DEFAULT '0',
  `ExpansionID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Flags` INT NOT NULL DEFAULT '0',
  `AutoFollowerHealRate` FLOAT NOT NULL DEFAULT '0',
  `MissionCostCurveID` INT NOT NULL DEFAULT '0',
  `AutoFollowerHealCostMult` FLOAT NOT NULL DEFAULT '0',
  `MapIDs1` INT NOT NULL DEFAULT '0',
  `MapIDs2` INT NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrFollowerQuality.db2
-- =============================================
DROP TABLE IF EXISTS `garr_follower_quality`;
CREATE TABLE `garr_follower_quality` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `XpThreshold` INT UNSIGNED NOT NULL DEFAULT '0',
  `QualityItemID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Quality` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `AbilityCount` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `TraitCount` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `GarrFollowerTypeID` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `ClassSpecID` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrMissionXFollower.db2
-- =============================================
DROP TABLE IF EXISTS `garr_mission_x_follower`;
CREATE TABLE `garr_mission_x_follower` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrFollowerID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrFollowerTypeID` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `BoardIndex` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `GarrMissionID` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrSpecialization.db2
-- =============================================
DROP TABLE IF EXISTS `garr_specialization`;
CREATE TABLE `garr_specialization` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Name` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Description` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `GarrTypeID` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `IconFileDataID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrFollowerTypeID` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `Param` INT NOT NULL DEFAULT '0',
  `Bonus1` INT NOT NULL DEFAULT '0',
  `Bonus2` INT NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_specialization_locale`;
CREATE TABLE `garr_specialization_locale` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `locale` VARCHAR(4) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL DEFAULT '',
  `Name_lang` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Description_lang` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`, `locale`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- CharShipmentContainer.db2
-- =============================================
DROP TABLE IF EXISTS `char_shipment_container`;
CREATE TABLE `char_shipment_container` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Description` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `PendingText` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `UiTextureKitID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrTypeID` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `GarrBuildingType` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `BaseCapacity` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `SmallDisplayInfoID` INT UNSIGNED NOT NULL DEFAULT '0',
  `MediumDisplayInfoID` INT UNSIGNED NOT NULL DEFAULT '0',
  `LargeDisplayInfoID` INT UNSIGNED NOT NULL DEFAULT '0',
  `WorkingSpellVisualID` INT UNSIGNED NOT NULL DEFAULT '0',
  `CompleteSpellVisualID` INT UNSIGNED NOT NULL DEFAULT '0',
  `WorkingDisplayInfoID` INT UNSIGNED NOT NULL DEFAULT '0',
  `MediumThreshold` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `LargeThreshold` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `Faction` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `CrossFactionID` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `char_shipment_container_locale`;
CREATE TABLE `char_shipment_container_locale` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `locale` VARCHAR(4) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL DEFAULT '',
  `Description_lang` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `PendingText_lang` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`, `locale`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrAbilityEffect.db2
-- =============================================
DROP TABLE IF EXISTS `garr_ability_effect`;
CREATE TABLE `garr_ability_effect` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrAbilityID` INT UNSIGNED NOT NULL DEFAULT '0',
  `EffectType` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `TargetMask` INT UNSIGNED NOT NULL DEFAULT '0',
  `Amount` FLOAT NOT NULL DEFAULT '0',
  `CombatWeightBase` FLOAT NOT NULL DEFAULT '0',
  `CombatWeightMax` FLOAT NOT NULL DEFAULT '0',
  `ActionValueFlat` FLOAT NOT NULL DEFAULT '0',
  `AbilityAction` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `MiscValueA` INT NOT NULL DEFAULT '0',
  `ActionHours` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `ActionRecordID` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrAutoCombatant.db2
-- =============================================
DROP TABLE IF EXISTS `garr_auto_combatant`;
CREATE TABLE `garr_auto_combatant` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Attack` INT NOT NULL DEFAULT '0',
  `Health` INT NOT NULL DEFAULT '0',
  `MaxHealth` INT NOT NULL DEFAULT '0',
  `AutoAttackSpellID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Role` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `BoardIndex` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `GarrEncounterID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrAutoSpellID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Flags` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrAutoSpellEffect.db2
-- =============================================
DROP TABLE IF EXISTS `garr_auto_spell_effect`;
CREATE TABLE `garr_auto_spell_effect` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrAutoSpellID` INT UNSIGNED NOT NULL DEFAULT '0',
  `EffectType` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `Targets` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `Amount` FLOAT NOT NULL DEFAULT '0',
  `MiscType` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `MiscValue` INT NOT NULL DEFAULT '0',
  `Period` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrClassSpecPlayerCond.db2
-- =============================================
DROP TABLE IF EXISTS `garr_class_spec_player_cond`;
CREATE TABLE `garr_class_spec_player_cond` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `ClassSpec` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `OrderIndex` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `GarrClassSpecID` INT UNSIGNED NOT NULL DEFAULT '0',
  `PlayerConditionID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrStringID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Flags` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_class_spec_player_cond_locale`;
CREATE TABLE `garr_class_spec_player_cond_locale` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `locale` VARCHAR(4) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL DEFAULT '',
  `ClassSpec_lang` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`, `locale`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrFollowerUICreature.db2
-- =============================================
DROP TABLE IF EXISTS `garr_follower_ui_creature`;
CREATE TABLE `garr_follower_ui_creature` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Race` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `Gender` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `CreatureID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Scale` FLOAT NOT NULL DEFAULT '0',
  `FileDataID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrFollowerID` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrMissionTexture.db2
-- =============================================
DROP TABLE IF EXISTS `garr_mission_texture`;
CREATE TABLE `garr_mission_texture` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `PosX` FLOAT NOT NULL DEFAULT '0',
  `PosY` FLOAT NOT NULL DEFAULT '0',
  `UiTextureAtlasMemberID` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrPlotUICategory.db2
-- =============================================
DROP TABLE IF EXISTS `garr_plot_ui_category`;
CREATE TABLE `garr_plot_ui_category` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Name` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `PlotType` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_plot_ui_category_locale`;
CREATE TABLE `garr_plot_ui_category_locale` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `locale` VARCHAR(4) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL DEFAULT '',
  `Name_lang` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`, `locale`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrString.db2
-- =============================================
DROP TABLE IF EXISTS `garr_string`;
CREATE TABLE `garr_string` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Text` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_string_locale`;
CREATE TABLE `garr_string_locale` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `locale` VARCHAR(4) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL DEFAULT '',
  `Text_lang` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`, `locale`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrTalentResearch.db2
-- =============================================
DROP TABLE IF EXISTS `garr_talent_research`;
CREATE TABLE `garr_talent_research` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GoldCost` BIGINT UNSIGNED NOT NULL DEFAULT '0',
  `CurrencyTypesID` INT UNSIGNED NOT NULL DEFAULT '0',
  `CurrencyTypesCost` BIGINT UNSIGNED NOT NULL DEFAULT '0',
  `DurationSecs` INT UNSIGNED NOT NULL DEFAULT '0',
  `RespecGoldCost` BIGINT UNSIGNED NOT NULL DEFAULT '0',
  `RespecCurrencyTypesID` INT UNSIGNED NOT NULL DEFAULT '0',
  `RespecCurrencyTypesCost` BIGINT UNSIGNED NOT NULL DEFAULT '0',
  `RespecDurationSecs` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrTalentSocketProperties.db2
-- =============================================
DROP TABLE IF EXISTS `garr_talent_socket_properties`;
CREATE TABLE `garr_talent_socket_properties` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrTalentSocketType` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `GarrTalentSocketSubtype` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrTalentMapPOI.db2
-- =============================================
DROP TABLE IF EXISTS `garr_talent_map_poi`;
CREATE TABLE `garr_talent_map_poi` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Position1` FLOAT NOT NULL DEFAULT '0',
  `Position2` FLOAT NOT NULL DEFAULT '0',
  `GarrTalentID` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrTalentRankGroupEntry.db2
-- =============================================
DROP TABLE IF EXISTS `garr_talent_rank_group_entry`;
CREATE TABLE `garr_talent_rank_group_entry` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrTalentRankGroupID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrTalentRankID` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrTalentRankGroupResearchMod.db2
-- =============================================
DROP TABLE IF EXISTS `garr_talent_rank_group_research_mod`;
CREATE TABLE `garr_talent_rank_group_research_mod` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `ModifierType` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `PlayerConditionID` INT UNSIGNED NOT NULL DEFAULT '0',
  `ModifierValue` FLOAT NOT NULL DEFAULT '0',
  `GarrTalentRankGroupID` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrTalTreeXGarrTalResearch.db2
-- =============================================
DROP TABLE IF EXISTS `garr_tal_tree_x_garr_tal_research`;
CREATE TABLE `garr_tal_tree_x_garr_tal_research` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrTalentTreeID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrTalentResearchID` INT UNSIGNED NOT NULL DEFAULT '0',
  `OrderIndex` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrUiAnimClassInfo.db2
-- =============================================
DROP TABLE IF EXISTS `garr_ui_anim_class_info`;
CREATE TABLE `garr_ui_anim_class_info` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `ClassID` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `IsFemale` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `WalkSpeed` FLOAT NOT NULL DEFAULT '0',
  `RunAnimID` INT UNSIGNED NOT NULL DEFAULT '0',
  `WalkAnimID` INT UNSIGNED NOT NULL DEFAULT '0',
  `IdleAnimID` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrUiAnimRaceInfo.db2
-- =============================================
DROP TABLE IF EXISTS `garr_ui_anim_race_info`;
CREATE TABLE `garr_ui_anim_race_info` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `RaceID` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `RunSpeedModifier` FLOAT NOT NULL DEFAULT '0',
  `RunAnimSpeedModifier` FLOAT NOT NULL DEFAULT '0',
  `WalkAnimSpeedModifier` FLOAT NOT NULL DEFAULT '0',
  `IdleAnimSpeedModifier` FLOAT NOT NULL DEFAULT '0',
  `PortraitScale` FLOAT NOT NULL DEFAULT '0',
  `PortraitHeight` FLOAT NOT NULL DEFAULT '0',
  `PortraitFieldOfView` FLOAT NOT NULL DEFAULT '0',
  `PortraitCameraPositionX` FLOAT NOT NULL DEFAULT '0',
  `PortraitCameraPositionY` FLOAT NOT NULL DEFAULT '0',
  `PortraitCameraPositionZ` FLOAT NOT NULL DEFAULT '0',
  `PortraitCameraTargetX` FLOAT NOT NULL DEFAULT '0',
  `PortraitCameraTargetY` FLOAT NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GossipXGarrTalentTrees.db2
-- =============================================
DROP TABLE IF EXISTS `gossip_x_garr_talent_trees`;
CREATE TABLE `gossip_x_garr_talent_trees` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrTalentTreeID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GossipID` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =============================================
-- GarrAutoSpell.db2
-- =============================================
DROP TABLE IF EXISTS `garr_auto_spell`;
CREATE TABLE `garr_auto_spell` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Name` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Description` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Cooldown` INT UNSIGNED NOT NULL DEFAULT '0',
  `Duration` INT UNSIGNED NOT NULL DEFAULT '0',
  `SchoolMask` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `SpellVisualID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Flags` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_auto_spell_locale`;
CREATE TABLE `garr_auto_spell_locale` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `locale` VARCHAR(4) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL DEFAULT '',
  `Name_lang` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Description_lang` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`, `locale`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
