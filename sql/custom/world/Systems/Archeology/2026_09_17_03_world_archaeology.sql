-- Add achievement reward for Archaeology related achievements
DELETE FROM `achievement_reward` WHERE `ID` IN (4854,4855,4856);
INSERT INTO `achievement_reward` (`ID`, `TitleA`, `TitleH`, `ItemID`, `Sender`, `Subject`, `Body`, `MailTemplateID`) VALUES
(4854, 189, 189, 0, 0, '', '', 0),
(4855, 190, 190, 0, 0, '', '', 0),
(4856, 191, 191, 0, 0, '', '', 0);
