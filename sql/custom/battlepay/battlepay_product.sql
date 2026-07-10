-- In-game Shop (BattlePay) server-defined products.
-- Each row maps a catalog productID (advertised to the client in GET_PRODUCT_LIST_RESPONSE) to how it is
-- paid for and what it grants. BattlePayMgr::LoadProducts() reads this from the world DB.
--   costMoney     : price in copper (0 = free)
--   costItemId/Count : optional token-item price (0 = none)
--   grantType     : 1 = item, 2 = spell (mount/toy/appearance)
--   grantId/grantCount : what to deliver
-- The productIds below match the reskinned demo catalog produced by gen_shop_catalog.py; regenerate both
-- together if you change the product set.

DROP TABLE IF EXISTS `battlepay_product`;
CREATE TABLE `battlepay_product` (
  `productId`     INT UNSIGNED    NOT NULL,
  `costMoney`     BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `costItemId`    INT UNSIGNED    NOT NULL DEFAULT 0,
  `costItemCount` INT UNSIGNED    NOT NULL DEFAULT 0,
  `grantType`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `grantId`       INT UNSIGNED    NOT NULL DEFAULT 0,
  `grantCount`    INT UNSIGNED    NOT NULL DEFAULT 1,
  `name`          VARCHAR(100)    NOT NULL DEFAULT '',
  PRIMARY KEY (`productId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT INTO `battlepay_product`
  (`productId`, `costMoney`, `costItemId`, `costItemCount`, `grantType`, `grantId`, `grantCount`, `name`) VALUES
(1616893, 500000,  0, 0, 1, 4500, 1,  'Traveler''s Tundra Pack'),
(1616898, 10000,   0, 0, 1, 2589, 20, 'Crate of Linen Cloth'),
(841541,  5000,    0, 0, 1, 159,  20, 'Case of Spring Water'),
(132620,  1000000, 0, 0, 2, 458,  1,  'Reins of the Brown Horse');
