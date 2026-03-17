-- =============================================================================
-- Revert: Change `Item` column back to BIGINT in all loot_template tables
--
-- Use this if you applied 04_fix_loot_template_item_column_type.sql and need
-- to revert back to BIGINT (which your imported database uses).
--
-- The C++ code has been fixed to use GetUInt64() instead, so BIGINT is now OK.
-- =============================================================================

-- Alter creature_loot_template
ALTER TABLE `creature_loot_template`
MODIFY COLUMN `Item` BIGINT NOT NULL DEFAULT 0;

-- Alter disenchant_loot_template
ALTER TABLE `disenchant_loot_template`
MODIFY COLUMN `Item` BIGINT NOT NULL DEFAULT 0;

-- Alter fishing_loot_template
ALTER TABLE `fishing_loot_template`
MODIFY COLUMN `Item` BIGINT NOT NULL DEFAULT 0;

-- Alter gameobject_loot_template
ALTER TABLE `gameobject_loot_template`
MODIFY COLUMN `Item` BIGINT NOT NULL DEFAULT 0;

-- Alter item_loot_template
ALTER TABLE `item_loot_template`
MODIFY COLUMN `Item` BIGINT NOT NULL DEFAULT 0;

-- Alter milling_loot_template
ALTER TABLE `milling_loot_template`
MODIFY COLUMN `Item` BIGINT NOT NULL DEFAULT 0;

-- Alter pickpocketing_loot_template
ALTER TABLE `pickpocketing_loot_template`
MODIFY COLUMN `Item` BIGINT NOT NULL DEFAULT 0;

-- Alter prospecting_loot_template
ALTER TABLE `prospecting_loot_template`
MODIFY COLUMN `Item` BIGINT NOT NULL DEFAULT 0;

-- Alter reference_loot_template
ALTER TABLE `reference_loot_template`
MODIFY COLUMN `Item` BIGINT NOT NULL DEFAULT 0;

-- Alter skinning_loot_template
ALTER TABLE `skinning_loot_template`
MODIFY COLUMN `Item` BIGINT NOT NULL DEFAULT 0;

-- Alter spell_loot_template
ALTER TABLE `spell_loot_template`
MODIFY COLUMN `Item` BIGINT NOT NULL DEFAULT 0;

-- Alter mail_loot_template if it exists
ALTER TABLE `mail_loot_template`
MODIFY COLUMN `Item` BIGINT NOT NULL DEFAULT 0;
