-- ============================================================================
-- WARM POOL BOT PERSISTENCE SCHEMA UPDATE
-- ============================================================================
--
-- Purpose: Enable warm pool bot persistence across server restarts
-- Version: 1.0.1
-- Date: 2026-01-16
--
-- This migration adds:
-- - bracket column for per-bracket pool tracking
-- - is_warm_pool flag to distinguish warm pool from JIT bots
-- - Indexes for bracket distribution queries
--
-- Architecture:
-- - Warm Pool Bots: Persist in database, checked at startup for correct bracket distribution
-- - JIT Bots: Created on-the-fly, deleted on shutdown (tracked in playerbot_jit_bots)
--
-- ============================================================================

-- ============================================================================
-- HELPER PROCEDURE: Add column if it doesn't exist (MySQL-compatible)
-- ============================================================================

DROP PROCEDURE IF EXISTS `playerbot_add_column_if_not_exists`;
DELIMITER //
CREATE PROCEDURE `playerbot_add_column_if_not_exists`(
    IN p_table VARCHAR(64),
    IN p_column VARCHAR(64),
    IN p_definition VARCHAR(255)
)
BEGIN
    DECLARE column_exists INT DEFAULT 0;

    SELECT COUNT(*) INTO column_exists
    FROM `information_schema`.`COLUMNS`
    WHERE `TABLE_SCHEMA` = DATABASE()
      AND `TABLE_NAME` = p_table
      AND `COLUMN_NAME` = p_column;

    IF column_exists = 0 THEN
        SET @sql = CONCAT('ALTER TABLE `', p_table, '` ADD COLUMN `', p_column, '` ', p_definition);
        PREPARE stmt FROM @sql;
        EXECUTE stmt;
        DEALLOCATE PREPARE stmt;
    END IF;
END //
DELIMITER ;

-- ============================================================================
-- ADD NEW COLUMNS
-- ============================================================================

-- Add bracket column to track per-bracket pools (8 brackets: 0-7)
-- Level bracket: 0=10-19, 1=20-29, 2=30-39, 3=40-49, 4=50-59, 5=60-69, 6=70-79, 7=80+
CALL `playerbot_add_column_if_not_exists`(
    'playerbot_instance_pool',
    'bracket',
    'TINYINT UNSIGNED NOT NULL DEFAULT 7 COMMENT ''Level bracket (0=10-19, 1=20-29, ..., 7=80+)'' AFTER `level`'
);

-- Add is_warm_pool flag to distinguish warm pool bots from any other tracked bots
-- Warm pool bots persist across restarts, JIT bots are deleted on shutdown
CALL `playerbot_add_column_if_not_exists`(
    'playerbot_instance_pool',
    'is_warm_pool',
    'TINYINT(1) NOT NULL DEFAULT 1 COMMENT ''True if this is a warm pool bot (persists across restarts)'' AFTER `bracket`'
);

-- Clean up helper procedure
DROP PROCEDURE IF EXISTS `playerbot_add_column_if_not_exists`;

-- ============================================================================
-- ADD INDEXES (using safe drop/create pattern)
-- ============================================================================

-- Helper procedure to add index if not exists
DROP PROCEDURE IF EXISTS `playerbot_add_index_if_not_exists`;
DELIMITER //
CREATE PROCEDURE `playerbot_add_index_if_not_exists`(
    IN p_table VARCHAR(64),
    IN p_index VARCHAR(64),
    IN p_columns VARCHAR(255)
)
BEGIN
    DECLARE index_exists INT DEFAULT 0;

    SELECT COUNT(*) INTO index_exists
    FROM `information_schema`.`STATISTICS`
    WHERE `TABLE_SCHEMA` = DATABASE()
      AND `TABLE_NAME` = p_table
      AND `INDEX_NAME` = p_index;

    IF index_exists = 0 THEN
        SET @sql = CONCAT('CREATE INDEX `', p_index, '` ON `', p_table, '`(', p_columns, ')');
        PREPARE stmt FROM @sql;
        EXECUTE stmt;
        DEALLOCATE PREPARE stmt;
    END IF;
END //
DELIMITER ;

-- Add index for fast bracket-based queries during startup reconciliation
CALL `playerbot_add_index_if_not_exists`(
    'playerbot_instance_pool',
    'idx_bracket_faction_role',
    '`bracket`, `faction`, `role`, `is_warm_pool`'
);

-- Add index for warm pool loading at startup
CALL `playerbot_add_index_if_not_exists`(
    'playerbot_instance_pool',
    'idx_warm_pool',
    '`is_warm_pool`, `slot_state`'
);

-- Clean up helper procedure
DROP PROCEDURE IF EXISTS `playerbot_add_index_if_not_exists`;

-- ============================================================================
-- STORED PROCEDURE: Get bracket distribution for reconciliation at startup
-- ============================================================================

DROP PROCEDURE IF EXISTS `GetWarmPoolBracketDistribution`;
DELIMITER //
CREATE PROCEDURE `GetWarmPoolBracketDistribution`()
BEGIN
    SELECT
        `bracket`,
        `faction`,
        `role`,
        COUNT(*) as `count`
    FROM `playerbot_instance_pool`
    WHERE `is_warm_pool` = 1
    GROUP BY `bracket`, `faction`, `role`
    ORDER BY `bracket`, `faction`, `role`;
END //
DELIMITER ;

-- ============================================================================
-- STORED PROCEDURE: Get warm pool bots for a specific bracket
-- ============================================================================

DROP PROCEDURE IF EXISTS `GetWarmPoolBotsForBracket`;
DELIMITER //
CREATE PROCEDURE `GetWarmPoolBotsForBracket`(IN p_bracket TINYINT UNSIGNED)
BEGIN
    SELECT
        `bot_guid`,
        `account_id`,
        `bot_name`,
        `role`,
        `faction`,
        `player_class`,
        `spec_id`,
        `level`,
        `gear_score`,
        `slot_state`,
        `assignment_count`,
        `successful_completions`
    FROM `playerbot_instance_pool`
    WHERE `is_warm_pool` = 1 AND `bracket` = p_bracket
    ORDER BY `faction`, `role`;
END //
DELIMITER ;

-- ============================================================================
-- VIEW: Warm pool bracket summary for monitoring
-- ============================================================================

DROP VIEW IF EXISTS `v_warm_pool_bracket_summary`;
CREATE VIEW `v_warm_pool_bracket_summary` AS
SELECT
    `bracket`,
    `faction`,
    SUM(CASE WHEN `role` = 'TANK' THEN 1 ELSE 0 END) as `tanks`,
    SUM(CASE WHEN `role` = 'HEALER' THEN 1 ELSE 0 END) as `healers`,
    SUM(CASE WHEN `role` = 'DPS' THEN 1 ELSE 0 END) as `dps`,
    COUNT(*) as `total`
FROM `playerbot_instance_pool`
WHERE `is_warm_pool` = 1
GROUP BY `bracket`, `faction`;

-- ============================================================================
-- VIEW: Warm pool health check - compare actual vs target distribution
-- ============================================================================

DROP VIEW IF EXISTS `v_warm_pool_health`;
CREATE VIEW `v_warm_pool_health` AS
SELECT
    b.`bracket`,
    b.`faction`,
    COALESCE(p.`tanks`, 0) as `actual_tanks`,
    10 as `target_tanks`,
    COALESCE(p.`healers`, 0) as `actual_healers`,
    15 as `target_healers`,
    COALESCE(p.`dps`, 0) as `actual_dps`,
    25 as `target_dps`,
    COALESCE(p.`total`, 0) as `actual_total`,
    50 as `target_total`,
    CASE
        WHEN COALESCE(p.`total`, 0) = 50 THEN 'HEALTHY'
        WHEN COALESCE(p.`total`, 0) >= 40 THEN 'WARNING'
        ELSE 'CRITICAL'
    END as `status`
FROM (
    -- Generate all bracket/faction combinations
    SELECT b.`bracket`, f.`faction`
    FROM (
        SELECT 0 as `bracket` UNION ALL SELECT 1 UNION ALL SELECT 2 UNION ALL SELECT 3
        UNION ALL SELECT 4 UNION ALL SELECT 5 UNION ALL SELECT 6 UNION ALL SELECT 7
    ) b
    CROSS JOIN (
        SELECT 'ALLIANCE' as `faction` UNION ALL SELECT 'HORDE'
    ) f
) b
LEFT JOIN `v_warm_pool_bracket_summary` p ON b.`bracket` = p.`bracket` AND b.`faction` = p.`faction`;

-- ============================================================================
-- DONE
-- ============================================================================

SELECT 'Warm Pool Persistence schema update applied successfully!' AS status;
