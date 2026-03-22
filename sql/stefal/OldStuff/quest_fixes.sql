# Westfall
INSERT INTO creature_loot_template VALUES (515, 0, 57756, 20, 1, 1, 1, 1,1,'Hot On the Trail: Murlocs');
INSERT INTO creature_loot_template VALUES (126, 0, 57756, 20, 1, 1, 1, 1,1,'Hot On the Trail: Murlocs');

INSERT INTO creature_loot_template VALUES (500, 0, 57755, 20, 1, 1, 1, 1,1,'Hot On the Trail: The Riverpaw Clan');
INSERT INTO creature_loot_template VALUES (117, 0, 57755, 20, 1, 1, 1, 1,1,'Hot On the Trail: The Riverpaw Clan');

# Redridge
INSERT INTO creature_loot_template VALUES (615, 0, 58952, 50, 1, 1, 1, 1,1,'Surveying Equipment');

INSERT INTO creature_loot_template VALUES (43350, 0, 58969, 100, 1, 1, 1, 1,1,'Jorgensen');
UPDATE `creature_template_difficulty` SET `LootID` = '43350' WHERE (`Entry` = '43350') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '43350' WHERE (`Entry` = '43350') and (`DifficultyID` = '1');

INSERT INTO creature_loot_template VALUES (43369, 0, 59033, 100, 1, 1, 1, 1,1,'And Last But Not Least... Danforth');
UPDATE `creature_template_difficulty` SET `LootID` = '43369' WHERE (`Entry` = '43369') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '43369' WHERE (`Entry` = '43369') and (`DifficultyID` = '1');

INSERT INTO creature_loot_template VALUES (703, 0, 59522, 100, 1, 1, 1, 1,1,'The Dark Tower');


