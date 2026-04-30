-- Account bank coinage (warband-shared coinage per bnet account)
CREATE TABLE IF NOT EXISTS `account_bank_coinage` (
  `battlenetAccountId` int unsigned NOT NULL,
  `coinage` bigint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`battlenetAccountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
