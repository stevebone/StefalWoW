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


-- NOTE: the old simplified Club Finder tables (`club_finder_post` / `club_finder_applicant`)
-- were removed - superseded by the SocialClub schema in
-- sql/custom/characters/SocialClub/SocialClub_characters.sql.
