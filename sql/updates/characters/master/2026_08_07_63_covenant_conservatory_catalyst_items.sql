--
-- Queen's Conservatory - the catalyst columns hold ITEM ids, not GameObject entries.
--
-- 2026_08_07_03 created `character_garrison_conservatory` with catalyst1..4 documented as
-- "gameobject_template.entry of the attached catalyst", naming GameObjects 353652 / 353653 / 353654
-- ("Catalyst of Power / Renewal / Might"). Those are Revendreth vampire-bottle props in AreaTable 10413 on
-- map 2222, not Conservatory objects - see the header of the world migration
-- 2026_08_07_63_covenant_conservatory_catalysts.sql for the full derivation.
--
-- The real catalysts are the items 176921 Temporal Leaves / 176922 Wild Nightbloom / 176832 Wildseed Root
-- Grain, so these columns now carry `garrison_conservatory_catalyst`.catalystItemId. This file only
-- re-documents the columns and clears any value written under the old (no-op) meaning; the engine also drops
-- unknown catalyst ids at load time with a log line, so applying this is belt-and-braces.
--
-- Idempotent.
--

-- Safety net if this file is applied without 2026_08_07_03 (identical definition, corrected comments).
CREATE TABLE IF NOT EXISTS `character_garrison_conservatory` (
  `guid`          BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Character GUID',
  `plotId`        TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0-based wildseed plot index',
  `wildseedEntry` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'garrison_conservatory_wildseed.wildseedEntry',
  `plantedTime`   BIGINT NOT NULL DEFAULT 0,
  `maturesAt`     BIGINT NOT NULL DEFAULT 0 COMMENT 'unix time the wildseed becomes harvestable',
  `catalyst1`     INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'garrison_conservatory_catalyst.catalystItemId of the attached catalyst',
  `catalyst2`     INT UNSIGNED NOT NULL DEFAULT 0,
  `catalyst3`     INT UNSIGNED NOT NULL DEFAULT 0,
  `catalyst4`     INT UNSIGNED NOT NULL DEFAULT 0,
  `state`         TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 empty, 1 growing, 2 ready',
  PRIMARY KEY (`guid`, `plotId`),
  KEY `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Queens Conservatory wildseed plots';

ALTER TABLE `character_garrison_conservatory`
  MODIFY `catalyst1` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'garrison_conservatory_catalyst.catalystItemId of the attached catalyst',
  MODIFY `catalyst2` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'garrison_conservatory_catalyst.catalystItemId of the attached catalyst',
  MODIFY `catalyst3` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'garrison_conservatory_catalyst.catalystItemId of the attached catalyst',
  MODIFY `catalyst4` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'garrison_conservatory_catalyst.catalystItemId of the attached catalyst';

-- The only values the old code could ever have written are those three GameObject entries (it validated
-- against gameobject_template), and they mean nothing under the new meaning. Nothing else is touched.
UPDATE `character_garrison_conservatory` SET `catalyst1` = 0 WHERE `catalyst1` IN (353652,353653,353654);
UPDATE `character_garrison_conservatory` SET `catalyst2` = 0 WHERE `catalyst2` IN (353652,353653,353654);
UPDATE `character_garrison_conservatory` SET `catalyst3` = 0 WHERE `catalyst3` IN (353652,353653,353654);
UPDATE `character_garrison_conservatory` SET `catalyst4` = 0 WHERE `catalyst4` IN (353652,353653,353654);
