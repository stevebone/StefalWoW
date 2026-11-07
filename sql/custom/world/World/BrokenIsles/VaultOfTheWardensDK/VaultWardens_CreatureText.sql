-- Vault of the Wardens Creature Text

DELETE FROM `creature_text` WHERE `creatureID` IN (92718);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(92718, 0, 0, 'I will need your help to stop Gul\'dan. Free the other Illidari, quickly!', 12, 0, 100, 0, 0, 57338, 98734, 0, 'Maiev Shadowsong to Player');
