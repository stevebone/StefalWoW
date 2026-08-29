
ALTER TABLE `item_template_addon`
ADD COLUMN `ScrappingLootId` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `MaxMoneyLoot`;