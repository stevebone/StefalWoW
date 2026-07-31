-- WoD work-order NPCs: show the "place a work order" gossip option alongside the building's tutorial quest.
--
-- Only Marianne Levine (Leatherworking, creature 78207) had a gossip menu (17425) carrying the ShipmentCrafter
-- option, so for every other profession building (Alchemy, Blacksmithing, etc.) the NPC offered ONLY its intro
-- quest ("Your First X Work Order"). A single offerable quest with no gossip menu auto-opens the quest dialog
-- (Player::SendPreparedQuest), which hid the work-order option entirely until the quest was finished.
--
-- Menu 17425's option uses GossipNpcOptionID 30265 (ShipmentCrafter). It is building-agnostic: selecting it
-- makes the client send CMSG_GARRISON_OPEN_SHIPMENT_NPC, and the server (Garrison::SendOpenShipmentUI) resolves
-- the shipment container from the NPC's OWN building/plot - so one shared menu works for every work-order NPC.
-- Quests come from creature_queststarter and are appended to whichever menu is shown, so the player now gets
-- both the quest and the work-order option; once the quest is done the work-order option opens directly.
--
-- Link every shipment-crafter "Work Orders" NPC that does not already have a gossip menu to menu 17425.
INSERT INTO `creature_template_gossip` (`CreatureID`, `MenuID`, `VerifiedBuild`)
SELECT ct.`entry`, 17425, 0
FROM `creature_template` ct
LEFT JOIN `creature_template_gossip` ctg ON ctg.`CreatureID` = ct.`entry`
WHERE ct.`subname` = 'Work Orders'
  AND ((ct.`npcflag` >> 32) & 16) = 16   -- UNIT_NPC_FLAG_2_SHIPMENT_CRAFTER
  AND ctg.`CreatureID` IS NULL;
