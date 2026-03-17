-- ============================================================================
-- BOT TEMPLATE SYSTEM DATABASE SCHEMA
-- ============================================================================
--
-- Migration: 003_bot_templates.sql
-- Purpose: Database schema for the Bot Template Repository and Clone Engine
-- Version: 1.0.0
-- Date: 2026-01-09
--
-- This schema supports:
-- - Pre-defined bot templates for each class/spec combination
-- - Gear set configurations for different item level tiers
-- - Talent builds per spec/role
-- - Action bar layouts per spec
-- - Class/spec/role mappings with WoW 12.0 data
--
-- Tables:
-- - playerbot_spec_info: Master class/spec reference data
-- - playerbot_bot_templates: Main template registry
-- - playerbot_template_gear_sets: Gear configurations per template/ilvl
-- - playerbot_template_gear_items: Individual gear slot items
-- - playerbot_template_talents: Talent configurations
-- - playerbot_template_actionbars: Action bar layouts
-- - playerbot_template_statistics: Usage tracking
-- - playerbot_class_race_matrix: Valid class/race combinations
--
-- ============================================================================

-- ============================================================================
-- TABLE: playerbot_spec_info
-- ============================================================================
-- Master reference table for all WoW 12.0 class specializations.
-- This is the authoritative source for class/spec/role mappings.
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_spec_info`;
CREATE TABLE `playerbot_spec_info` (
    `spec_id` INT UNSIGNED NOT NULL COMMENT 'ChrSpecialization.db2 ID',
    `class_id` TINYINT UNSIGNED NOT NULL COMMENT 'ChrClasses.db2 ID',
    `class_name` VARCHAR(32) NOT NULL COMMENT 'Class name (English)',
    `spec_name` VARCHAR(32) NOT NULL COMMENT 'Spec name (English)',
    `role` ENUM('TANK', 'HEALER', 'DPS') NOT NULL COMMENT 'Combat role',
    `spec_index` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Spec index within class (0-3)',
    `stat_priority` VARCHAR(128) DEFAULT NULL COMMENT 'Primary > Secondary stats',
    `armor_type` ENUM('CLOTH', 'LEATHER', 'MAIL', 'PLATE') NOT NULL COMMENT 'Armor class',
    `primary_stat` ENUM('STRENGTH', 'AGILITY', 'INTELLECT') NOT NULL COMMENT 'Primary stat',
    `enabled` TINYINT(1) NOT NULL DEFAULT 1 COMMENT 'Whether to generate templates',
    `notes` VARCHAR(255) DEFAULT NULL COMMENT 'Implementation notes',

    PRIMARY KEY (`spec_id`),
    INDEX `idx_class` (`class_id`),
    INDEX `idx_role` (`role`),
    INDEX `idx_class_role` (`class_id`, `role`),
    INDEX `idx_enabled` (`enabled`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Master class/spec reference for WoW 12.0 (The War Within)';

-- Insert all WoW 12.0 specializations
INSERT INTO `playerbot_spec_info`
    (`spec_id`, `class_id`, `class_name`, `spec_name`, `role`, `spec_index`, `stat_priority`, `armor_type`, `primary_stat`) VALUES
-- Warrior (class 1)
(71,  1, 'Warrior', 'Arms',        'DPS',    0, 'Str > Crit > Mastery > Haste > Vers', 'PLATE', 'STRENGTH'),
(72,  1, 'Warrior', 'Fury',        'DPS',    1, 'Str > Haste > Mastery > Crit > Vers', 'PLATE', 'STRENGTH'),
(73,  1, 'Warrior', 'Protection',  'TANK',   2, 'Str > Haste > Vers > Mastery > Crit', 'PLATE', 'STRENGTH'),

-- Paladin (class 2)
(65,  2, 'Paladin', 'Holy',        'HEALER', 0, 'Int > Haste > Crit > Mastery > Vers', 'PLATE', 'INTELLECT'),
(66,  2, 'Paladin', 'Protection',  'TANK',   1, 'Str > Haste > Mastery > Vers > Crit', 'PLATE', 'STRENGTH'),
(70,  2, 'Paladin', 'Retribution', 'DPS',    2, 'Str > Haste > Mastery > Crit > Vers', 'PLATE', 'STRENGTH'),

-- Hunter (class 3)
(253, 3, 'Hunter', 'Beast Mastery', 'DPS',   0, 'Agi > Haste > Crit > Mastery > Vers', 'MAIL', 'AGILITY'),
(254, 3, 'Hunter', 'Marksmanship',  'DPS',   1, 'Agi > Mastery > Crit > Haste > Vers', 'MAIL', 'AGILITY'),
(255, 3, 'Hunter', 'Survival',      'DPS',   2, 'Agi > Haste > Crit > Vers > Mastery', 'MAIL', 'AGILITY'),

-- Rogue (class 4)
(259, 4, 'Rogue', 'Assassination', 'DPS',    0, 'Agi > Crit > Mastery > Haste > Vers', 'LEATHER', 'AGILITY'),
(260, 4, 'Rogue', 'Outlaw',        'DPS',    1, 'Agi > Vers > Haste > Crit > Mastery', 'LEATHER', 'AGILITY'),
(261, 4, 'Rogue', 'Subtlety',      'DPS',    2, 'Agi > Crit > Vers > Mastery > Haste', 'LEATHER', 'AGILITY'),

-- Priest (class 5)
(256, 5, 'Priest', 'Discipline', 'HEALER',   0, 'Int > Haste > Crit > Vers > Mastery', 'CLOTH', 'INTELLECT'),
(257, 5, 'Priest', 'Holy',       'HEALER',   1, 'Int > Mastery > Crit > Vers > Haste', 'CLOTH', 'INTELLECT'),
(258, 5, 'Priest', 'Shadow',     'DPS',      2, 'Int > Haste > Mastery > Crit > Vers', 'CLOTH', 'INTELLECT'),

-- Death Knight (class 6)
(250, 6, 'Death Knight', 'Blood',  'TANK',   0, 'Str > Haste > Crit > Vers > Mastery', 'PLATE', 'STRENGTH'),
(251, 6, 'Death Knight', 'Frost',  'DPS',    1, 'Str > Crit > Mastery > Haste > Vers', 'PLATE', 'STRENGTH'),
(252, 6, 'Death Knight', 'Unholy', 'DPS',    2, 'Str > Mastery > Haste > Crit > Vers', 'PLATE', 'STRENGTH'),

-- Shaman (class 7)
(262, 7, 'Shaman', 'Elemental',   'DPS',     0, 'Int > Crit > Vers > Haste > Mastery', 'MAIL', 'INTELLECT'),
(263, 7, 'Shaman', 'Enhancement', 'DPS',     1, 'Agi > Haste > Crit > Mastery > Vers', 'MAIL', 'AGILITY'),
(264, 7, 'Shaman', 'Restoration', 'HEALER',  2, 'Int > Crit > Vers > Haste > Mastery', 'MAIL', 'INTELLECT'),

-- Mage (class 8)
(62,  8, 'Mage', 'Arcane', 'DPS',            0, 'Int > Mastery > Crit > Haste > Vers', 'CLOTH', 'INTELLECT'),
(63,  8, 'Mage', 'Fire',   'DPS',            1, 'Int > Haste > Mastery > Crit > Vers', 'CLOTH', 'INTELLECT'),
(64,  8, 'Mage', 'Frost',  'DPS',            2, 'Int > Crit > Haste > Vers > Mastery', 'CLOTH', 'INTELLECT'),

-- Warlock (class 9)
(265, 9, 'Warlock', 'Affliction',  'DPS',    0, 'Int > Haste > Mastery > Crit > Vers', 'CLOTH', 'INTELLECT'),
(266, 9, 'Warlock', 'Demonology',  'DPS',    1, 'Int > Haste > Mastery > Crit > Vers', 'CLOTH', 'INTELLECT'),
(267, 9, 'Warlock', 'Destruction', 'DPS',    2, 'Int > Haste > Crit > Mastery > Vers', 'CLOTH', 'INTELLECT'),

-- Monk (class 10)
(268, 10, 'Monk', 'Brewmaster', 'TANK',      0, 'Agi > Vers > Crit > Mastery > Haste', 'LEATHER', 'AGILITY'),
(270, 10, 'Monk', 'Mistweaver', 'HEALER',    1, 'Int > Crit > Vers > Haste > Mastery', 'LEATHER', 'INTELLECT'),
(269, 10, 'Monk', 'Windwalker', 'DPS',       2, 'Agi > Vers > Crit > Mastery > Haste', 'LEATHER', 'AGILITY'),

-- Druid (class 11)
(102, 11, 'Druid', 'Balance',     'DPS',     0, 'Int > Mastery > Haste > Crit > Vers', 'LEATHER', 'INTELLECT'),
(103, 11, 'Druid', 'Feral',       'DPS',     1, 'Agi > Crit > Mastery > Haste > Vers', 'LEATHER', 'AGILITY'),
(104, 11, 'Druid', 'Guardian',    'TANK',    2, 'Agi > Vers > Mastery > Haste > Crit', 'LEATHER', 'AGILITY'),
(105, 11, 'Druid', 'Restoration', 'HEALER',  3, 'Int > Haste > Mastery > Crit > Vers', 'LEATHER', 'INTELLECT'),

-- Demon Hunter (class 12)
(577, 12, 'Demon Hunter', 'Havoc',     'DPS',  0, 'Agi > Crit > Haste > Vers > Mastery', 'LEATHER', 'AGILITY'),
(581, 12, 'Demon Hunter', 'Vengeance', 'TANK', 1, 'Agi > Haste > Vers > Crit > Mastery', 'LEATHER', 'AGILITY'),

-- Evoker (class 13)
(1467, 13, 'Evoker', 'Devastation',  'DPS',    0, 'Int > Mastery > Crit > Haste > Vers', 'MAIL', 'INTELLECT'),
(1468, 13, 'Evoker', 'Preservation', 'HEALER', 1, 'Int > Mastery > Crit > Vers > Haste', 'MAIL', 'INTELLECT'),
(1473, 13, 'Evoker', 'Augmentation', 'DPS',    2, 'Int > Mastery > Haste > Crit > Vers', 'MAIL', 'INTELLECT');

-- ============================================================================
-- TABLE: playerbot_class_race_matrix
-- ============================================================================
-- Valid class/race combinations per faction for WoW 12.0.
-- Used when creating bots to ensure valid combinations.
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_class_race_matrix`;
CREATE TABLE `playerbot_class_race_matrix` (
    `class_id` TINYINT UNSIGNED NOT NULL COMMENT 'ChrClasses.db2 ID',
    `race_id` TINYINT UNSIGNED NOT NULL COMMENT 'ChrRaces.db2 ID',
    `race_name` VARCHAR(32) NOT NULL COMMENT 'Race name (English)',
    `faction` ENUM('ALLIANCE', 'HORDE') NOT NULL COMMENT 'Faction',
    `weight` FLOAT NOT NULL DEFAULT 1.0 COMMENT 'Selection weight (popularity)',
    `enabled` TINYINT(1) NOT NULL DEFAULT 1 COMMENT 'Whether to use for bot creation',

    PRIMARY KEY (`class_id`, `race_id`),
    INDEX `idx_faction` (`faction`),
    INDEX `idx_class_faction` (`class_id`, `faction`),
    INDEX `idx_enabled` (`enabled`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Valid class/race combinations for WoW 12.0';

-- Insert valid class/race combinations for WoW 12.0
-- Alliance races: Human(1), Dwarf(3), Night Elf(4), Gnome(7), Draenei(11), Worgen(22),
--                 Pandaren-A(25), Void Elf(29), Lightforged(30), Dark Iron(34), Kul Tiran(32),
--                 Mechagnome(37), Dracthyr-A(52), Earthen-A(85)
-- Horde races: Orc(2), Undead(5), Tauren(6), Troll(8), Goblin(9), Blood Elf(10),
--              Pandaren-H(26), Nightborne(27), Highmountain(28), Mag'har(36), Zandalari(31),
--              Vulpera(35), Dracthyr-H(70), Earthen-H(84)

-- Warrior (class 1) - All races
INSERT INTO `playerbot_class_race_matrix` (`class_id`, `race_id`, `race_name`, `faction`, `weight`) VALUES
(1, 1, 'Human', 'ALLIANCE', 1.5), (1, 3, 'Dwarf', 'ALLIANCE', 1.0), (1, 4, 'Night Elf', 'ALLIANCE', 1.0),
(1, 7, 'Gnome', 'ALLIANCE', 0.5), (1, 11, 'Draenei', 'ALLIANCE', 0.8), (1, 22, 'Worgen', 'ALLIANCE', 0.7),
(1, 25, 'Pandaren', 'ALLIANCE', 0.4), (1, 29, 'Void Elf', 'ALLIANCE', 0.6), (1, 30, 'Lightforged', 'ALLIANCE', 0.5),
(1, 34, 'Dark Iron', 'ALLIANCE', 0.6), (1, 32, 'Kul Tiran', 'ALLIANCE', 0.5), (1, 37, 'Mechagnome', 'ALLIANCE', 0.3),
(1, 85, 'Earthen', 'ALLIANCE', 0.7),
(1, 2, 'Orc', 'HORDE', 1.5), (1, 5, 'Undead', 'HORDE', 1.0), (1, 6, 'Tauren', 'HORDE', 1.2),
(1, 8, 'Troll', 'HORDE', 0.8), (1, 9, 'Goblin', 'HORDE', 0.4), (1, 10, 'Blood Elf', 'HORDE', 1.3),
(1, 26, 'Pandaren', 'HORDE', 0.4), (1, 27, 'Nightborne', 'HORDE', 0.6), (1, 28, 'Highmountain', 'HORDE', 0.7),
(1, 36, 'Mag''har', 'HORDE', 0.8), (1, 31, 'Zandalari', 'HORDE', 0.7), (1, 35, 'Vulpera', 'HORDE', 0.5),
(1, 84, 'Earthen', 'HORDE', 0.7);

-- Paladin (class 2) - Limited races
INSERT INTO `playerbot_class_race_matrix` (`class_id`, `race_id`, `race_name`, `faction`, `weight`) VALUES
(2, 1, 'Human', 'ALLIANCE', 2.0), (2, 3, 'Dwarf', 'ALLIANCE', 1.2), (2, 11, 'Draenei', 'ALLIANCE', 1.5),
(2, 30, 'Lightforged', 'ALLIANCE', 1.0), (2, 34, 'Dark Iron', 'ALLIANCE', 0.8), (2, 85, 'Earthen', 'ALLIANCE', 0.7),
(2, 6, 'Tauren', 'HORDE', 1.3), (2, 10, 'Blood Elf', 'HORDE', 2.0), (2, 31, 'Zandalari', 'HORDE', 1.0),
(2, 84, 'Earthen', 'HORDE', 0.7);

-- Hunter (class 3) - All races
INSERT INTO `playerbot_class_race_matrix` (`class_id`, `race_id`, `race_name`, `faction`, `weight`) VALUES
(3, 1, 'Human', 'ALLIANCE', 1.0), (3, 3, 'Dwarf', 'ALLIANCE', 1.5), (3, 4, 'Night Elf', 'ALLIANCE', 1.3),
(3, 7, 'Gnome', 'ALLIANCE', 0.4), (3, 11, 'Draenei', 'ALLIANCE', 0.7), (3, 22, 'Worgen', 'ALLIANCE', 0.8),
(3, 25, 'Pandaren', 'ALLIANCE', 0.4), (3, 29, 'Void Elf', 'ALLIANCE', 0.6), (3, 30, 'Lightforged', 'ALLIANCE', 0.4),
(3, 34, 'Dark Iron', 'ALLIANCE', 0.5), (3, 32, 'Kul Tiran', 'ALLIANCE', 0.6), (3, 37, 'Mechagnome', 'ALLIANCE', 0.3),
(3, 85, 'Earthen', 'ALLIANCE', 0.6),
(3, 2, 'Orc', 'HORDE', 1.4), (3, 5, 'Undead', 'HORDE', 0.7), (3, 6, 'Tauren', 'HORDE', 0.9),
(3, 8, 'Troll', 'HORDE', 1.5), (3, 9, 'Goblin', 'HORDE', 0.5), (3, 10, 'Blood Elf', 'HORDE', 1.2),
(3, 26, 'Pandaren', 'HORDE', 0.4), (3, 27, 'Nightborne', 'HORDE', 0.5), (3, 28, 'Highmountain', 'HORDE', 0.8),
(3, 36, 'Mag''har', 'HORDE', 0.9), (3, 31, 'Zandalari', 'HORDE', 0.8), (3, 35, 'Vulpera', 'HORDE', 0.7),
(3, 84, 'Earthen', 'HORDE', 0.6);

-- Rogue (class 4) - Most races except Tauren, Highmountain, Kul Tiran, Dracthyr
INSERT INTO `playerbot_class_race_matrix` (`class_id`, `race_id`, `race_name`, `faction`, `weight`) VALUES
(4, 1, 'Human', 'ALLIANCE', 1.3), (4, 3, 'Dwarf', 'ALLIANCE', 0.7), (4, 4, 'Night Elf', 'ALLIANCE', 1.5),
(4, 7, 'Gnome', 'ALLIANCE', 0.8), (4, 22, 'Worgen', 'ALLIANCE', 1.0), (4, 25, 'Pandaren', 'ALLIANCE', 0.4),
(4, 29, 'Void Elf', 'ALLIANCE', 0.9), (4, 34, 'Dark Iron', 'ALLIANCE', 0.6), (4, 37, 'Mechagnome', 'ALLIANCE', 0.4),
(4, 85, 'Earthen', 'ALLIANCE', 0.5),
(4, 2, 'Orc', 'HORDE', 0.9), (4, 5, 'Undead', 'HORDE', 1.4), (4, 8, 'Troll', 'HORDE', 1.0),
(4, 9, 'Goblin', 'HORDE', 0.8), (4, 10, 'Blood Elf', 'HORDE', 1.6), (4, 26, 'Pandaren', 'HORDE', 0.4),
(4, 27, 'Nightborne', 'HORDE', 0.7), (4, 36, 'Mag''har', 'HORDE', 0.6), (4, 35, 'Vulpera', 'HORDE', 0.9),
(4, 84, 'Earthen', 'HORDE', 0.5);

-- Priest (class 5) - All races except Orcs, and excludes some
INSERT INTO `playerbot_class_race_matrix` (`class_id`, `race_id`, `race_name`, `faction`, `weight`) VALUES
(5, 1, 'Human', 'ALLIANCE', 1.5), (5, 3, 'Dwarf', 'ALLIANCE', 1.0), (5, 4, 'Night Elf', 'ALLIANCE', 1.2),
(5, 7, 'Gnome', 'ALLIANCE', 0.6), (5, 11, 'Draenei', 'ALLIANCE', 1.3), (5, 22, 'Worgen', 'ALLIANCE', 0.5),
(5, 25, 'Pandaren', 'ALLIANCE', 0.5), (5, 29, 'Void Elf', 'ALLIANCE', 1.0), (5, 30, 'Lightforged', 'ALLIANCE', 0.8),
(5, 34, 'Dark Iron', 'ALLIANCE', 0.6), (5, 32, 'Kul Tiran', 'ALLIANCE', 0.5), (5, 37, 'Mechagnome', 'ALLIANCE', 0.4),
(5, 85, 'Earthen', 'ALLIANCE', 0.6),
(5, 5, 'Undead', 'HORDE', 1.4), (5, 6, 'Tauren', 'HORDE', 0.8), (5, 8, 'Troll', 'HORDE', 0.9),
(5, 9, 'Goblin', 'HORDE', 0.5), (5, 10, 'Blood Elf', 'HORDE', 1.6), (5, 26, 'Pandaren', 'HORDE', 0.5),
(5, 27, 'Nightborne', 'HORDE', 0.8), (5, 31, 'Zandalari', 'HORDE', 0.7), (5, 35, 'Vulpera', 'HORDE', 0.6),
(5, 84, 'Earthen', 'HORDE', 0.6);

-- Death Knight (class 6) - All races (Allied races require achievement)
INSERT INTO `playerbot_class_race_matrix` (`class_id`, `race_id`, `race_name`, `faction`, `weight`) VALUES
(6, 1, 'Human', 'ALLIANCE', 1.3), (6, 3, 'Dwarf', 'ALLIANCE', 0.9), (6, 4, 'Night Elf', 'ALLIANCE', 1.1),
(6, 7, 'Gnome', 'ALLIANCE', 0.5), (6, 11, 'Draenei', 'ALLIANCE', 0.8), (6, 22, 'Worgen', 'ALLIANCE', 1.0),
(6, 25, 'Pandaren', 'ALLIANCE', 0.3), (6, 29, 'Void Elf', 'ALLIANCE', 0.7), (6, 30, 'Lightforged', 'ALLIANCE', 0.4),
(6, 34, 'Dark Iron', 'ALLIANCE', 0.6), (6, 32, 'Kul Tiran', 'ALLIANCE', 0.5), (6, 37, 'Mechagnome', 'ALLIANCE', 0.3),
(6, 85, 'Earthen', 'ALLIANCE', 0.5),
(6, 2, 'Orc', 'HORDE', 1.2), (6, 5, 'Undead', 'HORDE', 1.5), (6, 6, 'Tauren', 'HORDE', 0.9),
(6, 8, 'Troll', 'HORDE', 0.8), (6, 9, 'Goblin', 'HORDE', 0.4), (6, 10, 'Blood Elf', 'HORDE', 1.4),
(6, 26, 'Pandaren', 'HORDE', 0.3), (6, 27, 'Nightborne', 'HORDE', 0.6), (6, 28, 'Highmountain', 'HORDE', 0.5),
(6, 36, 'Mag''har', 'HORDE', 0.7), (6, 31, 'Zandalari', 'HORDE', 0.6), (6, 35, 'Vulpera', 'HORDE', 0.5),
(6, 84, 'Earthen', 'HORDE', 0.5);

-- Shaman (class 7) - Limited races
INSERT INTO `playerbot_class_race_matrix` (`class_id`, `race_id`, `race_name`, `faction`, `weight`) VALUES
(7, 3, 'Dwarf', 'ALLIANCE', 1.2), (7, 11, 'Draenei', 'ALLIANCE', 1.5), (7, 25, 'Pandaren', 'ALLIANCE', 0.6),
(7, 34, 'Dark Iron', 'ALLIANCE', 0.8), (7, 32, 'Kul Tiran', 'ALLIANCE', 0.7), (7, 85, 'Earthen', 'ALLIANCE', 0.8),
(7, 2, 'Orc', 'HORDE', 1.5), (7, 6, 'Tauren', 'HORDE', 1.3), (7, 8, 'Troll', 'HORDE', 1.2),
(7, 9, 'Goblin', 'HORDE', 0.6), (7, 26, 'Pandaren', 'HORDE', 0.5), (7, 28, 'Highmountain', 'HORDE', 1.0),
(7, 36, 'Mag''har', 'HORDE', 0.9), (7, 31, 'Zandalari', 'HORDE', 1.1), (7, 35, 'Vulpera', 'HORDE', 0.7),
(7, 84, 'Earthen', 'HORDE', 0.8);

-- Mage (class 8) - All races
INSERT INTO `playerbot_class_race_matrix` (`class_id`, `race_id`, `race_name`, `faction`, `weight`) VALUES
(8, 1, 'Human', 'ALLIANCE', 1.4), (8, 3, 'Dwarf', 'ALLIANCE', 0.6), (8, 4, 'Night Elf', 'ALLIANCE', 0.9),
(8, 7, 'Gnome', 'ALLIANCE', 1.2), (8, 11, 'Draenei', 'ALLIANCE', 0.7), (8, 22, 'Worgen', 'ALLIANCE', 0.6),
(8, 25, 'Pandaren', 'ALLIANCE', 0.5), (8, 29, 'Void Elf', 'ALLIANCE', 1.0), (8, 30, 'Lightforged', 'ALLIANCE', 0.4),
(8, 34, 'Dark Iron', 'ALLIANCE', 0.5), (8, 32, 'Kul Tiran', 'ALLIANCE', 0.4), (8, 37, 'Mechagnome', 'ALLIANCE', 0.6),
(8, 85, 'Earthen', 'ALLIANCE', 0.5),
(8, 2, 'Orc', 'HORDE', 0.7), (8, 5, 'Undead', 'HORDE', 1.1), (8, 8, 'Troll', 'HORDE', 1.0),
(8, 9, 'Goblin', 'HORDE', 0.7), (8, 10, 'Blood Elf', 'HORDE', 1.8), (8, 26, 'Pandaren', 'HORDE', 0.5),
(8, 27, 'Nightborne', 'HORDE', 1.2), (8, 35, 'Vulpera', 'HORDE', 0.6), (8, 84, 'Earthen', 'HORDE', 0.5);

-- Warlock (class 9) - Limited races (no Draenei, Tauren, etc.)
INSERT INTO `playerbot_class_race_matrix` (`class_id`, `race_id`, `race_name`, `faction`, `weight`) VALUES
(9, 1, 'Human', 'ALLIANCE', 1.5), (9, 3, 'Dwarf', 'ALLIANCE', 0.6), (9, 7, 'Gnome', 'ALLIANCE', 1.0),
(9, 22, 'Worgen', 'ALLIANCE', 0.8), (9, 29, 'Void Elf', 'ALLIANCE', 1.1), (9, 34, 'Dark Iron', 'ALLIANCE', 0.7),
(9, 37, 'Mechagnome', 'ALLIANCE', 0.5), (9, 85, 'Earthen', 'ALLIANCE', 0.5),
(9, 2, 'Orc', 'HORDE', 1.2), (9, 5, 'Undead', 'HORDE', 1.5), (9, 8, 'Troll', 'HORDE', 0.8),
(9, 9, 'Goblin', 'HORDE', 0.7), (9, 10, 'Blood Elf', 'HORDE', 1.6), (9, 27, 'Nightborne', 'HORDE', 0.9),
(9, 35, 'Vulpera', 'HORDE', 0.7), (9, 84, 'Earthen', 'HORDE', 0.5);

-- Monk (class 10) - All races except Goblins, Worgen, Lightforged
INSERT INTO `playerbot_class_race_matrix` (`class_id`, `race_id`, `race_name`, `faction`, `weight`) VALUES
(10, 1, 'Human', 'ALLIANCE', 1.2), (10, 3, 'Dwarf', 'ALLIANCE', 0.7), (10, 4, 'Night Elf', 'ALLIANCE', 1.1),
(10, 7, 'Gnome', 'ALLIANCE', 0.6), (10, 11, 'Draenei', 'ALLIANCE', 0.8), (10, 25, 'Pandaren', 'ALLIANCE', 1.8),
(10, 29, 'Void Elf', 'ALLIANCE', 0.7), (10, 34, 'Dark Iron', 'ALLIANCE', 0.6), (10, 32, 'Kul Tiran', 'ALLIANCE', 0.5),
(10, 37, 'Mechagnome', 'ALLIANCE', 0.4), (10, 85, 'Earthen', 'ALLIANCE', 0.6),
(10, 2, 'Orc', 'HORDE', 0.9), (10, 5, 'Undead', 'HORDE', 0.8), (10, 6, 'Tauren', 'HORDE', 1.0),
(10, 8, 'Troll', 'HORDE', 1.1), (10, 10, 'Blood Elf', 'HORDE', 1.4), (10, 26, 'Pandaren', 'HORDE', 1.8),
(10, 27, 'Nightborne', 'HORDE', 0.7), (10, 28, 'Highmountain', 'HORDE', 0.8), (10, 36, 'Mag''har', 'HORDE', 0.6),
(10, 31, 'Zandalari', 'HORDE', 0.9), (10, 35, 'Vulpera', 'HORDE', 0.8), (10, 84, 'Earthen', 'HORDE', 0.6);

-- Druid (class 11) - Very limited races
INSERT INTO `playerbot_class_race_matrix` (`class_id`, `race_id`, `race_name`, `faction`, `weight`) VALUES
(11, 4, 'Night Elf', 'ALLIANCE', 2.0), (11, 22, 'Worgen', 'ALLIANCE', 1.0), (11, 32, 'Kul Tiran', 'ALLIANCE', 0.7),
(11, 6, 'Tauren', 'HORDE', 1.8), (11, 8, 'Troll', 'HORDE', 1.2), (11, 28, 'Highmountain', 'HORDE', 0.8),
(11, 31, 'Zandalari', 'HORDE', 1.0);

-- Demon Hunter (class 12) - Night Elf and Blood Elf only
INSERT INTO `playerbot_class_race_matrix` (`class_id`, `race_id`, `race_name`, `faction`, `weight`) VALUES
(12, 4, 'Night Elf', 'ALLIANCE', 2.0),
(12, 10, 'Blood Elf', 'HORDE', 2.0);

-- Evoker (class 13) - Dracthyr only
INSERT INTO `playerbot_class_race_matrix` (`class_id`, `race_id`, `race_name`, `faction`, `weight`) VALUES
(13, 52, 'Dracthyr', 'ALLIANCE', 1.0),
(13, 70, 'Dracthyr', 'HORDE', 1.0);

-- ============================================================================
-- TABLE: playerbot_bot_templates
-- ============================================================================
-- Main template registry. One row per class/spec combination.
-- Contains metadata and references to gear/talent/actionbar data.
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_bot_templates`;
CREATE TABLE `playerbot_bot_templates` (
    `template_id` INT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'Unique template ID',
    `spec_id` INT UNSIGNED NOT NULL COMMENT 'Specialization ID (FK to spec_info)',
    `class_id` TINYINT UNSIGNED NOT NULL COMMENT 'Denormalized class ID for fast queries',
    `role` TINYINT UNSIGNED NOT NULL DEFAULT 2 COMMENT 'Combat role: 0=Tank, 1=Healer, 2=DPS',

    -- Metadata
    `template_name` VARCHAR(64) NOT NULL COMMENT 'Human-readable name (e.g., Warrior_Arms)',
    `version` INT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'Template version for updates',
    `patch_version` VARCHAR(16) DEFAULT '12.0.0' COMMENT 'WoW patch this template is for',

    -- Status
    `enabled` TINYINT(1) NOT NULL DEFAULT 1 COMMENT 'Whether template is active',
    `validated` TINYINT(1) NOT NULL DEFAULT 0 COMMENT 'Whether template has been validated',
    `last_validated` TIMESTAMP NULL DEFAULT NULL COMMENT 'Last validation timestamp',

    -- Pre-serialized data blobs (for fast cloning)
    `talent_blob` TEXT DEFAULT NULL COMMENT 'Hex-encoded serialized talent data',
    `actionbar_blob` TEXT DEFAULT NULL COMMENT 'Hex-encoded serialized action bar data',

    -- Configuration
    `default_pvp_talents` TINYINT(1) NOT NULL DEFAULT 0 COMMENT 'Use PvP talent defaults',
    `hero_talent_tree_id` INT UNSIGNED DEFAULT NULL COMMENT 'Default hero talent tree',
    `priority_weight` INT UNSIGNED NOT NULL DEFAULT 100 COMMENT 'Selection priority weight',

    -- Timestamps
    `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    PRIMARY KEY (`template_id`),
    UNIQUE KEY `uk_spec_id` (`spec_id`),
    INDEX `idx_enabled` (`enabled`),
    INDEX `idx_validated` (`validated`),
    INDEX `idx_class_role` (`class_id`, `role`),
    CONSTRAINT `fk_template_spec` FOREIGN KEY (`spec_id`)
        REFERENCES `playerbot_spec_info`(`spec_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Main bot template registry';

-- Auto-generate templates for all specs
INSERT INTO `playerbot_bot_templates` (`spec_id`, `class_id`, `role`, `template_name`, `enabled`)
SELECT
    `spec_id`,
    `class_id`,
    CASE `role` WHEN 'TANK' THEN 0 WHEN 'HEALER' THEN 1 ELSE 2 END AS `role`,
    CONCAT(`class_name`, '_', `spec_name`) AS `template_name`,
    1 AS `enabled`
FROM `playerbot_spec_info`
WHERE `enabled` = 1;

-- ============================================================================
-- TABLE: playerbot_template_gear_sets
-- ============================================================================
-- Gear set configurations for each template at different item level tiers.
-- Each template can have multiple gear sets for different content levels.
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_template_gear_sets`;
CREATE TABLE `playerbot_template_gear_sets` (
    `gear_set_id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `template_id` INT UNSIGNED NOT NULL COMMENT 'FK to templates',
    `target_ilvl` INT UNSIGNED NOT NULL COMMENT 'Target average item level',
    `actual_gear_score` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Calculated gear score',
    `gear_set_name` VARCHAR(64) DEFAULT NULL COMMENT 'Human-readable name',
    `content_tier` VARCHAR(32) DEFAULT NULL COMMENT 'Content tier (e.g., Heroic_Dungeon, Normal_Raid)',
    `enabled` TINYINT(1) NOT NULL DEFAULT 1,

    PRIMARY KEY (`gear_set_id`),
    UNIQUE KEY `uk_template_ilvl` (`template_id`, `target_ilvl`),
    INDEX `idx_item_level` (`target_ilvl`),
    CONSTRAINT `fk_gearset_template` FOREIGN KEY (`template_id`)
        REFERENCES `playerbot_bot_templates`(`template_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Gear set definitions per template and item level tier';

-- ============================================================================
-- TABLE: playerbot_template_gear_items
-- ============================================================================
-- Individual gear slot items for each gear set.
-- Each gear set has up to 19 equipment slots.
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_template_gear_items`;
CREATE TABLE `playerbot_template_gear_items` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `gear_set_id` INT UNSIGNED NOT NULL COMMENT 'FK to gear_sets',
    `slot_id` TINYINT UNSIGNED NOT NULL COMMENT 'Equipment slot (0-18)',
    `item_id` INT UNSIGNED NOT NULL COMMENT 'Item entry ID',
    `item_level` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Item level',
    `enchant_id` INT UNSIGNED DEFAULT 0 COMMENT 'Enchant ID',
    `gem1_id` INT UNSIGNED DEFAULT 0 COMMENT 'First gem ID',
    `gem2_id` INT UNSIGNED DEFAULT 0 COMMENT 'Second gem ID',
    `gem3_id` INT UNSIGNED DEFAULT 0 COMMENT 'Third gem ID',
    `bonus_list` VARCHAR(64) DEFAULT NULL COMMENT 'Comma-separated bonus IDs',

    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_gearset_slot` (`gear_set_id`, `slot_id`),
    INDEX `idx_item_id` (`item_id`),
    CONSTRAINT `fk_gearitem_gearset` FOREIGN KEY (`gear_set_id`)
        REFERENCES `playerbot_template_gear_sets`(`gear_set_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Individual gear items per slot for each gear set';

-- ============================================================================
-- TABLE: playerbot_template_talents
-- ============================================================================
-- Talent configurations for each template.
-- Stores the selected talents from the talent tree.
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_template_talents`;
CREATE TABLE `playerbot_template_talents` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `template_id` INT UNSIGNED NOT NULL COMMENT 'FK to templates',
    `talent_tier` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Talent tier/row (0-6)',
    `talent_column` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Talent column (0-2)',
    `talent_id` INT UNSIGNED NOT NULL COMMENT 'Talent spell ID',
    `is_pvp_talent` TINYINT(1) NOT NULL DEFAULT 0 COMMENT 'Whether this is a PvP talent',
    `enabled` TINYINT(1) NOT NULL DEFAULT 1 COMMENT 'Whether this talent is enabled',

    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_template_tier_col` (`template_id`, `talent_tier`, `talent_column`, `is_pvp_talent`),
    INDEX `idx_template_pvp` (`template_id`, `is_pvp_talent`),
    CONSTRAINT `fk_talent_template` FOREIGN KEY (`template_id`)
        REFERENCES `playerbot_bot_templates`(`template_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Talent selections for each template';

-- ============================================================================
-- TABLE: playerbot_template_actionbars
-- ============================================================================
-- Action bar configurations for each template.
-- Maps ability/item/macro to specific bar slots.
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_template_actionbars`;
CREATE TABLE `playerbot_template_actionbars` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `template_id` INT UNSIGNED NOT NULL COMMENT 'FK to templates',
    `action_bar` TINYINT UNSIGNED NOT NULL COMMENT 'Action bar number (0-7)',
    `slot` TINYINT UNSIGNED NOT NULL COMMENT 'Slot on bar (0-11)',
    `action_type` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=Spell, 1=Item, 64=Macro, 128=Companion',
    `action_id` INT UNSIGNED NOT NULL COMMENT 'Spell/Item/Macro ID',
    `enabled` TINYINT(1) NOT NULL DEFAULT 1 COMMENT 'Whether this action is enabled',
    `priority` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'For rotation priority hints',
    `is_rotational` TINYINT(1) NOT NULL DEFAULT 0 COMMENT 'Used in DPS rotation',
    `is_defensive` TINYINT(1) NOT NULL DEFAULT 0 COMMENT 'Defensive cooldown',
    `is_interrupt` TINYINT(1) NOT NULL DEFAULT 0 COMMENT 'Interrupt ability',
    `description` VARCHAR(64) DEFAULT NULL COMMENT 'Ability name for reference',

    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_template_bar_slot` (`template_id`, `action_bar`, `slot`),
    INDEX `idx_template` (`template_id`),
    INDEX `idx_action_id` (`action_id`),
    INDEX `idx_enabled` (`enabled`),
    CONSTRAINT `fk_actionbar_template` FOREIGN KEY (`template_id`)
        REFERENCES `playerbot_bot_templates`(`template_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Action bar configurations for each template';

-- ============================================================================
-- TABLE: playerbot_template_statistics
-- ============================================================================
-- Usage statistics for templates (for optimization).
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_template_statistics`;
CREATE TABLE `playerbot_template_statistics` (
    `template_id` INT UNSIGNED NOT NULL,
    `total_uses` BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Total times cloned',
    `last_used` TIMESTAMP NULL DEFAULT NULL,
    `avg_creation_time_ms` INT UNSIGNED NOT NULL DEFAULT 0,
    `min_creation_time_ms` INT UNSIGNED NOT NULL DEFAULT 0,
    `max_creation_time_ms` INT UNSIGNED NOT NULL DEFAULT 0,
    `successful_clones` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `failed_clones` BIGINT UNSIGNED NOT NULL DEFAULT 0,

    PRIMARY KEY (`template_id`),
    CONSTRAINT `fk_stats_template` FOREIGN KEY (`template_id`)
        REFERENCES `playerbot_bot_templates`(`template_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Template usage statistics';

-- Initialize statistics for all templates
INSERT INTO `playerbot_template_statistics` (`template_id`)
SELECT `template_id` FROM `playerbot_bot_templates`;

-- ============================================================================
-- TABLE: playerbot_template_config
-- ============================================================================
-- Configuration for the template system.
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_template_config`;
CREATE TABLE `playerbot_template_config` (
    `config_key` VARCHAR(64) NOT NULL,
    `config_value` VARCHAR(255) NOT NULL DEFAULT '',
    `description` VARCHAR(255) DEFAULT NULL,
    `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    PRIMARY KEY (`config_key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Template system configuration';

INSERT INTO `playerbot_template_config` (`config_key`, `config_value`, `description`) VALUES
('enabled', '1', 'Master enable for template system'),
('auto_validate', '1', 'Automatically validate templates on load'),
('default_gear_ilvl_offset', '0', 'Item level offset from target'),
('prefer_set_bonuses', '1', 'Prefer tier set items when available'),
('use_pvp_talents_in_bg', '1', 'Apply PvP talents when entering battlegrounds'),
('default_gear_tier', '450', 'Default item level for new templates'),
('cache_templates', '1', 'Cache templates in memory'),
('log_template_usage', '1', 'Log template clone operations'),
('validate_gear_on_clone', '0', 'Validate all gear items exist before cloning'),
('fallback_to_defaults', '1', 'Use default templates if DB load fails');

-- ============================================================================
-- VIEWS FOR TEMPLATE MANAGEMENT
-- ============================================================================

-- Complete template view with spec info
CREATE OR REPLACE VIEW `v_template_details` AS
SELECT
    t.`template_id`,
    t.`template_name`,
    t.`spec_id`,
    t.`class_id`,
    s.`class_name`,
    s.`spec_name`,
    t.`role`,
    s.`armor_type`,
    s.`primary_stat`,
    t.`enabled`,
    t.`validated`,
    t.`version`,
    t.`patch_version`,
    ts.`total_uses`,
    ts.`avg_creation_time_ms`
FROM `playerbot_bot_templates` t
JOIN `playerbot_spec_info` s ON t.`spec_id` = s.`spec_id`
LEFT JOIN `playerbot_template_statistics` ts ON t.`template_id` = ts.`template_id`;

-- Templates by role view
CREATE OR REPLACE VIEW `v_templates_by_role` AS
SELECT
    s.`role`,
    COUNT(*) as `template_count`,
    SUM(CASE WHEN t.`enabled` = 1 THEN 1 ELSE 0 END) as `enabled_count`,
    SUM(CASE WHEN t.`validated` = 1 THEN 1 ELSE 0 END) as `validated_count`
FROM `playerbot_bot_templates` t
JOIN `playerbot_spec_info` s ON t.`spec_id` = s.`spec_id`
GROUP BY s.`role`;

-- Gear sets overview
CREATE OR REPLACE VIEW `v_gear_sets_overview` AS
SELECT
    t.`template_name`,
    gs.`target_ilvl`,
    gs.`gear_set_name`,
    gs.`content_tier`,
    gs.`actual_gear_score`,
    COUNT(gi.`id`) as `items_defined`
FROM `playerbot_bot_templates` t
JOIN `playerbot_template_gear_sets` gs ON t.`template_id` = gs.`template_id`
LEFT JOIN `playerbot_template_gear_items` gi ON gs.`gear_set_id` = gi.`gear_set_id`
GROUP BY t.`template_name`, gs.`gear_set_id`;

-- ============================================================================
-- STORED PROCEDURES
-- ============================================================================

DELIMITER //

-- Procedure to validate a template
CREATE PROCEDURE IF NOT EXISTS `ValidateTemplate`(IN p_template_id INT UNSIGNED)
BEGIN
    DECLARE v_spec_id INT UNSIGNED;
    DECLARE v_gear_count INT;
    DECLARE v_talent_count INT;
    DECLARE v_is_valid TINYINT DEFAULT 1;

    -- Get spec ID
    SELECT `spec_id` INTO v_spec_id FROM `playerbot_bot_templates` WHERE `template_id` = p_template_id;

    IF v_spec_id IS NULL THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Template not found';
    END IF;

    -- Check gear sets exist
    SELECT COUNT(*) INTO v_gear_count
    FROM `playerbot_template_gear_sets`
    WHERE `template_id` = p_template_id AND `enabled` = 1;

    IF v_gear_count = 0 THEN
        SET v_is_valid = 0;
    END IF;

    -- Check talents exist
    SELECT COUNT(*) INTO v_talent_count
    FROM `playerbot_template_talents`
    WHERE `template_id` = p_template_id;

    IF v_talent_count < 5 THEN  -- Minimum expected talents
        SET v_is_valid = 0;
    END IF;

    -- Update validation status
    UPDATE `playerbot_bot_templates`
    SET `validated` = v_is_valid, `last_validated` = NOW()
    WHERE `template_id` = p_template_id;

    SELECT v_is_valid AS `is_valid`, v_gear_count AS `gear_sets`, v_talent_count AS `talents`;
END //

-- Procedure to get best template for role/faction
CREATE PROCEDURE IF NOT EXISTS `GetBestTemplate`(
    IN p_role VARCHAR(16),
    IN p_faction VARCHAR(16),
    IN p_preferred_class TINYINT UNSIGNED
)
BEGIN
    SELECT
        t.`template_id`,
        t.`template_name`,
        s.`class_id`,
        s.`class_name`,
        s.`spec_name`,
        ts.`avg_creation_time_ms`
    FROM `playerbot_bot_templates` t
    JOIN `playerbot_spec_info` s ON t.`spec_id` = s.`spec_id`
    JOIN `playerbot_class_race_matrix` cr ON s.`class_id` = cr.`class_id` AND cr.`faction` = p_faction
    LEFT JOIN `playerbot_template_statistics` ts ON t.`template_id` = ts.`template_id`
    WHERE s.`role` = p_role
      AND t.`enabled` = 1
      AND cr.`enabled` = 1
    ORDER BY
        CASE WHEN s.`class_id` = p_preferred_class THEN 0 ELSE 1 END,
        ts.`avg_creation_time_ms` ASC
    LIMIT 1;
END //

-- Procedure to record template usage
CREATE PROCEDURE IF NOT EXISTS `RecordTemplateUsage`(
    IN p_template_id INT UNSIGNED,
    IN p_creation_time_ms INT UNSIGNED,
    IN p_success TINYINT
)
BEGIN
    INSERT INTO `playerbot_template_statistics`
        (`template_id`, `total_uses`, `last_used`, `avg_creation_time_ms`,
         `min_creation_time_ms`, `max_creation_time_ms`, `successful_clones`, `failed_clones`)
    VALUES
        (p_template_id, 1, NOW(), p_creation_time_ms, p_creation_time_ms, p_creation_time_ms,
         IF(p_success = 1, 1, 0), IF(p_success = 0, 1, 0))
    ON DUPLICATE KEY UPDATE
        `total_uses` = `total_uses` + 1,
        `last_used` = NOW(),
        `avg_creation_time_ms` = ((`avg_creation_time_ms` * `total_uses`) + p_creation_time_ms) / (`total_uses` + 1),
        `min_creation_time_ms` = LEAST(`min_creation_time_ms`, p_creation_time_ms),
        `max_creation_time_ms` = GREATEST(`max_creation_time_ms`, p_creation_time_ms),
        `successful_clones` = `successful_clones` + IF(p_success = 1, 1, 0),
        `failed_clones` = `failed_clones` + IF(p_success = 0, 1, 0);
END //

DELIMITER ;

-- ============================================================================
-- DONE
-- ============================================================================

SELECT 'Bot Template System schema created successfully!' AS status;
SELECT COUNT(*) AS template_count FROM `playerbot_bot_templates`;
SELECT COUNT(*) AS spec_count FROM `playerbot_spec_info`;
SELECT COUNT(*) AS race_class_combos FROM `playerbot_class_race_matrix`;
