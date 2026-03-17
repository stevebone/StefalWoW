-- ====================================================================
-- Playerbot Talent Loadouts Table
-- ====================================================================
-- Purpose: Store predefined talent loadouts for automated bot creation
-- Integration: Used by BotTalentManager for instant level-up scenarios
-- ====================================================================

DROP TABLE IF EXISTS `playerbot_talent_loadouts`;

CREATE TABLE `playerbot_talent_loadouts` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `class_id` TINYINT UNSIGNED NOT NULL COMMENT 'Class ID (1-13)',
  `spec_id` TINYINT UNSIGNED NOT NULL COMMENT 'Specialization ID (0-3)',
  `min_level` TINYINT UNSIGNED NOT NULL COMMENT 'Minimum level for this loadout',
  `max_level` TINYINT UNSIGNED NOT NULL COMMENT 'Maximum level for this loadout',
  `talent_string` TEXT NOT NULL COMMENT 'Comma-separated talent entry IDs',
  `hero_talent_string` TEXT DEFAULT NULL COMMENT 'Comma-separated hero talent entry IDs (71+)',
  `description` VARCHAR(255) DEFAULT NULL COMMENT 'Loadout description',
  `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_class_spec` (`class_id`, `spec_id`),
  KEY `idx_level` (`min_level`, `max_level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Talent loadouts for automated bot creation';

-- ====================================================================
-- Example Loadouts (Warrior - Arms Spec)
-- ====================================================================
-- Note: These are placeholder entries. Actual talent IDs must be added
-- based on WoW 12.0 spell IDs from DBC/DB2 files
-- ====================================================================

INSERT INTO `playerbot_talent_loadouts`
(`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`)
VALUES
-- Warrior Arms (Spec ID 0) - Example entries
(1, 0, 1, 10, '', NULL, 'Arms Warrior 1-10: No talents (auto-learned)'),
(1, 0, 11, 20, '', NULL, 'Arms Warrior 11-20: Basic talents'),
(1, 0, 21, 30, '', NULL, 'Arms Warrior 21-30: Intermediate talents'),
(1, 0, 31, 40, '', NULL, 'Arms Warrior 31-40: Advanced talents'),
(1, 0, 41, 50, '', NULL, 'Arms Warrior 41-50: Expert talents'),
(1, 0, 51, 60, '', NULL, 'Arms Warrior 51-60: Master talents'),
(1, 0, 61, 70, '', NULL, 'Arms Warrior 61-70: Pre-War Within'),
(1, 0, 71, 80, '', '', 'Arms Warrior 71-80: War Within + Hero Talents');

-- ====================================================================
-- Notes on Implementation:
-- ====================================================================
-- 1. Talent Entry IDs: Must be actual spell IDs from TrinityCore's spell system
-- 2. Hero Talents: Only apply for levels 71-80 (War Within feature)
-- 3. Dual-Spec: Separate loadouts for each spec, applied independently
-- 4. Format: Talent strings are comma-separated (e.g., "12345,67890,11111")
-- 5. Auto-Generation: If table is empty, BotTalentManager builds default empty loadouts
-- ====================================================================

-- ====================================================================
-- TODO: Populate with Actual Talent Data
-- ====================================================================
-- Method 1: Manual curation from guides (Icy-Veins, Wowhead, etc.)
-- Method 2: Extraction from player character database
-- Method 3: Auto-generation based on talent tree structure
-- ====================================================================
