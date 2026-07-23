-- Eastern Plaguelands Fixes

-- Creature Difficulty
-- Swim flag addition
UPDATE creature_template_difficulty SET StaticFlags1 = StaticFlags1 | 0x10000000 WHERE Entry IN (16378) AND DifficultyID = 0;
UPDATE creature_template_difficulty SET StaticFlags1 = StaticFlags1 & ~0x20000000 WHERE Entry IN (16378) AND DifficultyID = 0;

-- Creature Fixes
UPDATE creature
SET MovementType = 1,
    wander_distance = 8
WHERE id IN (
    8597,8601,8603,8534,8541,8538,
    8525,8543,8548,8526,8520,11291,
    11290,8596,8521,8602,8524,45851,
    8523,8530,8555,8557,8563,8565,
    8540,8537,8532,8528
);
