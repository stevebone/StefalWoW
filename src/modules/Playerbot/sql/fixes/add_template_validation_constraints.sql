-- ============================================================================
-- ADD VALIDATION CONSTRAINTS TO PREVENT INVALID TEMPLATE ENTRIES
-- ============================================================================
-- This script adds database-level safeguards to prevent invalid templates:
-- 1. CHECK constraints on class_id (must be 1-13, valid WoW classes)
-- 2. CHECK constraints on spec_id (must be > 0)
-- 3. CHECK constraints on role (must be 0-2)
-- 4. BEFORE INSERT trigger for comprehensive validation
-- 5. BEFORE UPDATE trigger for comprehensive validation
--
-- MySQL 8.0.16+ required for CHECK constraint support
-- ============================================================================

-- First, clean up any existing invalid entries
DELETE FROM `playerbot_bot_templates` WHERE `class_id` = 0 OR `class_id` > 13;
DELETE FROM `playerbot_bot_templates` WHERE `spec_id` = 0;

-- Drop existing constraints if they exist (safe to run multiple times)
-- Note: MySQL doesn't have IF EXISTS for constraints, so we use a procedure

DELIMITER //

DROP PROCEDURE IF EXISTS AddTemplateConstraints//

CREATE PROCEDURE AddTemplateConstraints()
BEGIN
    DECLARE constraint_exists INT DEFAULT 0;

    -- Check if constraint already exists
    SELECT COUNT(*) INTO constraint_exists
    FROM information_schema.TABLE_CONSTRAINTS
    WHERE CONSTRAINT_SCHEMA = DATABASE()
      AND TABLE_NAME = 'playerbot_bot_templates'
      AND CONSTRAINT_NAME = 'chk_valid_class_id';

    -- Add CHECK constraint for class_id if it doesn't exist
    IF constraint_exists = 0 THEN
        ALTER TABLE `playerbot_bot_templates`
        ADD CONSTRAINT `chk_valid_class_id`
        CHECK (`class_id` >= 1 AND `class_id` <= 13);
    END IF;

    -- Check for spec_id constraint
    SELECT COUNT(*) INTO constraint_exists
    FROM information_schema.TABLE_CONSTRAINTS
    WHERE CONSTRAINT_SCHEMA = DATABASE()
      AND TABLE_NAME = 'playerbot_bot_templates'
      AND CONSTRAINT_NAME = 'chk_valid_spec_id';

    IF constraint_exists = 0 THEN
        ALTER TABLE `playerbot_bot_templates`
        ADD CONSTRAINT `chk_valid_spec_id`
        CHECK (`spec_id` > 0);
    END IF;

    -- Check for role constraint
    SELECT COUNT(*) INTO constraint_exists
    FROM information_schema.TABLE_CONSTRAINTS
    WHERE CONSTRAINT_SCHEMA = DATABASE()
      AND TABLE_NAME = 'playerbot_bot_templates'
      AND CONSTRAINT_NAME = 'chk_valid_role';

    IF constraint_exists = 0 THEN
        ALTER TABLE `playerbot_bot_templates`
        ADD CONSTRAINT `chk_valid_role`
        CHECK (`role` >= 0 AND `role` <= 2);
    END IF;

    -- Check for template_name constraint (not empty)
    SELECT COUNT(*) INTO constraint_exists
    FROM information_schema.TABLE_CONSTRAINTS
    WHERE CONSTRAINT_SCHEMA = DATABASE()
      AND TABLE_NAME = 'playerbot_bot_templates'
      AND CONSTRAINT_NAME = 'chk_valid_template_name';

    IF constraint_exists = 0 THEN
        ALTER TABLE `playerbot_bot_templates`
        ADD CONSTRAINT `chk_valid_template_name`
        CHECK (LENGTH(`template_name`) > 0);
    END IF;

END//

DELIMITER ;

-- Execute the procedure
CALL AddTemplateConstraints();

-- Clean up
DROP PROCEDURE IF EXISTS AddTemplateConstraints;

-- ============================================================================
-- ADD VALIDATION TRIGGERS
-- ============================================================================

DELIMITER //

-- Drop existing triggers if they exist
DROP TRIGGER IF EXISTS `trg_bot_templates_before_insert`//
DROP TRIGGER IF EXISTS `trg_bot_templates_before_update`//

-- BEFORE INSERT trigger - validates all fields
CREATE TRIGGER `trg_bot_templates_before_insert`
BEFORE INSERT ON `playerbot_bot_templates`
FOR EACH ROW
BEGIN
    -- Validate class_id (WoW classes are 1-13)
    IF NEW.class_id < 1 OR NEW.class_id > 13 THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'Invalid class_id: must be between 1 and 13 (valid WoW class IDs)';
    END IF;

    -- Validate spec_id exists in spec_info
    IF NOT EXISTS (SELECT 1 FROM playerbot_spec_info WHERE spec_id = NEW.spec_id) THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'Invalid spec_id: must reference a valid entry in playerbot_spec_info';
    END IF;

    -- Validate role (0=Tank, 1=Healer, 2=DPS)
    IF NEW.role < 0 OR NEW.role > 2 THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'Invalid role: must be 0 (Tank), 1 (Healer), or 2 (DPS)';
    END IF;

    -- Validate template_name is not empty
    IF NEW.template_name IS NULL OR LENGTH(TRIM(NEW.template_name)) = 0 THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'Invalid template_name: cannot be empty';
    END IF;

    -- Validate class_id matches spec_info
    IF NOT EXISTS (SELECT 1 FROM playerbot_spec_info
                   WHERE spec_id = NEW.spec_id AND class_id = NEW.class_id) THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'class_id does not match the class for the given spec_id in playerbot_spec_info';
    END IF;
END//

-- BEFORE UPDATE trigger - validates all fields
CREATE TRIGGER `trg_bot_templates_before_update`
BEFORE UPDATE ON `playerbot_bot_templates`
FOR EACH ROW
BEGIN
    -- Validate class_id (WoW classes are 1-13)
    IF NEW.class_id < 1 OR NEW.class_id > 13 THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'Invalid class_id: must be between 1 and 13 (valid WoW class IDs)';
    END IF;

    -- Validate spec_id exists in spec_info
    IF NOT EXISTS (SELECT 1 FROM playerbot_spec_info WHERE spec_id = NEW.spec_id) THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'Invalid spec_id: must reference a valid entry in playerbot_spec_info';
    END IF;

    -- Validate role (0=Tank, 1=Healer, 2=DPS)
    IF NEW.role < 0 OR NEW.role > 2 THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'Invalid role: must be 0 (Tank), 1 (Healer), or 2 (DPS)';
    END IF;

    -- Validate template_name is not empty
    IF NEW.template_name IS NULL OR LENGTH(TRIM(NEW.template_name)) = 0 THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'Invalid template_name: cannot be empty';
    END IF;

    -- Validate class_id matches spec_info
    IF NOT EXISTS (SELECT 1 FROM playerbot_spec_info
                   WHERE spec_id = NEW.spec_id AND class_id = NEW.class_id) THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'class_id does not match the class for the given spec_id in playerbot_spec_info';
    END IF;
END//

DELIMITER ;

-- ============================================================================
-- VERIFICATION
-- ============================================================================

-- Show constraints
SELECT 'Constraints added:' AS status;
SELECT CONSTRAINT_NAME, CONSTRAINT_TYPE
FROM information_schema.TABLE_CONSTRAINTS
WHERE TABLE_SCHEMA = DATABASE()
  AND TABLE_NAME = 'playerbot_bot_templates';

-- Show triggers
SELECT 'Triggers added:' AS status;
SELECT TRIGGER_NAME, EVENT_MANIPULATION, ACTION_TIMING
FROM information_schema.TRIGGERS
WHERE TRIGGER_SCHEMA = DATABASE()
  AND EVENT_OBJECT_TABLE = 'playerbot_bot_templates';

-- Verify template counts
SELECT 'Template verification:' AS status;
SELECT role, COUNT(*) as count FROM playerbot_bot_templates GROUP BY role;
SELECT COUNT(*) as total_templates FROM playerbot_bot_templates;

-- Test constraint (should fail) - commented out to not disrupt execution
-- INSERT INTO playerbot_bot_templates (spec_id, class_id, role, template_name) VALUES (0, 0, 0, '');

SELECT 'Database constraints successfully added!' AS result;
