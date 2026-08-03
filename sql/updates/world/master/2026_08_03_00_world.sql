-- Order-hall "Further Advancement" quests: give the Order Advancement advisors their gossip flag.
--
-- Quests 46778-46789 ("Further Advancement", one per class hall) carry a custom Type-3 (TALKTO) objective directing the
-- player to the hall's Order Advancement advisor. Most advisors already have a gossip npcflag, but three were left at
-- npcflag=0, so the client can't treat them as talk-to targets and renders the objective as "Slay <advisor>" (and it
-- can never be completed - you can't open gossip). Give them the gossip flag (matching the working advisors, npcflag=1):
--   108050 Survivalist Bahn      (Hunter, 46783)
--    98939 Number Nine Jia       (Monk,   46785)
--   112199 Journeyman Goldmine   (Mage,   46781)
UPDATE `creature_template` SET `npcflag` = `npcflag` | 1 WHERE `entry` IN (108050, 98939, 112199);
