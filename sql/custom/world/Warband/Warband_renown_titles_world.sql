-- Certain factions have achievements which reward a title
-- The title reward requires a row into achievement_reward
-- Certain factions have a quest which on completion triggers a player learn spell

-- Quest 70834 for Dragonscale Expedition
-- https://www.wowhead.com/spell=388278/intrepid-explorer

-- Quest 70916 for Valdrakken
-- https://www.wowhead.com/spell=388978/ally-of-dragons

-- Quest 70969 for Iskaara
-- https://www.wowhead.com/spell=389021/of-iskaara

-- Dragonflight
DELETE FROM `achievement_reward` WHERE `ID` IN (16494,16760,18615,41174,41177,41180,41181,41183);
INSERT INTO `achievement_reward` (`ID`, `TitleA`, `TitleH`, `ItemID`, `Sender`, `Subject`, `MailTemplateID`) VALUES 
(41174, 736, 736, 0, 0, 0, 0), -- Dragonscale Expedition
(41177, 801, 801, 0, 0, 0, 0), -- Dream Wardens
(41180, 734, 734, 0, 0, 0, 0), -- Maruuk Centaur
(41181, 737, 737, 0, 0, 0, 0), -- Iskaara
(41182, 735, 735, 0, 0, 0, 0), -- Valdrakken
(41183, 768, 768, 0, 0, 0, 0), -- Loamm
(16494, 745, 745, 0, 0, 0, 0), -- Wrathion
(16760, 744, 744, 0, 0, 0, 0), -- Sabellian
(18615, 781, 781, 0, 0, 0, 0); -- Soridormi

-- Alternative reward title from quest but this may not be warband available
UPDATE `quest_template` SET `RewardTitle` = 734 WHERE `ID` = 71091 AND `RewardTitle` = 0; -- Maruuk Centaur 2503: Khansguard
UPDATE `quest_template` SET `RewardTitle` = 736 WHERE `ID` = 70834 AND `RewardTitle` = 0; -- Dragonscale Expedition 2507: Intrepid Explorer
UPDATE `quest_template` SET `RewardTitle` = 735 WHERE `ID` = 70916 AND `RewardTitle` = 0; -- Valdrakken Accord 2510: Ally of Dragons
UPDATE `quest_template` SET `RewardTitle` = 737 WHERE `ID` = 70969 AND `RewardTitle` = 0; -- Iskaara Tuskarr 2511: Of Iskaara