-- Deadmines Dungeon
-- Old version before Cataclysm

-- NPC: 598 Defias Miner
-- NPC: 634 Defias Overseer
-- NPC: 641 Defias Woodcarver
-- NPC: 642 Sneeds Shredder
-- NPC: 644 RhahkZor
-- NPC: 1725 Defias Watchman
-- NPC: 1729 Defias Evoker
-- NPC: 4416 Defias Strip Miner

UPDATE `instance_template` SET `script` = 'custom_instance_deadmines' WHERE (`map` = '36');

UPDATE `creature_template` SET `ScriptName` = 'boss_rhahkzor' WHERE `entry` = 644;

DELETE FROM `creature_template_difficulty` WHERE `entry` IN (598,634,641,642,644,1725,1729,4416);
INSERT INTO `creature_template_difficulty` VALUES
(598, 1, 0, 0, 2872, 0, 1, 1, 1, 1.7, 409, 2097224, 0, 0, 598, 598, 0, 7, 54, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Miner
(634, 1, 0, 0, 2872, 0, 6, 1, 1, 1.7, 439, 2097224, 0, 0, 634, 634, 0, 26, 158, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Overseer
(641, 1, 0, 0, 2872, 0, 5, 1, 1, 1.7, 443, 2097224, 0, 0, 641, 641, 0, 32, 161, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Woodcarver
(642, 1, 0, 0, 2872, 0, 25, 1, 1, 1.7, 444, 2097256, 128, 0, 642, 0, 0, 159, 262, 0, 0, 0, 0, 0, 0, 0, 0, -1), -- Sneeds Shredder
(644, 1, 0, 0, 2872, 0, 25, 1, 1, 1.7, 446, 2097256, 128, 0, 644, 644, 0, 108, 298, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- RhahkZor
(1725, 1, 0, 0, 2872, 0, 1, 1, 1, 1.7, 1416, 0, 0, 0, 1725, 1725, 0, 34, 103, 268959744, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Watchman
(1729, 1, 0, 0, 2872, 0, 4.6, 0.7264, 1, 1.7, 1419, 2097224, 0, 0, 1729, 1729, 0, 23, 156, 268959744, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Evoker
(4416, 1, 0, 0, 2872, 0, 4, 1, 1, 1.7, 3873, 2097224, 0, 0, 4416, 4416, 0, 8, 56, 524288, 0, 0, 0, 0, 0, 0, 0, -1); -- Defias Strip Miner

