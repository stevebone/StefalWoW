--
-- Phase 10A.1 - Covenant.db2
-- Required as FK target for RenownRewards.db2 (RenownRewards.CovenantID -> Covenant.ID).
-- Covenant.FactionID maps to the Major Faction's Faction.ID; Covenant.CurrencyTypesID
-- maps to the renown currency (mirror of Faction.RenownCurrencyID).
--
DROP TABLE IF EXISTS `covenant`;
CREATE TABLE `covenant` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `Name` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Description` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `BountySetID` int NOT NULL DEFAULT '0',
  `SkillLineID` int NOT NULL DEFAULT '0',
  `DeathTeleportSpellID` int NOT NULL DEFAULT '0',
  `Unknown902_6` int NOT NULL DEFAULT '0',
  `Unknown902_7` int NOT NULL DEFAULT '0',
  `FactionID` int NOT NULL DEFAULT '0',
  `CurrencyTypesID` int NOT NULL DEFAULT '0',
  `RequiredPlayerConditionID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `covenant_locale`;
CREATE TABLE `covenant_locale` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `locale` varchar(4) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Name_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Description_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`locale`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

--
-- Phase 10A.2 - RenownRewards.db2 + RenownRewardsPlunderstorm.db2
-- The reward track per Major Faction. RenownRewards.CovenantID joins to
-- Covenant.ID (added in 10A.1); Covenant.FactionID resolves back to the
-- Faction.ID of the Major Faction owning this reward.
--
DROP TABLE IF EXISTS `renown_rewards`;
CREATE TABLE `renown_rewards` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `Name` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Description` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `ToastDescription` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
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
  `RewardCategory` int NOT NULL DEFAULT '0',
  `QuestID` int NOT NULL DEFAULT '0',
  `PlayerConditionID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `renown_rewards_locale`;
CREATE TABLE `renown_rewards_locale` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `locale` varchar(4) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Name_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Description_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `ToastDescription_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`locale`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `renown_rewards_plunderstorm`;
CREATE TABLE `renown_rewards_plunderstorm` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `Name` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Description` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `CovenantID` int NOT NULL DEFAULT '0',
  `Level` int NOT NULL DEFAULT '0',
  `Icon` int NOT NULL DEFAULT '0',
  `RewardCategory` int NOT NULL DEFAULT '0',
  `UiOrder` int NOT NULL DEFAULT '0',
  `SpellID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `renown_rewards_plunderstorm_locale`;
CREATE TABLE `renown_rewards_plunderstorm_locale` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `locale` varchar(4) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Name_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Description_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`locale`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

--
-- Phase 10A.4 - FactionGroup.db2 + CurrencyCategory.db2
-- FactionGroup (4 rows: Player/Alliance/Horde/Monster) provides the bitmask
-- values referenced by FactionTemplate.FactionGroup/FriendGroup/EnemyGroup
-- and the per-side honor/conquest currency texture file IDs.
-- CurrencyCategory (35 rows) provides parent categories for currency UI
-- grouping, including category 142 (Renown) used by Major Faction
-- renown currencies.
--
DROP TABLE IF EXISTS `faction_group`;
CREATE TABLE `faction_group` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `InternalName` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Name` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `MaskID` tinyint unsigned NOT NULL DEFAULT '0',
  `HonorCurrencyTextureFileID` int NOT NULL DEFAULT '0',
  `ConquestCurrencyTextureFileID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `faction_group_locale`;
CREATE TABLE `faction_group_locale` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `locale` varchar(4) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Name_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`locale`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `currency_category`;
CREATE TABLE `currency_category` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `Name` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `Flags` int NOT NULL DEFAULT '0',
  `ExpansionID` tinyint unsigned NOT NULL DEFAULT '0',
  `ParentCategoryID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `currency_category_locale`;
CREATE TABLE `currency_category_locale` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `locale` varchar(4) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Name_lang` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`locale`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

--
-- Phase 10L - UiTextureKit.db2
-- Resolves Campaign.UiTextureKitID -> KitPrefix string. Used by the
-- Major Faction renown UI to compute textureKit suffixes (atlas member
-- names like "MajorFaction-DragonscaleExpedition-Background"). Loaded
-- so MajorFactionMgr::GetTextureKitPrefix(factionId) can walk the chain
-- faction -> renown campaign -> Campaign.UiTextureKitID -> UiTextureKit.KitPrefix.
--
DROP TABLE IF EXISTS `ui_texture_kit`;
CREATE TABLE `ui_texture_kit` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `KitPrefix` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;