-- In-game Shop (BattlePay) server-defined products.
-- Promoted from sql/custom/battlepay/battlepay_product.sql to an auto-applying world update so a
-- tester's DB gets it on startup without a manual import (sql/custom is never applied by the core).
-- Each row maps a catalog productID (advertised to the client in the shipped GET_PRODUCT_LIST_RESPONSE
-- blob, data/battlepay/product_list_68275.bin) to how it is paid for and what it grants.
--   costMoney            : price in copper (0 = free)
--   costItemId/Count     : optional token-item price (0 = none)
--   grantType            : 1 = item, 2 = spell (mount/toy/appearance)
--   grantId/grantCount   : what to deliver
-- The productIds MUST match the shipped blob. Regenerate both together with gen_shop_catalog.py.
-- Idempotent: re-running (or a re-applied hash) restores exactly these rows.

CREATE TABLE IF NOT EXISTS `battlepay_product` (
  `productId`     INT UNSIGNED     NOT NULL,
  `costMoney`     BIGINT UNSIGNED  NOT NULL DEFAULT 0,
  `costItemId`    INT UNSIGNED     NOT NULL DEFAULT 0,
  `costItemCount` INT UNSIGNED     NOT NULL DEFAULT 0,
  `grantType`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `grantId`       INT UNSIGNED     NOT NULL DEFAULT 0,
  `grantCount`    INT UNSIGNED     NOT NULL DEFAULT 1,
  `name`          VARCHAR(100)     NOT NULL DEFAULT '',
  PRIMARY KEY (`productId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DELETE FROM `battlepay_product` WHERE `productId` IN (1616893, 1616898, 841541, 132620);
INSERT INTO `battlepay_product`
  (`productId`, `costMoney`, `costItemId`, `costItemCount`, `grantType`, `grantId`, `grantCount`, `name`) VALUES
(1616893, 500000,  0, 0, 1, 4500, 1,  'Traveler''s Tundra Pack'),
(1616898, 10000,   0, 0, 1, 2589, 20, 'Crate of Linen Cloth'),
(841541,  5000,    0, 0, 1, 159,  20, 'Case of Spring Water'),
(132620,  1000000, 0, 0, 2, 458,  1,  'Reins of the Brown Horse');
