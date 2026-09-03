-- BfA War Campaign follower-mission table: add the GarrisonMissionNpc trigger.
-- The mission frame opens via OptionNpc 27 (GarrisonMissionNpc) + GossipNpcOptionID 30323
-- (client resolves 30323 in GossipNPCOption.db2 -> PlayerInteractionType::GarrMission(32);
--  HandleOpenMissionNpc then sends SendDeleteExpiredMissionsResult for ALL garrisons incl. type-9).
-- No NPC in Boralus had this option, so the champion-mission frame was unreachable.
-- Added as a SECOND option on Halford Wyrmbane (135612, menu 23337) — the war leader the player
-- already uses — WITHOUT touching the Zandalar Campaign adventure-map table (144635).
DELETE FROM `gossip_menu_option` WHERE `MenuID`=23337 AND `OptionID`=90;
INSERT INTO `gossip_menu_option`
(`MenuID`,`GossipOptionID`,`OptionID`,`OptionNpc`,`OptionText`,`OptionBroadcastTextID`,`Language`,`Flags`,`ActionMenuID`,`ActionPoiID`,`GossipNpcOptionID`,`BoxCoded`,`BoxMoney`,`BoxText`,`BoxBroadcastTextID`,`SpellID`,`OverrideIconID`,`VerifiedBuild`)
VALUES (23337,90017600,90,27,'Show me the war table. <Champion Missions>',0,0,0,0,0,30323,0,0,'',0,0,0,68275);
