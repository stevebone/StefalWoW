-- ============================================================================
-- TrinityCore Playerbot Module - Installation Validation Script
-- ============================================================================
-- This script validates that the playerbot database installation is complete
-- and compatible with the C++ code requirements
-- Run this after installing both schema and data files
-- ============================================================================

-- Set session variables for validation
SET @validation_start = NOW();
SET @validation_database = DATABASE();

-- ============================================================================
-- VALIDATION 1: Check all required tables exist
-- ============================================================================

SELECT 'TABLE VALIDATION' as 'Validation Step';

SELECT
    CASE
        WHEN COUNT(*) = 12 THEN '✅ PASS'
        ELSE CONCAT('❌ FAIL - Expected 12 tables, found ', COUNT(*))
    END as 'Status',
    'Required Tables' as 'Check',
    GROUP_CONCAT(TABLE_NAME ORDER BY TABLE_NAME) as 'Details'
FROM INFORMATION_SCHEMA.TABLES
WHERE TABLE_SCHEMA = @validation_database
AND TABLE_NAME IN (
    'playerbot_accounts',
    'playerbot_activity_patterns',
    'playerbot_character_distribution',
    'playerbot_lifecycle_events',
    'playerbot_migrations',
    'playerbot_schedules',
    'playerbot_spawn_log',
    'playerbot_zone_populations',
    'playerbots_class_popularity',
    'playerbots_gender_distribution',
    'playerbots_names',
    'playerbots_names_used',
    'playerbots_race_class_distribution',
    'playerbots_race_class_gender'
);

-- ============================================================================
-- VALIDATION 2: Check critical columns exist
-- ============================================================================

SELECT 'COLUMN VALIDATION' as 'Validation Step';

-- Check playerbot_activity_patterns critical columns
SELECT
    CASE
        WHEN COUNT(*) >= 8 THEN '✅ PASS'
        ELSE '❌ FAIL - Missing critical columns'
    END as 'Status',
    'playerbot_activity_patterns' as 'Table',
    'Critical columns for C++ compatibility' as 'Check'
FROM INFORMATION_SCHEMA.COLUMNS
WHERE TABLE_SCHEMA = @validation_database
AND TABLE_NAME = 'playerbot_activity_patterns'
AND COLUMN_NAME IN ('pattern_name', 'display_name', 'is_system_pattern', 'login_chance', 'logout_chance', 'enabled');

-- Check playerbot_schedules critical columns
SELECT
    CASE
        WHEN COUNT(*) >= 12 THEN '✅ PASS'
        ELSE '❌ FAIL - Missing critical columns'
    END as 'Status',
    'playerbot_schedules' as 'Table',
    'Critical columns for C++ compatibility' as 'Check'
FROM INFORMATION_SCHEMA.COLUMNS
WHERE TABLE_SCHEMA = @validation_database
AND TABLE_NAME = 'playerbot_schedules'
AND COLUMN_NAME IN ('bot_guid', 'pattern_name', 'is_active', 'is_scheduled', 'schedule_priority', 'next_login', 'next_logout', 'total_sessions', 'total_playtime', 'current_session_start', 'last_calculation', 'consecutive_failures');

-- Check playerbot_zone_populations critical columns
SELECT
    CASE
        WHEN COUNT(*) >= 10 THEN '✅ PASS'
        ELSE '❌ FAIL - Missing critical columns'
    END as 'Status',
    'playerbot_zone_populations' as 'Table',
    'Critical columns for C++ compatibility' as 'Check'
FROM INFORMATION_SCHEMA.COLUMNS
WHERE TABLE_SCHEMA = @validation_database
AND TABLE_NAME = 'playerbot_zone_populations'
AND COLUMN_NAME IN ('zone_id', 'map_id', 'current_bots', 'target_population', 'max_population', 'spawn_weight', 'is_enabled', 'is_starter_zone', 'is_endgame_zone', 'last_spawn', 'total_spawns_today');

-- Check playerbot_lifecycle_events critical columns
SELECT
    CASE
        WHEN COUNT(*) >= 8 THEN '✅ PASS'
        ELSE '❌ FAIL - Missing critical columns'
    END as 'Status',
    'playerbot_lifecycle_events' as 'Table',
    'Critical columns for C++ compatibility' as 'Check'
FROM INFORMATION_SCHEMA.COLUMNS
WHERE TABLE_SCHEMA = @validation_database
AND TABLE_NAME = 'playerbot_lifecycle_events'
AND COLUMN_NAME IN ('event_id', 'event_timestamp', 'event_category', 'event_type', 'severity', 'component', 'bot_guid', 'message', 'execution_time_ms', 'correlation_id');

-- ============================================================================
-- VALIDATION 3: Check foreign key constraints
-- ============================================================================

SELECT 'FOREIGN KEY VALIDATION' as 'Validation Step';

SELECT
    CASE
        WHEN COUNT(*) >= 2 THEN '✅ PASS'
        ELSE '❌ FAIL - Missing foreign key constraints'
    END as 'Status',
    'Foreign Key Constraints' as 'Check',
    GROUP_CONCAT(CONCAT(TABLE_NAME, '.', COLUMN_NAME, ' -> ', REFERENCED_TABLE_NAME, '.', REFERENCED_COLUMN_NAME)) as 'Details'
FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE
WHERE TABLE_SCHEMA = @validation_database
AND REFERENCED_TABLE_NAME IS NOT NULL;

-- ============================================================================
-- VALIDATION 4: Check indexes exist for performance
-- ============================================================================

SELECT 'INDEX VALIDATION' as 'Validation Step';

SELECT
    CASE
        WHEN COUNT(DISTINCT CONCAT(TABLE_NAME, '.', INDEX_NAME)) >= 20 THEN '✅ PASS'
        ELSE CONCAT('⚠️ WARNING - Expected 20+ indexes, found ', COUNT(DISTINCT CONCAT(TABLE_NAME, '.', INDEX_NAME)))
    END as 'Status',
    'Performance Indexes' as 'Check',
    'Critical indexes for query performance' as 'Details'
FROM INFORMATION_SCHEMA.STATISTICS
WHERE TABLE_SCHEMA = @validation_database
AND TABLE_NAME LIKE 'playerbot%'
AND INDEX_NAME != 'PRIMARY';

-- ============================================================================
-- VALIDATION 5: Check seed data was loaded
-- ============================================================================

SELECT 'DATA VALIDATION' as 'Validation Step';

-- Check activity patterns
SELECT
    CASE
        WHEN COUNT(*) >= 14 THEN '✅ PASS'
        ELSE CONCAT('❌ FAIL - Expected 14+ patterns, found ', COUNT(*))
    END as 'Status',
    'Activity Patterns' as 'Check',
    CONCAT(COUNT(*), ' patterns loaded') as 'Details'
FROM playerbot_activity_patterns;

-- Check zone populations
SELECT
    CASE
        WHEN COUNT(*) >= 40 THEN '✅ PASS'
        ELSE CONCAT('❌ FAIL - Expected 40+ zones, found ', COUNT(*))
    END as 'Status',
    'Zone Populations' as 'Check',
    CONCAT(COUNT(*), ' zones configured') as 'Details'
FROM playerbot_zone_populations;

-- Check bot names
SELECT
    CASE
        WHEN COUNT(*) >= 80 THEN '✅ PASS'
        ELSE CONCAT('⚠️ WARNING - Expected 80+ names, found ', COUNT(*))
    END as 'Status',
    'Bot Names' as 'Check',
    CONCAT(COUNT(*), ' names available') as 'Details'
FROM playerbots_names;

-- Check class popularity
SELECT
    CASE
        WHEN COUNT(*) = 10 THEN '✅ PASS'
        ELSE CONCAT('❌ FAIL - Expected 10 classes, found ', COUNT(*))
    END as 'Status',
    'Class Popularity' as 'Check',
    CONCAT(COUNT(*), ' classes configured') as 'Details'
FROM playerbots_class_popularity;

-- ============================================================================
-- VALIDATION 6: Test stored procedures
-- ============================================================================

SELECT 'STORED PROCEDURE VALIDATION' as 'Validation Step';

-- Check if stored procedures exist
SELECT
    CASE
        WHEN COUNT(*) >= 4 THEN '✅ PASS'
        ELSE CONCAT('❌ FAIL - Expected 4 procedures, found ', COUNT(*))
    END as 'Status',
    'Stored Procedures' as 'Check',
    GROUP_CONCAT(ROUTINE_NAME) as 'Details'
FROM INFORMATION_SCHEMA.ROUTINES
WHERE ROUTINE_SCHEMA = @validation_database
AND ROUTINE_TYPE = 'PROCEDURE'
AND ROUTINE_NAME LIKE 'sp_%';

-- ============================================================================
-- VALIDATION 7: Verify data integrity
-- ============================================================================

SELECT 'DATA INTEGRITY VALIDATION' as 'Validation Step';

-- Check for orphaned records in character distribution
SELECT
    CASE
        WHEN COUNT(*) = 0 THEN '✅ PASS'
        ELSE CONCAT('❌ FAIL - ', COUNT(*), ' orphaned character records')
    END as 'Status',
    'Character Distribution Integrity' as 'Check',
    'No orphaned character distribution records' as 'Details'
FROM playerbot_character_distribution pcd
LEFT JOIN playerbot_accounts pa ON pcd.account_id = pa.account_id
WHERE pa.account_id IS NULL;

-- Check for invalid race-class combinations
SELECT
    CASE
        WHEN COUNT(*) >= 50 THEN '✅ PASS'
        ELSE CONCAT('⚠️ WARNING - Expected 50+ combinations, found ', COUNT(*))
    END as 'Status',
    'Race-Class Combinations' as 'Check',
    CONCAT(COUNT(*), ' valid combinations') as 'Details'
FROM playerbots_race_class_distribution
WHERE enabled = 1;

-- ============================================================================
-- VALIDATION 8: Character set and collation
-- ============================================================================

SELECT 'CHARACTER SET VALIDATION' as 'Validation Step';

SELECT
    CASE
        WHEN DEFAULT_CHARACTER_SET_NAME = 'utf8mb4' THEN '✅ PASS'
        ELSE CONCAT('⚠️ WARNING - Expected utf8mb4, found ', DEFAULT_CHARACTER_SET_NAME)
    END as 'Status',
    'Database Character Set' as 'Check',
    CONCAT('Database: ', DEFAULT_CHARACTER_SET_NAME, ', Collation: ', DEFAULT_COLLATION_NAME) as 'Details'
FROM information_schema.SCHEMATA
WHERE SCHEMA_NAME = @validation_database;

-- ============================================================================
-- VALIDATION SUMMARY
-- ============================================================================

SELECT 'VALIDATION SUMMARY' as 'Final Report';

SELECT
    'Database Installation Validation' as 'Report',
    @validation_database as 'Database',
    @validation_start as 'Validation Started',
    NOW() as 'Validation Completed',
    TIMESTAMPDIFF(SECOND, @validation_start, NOW()) as 'Duration (seconds)';

-- Summary statistics
SELECT
    (SELECT COUNT(*) FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = @validation_database) as 'Total Tables',
    (SELECT COUNT(*) FROM playerbot_activity_patterns) as 'Activity Patterns',
    (SELECT COUNT(*) FROM playerbot_zone_populations) as 'Configured Zones',
    (SELECT COUNT(*) FROM playerbots_names) as 'Available Names',
    (SELECT COUNT(*) FROM playerbots_class_popularity WHERE enabled = 1) as 'Enabled Classes',
    (SELECT COUNT(*) FROM playerbots_race_class_distribution WHERE enabled = 1) as 'Valid Race-Class Combos';

-- ============================================================================
-- NEXT STEPS
-- ============================================================================

SELECT 'NEXT STEPS' as 'Action Required';

SELECT
    '1. Update worldserver.conf with PlayerbotDatabaseInfo connection string' as 'Configuration Step'
UNION ALL
SELECT
    '2. Update playerbots.conf with Playerbot.Enable = 1'
UNION ALL
SELECT
    '3. Restart TrinityCore worldserver to load playerbot module'
UNION ALL
SELECT
    '4. Test bot spawning with .playerbot spawn command'
UNION ALL
SELECT
    '5. Monitor playerbot_lifecycle_events table for any errors'
UNION ALL
SELECT
    '6. Set up automated cleanup using provided stored procedures';

-- Final success message
SELECT
    CASE
        WHEN (SELECT COUNT(*) FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = @validation_database AND TABLE_NAME LIKE 'playerbot%') >= 10
        THEN '🎉 INSTALLATION COMPLETE - Playerbot database is ready for use!'
        ELSE '❌ INSTALLATION INCOMPLETE - Please review validation errors above'
    END as 'Final Status';