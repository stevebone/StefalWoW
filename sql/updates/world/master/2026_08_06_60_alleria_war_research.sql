-- Alleria Windrunner (143846, Boralus) War Research trait window opened empty: her GarrisonTalentNpc
-- gossip option (OptionNpc 32, menu 22781) had a NULL GossipNpcOptionID, so the client couldn't
-- resolve which GarrTalentTree to display (same NULL-interaction bug as the Mission Command Table).
-- Fix: GossipNpcOptionID 30935 -> GarrTalentTree 152 "War Research" (the ALLIANCE tree; its T5 talent
-- is "Horde Ambassador"; tree 153 with "Alliance Ambassador" is the Horde one).
-- NOTE: the 11 talents each carry their own PlayerConditionID, so they unlock progressively with
-- war-campaign progress + are researched with War Resources (currency 1560) — not all free at once.
UPDATE `gossip_menu_option` SET `GossipNpcOptionID`=30935 WHERE `MenuID`=22781 AND `OptionID`=0;
