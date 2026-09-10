--
-- Trophy.db2 hotfix mirror (garrison monument trophies).
--
-- Trophy.db2 (FileDataId 975024, layout 0xA17123C5) has shipped in every build for years and TrinityCore
-- already carried correct metadata for it (`TrophyMeta` in DB2Metadata.h) and already extracted it
-- (DBFilesClientList.h), but no store ever loaded it - there was no TrophyEntry, no TrophyLoadInfo, no
-- sTrophyStore and no HOTFIX_SEL_TROPHY. sTrophyStore is added in this branch, which requires these two
-- mirror tables to exist: HotfixDatabase prepares its statements against them unconditionally at startup,
-- so worldserver will ABORT ON BOOT if this file has not been applied.
--
-- The tables ship EMPTY on purpose. That is the normal state for a hotfix mirror: the 16 real rows are read
-- from the client's own Trophy.db2 (M:/WorldofWarcraft/dbc/enUS/Trophy.db2, 779 bytes, 16 records), and
-- these tables exist only so a row can be overridden or added later without a client patch.
--
-- Column order and types are taken from the layout hash DB2Metadata.h already pins for 12.0.7.68275
-- (0xA17123C5), cross-checked against the WoWDBDefs definition for that same layout, which lists it as
--     $noninline,id$ID<32> / Name_lang / TrophyTypeID<u8> / GameObjectDisplayInfoID<32> / PlayerConditionID<u32>
-- and against the file's own field-storage info (TrophyTypeID is 3 bits, GameObjectDisplayInfoID 16 signed
-- bits, PlayerConditionID a 4-bit index into a 9-entry pallet).
--
-- Idempotent.
--

DROP TABLE IF EXISTS `trophy`;
CREATE TABLE `trophy` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Name` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `TrophyTypeID` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `GameObjectDisplayInfoID` INT NOT NULL DEFAULT '0',
  `PlayerConditionID` INT UNSIGNED NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `trophy_locale`;
CREATE TABLE `trophy_locale` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `locale` VARCHAR(4) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Name_lang` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `VerifiedBuild` INT NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`,`locale`,`VerifiedBuild`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- No INSERTs. The client file is the source of the 16 rows; adding one here would only be to override the
-- client, which nothing in this branch needs.
