-- ============================================================================
-- FIX: Remove corrupt template entry with class_id=0
-- ============================================================================
-- Issue: A corrupt entry exists with template_id=0, class_id=0, spec_id=0
-- This causes JIT bot creation failures for Horde faction because class_id=0
-- has no valid races in the class_race_matrix.
--
-- Root cause: Unknown - possibly manual insert or migration bug
-- Effect: GetValidRaces returns empty for class 0, template rejected for Horde
-- ============================================================================

-- Delete any templates with invalid class_id (0 is not a valid WoW class)
DELETE FROM `playerbot_bot_templates` WHERE `class_id` = 0;

-- Delete any templates with invalid spec_id (0 is not a valid spec)
DELETE FROM `playerbot_bot_templates` WHERE `spec_id` = 0;

-- Delete any orphan statistics entries
DELETE FROM `playerbot_template_statistics`
WHERE `template_id` NOT IN (SELECT `template_id` FROM `playerbot_bot_templates`);

-- Verify fix
SELECT 'Templates after cleanup:' AS status;
SELECT COUNT(*) AS total_templates FROM `playerbot_bot_templates`;
SELECT `role`, COUNT(*) AS count FROM `playerbot_bot_templates` GROUP BY `role`;

-- Show any remaining invalid templates (should be empty)
SELECT 'Invalid templates (should be empty):' AS status;
SELECT * FROM `playerbot_bot_templates` WHERE `class_id` = 0 OR `spec_id` = 0;
