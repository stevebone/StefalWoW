-- Quest: 29423 The Passion of Shen-zin Su
-- NPC: 54787 Huo - spawns follower
-- NPC: 54958 Huo (spawned)
-- NPC: 57720 Ji Firepaw
-- NPC: 57721 Aysa Cloudsinger
-- NPC: 54786 Master Shang Xi

DELETE FROM `quest_template_addon` WHERE `ID` = 29423;
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) 
VALUES ('29423', '0', '0', '0', '29422', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

UPDATE `creature_template` SET `ScriptName` = 'npc_huo_spawn_follower' WHERE `entry` = 54787;
UPDATE `creature_template` SET `ScriptName` = 'npc_huo_follower' WHERE `entry` = 54958;
UPDATE `creature_template` SET `ScriptName` = 'npc_master_shang' WHERE (`entry` = '54786');

UPDATE `creature` SET `StringId` = 'master_shang_trigger' WHERE (`guid` = '450020');

DELETE FROM `creature_text` WHERE `CreatureID` = 54786;
INSERT INTO creature_text (CreatureID, groupid, id, text, type, language, probability, emote, duration, sound, BroadcastTextID, TextRange, comment) VALUES
(54786, 0, 0, 'Welcome, Huo. The people have missed your warmth.', 12, 0, 100, 1, 0, 0, 0, 0, 'Master Shang Xi greeting Huo'),
(54786, 1, 0, 'You have conquered every challenge I put before you, $n. You have found Huo and brought him safely to the temple.', 12, 0, 100, 1, 0, 0, 0, 0, 'Master Shang Xi praises player'),
(54786, 2, 0, 'There is a much larger problem we face now, my students. Shen-zin Su is in pain. If we do not act, the very land on which we stand could die, and all of us with it.', 12, 0, 100, 1, 0, 0, 0, 0, 'Shang Xi announces crisis'),
(54786, 3, 0, 'We need to speak to Shen-zin Su and discover how to heal it. And to do that, we need the four elemental spirits returned. Huo was the first.', 12, 0, 100, 1, 0, 0, 0, 0, 'Shang Xi explains the mission'),
(54786, 4, 0, 'Ji, I\'d like you to go to the Dai-Lo Farmstead in search of Wugou, the spirit of earth.', 12, 0, 100, 1, 0, 0, 0, 0, 'Shang Xi instructs Ji'),
(54786, 5, 0, 'Aysa, I want you to go to the Singing Pools to find Shu, the spirit of water.', 12, 0, 100, 1, 0, 0, 0, 0, 'Shang Xi instructs Aysa'),
(54786, 6, 0, 'And $n, you shall be the hand that guides us all. Speak with me for a moment before you join Aysa at the Singing Pools to the east.', 12, 0, 100, 1, 0, 0, 0, 0, 'Final message to player');

DELETE FROM `creature_text` WHERE `CreatureID` = 57720;
INSERT INTO creature_text (CreatureID, groupid, id, text, type, language, probability, emote, duration, sound, BroadcastTextID, TextRange, comment) VALUES
(57720, 0, 0, 'On it!', 12, 0, 100, 1, 0, 0, 0, 0, 'Ji Firepaw responds');

DELETE FROM `creature_text` WHERE `CreatureID` = 57721;
INSERT INTO creature_text (CreatureID, groupid, id, text, type, language, probability, emote, duration, sound, BroadcastTextID, TextRange, comment) VALUES
(57721, 0, 0, 'Yes, master.', 12, 0, 100, 1, 0, 0, 0, 0, 'Aysa Cloudsinger responds');

DELETE FROM `creature_text` WHERE `CreatureID` IN (60248, 60253);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES ('60248', '0', '0', 'Is that... Is that Huo?', '12', '0', '100', '1', '0', '0', '0', '0', '0', 'Chia-hui Autumnleaf when Huo Nearby');
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES ('60253', '0', '0', 'It is! Well done $n', '12', '0', '100', '1', '0', '0', '0', '0', '0', 'Brewer Lin when Huo Nearby');
