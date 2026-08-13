
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

--
-- In-game Shop catalog administration RBAC permissions.
--   886 = Command: reload shop_catalog  (.reload shop_catalog)
--   887 = Command: shop                 (.shop list/enable/disable/price/window/feature/preview)
-- Linked into the same groups as the neighbouring reload / GM command permissions so the default
-- GM roles pick them up (196 = reload group, 197 = GM command group).

DELETE FROM `rbac_permissions` WHERE `id` IN (1001,1002);
INSERT INTO `rbac_permissions` (`id`,`name`) VALUES
(1001,'Command: reload shop_catalog'),
(1002,'Command: shop');

DELETE FROM `rbac_linked_permissions` WHERE `linkedId` IN (1001,1002);
INSERT INTO `rbac_linked_permissions` (`id`,`linkedId`) VALUES
(196,1001),
(197,1002);

--
-- In-game Shop / BattlePay entitlement ("distribution") store, account level.
--
-- An entitlement is a purchased-but-not-yet-applied product: the retail "own it now, apply it later"
-- model that backs buying at character select and every service product (boost, rename, faction/race
-- change, transfer) whose grant targets a character chosen AFTER the purchase.
--
-- It lives in the AUTH database for the same reason `account_battlepay_purchase` does: the Shop opens
-- at character select, where no character database context exists yet, and an entitlement is owned by
-- the ACCOUNT, not by a character or a realm. `id` doubles as the wire DistributionID and is allocated
-- with the realm id in its high 32 bits so two realms sharing this auth DB never collide, exactly like
-- the purchase ledger's PurchaseID.
--
-- Lifecycle (see BattlePayEntitlements in BattlePayMgr.h):
--   1 AVAILABLE - owned, unassigned. The only status ever sent to the client (the sole value observed
--                 on the wire, 68275 capture).
--   2 CLAIMED   - transient: an assign is in flight and has won the compare-and-swap on `claimToken`.
--   3 BOUND     - assigned to `targetCharacter` on `realmId`; the payload is delivered the next time
--                 that character logs in.
--   4 FINISHED  - delivered; terminal.
--   5 REVOKED   - withdrawn by an admin / refunded; terminal.
--
-- `claimToken` is what makes assignment safe against a replay or a second realm: the claiming session
-- writes a random token together with status 2 under a `status = 1` guard, then reads the row back and
-- proceeds only if ITS token survived. Two racing assigns can therefore never both succeed, without
-- needing an affected-rows count.
DROP TABLE IF EXISTS `account_battlepay_entitlement`;
CREATE TABLE `account_battlepay_entitlement` (
  `id` bigint unsigned NOT NULL COMMENT 'DistributionID sent on the wire; high 32 bits = realm id',
  `account` int unsigned NOT NULL DEFAULT '0' COMMENT 'Owning game account',
  `productId` int unsigned NOT NULL DEFAULT '0' COMMENT 'shop_product.productId this entitlement grants',
  `serviceType` tinyint unsigned NOT NULL DEFAULT '0' COMMENT '0 = deferred delivery of the product payload; else the VAS service type from shop_product_deliverable.id where type = 5',
  `status` tinyint unsigned NOT NULL DEFAULT '1' COMMENT '1 available, 2 claimed, 3 bound to a character, 4 finished, 5 revoked',
  `purchaseId` bigint unsigned NOT NULL DEFAULT '0' COMMENT 'account_battlepay_purchase.id that created this entitlement',
  `claimToken` bigint unsigned NOT NULL DEFAULT '0' COMMENT 'Compare-and-swap token held by the session currently assigning this entitlement',
  `realmId` int unsigned NOT NULL DEFAULT '0' COMMENT 'Realm the target character lives on; 0 while unassigned',
  `targetCharacter` bigint unsigned NOT NULL DEFAULT '0' COMMENT 'Character guid counter it was assigned to; 0 while unassigned',
  `createTime` bigint NOT NULL DEFAULT '0' COMMENT 'Unix seconds',
  `updateTime` bigint NOT NULL DEFAULT '0' COMMENT 'Unix seconds of the last status change',
  PRIMARY KEY (`id`),
  KEY `idx_account_status` (`account`, `status`),
  KEY `idx_pending_delivery` (`realmId`, `targetCharacter`, `status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='In-game Shop / BattlePay entitlements (distributions)';
