-- Orgrimmar Fixes

-- NPC: 184787 Boss Magor <Orgrimmar Artisans Guild>

-- Creature Difficulty
DELETE FROM `creature_template_difficulty` WHERE Entry IN (184787) AND `DifficultyID` = 0;
UPDATE `creature_template_difficulty` SET `DifficultyID` = 0 WHERE Entry IN (184787) AND `DifficultyID` = 1;
