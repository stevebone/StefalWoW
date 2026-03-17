-- Playerbot Race/Class Gender Override Table
-- Specific gender preferences for certain race/class combinations

CREATE TABLE IF NOT EXISTS `playerbots_race_class_gender` (
    `race` TINYINT UNSIGNED NOT NULL COMMENT 'Race ID from ChrRaces.db2',
    `class` TINYINT UNSIGNED NOT NULL COMMENT 'Class ID from ChrClasses.db2',
    `male_percentage` TINYINT UNSIGNED NOT NULL COMMENT 'Male percentage for this specific combination',
    PRIMARY KEY (`race`, `class`),
    CHECK (`male_percentage` <= 100),
    INDEX `idx_male_percentage` (`male_percentage`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Race/class specific gender overrides for bot characters';

-- Insert race/class specific gender preferences that override general race preferences
-- These represent combinations where the class fantasy significantly affects gender choice

INSERT INTO `playerbots_race_class_gender` VALUES
-- Warrior class tends to be more male across most races
(4, 1, 60),    -- Night Elf Warrior (override general 35% male to 60% for warriors)
(11, 1, 65),   -- Draenei Warrior (override general 42% male to 65% for warriors)
(10, 1, 45),   -- Blood Elf Warrior (override general 25% male to 45% for warriors)

-- Paladin class - holy warriors tend to be more balanced
(4, 2, 55),    -- Night Elf Paladin (more male for holy warrior theme)
(10, 2, 35),   -- Blood Elf Paladin (less extreme female preference)
(11, 2, 50),   -- Draenei Paladin (balanced for holy warriors)

-- Hunter class - some races prefer female hunters
(4, 3, 25),    -- Night Elf Hunter (strong female preference, ranger theme)
(10, 3, 30),   -- Blood Elf Hunter (female preference for elegant archer)

-- Rogue class - stealth and agility themes
(4, 4, 30),    -- Night Elf Rogue (female shadowmeld rogues popular)
(10, 4, 35),   -- Blood Elf Rogue (elegant assassin theme)
(1, 4, 65),    -- Human Rogue (male preference for human thieves)

-- Priest class - healing and shadow themes
(5, 5, 40),    -- Undead Priest (shadow priest theme, more balanced)
(10, 5, 20),   -- Blood Elf Priest (strong female preference for holy magic)
(11, 5, 35),   -- Draenei Priest (holy female priests popular)

-- Death Knight class - tends to be more male regardless of race
(4, 6, 55),    -- Night Elf Death Knight (death theme overrides elegance)
(10, 6, 45),   -- Blood Elf Death Knight (death knight masculinity)
(11, 6, 60),   -- Draenei Death Knight (fallen paladin theme)

-- Shaman class - elemental themes
(6, 7, 75),    -- Tauren Shaman (spiritual leader, male preference)
(8, 7, 65),    -- Troll Shaman (witch doctor theme, male preference)
(11, 7, 55),   -- Draenei Shaman (elemental warrior theme)

-- Mage class - intellectual spellcasters
(1, 8, 50),    -- Human Mage (balanced for scholarly theme)
(7, 8, 45),    -- Gnome Mage (intelligence over gender stereotypes)
(10, 8, 20),   -- Blood Elf Mage (arcane elegance, female preference)

-- Warlock class - dark magic themes
(1, 9, 65),    -- Human Warlock (dark male warlock stereotype)
(5, 9, 55),    -- Undead Warlock (slightly more male for dark magic)
(10, 9, 30),   -- Blood Elf Warlock (sophisticated dark magic)

-- Monk class - martial arts themes
(24, 10, 60),  -- Pandaren Monk (martial arts master stereotype)
(25, 10, 60),  -- Pandaren Monk (Horde)
(1, 10, 55),   -- Human Monk (balanced martial artist)

-- Druid class - nature themes vary by race
(4, 11, 25),   -- Night Elf Druid (nature goddess theme, female preference)
(6, 11, 70),   -- Tauren Druid (spiritual leader theme, male preference)
(8, 11, 50),   -- Troll Druid (balanced nature connection)

-- Demon Hunter class - vengeance themes
(4, 12, 55),   -- Night Elf Demon Hunter (vengeance warrior, more male)
(10, 12, 40),  -- Blood Elf Demon Hunter (sacrificial theme, less female)

-- Evoker class - draconic magic themes
(70, 13, 50),  -- Dracthyr Evoker Alliance (balanced dragon magic)
(52, 13, 50);  -- Dracthyr Evoker Horde (balanced dragon magic)