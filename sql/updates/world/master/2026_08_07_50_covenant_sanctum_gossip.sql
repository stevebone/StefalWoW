--
-- Covenant P2.4 - resolve the NULL GossipNpcOptionID on the "Show me the Sanctum." options.
--
-- Player::OnGossipSelect only sends SMSG_GOSSIP_OPTION_NPC_INTERACTION when the option carries a
-- GossipNpcOptionID; without it the client has nothing to resolve to PlayerInteractionType::GarrTalent and the
-- option does nothing. Three of the four Sanctum Upgrades NPCs have it NULL.
--
-- Resolution (GossipNPCOption.db2, build 12.0.7.68275): the working Night Fae row - Zayhad, The Builder (165702),
-- menu 25513, GossipNpcOptionID 31332 - carries GossipNpcOption = 32 (GarrisonTalent) and
-- GarrTalentTreeID = 328 = "Ardenweald - Resevoir Upgrades" (GarrTalentTree.FeatureTypeIndex 4 =
-- Reservoir Upgrades, FeatureSubtypeIndex 3 = Night Fae). So the sanctum option is the covenant's Reservoir
-- Upgrades tree, and the remaining three resolve by FeatureSubtypeIndex (= CovenantID):
--
--   tree 327 "Bastion - Resevoir Upgrades"      sub 1 Kyrian     -> Haephus            (167745) menu 25654
--   tree 326 "Revendreth - Resevoir Upgrades"   sub 2 Venthyr    -> Foreman Flatfinger (172605) menu 26197
--   tree 328 "Ardenweald - Resevoir Upgrades"   sub 3 Night Fae  -> Zayhad             (165702) menu 25513 (already set)
--   tree 329 "Maldraxxus- Resevoir Upgrades"    sub 4 Necrolord  -> Arkadia Moa        (161909) menu 26226
--
-- GossipNPCOption rows 31387/31388/31389/31390 (GossipIndex 54074-54077) are the one complete, contiguous
-- per-covenant set of option-32 Reservoir rows. Duplicates exist (31332/31392/32296/32363) that differ ONLY in
-- GossipIndex - the retail gossip-option id - so which one a menu really used is a sniff-only fact; every other
-- column, including GossipNpcOption and GarrTalentTreeID, is identical and the client behaviour is the same.
-- The already-working Night Fae row (31332) is intentionally left as-is.
--
-- Idempotent: only fills rows that are still NULL.

UPDATE `gossip_menu_option` SET `GossipNpcOptionID` = 31388 WHERE `MenuID` = 25654 AND `OptionNpc` = 32 AND `GossipNpcOptionID` IS NULL; -- Haephus (Kyrian)    -> tree 327
UPDATE `gossip_menu_option` SET `GossipNpcOptionID` = 31390 WHERE `MenuID` = 26197 AND `OptionNpc` = 32 AND `GossipNpcOptionID` IS NULL; -- Flatfinger (Venthyr) -> tree 326
UPDATE `gossip_menu_option` SET `GossipNpcOptionID` = 31389 WHERE `MenuID` = 26226 AND `OptionNpc` = 32 AND `GossipNpcOptionID` IS NULL; -- Arkadia Moa (Necrolord) -> tree 329
