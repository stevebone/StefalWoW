-- Quest: 29521 The Singing Pools
-- Quest: 29661 The Lesson of Dry Fur
-- Quest: 29663 The Lesson of the Balanced Rock
-- Quest: 29676 Finding an Old Friend

 

DELETE FROM `areatrigger_scripts` WHERE `entry` IN (6988, 6989);
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES ('6988', 'at_singing_pools_transform');
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES ('6989', 'at_singing_pools_transform');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29521, 29661, 29663, 29676);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`) 
VALUES 
('29521', '0', '0', '0', '0', '29661', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('29661', '0', '0', '0', '29521', '29676', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('29663', '0', '0', '0', '29521', '29676', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('29676', '0', '0', '0', '29663', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0');