-- Creature texts for npcs on the housing maps

-- Alliance
DELETE FROM `creature_text` WHERE `creatureID` IN (233063,249850);

INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(233063, 0, 0, 'Ah, a prospective homeowner! Welcome to the neighborhood!', 12, 0, 100, 273, 0, 299725, 296708, 0, 'Lyssabel Dawnpetal to Player'), -- when player comes within range and has quest 

(249850, 0, 0, 'I do enjoy a good stroll about town. Let''s see what plots are available.', 12, 0, 100, 0, 0, 299727, 296707, 0, 'Lyssabel Dawnpetal to Player'), -- on spawn 1
(249850, 1, 0, 'And if you have any questions or find your options are too limited, please speak with me.', 12, 0, 100, 0, 0, 299728, 296705, 0, 'Lyssabel Dawnpetal to Player'), -- on spawn 2
(249850, 2, 0, 'It appears someone has already purchased this plot. But there may be some recourse, if you wish to speak with me on it.', 12, 0, 100, 0, 0, 299732, 296713, 0, 'Lyssabel Dawnpetal to Player'), -- player near plot occupied
(249850, 2, 1, 'This plot is taken. A pity, but not insurmountable. I should be able to help if you speak with me.', 12, 0, 100, 0, 0, 299731, 296712, 0, 'Lyssabel Dawnpetal to Player'), -- player near plot occupied
(249850, 3, 0, 'This plot is a fine choice! I look forward to seeing what you do with it.', 12, 0, 100, 0, 0, 299737, 296732, 0, 'Lyssabel Dawnpetal to Player'); -- player bought house