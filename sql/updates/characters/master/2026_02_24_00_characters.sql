ALTER TABLE `character_garrison_buildings`
  ADD COLUMN `currentGarSpecId` int unsigned NOT NULL DEFAULT '0' AFTER `active`,
  ADD COLUMN `timeSpecCooldown` bigint NOT NULL DEFAULT '0' AFTER `currentGarSpecId`;

CREATE TABLE IF NOT EXISTS `character_garrison_specializations` (
  `guid` bigint unsigned NOT NULL,
  `specId` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`specId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
