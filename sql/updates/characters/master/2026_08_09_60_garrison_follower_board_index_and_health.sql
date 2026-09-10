--
-- Adventures companions: persist current health and the board slot they were deployed in.
--
-- `character_garrison_followers` stored neither. Two consequences, both visible in game:
--
--  1. `GarrisonFollower.Health` was assigned once at recruit time (Garrison::AddFollower) and then thrown
--     away, so every login reloaded the whole roster at health 0. That is the `health = 0` the tester saw on
--     a level-1 companion whose statline maximum is 250.
--
--  2. A companion's slot on the Adventures board (client enum `GarrAutoBoardIndex`, ally slots 0..4) existed
--     only for the lifetime of one CMSG_GARRISON_START_MISSION - and in fact was not even read from it. A
--     mission that was in progress across a logout came back with every companion unplaced, and the
--     mission-complete screen throws when it cannot resolve a board frame for a follower
--     (Blizzard_AdventuresCompleteScreen.lua:140).
--
-- Defaults are chosen so existing rows load correctly without a data migration:
--   health     0  -> Garrison::LoadFromDB restores any follower at <= 0 to its statline maximum, so legacy
--                    rows come back at full rather than looking wiped.
--   boardIndex -1 -> GarrAutoBoardIndex::None. LoadFromDB re-runs AssignMissionBoardIndexes for any
--                    in-progress mission whose companions are unplaced, filling them in retail's own
--                    auto-assignment order, so missions already running when this is applied are repaired
--                    on the next login rather than staying broken.
--
-- Idempotent: both columns are added only when absent.
--

DROP PROCEDURE IF EXISTS `tc_add_garrison_follower_board_columns`;
DELIMITER $$
CREATE PROCEDURE `tc_add_garrison_follower_board_columns`()
BEGIN
    IF NOT EXISTS (SELECT 1 FROM `information_schema`.`COLUMNS`
                   WHERE `TABLE_SCHEMA` = DATABASE()
                     AND `TABLE_NAME` = 'character_garrison_followers'
                     AND `COLUMN_NAME` = 'health') THEN
        ALTER TABLE `character_garrison_followers`
            ADD COLUMN `health` int NOT NULL DEFAULT 0 AFTER `garrType`;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM `information_schema`.`COLUMNS`
                   WHERE `TABLE_SCHEMA` = DATABASE()
                     AND `TABLE_NAME` = 'character_garrison_followers'
                     AND `COLUMN_NAME` = 'boardIndex') THEN
        ALTER TABLE `character_garrison_followers`
            ADD COLUMN `boardIndex` tinyint NOT NULL DEFAULT -1 AFTER `health`;
    END IF;
END$$
DELIMITER ;

CALL `tc_add_garrison_follower_board_columns`();
DROP PROCEDURE `tc_add_garrison_follower_board_columns`;
