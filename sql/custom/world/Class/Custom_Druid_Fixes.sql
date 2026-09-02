-- Nature's Grace (450347) — replace native script with custom version
-- Midnight DB2 no longer has EFFECT_2 and the spell no longer stacks.
-- Native spell_dru_natures_grace in spell_druid.cpp becomes dead code.
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_dru_natures_grace';
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_dru_natures_grace_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(450347, 'spell_dru_natures_grace_custom');

-- Nature's Grace Eclipse (48517 Solar, 48518 Lunar) — replace native script with custom version
-- Native spell_dru_natures_grace_eclipse referenced EFFECT_2 of Nature's Grace which no longer exists.
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_dru_natures_grace_eclipse';
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_dru_natures_grace_eclipse_custom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(48517, 'spell_dru_natures_grace_eclipse_custom'),
(48518, 'spell_dru_natures_grace_eclipse_custom');

-- Eclipse mechanic (79577 Dummy, 48517 Solar Aura, 48518 Lunar Aura, 329910 OOC)
-- Midnight DB2 no longer has SPELL_DRUID_ECLIPSE_DUMMY (79577), SPELL_DRUID_ECLIPSE_LUNAR_SPELL_CNT (326055),
-- or SPELL_DRUID_ECLIPSE_SOLAR_SPELL_CNT (326053). The Eclipse system is now purely DB2-driven.
-- Remove all script links — native spell_dru_eclipse_aura, spell_dru_eclipse_dummy, spell_dru_eclipse_ooc become dead code.
DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dru_eclipse_aura', 'spell_dru_eclipse_dummy', 'spell_dru_eclipse_ooc');



-- Hidden tracking quests for character (druid) form customization unlock chain
-- Spell quest-complete effect completes this quest, which in turn completes the achievement
-- needed for the form customization requirement check.
-- these are internal quests not "sniffable" and not listed on Wowhead

DELETE FROM `quest_template` WHERE `ID` IN (65047,62675,62676,62677,62678,64986,64987,
65058,65059,65048,65062,62672,62673,62674,65061,78519,78507,78515,78522,78448,78518,78525);
INSERT INTO `quest_template` (`ID`, `QuestType`, `Flags`, `LogTitle`) VALUES
(64986, 2, 1024, '<Hidden> (Druid) Unlocked Twilight Runestag Form [DNT]'),
(65061, 2, 1024, '<Hidden> (Druid) Unlocked Midnight Runestag Form [DNT]'),
(62677, 2, 1024, '<Hidden> (Druid) Unlocked Cheetah Form [DNT]'),
(62678, 2, 1024, '<Hidden> (Druid) Unlocked Doe Form [DNT]'),
(62675, 2, 1024, '<Hidden> (Druid) Unlocked Sentinel Form [DNT]'),
(62676, 2, 1024, '<Hidden> (Druid) Unlocked Humble Flyer Form [DNT]'),
(64987, 2, 1024, '<Hidden> (Druid) Unlocked Shimmering Ardenmoth Form [DNT]'),
(65058, 2, 1024, '<Hidden> (Druid) Unlocked Duskwing Raven Form [DNT]'),
(65047, 2, 1024, '<Hidden> (Druid) Unlocked Nightwing Raven Form [DNT]'),
(65059, 2, 1024, '<Hidden> (Druid) Unlocked Regal Dredbat Form [DNT]'),
(65048, 2, 1024, '<Hidden> (Druid) Unlocked Gloomstalker Dredbat Form [DNT]'),
(65062, 2, 1024, '<Hidden> (Druid) Unlocked Sable Ardenmoth Form [DNT]'),
(62673, 2, 1024, '<Hidden> (Druid) Unlocked Orca Form [DNT]'),
(62674, 2, 1024, '<Hidden> (Druid) Unlocked Dolphin Form [DNT]'),
(62672, 2, 1024, '<Hidden> (Druid) Unlocked Tideskipper Form [DNT]'),
(78519, 2, 1024, '<Hidden> (Druid) Unlocked Loamy Umbraclaw Form [DNT]'),
(78507, 2, 1024, '<Hidden> (Druid) Unlocked Evergreen Dreamsaber Form [DNT]'),
(78515, 2, 1024, '<Hidden> (Druid) Unlocked Thriving Dreamtalon Form [DNT]'),
(78522, 2, 1024, '<Hidden> (Druid) Unlocked Lush Dreamstag Form [DNT]'),
(78448, 2, 1024, '<Hidden> (Druid) Unlocked Slumbering Somnowl Form [DNT]'),
(78518, 2, 1024, '<Hidden> (Druid) Unlocked Bristlebruin Fur Form [DNT]'),
(78525, 2, 1024, '<Hidden> (Druid) Unlocked Fire Moonkin Form [DNT]');
