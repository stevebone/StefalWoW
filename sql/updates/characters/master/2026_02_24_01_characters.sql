CREATE TABLE IF NOT EXISTS `character_garrison_shipments` (
  `dbId` bigint unsigned NOT NULL,
  `guid` bigint unsigned NOT NULL,
  `shipmentId` int unsigned NOT NULL DEFAULT '0' COMMENT 'CharShipment.ID',
  `plotInstanceId` int unsigned NOT NULL DEFAULT '0',
  `creationTime` bigint NOT NULL DEFAULT '0',
  `duration` int NOT NULL DEFAULT '0',
  `assignedFollowerDbId` bigint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`dbId`),
  KEY `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
