-- Silithus Fixes

-- NPC: 132584 Xaarshej

-- Incorrect factions
UPDATE creature_template SET faction = 14 WHERE entry = 132584;

-- Creature Difficulty
UPDATE `creature_template_difficulty` SET `ContentTuningID` = 650, `LootID` = 132584 WHERE `Entry` = 132584 AND `DifficultyID` = 0;
