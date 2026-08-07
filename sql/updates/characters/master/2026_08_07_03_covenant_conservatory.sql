--
-- Queen's Conservatory (Night Fae unique covenant sanctum feature, GarrTalentTree 319).
-- One row per OCCUPIED wildseed plot; empty plots are not stored. plotId is 0-based and is bounded by the
-- number of researched tiers of tree 319 (max 5 - GarrTalentTree 319 MaxTiers). state: 0 empty, 1 growing,
-- 2 ready to harvest. catalyst1..4 hold GameObject entries (353652 Catalyst of Power / 353653 Catalyst of
-- Renewal / 353654 Catalyst of Might); talent 1090 "Final Forms" is what caps the links at four.
-- Idempotent.
--
CREATE TABLE IF NOT EXISTS `character_garrison_conservatory` (
  `guid`          BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Character GUID',
  `plotId`        TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0-based wildseed plot index',
  `wildseedEntry` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'garrison_conservatory_wildseed.wildseedEntry',
  `plantedTime`   BIGINT NOT NULL DEFAULT 0,
  `maturesAt`     BIGINT NOT NULL DEFAULT 0 COMMENT 'unix time the wildseed becomes harvestable',
  `catalyst1`     INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'gameobject_template.entry of the attached catalyst',
  `catalyst2`     INT UNSIGNED NOT NULL DEFAULT 0,
  `catalyst3`     INT UNSIGNED NOT NULL DEFAULT 0,
  `catalyst4`     INT UNSIGNED NOT NULL DEFAULT 0,
  `state`         TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 empty, 1 growing, 2 ready',
  PRIMARY KEY (`guid`, `plotId`),
  KEY `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Queen''s Conservatory wildseed plots';
