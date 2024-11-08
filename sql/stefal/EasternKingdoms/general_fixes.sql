-- general and misc changes or fixes
-- fix narg the taskmaster difficulty
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '73' WHERE (`Entry` = '79') and (`DifficultyID` = '0');
