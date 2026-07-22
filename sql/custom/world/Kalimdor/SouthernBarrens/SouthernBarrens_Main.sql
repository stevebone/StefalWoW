-- Southern Barrens Fixes

-- NPC: 10378 Omusa Thunderhorn <Wind Rider Master>
-- NPC: 5944 Yonada <Tailoring and Leatherworking Supplier>
-- NPC: 8016 Barrens Guard
-- NPC: 3703 Krulmoo Fullmoon <Leatherworking Trainer>
-- NPC: 6387 Dranh <Skinning Trainer>
-- NPC: 37167 Stonetalon Prisoner

-- Template fixes
UPDATE creature_template SET unit_flags3 = 8193, npcflag = 0 WHERE entry IN (10378,5944,3703,6387);
UPDATE creature_template SET unit_flags3 = 8193 WHERE entry = 8016;

-- Template Addons
INSERT INTO creature_template_addon (Entry, StandState) VALUES (37167, 1);
