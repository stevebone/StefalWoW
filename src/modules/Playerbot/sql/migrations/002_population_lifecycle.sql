-- ============================================================================
-- POPULATION LIFECYCLE MANAGEMENT DATABASE SCHEMA
-- Migration: 002_population_lifecycle
-- Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
--
-- This migration creates tables for:
--   1. Bot Protection Status - Track which bots are protected from deletion
--   2. Friend References - Track player->bot friendships
--   3. Retirement Queue - Manage graceful bot retirement
--   4. Bracket Transitions - Track level bracket flow statistics
--   5. Bracket Statistics - Aggregate bracket flow metrics
-- ============================================================================

-- ============================================================================
-- PROTECTION TABLES
-- ============================================================================

-- Bot protection status tracking
-- Determines which bots cannot be retired due to social connections
CREATE TABLE IF NOT EXISTS `playerbot_protection_status` (
    `bot_guid` BIGINT UNSIGNED NOT NULL COMMENT 'Bot character GUID',
    `protection_flags` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Bitmask of ProtectionReason flags',
    `guild_guid` BIGINT UNSIGNED DEFAULT NULL COMMENT 'Guild GUID if bot is in a guild',
    `friend_count` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Number of players who have this bot as friend',
    `interaction_count` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Total player interactions',
    `last_interaction` TIMESTAMP NULL COMMENT 'Last time a player interacted with this bot',
    `last_group_time` TIMESTAMP NULL COMMENT 'Last time bot was in a group with a player',
    `protection_score` FLOAT NOT NULL DEFAULT 0 COMMENT 'Calculated protection priority (higher = more protected)',
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT 'When protection tracking started',
    `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last update time',
    PRIMARY KEY (`bot_guid`),
    INDEX `idx_protection_flags` (`protection_flags`),
    INDEX `idx_protection_score` (`protection_score`),
    INDEX `idx_guild_guid` (`guild_guid`),
    INDEX `idx_last_interaction` (`last_interaction`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Bot protection status for lifecycle management';

-- Player-to-bot friend references
-- Tracks which players have which bots on their friend list
CREATE TABLE IF NOT EXISTS `playerbot_friend_references` (
    `bot_guid` BIGINT UNSIGNED NOT NULL COMMENT 'Bot character GUID',
    `player_guid` BIGINT UNSIGNED NOT NULL COMMENT 'Player character GUID who has bot as friend',
    `added_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT 'When friendship was established',
    PRIMARY KEY (`bot_guid`, `player_guid`),
    INDEX `idx_player_guid` (`player_guid`),
    INDEX `idx_bot_guid` (`bot_guid`),
    INDEX `idx_added_at` (`added_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Player-to-bot friend list references';

-- ============================================================================
-- RETIREMENT TABLES
-- ============================================================================

-- Bot retirement queue
-- Manages graceful bot retirement with cooling period
CREATE TABLE IF NOT EXISTS `playerbot_retirement_queue` (
    `bot_guid` BIGINT UNSIGNED NOT NULL COMMENT 'Bot character GUID',
    `queued_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT 'When bot was queued for retirement',
    `scheduled_deletion` TIMESTAMP NOT NULL COMMENT 'When bot will be deleted (after cooling period)',
    `retirement_reason` VARCHAR(255) DEFAULT NULL COMMENT 'Reason for retirement',
    `retirement_state` ENUM('PENDING', 'COOLING', 'EXITING', 'CANCELLED', 'COMPLETED') DEFAULT 'PENDING' COMMENT 'Current retirement state',
    `bracket_at_queue` TINYINT UNSIGNED NOT NULL COMMENT 'Level bracket when queued (0-3)',
    `level_at_queue` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'Character level when queued',
    `protection_score_at_queue` FLOAT NOT NULL DEFAULT 0 COMMENT 'Protection score when queued',
    `playtime_minutes` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Total playtime in minutes',
    `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last update time',
    PRIMARY KEY (`bot_guid`),
    INDEX `idx_scheduled_deletion` (`scheduled_deletion`),
    INDEX `idx_retirement_state` (`retirement_state`),
    INDEX `idx_bracket_at_queue` (`bracket_at_queue`),
    INDEX `idx_queued_at` (`queued_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Bot retirement queue for lifecycle management';

-- ============================================================================
-- BRACKET FLOW TRACKING TABLES
-- ============================================================================

-- Individual bracket transition records
-- Tracks each bot's level bracket transitions for flow prediction
CREATE TABLE IF NOT EXISTS `playerbot_bracket_transitions` (
    `id` BIGINT UNSIGNED AUTO_INCREMENT COMMENT 'Unique transition ID',
    `bot_guid` BIGINT UNSIGNED NOT NULL COMMENT 'Bot character GUID',
    `from_bracket` TINYINT UNSIGNED NOT NULL COMMENT 'Source bracket (0=Starting, 1=Chromie, 2=DF, 3=TWW)',
    `to_bracket` TINYINT UNSIGNED NOT NULL COMMENT 'Destination bracket',
    `from_level` TINYINT UNSIGNED NOT NULL COMMENT 'Level before transition',
    `to_level` TINYINT UNSIGNED NOT NULL COMMENT 'Level after transition',
    `transition_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT 'When transition occurred',
    `time_in_bracket_seconds` INT UNSIGNED NOT NULL COMMENT 'Seconds spent in source bracket',
    PRIMARY KEY (`id`),
    INDEX `idx_bot_guid` (`bot_guid`),
    INDEX `idx_from_bracket` (`from_bracket`),
    INDEX `idx_to_bracket` (`to_bracket`),
    INDEX `idx_transition_time` (`transition_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Bot level bracket transition history';

-- Aggregate bracket statistics
-- Pre-computed statistics for flow prediction
CREATE TABLE IF NOT EXISTS `playerbot_bracket_statistics` (
    `bracket_id` TINYINT UNSIGNED NOT NULL COMMENT 'Bracket ID (0-3)',
    `bracket_name` VARCHAR(32) NOT NULL COMMENT 'Bracket name for reference',
    `avg_time_in_bracket_seconds` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Average time bots spend in this bracket',
    `median_time_in_bracket_seconds` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Median time in bracket',
    `min_time_in_bracket_seconds` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Minimum observed time',
    `max_time_in_bracket_seconds` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Maximum observed time',
    `sample_count` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Number of transitions sampled',
    `outflow_rate_per_hour` FLOAT NOT NULL DEFAULT 0 COMMENT 'Average bots leaving bracket per hour',
    `inflow_rate_per_hour` FLOAT NOT NULL DEFAULT 0 COMMENT 'Average bots entering bracket per hour',
    `last_updated` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last calculation time',
    PRIMARY KEY (`bracket_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aggregate bracket flow statistics';

-- Initialize bracket statistics with default values
INSERT INTO `playerbot_bracket_statistics` (`bracket_id`, `bracket_name`, `avg_time_in_bracket_seconds`) VALUES
(0, 'Starting (L1-10)', 7200),      -- 2 hours default for 1-10
(1, 'ChromieTime (L10-60)', 86400), -- 24 hours default for 10-60
(2, 'Dragonflight (L60-70)', 43200),-- 12 hours default for 60-70
(3, 'TheWarWithin (L70-80)', 0)     -- Max level, no outflow
ON DUPLICATE KEY UPDATE `bracket_name` = VALUES(`bracket_name`);

-- ============================================================================
-- PLAYER ACTIVITY TRACKING TABLES
-- ============================================================================

-- Player activity snapshots for demand calculation
-- Tracks where real players are playing for demand-driven spawning
CREATE TABLE IF NOT EXISTS `playerbot_player_activity` (
    `player_guid` BIGINT UNSIGNED NOT NULL COMMENT 'Player character GUID',
    `zone_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Current zone ID',
    `map_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Current map ID',
    `level` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'Player level',
    `bracket_id` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Level bracket (0-3)',
    `last_update` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last activity update',
    `session_start` TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT 'When player logged in',
    `is_active` TINYINT(1) NOT NULL DEFAULT 1 COMMENT 'Whether player is currently active',
    PRIMARY KEY (`player_guid`),
    INDEX `idx_zone_id` (`zone_id`),
    INDEX `idx_level` (`level`),
    INDEX `idx_bracket_id` (`bracket_id`),
    INDEX `idx_is_active` (`is_active`),
    INDEX `idx_last_update` (`last_update`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Real player activity tracking for demand calculation';

-- Zone demand statistics (aggregated)
CREATE TABLE IF NOT EXISTS `playerbot_zone_demand` (
    `zone_id` INT UNSIGNED NOT NULL COMMENT 'Zone ID',
    `map_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Map ID',
    `zone_name` VARCHAR(100) DEFAULT NULL COMMENT 'Zone name for reference',
    `player_count` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Current player count',
    `bot_count` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Current bot count',
    `min_level` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'Minimum level for zone',
    `max_level` TINYINT UNSIGNED NOT NULL DEFAULT 80 COMMENT 'Maximum level for zone',
    `demand_score` FLOAT NOT NULL DEFAULT 0 COMMENT 'Calculated demand score',
    `last_calculated` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last calculation time',
    PRIMARY KEY (`zone_id`),
    INDEX `idx_demand_score` (`demand_score` DESC),
    INDEX `idx_player_count` (`player_count` DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Zone demand statistics for spawn prioritization';

-- ============================================================================
-- LIFECYCLE METRICS TABLES
-- ============================================================================

-- Daily lifecycle metrics for monitoring and tuning
CREATE TABLE IF NOT EXISTS `playerbot_lifecycle_metrics` (
    `date` DATE NOT NULL COMMENT 'Metrics date',
    `hour` TINYINT UNSIGNED NOT NULL COMMENT 'Hour of day (0-23)',
    `total_bots` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Total bot count',
    `protected_bots` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Protected bot count',
    `bots_created` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Bots created this hour',
    `bots_retired` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Bots retired this hour',
    `bots_rescued` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Bots rescued from retirement',
    `bracket_0_count` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Starting bracket count',
    `bracket_1_count` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'ChromieTime bracket count',
    `bracket_2_count` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Dragonflight bracket count',
    `bracket_3_count` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'TheWarWithin bracket count',
    `avg_player_count` FLOAT NOT NULL DEFAULT 0 COMMENT 'Average player count',
    `peak_player_count` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Peak player count',
    PRIMARY KEY (`date`, `hour`),
    INDEX `idx_date` (`date`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Hourly lifecycle metrics for monitoring';

-- ============================================================================
-- CLEANUP PROCEDURES
-- ============================================================================

-- Delete old transition records (keep 30 days)
DELIMITER //
CREATE PROCEDURE IF NOT EXISTS `cleanup_bracket_transitions`()
BEGIN
    DELETE FROM `playerbot_bracket_transitions`
    WHERE `transition_time` < DATE_SUB(NOW(), INTERVAL 30 DAY);
END //
DELIMITER ;

-- Delete old lifecycle metrics (keep 90 days)
DELIMITER //
CREATE PROCEDURE IF NOT EXISTS `cleanup_lifecycle_metrics`()
BEGIN
    DELETE FROM `playerbot_lifecycle_metrics`
    WHERE `date` < DATE_SUB(CURDATE(), INTERVAL 90 DAY);
END //
DELIMITER ;

-- ============================================================================
-- SCHEDULED EVENTS (if event scheduler is enabled)
-- ============================================================================

-- Note: These events require event_scheduler = ON in MySQL config

-- Daily cleanup at 4 AM server time
-- CREATE EVENT IF NOT EXISTS `evt_cleanup_bracket_transitions`
-- ON SCHEDULE EVERY 1 DAY STARTS '2024-01-01 04:00:00'
-- DO CALL cleanup_bracket_transitions();

-- CREATE EVENT IF NOT EXISTS `evt_cleanup_lifecycle_metrics`
-- ON SCHEDULE EVERY 1 DAY STARTS '2024-01-01 04:30:00'
-- DO CALL cleanup_lifecycle_metrics();
