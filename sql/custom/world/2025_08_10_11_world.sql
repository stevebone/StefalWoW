-- Quest: 29779 The direct solution
-- Quest: 29780 Do no evil
-- Quest: 29781 Monkey Advisory warning
-- NPC: 55632 Fe-Feng Ruffian
-- NPC: 57465 Fe-Feng Ruffian
-- NPC: 55466 Fe-Feng Firethief
-- NPC: 55634 Ruk-Ruk
-- NPC: 64322 Ruk-Ruk's rocket
-- NPC: 65558 Ji Firepaw (with player)

DELETE FROM `spell_script_names` WHERE `spell_id` = 125699;
INSERT INTO `spell_script_names` VALUES (125699, 'spell_ruk_ruk_ooksplosions');

DELETE FROM `creature_queststarter` WHERE `quest` IN (29779,29780,29781);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55583', '29779', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55583', '29780', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55583', '29781', '0');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29779,29780,29781);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29779', '0', '0', '0', '29778', '29784', '-29779', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29780', '0', '0', '0', '29778', '29784', '-29779', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29781', '0', '0', '0', '29778', '29784', '-29779', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

UPDATE `creature_template` SET `ScriptName` = 'npc_ruk_ruk' WHERE `Entry` = 55634;
UPDATE `creature_template` SET `ScriptName` = 'npc_ruk_ruk_rocket' WHERE `Entry` = 64322;

UPDATE `creature_template` SET `faction` = '16' WHERE (`entry` = '57465'); -- was 14
UPDATE `creature_template` SET `unit_flags` = '0' WHERE (`entry` = '55632'); -- was 537133824

DELETE FROM `creature_template_addon` WHERE `entry` = 55633;
INSERT INTO `creature_template_addon` (`entry`, `SheathState`, `auras`) VALUES ('55633', '1', '127932');

