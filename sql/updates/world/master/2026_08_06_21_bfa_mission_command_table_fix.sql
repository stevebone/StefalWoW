-- Fix the Boralus (Wind's Redemption) Mission Command Table creature 138704.
-- It was non-functional: npcflag=0 (not clickable) AND its gossip option (menu 22141) had a NULL
-- GossipNpcOptionID so the AdventureMap interaction could not resolve. The correctly-configured copy
-- 138706 (spawned in Zandalar) has the gossip flag + GossipNpcOptionID 31003 -> AdventureMap
-- UiMapID 1011 (the war-campaign map). Match 138704 to it.
UPDATE `creature_template` SET `npcflag`=137438953473 WHERE `entry`=138704;
UPDATE `gossip_menu_option` SET `GossipNpcOptionID`=31003 WHERE `MenuID`=22141 AND `OptionID`=0;
-- Revert the earlier wrong hypothesis: the table uses AdventureMap(OptionNpc 31), NOT
-- GarrisonMissionNpc(27). Remove the test option added to Halford (135612, menu 23337).
DELETE FROM `gossip_menu_option` WHERE `MenuID`=23337 AND `OptionID`=90;
