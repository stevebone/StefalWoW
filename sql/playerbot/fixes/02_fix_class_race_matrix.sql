-- ============================================================================
-- FIX: PLAYERBOT CLASS/RACE MATRIX - ENSURE ALL RACES ENABLED
-- ============================================================================
--
-- Purpose: Ensure all races are enabled for servers with full race support
-- Version: 1.0.1
-- Date: 2026-01-17
--
-- This script ensures all races in the matrix are enabled.
-- The C++ code validates against playercreateinfo at runtime.
--
-- ============================================================================

-- Enable ALL races in the matrix
UPDATE `playerbot_class_race_matrix` SET `enabled` = 1;

-- Show summary
SELECT
    `faction`,
    COUNT(*) as `total_entries`,
    SUM(CASE WHEN `enabled` = 1 THEN 1 ELSE 0 END) as `enabled_count`
FROM `playerbot_class_race_matrix`
GROUP BY `faction`;

SELECT 'All races enabled in class/race matrix!' AS status;
