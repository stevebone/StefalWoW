-- Quest: 26297 The dawning of a new day
-- NPC: 43515 Trigger

UPDATE `creature_template` SET `ScriptName` = 'npc_shadowy_trigger' WHERE (`entry` = '43515');
UPDATE `creature` SET `position_x` = '-10954.5', `position_y` = '1509', `position_z` = '54' WHERE (`guid` = '274460');

-- Quest: 26320 A vision of the past
-- Required object for Incense burner
DELETE FROM `gameobject` WHERE `id` = 204039;
INSERT INTO `gameobject` VALUES
('900042', '204039', '36', '1581', '1581', '1', '0', '0', '0', '-1', '-14.8352', '-386.401', '62.7048', '4.90635', '-0', '-0', '-0.635317', '0.772252', '300', '255', '1', '', NULL, '0');

-- Quest: 26322 Rise of the brotherhood
-- NPC: 42635 Ripsnarl

UPDATE `creature_template` SET `ScriptName` = 'npc_ripsnarl' WHERE (`entry` = '42635');
UPDATE `creature_template` SET `ScriptName` = 'npc_rise_br' WHERE (`entry` = '234');
UPDATE `creature_template` SET `ScriptName` = 'npc_defias_blackguard' WHERE (`entry` = '42769');
UPDATE `creature_template_addon` SET `auras` = '6408' WHERE (`entry` = '42769');

UPDATE `creature_template` SET `faction` = '35' WHERE entry IN (42769,42755,42753); -- temp faction since the original ones NOT working
UPDATE `creature_template` SET `faction` = '12', `npcflag` = '8195', `unit_flags` = '768', `unit_flags2` = '2048', `flags_extra` = '66' WHERE (`entry` = '110587');

DELETE FROM `creature_template_difficulty` WHERE `entry` = 110587;
INSERT INTO `creature_template_difficulty` VALUES
('110587', '0', '0', '0', '881', '10', '3', '3', '1', '4.6', '112698', '0', '0', '0', '0', '0', '0', '0', '0', '524288', '0', '0', '0', '0', '0', '0', '0', '0'),
('110587', '1', '0', '0', '881', '10', '3', '3', '1', '4.6', '112698', '0', '0', '0', '0', '0', '0', '0', '0', '524288', '0', '0', '0', '0', '0', '0', '0', '0');


DELETE FROM `creature` WHERE `ID` IN (110587,45937);
INSERT INTO `creature` VALUES
('900261', '110587', '0', '40', '108', '0', '0', '226', '0', '-1', '0', '0', '-10553.6', '1034.42', '57.0394', '0.0174533', '120', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0'),
('900262', '45937', '0', '40', '108', '0', '0', '0', '0', '-1', '0', '0', '-10499.4', '1063.97', '56.2213', '1.99382', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0'),
('900263', '45937', '0', '40', '108', '0', '0', '0', '0', '-1', '0', '0', '-10527', '1049.6', '56.8188', '0.745025', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0');
