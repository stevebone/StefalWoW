--
-- Path of Ascension (Kyrian unique covenant sanctum feature, GarrTalentTree 320).
--
-- One row per Shadowlands memory the character has captured, plus how far that memory has been taken.
--
-- `memoryId` references `garrison_ascension_memory`.`memoryId` in the world DB (authored content - see
-- sql/updates/world/master/2026_08_07_80_covenant_ascension_memories.sql for why that roster is not derived).
-- A stored row whose memoryId no longer has a world row is dropped with an error at load
-- (PathOfAscension::LoadFromDB) rather than carried as an id no gate can evaluate.
--
-- `highestTrialWon` is the high-water mark over the four trials, which are Difficulty.db2 rows in the 68275
-- client, not an invented scale:
--     0 none, 1 Trial of Courage (Difficulty 168), 2 Trial of Loyalty (169),
--     3 Trial of Wisdom (170), 4 Trial of Humility (171).
-- All four are InstanceType 5 (MAP_SCENARIO) with MinPlayers = MaxPlayers = 1, bound to map 2375
-- "9.0 Bastion Arena - Path of Ascension" by MapDifficulty 4795-4798 - which is why this is stored per
-- character and not per group.
--
-- Only the covenant sanctum (GarrType 111) has one, so the table is keyed by guid alone and is written and
-- purged from Garrison::SaveToDB / Garrison::DeleteFromDB behind a GARRISON_TYPE_COVENANT check - the same
-- shape as `character_garrison_conservatory` and `character_garrison_abomination_factory`.
--
-- Captures are deliberately NOT deleted when the covenant changes or a talent is reset: the character keeps
-- what it captured and simply loses access until the tiers are re-researched (Garrison::Update clamps the
-- trial high-water mark to the ceiling the remaining research supports).
--
-- Idempotent.
--
CREATE TABLE IF NOT EXISTS `character_garrison_path_of_ascension` (
  `guid`              BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Character GUID',
  `memoryId`          INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'garrison_ascension_memory.memoryId',
  `capturedTime`      BIGINT NOT NULL DEFAULT 0 COMMENT 'unix time the memory was captured',
  `highestTrialWon`   TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 none, 1 Courage, 2 Loyalty, 3 Wisdom, 4 Humility',
  `lastCompletedTime` BIGINT NOT NULL DEFAULT 0 COMMENT 'unix time of the most recent trial win',
  PRIMARY KEY (`guid`, `memoryId`),
  KEY `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Path of Ascension captured memories and trial progress';
