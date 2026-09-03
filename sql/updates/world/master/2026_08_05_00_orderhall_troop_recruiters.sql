-- Class Order Hall work orders (troop recruitment) — Hunter Trueshot Lodge (map 1220)
-- Engine: GarrisonMgr::LoadOrderHallShipments + Garrison::CreateTroopShipment (plotless capacitive display).
-- ShipmentCrafter = GossipOptionNpc enum 28. Container is resolved SERVER-SIDE by NPC entry
-- (garrison_order_hall_shipment), so the GossipNpcOptionID only needs enum 28 (ShipmentCrafter);
-- the server's OpenShipmentNpcResult supplies the authoritative container.
--
-- Recruiter -> container -> troop (verified via CharShipment/GossipNPCOption db2):
--   Lenara  106444 -> 143 Squad of Archers  (troop 671) : retail gossip 30527 (menu 19997) — already correct
--   Sampson 106446 -> 144 Band of Trackers  (troop 672) : retail gossip 30528 (menu 19998)
--   Silus   106445 -> 228 Nightborne Hunters(troop 1023): no retail gossip (command-bar in retail) — synth menu 900010

-- 1) NPC -> shipment-container mapping read by GarrisonMgr::LoadOrderHallShipments()
CREATE TABLE IF NOT EXISTS `garrison_order_hall_shipment` (
  `npcEntry`    INT UNSIGNED NOT NULL COMMENT 'creature_template.entry of the troop recruiter',
  `containerId` INT UNSIGNED NOT NULL COMMENT 'CharShipmentContainer.db2 ID whose CharShipment.GarrFollowerID is the troop',
  `comment`     VARCHAR(128) NOT NULL DEFAULT '',
  PRIMARY KEY (`npcEntry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Class Order Hall troop recruiters -> shipment container';

DELETE FROM `garrison_order_hall_shipment` WHERE `npcEntry` IN (106446,106444,106445);
INSERT INTO `garrison_order_hall_shipment` (`npcEntry`,`containerId`,`comment`) VALUES
(106444,143,'Lenara - Squad of Archers'),
(106446,144,'Sampson - Band of Trackers'),
(106445,228,'Nighthuntress Silus - Nightborne Hunters');

-- 2) Spawn Nighthuntress Silus (106445) at Trueshot Lodge, beside Sampson/Lenara
DELETE FROM `creature` WHERE `guid`=50052005;
INSERT INTO `creature`
(`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`)
VALUES
(50052005,106445,1220,7503,7877,0,0,0,0,-1,0,0,4602.0,5180.0,854.9,2.60081,180,0,0,100,0);

-- 3) Silus gossip: synth menu 900010 with a ShipmentCrafter option (enum 28).
--    Reuses a valid enum-28 GossipNpcOptionID; server resolves container 228 by NPC entry.
DELETE FROM `gossip_menu` WHERE `MenuID`=900010;
INSERT INTO `gossip_menu` (`MenuID`,`TextID`) VALUES (900010,1);
DELETE FROM `gossip_menu_option` WHERE `MenuID`=900010;
INSERT INTO `gossip_menu_option`
(`MenuID`,`GossipOptionID`,`OptionID`,`OptionNpc`,`OptionText`,`OptionBroadcastTextID`,`Language`,`Flags`,`ActionMenuID`,`ActionPoiID`,`GossipNpcOptionID`,`BoxCoded`,`BoxMoney`,`BoxText`,`BoxBroadcastTextID`,`SpellID`,`OverrideIconID`,`VerifiedBuild`)
VALUES
(900010,9000101,0,28,'I have Nightborne ready for duty.',0,0,0,0,0,30527,0,0,NULL,0,NULL,NULL,0);

-- 4) Sampson gossip: menu 19998 currently has broken OptionNpc=21 (ArtifactRespec/NYI) options with NULL
--    GossipNpcOptionID. Replace with the correct ShipmentCrafter option (Band of Trackers, retail 30528).
DELETE FROM `gossip_menu_option` WHERE `MenuID`=19998;
INSERT INTO `gossip_menu_option`
(`MenuID`,`GossipOptionID`,`OptionID`,`OptionNpc`,`OptionText`,`OptionBroadcastTextID`,`Language`,`Flags`,`ActionMenuID`,`ActionPoiID`,`GossipNpcOptionID`,`BoxCoded`,`BoxMoney`,`BoxText`,`BoxBroadcastTextID`,`SpellID`,`OverrideIconID`,`VerifiedBuild`)
VALUES
(19998,9000102,0,28,'I need a troop of trackers.',0,0,0,0,0,30528,0,0,NULL,0,NULL,NULL,0);
-- Lenara (menu 19997, option 30527) is already correct in retail data — untouched.

-- 5) creature_template_gossip: Silus -> 900010; Lenara/Sampson keep their retail menus only.
DELETE FROM `creature_template_gossip` WHERE `CreatureID` IN (106444,106446) AND `MenuID`=900010;
DELETE FROM `creature_template_gossip` WHERE `CreatureID`=106445 AND `MenuID`=900010;
INSERT INTO `creature_template_gossip` (`CreatureID`,`MenuID`) VALUES (106445,900010);

-- 6) Ensure gossip flag (UNIT_NPC_FLAG_GOSSIP = 1) on all three recruiters
UPDATE `creature_template` SET `npcflag` = `npcflag` | 1 WHERE `entry` IN (106446,106444,106445);
