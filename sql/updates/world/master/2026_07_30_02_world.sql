-- WoD work-order NPCs part 2: add the "place a work order" gossip option to work-order NPCs that already
-- have a gossip menu but whose menu lacks a WORKING ShipmentCrafter option (some had none, some had an
-- OptionNpc=28 row with a NULL GossipNpcOptionID, which does not drive the work-order flow). Migration
-- 2026_07_30_01 handled NPCs with no menu at all (linked them to Marianne's menu 17425); this handles the
-- ones that own a menu, adding the option without disturbing their existing options (trapping, battle-pet
-- heal, etc.). Option cloned from Marianne's working row: OptionNpc=28, GossipNpcOptionID=30265 (building-
-- agnostic; the server resolves the shipment container from the NPC's own building), text/broadcast 83605.
INSERT INTO `gossip_menu_option`
  (`MenuID`, `GossipOptionID`, `OptionID`, `OptionNpc`, `OptionText`, `OptionBroadcastTextID`, `Language`,
   `Flags`, `ActionMenuID`, `ActionPoiID`, `GossipNpcOptionID`, `BoxCoded`, `BoxMoney`, `BoxText`,
   `BoxBroadcastTextID`, `SpellID`, `OverrideIconID`, `VerifiedBuild`)
SELECT m.`MenuID`,
       90000000 + m.`MenuID`,               -- unique GossipOptionID
       99,                                    -- OptionID slot (unused by these small garrison menus)
       28,                                    -- GossipOptionNpc::ShipmentCrafter
       'I would like to place a work order.',
       83605,                                 -- OptionBroadcastTextID (same string as Marianne's)
       0, 0, 0, 0,
       30265,                                 -- ShipmentCrafter GossipNpcOptionID (valid; NULL crashes client #132)
       0, 0, NULL, 0, NULL, NULL, 0
FROM (
    SELECT DISTINCT ctg.`MenuID`
    FROM `creature_template` ct
    JOIN `creature_template_gossip` ctg ON ctg.`CreatureID` = ct.`entry`
    WHERE ct.`subname` = 'Work Orders' AND ((ct.`npcflag` >> 32) & 16) = 16
      AND ctg.`MenuID` NOT IN (
          SELECT `MenuID` FROM `gossip_menu_option` WHERE `OptionNpc` = 28 AND `GossipNpcOptionID` = 30265)
) m;
