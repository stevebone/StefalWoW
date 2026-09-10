-- G4: allow a character to persist MULTIPLE garrisons (WoD garrison + Legion order hall + BfA war campaign + covenant).
-- Adds garrType scoping to every garrison sub-table + widens the character_garrison PK. Backfill is safe because
-- today every character has exactly one garrison (PK guid), so all its sub-rows belong to that garrison's type.

-- 1. Allow >1 garrison row per character.
ALTER TABLE `character_garrison` DROP PRIMARY KEY, ADD PRIMARY KEY (`guid`, `type`);

-- 2. Add garrType to each sub-table (archived_missions already has it).
ALTER TABLE `character_garrison_blueprints`      ADD COLUMN `garrType` tinyint unsigned NOT NULL DEFAULT 0;
ALTER TABLE `character_garrison_buildings`       ADD COLUMN `garrType` tinyint unsigned NOT NULL DEFAULT 0;
ALTER TABLE `character_garrison_specializations` ADD COLUMN `garrType` tinyint unsigned NOT NULL DEFAULT 0;
ALTER TABLE `character_garrison_followers`       ADD COLUMN `garrType` tinyint unsigned NOT NULL DEFAULT 0;
ALTER TABLE `character_garrison_missions`        ADD COLUMN `garrType` tinyint unsigned NOT NULL DEFAULT 0;
ALTER TABLE `character_garrison_shipments`       ADD COLUMN `garrType` tinyint unsigned NOT NULL DEFAULT 0;
ALTER TABLE `character_garrison_talents`         ADD COLUMN `garrType` tinyint unsigned NOT NULL DEFAULT 0;
ALTER TABLE `character_garrison_trophies`        ADD COLUMN `garrType` tinyint unsigned NOT NULL DEFAULT 0;

-- 3. Backfill: every existing sub-row belongs to that character's single garrison.
UPDATE `character_garrison_blueprints`      s JOIN `character_garrison` g ON g.`guid`=s.`guid` SET s.`garrType`=g.`type`;
UPDATE `character_garrison_buildings`       s JOIN `character_garrison` g ON g.`guid`=s.`guid` SET s.`garrType`=g.`type`;
UPDATE `character_garrison_specializations` s JOIN `character_garrison` g ON g.`guid`=s.`guid` SET s.`garrType`=g.`type`;
UPDATE `character_garrison_followers`       s JOIN `character_garrison` g ON g.`guid`=s.`guid` SET s.`garrType`=g.`type`;
UPDATE `character_garrison_missions`        s JOIN `character_garrison` g ON g.`guid`=s.`guid` SET s.`garrType`=g.`type`;
UPDATE `character_garrison_shipments`       s JOIN `character_garrison` g ON g.`guid`=s.`guid` SET s.`garrType`=g.`type`;
UPDATE `character_garrison_talents`         s JOIN `character_garrison` g ON g.`guid`=s.`guid` SET s.`garrType`=g.`type`;
UPDATE `character_garrison_trophies`        s JOIN `character_garrison` g ON g.`guid`=s.`guid` SET s.`garrType`=g.`type`;
