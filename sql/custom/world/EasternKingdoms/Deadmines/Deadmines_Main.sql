-- Deadmines Dungeon
-- Modern version after Cataclysm

-- NPC: 47162 Glubtok
-- NPC: 48974 Glubtok Main Platter
-- NPC: 48230 Ogre Henchman
-- NPC: 48262 Ogre Bodyguard
-- NPC: 48266 Defias Cannon
-- NPC: 50595 Stormwind Defender
-- NPC: 46890 Shattered Hand Assassin
-- NPC: 46889 Kagtha
-- NPC: 46902 Miss Mayhem
-- NPC: 48284 Mining Powder
-- NPC: 48279 Goblin Overseer
-- NPC: 48338 Mine Bunny
-- NPC: 48351 Mine Bunny
-- NPC: 48278 Mining Monkey
-- NPC: 48440 Mining Monkey
-- NPC: 48441 Mining Monkey
-- NPC: 48442 Mining Monkey
-- NPC: 48445 Oaf Lackey
-- NPC: 47297 Lumbering Oaf
-- NPC: 49136 Helix Crew
-- NPC: 47314 Sticky Bomb
-- NPC: 47297 Helix Gearbreaker
-- NPC: 48957 Fire Blossom
-- NPC: 48958 Frost Blossom
-- NPC: 48418 Defias Envoker
-- NPC: 48419 Defias Miner
-- NPC: 48420 Defias Digger
-- NPC: 48421 Defias Overseer
-- NPC: 49208 Prototype Reaper
-- NPC: 47404 Defias Watcher
-- NPC: 47403 Defias Reaper
-- NPC: 43778 Foe Reaper 5000
-- NPC: 47242 General Purpose bunny (for Foe Reaper energize spell).
-- NPC: 47468 Deadmines Foe Reaper Targetting Bunny
-- NPC: 49229 Molten Slag
-- NPC: 48502 Defias Enforcer
-- NPC: 48417 Defias Blood Wizzard
-- NPC: 48505 Defias Shadowguard

-- GO: 208002 Goblin Teleporter
-- GO: 207079 Ball and Chain

-- Creature Template Updates
UPDATE `creature_template` SET `unit_flags` = 0 WHERE `entry` = 48284; -- remove uninterract flag for mining powder so it can be attacked
UPDATE `creature_template` SET `faction` = 17 WHERE `entry` IN (47403,47404); -- Update attackable faction for Defias Reaper and Watcher was 1816

-- Creature Difficulties
-- Stormwind Defender and Shattered Hand Assassin have diff id 0 added for Followship Bots since they can be taken outside the dungeon
-- Diff 1 - Normal / Diff 2 - Heroic / Diff 24 - Timewalking

DELETE FROM `creature_template_difficulty` WHERE `entry` IN (47162,48229,48230,50595,46890,46903,46902,46889,46906,48262,48284,48279,48338,
48440,48441,48442,48278,48351,47296,47297,48445,47314,48957,48958,43778,47403,48418,48419,48420,48421,47404,49208,49229,48502,48417,48505);
INSERT INTO `creature_template_difficulty` VALUES
(48417, 1, 0, 0, 202, 0, 1, 0.7264, 1, 1, 41355, 0, 0, 0, 48417, 48417, 0, 8562, 8562, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(48417, 2, 0, 0, 1199, 0, 2, 0.7264, 1, 0.8, 41355, 0, 0, 0, 48417, 48417, 0, 8562, 8562, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48417, 24, 0, 0, 2872, 3, 1, 1, 1, 1, 41356, 0, 0, 0, 48417, 48417, 0, 8562, 8562, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Enforcer
(48502, 1, 0, 0, 202, 0, 1, 1, 1, 1, 41129, 0, 0, 0, 48502, 48502, 0, 7576, 7576, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(48502, 2, 0, 0, 1199, 0, 2, 1, 1, 0.8, 41129, 0, 0, 0, 48502, 48502, 0, 7576, 7576, 524288, 0, 0, 0, 0, 0, 0, 0, 45745), 
(48502, 24, 0, 0, 2872, 3, 1, 1, 1, 1, 41130, 0, 0, 0, 48502, 48502, 0, 7576, 7576, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Blood Wizzard
(48505, 1, 0, 0, 202, 0, 1, 1, 1, 1, 41119, 0, 0, 0, 48505, 48505, 0, 7403, 7403, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(48505, 2, 0, 0, 1199, 0, 2, 1, 1, 0.8, 41119, 0, 0, 0, 48505, 48505, 0, 7403, 7403, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48505, 24, 0, 0, 2872, 3, 1, 1, 1, 1, 41120, 0, 0, 0, 48505, 48505, 0, 7403, 7403, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Shadowguard
(49229, 2, 0, 0, 1199, 0, 0.5, 1, 1, 0.2, 40067, 512, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, 40593), -- Molten Slag (only Heroic)
(43778, 1, 1, 1, 202, 0, 5, 1, 1, 0.2, 48941 , 32872, 128, 0, 4377801, 0, 0, 20008, 20008, 524288, 0, 0, 0, 0, 0, 0, 0, 45338), -- 51015
(43778, 2, 2, 2, 1199, 0, 7, 1, 1, 0.5, 48941 , 32872, 128, 0, 4377802, 0, 0, 20008, 20008, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(43778, 24, 2, 2, 2872, 3, 5, 1, 1, 1, 51016, 32872, 128, 0, 4377824, 0, 0, 20008, 20008, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Foe Reaper 5000
(47403, 1, 0, 0, 202, 0, 1, 1, 1, 0.5, 48804, 32768, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45338), -- 43481
(47403, 2, 1, 1, 1199, 0, 2, 1, 1, 0.7, 48804, 32768, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45745),
(47403, 24, 1, 1, 2872, 3, 3, 1, 1, 3, 43482, 32768, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Reaper
(47404, 1, 0, 0, 202, 0, 1, 1, 1, 0.5, 48803, 32768, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45338),
(47404, 2, 1, 1, 1199, 0, 2, 1, 1, 0.7, 48803, 32768, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45745), -- 43476
(47404, 24, 1, 1, 2872, 3, 3, 1, 1, 3, 43477, 32768, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Watcher
(49208, 1, 0, 0, 202, 0, 10, 1, 1, 3, 40112, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(49208, 2, 0, 0, 1199, 0, 15, 1, 1, 5, 40112, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45745),
(49208, 24, 0, 0, 2872, 0, 10, 1, 1, 3, 40112, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45745), -- Prototype Reaper
(48421, 1, 0, 0, 202, 0, 1, 0, 1, 1, 41335, 0, 0, 0, 48421, 0, 0, 7165, 7165, 524288, 0, 0, 0, 0, 0, 0, 0, 0),
(48421, 2, 0, 0, 1199, 0, 3, 0, 1, 3, 41335, 0, 0, 0, 48421, 0, 0, 7165, 7165, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48421, 24, 0, 0, 2872, 3, 3, 0, 1, 3, 41336, 0, 0, 0, 48421, 0, 0, 7165, 7165, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Overseer
(48420, 1, 0, 0, 202, 0, 1, 1, 1, 1, 41338, 0, 0, 0, 48420, 48420, 0, 7903, 7903, 524288, 0, 0, 0, 0, 0, 0, 0, 0),
(48420, 2, 0, 0, 1199, 0, 1, 1, 1, 3, 41338, 0, 0, 0, 48420, 48420, 0, 7903, 7903, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48420, 24, 0, 0, 2872, 3, 1.5, 1, 1, 3, 41339, 0, 0, 0, 48420, 48420, 0, 7903, 7903, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Digger
(48419, 1, 0, 0, 202, 0, 1, 1, 1, 1, 41346, 0, 0, 0, 48419, 0, 0, 7906, 7906, 524288, 0, 0, 0, 0, 0, 0, 0, 0),
(48419, 2, 0, 0, 1199, 0, 3, 1, 1, 3, 41346, 0, 0, 0, 48419, 0, 0, 7906, 7906, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48419, 24, 0, 0, 2872, 3, 1.5, 1, 1, 5, 41347, 0, 0, 0, 48419, 0, 0, 7906, 7906, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Miner
(48418, 1, 0, 0, 202, 0, 1, 0.7264, 1, 1, 41350, 0, 0, 0, 48418, 0, 0, 8541, 8541, 524288, 0, 0, 0, 0, 0, 0, 0, 0),
(48418, 2, 0, 0, 1199, 0, 4, 0.7264, 1, 4, 41350, 0, 0, 0, 48418, 0, 0, 8541, 8541, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48418, 24, 0, 0, 2872, 3, 3, 3, 1, 3, 41351, 0, 0, 0, 48418, 0, 0, 8541, 8541, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Envoker
(48957, 1, 0, 0, 202, 0, 0.05, 1, 1, 1, 40468, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(48957, 2, 0, 0, 1199, 0, 0.05, 1, 1, 1, 40468, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(48957, 24, 0, 0, 2872, 0, 0.05, 1, 1, 1, 40468, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), -- Fire Blossom
(48958, 1, 0, 0, 202, 0, 0.05, 1, 1, 1, 40466, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(48958, 2, 0, 0, 1199, 0, 0.05, 1, 1, 1, 40466, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(48958, 24, 0, 0, 2872, 0, 0.05, 1, 1, 1, 40466, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), -- Frost Blossom
(47314, 1, 0, 0, 202, 0, 0.2, 1, 1, 1, 43659, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(47314, 2, 0, 0, 1199, 0, 0.2, 1, 1, 1, 43659, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45745),
(47314, 24, 0, 0, 2872, 3, 0.2, 1, 1, 1, 43660, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), -- Sticky Bomb
(47296, 1, 1, 1, 202, 0, 1, 1, 1, 1, 43704, 104, 128, 0, 4729601, 0, 0, 13404, 13404, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(47296, 2, 2, 2, 1199, 0, 5, 1, 1, 5, 43704, 104, 128, 0, 4729602, 0, 0, 13404, 13404, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(47296, 24, 2, 2, 2872, 3, 10, 1, 1, 3, 43705, 104, 128, 0, 4729624, 0, 0, 13404, 13404, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Helix Gearbreaker
(47297, 1, 1, 1, 202, 0, 1, 1, 1, 1, 43698, 104, 0, 0, 0, 47297, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(47297, 2, 2, 2, 1199, 0, 5, 1, 1, 5, 43698, 104, 0, 0, 0, 47297, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(47297, 24, 2, 2, 2872, 3, 7, 1, 1, 3, 43699, 104, 0, 0, 0, 47297, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Lumbering Oaf
(48445, 1, 0, 0, 202, 0, 1, 1, 1, 1, 41267, 0, 0, 0, 48445, 48445, 0, 7141, 7141, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(48445, 2, 0, 0, 1199, 0, 3, 1, 1, 3, 41267, 0, 0, 0, 48445, 48445, 0, 7141, 7141, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48445, 24, 0, 0, 2872, 3, 5, 1, 1, 3, 41268, 0, 0, 0, 48445, 48445, 0, 7141, 7141, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Oaf Lackey
(48278, 0, -1, -1, 202, 0, 1, 1, 1, 1, 41633, 1, 0, 0, 48278, 0, 0, 2137, 2137, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(48278, 2, 0, 0, 1199, 0, 1, 1, 1, 1, 41633, 1, 0, 0, 48278, 0, 0, 2137, 2137, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48278, 24, 0, 0, 2872, 3, 3, 1, 1, 1.8, 41634, 1, 0, 0, 48278, 0, 0, 2137, 2137, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(48440, 0, -1, -1, 202, 0, 1, 1, 1, 1, 41288, 1, 0, 0, 48440, 0, 0, 2137, 2137, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(48440, 2, 0, 0, 1199, 0, 1, 1, 1, 1.4, 41288, 1, 0, 0, 48440, 0, 0, 2137, 2137, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48440, 24, 0, 0, 2872, 3, 3, 1, 1, 1.8, 41289, 1, 0, 0, 48440, 0, 0, 2137, 2137, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(48441, 0, -1, -1, 202, 0, 1, 1, 1, 1, 41285, 1, 0, 0, 48441, 0, 0, 2285, 2285, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(48441, 2, 0, 0, 1199, 0, 1, 1, 1, 1.4, 41285, 1, 0, 0, 48441, 0, 0, 2285, 2285, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48441, 24, 0, 0, 2872, 3, 3, 1, 1, 1.8, 41286, 1, 0, 0, 48441, 0, 0, 2285, 2285, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(48442, 0, -1, -1, 202, 0, 1, 1, 1, 1, 41277, 1, 0, 0, 48442, 0, 0, 2285, 2285, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(48442, 2, 0, 0, 1199, 0, 1, 1, 1, 1.4, 41277, 1, 0, 0, 48442, 0, 0, 2285, 2285, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48442, 24, 0, 0, 2872, 3, 3, 1, 1, 1.8, 41278, 1, 0, 0, 48442, 0, 0, 2285, 2285, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Mining Monkey (multi versions)
(48338, 1, -1, -1, 202, 0, 1, 1, 1, 1, 41507, 0, 0, 0, 48338, 48338, 0, 8244, 8244, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(48338, 2, 0, 0, 1199, 0, 3, 1, 1, 3, 41507, 0, 0, 0, 48338, 48338, 0, 8244, 8244, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48338, 24, 0, 0, 2872, 3, 5, 1, 1, 3, 41508, 0, 0, 0, 48338, 48338, 0, 8244, 8244, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Mine Bunny
(48351, 1, 0, 0, 202, 0, 1, 1, 1, 1, 41481, 0, 0, 0, 48351, 48351, 0, 8244, 8244, 524288, 0, 0, 0, 0, 0, 0, 0, 0),
(48351, 2, 0, 0, 1199, 0, 3, 1, 1, 3, 41481, 0, 0, 0, 48351, 48351, 0, 8244, 8244, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48351, 24, 0, 0, 2872, 3, 5, 1, 1, 3, 41482, 0, 0, 0, 48351, 48351, 0, 8244, 8244, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Mine Bunny
(48279, 1, 0, 0, 202, 0, 1, 1, 1, 1, 41627, 0, 0, 0, 48279, 48279, 0, 6975, 6975, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(48279, 2, 0, 0, 1199, 0, 3, 1, 1, 3, 41627, 0, 0, 0, 48279, 48279, 0, 6975, 6975, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48279, 24, 0, 0, 2872, 3, 7, 1, 1, 3, 41628, 0, 0, 0, 48279, 48279, 0, 6975, 6975, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Goblin Overseer
(48284, 1, 	-1, -1, 202, 1, 1, 1, 1, 1, 41616, 0, 0, 0, 0, 0, 0, 0, 0, 536871168, 0, 0, 0, 0, 0, 0, 0, -1),
(48284, 2, 	0, 0, 1199, 1, 1, 1, 1, 1, 41617, 0, 0, 0, 0, 0, 0, 0, 0, 536871168, 0, 0, 0, 0, 0, 0, 0, -1),
(48284, 24, 0, 0, 2872, 3, 1, 1, 1, 1, 41617, 0, 0, 0, 0, 0, 0, 0, 0, 536871168, 0, 0, 0, 0, 0, 0, 0, -1), -- Mining Powder
(46906, 1, 0, 0, 202, 0, 1, 1, 1, 1, 44620, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46906, 2, 0, 0, 1199, 0, 1, 1, 1, 1, 44620, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46906, 24, 0, 0, 2872, 0, 1, 1, 1, 1, 44620, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Slinky Sharpshiv
(46889, 1, 0, 0, 202, 0, 1, 1, 1, 1, 44655, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46889, 2, 0, 0, 1199, 0, 1, 1, 1, 1, 44655, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46889, 24, 0, 0, 2872, 0, 1, 1, 1, 1, 44655, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Kagtha
(46902, 1, 0, 0, 202, 0, 1, 1, 1, 1, 44629, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46902, 2, 0, 0, 1199, 0, 1, 1, 1, 1, 44629, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46902, 24, 0, 0, 2872, 0, 1, 1, 1, 1, 44629, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Miss Mayhem
(46903, 1, 0, 0, 202, 0, 1, 1, 1, 1, 44628, 32768, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(46903, 2, 0, 0, 1199, 0, 1, 1, 1, 1, 44628, 32768, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(46903, 24, 0, 0, 2872, 0, 1, 1, 1, 1, 44628, 32768, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), -- Mayhem Prototype
(46890, 0, 0, 0, 202, 0, 1, 1, 1, 1, 44653, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46890, 1, 0, 0, 202, 0, 1, 1, 1, 1, 44653, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46890, 2, 0, 0, 1199, 0, 1, 1, 1, 1, 44653, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46890, 24, 0, 0, 2872, 0, 1, 1, 1, 1, 44653, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Shattered Hand Assassin
(50595, 0, 0, 0, 202, 0, 1, 1, 1, 1, 37807, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45338),
(50595, 1, 0, 0, 202, 0, 1, 1, 1, 2, 37807, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45338),
(50595, 2, 0, 0, 1199, 0, 1, 1, 1, 5, 37807, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45338),
(50595, 24, 0, 0, 2872, 0, 1, 1, 1, 5, 37807, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45745), -- Alliance Defender
(48229, 1, -1, -1, 202, 0, 1, 1, 1, 1, 41764, 0, 0, 0, 48229, 48229, 0, 7684, 7684, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(48229, 2, 0, 0, 1199, 0, 2, 1, 1, 3, 41764, 0, 0, 0, 48229, 48229, 0, 7684, 7684, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48229, 24, 0, 0, 2872, 3, 2, 1, 1, 3, 41765, 0, 0, 0, 48229, 48229, 0, 7684, 7684, 524288, 0, 0, 0, 0, 0, 0, 0, 64978), -- Kobold Digger
(48230, 1, 0, 0, 202, 0, 1, 1, 1, 1, 41758, 0, 0, 0, 48230, 48230, 0, 7084, 7084, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(48230, 2, 1, 1, 1199, 0, 3, 1, 1, 3, 41758, 0, 0, 0, 48230, 48230, 0, 7084, 7084, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48230, 24, 1, 1, 2872, 3, 7, 1, 1, 3, 41759, 0, 0, 0, 48230, 48230, 0, 7084, 7084, 524288, 0, 0, 0, 0, 0, 0, 0, 45745), -- Ogre Henchman
(48262, 1, 0, 0, 202, 	0, 1, 1, 1, 1, 41681, 0, 0, 0, 0, 0, 0, 4819, 4819, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(48262, 2, 1, 1, 1199, 	0, 3, 1, 1, 3, 41681, 0, 0, 0, 0, 0, 0, 4819, 4819, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48262, 24, 1, 1, 2872, 3, 7, 1, 1, 3, 41682, 0, 0, 0, 0, 0, 0, 4819, 4819, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Ogre Bodyguard
(47162, 1, 1, 1, 202, 0, 1, 1, 1, 1, 43995, 104, 128, 0, 4716201, 0, 0, 13158, 13158, 524288, 0, 0, 0, 0, 0, 0, 0, 64978),
(47162, 2, 2, 2, 1199, 0, 5, 1, 1, 5, 43995, 104, 128, 0, 4716202, 0, 0, 13158, 13158, 524288, 0, 0, 0, 0, 0, 0, 0, 64978),
(47162, 24, 2, 2, 2872, 3, 12, 1, 1, 3, 43996, 104, 128, 0, 4716224, 0, 0, 13158, 13158, 524288, 0, 0, 0, 0, 0, 0, 0, 64978); -- Glubtok

UPDATE `creature_template_addon` SET `PvPFlags` = 16, `emote` = 648 WHERE `entry` = 48229; -- Kobolds mining emote
UPDATE `creature_template_addon` SET `emote` = 648 WHERE `entry` IN (48440,48441,48442,48278); -- Mining monkeys emote - allow weapon to be seen
UPDATE `creature_template_addon` SET `auras` = '92001' WHERE `entry` = 48418; -- Defias Envoker Aura
UPDATE `creature_template_addon` SET `auras` = '87239 88348' WHERE `entry` = 43778; -- Foe Reaper 5000 auras

-- Spawn Updates
UPDATE `creature` SET `ZoneId` = 1581, `AreaId` = 1581, `spawnDifficulties` = '1,2,24' WHERE `map` = 36 AND `PhaseId` IN (0, 169);
UPDATE `gameobject` SET `ZoneId` = 1581, `AreaId` = 1581, `spawnDifficulties` = '1,2,24' WHERE `map` = 36 AND `PhaseId` IN (0, 169);

-- Schorch Mark bunny should have its model changed in the script to 36147
UPDATE `creature` SET `modelid` = 0 WHERE `guid` = 375860; 

-- Remove duplicate Mining Monkey spawn
DELETE FROM `creature` WHERE `guid` = 376054;

-- Remove Mine rats spawns as these are spawned during the Helix encounter
DELETE FROM `creature` WHERE `id` = 51462 AND `map` = 36;

-- Remove Wrong Goblin Craftsman spawns in Mast Room
-- These are the ones spawned at Goblin Foundry Room and move here
DELETE FROM `creature` WHERE `guid` IN (376095,376096,376102,376103);

-- Defias Miner spawns fixes
UPDATE `creature` SET `modelid` = 0 WHERE `id` = 48419 AND `map` = 36; -- remove old models to allow spawns with new ones
UPDATE `creature` SET `equipment_id` = 2 WHERE `guid` IN (375906,376023,376240,375827,376022); -- set miners with wood chopping equipment_id

-- Defias Digger spawns fixes
UPDATE `creature` SET `modelid` = 0 WHERE `id` = 48420 AND `map` = 36; -- remove old models to allow spawns with new ones

-- Delete Prototype Reaper as it spawns by boss AI
DELETE FROM `creature` WHERE `id` = 49208 and `map` = 36;

-- Delete Defias Watcher and Reaper as they are spawned by boss AI
-- This is needed for boss reset since encounter always starts after the 4 harvesters are dead
DELETE FROM `creature` WHERE `id` IN (47403, 47404) and `map` = 36;

-- Spawn Addons
DELETE FROM `creature_addon` WHERE `guid` IN (375906,376023,376240,375827,376022);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
-- Defias Miners with wood chop emote
(375906, 0, 0, 0, 0, 0, 0, 1, 0, 234, 0, 0, 0, 0, ''),
(376023, 0, 0, 0, 0, 0, 0, 1, 0, 234, 0, 0, 0, 0, ''),
(376240, 0, 0, 0, 0, 0, 0, 1, 0, 234, 0, 0, 0, 0, ''),
(375827, 0, 0, 0, 0, 0, 0, 1, 0, 234, 0, 0, 0, 0, ''),
(376022, 0, 0, 0, 0, 0, 0, 1, 0, 234, 0, 0, 0, 0, '');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (48229,48279,48440,48441,48442,48278,48351,48338,48418,48419,48420);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (48229,48279,48440,48441,48442,48278,48351,48338,48418,48419,48420) and `source_type` = 0;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, 
`event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `event_param_string`, 
`action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, 
`action_param_string`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `target_param_string`, 
`target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(48420, 0, 0, 0, '', 2, 0, 100, 1, 0, 30, 1000, 1000, 0, '', 11, 90994, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Defias Digger - HP PCT 30 - Cast Tunnel at 30% HP'),
(48419, 0, 0, 0, '', 2, 0, 100, 1, 0, 30, 1000, 1000, 0, '', 11, 90994, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Defias Miner - HP PCT 30 - Cast Tunnel at 30% HP'),
(48418, 0, 0, 0, '', 0, 0, 100, 0, 0, 0, 2300, 5900, 0, '', 11, 91004, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Defias Envoker - IC - Cast Holy Fire'),
(48418, 0, 1, 2, '', 2, 0, 100, 1, 0, 15, 1000, 1000, 0, '', 25, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Defias Envoker - HP PCT 15 - Flee'),
(48418, 0, 2, 0, '', 61, 0, 100, 1, 0, 15, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Defias Envoker - HP PCT 15 - Say 0'),
(48418, 0, 3, 0, '', 0, 0, 100, 0, 10000, 10000, 23000, 30000, 0, '', 11, 90047, 0, 0, 0, 0, 0, 0, '', 26, 15, 0, 0, 0, '', 0, 0, 0, 0, 'Defias Envoker - IC - Cast Renegade Strength'),
(48418, 0, 4, 0, '', 1, 0, 100, 0, 10000, 10000, 23000, 30000, 0, '', 11, 90047, 0, 0, 0, 0, 0, 0, '', 9, 48419, 0, 55, 0, '', 0, 0, 0, 0, 'Defias Envoker - OOC - Cast Renegade Strength'),
(48338, 0, 0, 0, '', 0, 0, 100, 0, 5000, 5000, 23000, 39000, 0, '', 11, 91032, 64, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mine Bunny - IC - Cast Drunken Haze'),
(48351, 0, 0, 0, '', 0, 0, 100, 0, 5000, 5000, 23000, 39000, 0, '', 11, 91032, 64, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mine Bunny - IC - Cast Drunken Haze'),
(48278, 0, 0, 0, '', 0, 0, 100, 0, 0, 0, 2300, 3900, 0, '', 11, 91038, 64, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mining Monkey - IC - Cast Throw'),
(48278, 0, 1, 2, '', 2, 0, 100, 1, 0, 15, 1000, 1000, 0, '', 25, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mining Monkey - HP PCT 15 - Flee'),
(48278, 0, 2, 0, '', 61, 0, 100, 1, 0, 15, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mining Monkey - HP PCT 15 - Say 0'),
(48278, 0, 3, 0, '', 11, 0, 100, 0, 0, 0, 0, 0, 0, '', 103, 1, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mining Monkey - On Respawn - Apply ROOT'),
(48440, 0, 0, 0, '', 0, 0, 100, 0, 0, 0, 2300, 3900, 0, '', 11, 91038, 64, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mining Monkey - IC - Cast Throw'),
(48440, 0, 1, 2, '', 2, 0, 100, 1, 0, 15, 1000, 1000, 0, '', 25, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mining Monkey - HP PCT 15 - Flee'),
(48440, 0, 2, 0, '', 61, 0, 100, 1, 0, 15, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mining Monkey - HP PCT 15 - Say 0'),
(48440, 0, 3, 0, '', 11, 0, 100, 0, 0, 0, 0, 0, 0, '', 103, 1, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mining Monkey - On Respawn - Apply ROOT'),
(48441, 0, 0, 0, '', 0, 0, 100, 0, 0, 0, 2300, 3900, 0, '', 11, 91038, 64, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mining Monkey - IC - Cast Throw'),
(48441, 0, 1, 2, '', 2, 0, 100, 1, 0, 15, 1000, 1000, 0, '', 25, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mining Monkey - HP PCT 15 - Flee'),
(48441, 0, 2, 0, '', 61, 0, 100, 1, 0, 15, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mining Monkey - HP PCT 15 - Say 0'),
(48441, 0, 3, 0, '', 11, 0, 100, 0, 0, 0, 0, 0, 0, '', 103, 1, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mining Monkey - On Respawn - Apply ROOT'),
(48442, 0, 0, 0, '', 0, 0, 100, 0, 0, 0, 2300, 3900, 0, '', 11, 91038, 64, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mining Monkey - IC - Cast Throw'),
(48442, 0, 1, 2, '', 2, 0, 100, 1, 0, 15, 1000, 1000, 0, '', 25, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mining Monkey - HP PCT 15 - Flee'),
(48442, 0, 2, 0, '', 61, 0, 100, 1, 0, 15, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mining Monkey - HP PCT 15 - Say 0'),
(48442, 0, 3, 0, '', 11, 0, 100, 0, 0, 0, 0, 0, 0, '', 103, 1, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mining Monkey - On Respawn - Apply ROOT'),
(48229, 0, 0, 0, '', 0, 0, 100, 0, 5000, 5000, 9000, 12000, 0, '', 11, 89663, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Kobold Digger - Cast Candle Blast'),
(48229, 0, 1, 0, '', 4, 0, 30, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Kobold Digger - On Aggro - Say 0'),
(48279, 0, 0, 0, '', 4, 0, 100, 1, 0, 0, 0, 0, 0, '', 11, 91034, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Defias Overseer - On Aggro - Cast Threatening Shout'),
(48279, 0, 1, 0, '', 0, 0, 100, 0, 3000, 4000, 8000, 12000, 0, '', 11, 91036, 0, 0, 0, 0, 0, 0, '', 5, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Defias Overseer - In Combat - Cast Motivate');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` = 48445;
DELETE FROM `smart_scripts` WHERE `entryorguid` = 48445;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_chance`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `target_type`, `comment`) VALUES 
(48445, 0, 0, 0, '', 4, 100, 0, 0, 0, 0, 11, 90098, 2, 'Oaf Lackey Cast Axe to the Head on Aggro'),
(48445, 0, 1, 2, '', 2, 100, 0, 30, 1000, 1000, 11, 8599, 1, 'Oaf Lackey Cast Enrage at 30% HP'),
(48445, 0, 2, 0, '', 61, 100, 0, 0, 0, 0, 1, 0, 1, 'Oaf Lackey Say Text at 30% HP');

 -- Deadmines Foe Reaper Targeting Bunny smart ai
SET @ENTRY := 47468;
UPDATE `creature_template` SET `AIName` = 'SmartAI', `ScriptName` = '' WHERE `entry` = @ENTRY;
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` = @ENTRY;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`, `Difficulties`) VALUES
(@ENTRY, 0, 0, 0, 11, 0, 100, 0, 0, 36, 0, 0, 0, 11, 71371, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On respawn on map 36 - Self: Cast spell  71371 on Self', '');

 -- Molten Slag smart ai
SET @ENTRY := 49229;
UPDATE `creature_template` SET `AIName` = 'SmartAI', `ScriptName` = '' WHERE `entry` = @ENTRY;
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` = @ENTRY;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`, `Difficulties`) VALUES
(@ENTRY, 0, 0, 1, 11, 0, 100, 0, 0, 36, 0, 0, 0, 11, 1217811, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On respawn on map 36 - Self: Cast spell  1217811 on Self', ''),
(@ENTRY, 0, 1, 2, 61, 0, 100, 0, 0, 0, 0, 0, 0, 49, 0, 0, 0, 0, 0, 0, 0, 25, 100, 0, 0, 0, 0, 0, 0, 'On respawn on map 36 - Self: Attack Closest enemy creature in 50 yards', ''),
(@ENTRY, 0, 2, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 86, 91830, 0, 1, 0, 0, 0, 0, 25, 100, 0, 0, 0, 0, 0, 0, 'On respawn on map 36 - Self: Cast spell  91830 at Closest enemy creature in 100 yards', '');



-- Scripts
UPDATE `creature_template` SET `ScriptName` = 'npc_ogre_henchman' WHERE `entry` = 48230;
UPDATE `creature_template` SET `ScriptName` = 'npc_ogre_bodyguard' WHERE `entry` = 48262;
UPDATE `creature_template` SET `ScriptName` = 'npc_mining_powder' WHERE `entry` = 48284;
UPDATE `creature_template` SET `ScriptName` = 'boss_glubtok' WHERE `entry` = 47162;
UPDATE `creature_template` SET `ScriptName` = 'npc_glubtok_main_platter' WHERE `entry` = 48974;
UPDATE `creature_template` SET `ScriptName` = 'npc_glubtok_secondary_platter' WHERE `entry` IN (48975,48976,49039,49040,49041,49042);
UPDATE `creature_template` SET `ScriptName` = 'npc_helix_crew' WHERE `entry` = 49136; -- only heroic
UPDATE `creature_template` SET `ScriptName` = 'boss_helix_gearbreaker' WHERE `entry` = 47296;
UPDATE `creature_template` SET `ScriptName` = 'npc_lumbering_oaf' WHERE `entry` = 47297;
UPDATE `creature_template` SET `ScriptName` = 'npc_sticky_bomb' WHERE `entry` = 47314;
UPDATE `creature_template` SET `ScriptName` = 'npc_defias_overseer' WHERE `entry` = 48421;
UPDATE `creature_template` SET `ScriptName` = 'npc_goblin_foundry_worker' WHERE `entry` IN (48280,48439);
UPDATE `creature_template` SET `ScriptName` = 'npc_defias_foe_reaper_add' WHERE `entry` IN (47404,47403);
UPDATE `creature_template` SET `ScriptName` = 'boss_foe_reaper_5000' WHERE `entry` = 43778;

-- Spell Conditions
-- Delete conditions for OgrishMotivationNormal (89652)
DELETE FROM conditions WHERE SourceTypeOrReferenceId = 13 AND SourceEntry = 89652;

-- Delete conditions for OgrishMotivationHeroic (92747)
DELETE FROM conditions WHERE SourceTypeOrReferenceId = 13 AND SourceEntry = 92747;

-- For OgrishMotivationNormal (89652)
-- Condition 1: Target must be TYPEID_UNIT (creature only, not player)
INSERT INTO conditions (SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ElseGroup, ConditionTypeOrReference, ConditionTarget, ConditionValue1, ConditionValue2, ConditionValue3, NegativeCondition, ErrorType, ErrorTextId, ScriptName, COMMENT)
VALUES (13, 63, 89652, 0, 0, 51, 0, 5, 0, 0, 0, 0, 0, '', 'OgrishMotivationNormal - Target must be TYPEID_UNIT (creature only, not player)');

-- Condition 2: Target must NOT be hostile (exclude HATED and HOSTILE)
INSERT INTO conditions (SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ElseGroup, ConditionTypeOrReference, ConditionTarget, ConditionValue1, ConditionValue2, ConditionValue3, NegativeCondition, ErrorType, ErrorTextId, ScriptName, COMMENT)
VALUES (13, 63, 89652, 0, 0, 34, 0, 0, 3, 0, 1, 0, 0, '', 'OgrishMotivationNormal - Target must NOT be hostile (exclude HATED/HOSTILE)');

-- For OgrishMotivationHeroic (92747)
-- Condition 1: Target must be TYPEID_UNIT (creature only, not player)
INSERT INTO conditions (SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ElseGroup, ConditionTypeOrReference, ConditionTarget, ConditionValue1, ConditionValue2, ConditionValue3, NegativeCondition, ErrorType, ErrorTextId, ScriptName, COMMENT)
VALUES (13, 63, 92747, 0, 0, 51, 0, 5, 0, 0, 0, 0, 0, '', 'OgrishMotivationHeroic - Target must be TYPEID_UNIT (creature only, not player)');

-- Condition 2: Target must NOT be hostile (exclude HATED and HOSTILE)
INSERT INTO conditions (SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ElseGroup, ConditionTypeOrReference, ConditionTarget, ConditionValue1, ConditionValue2, ConditionValue3, NegativeCondition, ErrorType, ErrorTextId, ScriptName, COMMENT)
VALUES (13, 63, 92747, 0, 0, 34, 0, 0, 3, 0, 1, 0, 0, '', 'OgrishMotivationHeroic - Target must NOT be hostile (exclude HATED/HOSTILE)');

-- Foe Reaper Energize spell target bunnies
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 13 AND `SourceEntry` IN (89200);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `ConditionTypeOrReference`, `ConditionValue1`, `ConditionValue2`, `Comment`) VALUES 
(13, 1, 89200, 51, 5, 47242, 'Foe Reaper Energize target bunny'),
(13, 2, 89200, 51, 5, 47242, 'Foe Reaper Energize target bunny'),
(13, 3, 89200, 51, 5, 47242, 'Foe Reaper Energize target bunny');

-- Glubtok encounter
UPDATE `creature_template` SET `VehicleId` = '1367' WHERE (`entry` = '48976');
UPDATE `creature_template` SET `VehicleId` = '1368' WHERE (`entry` = '49041');
UPDATE `creature_template` SET `VehicleId` = '1369' WHERE (`entry` = '49042');

DELETE FROM `creature` WHERE `id` = 48974 AND `map` = 36;

-- Helix encounter
DELETE FROM `vehicle_seat_addon` WHERE `SeatEntry` = 8962;
INSERT INTO `vehicle_seat_addon` (`SeatEntry`, `SeatOrientation`) VALUES
(8962, 3.14159); -- Helix on players face

DELETE FROM `vehicle_template` WHERE `creatureId` IN (48974,48975,48976,49039,49040,49041,49042);
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`, `Pitch`, `CustomFlags`) VALUES
(48974, 0, NULL, 0),
(48975, 0, NULL, 0),
(48976, 0, NULL, 0),
(49039, 0, NULL, 0),
(49040, 0, NULL, 0),
(49041, 0, NULL, 0),
(49042, 0, NULL, 0);

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (48974,48975,48976,49039,49040,49041,49042,48266,49208,47403,47404);
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES
(49208, 46598, 1, 0), -- Prototype Reaper Ride Vehicle Spell
(47403, 46598, 1, 0), -- Defias Reaper Ride Vehicle Spell
(47404, 46598, 1, 0), -- Defias Watcher Ride Vehicle Spell
(48266, 46598, 1, 0),
(48974, 46598, 0, 0),
(48975, 46598, 0, 0),
(48976, 46598, 0, 0),
(49039, 46598, 0, 0),
(49040, 46598, 0, 0),
(49041, 46598, 0, 0),
(49042, 46598, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` IN (91397,87897,87900,89769,88278);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(88278, 'spell_helix_force_player_to_ride_oaf'),
(89769, 'spell_mining_powder_explode'),
(91397, 'spell_glubtok_firewall_targetting'),
(87897, 'spell_glubtok_generic_proc'),
(87900, 'spell_glubtok_generic_proc');

DELETE FROM `creature_template_spell` WHERE `CreatureID` IN (48266,49208,47403,47404);
INSERT INTO `creature_template_spell` (`CreatureID`, `Index`, `Spell`, `VerifiedBuild`) VALUES
(48266, 0, 91788, 39653), -- Defias Cannon Cannonball
(49208, 0, 91723, 30653), -- Prototype Reaper Reaper Strike 91734
(49208, 1, 91726, 30653), -- Prototype Reaper Charge 91735
(49208, 2, 91727, 30653), -- Prototype Reaper Pressurized Strike 91736
(47403, 0, 91723, 30653), -- Defias Reaper Reaper Strike
(47403, 1, 91726, 30653), -- Defias Reaper Charge
(47403, 2, 91727, 30653), -- Defias Reaper Pressurized Strike
(47404, 0, 91723, 30653), -- Defias Watcher Reaper Strike
(47404, 1, 91726, 30653), -- Defias Watcher Charge
(47404, 2, 91727, 30653); -- Defias Watcher Pressurized Strike

DELETE FROM `areatrigger_scripts` WHERE `Entry` IN (6350,6508,6353);
INSERT INTO `areatrigger_scripts` (`Entry`, `ScriptName`) VALUES
(6350, 'deadmines_goblin_foundry_spawn_at'),
(6508, 'deadmines_goblin_foundry_trigger1_at'),
(6353, 'deadmines_goblin_foundry_trigger2_at');

DELETE FROM `creature_text` WHERE `CreatureID` IN (47162,48974,48230,48229,48262);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES 
-- Ogre Bodyguard
(48262, 0, 0, 'Me smash! You die!', 12, 0, 100, 0, 0, 0, 0, 1926, 0, 'Ogre Bodyguard - Aggro'),
(48262, 0, 1, 'Raaar!!! Me smash $r!', 12, 0, 100, 0, 0, 0, 0, 1927, 0, 'Ogre Bodyguard - Aggro'),
(48262, 0, 2, 'I''ll crush you!', 12, 0, 100, 0, 0, 0, 0, 1925, 0, 'Ogre Bodyguard - Aggro'),
(48262, 1, 0, '%s goes into a frenzy!', 16, 0, 100, 0, 0, 0, 0, 0, 0, 'Ogre Bodyguard - combat Frenzy'),
(48262, 2, 0, 'Me da best bodyguard!', 12, 0, 100, 0, 0, 0, 0, 48523, 0, 'Ogre Bodyguard - Conversation 1'),
(48262, 3, 0, 'No, me!', 12, 0, 100, 0, 0, 0, 0, 48524, 0, 'Ogre Bodyguard - Conversation 2'),
(48262, 4, 0, 'Me got a big board for hit things!', 12, 0, 100, 0, 0, 0, 0, 48525, 0, 'Ogre Bodyguard - Conversation 3'),
(48262, 5, 0, 'Mine gots a nail in it!', 12, 0, 100, 0, 0, 0, 0, 48526, 0, 'Ogre Bodyguard - Conversation 4'),
(48262, 6, 0, 'Mine gots a bigger nail!', 12, 0, 100, 0, 0, 0, 0, 48527, 0, 'Ogre Bodyguard - Conversation 5'),
(48262, 7, 0, 'Mine gots a RUSTY nail!', 12, 0, 100, 0, 0, 0, 0, 48528, 0, 'Ogre Bodyguard - Conversation 6'),
(48262, 8, 0, 'Dats dangerous.', 12, 0, 100, 0, 0, 0, 0, 48529, 0, 'Ogre Bodyguard - Conversation 7'),
(48262, 9, 0, 'Only for da bad guys.', 12, 0, 100, 0, 0, 0, 0, 48530, 0, 'Ogre Bodyguard - Conversation 8'),
(48262, 10, 0, 'But I''m da best at watchin'' da door.', 12, 0, 100, 0, 0, 0, 0, 48531, 0, 'Ogre Bodyguard - Conversation 9'),
(48262, 11, 0, 'No I am!', 12, 0, 100, 0, 0, 0, 0, 48532, 0, 'Ogre Bodyguard - Conversation 10'),
-- Kobold Digger
(48229, 0, 0, 'You no take candle!', 12, 0, 100, 0, 0, 0, 0, 1868, 0, 'Kobold Digger - Aggro'),
-- Ogre Henchman
(48230, 0, 0, 'You hit rock or me hit you!', 12, 0, 100, 0, 0, 0, 0, 48443, 0, 'Ogre Henchman - OOC'),
(48230, 0, 1, 'You want me hit rock with you head?', 12, 0, 100, 0, 0, 0, 0, 48444, 0, 'Ogre Henchman - OOC'),
(48230, 0, 2, 'More smash rock! Rar!', 12, 0, 100, 0, 0, 0, 0, 48445, 0, 'Ogre Henchman - OOC'),
(48230, 0, 3, 'Slow kobold look tasty!', 12, 0, 100, 0, 0, 0, 0, 48447, 0, 'Ogre Henchman - OOC'),
(48230, 0, 4, 'Me has whole box of candles!  Maybe give you some if hit rock good!', 12, 0, 100, 0, 0, 0, 0, 48448, 0, 'Ogre Henchman - OOC'),
(48230, 0, 5, 'You want get punched?  You make rock die now!', 12, 0, 100, 0, 0, 0, 0, 48445, 0, 'Ogre Henchman - OOC'),
(48230, 0, 6, 'Me rip slow kobold to pieces!', 12, 0, 100, 0, 0, 0, 0, 48450, 0, 'Ogre Henchman - OOC'),
(48230, 0, 7, 'You too slow!  Want me get Glubtok?', 12, 0, 100, 0, 0, 0, 0, 48451, 0, 'Ogre Henchman - OOC'),
(48230, 1, 0, 'Dis look warm!  Me getting sleepy...', 12, 0, 100, 0, 0, 0, 0, 48452, 0, 'Ogre Henchman - OOC'),
(48230, 0, 8, 'Look at all dem boxes.  What in dis box?', 12, 0, 100, 0, 0, 0, 0, 48455, 0, 'Ogre Henchman - OOC'),
(48230, 2, 0, 'Why stinky candle talk so much?', 12, 0, 100, 0, 0, 0, 0, 48461, 0, 'Ogre Henchman - OOC'),
(48230, 3, 0, 'Me smash! You die!', 12, 0, 100, 0, 0, 0, 0, 1926, 0, 'Ogre Henchman - Aggro'),
(48230, 3, 1, 'Raaar!!! Me smash $r!', 12, 0, 100, 0, 0, 0, 0, 1927, 0, 'Ogre Henchman - Aggro'),
(48230, 3, 2, 'I''ll crush you!', 12, 0, 100, 0, 0, 0, 0, 1925, 0, 'Ogre Henchman - Aggro'),
-- Glubtok Encounter
(48974, 0, 0, '|TInterface\Icons\spell_holy_innerfire.blp:20|t Glubtok creates a moving |cFFFF0000|Hspell:91398|h[Fire Wall]|h|r!', 16, 0, 0, 0, 0, 0, 0, 49155, 0, 'Glubtok Firewall'),
(47162, 0, 0, 'TOO...MUCH...POWER!!!', 14, 0, 100, 15, 0, 21145, 0, 47422, 0, 'VO_DM_GlubtokBoth_Death01'),
(47162, 1, 0, 'ARCANE POWER!!!', 14, 0, 100, 15, 0, 21146, 0, 47363, 0, 'VO_DM_GlubtokBoth_Spell03'),
(47162, 2, 0, 'Glubtok show you da power of de arcane!', 14, 0, 100, 0, 0, 21151, 0, 47256, 0, 'VO_DM_GlubtokHead1_Aggro01'),
(47162, 3, 0, 'Ha ha ha ha!', 14, 0, 100, 11, 0, 21152, 0, 40112, 0, 'VO_DM_GlubtokHead1_Kill01'),
(47162, 3, 1, '''Sploded dat one!', 14, 0, 0, 0, 0, 21155, 0, 47434, 0, 'VO_DM_GlubtokHead2_Kill01'),
(47162, 4, 0, 'Fists of flame!', 14, 0, 100, 0, 0, 21153, 0, 47239, 0, 'VO_DM_GlubtokHead1_Spell01'),
(47162, 5, 0, 'Glubtok ready?', 14, 0, 100, 1, 0, 21154, 0, 47361, 0, 'VO_DM_GlubtokHead1_Spell02'),
(47162, 6, 0, 'Fists of frost!', 14, 0, 100, 0, 0, 21156, 0, 47238, 0, 'VO_DM_GlubtokHead2_Spell01'),
(47162, 7, 0, 'Let''s do it!', 14, 0, 100, 15, 0, 21157, 0, 108750, 0, 'VO_DM_GlubtokHead2_Spell02');

DELETE FROM `creature_text` WHERE `CreatureID` IN (48440,48441,48442,48278,48445,48418);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `BroadcastTextId`, `comment`) VALUES 
(48445, 0, 0, '%s becomes enraged!', 16, 100, 7798, 'Oaf Lackey Enraged'),
(48418, 0, 0, '%s attempts to run away in fear!', 16, 100, 1150, 'Defias Envoker Flee'),
(48278, 0, 0, '%s attempts to run away in fear!', 16, 100, 1150, 'Mining Monkey Flee'),
(48440, 0, 0, '%s attempts to run away in fear!', 16, 100, 1150, 'Mining Monkey Flee'),
(48441, 0, 0, '%s attempts to run away in fear!', 16, 100, 1150, 'Mining Monkey Flee'),
(48442, 0, 0, '%s attempts to run away in fear!', 16, 100, 1150, 'Mining Monkey Flee');

DELETE FROM `creature_text` WHERE `CreatureID` IN (47297,49136,47296);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `Sound`, `BroadcastTextId`, `comment`) VALUES 
(47296, 0, 0, 'The scales...have...tipped...', 14, 100, 20844, 47575, 'Helix Death'),
(47296, 1, 0, 'Only ten copper? You\'re not even worth killing!', 14, 100, 20845, 47574, 'Helix Kill'),
(47296, 2, 0, 'I didn''t need him! Not when I''ve got YOU oafs!', 14, 100, 20846, 47573, 'Helix Oaf Dead'),
(47296, 3, 0, 'Bombs away!', 14, 100, 20847, 47475, 'Helix Bombs Attack'),
(47296, 4, 0, 'Ready Oafie? Throw!', 14, 100, 20848, 47476, 'Helix Face Riding attack'),
(47296, 5, 0, 'The mistress will pay me handsomely for your heads!', 14, 100, 20849, 47474, 'Helix Aggro'),
(47296, 6, 0, 'Helix attaches a bomb to $n''s chest!', 41, 100, 0, 47544, 'Helix Chest Bomb'),
(49136, 0, 0, 'Blowin ''em to bits, boss!', 12, 100, 0, 46330, 'Helix Crew Sticky Bombs'),
(47297, 0, 0, 'No...NO!', 14, 100, 0, 47480, 'Lumbering Oaf charge'),
(47297, 1, 0, 'OAF SMASH!!', 14, 100, 0, 48117, 'Lumbering Oaf smash');

DELETE FROM `creature_text` WHERE `CreatureID` = 48421; -- Defias Overseer
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `Emote`, `BroadcastTextID`, `comment`) VALUES 
(48421, 0, 0, 'It''s broken.', 12, 100, 25, 48628, 'Defias Overseer 1'),
(48421, 1, 0, 'It''s not broken!', 12, 100, 274, 48629, 'Defias Overseer 2'),
(48421, 2, 0, 'Why''s it shooting steam out of the side there, then?', 12, 100, 6, 48630, 'Defias Overseer 3'),
(48421, 3, 0, 'That''s the...pressure release valve.', 12, 100, 396, 48631, 'Defias Overseer 4'),
(48421, 4, 0, 'In the middle of the pipe?', 12, 100, 6, 48632, 'Defias Overseer 5'),
(48421, 5, 0, 'Er...backup release valve?', 12, 100, 6, 48633, 'Defias Overseer 6'),
(48421, 6, 0, 'We should tell the Admiral.', 12, 100, 396, 48634, 'Defias Overseer 7'),
(48421, 7, 0, 'You tell him.', 12, 100, 396, 48636, 'Defias Overseer 8'),
(48421, 8, 0, 'No way!  He gives me the creeps!', 12, 100, 274, 48637, 'Defias Overseer 9'),
(48421, 9, 0, 'I know!  The way he looks at you with those hungry eyes.  I''m afraid to even turn my back!', 12, 100, 5, 48638, 'Defias Overseer 10'),
(48421, 10, 0, '%s attempts to run away in fear!', 16, 100, 0, 1150, 'Defias Overseer Flee');

DELETE FROM `creature_text` WHERE `CreatureID` = 43778;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `Sound`, `BroadcastTextID`, `comment`) VALUES 
(43778, 0, 0, 'Foe Reaper 5000 on-line. All systems nominal.', 14, 100, 22137, 47596, 'Foereaper Aggro'),
(43778, 1, 0, 'Overheat threshold exceeded. System failure. Wheat clog in port two. Shutting down.', 14, 100, 22138, 47643, 'Foereaper Death'),
(43778, 2, 0, 'Target destroyed', 14, 100, 22139, 47630, 'Foereaper kill'),
(43778, 3, 0, 'Target acquired. Harvesting servos engaged.', 14, 100, 22141, 47629, 'Foereaper Spell Harvesting'),
(43778, 4, 0, 'Acquiring target...', 14, 100, 22140, 47628, 'Foereaper Harvest Targetting'),
(43778, 5, 0, 'Overdrive engine activated', 14, 100, 22142, 47609, 'Foereaper Spell Overdrive'),
(43778, 6, 0, '|TInterface\Icons\ability_whirlwind.blp:20|t Foe Reaper 5000 begins to activate |cFFFF0000|Hspell:88481|h[Overdrive]|h|r!', 16, 100, 0, 48812, 'Foereaper Spell Overdrive'),
(43778, 7, 0, 'Safety restrictions off-line. Catastrophic system failure imminent.', 14, 100, 22143, 47642, 'Foereaper Spell Safety'),
(43778, 8, 0, '|TInterface\Icons\spell_fire_totemofwrath.blp:20|t Foe Reaper 5000 |cFFFF0000|Hspell:88522|h[Safety Restrictions are Off-line]|h|r!', 16, 100, 0, 50713, 'Foereaper Spell Safety'),
(43778, 9, 0, 'A stray jolt from the Foe Reaper has disrupted the foundry controls!', 41, 100, 0, 49315, 'Foereaper Init'),
(43778, 10, 0, 'The molten slag begins to bubble furiously!', 41, 100, 0, 49316, 'Foereaper Molten Slags');

DELETE FROM `creature_text` WHERE `CreatureID` IN (48280,48439);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `Sound`, `BroadcastTextID`, `comment`) VALUES 
(48280, 0, 0, 'They''re alive...', 12, 100, 0, 48475, 'Goblin Craftsman 1'),
(48280, 1, 0, 'THEY''RE ALIVE!!!', 14, 100, 0, 48476, 'Goblin Craftsman 2'),
(48280, 2, 0, 'NOOOOOOO!!!', 14, 100, 0, 48478, 'Goblin Craftsman 3'),
(48439, 0, 0, 'RUN!', 14, 100, 0, 48477, 'Goblin Engineer 1');

DELETE FROM `creature_text` WHERE `CreatureID` IN (45979);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `Sound`, `BroadcastTextID`, `comment`) VALUES 
(45979, 0, 0, 'The cannon was rigged with an explosive trap!', 41, 100, 0, 48790, 'Bunny for Defias Cannon');


-- GO: 207079 Ball and Chain
UPDATE `gameobject_template` SET `ScriptName` = 'go_ball_and_chain' WHERE `entry` = 207079;
-- GO: 208002 Goblin Teleporter
UPDATE `gameobject_template` SET `ScriptName` = 'go_goblin_teleporter' WHERE `entry` = 208002;

DELETE FROM `gossip_menu_option` WHERE `MenuID` = 12691;
INSERT INTO `gossip_menu_option` (`MenuID`, `GossipOptionID`, `OptionID`, `OptionText`, `OptionBroadcastTextID`, `VerifiedBuild`) VALUES
(12691, -3248896, 0, 'Press the button labeled \'Wood and Lumber.\'', 50716, 25996),
(12691, -3248897, 1, 'Press the button labeled \'Metal and Scraps.\'', 50717, 25937),
(12691, -3248898, 2, 'Press the button labeled \'Ship Parts.\'', 50718, 25996);

-- Gameobjects
SET @OGUID := 900000;
DELETE FROM `gameobject` WHERE `guid` = @OGUID+106;
INSERT INTO `gameobject` (guid, id, map, spawndifficulties, phaseUseFlags, phaseid, position_x, position_y, position_z, orientation, rotation2, rotation3, spawntimesecs, state) VALUES
-- Defias Gunpowder
(@OGUID+106, 17155, 	36, 1, 0, 0, -106.409, -617.284, 13.8495, -0.767945, -0.374607, 0.927184, 180, 1);

-- Deadmines Loot Tables

DELETE FROM `creature_loot_template` WHERE `Entry` IN (43778,4377801,4377802,47296,4729601,4729602,47162,4716201,4716202);
INSERT INTO `creature_loot_template` (`Entry`, `ItemType`, `Item`, `Chance`, `QuestRequired`, `LootMode`, `GroupID`, `MinCount`, `MaxCount`, `Comment`) VALUES
-- Glubtok loot normal and heroic
(4716201, 0, 5444, 37, 0, 1, 0, 1, 1, 'Glubtok Miner Cape'),
(4716201, 0, 2169, 32, 0, 1, 0, 1, 1, 'Glubtok Buzzer Blade'),
(4716201, 0, 5195, 32, 0, 1, 0, 1, 1, 'Glubtok Gold-Flecked Gloves'),

(4716202, 0, 65163, 21, 0, 1, 0, 1, 1, 'Glubtok Heroic Buzzer Blade'),
(4716202, 0, 63468, 20, 0, 1, 0, 1, 1, 'Glubtok Heroic Defias Brotherhood Vest'),
(4716202, 0, 63470, 20, 0, 1, 0, 1, 1, 'Glubtok Heroic Missing Diplomat Pauldrons'),
(4716202, 0, 63471, 20, 0, 1, 0, 1, 1, 'Glubtok Heroic Vest of the curious visitor'),
(4716202, 0, 63467, 20, 0, 1, 1, 1, 1, 'Glubtok Heroic Shadow of the past'),
(4716202, 0, 157628, 19, 0, 1, 0, 1, 1, 'Glubtok Heroic Gold-Flecked Gloves'),
-- Helix loot normal and heroic
(4729601, 0, 5199, 25, 0, 1, 0, 1, 1, 'Helix Smelting Pants'),
(4729601, 0, 5443, 22, 0, 1, 0, 1, 1, 'Helix Gold-Plated Buckler'),
(4729601, 0, 5191, 20, 0, 1, 0, 1, 1, 'Helix Cruel Barb'),
(4729601, 0, 5200, 19, 0, 1, 0, 1, 1, 'Helix Impaling Harpoon'),
(4729601, 0, 151062, 6, 0, 1, 0, 1, 1, 'Helix Armbands of Exiled Architects'),
(4729601, 0, 132556, 6, 0, 1, 0, 1, 1, 'Helix Smelters Britches'),
(4729601, 0, 151063, 4, 0, 1, 0, 1, 1, 'Helix Gear-Marked Gauntlets'),

(4729602, 0, 65164, 21, 0, 1, 0, 1, 1, 'Helix Cruel Barb'),
(4729602, 0, 63474, 20, 0, 1, 0, 1, 1, 'Helix Gear-Marked Gauntlets'),
(4729602, 0, 63476, 20, 0, 1, 0, 1, 1, 'Helix Gearbreaker Bindings'),
(4729602, 0, 63475, 20, 0, 1, 0, 1, 1, 'Helix Old Friends Gloves'),
(4729602, 0, 63473, 19, 0, 1, 0, 1, 1, 'Helix Cloak of Thredd'),
(4729602, 0, 157752, 1.4, 0, 1, 0, 1, 1, 'Helix Armbands of Exiled Architects'),
-- Foereaper loot normal and heroic
(4377801, 0, 5201, 31, 0, 1, 0, 1, 1, 'Foereaper Emberstone Staff'),
(4377801, 0, 1937, 28, 0, 1, 0, 1, 1, 'Foereaper Buzz Saw'),
(4377801, 0, 5187, 28, 0, 1, 0, 1, 1, 'Foereaper Foe Reaper'),
(4377801, 0, 151066, 4, 0, 1, 0, 1, 1, 'Foereaper Missing Diplomat Pauldrons'),
(4377801, 0, 151064, 4, 0, 1, 0, 1, 1, 'Foereaper Vest of the Curious Visitor'),
(4377801, 0, 151065, 3, 0, 1, 0, 1, 1, 'Foereaper Old Friends Gloves'),

(4377801, 0, 65167, 32, 0, 1, 0, 1, 1, 'Foereaper Emberstone Staff'),
(4377801, 0, 65166, 31, 0, 1, 0, 1, 1, 'Foereaper Buzz Saw'),
(4377801, 0, 65165, 31, 0, 1, 0, 1, 1, 'Foereaper Foe Reaper'),
(4377801, 0, 157755, 1, 0, 1, 0, 1, 1, 'Foereaper Missing Diplomat Pauldrons'),
(4377801, 0, 157753, 1, 0, 1, 0, 1, 1, 'Foereaper Vest of the Curious Visitor'),
(4377801, 0, 157754, 1, 0, 1, 0, 1, 1, 'Foereaper Old Friends Gloves');
