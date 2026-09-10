-- Garrison mission rewards (base item/currency beyond follower XP).
-- Base garrison mission rewards are server-authoritative in retail (NOT in client DB2), so they are
-- authored here. Rows below are REAL reward payloads decoded from 12.0.7 packet sniffs
-- (SMSG_GARRISON_ADD_MISSION_RESULT); unauthored missions fall back to GarrisonMgr's per-GarrType
-- resource-currency formula. Follower XP already comes from GarrMission.db2 BaseFollowerXP.
--   RewardType: 0 = base (granted on success), 1 = overmax/bonus (granted on bonus roll)
--   Gold is copper, emitted as currency id 0.
CREATE TABLE IF NOT EXISTS `garrison_mission_reward` (
  `GarrMissionId`    INT UNSIGNED NOT NULL COMMENT 'GarrMission.db2 ID',
  `Idx`              TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `RewardType`       TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=base, 1=overmax/bonus',
  `ItemId`           INT UNSIGNED NOT NULL DEFAULT 0,
  `ItemQuantity`     INT UNSIGNED NOT NULL DEFAULT 0,
  `CurrencyId`       INT UNSIGNED NOT NULL DEFAULT 0,
  `CurrencyQuantity` INT UNSIGNED NOT NULL DEFAULT 0,
  `Gold`             INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'copper',
  `FollowerXP`       INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = use DB2 BaseFollowerXP',
  PRIMARY KEY (`GarrMissionId`,`Idx`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Authored garrison/order-hall/war-campaign/adventure mission base rewards';

DELETE FROM `garrison_mission_reward` WHERE `GarrMissionId` IN
 (86,132,194,210,262,269,304,314,335,668,670,672,1681,1710,1755,1756,1757,1761,1765,1767,1773,1776,1796,1816,1866,1883,1897,1905,1906,2096,2097,2098,2099,2100);

INSERT INTO `garrison_mission_reward`
 (`GarrMissionId`,`Idx`,`RewardType`,`ItemId`,`ItemQuantity`,`CurrencyId`,`CurrencyQuantity`,`Gold`,`FollowerXP`) VALUES
-- WoD garrison (Garrison Resources 824 + gear items)
 (86,0,0,114053,1,0,0,0,0),
 (132,0,0,0,0,824,175,0,0),
 (194,0,0,0,0,824,48,0,0),
 (210,0,0,0,0,824,175,0,0),
 (262,0,0,120302,1,0,0,0,0),
 (269,0,0,0,0,824,225,0,0),
 (304,0,0,120301,1,0,0,0,0),
 (314,0,0,118529,1,0,0,0,0),
 (335,0,0,117492,1,0,0,0,0),
 (335,1,0,0,0,824,250,0,0),
-- WoD shipyard (Oil 1101)
 (668,0,0,0,0,1101,40,0,0),
 (670,0,0,0,0,1101,40,0,0),
 (672,0,0,0,0,1101,50,0,0),
-- BfA missions: item rewards (base so they always grant on success)
 (1681,0,0,147501,1,0,0,0,0),
 (1710,0,0,152326,1,0,0,0,0),
 (1755,0,0,146950,2,0,0,0,0),
 (1756,0,0,152960,1,0,0,0,0),
 (1757,0,0,124124,4,0,0,0,0),
 (1765,0,0,146943,2,0,0,0,0),
 (1767,0,0,124124,4,0,0,0,0),
 (1773,0,0,151844,1,0,0,0,0),
 (1776,0,0,152442,1,0,0,0,0),
 (1796,0,0,151568,2,0,0,0,0),
-- BfA missions: resource currency base + overmax bonus
 (1761,0,0,0,0,1533,25,0,0),  (1761,1,1,0,0,1533,25,0,0),
 (1816,0,0,0,0,1508,42,0,0),  (1816,1,1,0,0,1508,60,0,0),
 (1866,0,0,0,0,1553,271,0,0), (1866,1,1,0,0,1553,200,0,0),
 (1883,0,0,0,0,1553,241,0,0), (1883,1,1,0,0,1553,200,0,0),
 (1897,0,0,0,0,1579,25000,0,0),(1897,1,1,0,0,1579,22500,0,0),
 (1905,0,0,0,0,1553,466,0,0), (1905,1,1,0,0,1553,200,0,0),
 (1906,0,0,0,0,1599,22500,0,0),(1906,1,1,0,0,1599,10000,0,0),
-- BfA contract item set
 (2096,0,0,163535,1,0,0,0,0),
 (2097,0,0,163571,1,0,0,0,0),
 (2098,0,0,163594,1,0,0,0,0),
 (2099,0,0,163600,1,0,0,0,0),
 (2100,0,0,163610,1,0,0,0,0);
