-- Order-hall troop "standard" GOs are NOT permanent props: retail spawns the standard only when troops are READY,
-- and it despawns when clicked (troops spawn + walk off). So the always-on shared spawns are wrong. Store each
-- standard's spawn point here (engine spawns a PRIVATE standard per player when their order is ready) and remove the
-- shared spawns. The clock lives on the RECRUITER NPC while an order is recruiting (engine, personal spell visual).
CREATE TABLE IF NOT EXISTS `garrison_order_hall_standard` (
  `containerId` INT UNSIGNED NOT NULL,
  `goEntry`     INT UNSIGNED NOT NULL,
  `map`         INT UNSIGNED NOT NULL,
  `posX`        FLOAT NOT NULL,
  `posY`        FLOAT NOT NULL,
  `posZ`        FLOAT NOT NULL,
  `orientation` FLOAT NOT NULL DEFAULT 0,
  PRIMARY KEY (`containerId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Where the per-player "Training Troops" standard spawns when troops are ready';

DELETE FROM `garrison_order_hall_standard` WHERE `containerId` IN (143,144,228);
INSERT INTO `garrison_order_hall_standard` (`containerId`,`goEntry`,`map`,`posX`,`posY`,`posZ`,`orientation`) VALUES
(143,250894,1220,4582.72,5290.87,859.863,3.97722),
(144,250895,1220,4610.36,5171.42,854.817,2.80444),
(228,250896,1220,4624.46,5213.61,856.497,1.73379);

-- Remove the always-visible shared standards (engine now spawns them per-player, only when ready).
DELETE FROM `gameobject` WHERE `id` IN (250894,250895,250896) AND `map`=1220;
