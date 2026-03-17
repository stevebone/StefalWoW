-- ============================================================================
-- FIX: Update playerbot_instance_pool with correct account_id from characters
-- ============================================================================
-- Issue: Pool bots have account_id = 0, causing login failures
-- Solution: Update account_id from the characters table where it's available
-- ============================================================================

-- Update account_id from characters table
-- Note: This assumes the characters database is named 'characters'
-- Adjust the database name if different on your setup

UPDATE `playerbot_instance_pool` pip
INNER JOIN `characters`.`characters` c ON pip.`bot_guid` = c.`guid`
SET pip.`account_id` = c.`account`
WHERE pip.`account_id` = 0 AND c.`account` > 0;

-- Report how many were fixed
SELECT
    COUNT(*) as 'Bots with account_id = 0 remaining',
    (SELECT COUNT(*) FROM `playerbot_instance_pool` WHERE `account_id` > 0) as 'Bots with valid account_id'
FROM `playerbot_instance_pool`
WHERE `account_id` = 0;

-- ============================================================================
-- DONE
-- ============================================================================
SELECT 'Pool account IDs fix complete!' AS status;
