


-- Quest: 26215 Meet Two-Shoed Lou
DELETE FROM `quest_template_addon` WHERE `ID` = 26215;
INSERT INTO `retail_world`.`quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`) VALUES 
('26215', '0', '0', '79229', '26213', '26228');

-- Quest: 26232 Lou's Parting Thoughts
-- NPC: 42562 Thug Trigger
-- NPC: 42387 Thug
UPDATE `creature_template` SET `ScriptName` = 'npc_lous_parting_thoughts_trigger' WHERE (`entry` = '42562');
UPDATE `creature_template` SET `ScriptName` = 'npc_lous_parting_thoughts_thug' WHERE (`entry` = '42387');
DELETE FROM `creature` WHERE `ID` = 42387;






-- Quest: 26271 Feeding the hungry
-- NPC: 42384 Homeless Stormwind Citizen
-- NPC: 42386 Homeless Stormwind Citizen (Female)

UPDATE `creature_template` SET `ScriptName` = 'npc_hungry_hobo' WHERE (`entry` = '42384');
UPDATE `creature_template` SET `ScriptName` = 'npc_hungry_hobo' WHERE (`entry` = '42386');
UPDATE `creature_template` SET `npcflag` = '0' WHERE (`entry` = '42384');
UPDATE `creature_template` SET `npcflag` = '0' WHERE (`entry` = '42386');



