-- ============================================================================
-- WORLD DATABASE TABLES - Perks and Delves
-- ============================================================================

-- Perks Program: Vendor items
CREATE TABLE IF NOT EXISTS `perks_vendor_items` (
  `VendorItemID` INT NOT NULL COMMENT 'Unique vendor item ID in the perks system',
  `MountID` INT NOT NULL DEFAULT '0' COMMENT 'Mount ID (0 if not a mount)',
  `BattlePetSpeciesID` INT NOT NULL DEFAULT '0' COMMENT 'Battle pet species ID (0 if N/A)',
  `TransmogSetID` INT NOT NULL DEFAULT '0' COMMENT 'Transmog set ID (0 if N/A)',
  `ItemModifiedAppearanceID` INT NOT NULL DEFAULT '0' COMMENT 'Item modified appearance ID (0 if N/A)',
  `TransmogIllusionID` INT NOT NULL DEFAULT '0' COMMENT 'Transmog illusion ID (0 if N/A)',
  `ToyID` INT NOT NULL DEFAULT '0' COMMENT 'Toy ID (0 if N/A)',
  `WarbandSceneID` INT NOT NULL DEFAULT '0' COMMENT 'Warband scene ID (0 if N/A)',
  `Price` INT NOT NULL DEFAULT '0' COMMENT 'Cost in Traders Tender',
  `OriginalPrice` INT NOT NULL DEFAULT '0' COMMENT 'Pre-discount price (0 = no discount)',
  `AvailableUntil` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Unix timestamp expiry',
  `Disabled` TINYINT UNSIGNED NOT NULL DEFAULT '0' COMMENT '1 = item disabled',
  `DoesNotExpire` TINYINT UNSIGNED NOT NULL DEFAULT '0' COMMENT '1 = permanent item',
  PRIMARY KEY (`VendorItemID`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4 COMMENT='Perks Program vendor item definitions';

DELETE FROM `perks_vendor_items` WHERE `VendorItemID` IN (947, 1000, 1306, 1316, 1322, 1331, 1354, 1361);
INSERT INTO `perks_vendor_items` (`VendorItemID`, `MountID`, `BattlePetSpeciesID`, `TransmogSetID`, `ItemModifiedAppearanceID`, `TransmogIllusionID`, `ToyID`, `WarbandSceneID`, `Price`, `OriginalPrice`, `AvailableUntil`, `Disabled`, `DoesNotExpire`) VALUES 
(947, 0, 0, 0, 249023, 0, 0, 0, 200, 0, 1775055600, 0, 0),
(1000, 1218012, 0, 0, 0, 0, 0, 0, 700, 0, 1775055600, 0, 0),
(1306, 1270523, 0, 0, 0, 0, 0, 0, 500, 0, 1775055600, 0, 0),
(1316, 0, 0, 0, 302232, 0, 0, 0, 100, 0, 1775055600, 0, 0),
(1322, 0, 0, 0, 304086, 0, 0, 0, 80, 0, 1775055600, 0, 0),
(1331, 0, 0, 5355, 0, 0, 0, 0, 150, 0, 1775055600, 0, 0),
(1354, 0, 0, 0, 304226, 0, 0, 0, 80, 0, 1775055600, 0, 0),
(1361, 0, 0, 0, 304091, 0, 0, 0, 80, 0, 1775055600, 0, 0);

-- Perks Program: Monthly rotation
CREATE TABLE IF NOT EXISTS `perks_monthly_rotation` (
  `rotation_id` INT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'Primary key',
  `month_start` INT UNSIGNED NOT NULL COMMENT 'Unix timestamp: rotation start',
  `month_end` INT UNSIGNED NOT NULL COMMENT 'Unix timestamp: rotation end',
  `vendor_item_id` INT NOT NULL COMMENT 'FK to perks_vendor_items.VendorItemID',
  `comment` VARCHAR(255) DEFAULT NULL COMMENT 'Human-readable label (e.g. March 2026)',
  PRIMARY KEY (`rotation_id`),
  KEY `idx_rotation_dates` (`month_start`, `month_end`),
  KEY `idx_vendor_item` (`vendor_item_id`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4 COMMENT='Perks Program monthly item rotation schedule';

INSERT INTO `perks_monthly_rotation` (`month_start`, `month_end`, `vendor_item_id`, `comment`) VALUES
(1772377200, 1775055600, 1316, 'March 2026 - Transmog Appearance'),
(1772377200, 1775055600, 1000, 'March 2026 - Mount'),
(1772377200, 1775055600, 1322, 'March 2026 - Transmog Set'),
(1772377200, 1775055600, 947,  'March 2026 - Unknown Item 947'),
(1772377200, 1775055600, 1331, 'March 2026 - Unknown Item 1331'),
(1772377200, 1775055600, 1306, 'March 2026 - Mount (Freezable)');

-- Perks Program: Activity intervals
CREATE TABLE IF NOT EXISTS `perks_activity_intervals` (
  `interval_id`  INT UNSIGNED NOT NULL COMMENT 'IntervalID from PerksActivityXInterval.db2',
  `month_start`  INT UNSIGNED NOT NULL COMMENT 'Unix timestamp: window open (0 = always)',
  `month_end`    INT UNSIGNED NOT NULL COMMENT 'Unix timestamp: window close (INT_MAX = always)',
  `is_threshold` TINYINT UNSIGNED NOT NULL DEFAULT 0
                 COMMENT '0 = main activity list, 1 = trailing milestone slots',
  `comment`      VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`interval_id`, `month_start`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4 COMMENT='Perks Program activity interval schedule';

INSERT INTO `perks_activity_intervals` (`interval_id`, `month_start`, `month_end`, `is_threshold`, `comment`) VALUES
(4,   0, 2147483647, 0, 'Permanent interval 4'),
(5,   0, 2147483647, 0, 'Permanent interval 5'),
(8,   0, 2147483647, 0, 'Permanent interval 8'),
(37,  0, 2147483647, 0, 'Permanent interval 37'),
(110, 0, 2147483647, 0, 'Permanent interval 110'),
(111, 1740787200, 1743465600, 1, 'March 2026 threshold milestones (IntervalID 111)'),
(112, 1740787200, 1743465600, 1, 'March 2026 threshold milestones (IntervalID 112)'),
(113, 1740787200, 1743465600, 1, 'March 2026 threshold milestones (IntervalID 113)')
ON DUPLICATE KEY UPDATE `month_end` = VALUES(`month_end`), `comment` = VALUES(`comment`);

-- Perks Program: Current activities
CREATE TABLE IF NOT EXISTS `perks_current_activities` (
  `activity_id`  INT NOT NULL COMMENT 'PerksActivity ID from PerksActivity.db2',
  `is_threshold` TINYINT UNSIGNED NOT NULL DEFAULT '0'
                 COMMENT '0 = main list, 1 = threshold milestone slot',
  `month_start`  INT UNSIGNED NOT NULL COMMENT 'Unix timestamp: window open',
  `month_end`    INT UNSIGNED NOT NULL COMMENT 'Unix timestamp: window close',
  PRIMARY KEY (`activity_id`)
) ENGINE=INNODB DEFAULT CHARSET=utf8mb4 COMMENT='Perks Program direct activity list per Trading Post month';

INSERT INTO `perks_current_activities` (`activity_id`, `is_threshold`, `month_start`, `month_end`) VALUES
(6,   0, 1772377200, 1775055600), (10,  0, 1772377200, 1775055600), (11,  0, 1772377200, 1775055600),
(12,  0, 1772377200, 1775055600), (13,  0, 1772377200, 1775055600), (14,  0, 1772377200, 1775055600),
(61,  0, 1772377200, 1775055600), (65,  0, 1772377200, 1775055600), (66,  0, 1772377200, 1775055600),
(67,  0, 1772377200, 1775055600), (69,  0, 1772377200, 1775055600), (70,  0, 1772377200, 1775055600),
(73,  0, 1772377200, 1775055600), (266, 0, 1772377200, 1775055600), (267, 0, 1772377200, 1775055600),
(268, 0, 1772377200, 1775055600), (269, 0, 1772377200, 1775055600), (270, 0, 1772377200, 1775055600),
(271, 0, 1772377200, 1775055600), (312, 0, 1772377200, 1775055600), (313, 0, 1772377200, 1775055600),
(323, 0, 1772377200, 1775055600), (380, 0, 1772377200, 1775055600), (415, 0, 1772377200, 1775055600),
(416, 0, 1772377200, 1775055600), (417, 0, 1772377200, 1775055600), (418, 0, 1772377200, 1775055600),
(419, 0, 1772377200, 1775055600), (420, 0, 1772377200, 1775055600), (421, 0, 1772377200, 1775055600),
(422, 0, 1772377200, 1775055600), (423, 0, 1772377200, 1775055600), (435, 0, 1772377200, 1775055600),
(441, 0, 1772377200, 1775055600), (446, 0, 1772377200, 1775055600), (478, 0, 1772377200, 1775055600),
(479, 0, 1772377200, 1775055600), (501, 0, 1772377200, 1775055600), (502, 0, 1772377200, 1775055600),
(503, 0, 1772377200, 1775055600), (540, 0, 1772377200, 1775055600), (541, 0, 1772377200, 1775055600),
(542, 0, 1772377200, 1775055600), (543, 0, 1772377200, 1775055600), (588, 0, 1772377200, 1775055600),
(606, 0, 1772377200, 1775055600), (607, 0, 1772377200, 1775055600), (608, 0, 1772377200, 1775055600),
(609, 0, 1772377200, 1775055600), (645, 0, 1772377200, 1775055600), (646, 0, 1772377200, 1775055600),
(670, 0, 1772377200, 1775055600), (683, 0, 1772377200, 1775055600), (684, 0, 1772377200, 1775055600),
(782, 0, 1772377200, 1775055600), (806, 0, 1772377200, 1775055600), (809, 0, 1772377200, 1775055600),
(852, 0, 1772377200, 1775055600), (889, 0, 1772377200, 1775055600), (890, 0, 1772377200, 1775055600),
(891, 0, 1772377200, 1775055600), (892, 0, 1772377200, 1775055600), (893, 0, 1772377200, 1775055600),
(905, 0, 1772377200, 1775055600), (913, 0, 1772377200, 1775055600), (914, 0, 1772377200, 1775055600),
(917, 0, 1772377200, 1775055600), (920, 0, 1772377200, 1775055600), (921, 0, 1772377200, 1775055600),
(922, 1, 1772377200, 1775055600), (924, 1, 1772377200, 1775055600), (927, 1, 1772377200, 1775055600),
(928, 1, 1772377200, 1775055600), (934, 1, 1772377200, 1775055600)
ON DUPLICATE KEY UPDATE `month_start` = VALUES(`month_start`), `month_end` = VALUES(`month_end`);