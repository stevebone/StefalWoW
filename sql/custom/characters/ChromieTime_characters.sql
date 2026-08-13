ALTER TABLE `characters`
  ADD COLUMN `chromieTimeExpansionId` tinyint unsigned NOT NULL DEFAULT '0' AFTER `lastLoginBuild`;

ALTER TABLE `characters`
  ADD COLUMN `timerunningSeasonId` int unsigned NOT NULL DEFAULT '0' AFTER `chromieTimeExpansionId`;

