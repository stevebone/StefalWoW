-- Elwynn Forest Changes and Updates

-- NPC: 448 Hogger
-- NPC: 46932 Hogger Minion
-- NPC: 46943 Ragamuffin (child)
-- NPC: 65153 General Hammond Clay

-- Creature Template Updates
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_hogger', `AIName` = '' WHERE `entry` IN (448);
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_hogger_minion' WHERE `entry` IN (46932);

-- Creature Text Updates
DELETE FROM `creature_text` WHERE `CreatureID` IN (448, 46943);
DELETE FROM `creature_text` WHERE `CreatureID` = 65153 AND `GroupID` IN (0, 11, 12, 13);
INSERT INTO `creature_text` VALUES
('448', '0', '0', 'More bones to gnaw on...', '12', '0', '100', '0', '0', '0', '0', '1871', '0', 'Hogger Agro Say 1'),
('448', '0', '1', 'Grrrr... fresh meat!', '12', '0', '100', '0', '0', '0', '0', '1870', '0', 'Hogger Agro Say 2'),
('448', '1', '0', 'Yipe!  Help Hogger!', '14', '0', '100', '5', '0', '0', '0', '46936', '0', 'Hogger'),
('448', '2', '0', 'Hogger is eating!  Stop him!', '41', '0', '100', '5', '0', '0', '0', '46939', '0', 'Hogger'),
('448', '3', '0', 'Hogger is stunned!', '41', '0', '100', '5', '0', '0', '0', '46940', '0', 'Hogger'),
('448', '4', '0', 'No hurt Hogger!', '14', '0', '100', '5', '0', '0', '0', '47000', '0', 'Hogger'),
('448', '5', '0', 'Grrr...', '12', '0', '100', '5', '0', '0', '0', '46937', '0', 'Hogger'),
('448', '6', '0', 'Nooooo...', '12', '0', '100', '5', '0', '0', '0', '49938', '0', 'Hogger'),
('46943', '0', '0', 'General Hammond Clay!', '12', '0', '100', '0', '0', '0', '0', '47005', '0', 'Child to hogger'),
('46943', '1', '0', 'Wow!', '12', '0', '100', '0', '0', '0', '0', '47006', '0', 'Child to hogger wow'),
('65153', '0', '0', 'Hold your blade', '14', '0', '100', '396', '0', '0', '0', '0', '255', 'General Hammond Hogger 1'),
('65153', '11', '0', 'This beast leads the Riverpaw gnoll gang and may be the key to ending gnoll aggression in Elwynn.', '12', '0', '100', '25', '0', '0', '0', '47007', '0', 'General Hammond Hogger 2'),
('65153', '12', '0', 'We\\\'re taking him into custody in the name of King Varian Wrynn.', '12', '0', '100', '396', '0', '0', '0', '47008', '0', 'General Hammond Hogger 3'),
('65153', '13', '0', 'Take us to the Stockades', '12', '0', '100', '396', '0', '0', '0', '0', '255', 'General Hammond Hogger 4');

-- Script Updates
DELETE FROM `smart_scripts` WHERE `entryorguid` = 448; -- Remove Hogger SAI as now handled via scripting