-- Playerbot Gender Distribution Table
-- Based on World of Warcraft player preferences and fantasy race stereotypes

CREATE TABLE IF NOT EXISTS `playerbots_gender_distribution` (
    `race` TINYINT UNSIGNED NOT NULL PRIMARY KEY COMMENT 'Race ID from ChrRaces.db2',
    `race_name` VARCHAR(50) NOT NULL COMMENT 'Race name for reference',
    `male_percentage` TINYINT UNSIGNED NOT NULL DEFAULT 50 COMMENT 'Male character percentage (0-100)',
    `female_percentage` TINYINT UNSIGNED NOT NULL DEFAULT 50 COMMENT 'Female character percentage (0-100)',
    CHECK (`male_percentage` + `female_percentage` = 100),
    INDEX `idx_race_name` (`race_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Bot character gender distribution by race';

-- Insert gender distribution data based on player preferences and fantasy stereotypes
-- Data approximated from retail server statistics and player behavior patterns

INSERT INTO `playerbots_gender_distribution` VALUES
-- Alliance Races
(1, 'Human', 58, 42),           -- Humans: Slight male preference (versatile race)
(3, 'Dwarf', 72, 28),           -- Dwarves: Strong male preference (warrior culture)
(4, 'Night Elf', 35, 65),       -- Night Elves: Female preference (elegant, mystical)
(7, 'Gnome', 55, 45),           -- Gnomes: Slight male preference (technology focus)
(11, 'Draenei', 42, 58),        -- Draenei: Female preference (graceful, holy)
(22, 'Worgen', 65, 35),         -- Worgen: Male preference (beast/feral theme)
(29, 'Void Elf', 38, 62),       -- Void Elves: Female preference (elegant void magic)
(32, 'Kul Tiran', 62, 38),      -- Kul Tirans: Male preference (naval/maritime)
(37, 'Dark Iron Dwarf', 70, 30), -- Dark Iron: Strong male preference (industrial)
(34, 'Lightforged Draenei', 40, 60), -- Lightforged: Female preference (holy warriors)
(30, 'Mechagnome', 68, 32),     -- Mechagnomes: Male preference (technology)

-- Horde Races
(2, 'Orc', 75, 25),             -- Orcs: Strong male preference (warrior culture)
(5, 'Undead', 48, 52),          -- Undead: Balanced, slight female preference
(6, 'Tauren', 68, 32),          -- Tauren: Male preference (tribal/shamanic)
(8, 'Troll', 60, 40),           -- Trolls: Male preference (tribal warriors)
(9, 'Goblin', 55, 45),          -- Goblins: Slight male preference (business/tech)
(10, 'Blood Elf', 25, 75),      -- Blood Elves: Strong female preference (beauty/magic)
(27, 'Nightborne', 30, 70),     -- Nightborne: Strong female preference (elegant elves)
(28, 'Highmountain Tauren', 70, 30), -- Highmountain: Male preference (mountain warriors)
(31, 'Zandalari Troll', 58, 42), -- Zandalari: Male preference (empire builders)
(35, 'Vulpera', 45, 55),        -- Vulpera: Female preference (cute factor)
(36, 'Mag\'har Orc', 78, 22),   -- Mag'har: Strong male preference (pure orcs)

-- Neutral Races
(24, 'Pandaren (Alliance)', 52, 48), -- Pandaren: Balanced, slight male preference
(25, 'Pandaren (Horde)', 52, 48),   -- Pandaren: Balanced, slight male preference
(26, 'Pandaren (Neutral)', 52, 48), -- Pandaren: Balanced, slight male preference

-- Hero/Special Classes (more recent additions with modern preferences)
(70, 'Dracthyr (Alliance)', 45, 55), -- Dracthyr: Slight female preference (dragon magic)
(52, 'Dracthyr (Horde)', 45, 55),    -- Dracthyr: Slight female preference (dragon magic)

-- Death Knight specific races (if separate tracking needed)
(1, 'Human Death Knight', 62, 38),    -- Death Knights tend more male
(2, 'Orc Death Knight', 78, 22),
(3, 'Dwarf Death Knight', 75, 25),
(4, 'Night Elf Death Knight', 45, 55),
(5, 'Undead Death Knight', 55, 45),
(6, 'Tauren Death Knight', 72, 28),
(7, 'Gnome Death Knight', 60, 40),
(8, 'Troll Death Knight', 65, 35),
(10, 'Blood Elf Death Knight', 35, 65),
(11, 'Draenei Death Knight', 50, 50),

-- Demon Hunter specific races
(4, 'Night Elf Demon Hunter', 48, 52),  -- Demon Hunters: More balanced
(10, 'Blood Elf Demon Hunter', 32, 68); -- Blood Elf DH: Still female preference