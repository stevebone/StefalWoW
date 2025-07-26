-- Quest: 29421 Only the Worthy Shall Pass
-- Quest: 29422 Huo, the Spirit of Fire
-- NPC: 54135 Master Li Fei
-- NPC: 54734 Master Li Fei (challenge npc)

DELETE FROM `quest_template_addon` WHERE `ID` IN ('29421', '29422');
INSERT INTO `quest_template_addon` (`ID`, `PrevQuestID`, `NextQuestID`) VALUES ('29421', '29664', '29422');
INSERT INTO `quest_template_addon` (`ID`, `PrevQuestID`, `NextQuestID`, `ProvidedItemCount`) VALUES ('29422', '29421', '29423', '1');
UPDATE `creature` SET `spawntimesecs` = '720' WHERE (`guid` = '450596');

-- Set the scriptname for the original NPC (Master Li Fei)
UPDATE creature_template
SET ScriptName = 'npc_master_li_fei'
WHERE entry = 54135;

-- Set the scriptname for the challenge NPC
UPDATE creature_template
SET ScriptName = 'npc_master_li_fei_challenge'
WHERE entry = 54734;

DELETE FROM creature_text WHERE `CreatureID` = 54135;
INSERT INTO creature_text (CreatureID, groupid, id, text, type, language, probability, emote, comment)
VALUES
(54135, 0, 0, 'You have earned the right to proceed. Huo lies beyond.', 12, 0, 100, 0, 'Master Li Fei after challenge completed');