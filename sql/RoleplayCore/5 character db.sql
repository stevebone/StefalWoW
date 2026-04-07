-- Favorites
ALTER TABLE `character_pet` ADD COLUMN `favorite` tinyint unsigned NOT NULL DEFAULT '0' AFTER `specialization`;

-- Chromie Time Expansion
ALTER TABLE `characters` ADD COLUMN `chromieTimeExpansionId` tinyint unsigned NOT NULL DEFAULT '0' AFTER `transmogOutfitLocked`;