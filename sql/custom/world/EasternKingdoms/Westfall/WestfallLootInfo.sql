-- Westfall Loot Zone 2

-- First we remove ALL loot items for Westfall specific mobs to avoid leftovers
-- Other delete queries below may be redundant however needed for single operations
DELETE FROM creature_loot_template 
WHERE Entry IN (95,98,114,115,117,121,122,123,124,126,127,154,157,171,199,391,449,452,453,454,456,458,462,480,481,
500,501,504,513,515,517,519,550,589,590,594,830,831,832,834,1065,1109,1236,1424,6250,42357,42653,42669, 42677);

-- Delete Skinning Loot
DELETE FROM skinning_loot_template 
WHERE Entry IN (95,98,114,115,117,121,122,123,124,126,127,154,157,171,199,391,449,452,453,454,456,458,462,480,481,
500,501,504,513,515,517,519,550,589,590,594,830,831,832,834,1065,1109,1236,1424,6250,42357,42653,42669, 42677);

-- Quest Items are distributed PER creature entry
DELETE FROM creature_loot_template
WHERE (Entry, Item) IN (
    (98, 725),
    (114, 57911),
    (114, 57935),
    (124, 58111),
    (126, 1357),
    (126, 57756),
	(515, 57756),
    (154, 57786),
    (157, 57788),
    (391, 3636),
    (500, 57755),
	(117, 57755),
    (550, 1381),
    (834, 57787),
    (42669, 58204),
    (42677, 58117),
    (42677, 58118),
	(594, 915),
	(619, 915),
	(824, 915),
	(1726, 915),
	(1727, 915),
	(48417, 915),
	(48418, 915),
	(48419, 915),
	(48420, 915),
	(48421, 915),
	(48521, 915),
	(48522, 915),
	(84697, 829),
	(456, 1357),
	(515, 1357),
	(513, 1357),
	(95, 829),
	(121, 829),
	(122, 829),
	(449, 829),
	(450, 829),
	(481, 829),
	(502, 829),
	(504, 829),
	(589, 829),
	(590, 829)
);

INSERT INTO `creature_loot_template` (`Entry`, `ItemType`, `Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(84697, 0, 829, 80, 1, 1, 0, 1, 1, 'Red Leather Bandana'),
(95, 0, 829, 80, 1, 1, 0, 1, 1, 'Red Leather Bandana'),
(121, 0, 829, 80, 1, 1, 0, 1, 1, 'Red Leather Bandana'),
(122, 0, 829, 80, 1, 1, 0, 1, 1, 'Red Leather Bandana'),
(449, 0, 829, 80, 1, 1, 0, 1, 1, 'Red Leather Bandana'),
(450, 0, 829, 80, 1, 1, 0, 1, 1, 'Red Leather Bandana'),
(481, 0, 829, 80, 1, 1, 0, 1, 1, 'Red Leather Bandana'),
(502, 0, 829, 80, 1, 1, 0, 1, 1, 'Red Leather Bandana'),
(504, 0, 829, 80, 1, 1, 0, 1, 1, 'Red Leather Bandana'),
(589, 0, 829, 80, 1, 1, 0, 1, 1, 'Red Leather Bandana'),
(590, 0, 829, 80, 1, 1, 0, 1, 1, 'Red Leather Bandana'),

(594, 0, 915, 75, 1, 1, 0, 1, 1, 'Red Silk Bandana'),
(619, 0, 915, 80, 1, 1, 0, 1, 1, 'Red Silk Bandana'),
(824, 0, 915, 80, 1, 1, 0, 1, 1, 'Red Silk Bandana'),
(1726, 0, 915, 80, 1, 1, 0, 1, 1, 'Red Silk Bandana'),
(1727, 0, 915, 80, 1, 1, 0, 1, 1, 'Red Silk Bandana'),
(48417, 0, 915, 75, 1, 1, 0, 1, 1, 'Red Silk Bandana'),
(48418, 0, 915, 75, 1, 1, 0, 1, 1, 'Red Silk Bandana'),
(48419, 0, 915, 75, 1, 1, 0, 1, 1, 'Red Silk Bandana'),
(48420, 0, 915, 75, 1, 1, 0, 1, 1, 'Red Silk Bandana'),
(48421, 0, 915, 75, 1, 1, 0, 1, 1, 'Red Silk Bandana'),
(48521, 0, 915, 75, 1, 1, 0, 1, 1, 'Red Silk Bandana'),
(48522, 0, 915, 75, 1, 1, 0, 1, 1, 'Red Silk Bandana'),

(98, 0, 725, 75, 1, 1, 0, 1, 1, 'Gnoll Paw'),
(114, 0, 57911, 100, 1, 1, 0, 1, 1, 'Okra'),
(114, 0, 57935, 100, 1, 1, 0, 1, 1, 'Harvest Watcher Heart'),
(124, 0, 58111, 100, 1, 1, 0, 1, 1, 'Gnoll Attack Orders'),
(126, 0, 1357, 10, 1, 1, 0, 1, 1, 'Captain Sanders Treasure Map'),
(456, 0, 1357, 10, 1, 1, 0, 1, 1, 'Captain Sanders Treasure Map'),
(515, 0, 1357, 10, 1, 1, 0, 1, 1, 'Captain Sanders Treasure Map'),
(513, 0, 1357, 10, 1, 1, 0, 1, 1, 'Captain Sanders Treasure Map'),
(126, 0, 57756, 20, 1, 1, 0, 1, 1, 'Murloc Clue'),
(515, 0, 57756, 20, 1, 1, 0, 1, 1, 'Murloc Clue'),
(154, 0, 57786, 100, 1, 1, 0, 1, 1, 'Stringy Fleshripper Meat'),
(157, 0, 57788, 100, 1, 1, 0, 1, 1, 'Goretusk Flank'),
(391, 0, 3636, 100, 1, 1, 0, 1, 1, 'Scale of Old Murk-Eye'),
(500, 0, 57755, 30, 1, 1, 0, 1, 1, 'Riverpaw Gnoll Clue'),
(117, 0, 57755, 30, 1, 1, 0, 1, 1, 'Riverpaw Gnoll Clue'),
(550, 0, 1381, 100, 1, 1, 0, 1, 1, 'A Mysterious Message'),
(834, 0, 57787, 100, 1, 1, 0, 1, 1, 'Coyote Tail'),
(42669, 0, 58204, 100, 1, 1, 0, 1, 1, 'Chasm Ooze'),
(42677, 0, 58117, 100, 1, 1, 0, 1, 1, 'Red Bandana'),
(42677, 0, 58118, 100, 1, 1, 0, 1, 1, 'Red Bandana');

-- Specific Mob Flavor items
DELETE FROM `creature_loot_template`
WHERE (Entry, Item) IN (
    (519, 3188),
    (519, 6180),
    (462, 4454),
    (462, 115349),
    (462, 5971),
    (462, 555),
    (199, 555),
    (1109, 555),
    (154, 555),
    (199, 729),
    (1109, 729),
    (462, 729),
    (154, 729),
    (114, 732),
    (115, 732),
    (36, 732),
    (480, 732),
    (832, 2091),
	(832, 4290),
	(832, 7069),
    (42669, 20766),
    (42669, 20767),
    (42669, 20768),
    (834, 2673),
    (834, 2672),
    (513, 835),
	(114, 1274),
    (115, 1274),
    (36, 1274),
    (480, 1274),
	(115, 820),
	(1424, 6205),
	(1424, 6206),
	(517, 1405),
	(453, 1391),
    (123, 821)
);

INSERT INTO `creature_loot_template` (`Entry`, `ItemType`, `Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(519, 0, 3188, 20, 0, 1, 2, 1, 1, 'Coral Claymore - Slark'),
(519, 0, 6180, 30, 0, 1, 2, 1, 1, 'Slarkskin - Slark'),
(462, 0, 4454, 0, 0, 1, 2, 1, 1, 'Talon of Vultros - Vultros'),
(462, 0, 115349, 3, 0, 1, 2, 1, 1, 'Talon of Vultros - Vultros'),
(462, 0, 5971, 67, 0, 1, 2, 1, 1, 'Feathered Cape - Vultros'),
(462, 0, 555, 0.143063, 0, 1, 0, 1, 1, 'Rough Vulture Feathers - Vultros'),
(199, 0, 555, 0.143063, 0, 1, 0, 1, 1, 'Rough Vulture Feathers - Young Fleshripper'),
(1109, 0, 555, 0.143063, 0, 1, 0, 1, 1, 'Rough Vulture Feathers - Fleshripper'),
(154, 0, 555, 0.143063, 0, 1, 0, 1, 1, 'Rough Vulture Feathers - Greater Fleshripper'),
(199, 0, 729, 0.155337, 0, 1, 0, 1, 1, 'Stringy Vulture Meat - Young Fleshripper'),
(1109, 0, 729, 0.155337, 0, 1, 0, 1, 1, 'Stringy Vulture Meat - Fleshripper'),
(462, 0, 729, 0.155337, 0, 1, 0, 1, 1, 'Stringy Vulture Meat - Vultros'),
(154, 0, 729, 0.155337, 0, 1, 0, 1, 1, 'Stringy Vulture Meat - Greater Fleshripper'),
(114, 0, 732, 0.331584, 0, 1, 0, 1, 1, 'Ripe Okra - Harvest Watcher'),
(115, 0, 732, 0.331584, 0, 1, 0, 1, 1, 'Ripe Okra - Harvest Reaper'),
(36, 0, 732, 0.331584, 0, 1, 0, 1, 1, 'Ripe Okra - Harvest Golem'),
(480, 0, 732, 0.331584, 0, 1, 0, 1, 1, 'Ripe Okra - Rusty Harvest Golem'),
(114, 0, 1274, 0.331584, 0, 1, 0, 1, 1, 'Hops - Harvest Watcher'),
(115, 0, 1274, 0.331584, 0, 1, 0, 1, 1, 'Hops - Harvest Reaper'),
(36, 0, 1274, 0.331584, 0, 1, 0, 1, 1, 'Hops - Harvest Golem'),
(480, 0, 1274, 0.331584, 0, 1, 0, 1, 1, 'Hops - Rusty Harvest Golem'),
(832, 0, 2091, 28.7356, 0, 1, 0, 1, 1, 'Magic Dust - Unbound Cyclone'),
(832, 0, 7069, 1.7356, 0, 1, 0, 1, 1, 'Elemental Air - Unbound Cyclone'),
(832, 0, 4290, 1.7356, 0, 1, 0, 1, 1, 'Dust Bowl - Unbound Cyclone'),
(42669, 0, 20766, 0.661805, 0, 1, 2, 1, 1, 'Slimy Bag - Chasm Slime'),
(42669, 0, 20767, 0.247974, 0, 1, 2, 1, 1, 'Scum Covered Bag - Chasm Slime'),
(42669, 0, 20768, 0.476434, 0, 1, 2, 1, 1, 'Oozing Bag - Chasm Slime'),
(834, 0, 2673, 0.120386, 0, 1, 1, 1, 1, 'Coyote Meat - Coyote'),
(834, 0, 2672, 0.103611, 0, 1, 1, 1, 1, 'Stringy Wolf Meat - Coyote'),
(513, 0, 835, 4.503, 0, 1, 0, 1, 1, 'Large Rope Net - Murloc Netter'),
(123, 0, 821, 3, 0, 1, 0, 1, 1, 'Riverpaw Leather Vest - Riverpaw Mongrel'),
(453, 0, 1391, 3.89759, 0, 1, 26, 1, 1, 'Riverpaw Mystic Staff - Riverpaw Mystic'),
(1424, 0, 6206, 30, 0, 1, 1, 1, 1, 'Rock Chipper - Master Digger'),
(1424, 0, 6205, 10, 0, 1, 1, 1, 1, 'Borrowing Shovel - Master Digger'),
(517, 0, 1405, 1, 0, 1, 26, 1, 1, 'Foamspittle Staff - Murloc Oracle'),
(115, 0, 820, 1, 0, 1, 0, 1, 1, 'Slicer Blade - Harvest Reaper');

-- Loot tables
-- Delete only reference loot items (ItemType = 1) for specific NPCs
DELETE FROM creature_loot_template 
WHERE Entry IN (95,98,114,115,117,121,122,123,124,126,127,154,157,171,199,391,449,452,453,454,456,458,462,480,481,
500,501,504,513,515,517,519,550,589,590,594,830,831,832,834,1065,1109,1236,1424,6250,42357,42653,42669, 42677) 
AND ItemType = 1;

-- NPC: 42677 Moonbrook Thug
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(42677, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(42677, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(42677, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(42677, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(42677, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(42677, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(42677, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');


-- NPC: 550 Defias Messenger
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(550, 1, 25, 100, 0, 1, 1, 1, 'Defias Mobs (Westfall Specific)'),
(550, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(550, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(550, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(550, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(550, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(550, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(550, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 95 Defias Smuggler
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(95, 1, 25, 100, 0, 1, 1, 1, 'Defias Mobs (Westfall Specific)'),
(95, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(95, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(95, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(95, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(95, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(95, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(95, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 121 Defias Pathstalker
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(121, 1, 25, 100, 0, 1, 1, 1, 'Defias Mobs (Westfall Specific)'),
(121, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(121, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(121, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(121, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(121, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(121, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(121, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 122 Defias Highwayman
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(122, 1, 25, 100, 0, 1, 1, 1, 'Defias Mobs (Westfall Specific)'),
(122, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(122, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(122, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(122, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(122, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(122, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(122, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 449 Defias Knuckleduster
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(449, 1, 25, 100, 0, 1, 1, 1, 'Defias Mobs (Westfall Specific)'),
(449, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(449, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(449, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(449, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(449, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(449, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(449, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 481 Defias Footpad
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(481, 1, 25, 100, 0, 1, 1, 1, 'Defias Mobs (Westfall Specific)'),
(481, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(481, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(481, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(481, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(481, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(481, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(481, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 504 Defias Trapper
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(504, 1, 25, 100, 0, 1, 1, 1, 'Defias Mobs (Westfall Specific)'),
(504, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(504, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(504, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(504, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(504, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(504, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(504, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 589 Defias Pillager
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(589, 1, 25, 100, 0, 1, 1, 1, 'Defias Mobs (Westfall Specific)'),
(589, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(589, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(589, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(589, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(589, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(589, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(589, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 590 Defias Looter
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(590, 1, 25, 100, 0, 1, 1, 1, 'Defias Mobs (Westfall Specific)'),
(590, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(590, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(590, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(590, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(590, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(590, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(590, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 594 Defias Henchman
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(594, 1, 25, 100, 0, 1, 1, 1, 'Defias Mobs (Westfall Specific)'),
(594, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(594, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(594, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(594, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(594, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(594, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(594, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 98 Riverpaw Taskmaster
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(98, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(98, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(98, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(98, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(98, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(98, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(98, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 117 Riverpaw Gnoll
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(117, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(117, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(117, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(117, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(117, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(117, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(117, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 123 Riverpaw Mongrel
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(123, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(123, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(123, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(123, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(123, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(123, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(123, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 124 Riverpaw Brute
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(124, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(124, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(124, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(124, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(124, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(124, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(124, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 453 Riverpaw Mystic
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(453, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(453, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(453, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(453, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(453, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(453, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(453, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 501 Riverpaw Herbalist
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(501, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(501, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(501, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(501, 1, 20, 100, 0, 1, 1, 1, 'Herbs Zone 2'),
(501, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(501, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(501, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(501, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 452 Riverpaw Bandit
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(452, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(452, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(452, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(452, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(452, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(452, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(452, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 500 Riverpaw Scout
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(500, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(500, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(500, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(500, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(500, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(500, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(500, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 42653 Riverpaw Shaman
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(42653, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(42653, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(42653, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(42653, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(42653, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(42653, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(42653, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 1065 Jango Spothide
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(1065, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(1065, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(1065, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(1065, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(1065, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(1065, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(1065, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 114 Harvest Watcher
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(114, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(114, 1, 13, 100, 0, 1, 1, 1, 'Mechanical Zone 2'),
(114, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(114, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(114, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(114, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(114, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 115 Harvest Reaper
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(115, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(115, 1, 13, 100, 0, 1, 1, 1, 'Mechanical Zone 2'),
(115, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(115, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(115, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(115, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(115, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 480 Rusty Harvest Golem
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(480, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(480, 1, 13, 100, 0, 1, 1, 1, 'Mechanical Zone 2'),
(480, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(480, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(480, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(480, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(480, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 126 Murloc Coastrunner
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(126, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(126, 1, 8, 100, 0, 1, 1, 1, 'Murlocs'),
(126, 1, 21, 100, 0, 1, 1, 1, 'Fish Zone 2'),
(126, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(126, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(126, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(126, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(126, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 127 Murloc Tidehunter
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(127, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(127, 1, 8, 100, 0, 1, 1, 1, 'Murlocs'),
(127, 1, 21, 100, 0, 1, 1, 1, 'Fish Zone 2'),
(127, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(127, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(127, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(127, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(127, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 171 Murloc Warrior
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(171, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(171, 1, 8, 100, 0, 1, 1, 1, 'Murlocs'),
(171, 1, 21, 100, 0, 1, 1, 1, 'Fish Zone 2'),
(171, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(171, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(171, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(171, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(171, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 391 Old Murk-Eye
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(391, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(391, 1, 8, 100, 0, 1, 1, 1, 'Murlocs'),
(391, 1, 21, 100, 0, 1, 1, 1, 'Fish Zone 2'),
(391, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(391, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(391, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(391, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(391, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 456 Murloc Minor Oracle
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(456, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(456, 1, 8, 100, 0, 1, 1, 1, 'Murlocs'),
(456, 1, 21, 100, 0, 1, 1, 1, 'Fish Zone 2'),
(456, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(456, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(456, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(456, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(456, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 458 Murloc Hunter
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(458, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(458, 1, 8, 100, 0, 1, 1, 1, 'Murlocs'),
(458, 1, 21, 100, 0, 1, 1, 1, 'Fish Zone 2'),
(458, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(458, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(458, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(458, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(458, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 513 Murloc Netter
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(513, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(513, 1, 8, 100, 0, 1, 1, 1, 'Murlocs'),
(513, 1, 21, 100, 0, 1, 1, 1, 'Fish Zone 2'),
(513, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(513, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(513, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(513, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(513, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 515 Murloc Raider
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(515, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(515, 1, 8, 100, 0, 1, 1, 1, 'Murlocs'),
(515, 1, 21, 100, 0, 1, 1, 1, 'Fish Zone 2'),
(515, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(515, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(515, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(515, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(515, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 517 Murloc Oracle
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(517, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(517, 1, 8, 100, 0, 1, 1, 1, 'Murlocs'),
(517, 1, 21, 100, 0, 1, 1, 1, 'Fish Zone 2'),
(517, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(517, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(517, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(517, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(517, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 519 Slark
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(519, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(519, 1, 8, 100, 0, 1, 1, 1, 'Murlocs'),
(519, 1, 21, 100, 0, 1, 1, 1, 'Fish Zone 2'),
(519, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(519, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(519, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(519, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(519, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 154 Greater Fleshripper
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(154, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(154, 1, 11, 100, 0, 1, 1, 1, 'Birds Zone 2'),
(154, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(154, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(154, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(154, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(154, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 199 Young Fleshripper
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(199, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(199, 1, 11, 100, 0, 1, 1, 1, 'Birds Zone 2'),
(199, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(199, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(199, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(199, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(199, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 462 Vultros
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(462, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(462, 1, 11, 100, 0, 1, 1, 1, 'Birds Zone 2'),
(462, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(462, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(462, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(462, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(462, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 1109 Fleshripper
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(1109, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(1109, 1, 11, 100, 0, 1, 1, 1, 'Birds Zone 2'),
(1109, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(1109, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(1109, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(1109, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(1109, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 157 Goretusk
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(157, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(157, 1, 12, 100, 0, 1, 1, 1, 'Boars Zone 2'),
(157, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(157, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(157, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(157, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(157, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 454 Young Goretusk
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(454, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(454, 1, 12, 100, 0, 1, 1, 1, 'Boars Zone 2'),
(454, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(454, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(454, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(454, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(454, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 42357 Hulking Goretusk
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(42357, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(42357, 1, 12, 100, 0, 1, 1, 1, 'Boars Zone 2'),
(42357, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(42357, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(42357, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(42357, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(42357, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 830 Sand Crawler
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(830, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(830, 1, 14, 100, 0, 1, 1, 1, 'Crawler Zone 2'),
(830, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(830, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(830, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(830, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(830, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 831 Sea Crawler
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(831, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(831, 1, 14, 100, 0, 1, 1, 1, 'Crawler Zone 2'),
(831, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(831, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(831, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(831, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(831, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 6250 Crawler
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(6250, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(6250, 1, 14, 100, 0, 1, 1, 1, 'Crawler Zone 2'),
(6250, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(6250, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(6250, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(6250, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(6250, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 832 Unbound Cyclone
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(832, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(832, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(832, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(832, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(832, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(832, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 834 Coyote
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(834, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(834, 1, 10, 100, 0, 1, 1, 1, 'Beasts Zone 2'),
(834, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(834, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(834, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(834, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(834, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 1236 Kobold Digger
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(1236, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(1236, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(1236, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(1236, 1, 15, 100, 0, 1, 1, 1, 'Ore Zone 2'),
(1236, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(1236, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(1236, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(1236, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 1424 Master Digger
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(1424, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(1424, 1, 23, 100, 0, 1, 1, 1, 'Humanoids Zone 2'),
(1424, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(1424, 1, 15, 100, 0, 1, 1, 1, 'Ore Zone 2'),
(1424, 1, 19, 100, 0, 1, 1, 1, 'Bags Zone 2'),
(1424, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(1424, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(1424, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 42669 Chasm Slime
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(42669, 1, 24, 100, 0, 1, 1, 1, 'Armor Zone 2'), 
(42669, 1, 9, 100, 0, 1, 1, 1, 'Ooze/Slime Zone 2'),
(42669, 1, 22, 100, 0, 1, 1, 1, 'Consumable Zone 2'),
(42669, 1, 18, 100, 0, 1, 1, 1, 'Weapons Zone 2'),
(42669, 1, 17, 100, 0, 1, 1, 1, 'Recipes Zone 2'),
(42669, 1, 16, 100, 0, 1, 1, 1, 'Gems Zone 2');
