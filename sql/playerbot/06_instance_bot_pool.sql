-- ============================================================================
-- INSTANCE BOT POOL DATABASE SCHEMA
-- ============================================================================
--
-- Purpose: Database schema for the Instance Bot Pool system
-- Version: 1.0.0
-- Date: 2026-01-08
--
-- This schema supports:
-- - Pool bot registry (persistent pool members)
-- - Pool assignment history (for analytics)
-- - Pool statistics (for monitoring)
-- - Reservation tracking
--
-- Tables:
-- - playerbot_instance_pool: Main pool bot registry
-- - playerbot_pool_assignments: Assignment history
-- - playerbot_pool_statistics: Hourly statistics snapshots
-- - playerbot_pool_reservations: Active reservations (runtime only)
--
-- ============================================================================

-- Disable foreign key checks to allow dropping tables in any order
SET FOREIGN_KEY_CHECKS = 0;

-- ============================================================================
-- TABLE: playerbot_instance_pool
-- ============================================================================
-- Main registry of bots in the warm pool. Persists bot assignments,
-- state, and performance metrics across server restarts.
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_instance_pool`;
CREATE TABLE `playerbot_instance_pool` (
    -- Identity
    `bot_guid` BIGINT UNSIGNED NOT NULL COMMENT 'Bot character GUID',
    `account_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Account ID the bot belongs to',
    `bot_name` VARCHAR(12) NOT NULL DEFAULT '' COMMENT 'Character name',

    -- Classification
    `pool_type` ENUM('PVE', 'PVP_ALLIANCE', 'PVP_HORDE') NOT NULL DEFAULT 'PVE' COMMENT 'Pool type',
    `role` ENUM('TANK', 'HEALER', 'DPS') NOT NULL DEFAULT 'DPS' COMMENT 'Combat role',
    `player_class` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'WoW class ID',
    `spec_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Specialization ID',
    `faction` ENUM('ALLIANCE', 'HORDE') NOT NULL DEFAULT 'ALLIANCE' COMMENT 'Character faction',

    -- Stats
    `level` TINYINT UNSIGNED NOT NULL DEFAULT 80 COMMENT 'Character level',
    `gear_score` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Item level / gear score',
    `health_max` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Max health',
    `mana_max` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Max mana',

    -- State
    `slot_state` ENUM('EMPTY', 'CREATING', 'WARMING', 'READY', 'RESERVED', 'ASSIGNED', 'COOLDOWN', 'MAINTENANCE')
        NOT NULL DEFAULT 'READY' COMMENT 'Current slot state',
    `state_change_time` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'When state last changed',
    `last_assignment` TIMESTAMP NULL DEFAULT NULL COMMENT 'When last assigned to instance',

    -- Assignment Tracking
    `current_instance_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Current instance ID (0 if not assigned)',
    `current_content_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Current dungeon/raid/bg ID',
    `current_instance_type` ENUM('DUNGEON', 'RAID', 'BATTLEGROUND', 'ARENA') DEFAULT 'DUNGEON' COMMENT 'Type of current instance',
    `reservation_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Current reservation ID (0 if not reserved)',

    -- Performance Metrics
    `assignment_count` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Total lifetime assignments',
    `total_instance_time` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Total seconds spent in instances',
    `successful_completions` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Instances completed successfully',
    `early_exits` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Times removed before completion',

    -- Timestamps
    `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'When bot was added to pool',
    `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last update time',

    PRIMARY KEY (`bot_guid`),
    INDEX `idx_pool_type` (`pool_type`),
    INDEX `idx_role` (`role`),
    INDEX `idx_pool_role` (`pool_type`, `role`),
    INDEX `idx_slot_state` (`slot_state`),
    INDEX `idx_faction` (`faction`),
    INDEX `idx_level` (`level`),
    INDEX `idx_faction_role_state` (`faction`, `role`, `slot_state`),
    INDEX `idx_assignment_count` (`assignment_count`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Instance Bot Pool - Main registry of warm pool bots';

-- ============================================================================
-- TABLE: playerbot_pool_assignments
-- ============================================================================
-- History of bot assignments for analytics and monitoring.
-- Tracks when bots were assigned to instances and how long they stayed.
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_pool_assignments`;
CREATE TABLE `playerbot_pool_assignments` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'Unique assignment ID',

    -- Assignment Details
    `bot_guid` BIGINT UNSIGNED NOT NULL COMMENT 'Bot that was assigned',
    `instance_type` ENUM('DUNGEON', 'RAID', 'BATTLEGROUND', 'ARENA') NOT NULL COMMENT 'Type of instance',
    `instance_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Map instance ID',
    `content_id` INT UNSIGNED NOT NULL COMMENT 'Dungeon/Raid/BG ID',

    -- Bot Info at Assignment
    `bot_role` ENUM('TANK', 'HEALER', 'DPS') NOT NULL COMMENT 'Bot role at assignment',
    `bot_level` TINYINT UNSIGNED NOT NULL COMMENT 'Bot level at assignment',
    `bot_gear_score` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Bot gear score at assignment',
    `bot_faction` ENUM('ALLIANCE', 'HORDE') NOT NULL COMMENT 'Bot faction',

    -- Timing
    `assigned_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'When assignment started',
    `released_at` TIMESTAMP NULL DEFAULT NULL COMMENT 'When bot was released',
    `duration_seconds` INT UNSIGNED NULL DEFAULT NULL COMMENT 'Total time in instance',

    -- Result
    `completion_status` ENUM('IN_PROGRESS', 'SUCCESS', 'EARLY_EXIT', 'ERROR', 'TIMEOUT')
        NOT NULL DEFAULT 'IN_PROGRESS' COMMENT 'How the assignment ended',

    -- Reservation
    `reservation_id` INT UNSIGNED DEFAULT NULL COMMENT 'Reservation ID if pre-reserved',

    PRIMARY KEY (`id`),
    INDEX `idx_bot_guid` (`bot_guid`),
    INDEX `idx_instance_type` (`instance_type`),
    INDEX `idx_content_id` (`content_id`),
    INDEX `idx_instance` (`instance_type`, `instance_id`),
    INDEX `idx_assigned_at` (`assigned_at`),
    INDEX `idx_completion_status` (`completion_status`),
    FOREIGN KEY (`bot_guid`) REFERENCES `playerbot_instance_pool`(`bot_guid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Instance Bot Pool - Assignment history for analytics';

-- ============================================================================
-- TABLE: playerbot_pool_statistics
-- ============================================================================
-- Hourly snapshots of pool statistics for monitoring and capacity planning.
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_pool_statistics`;
CREATE TABLE `playerbot_pool_statistics` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'Unique snapshot ID',
    `snapshot_time` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'When snapshot was taken',

    -- Slot Counts
    `total_slots` INT UNSIGNED NOT NULL DEFAULT 0,
    `empty_slots` INT UNSIGNED NOT NULL DEFAULT 0,
    `creating_slots` INT UNSIGNED NOT NULL DEFAULT 0,
    `warming_slots` INT UNSIGNED NOT NULL DEFAULT 0,
    `ready_slots` INT UNSIGNED NOT NULL DEFAULT 0,
    `reserved_slots` INT UNSIGNED NOT NULL DEFAULT 0,
    `assigned_slots` INT UNSIGNED NOT NULL DEFAULT 0,
    `cooldown_slots` INT UNSIGNED NOT NULL DEFAULT 0,
    `maintenance_slots` INT UNSIGNED NOT NULL DEFAULT 0,

    -- Per-Role Ready Counts
    `ready_tanks` INT UNSIGNED NOT NULL DEFAULT 0,
    `ready_healers` INT UNSIGNED NOT NULL DEFAULT 0,
    `ready_dps` INT UNSIGNED NOT NULL DEFAULT 0,

    -- Per-Faction Ready Counts
    `ready_alliance` INT UNSIGNED NOT NULL DEFAULT 0,
    `ready_horde` INT UNSIGNED NOT NULL DEFAULT 0,

    -- Activity (hourly)
    `assignments_hour` INT UNSIGNED NOT NULL DEFAULT 0,
    `releases_hour` INT UNSIGNED NOT NULL DEFAULT 0,
    `jit_creations_hour` INT UNSIGNED NOT NULL DEFAULT 0,
    `reservations_hour` INT UNSIGNED NOT NULL DEFAULT 0,
    `cancellations_hour` INT UNSIGNED NOT NULL DEFAULT 0,

    -- Instance Type Breakdown (hourly)
    `dungeons_filled_hour` INT UNSIGNED NOT NULL DEFAULT 0,
    `raids_filled_hour` INT UNSIGNED NOT NULL DEFAULT 0,
    `battlegrounds_filled_hour` INT UNSIGNED NOT NULL DEFAULT 0,
    `arenas_filled_hour` INT UNSIGNED NOT NULL DEFAULT 0,

    -- Success/Failure (hourly)
    `successful_requests_hour` INT UNSIGNED NOT NULL DEFAULT 0,
    `failed_requests_hour` INT UNSIGNED NOT NULL DEFAULT 0,
    `timeout_requests_hour` INT UNSIGNED NOT NULL DEFAULT 0,

    -- Timing Metrics (microseconds/milliseconds)
    `avg_assignment_time_us` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Avg assignment time in microseconds',
    `avg_warmup_time_ms` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Avg warmup time in milliseconds',
    `avg_jit_creation_time_ms` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Avg JIT creation time in milliseconds',
    `peak_assignment_time_us` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Peak assignment time in microseconds',

    -- Calculated Metrics
    `utilization_pct` DECIMAL(5,2) NOT NULL DEFAULT 0.00 COMMENT 'Utilization percentage',
    `availability_pct` DECIMAL(5,2) NOT NULL DEFAULT 0.00 COMMENT 'Availability percentage',
    `success_rate_pct` DECIMAL(5,2) NOT NULL DEFAULT 100.00 COMMENT 'Request success rate',

    PRIMARY KEY (`id`),
    INDEX `idx_snapshot_time` (`snapshot_time`),
    INDEX `idx_utilization` (`utilization_pct`),
    INDEX `idx_success_rate` (`success_rate_pct`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Instance Bot Pool - Hourly statistics snapshots';

-- ============================================================================
-- TABLE: playerbot_pool_config
-- ============================================================================
-- Runtime configuration storage for pool settings.
-- Allows dynamic configuration without server restart.
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_pool_config`;
CREATE TABLE `playerbot_pool_config` (
    `config_key` VARCHAR(64) NOT NULL COMMENT 'Configuration key',
    `config_value` VARCHAR(255) NOT NULL DEFAULT '' COMMENT 'Configuration value',
    `description` VARCHAR(255) DEFAULT NULL COMMENT 'Description of the setting',
    `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    PRIMARY KEY (`config_key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Instance Bot Pool - Runtime configuration';

-- Insert default configuration
INSERT INTO `playerbot_pool_config` (`config_key`, `config_value`, `description`) VALUES
('enabled', '1', 'Master enable switch for instance bot pool'),
('alliance_tanks', '20', 'Number of Alliance tank bots in pool'),
('alliance_healers', '30', 'Number of Alliance healer bots in pool'),
('alliance_dps', '50', 'Number of Alliance DPS bots in pool'),
('horde_tanks', '20', 'Number of Horde tank bots in pool'),
('horde_healers', '30', 'Number of Horde healer bots in pool'),
('horde_dps', '50', 'Number of Horde DPS bots in pool'),
('cooldown_seconds', '300', 'Cooldown between bot assignments'),
('reservation_timeout_ms', '60000', 'Reservation timeout in milliseconds'),
('warmup_timeout_ms', '30000', 'Bot warmup timeout in milliseconds'),
('max_overflow_bots', '500', 'Maximum JIT overflow bots'),
('overflow_creation_rate', '10', 'JIT bots created per second'),
('auto_replenish', '1', 'Auto-replenish depleted pool slots'),
('persist_to_database', '1', 'Persist pool state to database'),
('warm_on_startup', '1', 'Warm pool on server startup'),
('jit_enabled', '1', 'Enable JIT factory'),
('log_assignments', '1', 'Log individual bot assignments'),
('log_pool_changes', '0', 'Log pool state changes'),
('log_reservations', '1', 'Log reservation operations');

-- ============================================================================
-- TABLE: playerbot_content_requirements (OVERRIDE TABLE)
-- ============================================================================
--
-- PURPOSE: Custom overrides for content requirements
--
-- IMPORTANT: This table is an OVERRIDE mechanism, NOT the primary data source!
--
-- Data Loading Order:
--   1. PRIMARY: DB2 files are loaded first (LFGDungeons.db2, etc.)
--      - Dungeons: Loaded from LFGDungeons.db2 (TypeID=1)
--      - Raids: Loaded from LFGDungeons.db2 (TypeID=2)
--      - Battlegrounds: Loaded from BattlemasterList.db2
--      - Arenas: Hardcoded defaults
--
--   2. OVERRIDE: This table is loaded second and REPLACES any matching entries
--      - Entries here will override the DB2 defaults
--      - Use this to customize specific dungeons/raids/BGs
--
-- RECOMMENDATION: Leave this table EMPTY unless you need to customize specific
-- content. The DB2 data provides correct values for all standard content.
--
-- Example use cases for adding entries:
--   - Override recommended gear score for a specific dungeon
--   - Adjust role counts (e.g., need 3 tanks for a specific raid boss)
--   - Change level requirements for custom server configurations
--
-- See: ContentRequirementDatabase::Initialize() in ContentRequirements.cpp
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_content_requirements`;
CREATE TABLE `playerbot_content_requirements` (
    `content_id` INT UNSIGNED NOT NULL COMMENT 'Dungeon/Raid/BG/Arena ID',
    `content_name` VARCHAR(64) NOT NULL DEFAULT '' COMMENT 'Human-readable name',
    `instance_type` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Type: 0=Dungeon, 1=Raid, 2=Battleground, 3=Arena',
    `difficulty` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Difficulty (0=Normal, 1=Heroic, 2=Mythic, etc.)',

    -- Player Requirements
    `min_players` INT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'Minimum players',
    `max_players` INT UNSIGNED NOT NULL DEFAULT 5 COMMENT 'Maximum players',
    `min_level` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'Minimum level',
    `max_level` TINYINT UNSIGNED NOT NULL DEFAULT 80 COMMENT 'Maximum level',
    `recommended_level` TINYINT UNSIGNED NOT NULL DEFAULT 80 COMMENT 'Recommended level',

    -- Role Requirements
    `min_tanks` INT UNSIGNED NOT NULL DEFAULT 1,
    `max_tanks` INT UNSIGNED NOT NULL DEFAULT 1,
    `recommended_tanks` INT UNSIGNED NOT NULL DEFAULT 1,
    `min_healers` INT UNSIGNED NOT NULL DEFAULT 1,
    `max_healers` INT UNSIGNED NOT NULL DEFAULT 1,
    `recommended_healers` INT UNSIGNED NOT NULL DEFAULT 1,
    `min_dps` INT UNSIGNED NOT NULL DEFAULT 3,
    `max_dps` INT UNSIGNED NOT NULL DEFAULT 3,
    `recommended_dps` INT UNSIGNED NOT NULL DEFAULT 3,

    -- Gear Requirements
    `min_gear_score` INT UNSIGNED NOT NULL DEFAULT 0,
    `recommended_gear_score` INT UNSIGNED NOT NULL DEFAULT 0,

    -- PvP Specific
    `requires_both_factions` TINYINT(1) NOT NULL DEFAULT 0 COMMENT 'Whether both factions needed',
    `players_per_faction` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Players per faction for PvP',

    -- Timing
    `estimated_duration_minutes` INT UNSIGNED NOT NULL DEFAULT 30 COMMENT 'Expected duration',

    PRIMARY KEY (`content_id`, `instance_type`, `difficulty`),
    INDEX `idx_instance_type` (`instance_type`),
    INDEX `idx_level_range` (`min_level`, `max_level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Override table for content requirements - see header comment';

-- ============================================================================
-- NO DEFAULT DATA - This table is intentionally empty!
-- ============================================================================
-- Primary data comes from DB2 files. Only add entries here to OVERRIDE defaults.
--
-- EXAMPLE: To override Nerub-ar Palace to require 3 tanks instead of 2:
--
-- INSERT INTO `playerbot_content_requirements`
--     (`content_id`, `content_name`, `instance_type`, `difficulty`,
--      `min_players`, `max_players`, `min_level`, `max_level`, `recommended_level`,
--      `min_tanks`, `max_tanks`, `recommended_tanks`,
--      `min_healers`, `max_healers`, `recommended_healers`,
--      `min_dps`, `max_dps`, `recommended_dps`,
--      `min_gear_score`, `recommended_gear_score`, `estimated_duration_minutes`) VALUES
-- (2769, 'Nerub-ar Palace (Mythic)', 1, 2, 1, 20, 80, 80, 80, 3, 3, 3, 5, 6, 5, 11, 12, 12, 540, 570, 300);
--
-- instance_type values: 0=Dungeon, 1=Raid, 2=Battleground, 3=Arena
-- difficulty values: 0=Normal, 1=Heroic, 2=Mythic, etc.
-- ============================================================================

-- ============================================================================
-- CLEANUP PROCEDURES
-- ============================================================================

-- Clean up old statistics (keep last 30 days)
DELIMITER //
CREATE PROCEDURE IF NOT EXISTS `CleanupPoolStatistics`()
BEGIN
    DELETE FROM `playerbot_pool_statistics`
    WHERE `snapshot_time` < DATE_SUB(NOW(), INTERVAL 30 DAY);
END //

-- Clean up old assignment history (keep last 90 days)
CREATE PROCEDURE IF NOT EXISTS `CleanupAssignmentHistory`()
BEGIN
    DELETE FROM `playerbot_pool_assignments`
    WHERE `assigned_at` < DATE_SUB(NOW(), INTERVAL 90 DAY);
END //
DELIMITER ;

-- ============================================================================
-- VIEWS FOR MONITORING
-- ============================================================================

-- Current pool status view
DROP VIEW IF EXISTS `v_pool_status`;
CREATE VIEW `v_pool_status` AS
SELECT
    `slot_state`,
    COUNT(*) as `count`,
    ROUND(COUNT(*) * 100.0 / (SELECT COUNT(*) FROM `playerbot_instance_pool`), 2) as `percentage`
FROM `playerbot_instance_pool`
GROUP BY `slot_state`;

-- Role distribution view
DROP VIEW IF EXISTS `v_pool_roles`;
CREATE VIEW `v_pool_roles` AS
SELECT
    `faction`,
    `role`,
    `slot_state`,
    COUNT(*) as `count`
FROM `playerbot_instance_pool`
GROUP BY `faction`, `role`, `slot_state`;

-- Recent assignments (use stored procedure instead - LIMIT not allowed in views)
DROP VIEW IF EXISTS `v_recent_assignments`;
DROP PROCEDURE IF EXISTS `GetRecentAssignments`;
DELIMITER //
CREATE PROCEDURE `GetRecentAssignments`(IN p_limit INT)
BEGIN
    IF p_limit IS NULL OR p_limit <= 0 THEN
        SET p_limit = 100;
    END IF;

    SET @sql = CONCAT(
        'SELECT `instance_type`, `content_id`, `bot_role`, `completion_status`, ',
        '`duration_seconds`, `assigned_at`, `released_at` ',
        'FROM `playerbot_pool_assignments` ',
        'ORDER BY `assigned_at` DESC LIMIT ', p_limit
    );
    PREPARE stmt FROM @sql;
    EXECUTE stmt;
    DEALLOCATE PREPARE stmt;
END //
DELIMITER ;

-- ============================================================================
-- INDEXES FOR PERFORMANCE
-- ============================================================================

-- Helper procedure to add index if not exists (MySQL-compatible)
DROP PROCEDURE IF EXISTS `pb_add_index_if_not_exists`;
DELIMITER //
CREATE PROCEDURE `pb_add_index_if_not_exists`(
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

-- Fast lookup of ready bots by faction and role
CALL `pb_add_index_if_not_exists`(
    'playerbot_instance_pool',
    'idx_pool_ready_faction_role',
    '`slot_state`, `faction`, `role`'
);

-- Fast recent assignments query
CALL `pb_add_index_if_not_exists`(
    'playerbot_pool_assignments',
    'idx_assignments_recent',
    '`assigned_at`'
);

-- Clean up helper procedure
DROP PROCEDURE IF EXISTS `pb_add_index_if_not_exists`;

-- Re-enable foreign key checks
SET FOREIGN_KEY_CHECKS = 1;

-- ============================================================================
-- DONE
-- ============================================================================

SELECT 'Instance Bot Pool schema created successfully!' AS status;
