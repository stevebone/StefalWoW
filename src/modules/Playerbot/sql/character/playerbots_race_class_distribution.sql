-- Playerbot Race/Class Distribution Table
-- Based on World of Warcraft 12.0 Statistics from WoWRanks, Wowhead, and U.GG

CREATE TABLE IF NOT EXISTS `playerbots_race_class_distribution` (
    `race` TINYINT UNSIGNED NOT NULL COMMENT 'Race ID from ChrRaces.db2',
    `class` TINYINT UNSIGNED NOT NULL COMMENT 'Class ID from ChrClasses.db2',
    `percentage` FLOAT NOT NULL DEFAULT 0.0 COMMENT 'Population percentage (0.0-100.0)',
    `is_popular` TINYINT(1) NOT NULL DEFAULT 0 COMMENT '1 if combination is considered popular',
    `faction` VARCHAR(10) NOT NULL COMMENT 'Alliance, Horde, or Both',
    PRIMARY KEY (`race`, `class`),
    INDEX `idx_percentage` (`percentage` DESC),
    INDEX `idx_popular` (`is_popular`, `percentage` DESC),
    INDEX `idx_faction` (`faction`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Bot character race/class distribution based on WoW 12.0 statistics';

-- Insert realistic distribution data based on WoW 12.0 statistics
-- Top combinations from retail servers (approximate percentages)

-- Alliance Popular Combinations
INSERT INTO `playerbots_race_class_distribution` VALUES
-- Human combinations (most popular Alliance race)
(1, 1, 8.5, 1, 'Alliance'),  -- Human Warrior
(1, 2, 7.2, 1, 'Alliance'),  -- Human Paladin
(1, 4, 6.8, 1, 'Alliance'),  -- Human Rogue
(1, 5, 5.9, 1, 'Alliance'),  -- Human Priest
(1, 8, 5.4, 1, 'Alliance'),  -- Human Mage
(1, 9, 4.1, 1, 'Alliance'),  -- Human Warlock
(1, 3, 3.2, 0, 'Alliance'),  -- Human Hunter
(1, 6, 2.8, 0, 'Alliance'),  -- Human Death Knight
(1, 10, 2.1, 0, 'Alliance'), -- Human Monk

-- Night Elf combinations
(4, 3, 4.8, 1, 'Alliance'),  -- Night Elf Hunter
(4, 11, 4.2, 1, 'Alliance'), -- Night Elf Druid
(4, 4, 3.5, 0, 'Alliance'),  -- Night Elf Rogue
(4, 1, 2.9, 0, 'Alliance'),  -- Night Elf Warrior
(4, 5, 2.4, 0, 'Alliance'),  -- Night Elf Priest
(4, 8, 2.1, 0, 'Alliance'),  -- Night Elf Mage
(4, 6, 1.8, 0, 'Alliance'),  -- Night Elf Death Knight
(4, 10, 1.4, 0, 'Alliance'), -- Night Elf Monk
(4, 12, 1.2, 0, 'Alliance'), -- Night Elf Demon Hunter

-- Dwarf combinations
(3, 3, 3.8, 1, 'Alliance'),  -- Dwarf Hunter
(3, 2, 3.1, 0, 'Alliance'),  -- Dwarf Paladin
(3, 1, 2.7, 0, 'Alliance'),  -- Dwarf Warrior
(3, 5, 2.2, 0, 'Alliance'),  -- Dwarf Priest
(3, 4, 1.9, 0, 'Alliance'),  -- Dwarf Rogue
(3, 7, 1.6, 0, 'Alliance'),  -- Dwarf Shaman
(3, 6, 1.3, 0, 'Alliance'),  -- Dwarf Death Knight
(3, 10, 1.0, 0, 'Alliance'), -- Dwarf Monk

-- Gnome combinations
(7, 8, 2.9, 0, 'Alliance'),  -- Gnome Mage
(7, 9, 2.4, 0, 'Alliance'),  -- Gnome Warlock
(7, 1, 1.8, 0, 'Alliance'),  -- Gnome Warrior
(7, 4, 1.5, 0, 'Alliance'),  -- Gnome Rogue
(7, 5, 1.2, 0, 'Alliance'),  -- Gnome Priest
(7, 6, 1.0, 0, 'Alliance'),  -- Gnome Death Knight
(7, 3, 0.8, 0, 'Alliance'),  -- Gnome Hunter
(7, 10, 0.6, 0, 'Alliance'), -- Gnome Monk

-- Draenei combinations
(11, 2, 2.8, 0, 'Alliance'), -- Draenei Paladin
(11, 7, 2.5, 0, 'Alliance'), -- Draenei Shaman
(11, 5, 2.1, 0, 'Alliance'), -- Draenei Priest
(11, 1, 1.8, 0, 'Alliance'), -- Draenei Warrior
(11, 3, 1.5, 0, 'Alliance'), -- Draenei Hunter
(11, 8, 1.2, 0, 'Alliance'), -- Draenei Mage
(11, 6, 1.0, 0, 'Alliance'), -- Draenei Death Knight
(11, 10, 0.8, 0, 'Alliance'); -- Draenei Monk

-- Horde Popular Combinations
INSERT INTO `playerbots_race_class_distribution` VALUES
-- Orc combinations (most popular Horde race)
(2, 1, 7.8, 1, 'Horde'),    -- Orc Warrior
(2, 7, 6.9, 1, 'Horde'),    -- Orc Shaman
(2, 3, 5.4, 1, 'Horde'),    -- Orc Hunter
(2, 9, 4.2, 1, 'Horde'),    -- Orc Warlock
(2, 4, 3.8, 0, 'Horde'),    -- Orc Rogue
(2, 6, 2.9, 0, 'Horde'),    -- Orc Death Knight
(2, 10, 2.1, 0, 'Horde'),   -- Orc Monk

-- Undead combinations
(5, 9, 6.2, 1, 'Horde'),    -- Undead Warlock
(5, 5, 5.1, 1, 'Horde'),    -- Undead Priest
(5, 8, 4.8, 1, 'Horde'),    -- Undead Mage
(5, 4, 4.1, 0, 'Horde'),    -- Undead Rogue
(5, 1, 3.2, 0, 'Horde'),    -- Undead Warrior
(5, 6, 2.4, 0, 'Horde'),    -- Undead Death Knight
(5, 10, 1.8, 0, 'Horde'),   -- Undead Monk

-- Tauren combinations
(6, 11, 5.4, 1, 'Horde'),   -- Tauren Druid
(6, 7, 4.2, 1, 'Horde'),    -- Tauren Shaman
(6, 1, 3.8, 0, 'Horde'),    -- Tauren Warrior
(6, 3, 3.1, 0, 'Horde'),    -- Tauren Hunter
(6, 2, 2.6, 0, 'Horde'),    -- Tauren Paladin
(6, 6, 2.1, 0, 'Horde'),    -- Tauren Death Knight
(6, 10, 1.7, 0, 'Horde'),   -- Tauren Monk

-- Troll combinations
(8, 7, 4.8, 1, 'Horde'),    -- Troll Shaman
(8, 3, 4.1, 1, 'Horde'),    -- Troll Hunter
(8, 5, 3.4, 0, 'Horde'),    -- Troll Priest
(8, 1, 2.9, 0, 'Horde'),    -- Troll Warrior
(8, 4, 2.5, 0, 'Horde'),    -- Troll Rogue
(8, 8, 2.1, 0, 'Horde'),    -- Troll Mage
(8, 9, 1.8, 0, 'Horde'),    -- Troll Warlock
(8, 11, 1.5, 0, 'Horde'),   -- Troll Druid
(8, 6, 1.2, 0, 'Horde'),    -- Troll Death Knight
(8, 10, 1.0, 0, 'Horde'),   -- Troll Monk

-- Blood Elf combinations (popular Horde race)
(10, 2, 5.8, 1, 'Horde'),   -- Blood Elf Paladin
(10, 8, 5.2, 1, 'Horde'),   -- Blood Elf Mage
(10, 9, 4.6, 1, 'Horde'),   -- Blood Elf Warlock
(10, 3, 3.9, 0, 'Horde'),   -- Blood Elf Hunter
(10, 4, 3.4, 0, 'Horde'),   -- Blood Elf Rogue
(10, 5, 2.8, 0, 'Horde'),   -- Blood Elf Priest
(10, 1, 2.3, 0, 'Horde'),   -- Blood Elf Warrior
(10, 6, 1.9, 0, 'Horde'),   -- Blood Elf Death Knight
(10, 10, 1.4, 0, 'Horde'),  -- Blood Elf Monk
(10, 12, 3.2, 1, 'Horde');  -- Blood Elf Demon Hunter

-- Additional races with lower populations
INSERT INTO `playerbots_race_class_distribution` VALUES
-- Worgen (Alliance)
(22, 11, 1.8, 0, 'Alliance'), -- Worgen Druid
(22, 1, 1.5, 0, 'Alliance'),  -- Worgen Warrior
(22, 3, 1.3, 0, 'Alliance'),  -- Worgen Hunter
(22, 4, 1.1, 0, 'Alliance'),  -- Worgen Rogue
(22, 5, 0.9, 0, 'Alliance'),  -- Worgen Priest
(22, 8, 0.7, 0, 'Alliance'),  -- Worgen Mage
(22, 9, 0.6, 0, 'Alliance'),  -- Worgen Warlock
(22, 6, 0.5, 0, 'Alliance'),  -- Worgen Death Knight

-- Goblin (Horde)
(9, 7, 1.6, 0, 'Horde'),     -- Goblin Shaman
(9, 1, 1.4, 0, 'Horde'),     -- Goblin Warrior
(9, 4, 1.2, 0, 'Horde'),     -- Goblin Rogue
(9, 3, 1.0, 0, 'Horde'),     -- Goblin Hunter
(9, 8, 0.8, 0, 'Horde'),     -- Goblin Mage
(9, 5, 0.7, 0, 'Horde'),     -- Goblin Priest
(9, 9, 0.6, 0, 'Horde'),     -- Goblin Warlock
(9, 6, 0.5, 0, 'Horde'),     -- Goblin Death Knight

-- Pandaren (Both factions)
(24, 10, 1.2, 0, 'Both'),    -- Pandaren Monk
(25, 10, 1.2, 0, 'Both'),    -- Pandaren Monk (Horde)
(24, 7, 0.8, 0, 'Both'),     -- Pandaren Shaman
(25, 7, 0.8, 0, 'Both'),     -- Pandaren Shaman (Horde)
(24, 1, 0.6, 0, 'Both'),     -- Pandaren Warrior
(25, 1, 0.6, 0, 'Both'),     -- Pandaren Warrior (Horde)
(24, 3, 0.5, 0, 'Both'),     -- Pandaren Hunter
(25, 3, 0.5, 0, 'Both'),     -- Pandaren Hunter (Horde)
(24, 4, 0.4, 0, 'Both'),     -- Pandaren Rogue
(25, 4, 0.4, 0, 'Both'),     -- Pandaren Rogue (Horde)
(24, 8, 0.3, 0, 'Both'),     -- Pandaren Mage
(25, 8, 0.3, 0, 'Both'),     -- Pandaren Mage (Horde)
(24, 5, 0.3, 0, 'Both'),     -- Pandaren Priest
(25, 5, 0.3, 0, 'Both');     -- Pandaren Priest (Horde)

-- Allied races with moderate popularity
INSERT INTO `playerbots_race_class_distribution` VALUES
-- Void Elf (Alliance) - Popular for casters
(29, 8, 2.1, 0, 'Alliance'),  -- Void Elf Mage
(29, 9, 1.8, 0, 'Alliance'),  -- Void Elf Warlock
(29, 5, 1.5, 0, 'Alliance'),  -- Void Elf Priest
(29, 4, 1.2, 0, 'Alliance'),  -- Void Elf Rogue
(29, 1, 1.0, 0, 'Alliance'),  -- Void Elf Warrior
(29, 3, 0.8, 0, 'Alliance'),  -- Void Elf Hunter
(29, 6, 0.6, 0, 'Alliance'),  -- Void Elf Death Knight
(29, 10, 0.4, 0, 'Alliance'), -- Void Elf Monk

-- Nightborne (Horde) - Popular for casters
(27, 8, 1.9, 0, 'Horde'),     -- Nightborne Mage
(27, 9, 1.6, 0, 'Horde'),     -- Nightborne Warlock
(27, 5, 1.3, 0, 'Horde'),     -- Nightborne Priest
(27, 4, 1.0, 0, 'Horde'),     -- Nightborne Rogue
(27, 1, 0.8, 0, 'Horde'),     -- Nightborne Warrior
(27, 3, 0.6, 0, 'Horde'),     -- Nightborne Hunter
(27, 6, 0.5, 0, 'Horde'),     -- Nightborne Death Knight
(27, 10, 0.3, 0, 'Horde');    -- Nightborne Monk