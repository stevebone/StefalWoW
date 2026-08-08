--
-- In-game Shop / BattlePay purchase ledger (account level). Shared home for BOTH the WoW Token branch
-- and the in-game Shop branch: it answers CMSG_BATTLE_PAY_GET_PURCHASE_LIST from real history and gives
-- purchases a PurchaseID that survives restarts. The PurchaseID (`id`) is allocated with the realm id in
-- its high 32 bits so two realms sharing this auth DB never collide.
DROP TABLE IF EXISTS `account_battlepay_purchase`;
CREATE TABLE `account_battlepay_purchase` (
  `id` bigint unsigned NOT NULL COMMENT 'Persistent monotonic PurchaseID sent on the wire; high 32 bits = realm id',
  `account` int unsigned NOT NULL DEFAULT '0' COMMENT 'Owning game account',
  `productId` int unsigned NOT NULL DEFAULT '0' COMMENT '0 is a VALID value (e.g. web-checkout purchases)',
  `status` int NOT NULL DEFAULT '0' COMMENT 'BattlepayPurchaseStatus: 6 = done, 4 = failed',
  `resultCode` int NOT NULL DEFAULT '0' COMMENT 'PurchaseResult: 0 = ok',
  `basePrice` bigint unsigned NOT NULL DEFAULT '0' COMMENT 'Copper',
  `userPrice` bigint unsigned NOT NULL DEFAULT '0' COMMENT 'Copper',
  `timeCreated` bigint NOT NULL DEFAULT '0' COMMENT 'Unix seconds',
  `walletName` varchar(32) NOT NULL DEFAULT '' COMMENT 'Wallet label; empty on this core, sent record-final',
  PRIMARY KEY (`id`),
  KEY `idx_account` (`account`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='In-game Shop / BattlePay purchase ledger';
