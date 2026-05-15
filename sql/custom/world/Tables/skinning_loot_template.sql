-- Skinning loot fix
DELETE FROM `skinning_loot_template` WHERE `Entry` = 1;
INSERT INTO `skinning_loot_template` VALUES
('1', '0', '2318', '70', '0', '1', '1', '1', '3', 'Starting Zones'),
('1', '0', '2934', '75', '0', '1', '1', '1', '4', 'Starting Zones');