-- Fix invalid faction which is NOT allowed
UPDATE `creature_template` SET `faction` = 35 WHERE `faction` = 0;