-- Skinning loot fix
DELETE FROM `skinning_loot_template` WHERE `Entry` IN (1,2);
INSERT INTO `skinning_loot_template` VALUES
('1', '0', '2318', '70', '0', '1', '1', '1', '3', 'Starting Zones'),
('1', '0', '2934', '75', '0', '1', '1', '1', '4', 'Starting Zones'),

('2', '0', '783', '2', '0', '1', '1', '1', '6', 'Zones 2'), -- Light Hide
('2', '0', '2318', '50', '0', '1', '1', '1', '7', 'Zones 2'), -- Light Leather
('2', '0', '2934', '50', '0', '1', '1', '1', '6', 'Zones 2'); -- Ruined Leather Scraps