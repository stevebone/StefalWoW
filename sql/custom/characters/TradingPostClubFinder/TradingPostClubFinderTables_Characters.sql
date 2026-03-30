-- ============================================================================
-- CHARACTER DATABASE TABLES - Perks Program
-- ============================================================================

CREATE TABLE IF NOT EXISTS `character_perks_currency` (
  `guid` BIGINT UNSIGNED NOT NULL COMMENT 'Character GUID',
  `currency` INT NOT NULL DEFAULT '0' COMMENT 'Current Traders Tender balance',
  `total_earned` INT NOT NULL DEFAULT '0' COMMENT 'Total Tender ever earned',
  `purchased_count` INT NOT NULL DEFAULT '0' COMMENT 'Total items purchased',
  PRIMARY KEY (`guid`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4 COMMENT='Perks Program player currency';

-- Player purchase history
CREATE TABLE IF NOT EXISTS `character_perks_purchases` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'Primary key',
  `guid` BIGINT UNSIGNED NOT NULL COMMENT 'Character GUID',
  `vendor_item_id` INT NOT NULL COMMENT 'Purchased VendorItemID',
  `purchase_time` INT UNSIGNED NOT NULL COMMENT 'Unix timestamp of purchase',
  `refundable` TINYINT UNSIGNED NOT NULL DEFAULT '1' COMMENT '1 = can still be refunded',
  PRIMARY KEY (`id`),
  KEY `idx_guid` (`guid`),
  KEY `idx_guid_vendor` (`guid`, `vendor_item_id`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4 COMMENT='Perks Program purchase history';

CREATE TABLE IF NOT EXISTS `character_perks_frozen` (
  `guid` BIGINT UNSIGNED NOT NULL COMMENT 'Character GUID',
  `vendor_item_id` INT NOT NULL COMMENT 'Frozen VendorItemID',
  PRIMARY KEY (`guid`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4 COMMENT='Perks Program frozen vendor item per character';


CREATE TABLE IF NOT EXISTS `character_perks_completed_milestones` (
  `guid`           BIGINT UNSIGNED NOT NULL COMMENT 'Character GUID',
  `activity_id`    INT NOT NULL COMMENT 'PerksActivity ID (from rotation-specific XInterval)',
  `completed_time` INT UNSIGNED NOT NULL COMMENT 'Unix timestamp when completed',
  PRIMARY KEY (`guid`, `activity_id`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4
  COMMENT='Perks Program completed threshold milestone activities per player';


-- Club Finder tables for guild recruitment system

DROP TABLE IF EXISTS `club_finder_applicant`;
DROP TABLE IF EXISTS `club_finder_post`;

CREATE TABLE `club_finder_post` (
    `postingID`         BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `guildID`           BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `description`       TEXT            NOT NULL,
    `playstyle`         INT UNSIGNED    NOT NULL DEFAULT 0,
    `interests`         INT UNSIGNED    NOT NULL DEFAULT 0,
    `specIDs`           VARCHAR(256)    NOT NULL DEFAULT '' COMMENT 'Comma-separated spec IDs being recruited',
    `classMask`         INT UNSIGNED    NOT NULL DEFAULT 0,
    `minLevel`          TINYINT UNSIGNED NOT NULL DEFAULT 10,
    `maxLevel`          TINYINT UNSIGNED NOT NULL DEFAULT 80,
    `slotsAvailable`    INT UNSIGNED    NOT NULL DEFAULT 0,
    `maxApplicants`     INT UNSIGNED    NOT NULL DEFAULT 0xFFFFFFFF,
    `language`          INT UNSIGNED    NOT NULL DEFAULT 0,
    `status`            TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=Active, 1=Closed',
    `timestamp`         INT UNSIGNED    NOT NULL DEFAULT 0,
    PRIMARY KEY (`postingID`),
    KEY `idx_guildID` (`guildID`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `club_finder_applicant` (
    `id`                BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `postingID`         BIGINT UNSIGNED NOT NULL,
    `playerGUID`        BIGINT UNSIGNED NOT NULL,
    `status`            TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=Pending, 1=Accepted, 2=Declined',
    `comment`           TEXT            NOT NULL,
    `timestamp`         INT UNSIGNED    NOT NULL DEFAULT 0,
    PRIMARY KEY (`id`),
    KEY `idx_postingID` (`postingID`),
    KEY `idx_playerGUID` (`playerGUID`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
