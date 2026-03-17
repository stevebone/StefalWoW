-- ============================================================================
-- JIT BOT TRACKING TABLE
-- ============================================================================
--
-- Purpose: Track JIT-created bots separately from world population bots
-- Version: 1.0.0
-- Date: 2026-01-13
--
-- CRITICAL FIX: The orphan cleanup was deleting ALL characters on
-- @playerbot.local accounts, including BotSpawner's reusable characters.
-- This table allows JITBotFactory to track ONLY its own bots so cleanup
-- doesn't impact world population bots from BotSpawner.
--
-- Usage:
-- - JITBotFactory inserts a record when creating a new bot
-- - Orphan cleanup only deletes characters that exist in this table
-- - On proper shutdown, JITBotFactory deletes both the character and this record
-- - On crash recovery, orphaned JIT bots (in this table but not logged in) are cleaned
-- - BotSpawner's characters are NOT in this table, so they're preserved
--
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_jit_bots`;
CREATE TABLE `playerbot_jit_bots` (
    `bot_guid` BIGINT UNSIGNED NOT NULL COMMENT 'JIT bot character GUID',
    `account_id` INT UNSIGNED NOT NULL COMMENT 'Account ID the bot was created on',
    `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'When the bot was created',
    `instance_type` ENUM('DUNGEON', 'RAID', 'BATTLEGROUND', 'ARENA') DEFAULT 'DUNGEON' COMMENT 'Type of instance this bot was created for',
    `request_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Original request ID that triggered creation',

    PRIMARY KEY (`bot_guid`),
    INDEX `idx_account_id` (`account_id`),
    INDEX `idx_created_at` (`created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Tracks JIT-created bots for targeted cleanup (preserves BotSpawner bots)';

-- ============================================================================
-- DONE
-- ============================================================================

SELECT 'JIT Bot Tracking table created successfully!' AS status;
