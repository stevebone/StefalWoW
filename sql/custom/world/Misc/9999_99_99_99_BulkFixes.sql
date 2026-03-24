-- Fix invalid faction which is NOT allowed
UPDATE `creature_template` SET `faction` = 35 WHERE `faction` = 0;

-- Loots that are 0.00000001 and the likes
UPDATE `creature_loot_template` SET `Chance` = 1 WHERE `Chance` < 1;