-- Fixes for Swamp Of Sorrow

-- NPC: 986 Haromm
-- NPC: 51983 Deadwind Widow

-- Wrong spawns
DELETE FROM creature WHERE id = 51983 AND guid = 293089;
DELETE FROM creature WHERE id = 51983 AND guid = 293088;

-- Difficulty fixes
-- Missing Content Tuning
UPDATE creature_template_difficulty SET ContentTuningID = 31 WHERE Entry = 986 AND DifficultyID = 0;
