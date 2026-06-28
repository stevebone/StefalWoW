-- Followship Database Base Schema
-- Creates the followship database with standard TrinityCore update tracking tables
-- and the bot_owners table (moved from characters database, renamed from followship_bot_owners)

CREATE TABLE `updates` (
  `name` varchar(200) NOT NULL COMMENT 'filename with extension of the update.',
  `hash` char(40) DEFAULT '' COMMENT 'sha1 hash of the sql file.',
  `state` enum('RELEASED','CUSTOM','MODULE') NOT NULL DEFAULT 'RELEASED' COMMENT 'defines if an update is released or archived.',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'timestamp when the query was applied.',
  `speed` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'time the query takes to apply in ms.',
  PRIMARY KEY (`name`) USING BTREE,
  UNIQUE KEY `unique_index` (`name`,`hash`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='List of all applied updates in this database.';

CREATE TABLE `updates_include` (
  `path` varchar(200) NOT NULL COMMENT 'directory to include. $ means relative to the source directory.',
  `state` enum('RELEASED','CUSTOM','MODULE') NOT NULL DEFAULT 'RELEASED' COMMENT 'defines if the directory contains released or archived updates.',
  PRIMARY KEY (`path`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='List of directories containing update files.';

INSERT INTO `updates_include` (`path`, `state`) VALUES
('$/sql/updates/followship', 'RELEASED');

CREATE TABLE `bot_owners` (
  `bot_id` INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  `bot_guid` BIGINT UNSIGNED NOT NULL,
  `bot_entry` INT UNSIGNED NOT NULL,
  `player_guid` BIGINT UNSIGNED NOT NULL,
  `hire_expiry_time` BIGINT UNSIGNED NOT NULL,

  UNIQUE KEY `uq_bot_player` (`bot_guid`, `player_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE `bot_templates` (
  `entry` INT UNSIGNED NOT NULL PRIMARY KEY,
  `bot_class` TINYINT UNSIGNED NOT NULL,
  `bot_race` TINYINT UNSIGNED NOT NULL,
  `companion_spell` INT UNSIGNED NOT NULL DEFAULT 0,
  `chatter_type` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `gender` TINYINT UNSIGNED NOT NULL DEFAULT 2,
  `pet_source` INT UNSIGNED NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Bot template definitions (moved from hardcoded BotEntryClassTable)';

INSERT INTO `bot_templates` (`entry`, `bot_class`, `bot_race`, `companion_spell`, `chatter_type`, `gender`, `pet_source`) VALUES
-- Priests
(141508, 2, 1, 0, 1, 2, 0),
(375, 2, 1, 10675, 2, 1, 0),
(376, 2, 1, 0, 2, 1, 0),
(377, 2, 1, 70613, 2, 1, 0),
(837, 2, 2, 0, 3, 0, 0),
(3595, 2, 3, 0, 2, 1, 0),
(5484, 2, 1, 0, 1, 0, 0),
(5489, 2, 1, 0, 2, 0, 0),
(16502, 2, 5, 0, 1, 0, 0),
(49749, 2, 1, 0, 2, 1, 0),
-- Warriors
(911, 1, 1, 0, 2, 0, 0),
(912, 1, 2, 0, 1, 0, 0),
(913, 1, 1, 171118, 2, 1, 0),
(914, 1, 1, 0, 2, 0, 0),
(3593, 1, 3, 15067, 2, 1, 0),
(16503, 1, 5, 0, 1, 0, 0),
(46405, 1, 1, 0, 3, 0, 0),
(49736, 1, 1, 0, 3, 0, 0),
(50595, 1, 1, 0, 3, 2, 0),
(112912, 1, 1, 0, 1, 0, 0),
(164939, 1, 2, 0, 3, 1, 0),
(164940, 1, 7, 0, 1, 0, 0),
-- Mages
(198, 3, 1, 10697, 1, 0, 0),
(328, 3, 1, 10696, 1, 0, 0),
(16500, 3, 5, 0, 3, 1, 0),
(37121, 3, 2, 0, 1, 1, 0),
(43006, 3, 3, 0, 2, 1, 0),
(49760, 3, 1, 1263739, 2, 1, 0),
-- Paladins
(925, 6, 1, 0, 3, 0, 0),
(926, 6, 2, 0, 1, 0, 0),
(927, 6, 1, 0, 1, 0, 0),
(928, 6, 1, 0, 2, 0, 0),
(5491, 6, 1, 0, 3, 0, 0),
(5492, 6, 1, 0, 2, 1, 0),
(16501, 6, 5, 408311, 2, 0, 0),
(49741, 6, 1, 1269627, 2, 1, 0),
-- Warlocks
(459, 8, 1, 153252, 2, 1, 0),
(461, 8, 1, 0, 3, 1, 0),
(906, 8, 1, 0, 2, 0, 0),
(5495, 8, 1, 0, 3, 1, 0),
(6373, 8, 1, 0, 2, 0, 0),
(6374, 8, 1, 0, 1, 1, 0),
(49769, 8, 1, 0, 1, 1, 0),
-- Druids
(3597, 5, 3, 15067, 1, 0, 0),
(5506, 5, 3, 0, 1, 0, 0),
-- Rogues
(915, 4, 1, 0, 1, 0, 0),
(917, 4, 1, 0, 3, 1, 0),
(918, 4, 1, 0, 1, 0, 0),
(5505, 4, 3, 0, 3, 0, 0),
(13283, 4, 1, 0, 1, 0, 0),
(49745, 4, 1, 0, 2, 0, 0),
(173819, 4, 1, 0, 3, 1, 0),
-- Hunters
(895, 7, 2, 0, 3, 0, 1125),
(5516, 7, 2, 0, 1, 0, 191979),
(43011, 7, 1, 0, 3, 1, 44628),
(43278, 7, 1, 0, 3, 1, 43292),
(46983, 7, 1, 0, 2, 0, 46982),
-- Monks
(63258, 10, 7, 17707, 2, 0, 0),
(63331, 10, 7, 0, 1, 0, 0),
(63285, 10, 7, 345740, 2, 0, 0),
(63235, 10, 7, 0, 1, 0, 0),
-- Shamans
(17089, 9, 5, 0, 3, 0, 0),
(20407, 9, 5, 0, 2, 1, 0);
