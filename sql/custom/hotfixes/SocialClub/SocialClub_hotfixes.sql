-- ============================================================================
-- SocialClub system - hotfixes database tables
-- Ported from RolePlay_Master (Club finder part 1)
--
-- These tables are OVERRIDE sources for DB2Storage: the base rows load from the
-- client db2 files (ChatProfanity.db2, GuildTabard*.db2); rows here only need to
-- exist for server-side custom records. Structure matches retail dumps.
-- ============================================================================

-- Table structure for table `chat_profanity`

DROP TABLE IF EXISTS `chat_profanity`;
CREATE TABLE `chat_profanity` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `Text` text,
  `Language` tinyint NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Table structure for table `chat_profanity_locale`

DROP TABLE IF EXISTS `chat_profanity_locale`;
CREATE TABLE `chat_profanity_locale` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `locale` varchar(4) NOT NULL,
  `Text_lang` text,
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`locale`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
/*!50500 PARTITION BY LIST  COLUMNS(locale)
(PARTITION deDE VALUES IN ('deDE') ENGINE = InnoDB,
 PARTITION esES VALUES IN ('esES') ENGINE = InnoDB,
 PARTITION esMX VALUES IN ('esMX') ENGINE = InnoDB,
 PARTITION frFR VALUES IN ('frFR') ENGINE = InnoDB,
 PARTITION itIT VALUES IN ('itIT') ENGINE = InnoDB,
 PARTITION koKR VALUES IN ('koKR') ENGINE = InnoDB,
 PARTITION ptBR VALUES IN ('ptBR') ENGINE = InnoDB,
 PARTITION ruRU VALUES IN ('ruRU') ENGINE = InnoDB,
 PARTITION zhCN VALUES IN ('zhCN') ENGINE = InnoDB,
 PARTITION zhTW VALUES IN ('zhTW') ENGINE = InnoDB) */;

-- Table structure for table `guild_tabard_emblem`

DROP TABLE IF EXISTS `guild_tabard_emblem`;
CREATE TABLE `guild_tabard_emblem` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `Component` int NOT NULL DEFAULT '0',
  `Color` int NOT NULL DEFAULT '0',
  `FileDataID` int NOT NULL DEFAULT '0',
  `EmblemID` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Table structure for table `guild_tabard_background`

DROP TABLE IF EXISTS `guild_tabard_background`;
CREATE TABLE `guild_tabard_background` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `Tier` int NOT NULL DEFAULT '0',
  `Component` int NOT NULL DEFAULT '0',
  `FileDataID` int NOT NULL DEFAULT '0',
  `Color` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Table structure for table `guild_tabard_border`

DROP TABLE IF EXISTS `guild_tabard_border`;
CREATE TABLE `guild_tabard_border` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `BorderID` int NOT NULL DEFAULT '0',
  `Tier` int NOT NULL DEFAULT '0',
  `Component` int NOT NULL DEFAULT '0',
  `FileDataID` int NOT NULL DEFAULT '0',
  `Color` int NOT NULL DEFAULT '0',
  `VerifiedBuild` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
