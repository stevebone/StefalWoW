-- Quest: 26290 Secrets of the tower
-- NPC: 7024 Agent Kearnen
-- NPC: 42656 Mercenary

DELETE FROM `creatures` WHERE `id` = 17234;
SET @CGUID := 900106;
INSERT INTO `creatures` VALUES
(@CGUID+154, '17234', '0', '40', '5289', '0', '0', '0', '0', '-1', '37356', '0', '-11135.7', '545.992', '70.3372', '0.24347', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0');


UPDATE `creature_template` SET `ScriptName` = 'npc_agent_kearnen' WHERE `entry` = 7024;
UPDATE `creature_template` SET `ScriptName` = 'npc_mortwake_tower_elite' WHERE `entry` = 42656;
UPDATE `creature_template` SET `faction` = '35', `ScriptName` = 'npc_shadowy_tower' WHERE (`entry` = '17234');

DELETE FROM `creature_text` WHERE CreatureID = 7024;
INSERT INTO creature_text
(CreatureID, groupid, id, text, type, probability, emote, duration, sound, BroadcastTextId)
VALUES
(7024, 0, 0, 'Headshot!', 15, 100, 0, 0, 0, 0),
(7024, 0, 1, 'Easy peasy!', 15, 100, 0, 0, 0, 0),
(7024, 0, 2, 'Not a chance!', 15, 100, 0, 0, 0, 0),
(7024, 0, 3, 'Got him!', 15, 100, 0, 0, 0, 0),
(7024, 0, 4, 'Got your back, $n', 15, 100, 0, 0, 0, 0);