-- Order Advancement "Unseen Path" (Hunter GarrTalent 377) unlocks a "Requisition a Seal of Broken Fate" work order
-- at Sentry Sprydash (100491, subname "Unseen Path"), capped at 3 per week (in exchange for Order Resources).
-- Engine: Garrison::CreateTroopShipment resolves the container by NPC, gates on requiredTalentId + weeklyLimit, and
-- CompleteShipment delivers the seal (CharShipment 381 in container 218 -> DummyItemID 139460). Item work order, no troop.

-- 1) Extend the recruiter->container map with the optional talent gate + weekly cap.
ALTER TABLE `garrison_order_hall_shipment`
  ADD COLUMN `requiredTalentId` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'GarrTalent that must be researched to place this order (0=none)' AFTER `containerId`,
  ADD COLUMN `weeklyLimit`      INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'per-week cap on placed orders (0=unlimited)' AFTER `requiredTalentId`;

-- 2) Sentry Sprydash -> Seal of Broken Fate container 218, gated on Unseen Path (377), 3 per week.
DELETE FROM `garrison_order_hall_shipment` WHERE `npcEntry`=100491;
INSERT INTO `garrison_order_hall_shipment` (`npcEntry`,`containerId`,`requiredTalentId`,`weeklyLimit`,`comment`) VALUES
(100491,218,377,3,'Sentry Sprydash - Seal of Broken Fate (Unseen Path)');

-- 3) Gossip: ShipmentCrafter option (OptionNpc=28) opening the seal container. GossipNpcOptionID 30658 = retail
--    "Requisition a Seal of Broken Fate" crafter option (GossipNPCOption enum 28 -> CharShipment 381).
DELETE FROM `gossip_menu` WHERE `MenuID`=900011;
INSERT INTO `gossip_menu` (`MenuID`,`TextID`) VALUES (900011,1);
DELETE FROM `gossip_menu_option` WHERE `MenuID`=900011;
INSERT INTO `gossip_menu_option`
(`MenuID`,`GossipOptionID`,`OptionID`,`OptionNpc`,`OptionText`,`OptionBroadcastTextID`,`Language`,`Flags`,`ActionMenuID`,`ActionPoiID`,`GossipNpcOptionID`,`BoxCoded`,`BoxMoney`,`BoxText`,`BoxBroadcastTextID`,`SpellID`,`OverrideIconID`,`VerifiedBuild`)
VALUES
(900011,9000111,0,28,'I would like to requisition a Seal of Broken Fate.',0,0,0,0,0,30658,0,0,NULL,0,NULL,NULL,0);

-- 4) Map Sprydash to the menu + ensure the gossip flag.
DELETE FROM `creature_template_gossip` WHERE `CreatureID`=100491 AND `MenuID`=900011;
INSERT INTO `creature_template_gossip` (`CreatureID`,`MenuID`) VALUES (100491,900011);
UPDATE `creature_template` SET `npcflag` = `npcflag` | 1 WHERE `entry`=100491;
