-- Classic Deadmines Loot Tables

-- First we remove ALL loot items for Westfall specific mobs to avoid leftovers
-- Other delete queries below may be redundant however needed for single operations
DELETE FROM creature_loot_template 
WHERE Entry IN (1732,639);

-- Quest Items are distributed PER creature entry
DELETE FROM creature_loot_template
WHERE (Entry, Item) IN (
(1732, 915),
(639, 3637)
);

INSERT INTO `creature_loot_template` (`Entry`, `ItemType`, `Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(1732, 0, 915, 75, 1, 1, 1, 1, 1, 'Red Silk Bandana'),
(639, 0, 3637, 100, 1, 1, 1, 1, 1, 'Head of VanCleef');

-- Specific Mob Flavor items
DELETE FROM `creature_loot_template`
WHERE (Entry, Item) IN (
	(639, 2874),
	(639, 5202),
	(639, 5193),
	(639, 10399),
	(639, 5191)
);

INSERT INTO `creature_loot_template` (`Entry`, `ItemType`, `Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(639, 0, 2874, 100, 0, 1, 1, 1, 1, 'An Unsent Letter - Edwin VanCleef'),
(639, 0, 5202, 25, 0, 1, 2, 1, 1, 'Corsairs Overshirt - Edwin VanCleef'),
(639, 0, 5193, 25, 0, 1, 2, 1, 1, 'Cape of the Brotherhood - Edwin VanCleef'),
(639, 0, 10399, 15, 0, 1, 2, 1, 1, 'Blackened Defias Armor - Edwin VanCleef'),
(639, 0, 5191, 15, 0, 1, 2, 1, 1, 'Blackened Defias Armor - Edwin VanCleef');

-- Loot tables
-- Delete only reference loot items (ItemType = 1) for specific NPCs
DELETE FROM creature_loot_template 
WHERE Entry IN (1732,639) 
AND ItemType = 1;

-- NPC: 1732 Defias Squallshaper
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(1732, 1, 25, 10, 0, 1, 1, 1, 'Defias Mobs (Westfall Specific)'),
(1732, 1, 26, 1, 0, 1, 1, 1, 'Defias Mobs (Deadmines Specific)'),
(1732, 1, 24, 10, 0, 1, 1, 1, 'Armor Zone 2'), 
(1732, 1, 23, 50, 0, 1, 1, 1, 'Humanoids Zone 2'),
(1732, 1, 22, 40, 0, 1, 1, 1, 'Consumable Zone 2'),
(1732, 1, 19, 10, 0, 1, 1, 1, 'Bags Zone 2'),
(1732, 1, 18, 10, 0, 1, 1, 1, 'Weapons Zone 2'),
(1732, 1, 17, 15, 0, 1, 1, 1, 'Recipes Zone 2'),
(1732, 1, 16, 10, 0, 1, 1, 1, 'Gems Zone 2');

-- NPC: 639 Edwin VanCleef
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(639, 1, 25, 10, 0, 1, 1, 1, 'Defias Mobs (Westfall Specific)'),
(1732, 1, 26, 1, 0, 1, 1, 1, 'Defias Mobs (Deadmines Specific)'),
(639, 1, 24, 10, 0, 1, 1, 1, 'Armor Zone 2'), 
(639, 1, 23, 50, 0, 1, 1, 1, 'Humanoids Zone 2'),
(639, 1, 22, 40, 0, 1, 1, 1, 'Consumable Zone 2'),
(639, 1, 19, 10, 0, 1, 1, 1, 'Bags Zone 2'),
(639, 1, 18, 10, 0, 1, 1, 1, 'Weapons Zone 2'),
(639, 1, 17, 15, 0, 1, 1, 1, 'Recipes Zone 2'),
(639, 1, 16, 10, 0, 1, 1, 1, 'Gems Zone 2');