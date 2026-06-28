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
