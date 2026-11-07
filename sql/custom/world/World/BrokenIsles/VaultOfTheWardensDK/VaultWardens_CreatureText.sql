-- Vault of the Wardens Creature Text

DELETE FROM `creature_text` WHERE `creatureID` IN (92718, 99631, 99632);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(92718, 0, 0, 'I will need your help to stop Gul\'dan. Free the other Illidari, quickly!', 12, 0, 100, 0, 0, 57338, 98734, 0, 'Maiev Shadowsong to Player'),
(99632, 0, 0, 'How many years were wasted in that cell?', 12, 0, 100, 0, 0, 57343, 100833, 0, 'Altruis the Sufferer to Player'),
(99632, 1, 0, 'It has been a long time, Kayn. I still believe in the cause. But, I do not blindly follow anyone... even Illidan.', 12, 0, 100, 1, 0, 57285, 91916, 0, 'Altruis the Sufferer to Player'),
(99631, 0, 0, 'I am awake? How is this possible?', 12, 0, 100, 0, 0, 57294, 100787, 0, 'Kayn Sunfury to Player'),
(99631, 1, 0, 'Why did you let THAT one free? He opposed Lord Illidan!', 12, 0, 100, 603, 0, 57320, 91600, 0, 'Kayn Sunfury to Player');
