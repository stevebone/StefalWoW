-- In-game Shop: the WoW Token as a purchasable product (deliverable type 3).
--
-- Adapted from feature/wow-token af910e463e (TK-1), whose original row targeted the pre-SH-6
-- `battlepay_product` table. That table was migrated to the catalog-admin schema and DROPPED by
-- 2026_08_09_00_world.sql, so the token product is expressed here as a `shop_product` row plus a
-- `shop_product_deliverable` of type 3 (WoW Token). This is the retail acquisition path: a token is
-- bought from the Shop for gold, after which it is an auctionable WoW Token the account owns
-- (deliverable type 3 -> WowTokenMgr::CreateToken -> account_wow_token, state 0).
--
-- productId 574806 is a visible catalog slot in data/battlepay/product_list_68275.bin. The SH-6 seed
-- shipped it as a display-only template slot ("Soul of the Aspects", no deliverable); here it is
-- reskinned to the WoW Token so the client renders it and StartPurchase(574806) routes to this row and
-- its type-3 deliverable.
--   currency 1 = gold; price = 2000000000 copper (200,000 gold) - adjust freely.
--   deliverable type 3, id 0, count N: N tokens are created (default 1).
-- Idempotent.

DELETE FROM `shop_product` WHERE `productId` = 574806;
INSERT INTO `shop_product`
  (`productId`,`enabled`,`name`,`description`,`currency`,`price`,`priceItemId`,`priceItemCount`,`displayPrice`,`displayFlags`,`groupId`,`ordering`,`featured`,`availableFrom`,`availableUntil`,`reqLevel`,`reqFaction`,`hideIfOwned`,`playerConditionId`,`comment`) VALUES
(574806, 1, 'WoW Token', 'Buy a WoW Token for gold; it becomes an auctionable token your account owns.', 1, 2000000000, 0, 0, NULL, 0, 0, 4, 0, NULL, NULL, 0, -1, 0, 0, 'WoW Token acquisition path (deliverable type 3), from wow-token TK-1');

DELETE FROM `shop_product_deliverable` WHERE `productId` = 574806;
INSERT INTO `shop_product_deliverable` (`productId`,`seq`,`type`,`id`,`count`) VALUES
(574806, 0, 3, 0, 1);
