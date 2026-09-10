ALTER TABLE `character_garrison_followers`
  ADD COLUMN `durability` int unsigned NOT NULL DEFAULT '0' AFTER `status`;
