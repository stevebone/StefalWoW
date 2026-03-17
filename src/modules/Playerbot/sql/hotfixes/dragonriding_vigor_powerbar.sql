-- ============================================================================
-- Playerbot Dragonriding System - Database Tables
--
-- All tables use the Playerbot database to avoid modifying TrinityCore's
-- core auth/world/characters databases. Account-wide progression is still
-- supported by using the account_id field.
--
-- Install into: Playerbot database (same as consolidated_playerbot_schema.sql)
-- ============================================================================

-- Set default engine and charset
SET default_storage_engine = InnoDB;
SET NAMES utf8mb4 COLLATE utf8mb4_unicode_ci;

-- ============================================================================
-- DRAGONRIDING PROGRESSION TABLES
-- ============================================================================

-- Account-wide Dragon Glyph collection tracking
-- Matches retail WoW - glyphs collected once are available on all characters
CREATE TABLE IF NOT EXISTS `playerbot_dragonriding_glyphs` (
    `account_id` BIGINT UNSIGNED NOT NULL COMMENT 'Account ID (links to auth.account)',
    `glyph_id` INT UNSIGNED NOT NULL COMMENT 'Unique glyph ID (1-72+)',
    `collected_time` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'When the glyph was collected',
    `character_guid` BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Character that collected it (for logging)',
    `map_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Map where collected',
    `zone_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Zone where collected',
    PRIMARY KEY (`account_id`, `glyph_id`),
    INDEX `idx_account` (`account_id`),
    INDEX `idx_collected_time` (`collected_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Account-wide Dragon Glyph collection (retail-accurate)';

-- Account-wide Dragonriding talent selections
-- Matches retail WoW - talents learned once apply to all characters
CREATE TABLE IF NOT EXISTS `playerbot_dragonriding_talents` (
    `account_id` BIGINT UNSIGNED NOT NULL COMMENT 'Account ID (links to auth.account)',
    `talent_id` INT UNSIGNED NOT NULL COMMENT 'Talent ID from DragonridingTalent enum',
    `learned_time` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'When talent was learned',
    `glyphs_spent` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Glyphs spent on this talent',
    PRIMARY KEY (`account_id`, `talent_id`),
    INDEX `idx_account` (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Account-wide Dragonriding talent selections (retail-accurate)';

-- ============================================================================
-- DRAGONRIDING DEFINITION TABLES (World Data)
-- ============================================================================

-- Dragonriding Talent definitions
-- Defines all 11 dragonriding talents with their effects and costs
-- Based on retail Dragonflight talent tree
CREATE TABLE IF NOT EXISTS `playerbot_dragonriding_talent_templates` (
    `talent_id` INT UNSIGNED NOT NULL PRIMARY KEY COMMENT 'Unique talent ID (DragonridingTalent enum)',
    `name` VARCHAR(100) NOT NULL COMMENT 'Talent display name',
    `description` TEXT COMMENT 'Talent description text',
    `branch` ENUM('vigor_capacity', 'regen_grounded', 'regen_flying', 'abilities', 'utility') NOT NULL COMMENT 'Talent tree branch',
    `tier` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'Order in branch (1, 2, 3)',
    `glyph_cost` INT UNSIGNED NOT NULL COMMENT 'Dragon Glyphs required to learn',
    `prerequisite_talent_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Must have this talent first (0=none)',
    `effect_type` ENUM('vigor_max', 'regen_grounded', 'regen_flying', 'regen_ground_skim', 'ability_unlock') NOT NULL COMMENT 'What this talent modifies',
    `effect_value` INT NOT NULL COMMENT 'New value (vigor count, ms regen, or spell ID)'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Dragonriding talent definitions (retail-accurate values)';

-- Dragon Glyph locations in the world
-- Defines all glyph positions in the Dragon Isles (72+ total)
CREATE TABLE IF NOT EXISTS `playerbot_dragonriding_glyph_templates` (
    `glyph_id` INT UNSIGNED NOT NULL PRIMARY KEY COMMENT 'Unique glyph ID',
    `map_id` INT UNSIGNED NOT NULL COMMENT 'Map ID (2444=Dragon Isles)',
    `zone_id` INT UNSIGNED NOT NULL COMMENT 'Zone/area ID',
    `zone_name` VARCHAR(100) COMMENT 'Zone name for reference',
    `pos_x` FLOAT NOT NULL COMMENT 'X coordinate',
    `pos_y` FLOAT NOT NULL COMMENT 'Y coordinate',
    `pos_z` FLOAT NOT NULL COMMENT 'Z coordinate',
    `collection_radius` FLOAT NOT NULL DEFAULT 10.0 COMMENT 'Radius to collect glyph',
    `achievement_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Achievement granted on collection',
    `name` VARCHAR(100) COMMENT 'Glyph name/description',
    INDEX `idx_map_zone` (`map_id`, `zone_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Dragon Glyph world locations';

-- ============================================================================
-- INSERT TALENT DEFINITIONS (Retail Accurate Values)
-- ============================================================================

-- Clear existing data for clean reinstall
DELETE FROM `playerbot_dragonriding_talent_templates`;

-- Vigor Capacity Branch (Left)
INSERT INTO `playerbot_dragonriding_talent_templates` VALUES
(1, 'Take to the Skies', 'Your Vigor is increased to 4.', 'vigor_capacity', 1, 1, 0, 'vigor_max', 4),
(2, 'Dragonriding Learner', 'Your Vigor is increased to 5.', 'vigor_capacity', 2, 4, 1, 'vigor_max', 5),
(3, 'Beyond Infinity', 'Your Vigor is increased to 6.', 'vigor_capacity', 3, 5, 2, 'vigor_max', 6);

-- Vigor Regen (Grounded) Branch (Middle)
INSERT INTO `playerbot_dragonriding_talent_templates` VALUES
(10, 'Dynamic Stretching', 'Your Vigor regeneration rate is accelerated to 1 Vigor every 25 seconds while grounded.', 'regen_grounded', 1, 3, 0, 'regen_grounded', 25000),
(11, 'Restorative Travels', 'Your Vigor regeneration rate is accelerated to 1 Vigor every 20 seconds while grounded.', 'regen_grounded', 2, 4, 10, 'regen_grounded', 20000),
(12, 'Yearning for the Sky', 'Your Vigor regeneration rate is accelerated to 1 Vigor every 15 seconds while grounded.', 'regen_grounded', 3, 5, 11, 'regen_grounded', 15000);

-- Vigor Regen (Flying) Branch (Right)
INSERT INTO `playerbot_dragonriding_talent_templates` VALUES
(20, 'Thrill Chaser', 'Thrill of the Skies generates 1 Vigor every 10 seconds.', 'regen_flying', 1, 3, 0, 'regen_flying', 10000),
(21, 'Thrill Seeker', 'Thrill of the Skies generates 1 Vigor every 5 seconds.', 'regen_flying', 2, 5, 20, 'regen_flying', 5000);

-- Utility Talents
INSERT INTO `playerbot_dragonriding_talent_templates` VALUES
(30, 'Ground Skimming', 'When dragonriding near the ground, you regenerate 1 Vigor every 30 seconds. Does not stack with Thrill of the Skies.', 'utility', 1, 4, 0, 'regen_ground_skim', 30000),
(31, 'Airborne Tumbling', 'Unlocks Whirling Surge ability.', 'abilities', 1, 3, 0, 'ability_unlock', 900004),
(32, 'At Home Aloft', 'Unlocks Aerial Halt ability, allowing you to hover in place.', 'abilities', 2, 2, 0, 'ability_unlock', 900005);

-- ============================================================================
-- INSERT GLYPH LOCATIONS
-- Dragon Isles zones: 2444 = Dragon Isles continent
-- NOTE: These are placeholder coordinates. Update with actual retail positions
-- from Wowhead/Icy Veins when available.
-- ============================================================================

-- Clear existing data for clean reinstall
DELETE FROM `playerbot_dragonriding_glyph_templates`;

-- The Waking Shores Glyphs (16 total)
INSERT INTO `playerbot_dragonriding_glyph_templates` (`glyph_id`, `map_id`, `zone_id`, `zone_name`, `pos_x`, `pos_y`, `pos_z`, `name`) VALUES
(1, 2444, 13644, 'The Waking Shores', -1256.7, -1345.2, 234.5, 'Crumbling Life Archway'),
(2, 2444, 13644, 'The Waking Shores', -1098.3, -1478.9, 287.2, 'Dragonheart Outpost'),
(3, 2444, 13644, 'The Waking Shores', -1423.5, -1123.7, 198.4, 'Flashfrost Enclave'),
(4, 2444, 13644, 'The Waking Shores', -1567.2, -987.4, 312.8, 'Life-Binder Observatory'),
(5, 2444, 13644, 'The Waking Shores', -1234.8, -1567.3, 256.7, 'Obsidian Bulwark'),
(6, 2444, 13644, 'The Waking Shores', -1389.1, -1298.5, 278.9, 'Obsidian Throne'),
(7, 2444, 13644, 'The Waking Shores', -1145.6, -1389.2, 234.1, 'Ruby Life Pools'),
(8, 2444, 13644, 'The Waking Shores', -1298.4, -1456.8, 289.3, 'Scalecracker Keep'),
(9, 2444, 13644, 'The Waking Shores', -1478.9, -1234.6, 312.5, 'Skytop Observatory'),
(10, 2444, 13644, 'The Waking Shores', -1567.3, -1098.7, 267.8, 'The Overflowing Spring'),
(11, 2444, 13644, 'The Waking Shores', -1123.5, -1567.9, 234.2, 'Uktulut Backwater'),
(12, 2444, 13644, 'The Waking Shores', -1389.7, -1389.4, 298.6, 'Uktulut Pier'),
(13, 2444, 13644, 'The Waking Shores', -1234.1, -1234.8, 256.4, 'Winglord''s Perch'),
(14, 2444, 13644, 'The Waking Shores', -1456.8, -1145.3, 312.9, 'Wingrest Embassy'),
(15, 2444, 13644, 'The Waking Shores', -1298.2, -1298.7, 278.5, 'Apex Canopy'),
(16, 2444, 13644, 'The Waking Shores', -1145.9, -1145.6, 234.7, 'Dragonbane Keep');

-- Ohn'ahran Plains Glyphs (12 total)
INSERT INTO `playerbot_dragonriding_glyph_templates` (`glyph_id`, `map_id`, `zone_id`, `zone_name`, `pos_x`, `pos_y`, `pos_z`, `name`) VALUES
(17, 2444, 13645, 'Ohn''ahran Plains', -2456.7, -2345.2, 134.5, 'Emerald Gardens'),
(18, 2444, 13645, 'Ohn''ahran Plains', -2298.3, -2478.9, 187.2, 'Maruukai'),
(19, 2444, 13645, 'Ohn''ahran Plains', -2623.5, -2123.7, 98.4, 'Mirror of the Sky'),
(20, 2444, 13645, 'Ohn''ahran Plains', -2767.2, -1987.4, 212.8, 'Nokhudon Hold'),
(21, 2444, 13645, 'Ohn''ahran Plains', -2434.8, -2567.3, 156.7, 'Ohn''iri Springs'),
(22, 2444, 13645, 'Ohn''ahran Plains', -2589.1, -2298.5, 178.9, 'Pinewood Post'),
(23, 2444, 13645, 'Ohn''ahran Plains', -2345.6, -2389.2, 134.1, 'Rusza''thar Reach'),
(24, 2444, 13645, 'Ohn''ahran Plains', -2498.4, -2456.8, 189.3, 'Shady Sanctuary'),
(25, 2444, 13645, 'Ohn''ahran Plains', -2678.9, -2234.6, 212.5, 'Szar Skeleth'),
(26, 2444, 13645, 'Ohn''ahran Plains', -2767.3, -2098.7, 167.8, 'The Eternal Kurgans'),
(27, 2444, 13645, 'Ohn''ahran Plains', -2323.5, -2567.9, 134.2, 'Windsong Rise'),
(28, 2444, 13645, 'Ohn''ahran Plains', -2589.7, -2389.4, 198.6, 'Ohn''ahra''s Roost');

-- The Azure Span Glyphs (14 total)
INSERT INTO `playerbot_dragonriding_glyph_templates` (`glyph_id`, `map_id`, `zone_id`, `zone_name`, `pos_x`, `pos_y`, `pos_z`, `name`) VALUES
(29, 2444, 13646, 'The Azure Span', -3456.7, -3345.2, 34.5, 'Azure Archives'),
(30, 2444, 13646, 'The Azure Span', -3298.3, -3478.9, 87.2, 'Camp Antonidas'),
(31, 2444, 13646, 'The Azure Span', -3623.5, -3123.7, -1.6, 'Cobalt Assembly'),
(32, 2444, 13646, 'The Azure Span', -3767.2, -2987.4, 112.8, 'Creektooth Den'),
(33, 2444, 13646, 'The Azure Span', -3434.8, -3567.3, 56.7, 'Forkriver Crossing'),
(34, 2444, 13646, 'The Azure Span', -3589.1, -3298.5, 78.9, 'Imbu'),
(35, 2444, 13646, 'The Azure Span', -3345.6, -3389.2, 34.1, 'Iskaara'),
(36, 2444, 13646, 'The Azure Span', -3498.4, -3456.8, 89.3, 'Kalthraz Fortress'),
(37, 2444, 13646, 'The Azure Span', -3678.9, -3234.6, 112.5, 'Lost Ruins'),
(38, 2444, 13646, 'The Azure Span', -3767.3, -3098.7, 67.8, 'Rhonin''s Shield'),
(39, 2444, 13646, 'The Azure Span', -3323.5, -3567.9, 34.2, 'Ruins of Karnthar'),
(40, 2444, 13646, 'The Azure Span', -3589.7, -3389.4, 98.6, 'The Fallen Course'),
(41, 2444, 13646, 'The Azure Span', -3434.1, -3234.8, 56.4, 'Vakthros'),
(42, 2444, 13646, 'The Azure Span', -3656.8, -3145.3, 112.9, 'Zelthrak Outpost');

-- Thaldraszus Glyphs (14 total)
INSERT INTO `playerbot_dragonriding_glyph_templates` (`glyph_id`, `map_id`, `zone_id`, `zone_name`, `pos_x`, `pos_y`, `pos_z`, `name`) VALUES
(43, 2444, 13647, 'Thaldraszus', -4456.7, -4345.2, 534.5, 'Algeth''ar Academy'),
(44, 2444, 13647, 'Thaldraszus', -4298.3, -4478.9, 587.2, 'Gelikyr Post'),
(45, 2444, 13647, 'Thaldraszus', -4623.5, -4123.7, 498.4, 'South Hold Gate'),
(46, 2444, 13647, 'Thaldraszus', -4767.2, -3987.4, 612.8, 'Stormshroud Peak'),
(47, 2444, 13647, 'Thaldraszus', -4434.8, -4567.3, 556.7, 'Temporal Conflux'),
(48, 2444, 13647, 'Thaldraszus', -4589.1, -4298.5, 578.9, 'Thaldraszus Apex'),
(49, 2444, 13647, 'Thaldraszus', -4345.6, -4389.2, 534.1, 'The Cascades'),
(50, 2444, 13647, 'Thaldraszus', -4498.4, -4456.8, 589.3, 'Tyrhold'),
(51, 2444, 13647, 'Thaldraszus', -4678.9, -4234.6, 612.5, 'Valdrakken'),
(52, 2444, 13647, 'Thaldraszus', -4767.3, -4098.7, 567.8, 'Vault of the Incarnates'),
(53, 2444, 13647, 'Thaldraszus', -4323.5, -4567.9, 534.2, 'Veiled Ossuary'),
(54, 2444, 13647, 'Thaldraszus', -4589.7, -4389.4, 598.6, 'Passage of Time'),
(55, 2444, 13647, 'Thaldraszus', -4434.1, -4234.8, 556.4, 'Tyrhold Reservoir'),
(56, 2444, 13647, 'Thaldraszus', -4656.8, -4145.3, 612.9, 'Valdrakken Apex');

-- The Forbidden Reach Glyphs (8 total - Patch 10.0.7)
INSERT INTO `playerbot_dragonriding_glyph_templates` (`glyph_id`, `map_id`, `zone_id`, `zone_name`, `pos_x`, `pos_y`, `pos_z`, `name`) VALUES
(57, 2444, 14022, 'The Forbidden Reach', -5456.7, -5345.2, 234.5, 'Caldera of the Menders'),
(58, 2444, 14022, 'The Forbidden Reach', -5298.3, -5478.9, 287.2, 'Dragonskull Island'),
(59, 2444, 14022, 'The Forbidden Reach', -5623.5, -5123.7, 198.4, 'Froststone Peak'),
(60, 2444, 14022, 'The Forbidden Reach', -5767.2, -4987.4, 312.8, 'Morqut Islet'),
(61, 2444, 14022, 'The Forbidden Reach', -5434.8, -5567.3, 256.7, 'Sharpscale Coast'),
(62, 2444, 14022, 'The Forbidden Reach', -5589.1, -5298.5, 278.9, 'Stormsunder Crater'),
(63, 2444, 14022, 'The Forbidden Reach', -5345.6, -5389.2, 234.1, 'Talon''s Watch'),
(64, 2444, 14022, 'The Forbidden Reach', -5498.4, -5456.8, 289.3, 'War Creche');

-- Zaralek Cavern Glyphs (10 total - Patch 10.1)
INSERT INTO `playerbot_dragonriding_glyph_templates` (`glyph_id`, `map_id`, `zone_id`, `zone_name`, `pos_x`, `pos_y`, `pos_z`, `name`) VALUES
(65, 2444, 14529, 'Zaralek Cavern', -6456.7, -6345.2, -534.5, 'Aberrus Approach'),
(66, 2444, 14529, 'Zaralek Cavern', -6298.3, -6478.9, -487.2, 'Acidbite Ravine'),
(67, 2444, 14529, 'Zaralek Cavern', -6623.5, -6123.7, -598.4, 'Glimmerogg'),
(68, 2444, 14529, 'Zaralek Cavern', -6767.2, -5987.4, -412.8, 'Loamm'),
(69, 2444, 14529, 'Zaralek Cavern', -6434.8, -6567.3, -556.7, 'Obsidian Rest'),
(70, 2444, 14529, 'Zaralek Cavern', -6589.1, -6298.5, -478.9, 'Slitherdrake Roost'),
(71, 2444, 14529, 'Zaralek Cavern', -6345.6, -6389.2, -534.1, 'The Throughway'),
(72, 2444, 14529, 'Zaralek Cavern', -6498.4, -6456.8, -489.3, 'Nal ks''kol'),
(73, 2444, 14529, 'Zaralek Cavern', -6678.9, -6234.6, -412.5, 'Zaqali Caldera'),
(74, 2444, 14529, 'Zaralek Cavern', -6767.3, -6098.7, -467.8, 'Crystal Pools');

-- ============================================================================
-- MIGRATION TRACKING
-- ============================================================================

-- Record this migration
INSERT INTO `playerbot_migrations` (`version`, `description`, `checksum`)
VALUES ('1.0.0-dragonriding', 'Add dragonriding progression tables (glyphs, talents)', MD5('dragonriding_tables.sql'))
ON DUPLICATE KEY UPDATE `applied_at` = CURRENT_TIMESTAMP;

-- ============================================================================
-- SCHEMA VALIDATION
-- ============================================================================
SELECT 'Dragonriding tables created successfully' as 'Status',
    (SELECT COUNT(*) FROM `playerbot_dragonriding_talent_templates`) as 'Talents defined',
    (SELECT COUNT(*) FROM `playerbot_dragonriding_glyph_templates`) as 'Glyphs defined',
    NOW() as 'Created at';

