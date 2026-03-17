DROP TABLE IF EXISTS `warband_scene_placement`;
CREATE TABLE `warband_scene_placement` (
  `PositionX` float NOT NULL DEFAULT '0',
  `PositionY` float NOT NULL DEFAULT '0',
  `PositionZ` float NOT NULL DEFAULT '0',
  `ID` int unsigned NOT NULL DEFAULT '0',
  `WarbandSceneID` int unsigned NOT NULL DEFAULT '0',
  `SlotType` int NOT NULL DEFAULT '0',
  `Rotation` float NOT NULL DEFAULT '0',
  `Scale` float NOT NULL DEFAULT '0',
  `Field_11_0_0_54210_004` int unsigned NOT NULL DEFAULT '0',
  `Field_11_0_0_54210_005` int unsigned NOT NULL DEFAULT '0',
  `SlotID` int NOT NULL DEFAULT '0',
  `Field_11_1_0_58221_009` int NOT NULL DEFAULT '0',
  `Field_12_0_0_63534_008` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `warband_scene_animation`;
CREATE TABLE `warband_scene_animation` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `SpellVisualKitID` int NOT NULL DEFAULT '0',
  `Event` int NOT NULL DEFAULT '0',
  `AnimKitID` int NOT NULL DEFAULT '0',
  `Field_11_0_0_54210_003` int NOT NULL DEFAULT '0',
  `TimeIsh` float NOT NULL DEFAULT '0',
  `Field_11_0_0_54935_005` tinyint unsigned NOT NULL DEFAULT '0',
  `Field_11_0_0_55000_006` tinyint unsigned NOT NULL DEFAULT '0',
  `Field_11_1_0_58221_008` tinyint NOT NULL DEFAULT '0',
  `Field_11_0_0_54210_005_0` int NOT NULL DEFAULT '0',
  `Field_11_0_0_54210_005_1` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `warband_scene_anim_chr_spec`;
CREATE TABLE `warband_scene_anim_chr_spec` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `WarbandSceneAnimationID` int NOT NULL DEFAULT '0',
  `ChrSpecializationID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `warband_scene_placement_filter_req`;
CREATE TABLE `warband_scene_placement_filter_req` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `Field_11_1_0_58221_000` bigint NOT NULL DEFAULT '0',
  `Field_11_1_0_58221_002` smallint unsigned NOT NULL DEFAULT '0',
  `Field_11_1_0_58221_005` tinyint NOT NULL DEFAULT '0',
  `Field_11_1_0_58221_003_0` int NOT NULL DEFAULT '0',
  `Field_11_1_0_58221_003_1` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `warband_scene_placement_option`;
CREATE TABLE `warband_scene_placement_option` (
  `PositionX` float NOT NULL DEFAULT '0',
  `PositionY` float NOT NULL DEFAULT '0',
  `PositionZ` float NOT NULL DEFAULT '0',
  `ID` int unsigned NOT NULL DEFAULT '0',
  `WarbandScenePlacementID` int unsigned NOT NULL DEFAULT '0',
  `Orientation` float NOT NULL DEFAULT '0',
  `Scale` float NOT NULL DEFAULT '0',
  `Field_11_1_0_58221_005` int NOT NULL DEFAULT '0',
  `Field_11_1_0_58221_006` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `warband_scene_plcmnt_anim_override`;
CREATE TABLE `warband_scene_plcmnt_anim_override` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `Field_11_0_0_54210_000` int NOT NULL DEFAULT '0',
  `WarbandSceneAnimationID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `warband_placement_display_info`;
CREATE TABLE `warband_placement_display_info` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `WarbandScenePlacementID` int unsigned NOT NULL DEFAULT '0',
  `Field_11_2_0_61476_001` int NOT NULL DEFAULT '0',
  `Field_11_2_0_61476_002` int NOT NULL DEFAULT '0',
  `Field_11_2_0_61476_003` int NOT NULL DEFAULT '0',
  `Field_11_2_0_61476_004` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `warband_scene_source_info`;
CREATE TABLE `warband_scene_source_info` (
  `SourceDescription` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `ID` int unsigned NOT NULL DEFAULT '0',
  `WarbandSceneID` int unsigned NOT NULL DEFAULT '0',
  `SourceType` tinyint NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `warband_scene_source_info_locale`;
CREATE TABLE `warband_scene_source_info_locale` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `locale` varchar(4) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `SourceDescription_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`locale`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
