-- Order Advancement: give the 6 class-hall advisors that lacked one a talent gossip menu.
--
-- The advisors that already open the Order Advancement tree (Winstone Wolfe, Einar, Melinda, Chronicler Elrianne,
-- Sir Alamande, Archivist Zubashi) each have a creature_template_gossip row pointing at a menu whose single option is
-- OptionNpc = 32 (GarrisonTalent) with GossipNpcOptionID = NULL - selecting it opens the PLAYER's class-order tree
-- directly (PlayerInteractionType::GarrTalent), no per-tree id needed. Menu 19646 (shared by the shaman/priest/paladin
-- advisors) has no class gate, so it is class-generic. The other six advisors had NO creature_template_gossip row at
-- all, so the client saw no gossip content and offered no interaction (clicking did nothing / sent a stale
-- questgiver-hello the server rejected). Point them at 19646 so they behave like the working advisors.
--   108050 Survivalist Bahn (Hunter), 98939 Number Nine Jia (Monk), 112199 Journeyman Goldmine (Rogue),
--    97989 Leafbeard (Druid),        108527 Loramus (Warlock),      110725 Archon Torias (Death Knight)
DELETE FROM `creature_template_gossip` WHERE `CreatureID` IN (108050, 98939, 112199, 97989, 108527, 110725);
INSERT INTO `creature_template_gossip` (`CreatureID`, `MenuID`) VALUES
(108050, 19646),
( 98939, 19646),
(112199, 19646),
( 97989, 19646),
(108527, 19646),
(110725, 19646);
