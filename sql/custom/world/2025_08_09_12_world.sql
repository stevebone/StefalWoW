-- NPC: 64885 Lorewalker Zan
-- NPC: 64876 Lorewalker Ruolin
-- NPC: 64875 Lorewalker Amai
-- NPC: 64879 Yin
-- NPC: 64880 Nan
-- NPC: 64881 Hao
-- Area: 8287 Lorewalker Zan

DELETE FROM `areatrigger_scripts` WHERE `entry` IN (8287);
INSERT INTO `areatrigger_scripts` VALUES
(8287, 'at_lorewalker_zan');

UPDATE `creature_template` SET `ScriptName` = 'npc_lorewalker_ruolin' WHERE `Entry` = 64876;

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
