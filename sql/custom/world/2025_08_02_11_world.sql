-- Quest: 29679 A new friend
-- Spell: 128589 Aysa Congrats timer -- gets applied after 117054
-- Spell: 103538 Summon Spirit of the water -- gets applied after 117054
-- Spell: 117054 Water Spout quest credit

-- Quest: 29680 The Source of Our Livelihood

-- temp fix until quest accept works from auto completed quests and new offered quests
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('54975', '29679', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('54975', '29680', '0');


DELETE FROM `quest_template_addon` WHERE `ID` IN (29679,29680);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29679', '0', '0', '0', '29678', '29680', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'quest_a_new_friend'),
('29680', '0', '0', '0', '29679', '29769', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `spell_script_names` WHERE `spell_id` IN (128588,128589);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('128588', 'spell_aysa_congrats_trigger_aura');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('128589', 'spell_aysa_congrats_timer');

DELETE FROM `creature_text` WHERE `CreatureID` = 54975;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES 
('54975', '0', '0', 'I have to admit, that looked pretty fun!', '12', '0', '100', '0', '0', '27394', '0', '66056', '1', 'Aysa Cloudsinger - quest A new friend'),
('54975', '1', '1', 'And it looks to me like you made a new friend.', '12', '0', '100', '0', '0', '27395', '0', '66057', '1', 'Aysa Cloudsinger - quest A new friend');

UPDATE `creature_addon` SET `auras` = '89304' WHERE (`guid` = '451090');
UPDATE `creature_template_addon` SET `auras` = '89304 116695' WHERE (`entry` = '60488');

