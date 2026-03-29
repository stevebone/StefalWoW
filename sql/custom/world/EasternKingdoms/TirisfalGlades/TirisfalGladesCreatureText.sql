-- Tirisfal Glades Creature Texts

-- Darnell
DELETE FROM `creature_text` WHERE `CreatureID` IN (49337);
INSERT INTO creature_text 
(CreatureID, GroupID, ID, Text, Type, Language, Probability, Emote, Duration, Sound, SoundPlayType, BroadcastTextId, TextRange, comment) VALUES
(49337, 0, 0, "Hello again.", 12, 0, 100, 3, 0, 0, 0, 49455, 0, "Darnell - Hello 1"),
(49337, 0, 1, "Greetings, $n", 12, 0, 100, 1, 0, 0, 0, 49305, 0, "Darnell - Hello 2"),
(49337, 1, 0, "I know the way to Deathknell. Come with me!", 12, 0, 100, 273, 0, 0, 0, 49456, 0, "Darnell - Way Deathknell"),
(49337, 2, 0, "Good, you\re still here. Now, where\s Deathguard Saltain?", 12, 0, 100, 1, 0, 0, 0, 49457, 0, "Darnell - Ask about Saltain?"),
(49337, 3, 0, "Ah, here he is.", 12, 0, 100, 1, 0, 0, 0, 49458, 0, "Darnell - Confirm Saltain"),
(49337, 4, 0, "Let\'s get moving, $n. Saltain said that we\'d fine some corpses up here.", 12, 0, 100, 1, 0, 0, 0, 49459, 0, "Darnell - Go corpses"),
(49337, 5, 0, "I think I see some corpses up ahead. Let\'s go, $n! You do the searching and fighting. I\'ll do the lifting.", 12, 0, 100, 0, 0, 0, 0, 49460, 0, "Darnell - Confirm corpses"),
(49337, 6, 0, "This way!", 12, 0, 100, 1, 0, 0, 0, 49296, 0, "Darnell - Shadow Grave"),
(49337, 6, 1, "The Shadow Grave is this way.  Follow me, $n.", 12, 0, 100, 1, 0, 0, 0, 49295, 0, "Darnell - Shadow Grave");
