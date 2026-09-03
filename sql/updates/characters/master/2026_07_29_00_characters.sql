-- WoD Garrison resource cache: per-garrison last-collection timestamp (accrual base).
ALTER TABLE `character_garrison` ADD COLUMN IF NOT EXISTS `cacheLastUsed` BIGINT NOT NULL DEFAULT 0 AFTER `lastMissionStartDay`;
