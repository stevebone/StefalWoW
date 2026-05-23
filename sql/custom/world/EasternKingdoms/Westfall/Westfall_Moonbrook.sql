-- NPC: 43515 Trigger Dawn of a new day

-- Quest: 26297 The dawning of a new day
UPDATE `quest_template_addon` SET `ScriptName` = 'quest_26297_the_dawning_of_a_new_day' WHERE ID = 26297;

UPDATE `creature_template` SET `ScriptName` = 'npc_custom_moonbrook_player_trigger' WHERE `entry` = 43515;

SET @CGUID := 900000;
DELETE FROM `creature` WHERE `guid` IN (@CGUID+821);
INSERT INTO `creature` VALUES
(@CGUID+821, 43515, 0, 40, 20, 0, 0, 0, 0, -1, 0, 0, -11021.1, 1487.15, 43.1937, 4.60869, 300, 0, 0, 100, 0, NULL, NULL, NULL, NULL, '', '', 0);
