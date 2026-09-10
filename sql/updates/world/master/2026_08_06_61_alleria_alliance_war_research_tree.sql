-- Fix: Alleria Windrunner (Alliance, creature 143846, gossip menu 22781) was routing to the
-- HORDE War Research talent tree (GarrTalentTree 152, GossipNpcOptionID 30935), whose PlayerCondition
-- 64059 racemask excludes Human -> every Tier-0 trait showed locked for Alliance players.
-- The Alliance War Research tree is 153 (Tier-0 traits 551/552, PlayerCondition 64046: racemask
-- includes Human, level gate = ContentTuning 467 min 35 / no upper cap, quest gate 53583|53602).
-- GossipNpcOptionID 30918 maps to GarrTalentTree 153 (Alliance). Supersedes 2026_08_06_60.
UPDATE `gossip_menu_option` SET `GossipNpcOptionID`=30918 WHERE `MenuID`=22781 AND `OptionID`=0;
