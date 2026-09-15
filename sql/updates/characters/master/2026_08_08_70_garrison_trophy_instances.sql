--
-- Garrison monument trophies: re-key the saved selection by TrophyInstanceID.
--
-- `character_garrison_trophies` was (guid, trophyId, garrType) with PRIMARY KEY (guid, trophyId) - a SET of
-- trophy ids per character. That cannot express what the feature actually stores.
--
-- A garrison has three monuments, not one. Each Monument Base gameobject carries a TrophyInstanceID in its
-- Data1 (the six spawned ones use 1, 2 and 6 on each side), and the client resolves what a given monument is
-- displaying by scanning SMSG_GARRISON_UPDATE_GARRISON_MONUMENT_SELECTIONS for the entry whose first field
-- equals that monument's own TrophyInstanceID - the monument tooltip builder at client RVA 0x1CAED30 does
-- exactly this lookup and then reads Trophy.db2 for the name. The saved state is therefore a MAP from
-- monument instance to trophy, one row per physical monument, and a plain set of trophy ids would make all
-- three plinths show the same statue with no way to say which is which.
--
-- Recreating rather than ALTERing: the old PRIMARY KEY (guid, trophyId) is wrong for the new shape (the same
-- trophy may legitimately sit on two different monuments, while one monument may hold only one trophy), and
-- the table holds no rows to preserve - nothing in the core ever wrote to it. Garrison::AddTrophy had no
-- caller outside the packet handlers, and those only ran on opcodes that the client never sent, because
-- nothing opened the monument interaction in the first place. Verified empty on this realm before writing
-- this file (SELECT COUNT(*) FROM character_garrison_trophies -> 0).
--
-- Columns:
--   guid              character
--   trophyInstanceId  which physical monument (gameobject Data1 of a GAMEOBJECT_TYPE_GARRISON_MONUMENT)
--   trophyId          Trophy.db2 row displayed on it
--   garrType          owning garrison type (2 = the WoD garrison, the only type that has monuments)
--
-- Idempotent.
--

DROP TABLE IF EXISTS `character_garrison_trophies`;
CREATE TABLE `character_garrison_trophies` (
  `guid`             BIGINT UNSIGNED NOT NULL,
  `trophyInstanceId` INT UNSIGNED NOT NULL DEFAULT 0,
  `trophyId`         INT UNSIGNED NOT NULL DEFAULT 0,
  `garrType`         TINYINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`,`garrType`,`trophyInstanceId`),
  KEY `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
