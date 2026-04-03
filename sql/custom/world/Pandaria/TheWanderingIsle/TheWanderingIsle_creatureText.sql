-- The Wandering Isle Creature Texts

-- Jojos
DELETE FROM `creature_text` WHERE `creatureID` IN (57669,57638);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('57669', '0', '0', 'Our crafters build the finest wooden planks.  They would weather the most brutal of storms.  But they are as water before the storm of my crushing skull.', '12', '0', '100', '1', '0', '0', '56343', '0', 'Jojo Ironbrow to Player'),
('57669', '1', '0', 'Nothing made by pandaren hands can withstand me.', '12', '0', '100', '2', '0', '0', '56344', '0', 'Jojo Ironbrow to Player'),
('57638', '0', '0', 'The reeds of the Singing Pools are the hardest in all of the land, but they are as air before my mighty brow.', '12', '0', '100', '1', '0', '0', '56333', '0', 'Jojo Ironbrow to Player'),
('57638', '1', '0', 'Many have tested my claim, and I yet stand proven.', '12', '0', '100', '2', '0', '0', '56334', '0', 'Jojo Ironbrow to Player');

-- Huo Spirit of fire quest
DELETE FROM `creature_text` WHERE (`CreatureID` IN (54786, 60248, 60253, 61126, 61127));
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(54786, 0, 0, 'Welcome, Huo. The people have missed your warmth.', 12, 0, 100, 2, 0, 27788, 0, 60608, 0, 'Master Shang Xi to Player'),
(60248, 0, 0, 'Is that... is that Huo?', 12, 0, 100, 25, 0, 0, 0, 59751, 0, 'Chia-hui Autumnleaf to Player'),
(60253, 0, 0, 'It is! Well done, $p!', 12, 0, 100, 71, 0, 0, 0, 59752, 0, 'Brewer Lin to Player'),
(54786, 1, 0, 'You have conquered every challenge I put before you, $n. You have found Huo and brought him safely to the temple.', 12, 0, 100, 1, 0, 27789, 0, 60600, 0, 'Master Shang Xi to Player'),
(54786, 1, 1, 'Your training has paid off, young $c. You have found Huo and brought him safely to the temple.', 12, 0, 100, 1, 0, 0, 0, 55254, 0, 'Master Shang Xi to Player'),
(54786, 2, 0, 'There is a much larger problem we face now, my students. Shen-zin Su is in pain. If we do not act, the very land on which we stand could die, and all of us with it.', 12, 0, 100, 1, 0, 27790, 0, 60601, 0, 'Master Shang Xi to Player'),
(54786, 3, 0, 'We need to speak to Shen-zin Su and discover how to heal it. And to do that, we need the four elemental spirits returned. Huo was the first.', 12, 0, 100, 1, 0, 27791, 0, 60602, 0, 'Master Shang Xi to Player'),
(54786, 4, 0, 'Ji, I''d like you to go to the Dai-Lo Farmstead in search of Wugou, the spirit of earth.', 12, 0, 100, 1, 0, 27792, 0, 60603, 0, 'Master Shang Xi to Player'),
(54786, 5, 0, 'Aysa, I want you to go to the Singing Pools to find Shu, the spirit of water.', 12, 0, 100, 1, 0, 27793, 0, 60604, 0, 'Master Shang Xi to Player'),
(54786, 6, 0, 'And $n, you shall be the hand that guides us all. Speak with me for a moment before you join Aysa at the Singing Pools to the east.', 12, 0, 100, 1, 0, 27794, 0, 60605, 0, 'Master Shang Xi to Player'),
(54786, 7, 0, 'Wugou and Shu are welcome here. We will care for them well.', 12, 0, 100, 1, 0, 27777, 0, 55296, 0, 'Master Shang Xi to Player'),
(54786, 8, 0, 'The only remaining spirit is Dafeng, who hides somewhere across the Dawning Span to the west.', 12, 0, 100, 1, 0, 27778, 0, 55297, 0, 'Master Shang Xi to Player'),
(54786, 9, 0, 'I can carry you to the top of the temple. Ji awaits you there to lead the way.', 12, 0, 100, 1, 0, 27779, 0, 55298, 0, 'Master Shang Xi to Player'),
(61126, 0, 0, 'Yes master.', 12, 0, 100, 2, 0, 27406, 0, 0, 0, 'Aysa Cloudsinger to Player'),
(61127, 0, 0, 'On it!', 12, 0, 100, 0, 0, 27306, 0, 60606, 0, 'Ji Firepaw to Player');

-- summoned children at temple
DELETE FROM `creature_text` WHERE (`CreatureID` IN (60250, 60249));
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `comment`) VALUES
(60250, 0, 0, 'Hey there! Are you the one that brought back the fire spirit?', 12, 0, 100, 3, 0, 0, 0, 59875, 'Cai to Deng'),
(60250, 1, 0, 'Of course he was hot! Don''t be dumb, Deng.', 12, 0, 100, 5, 0, 0, 0, 59877, 'Cai to Deng'),
(60250, 2, 0, 'Nuh uh. Bet they could for sure kick you over that hill, though! Hah!', 12, 0, 100, 274, 0, 0, 0, 59880, 'Cai to Deng'),
(60250, 3, 0, 'Okay bye.', 12, 0, 100, 3, 0, 0, 0, 59878, 'Cai to Deng'),
(60249, 0, 0, 'Was he hot?  I bet he was hot!', 12, 0, 100, 6, 0, 0, 0, 59876, 'Deng to Cai'),
(60249, 1, 0, 'I bet you''re really strong, huh?  You could probably kick that bridge right in half if you wanted to!', 12, 0, 100, 6, 0, 0, 0, 59879, 'Deng to Cai'),
(60249, 2, 0, 'Bye!', 12, 0, 100, 3, 0, 0, 0, 59884, 'Deng to Cai');

-- Ji at Temple spire
DELETE FROM `creature_text` WHERE `CreatureID` = 55694;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES 
('55694', '0', '0', 'Hey $p! This way to Morning Breeze Village.', '12', '0', '100', '0', '0', '27294', '54196', '0', 'Ji Firepaw to Player');

-- Ji at Farmstead and various mobs
DELETE FROM `creature_text` WHERE `creatureID` IN (55477,55483,55504);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('55477', '0', '0', 'Wake... up... DANGIT!', '12', '0', '100', '15', '0', '27345', '60447', '0', 'Ji Firepaw'),
('55477', '1', '0', 'This is ridiculous...', '12', '0', '100', '274', '0', '27346', '60448', '0', 'Ji Firepaw'),
('55477', '2', '0', 'You sleep like a rock!', '12', '0', '100', '5', '0', '27347', '60449', '0', 'Ji Firepaw'),
('55477', '3', '0', 'There\'s no way you\'ll sleep through THIS.', '12', '0', '100', '274', '0', '27348', '60450', '0', 'Ji Firepaw'),
('55477', '4', '0', 'Seriously?', '12', '0', '100', '18', '0', '27349', '60451', '0', 'Ji Firepaw'),
('55477', '5', '0', 'Why...', '12', '0', '100', '507', '0', '27350', '60452', '0', 'Ji Firepaw'),
('55477', '6', '0', 'won\'t...', '12', '0', '100', '509', '0', '27351', '60453', '0', 'Ji Firepaw'),
('55477', '7', '0', 'you...', '12', '0', '100', '507', '0', '27352', '60454', '0', 'Ji Firepaw'),
('55477', '8', '0', 'just...', '12', '0', '100', '509', '0', '27353', '60455', '0', 'Ji Firepaw'),
('55477', '9', '0', 'WAKE UP?!', '12', '0', '100', '22', '0', '27354', '60456', '0', 'Ji Firepaw'),
('55477', '10', '0', 'I will break you, little rock man!', '12', '0', '100', '25', '0', '27355', '60457', '0', 'Ji Firepaw'),
('55477', '11', '0', '%s sighs.', '16', '0', '100', '0', '0', '0', '55942', '0', 'Ji Firepaw to Player'),
('55483', '0', '0', 'Gimme all your vegetables!', '12', '0', '100', '0', '0', '0', '54873', '0', 'Plump Virmen to Player'),
('55483', '0', '1', 'AIIIIEEEEEEE!', '12', '0', '100', '0', '0', '0', '54874', '0', 'Plump Virmen to Player'),
('55483', '0', '2', 'This virmen land!', '12', '0', '100', '0', '0', '0', '54876', '0', 'Plump Virmen to Player'),
('55483', '0', '3', 'You no take carrot! You take turnip instead!', '12', '0', '100', '0', '0', '0', '54877', '0', 'Plump Virmen to Player'),
('55483', '0', '4', 'We rowdy!', '12', '0', '100', '0', '0', '0', '54875', '0', 'Plump Virmen to Player'),
('55504', '0', '0', 'Gimme all your vegetables!', '12', '0', '100', '0', '0', '0', '54873', '0', 'Plump Carrotcruncher to Player'),
('55504', '0', '1', 'AIIIIEEEEEEE!', '12', '0', '100', '0', '0', '0', '54874', '0', 'Plump Carrotcruncher to Player'),
('55504', '0', '2', 'This virmen land!', '12', '0', '100', '0', '0', '0', '54876', '0', 'Plump Carrotcruncher to Player'),
('55504', '0', '3', 'You no take carrot! You take turnip instead!', '12', '0', '100', '0', '0', '0', '54877', '0', 'Plump Carrotcruncher to Player'),
('55504', '0', '4', 'We rowdy!', '12', '0', '100', '0', '0', '0', '54875', '0', 'Plump Carrotcruncher to Player');

-- Wugou sleeping
DELETE FROM `creature_text` WHERE `creatureID` IN (55539);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('55539', '0', '0', '%s snorts loudly, and continues sleeping.', '16', '0', '100', '0', '0', '0', '54991', '0', 'Wugou to Player');

-- Wo-son Villager
DELETE FROM `creature_text` WHERE `creatureID` IN (57132);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('57132', '0', '0', 'I have had enough of this!', '12', '0', '100', '1', '0', '0', '0', '0', 'Wu-song Villager Random line'),
('57132', '0', '1', 'Just... go away!', '12', '0', '100', '1', '0', '0', '0', '0', 'Wu-song Villager Random line'),
('57132', '0', '2', 'You nasty... little creature!', '12', '0', '100', '1', '0', '0', '0', '0', 'Wu-song Villager Random line'),
('57132', '0', '3', 'Get off of me!', '12', '0', '100', '1', '0', '0', '0', '0', 'Wu-song Villager Random line');

-- Cart Tender
DELETE FROM `creature_text` WHERE `CreatureID` = 57712;
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`BroadcastTextId`,`TextRange`,`comment`) VALUES 
(57712,0,0,'Hello friend!  You\'re welcome to use my cart if you like.  It will take you to the Dai-Lo Farmstead.',12,0,100,3,0,0,56406,0,'Delivery Cart Tender to Player'),
(57712,1,0,'Hello friend!  You\'re welcome to use my cart if you like.  It will take you to the Temple of Five Dawns.',12,0,100,3,0,0,56415,0,'Delivery Cart Tender to Player');

-- Aysa at A new friend quest
DELETE FROM `creature_text` WHERE `CreatureID` = 54975;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES 
('54975', '0', '0', 'I have to admit, that looked pretty fun!', '12', '0', '100', '0', '0', '27394', '0', '66056', '1', 'Aysa Cloudsinger - quest A new friend'),
('54975', '1', '0', 'And it looks to me like you made a new friend.', '12', '0', '100', '0', '0', '27395', '0', '66057', '1', 'Aysa Cloudsinger - quest A new friend');
