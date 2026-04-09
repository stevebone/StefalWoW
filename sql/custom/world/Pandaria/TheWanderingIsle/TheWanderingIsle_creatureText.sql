-- The Wandering Isle Creature Texts

-- Jojos
DELETE FROM `creature_text` WHERE `creatureID` IN (57669,57638,57670);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('57669', '0', '0', 'Our crafters build the finest wooden planks.  They would weather the most brutal of storms.  But they are as water before the storm of my crushing skull.', '12', '0', '100', '1', '0', '0', '56343', '0', 'Jojo Ironbrow to Player'),
('57669', '1', '0', 'Nothing made by pandaren hands can withstand me.', '12', '0', '100', '2', '0', '0', '56344', '0', 'Jojo Ironbrow to Player'),
('57638', '0', '0', 'The reeds of the Singing Pools are the hardest in all of the land, but they are as air before my mighty brow.', '12', '0', '100', '1', '0', '0', '56333', '0', 'Jojo Ironbrow to Player'),
('57638', '1', '0', 'Many have tested my claim, and I yet stand proven.', '12', '0', '100', '2', '0', '0', '56334', '0', 'Jojo Ironbrow to Player'),
('57670', '0', '0', 'These are the densest stones across all of Shen-zin Su, yet they are as nothing before my mighty head.', '12', '0', '100', '1', '0', '0', '56355', '0', 'Jojo Ironbrow to Player'),
('57670', '1', '0', 'Any who yet doubt my strength are simply foolish.', '12', '0', '100', '2', '0', '0', '56356', '0', 'Jojo Ironbrow to Player');

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

DELETE FROM `creature_text` WHERE `CreatureID` IN (64885,64881,64880,64879,64875);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES 
('64885', '0', '0', 'Hello $n! The Lorewalker is beginning her lesson just down these stairs if you want to listen in.', '12', '0', '100', '3', '0', '0', '65573', '0', 'Lorewalker Zan to player'),
('64875', '0', '0', 'This is the Song of Liu Lang, the first pandaren explorer.', '12', '0', '100', '1', '0', '0', '65583', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '1', '0', 'The song is written in the old tongue, the language of emperors and scholars. Hardly anyone speaks it anymore.', '12', '0', '100', '6', '0', '0', '65584', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '2', '0', 'It is all about his adventures.', '12', '0', '100', '273', '0', '0', '65585', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '3', '0', 'Brave Liu Lang set out to explore the world on the back of a sea turtle.', '12', '0', '100', '1', '0', '0', '65586', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '4', '0', 'Does anyone remember the turtle\'s name? Yin?', '12', '0', '100', '25', '0', '0', '65587', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '5', '0', 'That\'s right, the turtle was called Shen-zin Su. At first, he was only big enough for Liu Lang to sit on.', '12', '0', '100', '1', '0', '0', '65588', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '6', '0', 'He grew and grew and grew, SO big, that now some people call him \"The Wandering Isle.\"', '12', '0', '100', '1', '0', '0', '65589', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '7', '0', 'Question, Hao?', '12', '0', '100', '25', '0', '0', '65590', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '8', '0', 'Yes, Hao - our home is on Shen-zin Su\'s back.', '12', '0', '100', '273', '0', '0', '65591', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '9', '0', 'Liu Lang discovered many things as he explored the world - oh - yes, Hao?', '12', '0', '100', '6', '0', '0', '65592', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '10', '0', 'That\'s a good question. Nobody has spoken to Shen-zin Su for many generations. He only ever spoke to Liu Lang.', '12', '0', '100', '1', '0', '0', '65593', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '11', '0', 'But I am certain that Shen-zin Su knows we are here. And he cares for every one of us!', '12', '0', '100', '66', '0', '0', '65594', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '12', '0', 'Especially Hao! Liu Lang once said, \"Noble is he who always asks questions.\"', '12', '0', '100', '6', '0', '0', '65595', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '13', '0', '\"We should all be like children, for the world is our elder, and has many things to teach us.\"', '12', '0', '100', '1', '0', '0', '65596', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '14', '0', 'Students! Students! Pay attention now.', '12', '0', '100', '22', '0', '0', '65597', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '15', '0', 'Does anyone know how often Liu Lang went back to Pandaria?', '12', '0', '100', '1', '0', '0', '65598', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '16', '0', 'Haha, no Yin. Liu Lang would eat wherever he explored.', '12', '0', '100', '11', '0', '0', '65599', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '17', '0', 'He returned to the main continent of Pandaria once every five years.', '12', '0', '100', '1', '0', '0', '65600', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '18', '0', 'You are right, Yin. It IS hidden away, behind a cloak of mists.', '12', '0', '100', '274', '0', '0', '65601', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '19', '0', 'Does anyone remember how Liu Lang was always able to find it? Nan?', '12', '0', '100', '6', '0', '0', '65602', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '20', '0', 'That\'s right! Liu Lang always had a way back home. Every five years, he would return, to pick up more explorers.', '12', '0', '100', '1', '0', '0', '65603', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '21', '0', 'Yes Hao! Only the BRAVEST pandaren joined Liu Lang on his turtle to explore the world.', '12', '0', '100', '273', '0', '0', '65604', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '22', '0', 'Shen-zin Su hasn\'t made landfall for a long time. But that hasn\'t stopped some pandaren from exploring, anyway.', '12', '0', '100', '1', '0', '0', '65605', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '23', '0', 'Yes, like Chen and Li Li! They\'re out, exploring the world. Maybe someday you will, too.', '12', '0', '100', '21', '0', '0', '65606', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '24', '0', 'I\'m glad you asked, Yin. Liu Lang explored the world his entire life, and raised many children here on the Wandering Isle.', '12', '0', '100', '1', '0', '0', '65607', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '25', '0', 'Eventually he grew very old, and visited Pandaria one final time.', '12', '0', '100', '1', '0', '0', '65608', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '26', '0', 'But nobody else wanted to go exploring with him. So he left.', '12', '0', '100', '6', '0', '0', '65609', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '27', '0', 'Shen-zin Su has never returned to Pandaria.', '12', '0', '100', '274', '0', '0', '65610', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '28', '0', 'Tired now, Liu Lang said goodbye to Shen-zin Su, his oldest friend.', '12', '0', '100', '1', '0', '0', '65611', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '29', '0', 'Then, Liu Lang went up to the Wood of Staves, carrying with him a bamboo umbrella that he always took on his adventures.', '12', '0', '100', '1', '0', '0', '65612', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '30', '0', 'He opened his umbrella, planted it in the ground, and sat underneath its cool shade.', '12', '0', '100', '6', '0', '0', '65613', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '31', '0', 'He closed his eyes, and became one with the land. And then - do you know what happened?', '12', '0', '100', '2', '0', '0', '65614', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '32', '0', 'His umbrella... sprouted! It grew roots, flowered, and became a tree!', '12', '0', '100', '5', '0', '0', '65615', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '33', '0', 'Well, it\'s true. If you ever go to the Wood of Staves, you can see it.', '12', '0', '100', '1', '0', '0', '65616', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '34', '0', 'Along with the budding staves of all the elders who came before us, growing now into giant trees.', '12', '0', '100', '274', '0', '0', '65617', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '35', '0', 'It\'s not a sad story! Liu Lang himself said, \"Never mourn a life well-lived.\"', '12', '0', '100', '1', '0', '0', '65618', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '36', '0', 'And I think he lived a very very good life. We owe everything we have to Liu Lang, the first pandaren explorer.', '12', '0', '100', '273', '0', '0', '65619', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '37', '0', 'We tell his story, so that he will always be remembered.', '12', '0', '100', '2', '0', '0', '65620', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '38', '0', 'Thank you, Ruolin! That was beautiful.', '12', '0', '100', '21', '0', '0', '65621', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64875', '39', '0', 'Could you sing it once more, for the students who just arrived?', '12', '0', '100', '6', '0', '0', '65622', '0', 'Lorewalker Amai to Lorewalker Ruolin'),
('64879', '0', '0', 'Shen-zin Su!', '12', '0', '100', '1', '0', '0', '65635', '0', 'Yin to Lorewalker Amai'),
('64879', '1', '0', '...Whenever he got hungry?', '12', '0', '100', '1', '0', '0', '65645', '0', 'Yin to Lorewalker Amai'),
('64879', '2', '0', 'But isn\'t Pandaria hidden somewhere? Nobody can find it!', '12', '0', '100', '1', '0', '0', '65646', '0', 'Yin to Lorewalker Amai'),
('64879', '3', '0', 'But we don\'t explore very much anymore, do we?', '12', '0', '100', '1', '0', '0', '65649', '0', 'Yin to Lorewalker Amai'),
('64879', '4', '0', 'Lorewalker! What ever happened to Liu Lang?', '12', '0', '100', '1', '0', '0', '65654', '0', 'Yin to Lorewalker Amai'),
('64879', '5', '0', 'That\'s a sad story!', '12', '0', '100', '1', '0', '0', '65658', '0', 'Yin to Lorewalker Amai'),
('64880', '0', '0', 'What is the song about?', '12', '0', '100', '1', '0', '0', '65634', '0', 'Nan to Lorewalker Amai'),
('64880', '1', '0', 'Sheesh, Hao. Haven\'t you ever paid attention?', '12', '0', '100', '1', '0', '0', '65638', '0', 'Nan to Lorewalker Amai'),
('64880', '2', '0', 'Even Hao?', '12', '0', '100', '6', '0', '0', '65641', '0', 'Nan to Lorewalker Amai'),
('64880', '3', '0', 'Blerch! I\'d rather jump off this bridge.', '12', '0', '100', '1', '0', '0', '65644', '0', 'Nan to Lorewalker Amai'),
('64880', '4', '0', 'Because sea turtles always return to the beach where they were born!', '12', '0', '100', '1', '0', '0', '65647', '0', 'Nan to Lorewalker Amai'),
('64880', '5', '0', 'Like Chen and Li Li Stormstout!', '12', '0', '100', '1', '0', '0', '65650', '0', 'Nan to Lorewalker Amai'),
('64880', '6', '0', 'No you don\'t. It\'s full of trolls.', '12', '0', '100', '1', '0', '0', '65652', '0', 'Nan to Lorewalker Amai'),
('64880', '7', '0', 'What? What?', '12', '0', '100', '1', '0', '0', '65656', '0', 'Nan to Lorewalker Amai'),
('64881', '0', '0', 'Why can\'t I understand any of the words?', '12', '0', '100', '1', '0', '0', '65631', '0', 'Hao to Lorewalker Amai'),
('64881', '1', '0', 'We live on the back of a big turtle?!', '12', '0', '100', '5', '0', '0', '65637', '0', 'Hao to Lorewalker Amai'),
('64881', '2', '0', 'I just thought it was weird that the mountains had flippers!', '12', '0', '100', '1', '0', '0', '65639', '0', 'Hao to Lorewalker Amai'),
('64881', '3', '0', 'Does the turtle know we\'re here?', '12', '0', '100', '1', '0', '0', '65640', '0', 'Hao to Lorewalker Amai'),
('64881', '4', '0', 'Hey!', '12', '0', '100', '1', '0', '0', '65642', '0', 'Hao to Lorewalker Amai'),
('64881', '5', '0', 'Yeah, you should all be more like me.', '12', '0', '100', '1', '0', '0', '65643', '0', 'Hao to Lorewalker Amai'),
('64881', '6', '0', 'Does that mean our moms and dads and grandparents were all explorers, too?', '12', '0', '100', '1', '0', '0', '65648', '0', 'Hao to Lorewalker Amai'),
('64881', '7', '0', 'I want to explore the world!', '12', '0', '100', '1', '0', '0', '65651', '0', 'Hao to Lorewalker Amai'),
('64881', '8', '0', 'Oh. Well, forget it then.', '12', '0', '100', '1', '0', '0', '65653', '0', 'Hao to Lorewalker Amai'),
('64881', '9', '0', 'What?', '12', '0', '100', '1', '0', '0', '65655', '0', 'Hao to Lorewalker Amai'),
('64881', '10', '0', 'That\'s un-possible!', '12', '0', '100', '1', '0', '0', '65657', '0', 'Hao to Lorewalker Amai');

DELETE FROM `creature_text` WHERE `creatureID` IN (55595,64506,55744);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('55595', '0', '0', 'Dafeng! What\'s wrong? Why are you hiding back here?', '12', '0', '100', '1', '0', '27403', '54362', '0', 'Aysa Cloudsinger to Player'),
('64506', '0', '0', 'That\'s the way to do it!', '12', '0', '100', '0', '0', '27380', '64766', '0', 'Aysa Cloudsinger to Zhao-Ren'),
('64506', '1', '0', 'You\'ve done it!  Now quickly, lets attack it while it\'s grounded.', '12', '0', '100', '0', '0', '27381', '64767', '0', 'Aysa Cloudsinger to Zhao-Ren'),
('64506', '2', '0', 'It\'s down!  Let\'s finish it off.', '12', '0', '100', '0', '0', '27382', '64768', '0', 'Aysa Cloudsinger to Zhao-Ren'),
('55744', '0', '0', 'Wait!', '12', '0', '100', '5', '0', '27400', '54355', '0', 'Aysa Cloudsinger to Player'),
('55744', '1', '0', 'We need to wait for the winds to settle, then make a break for the cover of the far hallway.', '12', '0', '100', '1', '0', '27401', '54356', '0', 'Aysa Cloudsinger to Player'),
('55744', '2', '0', 'Wait for another opening. I\'ll meet you on the far side.', '12', '0', '100', '1', '0', '27402', '54357', '0', 'Aysa Cloudsinger to Player');
