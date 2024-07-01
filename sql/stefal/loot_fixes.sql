# Update Loot Templates
#
# Creatures
#
# Incorrect quest item drops
UPDATE creature_loot_template SET QuestRequired=1 WHERE Item IN (772, 773, 780, 1019, 735, 6846, 6810);
#
# Goblin Assassin - Northshire
UPDATE `creature_template_difficulty` SET `LootID` = '50039', `PickPocketLootID` = '50039' WHERE (`Entry` = '50039') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '50039', `PickPocketLootID` = '50039' WHERE (`Entry` = '50039') and (`DifficultyID` = '1');

# Incorrect Loot
UPDATE creature_template_difficulty SET LootID=0 where entry=721; -- rabbit with no loot.
UPDATE creature_template_difficulty SET LootID=0 where entry=1412; -- squirrel with no loot.

# Objects
DELETE FROM gameobject_loot_template WHERE Entry=29958;
INSERT INTO gameobject_loot_template VALUES (203800, 57245, 0, 100, 1, 1, 0, 1, 2, 'Fishing Daily SW');


# Fishing
INSERT INTO fishing_loot_template VALUES (61, 11000, 11000, 100, 0, 1, 1, 1,1,'Fishing ThunderFalls - Elwynn');
INSERT INTO reference_loot_template VALUES (11000, 58899, 0, 100, 1, 1, 1, 1,1,'Violet Perch');

# Items
# Bag of shiny things
INSERT INTO item_loot_template VALUES
(67414, 6532, 0, 40, 0, 1, 1, 1, 3, 'Bag of shiny things'),
(67414, 8827, 0, 15, 0, 1, 1, 1, 2, 'Bag of shiny things'),
(67414, 3820, 0, 15, 0, 1, 1, 2, 5, 'Bag of shiny things'),
(67414, 2459, 0, 15, 0, 1, 1, 1, 2, 'Bag of shiny things'),
(67414, 6372, 0, 15, 0, 1, 1, 1, 2, 'Bag of shiny things'),
(67414, 67408, 0, 5, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 67406, 0, 5, 0, 1, 1, 1, 3, 'Bag of shiny things'),
(67414, 67397, 0, 5, 0, 1, 1, 1, 3, 'Bag of shiny things'),
(67414, 67398, 0, 5, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 67405, 0, 5, 0, 1, 1, 1, 3, 'Bag of shiny things'),
(67414, 67411, 0, 5, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 67409, 0, 5, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 67404, 0, 5, 0, 1, 1, 1, 3, 'Bag of shiny things'),
(67414, 67402, 0, 5, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 67412, 0, 5, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 67403, 0, 5, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 67400, 0, 5, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 67391, 0, 5, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 67385, 0, 5, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 67388, 0, 5, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 67399, 0, 5, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 67401, 0, 5, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 67407, 0, 5, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 67386, 0, 5, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 67410, 0, 5, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 929, 0, 5, 0, 1, 1, 1, 2, 'Bag of shiny things'),
(67414, 34834, 0, 2, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 34109, 0, 2, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 44983, 0, 2, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 33820, 0, 2, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 45991, 0, 2, 0, 1, 1, 1, 1, 'Bag of shiny things'),
(67414, 45992, 0, 2, 0, 1, 1, 1, 1, 'Bag of shiny things');