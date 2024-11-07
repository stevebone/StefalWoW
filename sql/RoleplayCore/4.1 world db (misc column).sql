-- gameobject
ALTER TABLE `gameobject`ADD COLUMN `size` FLOAT NOT NULL DEFAULT '-1';
ALTER TABLE `gameobject`ADD COLUMN `visibility` FLOAT NOT NULL DEFAULT '256';

-- creature
ALTER TABLE `creature` ADD COLUMN `size` FLOAT NOT NULL DEFAULT '-1';

-- npc_vendor
ALTER TABLE `npc_vendor` ADD COLUMN `OverrideGoldCost` INT NOT NULL DEFAULT '-1';