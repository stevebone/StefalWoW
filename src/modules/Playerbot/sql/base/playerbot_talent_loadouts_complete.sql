-- ====================================================================
-- Playerbot Talent Loadouts Table - COMPLETE DATA FOR ALL CLASSES
-- ====================================================================
-- Purpose: Store predefined talent loadouts for automated bot creation
-- Integration: Used by BotTalentManager for instant level-up scenarios
-- ====================================================================
-- IMPORTANT: This file contains structural framework for all 39 specs
-- Talent strings are EMPTY and need to be populated with actual spell IDs
-- See schema comments for details on talent string format
-- ====================================================================

-- Clean slate
TRUNCATE TABLE `playerbot_talent_loadouts`;

-- ====================================================================
-- WARRIOR (Class ID: 1)
-- ====================================================================

-- Arms (Spec ID: 71)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(1, 71, 1, 10, '', NULL, 'Arms Warrior 1-10: Early leveling (auto-learned spells)'),
(1, 71, 11, 20, '', NULL, 'Arms Warrior 11-20: Basic talents'),
(1, 71, 21, 30, '', NULL, 'Arms Warrior 21-30: Intermediate talents'),
(1, 71, 31, 40, '', NULL, 'Arms Warrior 31-40: Advanced talents'),
(1, 71, 41, 50, '', NULL, 'Arms Warrior 41-50: Expert talents'),
(1, 71, 51, 60, '', NULL, 'Arms Warrior 51-60: Master talents'),
(1, 71, 61, 70, '', NULL, 'Arms Warrior 61-70: Pre-War Within'),
(1, 71, 71, 80, '', '', 'Arms Warrior 71-80: War Within + Hero Talents (Colossus/Slayer)');

-- Fury (Spec ID: 72)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(1, 72, 1, 10, '', NULL, 'Fury Warrior 1-10: Early leveling (auto-learned spells)'),
(1, 72, 11, 20, '', NULL, 'Fury Warrior 11-20: Basic talents'),
(1, 72, 21, 30, '', NULL, 'Fury Warrior 21-30: Intermediate talents'),
(1, 72, 31, 40, '', NULL, 'Fury Warrior 31-40: Advanced talents'),
(1, 72, 41, 50, '', NULL, 'Fury Warrior 41-50: Expert talents'),
(1, 72, 51, 60, '', NULL, 'Fury Warrior 51-60: Master talents'),
(1, 72, 61, 70, '', NULL, 'Fury Warrior 61-70: Pre-War Within'),
(1, 72, 71, 80, '', '', 'Fury Warrior 71-80: War Within + Hero Talents (Mountain Thane/Slayer)');

-- Protection (Spec ID: 73)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(1, 73, 1, 10, '', NULL, 'Protection Warrior 1-10: Early leveling (auto-learned spells)'),
(1, 73, 11, 20, '', NULL, 'Protection Warrior 11-20: Basic talents'),
(1, 73, 21, 30, '', NULL, 'Protection Warrior 21-30: Intermediate talents'),
(1, 73, 31, 40, '', NULL, 'Protection Warrior 31-40: Advanced talents'),
(1, 73, 41, 50, '', NULL, 'Protection Warrior 41-50: Expert talents'),
(1, 73, 51, 60, '', NULL, 'Protection Warrior 51-60: Master talents'),
(1, 73, 61, 70, '', NULL, 'Protection Warrior 61-70: Pre-War Within'),
(1, 73, 71, 80, '', '', 'Protection Warrior 71-80: War Within + Hero Talents (Colossus/Mountain Thane)');

-- ====================================================================
-- PALADIN (Class ID: 2)
-- ====================================================================

-- Holy (Spec ID: 65)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(2, 65, 1, 10, '', NULL, 'Holy Paladin 1-10: Early leveling (auto-learned spells)'),
(2, 65, 11, 20, '', NULL, 'Holy Paladin 11-20: Basic talents'),
(2, 65, 21, 30, '', NULL, 'Holy Paladin 21-30: Intermediate talents'),
(2, 65, 31, 40, '', NULL, 'Holy Paladin 31-40: Advanced talents'),
(2, 65, 41, 50, '', NULL, 'Holy Paladin 41-50: Expert talents'),
(2, 65, 51, 60, '', NULL, 'Holy Paladin 51-60: Master talents'),
(2, 65, 61, 70, '', NULL, 'Holy Paladin 61-70: Pre-War Within'),
(2, 65, 71, 80, '', '', 'Holy Paladin 71-80: War Within + Hero Talents (Lightsmith/Herald of the Sun)');

-- Protection (Spec ID: 66)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(2, 66, 1, 10, '', NULL, 'Protection Paladin 1-10: Early leveling (auto-learned spells)'),
(2, 66, 11, 20, '', NULL, 'Protection Paladin 11-20: Basic talents'),
(2, 66, 21, 30, '', NULL, 'Protection Paladin 21-30: Intermediate talents'),
(2, 66, 31, 40, '', NULL, 'Protection Paladin 31-40: Advanced talents'),
(2, 66, 41, 50, '', NULL, 'Protection Paladin 41-50: Expert talents'),
(2, 66, 51, 60, '', NULL, 'Protection Paladin 51-60: Master talents'),
(2, 66, 61, 70, '', NULL, 'Protection Paladin 61-70: Pre-War Within'),
(2, 66, 71, 80, '', '', 'Protection Paladin 71-80: War Within + Hero Talents (Lightsmith/Templar)');

-- Retribution (Spec ID: 70)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(2, 70, 1, 10, '', NULL, 'Retribution Paladin 1-10: Early leveling (auto-learned spells)'),
(2, 70, 11, 20, '', NULL, 'Retribution Paladin 11-20: Basic talents'),
(2, 70, 21, 30, '', NULL, 'Retribution Paladin 21-30: Intermediate talents'),
(2, 70, 31, 40, '', NULL, 'Retribution Paladin 31-40: Advanced talents'),
(2, 70, 41, 50, '', NULL, 'Retribution Paladin 41-50: Expert talents'),
(2, 70, 51, 60, '', NULL, 'Retribution Paladin 51-60: Master talents'),
(2, 70, 61, 70, '', NULL, 'Retribution Paladin 61-70: Pre-War Within'),
(2, 70, 71, 80, '', '', 'Retribution Paladin 71-80: War Within + Hero Talents (Herald of the Sun/Templar)');

-- ====================================================================
-- HUNTER (Class ID: 3)
-- ====================================================================

-- Beast Mastery (Spec ID: 253)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(3, 253, 1, 10, '', NULL, 'Beast Mastery Hunter 1-10: Early leveling (auto-learned spells)'),
(3, 253, 11, 20, '', NULL, 'Beast Mastery Hunter 11-20: Basic talents'),
(3, 253, 21, 30, '', NULL, 'Beast Mastery Hunter 21-30: Intermediate talents'),
(3, 253, 31, 40, '', NULL, 'Beast Mastery Hunter 31-40: Advanced talents'),
(3, 253, 41, 50, '', NULL, 'Beast Mastery Hunter 41-50: Expert talents'),
(3, 253, 51, 60, '', NULL, 'Beast Mastery Hunter 51-60: Master talents'),
(3, 253, 61, 70, '', NULL, 'Beast Mastery Hunter 61-70: Pre-War Within'),
(3, 253, 71, 80, '', '', 'Beast Mastery Hunter 71-80: War Within + Hero Talents (Dark Ranger/Pack Leader)');

-- Marksmanship (Spec ID: 254)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(3, 254, 1, 10, '', NULL, 'Marksmanship Hunter 1-10: Early leveling (auto-learned spells)'),
(3, 254, 11, 20, '', NULL, 'Marksmanship Hunter 11-20: Basic talents'),
(3, 254, 21, 30, '', NULL, 'Marksmanship Hunter 21-30: Intermediate talents'),
(3, 254, 31, 40, '', NULL, 'Marksmanship Hunter 31-40: Advanced talents'),
(3, 254, 41, 50, '', NULL, 'Marksmanship Hunter 41-50: Expert talents'),
(3, 254, 51, 60, '', NULL, 'Marksmanship Hunter 51-60: Master talents'),
(3, 254, 61, 70, '', NULL, 'Marksmanship Hunter 61-70: Pre-War Within'),
(3, 254, 71, 80, '', '', 'Marksmanship Hunter 71-80: War Within + Hero Talents (Dark Ranger/Sentinel)');

-- Survival (Spec ID: 255)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(3, 255, 1, 10, '', NULL, 'Survival Hunter 1-10: Early leveling (auto-learned spells)'),
(3, 255, 11, 20, '', NULL, 'Survival Hunter 11-20: Basic talents'),
(3, 255, 21, 30, '', NULL, 'Survival Hunter 21-30: Intermediate talents'),
(3, 255, 31, 40, '', NULL, 'Survival Hunter 31-40: Advanced talents'),
(3, 255, 41, 50, '', NULL, 'Survival Hunter 41-50: Expert talents'),
(3, 255, 51, 60, '', NULL, 'Survival Hunter 51-60: Master talents'),
(3, 255, 61, 70, '', NULL, 'Survival Hunter 61-70: Pre-War Within'),
(3, 255, 71, 80, '', '', 'Survival Hunter 71-80: War Within + Hero Talents (Pack Leader/Sentinel)');

-- ====================================================================
-- ROGUE (Class ID: 4)
-- ====================================================================

-- Assassination (Spec ID: 259)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(4, 259, 1, 10, '', NULL, 'Assassination Rogue 1-10: Early leveling (auto-learned spells)'),
(4, 259, 11, 20, '', NULL, 'Assassination Rogue 11-20: Basic talents'),
(4, 259, 21, 30, '', NULL, 'Assassination Rogue 21-30: Intermediate talents'),
(4, 259, 31, 40, '', NULL, 'Assassination Rogue 31-40: Advanced talents'),
(4, 259, 41, 50, '', NULL, 'Assassination Rogue 41-50: Expert talents'),
(4, 259, 51, 60, '', NULL, 'Assassination Rogue 51-60: Master talents'),
(4, 259, 61, 70, '', NULL, 'Assassination Rogue 61-70: Pre-War Within'),
(4, 259, 71, 80, '', '', 'Assassination Rogue 71-80: War Within + Hero Talents (Deathstalker/Fatebound)');

-- Outlaw (Spec ID: 260)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(4, 260, 1, 10, '', NULL, 'Outlaw Rogue 1-10: Early leveling (auto-learned spells)'),
(4, 260, 11, 20, '', NULL, 'Outlaw Rogue 11-20: Basic talents'),
(4, 260, 21, 30, '', NULL, 'Outlaw Rogue 21-30: Intermediate talents'),
(4, 260, 31, 40, '', NULL, 'Outlaw Rogue 31-40: Advanced talents'),
(4, 260, 41, 50, '', NULL, 'Outlaw Rogue 41-50: Expert talents'),
(4, 260, 51, 60, '', NULL, 'Outlaw Rogue 51-60: Master talents'),
(4, 260, 61, 70, '', NULL, 'Outlaw Rogue 61-70: Pre-War Within'),
(4, 260, 71, 80, '', '', 'Outlaw Rogue 71-80: War Within + Hero Talents (Fatebound/Trickster)');

-- Subtlety (Spec ID: 261)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(4, 261, 1, 10, '', NULL, 'Subtlety Rogue 1-10: Early leveling (auto-learned spells)'),
(4, 261, 11, 20, '', NULL, 'Subtlety Rogue 11-20: Basic talents'),
(4, 261, 21, 30, '', NULL, 'Subtlety Rogue 21-30: Intermediate talents'),
(4, 261, 31, 40, '', NULL, 'Subtlety Rogue 31-40: Advanced talents'),
(4, 261, 41, 50, '', NULL, 'Subtlety Rogue 41-50: Expert talents'),
(4, 261, 51, 60, '', NULL, 'Subtlety Rogue 51-60: Master talents'),
(4, 261, 61, 70, '', NULL, 'Subtlety Rogue 61-70: Pre-War Within'),
(4, 261, 71, 80, '', '', 'Subtlety Rogue 71-80: War Within + Hero Talents (Deathstalker/Trickster)');

-- ====================================================================
-- PRIEST (Class ID: 5)
-- ====================================================================

-- Discipline (Spec ID: 256)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(5, 256, 1, 10, '', NULL, 'Discipline Priest 1-10: Early leveling (auto-learned spells)'),
(5, 256, 11, 20, '', NULL, 'Discipline Priest 11-20: Basic talents'),
(5, 256, 21, 30, '', NULL, 'Discipline Priest 21-30: Intermediate talents'),
(5, 256, 31, 40, '', NULL, 'Discipline Priest 31-40: Advanced talents'),
(5, 256, 41, 50, '', NULL, 'Discipline Priest 41-50: Expert talents'),
(5, 256, 51, 60, '', NULL, 'Discipline Priest 51-60: Master talents'),
(5, 256, 61, 70, '', NULL, 'Discipline Priest 61-70: Pre-War Within'),
(5, 256, 71, 80, '', '', 'Discipline Priest 71-80: War Within + Hero Talents (Oracle/Voidweaver)');

-- Holy (Spec ID: 257)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(5, 257, 1, 10, '', NULL, 'Holy Priest 1-10: Early leveling (auto-learned spells)'),
(5, 257, 11, 20, '', NULL, 'Holy Priest 11-20: Basic talents'),
(5, 257, 21, 30, '', NULL, 'Holy Priest 21-30: Intermediate talents'),
(5, 257, 31, 40, '', NULL, 'Holy Priest 31-40: Advanced talents'),
(5, 257, 41, 50, '', NULL, 'Holy Priest 41-50: Expert talents'),
(5, 257, 51, 60, '', NULL, 'Holy Priest 51-60: Master talents'),
(5, 257, 61, 70, '', NULL, 'Holy Priest 61-70: Pre-War Within'),
(5, 257, 71, 80, '', '', 'Holy Priest 71-80: War Within + Hero Talents (Archon/Oracle)');

-- Shadow (Spec ID: 258)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(5, 258, 1, 10, '', NULL, 'Shadow Priest 1-10: Early leveling (auto-learned spells)'),
(5, 258, 11, 20, '', NULL, 'Shadow Priest 11-20: Basic talents'),
(5, 258, 21, 30, '', NULL, 'Shadow Priest 21-30: Intermediate talents'),
(5, 258, 31, 40, '', NULL, 'Shadow Priest 31-40: Advanced talents'),
(5, 258, 41, 50, '', NULL, 'Shadow Priest 41-50: Expert talents'),
(5, 258, 51, 60, '', NULL, 'Shadow Priest 51-60: Master talents'),
(5, 258, 61, 70, '', NULL, 'Shadow Priest 61-70: Pre-War Within'),
(5, 258, 71, 80, '', '', 'Shadow Priest 71-80: War Within + Hero Talents (Archon/Voidweaver)');

-- ====================================================================
-- DEATH KNIGHT (Class ID: 6)
-- ====================================================================

-- Blood (Spec ID: 250)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(6, 250, 1, 10, '', NULL, 'Blood Death Knight 1-10: Early leveling (auto-learned spells)'),
(6, 250, 11, 20, '', NULL, 'Blood Death Knight 11-20: Basic talents'),
(6, 250, 21, 30, '', NULL, 'Blood Death Knight 21-30: Intermediate talents'),
(6, 250, 31, 40, '', NULL, 'Blood Death Knight 31-40: Advanced talents'),
(6, 250, 41, 50, '', NULL, 'Blood Death Knight 41-50: Expert talents'),
(6, 250, 51, 60, '', NULL, 'Blood Death Knight 51-60: Master talents'),
(6, 250, 61, 70, '', NULL, 'Blood Death Knight 61-70: Pre-War Within'),
(6, 250, 71, 80, '', '', 'Blood Death Knight 71-80: War Within + Hero Talents (Deathbringer/San\'layn)');

-- Frost (Spec ID: 251)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(6, 251, 1, 10, '', NULL, 'Frost Death Knight 1-10: Early leveling (auto-learned spells)'),
(6, 251, 11, 20, '', NULL, 'Frost Death Knight 11-20: Basic talents'),
(6, 251, 21, 30, '', NULL, 'Frost Death Knight 21-30: Intermediate talents'),
(6, 251, 31, 40, '', NULL, 'Frost Death Knight 31-40: Advanced talents'),
(6, 251, 41, 50, '', NULL, 'Frost Death Knight 41-50: Expert talents'),
(6, 251, 51, 60, '', NULL, 'Frost Death Knight 51-60: Master talents'),
(6, 251, 61, 70, '', NULL, 'Frost Death Knight 61-70: Pre-War Within'),
(6, 251, 71, 80, '', '', 'Frost Death Knight 71-80: War Within + Hero Talents (Deathbringer/Rider of the Apocalypse)');

-- Unholy (Spec ID: 252)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(6, 252, 1, 10, '', NULL, 'Unholy Death Knight 1-10: Early leveling (auto-learned spells)'),
(6, 252, 11, 20, '', NULL, 'Unholy Death Knight 11-20: Basic talents'),
(6, 252, 21, 30, '', NULL, 'Unholy Death Knight 21-30: Intermediate talents'),
(6, 252, 31, 40, '', NULL, 'Unholy Death Knight 31-40: Advanced talents'),
(6, 252, 41, 50, '', NULL, 'Unholy Death Knight 41-50: Expert talents'),
(6, 252, 51, 60, '', NULL, 'Unholy Death Knight 51-60: Master talents'),
(6, 252, 61, 70, '', NULL, 'Unholy Death Knight 61-70: Pre-War Within'),
(6, 252, 71, 80, '', '', 'Unholy Death Knight 71-80: War Within + Hero Talents (Rider of the Apocalypse/San\'layn)');

-- ====================================================================
-- SHAMAN (Class ID: 7)
-- ====================================================================

-- Elemental (Spec ID: 262)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(7, 262, 1, 10, '', NULL, 'Elemental Shaman 1-10: Early leveling (auto-learned spells)'),
(7, 262, 11, 20, '', NULL, 'Elemental Shaman 11-20: Basic talents'),
(7, 262, 21, 30, '', NULL, 'Elemental Shaman 21-30: Intermediate talents'),
(7, 262, 31, 40, '', NULL, 'Elemental Shaman 31-40: Advanced talents'),
(7, 262, 41, 50, '', NULL, 'Elemental Shaman 41-50: Expert talents'),
(7, 262, 51, 60, '', NULL, 'Elemental Shaman 51-60: Master talents'),
(7, 262, 61, 70, '', NULL, 'Elemental Shaman 61-70: Pre-War Within'),
(7, 262, 71, 80, '', '', 'Elemental Shaman 71-80: War Within + Hero Talents (Farseer/Stormbringer)');

-- Enhancement (Spec ID: 263)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(7, 263, 1, 10, '', NULL, 'Enhancement Shaman 1-10: Early leveling (auto-learned spells)'),
(7, 263, 11, 20, '', NULL, 'Enhancement Shaman 11-20: Basic talents'),
(7, 263, 21, 30, '', NULL, 'Enhancement Shaman 21-30: Intermediate talents'),
(7, 263, 31, 40, '', NULL, 'Enhancement Shaman 31-40: Advanced talents'),
(7, 263, 41, 50, '', NULL, 'Enhancement Shaman 41-50: Expert talents'),
(7, 263, 51, 60, '', NULL, 'Enhancement Shaman 51-60: Master talents'),
(7, 263, 61, 70, '', NULL, 'Enhancement Shaman 61-70: Pre-War Within'),
(7, 263, 71, 80, '', '', 'Enhancement Shaman 71-80: War Within + Hero Talents (Stormbringer/Totemic)');

-- Restoration (Spec ID: 264)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(7, 264, 1, 10, '', NULL, 'Restoration Shaman 1-10: Early leveling (auto-learned spells)'),
(7, 264, 11, 20, '', NULL, 'Restoration Shaman 11-20: Basic talents'),
(7, 264, 21, 30, '', NULL, 'Restoration Shaman 21-30: Intermediate talents'),
(7, 264, 31, 40, '', NULL, 'Restoration Shaman 31-40: Advanced talents'),
(7, 264, 41, 50, '', NULL, 'Restoration Shaman 41-50: Expert talents'),
(7, 264, 51, 60, '', NULL, 'Restoration Shaman 51-60: Master talents'),
(7, 264, 61, 70, '', NULL, 'Restoration Shaman 61-70: Pre-War Within'),
(7, 264, 71, 80, '', '', 'Restoration Shaman 71-80: War Within + Hero Talents (Farseer/Totemic)');

-- ====================================================================
-- MAGE (Class ID: 8)
-- ====================================================================

-- Arcane (Spec ID: 62)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(8, 62, 1, 10, '', NULL, 'Arcane Mage 1-10: Early leveling (auto-learned spells)'),
(8, 62, 11, 20, '', NULL, 'Arcane Mage 11-20: Basic talents'),
(8, 62, 21, 30, '', NULL, 'Arcane Mage 21-30: Intermediate talents'),
(8, 62, 31, 40, '', NULL, 'Arcane Mage 31-40: Advanced talents'),
(8, 62, 41, 50, '', NULL, 'Arcane Mage 41-50: Expert talents'),
(8, 62, 51, 60, '', NULL, 'Arcane Mage 51-60: Master talents'),
(8, 62, 61, 70, '', NULL, 'Arcane Mage 61-70: Pre-War Within'),
(8, 62, 71, 80, '', '', 'Arcane Mage 71-80: War Within + Hero Talents (Spellslinger/Sunfury)');

-- Fire (Spec ID: 63)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(8, 63, 1, 10, '', NULL, 'Fire Mage 1-10: Early leveling (auto-learned spells)'),
(8, 63, 11, 20, '', NULL, 'Fire Mage 11-20: Basic talents'),
(8, 63, 21, 30, '', NULL, 'Fire Mage 21-30: Intermediate talents'),
(8, 63, 31, 40, '', NULL, 'Fire Mage 31-40: Advanced talents'),
(8, 63, 41, 50, '', NULL, 'Fire Mage 41-50: Expert talents'),
(8, 63, 51, 60, '', NULL, 'Fire Mage 51-60: Master talents'),
(8, 63, 61, 70, '', NULL, 'Fire Mage 61-70: Pre-War Within'),
(8, 63, 71, 80, '', '', 'Fire Mage 71-80: War Within + Hero Talents (Frostfire/Sunfury)');

-- Frost (Spec ID: 64)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(8, 64, 1, 10, '', NULL, 'Frost Mage 1-10: Early leveling (auto-learned spells)'),
(8, 64, 11, 20, '', NULL, 'Frost Mage 11-20: Basic talents'),
(8, 64, 21, 30, '', NULL, 'Frost Mage 21-30: Intermediate talents'),
(8, 64, 31, 40, '', NULL, 'Frost Mage 31-40: Advanced talents'),
(8, 64, 41, 50, '', NULL, 'Frost Mage 41-50: Expert talents'),
(8, 64, 51, 60, '', NULL, 'Frost Mage 51-60: Master talents'),
(8, 64, 61, 70, '', NULL, 'Frost Mage 61-70: Pre-War Within'),
(8, 64, 71, 80, '', '', 'Frost Mage 71-80: War Within + Hero Talents (Frostfire/Spellslinger)');

-- ====================================================================
-- WARLOCK (Class ID: 9)
-- ====================================================================

-- Affliction (Spec ID: 265)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(9, 265, 1, 10, '', NULL, 'Affliction Warlock 1-10: Early leveling (auto-learned spells)'),
(9, 265, 11, 20, '', NULL, 'Affliction Warlock 11-20: Basic talents'),
(9, 265, 21, 30, '', NULL, 'Affliction Warlock 21-30: Intermediate talents'),
(9, 265, 31, 40, '', NULL, 'Affliction Warlock 31-40: Advanced talents'),
(9, 265, 41, 50, '', NULL, 'Affliction Warlock 41-50: Expert talents'),
(9, 265, 51, 60, '', NULL, 'Affliction Warlock 51-60: Master talents'),
(9, 265, 61, 70, '', NULL, 'Affliction Warlock 61-70: Pre-War Within'),
(9, 265, 71, 80, '', '', 'Affliction Warlock 71-80: War Within + Hero Talents (Hellcaller/Soul Harvester)');

-- Demonology (Spec ID: 266)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(9, 266, 1, 10, '', NULL, 'Demonology Warlock 1-10: Early leveling (auto-learned spells)'),
(9, 266, 11, 20, '', NULL, 'Demonology Warlock 11-20: Basic talents'),
(9, 266, 21, 30, '', NULL, 'Demonology Warlock 21-30: Intermediate talents'),
(9, 266, 31, 40, '', NULL, 'Demonology Warlock 31-40: Advanced talents'),
(9, 266, 41, 50, '', NULL, 'Demonology Warlock 41-50: Expert talents'),
(9, 266, 51, 60, '', NULL, 'Demonology Warlock 51-60: Master talents'),
(9, 266, 61, 70, '', NULL, 'Demonology Warlock 61-70: Pre-War Within'),
(9, 266, 71, 80, '', '', 'Demonology Warlock 71-80: War Within + Hero Talents (Diabolist/Soul Harvester)');

-- Destruction (Spec ID: 267)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(9, 267, 1, 10, '', NULL, 'Destruction Warlock 1-10: Early leveling (auto-learned spells)'),
(9, 267, 11, 20, '', NULL, 'Destruction Warlock 11-20: Basic talents'),
(9, 267, 21, 30, '', NULL, 'Destruction Warlock 21-30: Intermediate talents'),
(9, 267, 31, 40, '', NULL, 'Destruction Warlock 31-40: Advanced talents'),
(9, 267, 41, 50, '', NULL, 'Destruction Warlock 41-50: Expert talents'),
(9, 267, 51, 60, '', NULL, 'Destruction Warlock 51-60: Master talents'),
(9, 267, 61, 70, '', NULL, 'Destruction Warlock 61-70: Pre-War Within'),
(9, 267, 71, 80, '', '', 'Destruction Warlock 71-80: War Within + Hero Talents (Diabolist/Hellcaller)');

-- ====================================================================
-- MONK (Class ID: 10)
-- ====================================================================

-- Brewmaster (Spec ID: 268)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(10, 268, 1, 10, '', NULL, 'Brewmaster Monk 1-10: Early leveling (auto-learned spells)'),
(10, 268, 11, 20, '', NULL, 'Brewmaster Monk 11-20: Basic talents'),
(10, 268, 21, 30, '', NULL, 'Brewmaster Monk 21-30: Intermediate talents'),
(10, 268, 31, 40, '', NULL, 'Brewmaster Monk 31-40: Advanced talents'),
(10, 268, 41, 50, '', NULL, 'Brewmaster Monk 41-50: Expert talents'),
(10, 268, 51, 60, '', NULL, 'Brewmaster Monk 51-60: Master talents'),
(10, 268, 61, 70, '', NULL, 'Brewmaster Monk 61-70: Pre-War Within'),
(10, 268, 71, 80, '', '', 'Brewmaster Monk 71-80: War Within + Hero Talents (Master of Harmony/Shado-Pan)');

-- Windwalker (Spec ID: 269)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(10, 269, 1, 10, '', NULL, 'Windwalker Monk 1-10: Early leveling (auto-learned spells)'),
(10, 269, 11, 20, '', NULL, 'Windwalker Monk 11-20: Basic talents'),
(10, 269, 21, 30, '', NULL, 'Windwalker Monk 21-30: Intermediate talents'),
(10, 269, 31, 40, '', NULL, 'Windwalker Monk 31-40: Advanced talents'),
(10, 269, 41, 50, '', NULL, 'Windwalker Monk 41-50: Expert talents'),
(10, 269, 51, 60, '', NULL, 'Windwalker Monk 51-60: Master talents'),
(10, 269, 61, 70, '', NULL, 'Windwalker Monk 61-70: Pre-War Within'),
(10, 269, 71, 80, '', '', 'Windwalker Monk 71-80: War Within + Hero Talents (Conduit of the Celestials/Shado-Pan)');

-- Mistweaver (Spec ID: 270)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(10, 270, 1, 10, '', NULL, 'Mistweaver Monk 1-10: Early leveling (auto-learned spells)'),
(10, 270, 11, 20, '', NULL, 'Mistweaver Monk 11-20: Basic talents'),
(10, 270, 21, 30, '', NULL, 'Mistweaver Monk 21-30: Intermediate talents'),
(10, 270, 31, 40, '', NULL, 'Mistweaver Monk 31-40: Advanced talents'),
(10, 270, 41, 50, '', NULL, 'Mistweaver Monk 41-50: Expert talents'),
(10, 270, 51, 60, '', NULL, 'Mistweaver Monk 51-60: Master talents'),
(10, 270, 61, 70, '', NULL, 'Mistweaver Monk 61-70: Pre-War Within'),
(10, 270, 71, 80, '', '', 'Mistweaver Monk 71-80: War Within + Hero Talents (Conduit of the Celestials/Master of Harmony)');

-- ====================================================================
-- DRUID (Class ID: 11)
-- ====================================================================

-- Balance (Spec ID: 102)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(11, 102, 1, 10, '', NULL, 'Balance Druid 1-10: Early leveling (auto-learned spells)'),
(11, 102, 11, 20, '', NULL, 'Balance Druid 11-20: Basic talents'),
(11, 102, 21, 30, '', NULL, 'Balance Druid 21-30: Intermediate talents'),
(11, 102, 31, 40, '', NULL, 'Balance Druid 31-40: Advanced talents'),
(11, 102, 41, 50, '', NULL, 'Balance Druid 41-50: Expert talents'),
(11, 102, 51, 60, '', NULL, 'Balance Druid 51-60: Master talents'),
(11, 102, 61, 70, '', NULL, 'Balance Druid 61-70: Pre-War Within'),
(11, 102, 71, 80, '', '', 'Balance Druid 71-80: War Within + Hero Talents (Elune\'s Chosen/Keeper of the Grove)');

-- Feral (Spec ID: 103)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(11, 103, 1, 10, '', NULL, 'Feral Druid 1-10: Early leveling (auto-learned spells)'),
(11, 103, 11, 20, '', NULL, 'Feral Druid 11-20: Basic talents'),
(11, 103, 21, 30, '', NULL, 'Feral Druid 21-30: Intermediate talents'),
(11, 103, 31, 40, '', NULL, 'Feral Druid 31-40: Advanced talents'),
(11, 103, 41, 50, '', NULL, 'Feral Druid 41-50: Expert talents'),
(11, 103, 51, 60, '', NULL, 'Feral Druid 51-60: Master talents'),
(11, 103, 61, 70, '', NULL, 'Feral Druid 61-70: Pre-War Within'),
(11, 103, 71, 80, '', '', 'Feral Druid 71-80: War Within + Hero Talents (Druid of the Claw/Wildstalker)');

-- Guardian (Spec ID: 104)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(11, 104, 1, 10, '', NULL, 'Guardian Druid 1-10: Early leveling (auto-learned spells)'),
(11, 104, 11, 20, '', NULL, 'Guardian Druid 11-20: Basic talents'),
(11, 104, 21, 30, '', NULL, 'Guardian Druid 21-30: Intermediate talents'),
(11, 104, 31, 40, '', NULL, 'Guardian Druid 31-40: Advanced talents'),
(11, 104, 41, 50, '', NULL, 'Guardian Druid 41-50: Expert talents'),
(11, 104, 51, 60, '', NULL, 'Guardian Druid 51-60: Master talents'),
(11, 104, 61, 70, '', NULL, 'Guardian Druid 61-70: Pre-War Within'),
(11, 104, 71, 80, '', '', 'Guardian Druid 71-80: War Within + Hero Talents (Druid of the Claw/Elune\'s Chosen)');

-- Restoration (Spec ID: 105)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(11, 105, 1, 10, '', NULL, 'Restoration Druid 1-10: Early leveling (auto-learned spells)'),
(11, 105, 11, 20, '', NULL, 'Restoration Druid 11-20: Basic talents'),
(11, 105, 21, 30, '', NULL, 'Restoration Druid 21-30: Intermediate talents'),
(11, 105, 31, 40, '', NULL, 'Restoration Druid 31-40: Advanced talents'),
(11, 105, 41, 50, '', NULL, 'Restoration Druid 41-50: Expert talents'),
(11, 105, 51, 60, '', NULL, 'Restoration Druid 51-60: Master talents'),
(11, 105, 61, 70, '', NULL, 'Restoration Druid 61-70: Pre-War Within'),
(11, 105, 71, 80, '', '', 'Restoration Druid 71-80: War Within + Hero Talents (Keeper of the Grove/Wildstalker)');

-- ====================================================================
-- DEMON HUNTER (Class ID: 12)
-- ====================================================================

-- Havoc (Spec ID: 577)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(12, 577, 1, 10, '', NULL, 'Havoc Demon Hunter 1-10: Early leveling (auto-learned spells)'),
(12, 577, 11, 20, '', NULL, 'Havoc Demon Hunter 11-20: Basic talents'),
(12, 577, 21, 30, '', NULL, 'Havoc Demon Hunter 21-30: Intermediate talents'),
(12, 577, 31, 40, '', NULL, 'Havoc Demon Hunter 31-40: Advanced talents'),
(12, 577, 41, 50, '', NULL, 'Havoc Demon Hunter 41-50: Expert talents'),
(12, 577, 51, 60, '', NULL, 'Havoc Demon Hunter 51-60: Master talents'),
(12, 577, 61, 70, '', NULL, 'Havoc Demon Hunter 61-70: Pre-War Within'),
(12, 577, 71, 80, '', '', 'Havoc Demon Hunter 71-80: War Within + Hero Talents (Aldrachi Reaver/Fel-Scarred)');

-- Vengeance (Spec ID: 581)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(12, 581, 1, 10, '', NULL, 'Vengeance Demon Hunter 1-10: Early leveling (auto-learned spells)'),
(12, 581, 11, 20, '', NULL, 'Vengeance Demon Hunter 11-20: Basic talents'),
(12, 581, 21, 30, '', NULL, 'Vengeance Demon Hunter 21-30: Intermediate talents'),
(12, 581, 31, 40, '', NULL, 'Vengeance Demon Hunter 31-40: Advanced talents'),
(12, 581, 41, 50, '', NULL, 'Vengeance Demon Hunter 41-50: Expert talents'),
(12, 581, 51, 60, '', NULL, 'Vengeance Demon Hunter 51-60: Master talents'),
(12, 581, 61, 70, '', NULL, 'Vengeance Demon Hunter 61-70: Pre-War Within'),
(12, 581, 71, 80, '', '', 'Vengeance Demon Hunter 71-80: War Within + Hero Talents (Aldrachi Reaver/Fel-Scarred)');

-- ====================================================================
-- EVOKER (Class ID: 13)
-- ====================================================================

-- Devastation (Spec ID: 1467)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(13, 1467, 1, 10, '', NULL, 'Devastation Evoker 1-10: Early leveling (auto-learned spells)'),
(13, 1467, 11, 20, '', NULL, 'Devastation Evoker 11-20: Basic talents'),
(13, 1467, 21, 30, '', NULL, 'Devastation Evoker 21-30: Intermediate talents'),
(13, 1467, 31, 40, '', NULL, 'Devastation Evoker 31-40: Advanced talents'),
(13, 1467, 41, 50, '', NULL, 'Devastation Evoker 41-50: Expert talents'),
(13, 1467, 51, 60, '', NULL, 'Devastation Evoker 51-60: Master talents'),
(13, 1467, 61, 70, '', NULL, 'Devastation Evoker 61-70: Pre-War Within'),
(13, 1467, 71, 80, '', '', 'Devastation Evoker 71-80: War Within + Hero Talents (Chronowarden/Flameshaper)');

-- Preservation (Spec ID: 1468)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(13, 1468, 1, 10, '', NULL, 'Preservation Evoker 1-10: Early leveling (auto-learned spells)'),
(13, 1468, 11, 20, '', NULL, 'Preservation Evoker 11-20: Basic talents'),
(13, 1468, 21, 30, '', NULL, 'Preservation Evoker 21-30: Intermediate talents'),
(13, 1468, 31, 40, '', NULL, 'Preservation Evoker 31-40: Advanced talents'),
(13, 1468, 41, 50, '', NULL, 'Preservation Evoker 41-50: Expert talents'),
(13, 1468, 51, 60, '', NULL, 'Preservation Evoker 51-60: Master talents'),
(13, 1468, 61, 70, '', NULL, 'Preservation Evoker 61-70: Pre-War Within'),
(13, 1468, 71, 80, '', '', 'Preservation Evoker 71-80: War Within + Hero Talents (Chronowarden/Flameshaper)');

-- Augmentation (Spec ID: 1473)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(13, 1473, 1, 10, '', NULL, 'Augmentation Evoker 1-10: Early leveling (auto-learned spells)'),
(13, 1473, 11, 20, '', NULL, 'Augmentation Evoker 11-20: Basic talents'),
(13, 1473, 21, 30, '', NULL, 'Augmentation Evoker 21-30: Intermediate talents'),
(13, 1473, 31, 40, '', NULL, 'Augmentation Evoker 31-40: Advanced talents'),
(13, 1473, 41, 50, '', NULL, 'Augmentation Evoker 41-50: Expert talents'),
(13, 1473, 51, 60, '', NULL, 'Augmentation Evoker 51-60: Master talents'),
(13, 1473, 61, 70, '', NULL, 'Augmentation Evoker 61-70: Pre-War Within'),
(13, 1473, 71, 80, '', '', 'Augmentation Evoker 71-80: War Within + Hero Talents (Chronowarden/Scalecommander)');

-- ====================================================================
-- IMPLEMENTATION NOTES
-- ====================================================================
-- Total Entries: 312 (39 specializations × 8 level brackets)
--
-- TALENT STRING FORMAT:
-- - talent_string: Comma-separated spell IDs (e.g., "12345,67890,11111")
-- - hero_talent_string: Comma-separated hero talent spell IDs (71-80 only)
--
-- HOW TO POPULATE ACTUAL TALENT DATA:
-- Method 1: Extract from player character database
--   SELECT talent_string FROM characters WHERE class=X AND spec=Y AND level=Z
--
-- Method 2: Query DB2 files using TrinityCore MCP tools
--   Use mcp__trinitycore__query-dbc("Talent.db2", talentId)
--
-- Method 3: Manual curation from guides (Icy-Veins, Wowhead)
--   Research optimal builds and map to spell IDs
--
-- LEVEL-APPROPRIATE TALENT SELECTION:
-- - Levels 1-10: No talents (empty string valid)
-- - Levels 11-70: Progressive talent unlocks
-- - Levels 71-80: Regular talents + Hero talents
--
-- HERO TALENT NOTES:
-- - Only for levels 71-80 (War Within expansion feature)
-- - Each class has 2-3 hero talent trees per specialization
-- - Description field indicates which hero trees are available
-- ====================================================================
