--
-- WoW Token holdings and token market listings (account level, queried at character select).
DROP TABLE IF EXISTS `account_wow_token`;
CREATE TABLE `account_wow_token` (
  `id` bigint unsigned NOT NULL COMMENT 'Token id sent to the client',
  `account` int unsigned NOT NULL DEFAULT '0' COMMENT 'Owning account, 0 while listed on the market',
  `state` tinyint unsigned NOT NULL DEFAULT '0' COMMENT '0 Auctionable, 1 Consumable, 2 Listed on the market',
  `price` bigint unsigned NOT NULL DEFAULT '0' COMMENT 'Copper, only meaningful while state = 2',
  `createTime` bigint NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `idx_account` (`account`),
  KEY `idx_state` (`state`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='WoW Token holdings and market listings';
