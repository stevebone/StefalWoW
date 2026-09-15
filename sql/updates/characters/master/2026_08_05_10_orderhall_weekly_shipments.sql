-- Per-character weekly cap tracking for order-hall work orders (e.g. the Hunter "Unseen Path" Seal of Broken Fate
-- order, capped at 3 per week). `weekReset` = the server's weekly quest reset; once passed, the counter starts fresh.
CREATE TABLE IF NOT EXISTS `character_garrison_weekly_shipments` (
  `guid`      BIGINT UNSIGNED NOT NULL,
  `npcEntry`  INT UNSIGNED    NOT NULL,
  `placed`    INT UNSIGNED    NOT NULL DEFAULT 0,
  `weekReset` BIGINT          NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`,`npcEntry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Order-hall weekly-capped work orders (Garrison::CreateTroopShipment gate)';
