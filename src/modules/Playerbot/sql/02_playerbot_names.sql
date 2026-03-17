-- Sample bot names for the Playerbot system
-- This file populates the playerbots_names table with a variety of names

-- Clear existing names (optional - comment out if you want to append)
-- TRUNCATE TABLE playerbots_names;

-- Male names (gender = 0)
INSERT INTO `playerbots_names` (`name`, `gender`) VALUES
-- Common Human/Alliance style names
('Aldric', 0), ('Gareth', 0), ('Marcus', 0), ('William', 0), ('Edward', 0),
('Richard', 0), ('Thomas', 0), ('Robert', 0), ('James', 0), ('Henry', 0),
('Arthur', 0), ('Charles', 0), ('George', 0), ('Francis', 0), ('Albert', 0),
('Frederick', 0), ('Samuel', 0), ('Benjamin', 0), ('Nicholas', 0), ('Alexander', 0),
('Jonathan', 0), ('Christopher', 0), ('Matthew', 0), ('Daniel', 0), ('Michael', 0),
('Stephen', 0), ('Andrew', 0), ('Patrick', 0), ('Lawrence', 0), ('Vincent', 0),
('Gregory', 0), ('Raymond', 0), ('Timothy', 0), ('Kenneth', 0), ('Eugene', 0),
('Russell', 0), ('Walter', 0), ('Harold', 0), ('Douglas', 0), ('Gerald', 0),

-- Dwarf style names
('Thorgrim', 0), ('Balin', 0), ('Thorin', 0), ('Gimli', 0), ('Durin', 0),
('Dwalin', 0), ('Gloin', 0), ('Oin', 0), ('Bifur', 0), ('Bofur', 0),
('Bombur', 0), ('Nori', 0), ('Dori', 0), ('Ori', 0), ('Flint', 0),
('Magni', 0), ('Muradin', 0), ('Brann', 0), ('Thaurissan', 0), ('Ironforge', 0),

-- Night Elf style names
('Malfurion', 0), ('Illidan', 0), ('Tyrande', 0), ('Cenarius', 0), ('Xavius', 0),
('Fandral', 0), ('Broll', 0), ('Jarod', 0), ('Ravencrest', 0), ('Shadowsong', 0),
('Stormrage', 0), ('Whisperwind', 0), ('Staghelm', 0), ('Bearmantle', 0), ('Moonrage', 0),

-- Gnome style names
('Gelbin', 0), ('Sicco', 0), ('Tinker', 0), ('Mekkatorque', 0), ('Thermaplugg', 0),
('Cogspinner', 0), ('Geargrind', 0), ('Fizzlebolt', 0), ('Sparkwrench', 0), ('Cogspin', 0),

-- Draenei style names
('Velen', 0), ('Akama', 0), ('Nobundo', 0), ('Maraad', 0), ('Hatuun', 0),
('Khadgar', 0), ('Turalyon', 0), ('Kurdran', 0), ('Alleria', 0), ('Danath', 0),

-- Orc style names
('Thrall', 0), ('Durotan', 0), ('Orgrim', 0), ('Grommash', 0), ('Garrosh', 0),
('Doomhammer', 0), ('Hellscream', 0), ('Blackhand', 0), ('Kilrogg', 0), ('Kargath', 0),
('Nazgrel', 0), ('Saurfang', 0), ('Eitrigg', 0), ('Rehgar', 0), ('Jorin', 0),
('Broxigar', 0), ('Varok', 0), ('Nazgrim', 0), ('Malkorok', 0), ('Zaela', 0),

-- Undead style names
('Sylvanas', 0), ('Nathanos', 0), ('Putress', 0), ('Faranell', 0), ('Belmont', 0),
('Varimathras', 0), ('Balnazzar', 0), ('Detheroc', 0), ('Tichondrius', 0), ('Anetheron', 0),
('Archimonde', 0), ('Mannoroth', 0), ('Magtheridon', 0), ('Azgalor', 0), ('Kazzak', 0),

-- Tauren style names
('Cairne', 0), ('Baine', 0), ('Hamuul', 0), ('Runetotem', 0), ('Bloodhoof', 0),
('Thunderhorn', 0), ('Skychaser', 0), ('Wildmane', 0), ('Ragetotem', 0), ('Grimtotem', 0),
('Highmountain', 0), ('Rivermane', 0), ('Winterhoof', 0), ('Mistrunner', 0), ('Dawnstrider', 0),

-- Troll style names
('Voljin', 0), ('Senjin', 0), ('Rokhan', 0), ('Zalazane', 0), ('Zuljin', 0),
('Jintha', 0), ('Venoxis', 0), ('Mandokir', 0), ('Marli', 0), ('Thekal', 0),
('Arlokk', 0), ('Jeklik', 0), ('Hakkari', 0), ('Gurubashi', 0), ('Amani', 0),

-- Blood Elf style names
('Kaelthas', 0), ('Rommath', 0), ('Lorthemar', 0), ('Halduron', 0), ('Aethas', 0),
('Theron', 0), ('Sunstrider', 0), ('Brightwing', 0), ('Dawnseeker', 0), ('Sunreaver', 0),
('Bloodsworn', 0), ('Spellbreaker', 0), ('Farstrider', 0), ('Sunfury', 0), ('Dawncaller', 0),

-- Additional generic fantasy names
('Aiden', 0), ('Blake', 0), ('Connor', 0), ('Derek', 0), ('Ethan', 0),
('Felix', 0), ('Gabriel', 0), ('Hunter', 0), ('Ivan', 0), ('Jacob', 0),
('Kyle', 0), ('Liam', 0), ('Mason', 0), ('Nathan', 0), ('Oliver', 0),
('Peter', 0), ('Quinn', 0), ('Ryan', 0), ('Sean', 0), ('Tyler', 0),
('Ulrich', 0), ('Victor', 0), ('Wesley', 0), ('Xavier', 0), ('Zachary', 0);

-- Female names (gender = 1)
INSERT INTO `playerbots_names` (`name`, `gender`) VALUES
-- Common Human/Alliance style names
('Jaina', 1), ('Katherine', 1), ('Elizabeth', 1), ('Margaret', 1), ('Dorothy', 1),
('Sarah', 1), ('Jessica', 1), ('Michelle', 1), ('Amanda', 1), ('Melissa', 1),
('Jennifer', 1), ('Patricia', 1), ('Barbara', 1), ('Susan', 1), ('Linda', 1),
('Mary', 1), ('Lisa', 1), ('Nancy', 1), ('Karen', 1), ('Betty', 1),
('Helen', 1), ('Sandra', 1), ('Donna', 1), ('Carol', 1), ('Ruth', 1),
('Sharon', 1), ('Laura', 1), ('Cynthia', 1), ('Amy', 1), ('Angela', 1),
('Brenda', 1), ('Emma', 1), ('Anna', 1), ('Marie', 1), ('Christine', 1),
('Deborah', 1), ('Martha', 1), ('Maria', 1), ('Heather', 1), ('Diane', 1),

-- Night Elf style names
('Tyrande', 1), ('Maiev', 1), ('Shandris', 1), ('Azshara', 1), ('Elune', 1),
('Feathermoon', 1), ('Whisperwind', 1), ('Starweaver', 1), ('Moonwhisper', 1), ('Shadowsong', 1),
('Starfall', 1), ('Nightwhisper', 1), ('Dawnweaver', 1), ('Moonfire', 1), ('Starlight', 1),

-- Dwarf style names
('Moira', 1), ('Bronzebeard', 1), ('Ironforge', 1), ('Wildhammer', 1), ('Anvilmar', 1),
('Stormhammer', 1), ('Goldbeard', 1), ('Ironfoot', 1), ('Stoneform', 1), ('Deepforge', 1),

-- Gnome style names
('Chromie', 1), ('Millhouse', 1), ('Tinkerspell', 1), ('Cogwheel', 1), ('Sprocket', 1),
('Gearshift', 1), ('Wrenchcrank', 1), ('Boltbucket', 1), ('Fizzlewick', 1), ('Sparkplug', 1),

-- Draenei style names
('Yrel', 1), ('Ishanah', 1), ('Naielle', 1), ('Askara', 1), ('Dornaa', 1),
('Nobundo', 1), ('Khallia', 1), ('Emony', 1), ('Jessera', 1), ('Anchorite', 1),

-- Orc style names
('Draka', 1), ('Aggra', 1), ('Garona', 1), ('Zaela', 1), ('Geyah', 1),
('Kashur', 1), ('Greatmother', 1), ('Mankrik', 1), ('Sheyala', 1), ('Thura', 1),

-- Undead style names
('Sylvanas', 1), ('Calia', 1), ('Lilian', 1), ('Voss', 1), ('Faranell', 1),
('Velonara', 1), ('Delaryn', 1), ('Sira', 1), ('Arthura', 1), ('Renee', 1),

-- Tauren style names
('Magatha', 1), ('Hamuul', 1), ('Melor', 1), ('Tawnbranch', 1), ('Windtotem', 1),
('Skyhorn', 1), ('Highmountain', 1), ('Riverwind', 1), ('Eagletalon', 1), ('Moonwhisper', 1),

-- Troll style names
('Zentabra', 1), ('Vanira', 1), ('Shadra', 1), ('Bethekk', 1), ('Shirvallah', 1),
('Hireek', 1), ('Jeklik', 1), ('Marli', 1), ('Arlokk', 1), ('Shadehunter', 1),

-- Blood Elf style names
('Liadrin', 1), ('Valeera', 1), ('Alleria', 1), ('Vereesa', 1), ('Sylvanas', 1),
('Sunweaver', 1), ('Dawnblade', 1), ('Goldensword', 1), ('Brightwing', 1), ('Sunseeker', 1),
('Dawnrunner', 1), ('Sunsorrow', 1), ('Bloodwatcher', 1), ('Spellbinder', 1), ('Sunreaver', 1),

-- Additional generic fantasy names
('Alexis', 1), ('Brianna', 1), ('Catherine', 1), ('Diana', 1), ('Elena', 1),
('Fiona', 1), ('Gabrielle', 1), ('Hannah', 1), ('Isabella', 1), ('Jasmine', 1),
('Kaitlyn', 1), ('Lily', 1), ('Madison', 1), ('Natalie', 1), ('Olivia', 1),
('Penelope', 1), ('Quinn', 1), ('Rachel', 1), ('Sophia', 1), ('Taylor', 1),
('Uma', 1), ('Victoria', 1), ('Wendy', 1), ('Xena', 1), ('Yasmine', 1), ('Zoe', 1),

-- Additional fantasy-themed female names
('Aerith', 1), ('Aria', 1), ('Aurora', 1), ('Celeste', 1), ('Luna', 1),
('Nova', 1), ('Seraphina', 1), ('Stella', 1), ('Terra', 1), ('Vega', 1),
('Lyra', 1), ('Nyx', 1), ('Phoenix', 1), ('Raven', 1), ('Scarlett', 1),
('Violet', 1), ('Willow', 1), ('Winter', 1), ('Iris', 1), ('Jade', 1),
('Pearl', 1), ('Rose', 1), ('Ruby', 1), ('Sage', 1), ('Sky', 1);

-- Verify the count
SELECT 
    'Total names loaded:' AS info,
    COUNT(*) AS total,
    SUM(CASE WHEN gender = 0 THEN 1 ELSE 0 END) AS male_names,
    SUM(CASE WHEN gender = 1 THEN 1 ELSE 0 END) AS female_names
FROM playerbots_names;
