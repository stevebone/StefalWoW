-- Skinning loot fix
DELETE FROM `skinning_loot_template` WHERE `Entry` IN (1,2,3);
INSERT INTO `skinning_loot_template` VALUES
(1, 0, 2318, 70, 0, 1, 1, 1, 3, 'Starting Zones'),
(1, 0, 2934, 75, 0, 1, 1, 1, 4, 'Starting Zones'),

(2, 0, 783, 2, 0, 1, 1, 1, 6, 'Zones 2'), -- Light Hide
(2, 0, 2318, 50, 0, 1, 1, 1, 7, 'Zones 2'), -- Light Leather
(2, 0, 2934, 50, 0, 1, 1, 1, 6, 'Zones 2'), -- Ruined Leather Scraps

(3, 0, 783, 4, 0, 1, 1, 1, 5, 'Zones 3'), -- Light Hide
(3, 0, 2318, 60, 0, 1, 1, 1, 7, 'Zones 3'), -- Light Leather
(3, 0, 2934, 5, 0, 1, 1, 1, 2, 'Zones 3'), -- Ruined Leather Scraps
(3, 0, 2319, 35, 0, 1, 1, 1, 6, 'Zones 3'), -- Medium Leather
(3, 0, 4232, 3, 0, 1, 1, 1, 5, 'Zones 3'); -- Medium Hide
