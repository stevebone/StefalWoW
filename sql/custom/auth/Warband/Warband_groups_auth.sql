-- Warband account tables (auth database).
-- Ported from RolePlay_master: group ids are scoped to the game account - the client
-- picks new ids counting from the highest id it was sent; groups are region-wide and
-- shared by all realms.
-- realmId semantics differ per table: on groups it is provenance only (the realm that
-- created the row); on members it is the member character's HOME realm - character
-- guid counters collide across realms, so the home realm is needed to rebuild the
-- full guid on the wire.

DROP TABLE IF EXISTS `account_warband_group_members`;
DROP TABLE IF EXISTS `account_warband_groups`;

CREATE TABLE `account_warband_groups` (
  `id` bigint(20) unsigned NOT NULL,
  `accountId` int(10) unsigned NOT NULL,
  `realmId` int(10) unsigned NOT NULL DEFAULT '1',
  `orderIndex` tinyint(3) unsigned NOT NULL,
  `name` varchar(257) NOT NULL,
  `warbandSceneId` int(10) unsigned NOT NULL DEFAULT '0',
  `flags` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`accountId`, `id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `account_warband_group_members` (
  `accountId` int(10) unsigned NOT NULL,
  `realmId` int(10) unsigned NOT NULL DEFAULT '1',
  `groupId` bigint(20) unsigned NOT NULL,
  `characterGuid` bigint(20) unsigned NOT NULL,
  `placementId` int(10) unsigned NOT NULL,
  `type` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`accountId`, `groupId`, `characterGuid`),
  CONSTRAINT `fk_warband_group` FOREIGN KEY (`accountId`, `groupId`)
    REFERENCES `account_warband_groups` (`accountId`, `id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


