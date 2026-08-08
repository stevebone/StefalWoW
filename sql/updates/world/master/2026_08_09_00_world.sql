-- In-game Shop (BattlePay) catalog administration - replaces battlepay_product.
--
-- The shipped catalog blob (data/battlepay/product_list_68275.bin) is now a TEMPLATE. At startup and
-- on `.reload shop_catalog`, BattlePayMgr reskins its 9 simple-shape slots from `shop_product` via the
-- byte-exact BattlePayCatalogWriter and records a slot->product routing map. Everything the wire cannot
-- express (enable/disable, windows, level/faction/owned/condition gates) is enforced server-side at
-- purchase time. See src/server/game/BattlePay/ and BattlePay/README_DEPLOY.md.

CREATE TABLE IF NOT EXISTS `shop_product` (
  `productId`         INT UNSIGNED     NOT NULL,           -- catalog productId (routing frees it from blob slot ids)
  `enabled`           TINYINT UNSIGNED NOT NULL DEFAULT 1, -- 0 = withheld from assembly + purchase-gated
  `name`              VARCHAR(255)     NOT NULL,           -- card title (wire name1)
  `description`       VARCHAR(1000)    NOT NULL DEFAULT '',-- card body (wire name2)
  `currency`          TINYINT UNSIGNED NOT NULL DEFAULT 1, -- 0 free | 1 gold(copper) | 2 item-token | 3 custom-currency
  `price`             BIGINT UNSIGNED  NOT NULL DEFAULT 0, -- copper (currency 1) or currency amount (3)
  `priceItemId`       INT UNSIGNED     NOT NULL DEFAULT 0, -- currency 2: token item
  `priceItemCount`    INT UNSIGNED     NOT NULL DEFAULT 0,
  `displayPrice`      BIGINT UNSIGNED  DEFAULT NULL,       -- wire fixed-point /100000 override; NULL = derived
  `displayFlags`      INT UNSIGNED     NOT NULL DEFAULT 0, -- BattlepayDisplayFlags (8=HiddenPrice, 256=HideWhenOwned)
  `groupId`           INT UNSIGNED     NOT NULL DEFAULT 0, -- desired category (rendered after ShopEntry crack, SH-7)
  `ordering`          INT              NOT NULL DEFAULT 0, -- slot-assignment priority (lower = earlier slot)
  `featured`          TINYINT UNSIGNED NOT NULL DEFAULT 0, -- claim on prominent slots
  `availableFrom`     TIMESTAMP        NULL DEFAULT NULL,  -- NULL = always
  `availableUntil`    TIMESTAMP        NULL DEFAULT NULL,
  `reqLevel`          TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `reqFaction`        TINYINT          NOT NULL DEFAULT -1,-- -1 any, else TeamId (0 alliance, 1 horde)
  `hideIfOwned`       TINYINT UNSIGNED NOT NULL DEFAULT 0, -- sets wire flag 256 + purchase gate
  `playerConditionId` INT UNSIGNED     NOT NULL DEFAULT 0, -- 0 = none; server-side PlayerCondition check
  `comment`           VARCHAR(255)     NOT NULL DEFAULT '',
  PRIMARY KEY (`productId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `shop_product_deliverable` (
  `productId` INT UNSIGNED     NOT NULL,
  `seq`       TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `type`      TINYINT UNSIGNED NOT NULL,             -- 1 item | 2 spell | 3 WoW Token | 4 game-time (reserved) | 5 service (reserved)
  `id`        INT UNSIGNED     NOT NULL DEFAULT 0,   -- itemId / spellId / 0 (token) / days / serviceType
  `count`     INT UNSIGNED     NOT NULL DEFAULT 1,
  PRIMARY KEY (`productId`, `seq`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `shop_slot_override` (
  `slotIndex` TINYINT UNSIGNED NOT NULL,             -- 0..N-1 (N = simple-shape slots in the template, 9 @68275)
  `productId` INT UNSIGNED     NOT NULL,             -- 0 = force the slot to the inert placeholder
  PRIMARY KEY (`slotIndex`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Seed = the 9 slots of the shipped 68275 template, so assembly reproduces the shipped catalog with no
-- placeholders. Slots 0-3 are the former battlepay_product rows (real deliverables); slots 4-8 are the
-- template's display-only mounts/pets (no deliverable yet -> visible but purchase returns 57, exactly as
-- before). Idempotent restore of exactly these rows.
DELETE FROM `shop_product` WHERE `productId` IN (1616893,1616898,841541,132620,574806,254652,666530,874857,576138);
INSERT INTO `shop_product`
  (`productId`,`enabled`,`name`,`description`,`currency`,`price`,`priceItemId`,`priceItemCount`,`displayPrice`,`displayFlags`,`groupId`,`ordering`,`featured`,`availableFrom`,`availableUntil`,`reqLevel`,`reqFaction`,`hideIfOwned`,`playerConditionId`,`comment`) VALUES
(1616893, 1, 'Traveler''s Tundra Pack', 'A roomy 16-slot backpack for the road.', 1, 500000, 0, 0, NULL, 10, 0, 0, 0, NULL, NULL, 0, -1, 0, 0, 'migrated from battlepay_product'),
(1616898, 1, 'Crate of Linen Cloth', 'A bundle of 20 bolts of linen cloth.', 1, 10000, 0, 0, NULL, 78, 0, 1, 0, NULL, NULL, 0, -1, 0, 0, 'migrated from battlepay_product'),
(841541, 1, 'Case of Spring Water', '20 flasks of refreshing spring water.', 1, 5000, 0, 0, NULL, 81, 0, 2, 0, NULL, NULL, 0, -1, 0, 0, 'migrated from battlepay_product'),
(132620, 1, 'Reins of the Brown Horse', 'A sturdy brown riding horse.', 1, 1000000, 0, 0, NULL, 10, 0, 3, 0, NULL, NULL, 0, -1, 1, 0, 'migrated from battlepay_product'),
(574806, 1, 'Soul of the Aspects', 'Untamed but friendly, this golden dragon will fly the skies with you.', 1, 0, 0, 0, 100000, 10, 0, 4, 0, NULL, NULL, 0, -1, 0, 0, 'display-only template slot (no deliverable)'),
(254652, 1, 'Lil'' Ragnaros', 'Your foes will be purged by fire with Lil'' Ragnaros on your team.', 1, 0, 0, 0, 100000, 72, 0, 5, 0, NULL, NULL, 0, -1, 0, 0, 'display-only template slot (no deliverable)'),
(666530, 1, 'Cinder Kitten', 'This Cinder Kitten will char your face off with cuteness.', 1, 0, 0, 0, 100000, 10, 0, 6, 0, NULL, NULL, 0, -1, 0, 0, 'display-only template slot (no deliverable)'),
(874857, 1, 'Blossoming Ancient', 'Grow your collection with the life-giving Blossoming Ancient.', 1, 0, 0, 0, 100000, 72, 0, 7, 0, NULL, NULL, 0, -1, 0, 0, 'display-only template slot (no deliverable)'),
(576138, 1, 'Heart of the Aspects', 'Glowing with inner light, this luminous flying dragon defies the darkness.', 1, 0, 0, 0, 250000, 83, 0, 8, 0, NULL, NULL, 0, -1, 0, 0, 'display-only template slot (no deliverable)');

DELETE FROM `shop_product_deliverable` WHERE `productId` IN (1616893,1616898,841541,132620);
INSERT INTO `shop_product_deliverable` (`productId`,`seq`,`type`,`id`,`count`) VALUES
(1616893, 0, 1, 4500, 1),
(1616898, 0, 1, 2589, 20),
(841541, 0, 1, 159, 20),
(132620, 0, 2, 458, 1);

-- battlepay_product's only reader (BattlePayMgr::LoadProducts) has been re-targeted to shop_product.
DROP TABLE IF EXISTS `battlepay_product`;
