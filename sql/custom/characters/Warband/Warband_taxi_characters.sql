CREATE TABLE IF NOT EXISTS `warband_taxi_mask` (
  `battlenetAccountId` int unsigned NOT NULL,
  `taximask` text NOT NULL,
  PRIMARY KEY (`battlenetAccountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;