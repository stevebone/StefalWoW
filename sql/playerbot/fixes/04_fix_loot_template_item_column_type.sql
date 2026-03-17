-- =============================================================================
-- Fix: Change `item` column from BIGINT to INT UNSIGNED in all loot_template tables
--
-- Problem: Database was imported with `item` columns as BIGINT (LONGLONG)
--          but TrinityCore C++ code expects INT (32-bit) for item entries.
--          This causes: "Field::GetUInt32 on LONGLONG field .item at index 0"
--
-- Affected tables (all *_loot_template tables with an `item` column):
--   - creature_loot_template
--   - disenchant_loot_template
--   - fishing_loot_template
--   - gameobject_loot_template
--   - item_loot_template
--   - milling_loot_template
--   - pickpocketing_loot_template
--   - prospecting_loot_template
--   - reference_loot_template
--   - skinning_loot_template
--   - spell_loot_template
-- =============================================================================

-- Alter creature_loot_template
ALTER TABLE `creature_loot_template`
MODIFY COLUMN `Item` INT UNSIGNED NOT NULL DEFAULT 0;

-- Alter disenchant_loot_template
ALTER TABLE `disenchant_loot_template`
MODIFY COLUMN `Item` INT UNSIGNED NOT NULL DEFAULT 0;

-- Alter fishing_loot_template
ALTER TABLE `fishing_loot_template`
MODIFY COLUMN `Item` INT UNSIGNED NOT NULL DEFAULT 0;

-- Alter gameobject_loot_template
ALTER TABLE `gameobject_loot_template`
MODIFY COLUMN `Item` INT UNSIGNED NOT NULL DEFAULT 0;

-- Alter item_loot_template
ALTER TABLE `item_loot_template`
MODIFY COLUMN `Item` INT UNSIGNED NOT NULL DEFAULT 0;

-- Alter milling_loot_template
ALTER TABLE `milling_loot_template`
MODIFY COLUMN `Item` INT UNSIGNED NOT NULL DEFAULT 0;

-- Alter pickpocketing_loot_template
ALTER TABLE `pickpocketing_loot_template`
MODIFY COLUMN `Item` INT UNSIGNED NOT NULL DEFAULT 0;

-- Alter prospecting_loot_template
ALTER TABLE `prospecting_loot_template`
MODIFY COLUMN `Item` INT UNSIGNED NOT NULL DEFAULT 0;

-- Alter reference_loot_template
ALTER TABLE `reference_loot_template`
MODIFY COLUMN `Item` INT UNSIGNED NOT NULL DEFAULT 0;

-- Alter skinning_loot_template
ALTER TABLE `skinning_loot_template`
MODIFY COLUMN `Item` INT UNSIGNED NOT NULL DEFAULT 0;

-- Alter spell_loot_template
ALTER TABLE `spell_loot_template`
MODIFY COLUMN `Item` INT UNSIGNED NOT NULL DEFAULT 0;

-- Also check mail_loot_template if it exists
-- (Not all TrinityCore versions have this table)
ALTER TABLE `mail_loot_template`
MODIFY COLUMN `Item` INT UNSIGNED NOT NULL DEFAULT 0;
