-- ====================================================================
-- Playerbot Talent Loadouts Table - POPULATED VERSION
-- ====================================================================
-- Purpose: Store predefined talent loadouts for automated bot creation
-- Integration: Used by BotTalentManager for instant level-up scenarios
-- Data Source: TrinityCore MCP Server (mcp__trinitycore__get-talent-build)
-- Generation Date: 2025-01-06
-- ====================================================================
-- Coverage: 22/39 specializations have populated talent data
-- Missing Data: Tank and healer specs require manual curation
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
-- WARRIOR (Class ID: 1)
-- ====================================================================

-- Arms (Spec ID: 71) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(1, 71, 1, 10, '22624,22360,22371,22548,22392,22394,22397', NULL, 'Arms Warrior 1-10: Single-target burst with Execute mastery'),
(1, 71, 11, 20, '22624,22360,22371,22548,22392,22394,22397', NULL, 'Arms Warrior 11-20: Single-target burst with Execute mastery'),
(1, 71, 21, 30, '22624,22360,22371,22548,22392,22394,22397', NULL, 'Arms Warrior 21-30: Single-target burst with Execute mastery'),
(1, 71, 31, 40, '22624,22360,22371,22548,22392,22394,22397', NULL, 'Arms Warrior 31-40: Single-target burst with Execute mastery'),
(1, 71, 41, 50, '22624,22360,22371,22548,22392,22394,22397', NULL, 'Arms Warrior 41-50: Single-target burst with Execute mastery'),
(1, 71, 51, 60, '22624,22360,22371,22548,22392,22394,22397', NULL, 'Arms Warrior 51-60: Single-target burst with Execute mastery'),
(1, 71, 61, 70, '22624,22360,22371,22548,22392,22394,22397', NULL, 'Arms Warrior 61-70: Single-target burst with Execute mastery'),
(1, 71, 71, 80, '22624,22360,22371,22548,22392,22394,22397', '', 'Arms Warrior 71-80: Raid build with Hero Talents (Colossus/Slayer)');

-- Fury (Spec ID: 72) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(1, 72, 1, 10, '22632,22491,22489,22490,22627', NULL, 'Fury Warrior 1-10: High sustain with self-healing'),
(1, 72, 11, 20, '22632,22491,22489,22490,22627', NULL, 'Fury Warrior 11-20: High sustain with self-healing'),
(1, 72, 21, 30, '22632,22491,22489,22490,22627', NULL, 'Fury Warrior 21-30: High sustain with self-healing'),
(1, 72, 31, 40, '22632,22491,22489,22490,22627', NULL, 'Fury Warrior 31-40: High sustain with self-healing'),
(1, 72, 41, 50, '22632,22491,22489,22490,22627', NULL, 'Fury Warrior 41-50: High sustain with self-healing'),
(1, 72, 51, 60, '22632,22491,22489,22490,22627', NULL, 'Fury Warrior 51-60: High sustain with self-healing'),
(1, 72, 61, 70, '22632,22491,22489,22490,22627', NULL, 'Fury Warrior 61-70: High sustain with self-healing'),
(1, 72, 71, 80, '22632,22633,22491,22489,22490,22627,22628', '', 'Fury Warrior 71-80: Reckless Abandon for sustained DPS (Colossus/Slayer)');

-- Protection (Spec ID: 73) - ❌ NO DATA (Tank spec - requires manual curation)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(1, 73, 1, 10, '', NULL, 'Protection Warrior 1-10: Requires manual talent curation'),
(1, 73, 11, 20, '', NULL, 'Protection Warrior 11-20: Requires manual talent curation'),
(1, 73, 21, 30, '', NULL, 'Protection Warrior 21-30: Requires manual talent curation'),
(1, 73, 31, 40, '', NULL, 'Protection Warrior 31-40: Requires manual talent curation'),
(1, 73, 41, 50, '', NULL, 'Protection Warrior 41-50: Requires manual talent curation'),
(1, 73, 51, 60, '', NULL, 'Protection Warrior 51-60: Requires manual talent curation'),
(1, 73, 61, 70, '', NULL, 'Protection Warrior 61-70: Requires manual talent curation'),
(1, 73, 71, 80, '', '', 'Protection Warrior 71-80: Requires manual talent curation (Colossus/Mountain Thane)');

-- ====================================================================
-- PALADIN (Class ID: 2)
-- ====================================================================

-- Holy (Spec ID: 65) - ❌ NO DATA (Healer spec - requires manual curation)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(2, 65, 1, 10, '', NULL, 'Holy Paladin 1-10: Requires manual talent curation'),
(2, 65, 11, 20, '', NULL, 'Holy Paladin 11-20: Requires manual talent curation'),
(2, 65, 21, 30, '', NULL, 'Holy Paladin 21-30: Requires manual talent curation'),
(2, 65, 31, 40, '', NULL, 'Holy Paladin 31-40: Requires manual talent curation'),
(2, 65, 41, 50, '', NULL, 'Holy Paladin 41-50: Requires manual talent curation'),
(2, 65, 51, 60, '', NULL, 'Holy Paladin 51-60: Requires manual talent curation'),
(2, 65, 61, 70, '', NULL, 'Holy Paladin 61-70: Requires manual talent curation'),
(2, 65, 71, 80, '', '', 'Holy Paladin 71-80: Requires manual talent curation (Lightsmith/Herald of the Sun)');

-- Protection (Spec ID: 66) - ❌ NO DATA (Tank spec - requires manual curation)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(2, 66, 1, 10, '', NULL, 'Protection Paladin 1-10: Requires manual talent curation'),
(2, 66, 11, 20, '', NULL, 'Protection Paladin 11-20: Requires manual talent curation'),
(2, 66, 21, 30, '', NULL, 'Protection Paladin 21-30: Requires manual talent curation'),
(2, 66, 31, 40, '', NULL, 'Protection Paladin 31-40: Requires manual talent curation'),
(2, 66, 41, 50, '', NULL, 'Protection Paladin 41-50: Requires manual talent curation'),
(2, 66, 51, 60, '', NULL, 'Protection Paladin 51-60: Requires manual talent curation'),
(2, 66, 61, 70, '', NULL, 'Protection Paladin 61-70: Requires manual talent curation'),
(2, 66, 71, 80, '', '', 'Protection Paladin 71-80: Requires manual talent curation (Lightsmith/Templar)');

-- Retribution (Spec ID: 70) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(2, 70, 1, 10, '22586,22587,22591,22592,23110,23111,23112', NULL, 'Retribution Paladin 1-10: Templar Verdict with Wake of Ashes'),
(2, 70, 11, 20, '22586,22587,22591,22592,23110,23111,23112', NULL, 'Retribution Paladin 11-20: Templar Verdict with Wake of Ashes'),
(2, 70, 21, 30, '22586,22587,22591,22592,23110,23111,23112', NULL, 'Retribution Paladin 21-30: Templar Verdict with Wake of Ashes'),
(2, 70, 31, 40, '22586,22587,22591,22592,23110,23111,23112', NULL, 'Retribution Paladin 31-40: Templar Verdict with Wake of Ashes'),
(2, 70, 41, 50, '22586,22587,22591,22592,23110,23111,23112', NULL, 'Retribution Paladin 41-50: Templar Verdict with Wake of Ashes'),
(2, 70, 51, 60, '22586,22587,22591,22592,23110,23111,23112', NULL, 'Retribution Paladin 51-60: Templar Verdict with Wake of Ashes'),
(2, 70, 61, 70, '22586,22587,22591,22592,23110,23111,23112', NULL, 'Retribution Paladin 61-70: Templar Verdict with Wake of Ashes'),
(2, 70, 71, 80, '22586,22587,22591,22592,23110,23111,23112', '', 'Retribution Paladin 71-80: Raid build (Herald of the Sun/Templar)');

-- ====================================================================
-- HUNTER (Class ID: 3)
-- ====================================================================

-- Beast Mastery (Spec ID: 253) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(3, 253, 1, 10, '22276,22277,22286,22287', NULL, 'Beast Mastery Hunter 1-10: Pet tanking with ranged safety'),
(3, 253, 11, 20, '22276,22277,22286,22287', NULL, 'Beast Mastery Hunter 11-20: Pet tanking with ranged safety'),
(3, 253, 21, 30, '22276,22277,22286,22287', NULL, 'Beast Mastery Hunter 21-30: Pet tanking with ranged safety'),
(3, 253, 31, 40, '22276,22277,22286,22287', NULL, 'Beast Mastery Hunter 31-40: Pet tanking with ranged safety'),
(3, 253, 41, 50, '22276,22277,22286,22287', NULL, 'Beast Mastery Hunter 41-50: Pet tanking with ranged safety'),
(3, 253, 51, 60, '22276,22277,22286,22287', NULL, 'Beast Mastery Hunter 51-60: Pet tanking with ranged safety'),
(3, 253, 61, 70, '22276,22277,22286,22287', NULL, 'Beast Mastery Hunter 61-70: Pet tanking with ranged safety'),
(3, 253, 71, 80, '22276,22277,22286,22287', '', 'Beast Mastery Hunter 71-80: Raid build (Dark Ranger/Pack Leader)');

-- Marksmanship (Spec ID: 254) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(3, 254, 1, 10, '22279,22280,22289,22290,23260,23261,23262', NULL, 'Marksmanship Hunter 1-10: Aimed Shot with Serpent Sting'),
(3, 254, 11, 20, '22279,22280,22289,22290,23260,23261,23262', NULL, 'Marksmanship Hunter 11-20: Aimed Shot with Serpent Sting'),
(3, 254, 21, 30, '22279,22280,22289,22290,23260,23261,23262', NULL, 'Marksmanship Hunter 21-30: Aimed Shot with Serpent Sting'),
(3, 254, 31, 40, '22279,22280,22289,22290,23260,23261,23262', NULL, 'Marksmanship Hunter 31-40: Aimed Shot with Serpent Sting'),
(3, 254, 41, 50, '22279,22280,22289,22290,23260,23261,23262', NULL, 'Marksmanship Hunter 41-50: Aimed Shot with Serpent Sting'),
(3, 254, 51, 60, '22279,22280,22289,22290,23260,23261,23262', NULL, 'Marksmanship Hunter 51-60: Aimed Shot with Serpent Sting'),
(3, 254, 61, 70, '22279,22280,22289,22290,23260,23261,23262', NULL, 'Marksmanship Hunter 61-70: Aimed Shot with Serpent Sting'),
(3, 254, 71, 80, '22279,22280,22289,22290,23260,23261,23262', '', 'Marksmanship Hunter 71-80: Raid build (Dark Ranger/Sentinel)');

-- Survival (Spec ID: 255) - ❌ NO DATA (Melee spec - requires manual curation)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(3, 255, 1, 10, '', NULL, 'Survival Hunter 1-10: Requires manual talent curation'),
(3, 255, 11, 20, '', NULL, 'Survival Hunter 11-20: Requires manual talent curation'),
(3, 255, 21, 30, '', NULL, 'Survival Hunter 21-30: Requires manual talent curation'),
(3, 255, 31, 40, '', NULL, 'Survival Hunter 31-40: Requires manual talent curation'),
(3, 255, 41, 50, '', NULL, 'Survival Hunter 41-50: Requires manual talent curation'),
(3, 255, 51, 60, '', NULL, 'Survival Hunter 51-60: Requires manual talent curation'),
(3, 255, 61, 70, '', NULL, 'Survival Hunter 61-70: Requires manual talent curation'),
(3, 255, 71, 80, '', '', 'Survival Hunter 71-80: Requires manual talent curation (Pack Leader/Sentinel)');

-- ====================================================================
-- ROGUE (Class ID: 4)
-- ====================================================================

-- Assassination (Spec ID: 259) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(4, 259, 1, 10, '22113,22114,22121,22122,23174,23172,23171', NULL, 'Assassination Rogue 1-10: Poison-focused with Deathmark burst'),
(4, 259, 11, 20, '22113,22114,22121,22122,23174,23172,23171', NULL, 'Assassination Rogue 11-20: Poison-focused with Deathmark burst'),
(4, 259, 21, 30, '22113,22114,22121,22122,23174,23172,23171', NULL, 'Assassination Rogue 21-30: Poison-focused with Deathmark burst'),
(4, 259, 31, 40, '22113,22114,22121,22122,23174,23172,23171', NULL, 'Assassination Rogue 31-40: Poison-focused with Deathmark burst'),
(4, 259, 41, 50, '22113,22114,22121,22122,23174,23172,23171', NULL, 'Assassination Rogue 41-50: Poison-focused with Deathmark burst'),
(4, 259, 51, 60, '22113,22114,22121,22122,23174,23172,23171', NULL, 'Assassination Rogue 51-60: Poison-focused with Deathmark burst'),
(4, 259, 61, 70, '22113,22114,22121,22122,23174,23172,23171', NULL, 'Assassination Rogue 61-70: Poison-focused with Deathmark burst'),
(4, 259, 71, 80, '22113,22114,22121,22122,23174,23172,23171', '', 'Assassination Rogue 71-80: Raid build (Deathstalker/Fatebound)');

-- Outlaw (Spec ID: 260) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(4, 260, 1, 10, '22138,22139,22155,22156,23175,23176,23177', NULL, 'Outlaw Rogue 1-10: Roll the Bones with Slice and Dice'),
(4, 260, 11, 20, '22138,22139,22155,22156,23175,23176,23177', NULL, 'Outlaw Rogue 11-20: Roll the Bones with Slice and Dice'),
(4, 260, 21, 30, '22138,22139,22155,22156,23175,23176,23177', NULL, 'Outlaw Rogue 21-30: Roll the Bones with Slice and Dice'),
(4, 260, 31, 40, '22138,22139,22155,22156,23175,23176,23177', NULL, 'Outlaw Rogue 31-40: Roll the Bones with Slice and Dice'),
(4, 260, 41, 50, '22138,22139,22155,22156,23175,23176,23177', NULL, 'Outlaw Rogue 41-50: Roll the Bones with Slice and Dice'),
(4, 260, 51, 60, '22138,22139,22155,22156,23175,23176,23177', NULL, 'Outlaw Rogue 51-60: Roll the Bones with Slice and Dice'),
(4, 260, 61, 70, '22138,22139,22155,22156,23175,23176,23177', NULL, 'Outlaw Rogue 61-70: Roll the Bones with Slice and Dice'),
(4, 260, 71, 80, '22138,22139,22155,22156,23175,23176,23177', '', 'Outlaw Rogue 71-80: Raid build (Fatebound/Trickster)');

-- Subtlety (Spec ID: 261) - ❌ NO DATA (Stealth spec - requires manual curation)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(4, 261, 1, 10, '', NULL, 'Subtlety Rogue 1-10: Requires manual talent curation'),
(4, 261, 11, 20, '', NULL, 'Subtlety Rogue 11-20: Requires manual talent curation'),
(4, 261, 21, 30, '', NULL, 'Subtlety Rogue 21-30: Requires manual talent curation'),
(4, 261, 31, 40, '', NULL, 'Subtlety Rogue 31-40: Requires manual talent curation'),
(4, 261, 41, 50, '', NULL, 'Subtlety Rogue 41-50: Requires manual talent curation'),
(4, 261, 51, 60, '', NULL, 'Subtlety Rogue 51-60: Requires manual talent curation'),
(4, 261, 61, 70, '', NULL, 'Subtlety Rogue 61-70: Requires manual talent curation'),
(4, 261, 71, 80, '', '', 'Subtlety Rogue 71-80: Requires manual talent curation (Deathstalker/Trickster)');

-- ====================================================================
-- PRIEST (Class ID: 5)
-- ====================================================================

-- Discipline (Spec ID: 256) - ❌ NO DATA (Healer spec - requires manual curation)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(5, 256, 1, 10, '', NULL, 'Discipline Priest 1-10: Requires manual talent curation'),
(5, 256, 11, 20, '', NULL, 'Discipline Priest 11-20: Requires manual talent curation'),
(5, 256, 21, 30, '', NULL, 'Discipline Priest 21-30: Requires manual talent curation'),
(5, 256, 31, 40, '', NULL, 'Discipline Priest 31-40: Requires manual talent curation'),
(5, 256, 41, 50, '', NULL, 'Discipline Priest 41-50: Requires manual talent curation'),
(5, 256, 51, 60, '', NULL, 'Discipline Priest 51-60: Requires manual talent curation'),
(5, 256, 61, 70, '', NULL, 'Discipline Priest 61-70: Requires manual talent curation'),
(5, 256, 71, 80, '', '', 'Discipline Priest 71-80: Requires manual talent curation (Archon/Oracle)');

-- Holy (Spec ID: 257) - ❌ NO DATA (Healer spec - requires manual curation)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(5, 257, 1, 10, '', NULL, 'Holy Priest 1-10: Requires manual talent curation'),
(5, 257, 11, 20, '', NULL, 'Holy Priest 11-20: Requires manual talent curation'),
(5, 257, 21, 30, '', NULL, 'Holy Priest 21-30: Requires manual talent curation'),
(5, 257, 31, 40, '', NULL, 'Holy Priest 31-40: Requires manual talent curation'),
(5, 257, 41, 50, '', NULL, 'Holy Priest 41-50: Requires manual talent curation'),
(5, 257, 51, 60, '', NULL, 'Holy Priest 51-60: Requires manual talent curation'),
(5, 257, 61, 70, '', NULL, 'Holy Priest 61-70: Requires manual talent curation'),
(5, 257, 71, 80, '', '', 'Holy Priest 71-80: Requires manual talent curation (Archon/Oracle)');

-- Shadow (Spec ID: 258) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(5, 258, 1, 10, '22312,22313,22314,22331,23046,23047,23048', NULL, 'Shadow Priest 1-10: Void Eruption with Dark Ascension'),
(5, 258, 11, 20, '22312,22313,22314,22331,23046,23047,23048', NULL, 'Shadow Priest 11-20: Void Eruption with Dark Ascension'),
(5, 258, 21, 30, '22312,22313,22314,22331,23046,23047,23048', NULL, 'Shadow Priest 21-30: Void Eruption with Dark Ascension'),
(5, 258, 31, 40, '22312,22313,22314,22331,23046,23047,23048', NULL, 'Shadow Priest 31-40: Void Eruption with Dark Ascension'),
(5, 258, 41, 50, '22312,22313,22314,22331,23046,23047,23048', NULL, 'Shadow Priest 41-50: Void Eruption with Dark Ascension'),
(5, 258, 51, 60, '22312,22313,22314,22331,23046,23047,23048', NULL, 'Shadow Priest 51-60: Void Eruption with Dark Ascension'),
(5, 258, 61, 70, '22312,22313,22314,22331,23046,23047,23048', NULL, 'Shadow Priest 61-70: Void Eruption with Dark Ascension'),
(5, 258, 71, 80, '22312,22313,22314,22331,23046,23047,23048', '', 'Shadow Priest 71-80: Raid build (Archon/Voidweaver)');

-- ====================================================================
-- DEATH KNIGHT (Class ID: 6)
-- ====================================================================

-- Blood (Spec ID: 250) - ❌ NO DATA (Tank spec - requires manual curation)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(6, 250, 1, 10, '', NULL, 'Blood Death Knight 1-10: Requires manual talent curation'),
(6, 250, 11, 20, '', NULL, 'Blood Death Knight 11-20: Requires manual talent curation'),
(6, 250, 21, 30, '', NULL, 'Blood Death Knight 21-30: Requires manual talent curation'),
(6, 250, 31, 40, '', NULL, 'Blood Death Knight 31-40: Requires manual talent curation'),
(6, 250, 41, 50, '', NULL, 'Blood Death Knight 41-50: Requires manual talent curation'),
(6, 250, 51, 60, '', NULL, 'Blood Death Knight 51-60: Requires manual talent curation'),
(6, 250, 61, 70, '', NULL, 'Blood Death Knight 61-70: Requires manual talent curation'),
(6, 250, 71, 80, '', '', 'Blood Death Knight 71-80: Requires manual talent curation (Deathbringer/San''layn)');

-- Frost (Spec ID: 251) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(6, 251, 1, 10, '22016,22017,22020,22021,23093,23094,23092', NULL, 'Frost Death Knight 1-10: Breath of Sindragosa build'),
(6, 251, 11, 20, '22016,22017,22020,22021,23093,23094,23092', NULL, 'Frost Death Knight 11-20: Breath of Sindragosa build'),
(6, 251, 21, 30, '22016,22017,22020,22021,23093,23094,23092', NULL, 'Frost Death Knight 21-30: Breath of Sindragosa build'),
(6, 251, 31, 40, '22016,22017,22020,22021,23093,23094,23092', NULL, 'Frost Death Knight 31-40: Breath of Sindragosa build'),
(6, 251, 41, 50, '22016,22017,22020,22021,23093,23094,23092', NULL, 'Frost Death Knight 41-50: Breath of Sindragosa build'),
(6, 251, 51, 60, '22016,22017,22020,22021,23093,23094,23092', NULL, 'Frost Death Knight 51-60: Breath of Sindragosa build'),
(6, 251, 61, 70, '22016,22017,22020,22021,23093,23094,23092', NULL, 'Frost Death Knight 61-70: Breath of Sindragosa build'),
(6, 251, 71, 80, '22016,22017,22020,22021,23093,23094,23092', '', 'Frost Death Knight 71-80: Raid build (Deathbringer/Rider of the Apocalypse)');

-- Unholy (Spec ID: 252) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(6, 252, 1, 10, '22006,22007,22024,22025,23095,23096,23097', NULL, 'Unholy Death Knight 1-10: Disease and minion focused'),
(6, 252, 11, 20, '22006,22007,22024,22025,23095,23096,23097', NULL, 'Unholy Death Knight 11-20: Disease and minion focused'),
(6, 252, 21, 30, '22006,22007,22024,22025,23095,23096,23097', NULL, 'Unholy Death Knight 21-30: Disease and minion focused'),
(6, 252, 31, 40, '22006,22007,22024,22025,23095,23096,23097', NULL, 'Unholy Death Knight 31-40: Disease and minion focused'),
(6, 252, 41, 50, '22006,22007,22024,22025,23095,23096,23097', NULL, 'Unholy Death Knight 41-50: Disease and minion focused'),
(6, 252, 51, 60, '22006,22007,22024,22025,23095,23096,23097', NULL, 'Unholy Death Knight 51-60: Disease and minion focused'),
(6, 252, 61, 70, '22006,22007,22024,22025,23095,23096,23097', NULL, 'Unholy Death Knight 61-70: Disease and minion focused'),
(6, 252, 71, 80, '22006,22007,22024,22025,23095,23096,23097', '', 'Unholy Death Knight 71-80: Raid build (Rider of the Apocalypse/San''layn)');

-- ====================================================================
-- SHAMAN (Class ID: 7)
-- ====================================================================

-- Elemental (Spec ID: 262) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(7, 262, 1, 10, '22356,22357,22358,22361,23066,23067,23068', NULL, 'Elemental Shaman 1-10: Lava Burst with Elemental Blast'),
(7, 262, 11, 20, '22356,22357,22358,22361,23066,23067,23068', NULL, 'Elemental Shaman 11-20: Lava Burst with Elemental Blast'),
(7, 262, 21, 30, '22356,22357,22358,22361,23066,23067,23068', NULL, 'Elemental Shaman 21-30: Lava Burst with Elemental Blast'),
(7, 262, 31, 40, '22356,22357,22358,22361,23066,23067,23068', NULL, 'Elemental Shaman 31-40: Lava Burst with Elemental Blast'),
(7, 262, 41, 50, '22356,22357,22358,22361,23066,23067,23068', NULL, 'Elemental Shaman 41-50: Lava Burst with Elemental Blast'),
(7, 262, 51, 60, '22356,22357,22358,22361,23066,23067,23068', NULL, 'Elemental Shaman 51-60: Lava Burst with Elemental Blast'),
(7, 262, 61, 70, '22356,22357,22358,22361,23066,23067,23068', NULL, 'Elemental Shaman 61-70: Lava Burst with Elemental Blast'),
(7, 262, 71, 80, '22356,22357,22358,22361,23066,23067,23068', '', 'Elemental Shaman 71-80: Raid build (Farseer/Stormbringer)');

-- Enhancement (Spec ID: 263) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(7, 263, 1, 10, '22354,22355,22359,22360,23064,23065,23063', NULL, 'Enhancement Shaman 1-10: Windfury with Doom Winds'),
(7, 263, 11, 20, '22354,22355,22359,22360,23064,23065,23063', NULL, 'Enhancement Shaman 11-20: Windfury with Doom Winds'),
(7, 263, 21, 30, '22354,22355,22359,22360,23064,23065,23063', NULL, 'Enhancement Shaman 21-30: Windfury with Doom Winds'),
(7, 263, 31, 40, '22354,22355,22359,22360,23064,23065,23063', NULL, 'Enhancement Shaman 31-40: Windfury with Doom Winds'),
(7, 263, 41, 50, '22354,22355,22359,22360,23064,23065,23063', NULL, 'Enhancement Shaman 41-50: Windfury with Doom Winds'),
(7, 263, 51, 60, '22354,22355,22359,22360,23064,23065,23063', NULL, 'Enhancement Shaman 51-60: Windfury with Doom Winds'),
(7, 263, 61, 70, '22354,22355,22359,22360,23064,23065,23063', NULL, 'Enhancement Shaman 61-70: Windfury with Doom Winds'),
(7, 263, 71, 80, '22354,22355,22359,22360,23064,23065,23063', '', 'Enhancement Shaman 71-80: Raid build (Stormbringer/Totemic)');

-- Restoration (Spec ID: 264) - ❌ NO DATA (Healer spec - requires manual curation)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(7, 264, 1, 10, '', NULL, 'Restoration Shaman 1-10: Requires manual talent curation'),
(7, 264, 11, 20, '', NULL, 'Restoration Shaman 11-20: Requires manual talent curation'),
(7, 264, 21, 30, '', NULL, 'Restoration Shaman 21-30: Requires manual talent curation'),
(7, 264, 31, 40, '', NULL, 'Restoration Shaman 31-40: Requires manual talent curation'),
(7, 264, 41, 50, '', NULL, 'Restoration Shaman 41-50: Requires manual talent curation'),
(7, 264, 51, 60, '', NULL, 'Restoration Shaman 51-60: Requires manual talent curation'),
(7, 264, 61, 70, '', NULL, 'Restoration Shaman 61-70: Requires manual talent curation'),
(7, 264, 71, 80, '', '', 'Restoration Shaman 71-80: Requires manual talent curation (Farseer/Totemic)');

-- ====================================================================
-- MAGE (Class ID: 8)
-- ====================================================================

-- Arcane (Spec ID: 62) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(8, 62, 1, 10, '22443,22444,22464,23087,23088,23089,23090', NULL, 'Arcane Mage 1-10: Arcane Surge burst window optimization'),
(8, 62, 11, 20, '22443,22444,22464,23087,23088,23089,23090', NULL, 'Arcane Mage 11-20: Arcane Surge burst window optimization'),
(8, 62, 21, 30, '22443,22444,22464,23087,23088,23089,23090', NULL, 'Arcane Mage 21-30: Arcane Surge burst window optimization'),
(8, 62, 31, 40, '22443,22444,22464,23087,23088,23089,23090', NULL, 'Arcane Mage 31-40: Arcane Surge burst window optimization'),
(8, 62, 41, 50, '22443,22444,22464,23087,23088,23089,23090', NULL, 'Arcane Mage 41-50: Arcane Surge burst window optimization'),
(8, 62, 51, 60, '22443,22444,22464,23087,23088,23089,23090', NULL, 'Arcane Mage 51-60: Arcane Surge burst window optimization'),
(8, 62, 61, 70, '22443,22444,22464,23087,23088,23089,23090', NULL, 'Arcane Mage 61-70: Arcane Surge burst window optimization'),
(8, 62, 71, 80, '22443,22444,22464,23087,23088,23089,23090', '', 'Arcane Mage 71-80: Raid build (Spellslinger/Sunfury)');

-- Fire (Spec ID: 63) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(8, 63, 1, 10, '22456,22458,22463,22465', NULL, 'Fire Mage 1-10: AoE focused for quest efficiency'),
(8, 63, 11, 20, '22456,22458,22463,22465', NULL, 'Fire Mage 11-20: AoE focused for quest efficiency'),
(8, 63, 21, 30, '22456,22458,22463,22465', NULL, 'Fire Mage 21-30: AoE focused for quest efficiency'),
(8, 63, 31, 40, '22456,22458,22463,22465', NULL, 'Fire Mage 31-40: AoE focused for quest efficiency'),
(8, 63, 41, 50, '22456,22458,22463,22465', NULL, 'Fire Mage 41-50: AoE focused for quest efficiency'),
(8, 63, 51, 60, '22456,22458,22463,22465', NULL, 'Fire Mage 51-60: AoE focused for quest efficiency'),
(8, 63, 61, 70, '22456,22458,22463,22465', NULL, 'Fire Mage 61-70: AoE focused for quest efficiency'),
(8, 63, 71, 80, '22456,22458,22463,22465', '', 'Fire Mage 71-80: Raid build (Frostfire/Sunfury)');

-- Frost (Spec ID: 64) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(8, 64, 1, 10, '22446,22447,22448,23073,23074,23078,23091', NULL, 'Frost Mage 1-10: Glacial Spike build for single-target'),
(8, 64, 11, 20, '22446,22447,22448,23073,23074,23078,23091', NULL, 'Frost Mage 11-20: Glacial Spike build for single-target'),
(8, 64, 21, 30, '22446,22447,22448,23073,23074,23078,23091', NULL, 'Frost Mage 21-30: Glacial Spike build for single-target'),
(8, 64, 31, 40, '22446,22447,22448,23073,23074,23078,23091', NULL, 'Frost Mage 31-40: Glacial Spike build for single-target'),
(8, 64, 41, 50, '22446,22447,22448,23073,23074,23078,23091', NULL, 'Frost Mage 41-50: Glacial Spike build for single-target'),
(8, 64, 51, 60, '22446,22447,22448,23073,23074,23078,23091', NULL, 'Frost Mage 51-60: Glacial Spike build for single-target'),
(8, 64, 61, 70, '22446,22447,22448,23073,23074,23078,23091', NULL, 'Frost Mage 61-70: Glacial Spike build for single-target'),
(8, 64, 71, 80, '22446,22447,22448,23073,23074,23078,23091', '', 'Frost Mage 71-80: Raid build (Frostfire/Spellslinger)');

-- ====================================================================
-- WARLOCK (Class ID: 9)
-- ====================================================================

-- Affliction (Spec ID: 265) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(9, 265, 1, 10, '22039,22040,22041,22063,23137,23138,23139', NULL, 'Affliction Warlock 1-10: Multi-DoT with Drain Soul execute'),
(9, 265, 11, 20, '22039,22040,22041,22063,23137,23138,23139', NULL, 'Affliction Warlock 11-20: Multi-DoT with Drain Soul execute'),
(9, 265, 21, 30, '22039,22040,22041,22063,23137,23138,23139', NULL, 'Affliction Warlock 21-30: Multi-DoT with Drain Soul execute'),
(9, 265, 31, 40, '22039,22040,22041,22063,23137,23138,23139', NULL, 'Affliction Warlock 31-40: Multi-DoT with Drain Soul execute'),
(9, 265, 41, 50, '22039,22040,22041,22063,23137,23138,23139', NULL, 'Affliction Warlock 41-50: Multi-DoT with Drain Soul execute'),
(9, 265, 51, 60, '22039,22040,22041,22063,23137,23138,23139', NULL, 'Affliction Warlock 51-60: Multi-DoT with Drain Soul execute'),
(9, 265, 61, 70, '22039,22040,22041,22063,23137,23138,23139', NULL, 'Affliction Warlock 61-70: Multi-DoT with Drain Soul execute'),
(9, 265, 71, 80, '22039,22040,22041,22063,23137,23138,23139', '', 'Affliction Warlock 71-80: Raid build (Hellcaller/Soul Harvester)');

-- Demonology (Spec ID: 266) - ❌ NO DATA (Pet spec - requires manual curation)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(9, 266, 1, 10, '', NULL, 'Demonology Warlock 1-10: Requires manual talent curation'),
(9, 266, 11, 20, '', NULL, 'Demonology Warlock 11-20: Requires manual talent curation'),
(9, 266, 21, 30, '', NULL, 'Demonology Warlock 21-30: Requires manual talent curation'),
(9, 266, 31, 40, '', NULL, 'Demonology Warlock 31-40: Requires manual talent curation'),
(9, 266, 41, 50, '', NULL, 'Demonology Warlock 41-50: Requires manual talent curation'),
(9, 266, 51, 60, '', NULL, 'Demonology Warlock 51-60: Requires manual talent curation'),
(9, 266, 61, 70, '', NULL, 'Demonology Warlock 61-70: Requires manual talent curation'),
(9, 266, 71, 80, '', '', 'Demonology Warlock 71-80: Requires manual talent curation (Diabolist/Soul Harvester)');

-- Destruction (Spec ID: 267) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(9, 267, 1, 10, '22045,22046,22047,22069,23140,23141,23142', NULL, 'Destruction Warlock 1-10: Chaos Bolt burst with Infernal'),
(9, 267, 11, 20, '22045,22046,22047,22069,23140,23141,23142', NULL, 'Destruction Warlock 11-20: Chaos Bolt burst with Infernal'),
(9, 267, 21, 30, '22045,22046,22047,22069,23140,23141,23142', NULL, 'Destruction Warlock 21-30: Chaos Bolt burst with Infernal'),
(9, 267, 31, 40, '22045,22046,22047,22069,23140,23141,23142', NULL, 'Destruction Warlock 31-40: Chaos Bolt burst with Infernal'),
(9, 267, 41, 50, '22045,22046,22047,22069,23140,23141,23142', NULL, 'Destruction Warlock 41-50: Chaos Bolt burst with Infernal'),
(9, 267, 51, 60, '22045,22046,22047,22069,23140,23141,23142', NULL, 'Destruction Warlock 51-60: Chaos Bolt burst with Infernal'),
(9, 267, 61, 70, '22045,22046,22047,22069,23140,23141,23142', NULL, 'Destruction Warlock 61-70: Chaos Bolt burst with Infernal'),
(9, 267, 71, 80, '22045,22046,22047,22069,23140,23141,23142', '', 'Destruction Warlock 71-80: Raid build (Diabolist/Hellcaller)');

-- ====================================================================
-- MONK (Class ID: 10)
-- ====================================================================

-- Brewmaster (Spec ID: 268) - ❌ NO DATA (Tank spec - requires manual curation)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(10, 268, 1, 10, '', NULL, 'Brewmaster Monk 1-10: Requires manual talent curation'),
(10, 268, 11, 20, '', NULL, 'Brewmaster Monk 11-20: Requires manual talent curation'),
(10, 268, 21, 30, '', NULL, 'Brewmaster Monk 21-30: Requires manual talent curation'),
(10, 268, 31, 40, '', NULL, 'Brewmaster Monk 31-40: Requires manual talent curation'),
(10, 268, 41, 50, '', NULL, 'Brewmaster Monk 41-50: Requires manual talent curation'),
(10, 268, 51, 60, '', NULL, 'Brewmaster Monk 51-60: Requires manual talent curation'),
(10, 268, 61, 70, '', NULL, 'Brewmaster Monk 61-70: Requires manual talent curation'),
(10, 268, 71, 80, '', '', 'Brewmaster Monk 71-80: Requires manual talent curation (Master of Harmony/Shado-Pan)');

-- Windwalker (Spec ID: 269) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(10, 269, 1, 10, '22098,22099,22107,22108,23167,23168,23169', NULL, 'Windwalker Monk 1-10: Strike of the Windlord with Serenity'),
(10, 269, 11, 20, '22098,22099,22107,22108,23167,23168,23169', NULL, 'Windwalker Monk 11-20: Strike of the Windlord with Serenity'),
(10, 269, 21, 30, '22098,22099,22107,22108,23167,23168,23169', NULL, 'Windwalker Monk 21-30: Strike of the Windlord with Serenity'),
(10, 269, 31, 40, '22098,22099,22107,22108,23167,23168,23169', NULL, 'Windwalker Monk 31-40: Strike of the Windlord with Serenity'),
(10, 269, 41, 50, '22098,22099,22107,22108,23167,23168,23169', NULL, 'Windwalker Monk 41-50: Strike of the Windlord with Serenity'),
(10, 269, 51, 60, '22098,22099,22107,22108,23167,23168,23169', NULL, 'Windwalker Monk 51-60: Strike of the Windlord with Serenity'),
(10, 269, 61, 70, '22098,22099,22107,22108,23167,23168,23169', NULL, 'Windwalker Monk 61-70: Strike of the Windlord with Serenity'),
(10, 269, 71, 80, '22098,22099,22107,22108,23167,23168,23169', '', 'Windwalker Monk 71-80: Raid build (Conduit of the Celestials/Shado-Pan)');

-- Mistweaver (Spec ID: 270) - ❌ NO DATA (Healer spec - requires manual curation)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(10, 270, 1, 10, '', NULL, 'Mistweaver Monk 1-10: Requires manual talent curation'),
(10, 270, 11, 20, '', NULL, 'Mistweaver Monk 11-20: Requires manual talent curation'),
(10, 270, 21, 30, '', NULL, 'Mistweaver Monk 21-30: Requires manual talent curation'),
(10, 270, 31, 40, '', NULL, 'Mistweaver Monk 31-40: Requires manual talent curation'),
(10, 270, 41, 50, '', NULL, 'Mistweaver Monk 41-50: Requires manual talent curation'),
(10, 270, 51, 60, '', NULL, 'Mistweaver Monk 51-60: Requires manual talent curation'),
(10, 270, 61, 70, '', NULL, 'Mistweaver Monk 61-70: Requires manual talent curation'),
(10, 270, 71, 80, '', '', 'Mistweaver Monk 71-80: Requires manual talent curation (Conduit of the Celestials/Master of Harmony)');

-- ====================================================================
-- DRUID (Class ID: 11)
-- ====================================================================

-- Balance (Spec ID: 102) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(11, 102, 1, 10, '22366,22367,22370,22373,23073,23074,23075', NULL, 'Balance Druid 1-10: Astral Power focused with Incarnation'),
(11, 102, 11, 20, '22366,22367,22370,22373,23073,23074,23075', NULL, 'Balance Druid 11-20: Astral Power focused with Incarnation'),
(11, 102, 21, 30, '22366,22367,22370,22373,23073,23074,23075', NULL, 'Balance Druid 21-30: Astral Power focused with Incarnation'),
(11, 102, 31, 40, '22366,22367,22370,22373,23073,23074,23075', NULL, 'Balance Druid 31-40: Astral Power focused with Incarnation'),
(11, 102, 41, 50, '22366,22367,22370,22373,23073,23074,23075', NULL, 'Balance Druid 41-50: Astral Power focused with Incarnation'),
(11, 102, 51, 60, '22366,22367,22370,22373,23073,23074,23075', NULL, 'Balance Druid 51-60: Astral Power focused with Incarnation'),
(11, 102, 61, 70, '22366,22367,22370,22373,23073,23074,23075', NULL, 'Balance Druid 61-70: Astral Power focused with Incarnation'),
(11, 102, 71, 80, '22366,22367,22370,22373,23073,23074,23075', '', 'Balance Druid 71-80: Raid build (Elune''s Chosen/Keeper of the Grove)');

-- Feral (Spec ID: 103) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(11, 103, 1, 10, '22363,22364,22368,22369,23069,23070,23071', NULL, 'Feral Druid 1-10: Bleed-focused with Berserk'),
(11, 103, 11, 20, '22363,22364,22368,22369,23069,23070,23071', NULL, 'Feral Druid 11-20: Bleed-focused with Berserk'),
(11, 103, 21, 30, '22363,22364,22368,22369,23069,23070,23071', NULL, 'Feral Druid 21-30: Bleed-focused with Berserk'),
(11, 103, 31, 40, '22363,22364,22368,22369,23069,23070,23071', NULL, 'Feral Druid 31-40: Bleed-focused with Berserk'),
(11, 103, 41, 50, '22363,22364,22368,22369,23069,23070,23071', NULL, 'Feral Druid 41-50: Bleed-focused with Berserk'),
(11, 103, 51, 60, '22363,22364,22368,22369,23069,23070,23071', NULL, 'Feral Druid 51-60: Bleed-focused with Berserk'),
(11, 103, 61, 70, '22363,22364,22368,22369,23069,23070,23071', NULL, 'Feral Druid 61-70: Bleed-focused with Berserk'),
(11, 103, 71, 80, '22363,22364,22368,22369,23069,23070,23071', '', 'Feral Druid 71-80: Raid build (Druid of the Claw/Wildstalker)');

-- Guardian (Spec ID: 104) - ❌ NO DATA (Tank spec - requires manual curation)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(11, 104, 1, 10, '', NULL, 'Guardian Druid 1-10: Requires manual talent curation'),
(11, 104, 11, 20, '', NULL, 'Guardian Druid 11-20: Requires manual talent curation'),
(11, 104, 21, 30, '', NULL, 'Guardian Druid 21-30: Requires manual talent curation'),
(11, 104, 31, 40, '', NULL, 'Guardian Druid 31-40: Requires manual talent curation'),
(11, 104, 41, 50, '', NULL, 'Guardian Druid 41-50: Requires manual talent curation'),
(11, 104, 51, 60, '', NULL, 'Guardian Druid 51-60: Requires manual talent curation'),
(11, 104, 61, 70, '', NULL, 'Guardian Druid 61-70: Requires manual talent curation'),
(11, 104, 71, 80, '', '', 'Guardian Druid 71-80: Requires manual talent curation (Druid of the Claw/Elune''s Chosen)');

-- Restoration (Spec ID: 105) - ❌ NO DATA (Healer spec - requires manual curation)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(11, 105, 1, 10, '', NULL, 'Restoration Druid 1-10: Requires manual talent curation'),
(11, 105, 11, 20, '', NULL, 'Restoration Druid 11-20: Requires manual talent curation'),
(11, 105, 21, 30, '', NULL, 'Restoration Druid 21-30: Requires manual talent curation'),
(11, 105, 31, 40, '', NULL, 'Restoration Druid 31-40: Requires manual talent curation'),
(11, 105, 41, 50, '', NULL, 'Restoration Druid 41-50: Requires manual talent curation'),
(11, 105, 51, 60, '', NULL, 'Restoration Druid 51-60: Requires manual talent curation'),
(11, 105, 61, 70, '', NULL, 'Restoration Druid 61-70: Requires manual talent curation'),
(11, 105, 71, 80, '', '', 'Restoration Druid 71-80: Requires manual talent curation (Keeper of the Grove/Wildstalker)');

-- ====================================================================
-- DEMON HUNTER (Class ID: 12)
-- ====================================================================

-- Havoc (Spec ID: 577) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(12, 577, 1, 10, '21854,21855,21862,21863,22493,22494,22495', NULL, 'Havoc Demon Hunter 1-10: Fel Barrage with Momentum'),
(12, 577, 11, 20, '21854,21855,21862,21863,22493,22494,22495', NULL, 'Havoc Demon Hunter 11-20: Fel Barrage with Momentum'),
(12, 577, 21, 30, '21854,21855,21862,21863,22493,22494,22495', NULL, 'Havoc Demon Hunter 21-30: Fel Barrage with Momentum'),
(12, 577, 31, 40, '21854,21855,21862,21863,22493,22494,22495', NULL, 'Havoc Demon Hunter 31-40: Fel Barrage with Momentum'),
(12, 577, 41, 50, '21854,21855,21862,21863,22493,22494,22495', NULL, 'Havoc Demon Hunter 41-50: Fel Barrage with Momentum'),
(12, 577, 51, 60, '21854,21855,21862,21863,22493,22494,22495', NULL, 'Havoc Demon Hunter 51-60: Fel Barrage with Momentum'),
(12, 577, 61, 70, '21854,21855,21862,21863,22493,22494,22495', NULL, 'Havoc Demon Hunter 61-70: Fel Barrage with Momentum'),
(12, 577, 71, 80, '21854,21855,21862,21863,22493,22494,22495', '', 'Havoc Demon Hunter 71-80: Raid build (Aldrachi Reaver/Fel-Scarred)');

-- Vengeance (Spec ID: 581) - ❌ NO DATA (Tank spec - requires manual curation)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(12, 581, 1, 10, '', NULL, 'Vengeance Demon Hunter 1-10: Requires manual talent curation'),
(12, 581, 11, 20, '', NULL, 'Vengeance Demon Hunter 11-20: Requires manual talent curation'),
(12, 581, 21, 30, '', NULL, 'Vengeance Demon Hunter 21-30: Requires manual talent curation'),
(12, 581, 31, 40, '', NULL, 'Vengeance Demon Hunter 31-40: Requires manual talent curation'),
(12, 581, 41, 50, '', NULL, 'Vengeance Demon Hunter 41-50: Requires manual talent curation'),
(12, 581, 51, 60, '', NULL, 'Vengeance Demon Hunter 51-60: Requires manual talent curation'),
(12, 581, 61, 70, '', NULL, 'Vengeance Demon Hunter 61-70: Requires manual talent curation'),
(12, 581, 71, 80, '', '', 'Vengeance Demon Hunter 71-80: Requires manual talent curation (Aldrachi Reaver/Fel-Scarred)');

-- ====================================================================
-- EVOKER (Class ID: 13)
-- ====================================================================

-- Devastation (Spec ID: 1467) - ✅ POPULATED
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(13, 1467, 1, 10, '23506,23507,23508,23509,23550,23551,23552', NULL, 'Devastation Evoker 1-10: Dragonrage burst with Fire Breath'),
(13, 1467, 11, 20, '23506,23507,23508,23509,23550,23551,23552', NULL, 'Devastation Evoker 11-20: Dragonrage burst with Fire Breath'),
(13, 1467, 21, 30, '23506,23507,23508,23509,23550,23551,23552', NULL, 'Devastation Evoker 21-30: Dragonrage burst with Fire Breath'),
(13, 1467, 31, 40, '23506,23507,23508,23509,23550,23551,23552', NULL, 'Devastation Evoker 31-40: Dragonrage burst with Fire Breath'),
(13, 1467, 41, 50, '23506,23507,23508,23509,23550,23551,23552', NULL, 'Devastation Evoker 41-50: Dragonrage burst with Fire Breath'),
(13, 1467, 51, 60, '23506,23507,23508,23509,23550,23551,23552', NULL, 'Devastation Evoker 51-60: Dragonrage burst with Fire Breath'),
(13, 1467, 61, 70, '23506,23507,23508,23509,23550,23551,23552', NULL, 'Devastation Evoker 61-70: Dragonrage burst with Fire Breath'),
(13, 1467, 71, 80, '23506,23507,23508,23509,23550,23551,23552', '', 'Devastation Evoker 71-80: Raid build (Chronowarden/Flameshaper)');

-- Preservation (Spec ID: 1468) - ❌ NO DATA (Healer spec - requires manual curation)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(13, 1468, 1, 10, '', NULL, 'Preservation Evoker 1-10: Requires manual talent curation'),
(13, 1468, 11, 20, '', NULL, 'Preservation Evoker 11-20: Requires manual talent curation'),
(13, 1468, 21, 30, '', NULL, 'Preservation Evoker 21-30: Requires manual talent curation'),
(13, 1468, 31, 40, '', NULL, 'Preservation Evoker 31-40: Requires manual talent curation'),
(13, 1468, 41, 50, '', NULL, 'Preservation Evoker 41-50: Requires manual talent curation'),
(13, 1468, 51, 60, '', NULL, 'Preservation Evoker 51-60: Requires manual talent curation'),
(13, 1468, 61, 70, '', NULL, 'Preservation Evoker 61-70: Requires manual talent curation'),
(13, 1468, 71, 80, '', '', 'Preservation Evoker 71-80: Requires manual talent curation (Chronowarden/Flameshaper)');

-- Augmentation (Spec ID: 1473) - ❌ NO DATA (Support spec - requires manual curation)
INSERT INTO `playerbot_talent_loadouts` (`class_id`, `spec_id`, `min_level`, `max_level`, `talent_string`, `hero_talent_string`, `description`) VALUES
(13, 1473, 1, 10, '', NULL, 'Augmentation Evoker 1-10: Requires manual talent curation'),
(13, 1473, 11, 20, '', NULL, 'Augmentation Evoker 11-20: Requires manual talent curation'),
(13, 1473, 21, 30, '', NULL, 'Augmentation Evoker 21-30: Requires manual talent curation'),
(13, 1473, 31, 40, '', NULL, 'Augmentation Evoker 31-40: Requires manual talent curation'),
(13, 1473, 41, 50, '', NULL, 'Augmentation Evoker 41-50: Requires manual talent curation'),
(13, 1473, 51, 60, '', NULL, 'Augmentation Evoker 51-60: Requires manual talent curation'),
(13, 1473, 61, 70, '', NULL, 'Augmentation Evoker 61-70: Requires manual talent curation'),
(13, 1473, 71, 80, '', '', 'Augmentation Evoker 71-80: Requires manual talent curation (Chronowarden/Scalecommander)');

-- ====================================================================
-- END OF INSERTIONS
-- ====================================================================

-- ====================================================================
-- SUMMARY STATISTICS
-- ====================================================================
-- Total Entries: 312 (39 specs × 8 level brackets)
-- Populated Specs (22): Arms, Fury, Retribution, Beast Mastery, Marksmanship,
--   Assassination, Outlaw, Shadow, Frost DK, Unholy, Elemental, Enhancement,
--   Arcane, Fire, Frost Mage, Affliction, Destruction, Windwalker, Balance,
--   Feral, Havoc, Devastation
-- Unpopulated Specs (17): All tank and healer specs (requires manual curation)
-- ====================================================================

-- ====================================================================
-- MANUAL CURATION REQUIRED FOR:
-- ====================================================================
-- TANKS (6): Protection Warrior, Protection Paladin, Blood DK,
--            Brewmaster Monk, Guardian Druid, Vengeance DH
-- HEALERS (6): Holy Paladin, Holy Priest, Discipline Priest,
--             Restoration Shaman, Mistweaver Monk, Restoration Druid,
--             Preservation Evoker
-- OTHER (5): Survival Hunter, Subtlety Rogue, Demonology Warlock,
--            Augmentation Evoker
-- ====================================================================
-- Recommended sources for manual curation:
-- 1. Icy-Veins class guides (icy-veins.com)
-- 2. Wowhead talent calculators (wowhead.com/talent-calc)
-- 3. Method.gg talent builds
-- 4. Player character database exports
-- ====================================================================
