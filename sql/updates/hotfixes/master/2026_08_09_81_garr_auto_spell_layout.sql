--
-- Shadowlands Adventures: correct the hotfix column layout of the two remaining auto-combat DB2
-- mirrors. Companion piece to 2026_08_07_00_garr_autocombat_layout.sql, which fixed
-- garr_auto_combatant / garr_encounter / garr_mission_x_encounter but left these two alone.
--
-- GarrAutoSpell.db2 (layout 0x8067D16A) and GarrAutoSpellEffect.db2 (layout 0xACEA7666) as shipped by
-- client 12.0.7.68275 do not have the columns these tables were created with. The names below are
-- taken from the WoWDBDefs definition for the layout hash that DB2Metadata.h already pins for this
-- build. As before the old names were positionally aligned but semantically wrong:
--   garr_auto_spell         ... Cooldown/Duration/SchoolMask/SpellVisualID/Flags
--                            -> ... Cooldown/Duration/Flags/SchoolMask/IconFileDataID
--                               (so SchoolMask was being read out of Flags, and the Adventures replay
--                                picked its spell visual from a mask that was always 0 or 1)
--   garr_auto_spell_effect  ID/GarrAutoSpellID/EffectType/Targets/Amount/MiscType/MiscValue/Period
--                            -> ID/GarrAutoSpellID/EffectIndex/Effect/Points/TargetType/Flags/Period
--                               (one position off from column 2 on: the simulator was switching on
--                                the row's EffectIndex as if it were the effect kind, and selecting
--                                targets with the effect kind as if it were the target mask)
--
-- Both are hotfix mirrors that hold no rows on this realm (the data is read from the client DB2
-- files), so recreating them loses nothing. garr_auto_spell_locale is unchanged and left alone.
--

DROP TABLE IF EXISTS `garr_auto_spell`;
CREATE TABLE `garr_auto_spell` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `Name` TEXT DEFAULT NULL,
  `Description` TEXT DEFAULT NULL,
  `Cooldown` INT NOT NULL DEFAULT '0',
  `Duration` INT NOT NULL DEFAULT '0',
  `Flags` INT NOT NULL DEFAULT '0',
  `SchoolMask` INT NOT NULL DEFAULT '0',
  `IconFileDataID` INT NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `garr_auto_spell_effect`;
CREATE TABLE `garr_auto_spell_effect` (
  `ID` INT UNSIGNED NOT NULL DEFAULT '0',
  `GarrAutoSpellID` INT UNSIGNED NOT NULL DEFAULT '0',
  `EffectIndex` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `Effect` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `Points` FLOAT NOT NULL DEFAULT '0',
  `TargetType` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `Flags` INT NOT NULL DEFAULT '0',
  `Period` INT NOT NULL DEFAULT '0',
  `VerifiedBuild` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
