-- Quest Templates
DELETE FROM `quest_template_addon` WHERE `ID` IN (29768, 29771, 29769, 29772, 29774, 29775);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29768', '0', '0', '0', '29769', '29772', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''), -- Quest: 29768 Missing Mallet
('29771', '0', '0', '0', '29769', '29772', '-29768', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''), -- Quest: 29771 Stronger Than Wood
('29769', '0', '0', '0', '29680', '29768', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''), -- Quest: 29769 Rascals
('29772', '0', '0', '0', '29768', '29774', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''), -- Quest: 29772 Raucous Rousing
('29774', '0', '0', '0', '29772', '29775', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''), -- Quest: 29774 Not In the Face!
('29775', '0', '0', '0', '29774', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''); -- Quest: 29775 The Spirit and Body of Shen-zin Su

DELETE FROM `creature_queststarter` WHERE `quest` IN (29768, 29771);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55477', '29768', '0'); -- Quest: 29768 Missing Mallet
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55478', '29771', '0'); -- Quest: 29771 Stronger Than Wood


-- Jojo quest crowd
DELETE FROM `creature_addon` WHERE `guid` IN (451158,451160,451170,451171,451173,451174,451178);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES

('451158', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'), -- farmstead jojo power quest
('451160', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451170', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451171', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451173', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451174', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451178', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),