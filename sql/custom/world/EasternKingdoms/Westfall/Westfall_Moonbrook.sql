-- NPC: 43515 Trigger Dawn of a new day

-- Quest: 26297 The dawning of a new day
UPDATE `quest_template_addon` SET `ScriptName` = 'quest_26297_the_dawning_of_a_new_day' WHERE ID = 26297;

UPDATE `creature_template` SET `ScriptName` = 'npc_custom_moonbrook_player_trigger' WHERE `entry` = 43515;

SET @CGUID := 900000;
DELETE FROM `creature` WHERE `guid` IN (@CGUID+821);
INSERT INTO `creature` VALUES
(@CGUID+821, 43515, 0, 40, 20, 0, 0, 0, 0, -1, 0, 0, -11021.1, 1487.15, 43.1937, 4.60869, 300, 0, 0, 100, 0, NULL, NULL, NULL, NULL, '', '', 0);

-- Quest: 26320 A vision of the past
UPDATE `creature_template` SET `ScriptName` = 'npc_vision_of_the_past', `VehicleId` = 916 WHERE `entry` = 42693;

DELETE FROM `vehicle_template` WHERE `creatureId` = 42693;
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`, `Pitch`, `CustomFlags`) VALUES
(42693, 0, 0, 0);

-- Required object for Incense burner
SET @OGUID := 900000;
DELETE FROM `gameobject` WHERE `guid` = @OGUID+70;
INSERT INTO `gameobject` VALUES
(@OGUID+70, '204039', '36', '1581', '1581', '1', '0', '0', '0', '-1', '-14.8352', '-386.401', '62.7048', '4.90635', '-0', '-0', '-0.635317', '0.772252', '300', '255', '1', '', NULL, '0');
