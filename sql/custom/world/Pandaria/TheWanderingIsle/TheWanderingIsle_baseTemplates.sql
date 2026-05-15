-- NPC: 60858 Deepscale Fleshripper (phase 544)
-- NPC: 60780 Deepscale Ravager (phase 544)
-- NPC: 56360 Deepscale Tormentor (phase 1835)
-- NPC: 60722 Deepscale Tormentor
-- NPC: 56007 Darkened Horror (phase 1835)
-- NPC: 56008 Darkened Terror (phase 1835)
-- NPC: 60685 Deepscale Aggressor
-- NPC: 56009 Vordraka
-- NPC: 53705 Pangolin
-- NPC: 56172 Ash moth
-- NPC: 56174 Tiger cub

-- NPC: 55940 Jojo at the Alliance camp
-- NPC: 55942 Ji at the Hut
-- NPC: 55999 Injured Sailor (phase 1835)
-- NPC: 56362 Skyseeker Sailor (phase 1835)
-- NPC: 56476 Injured Sailor Rescue controller (phase 1835)
-- NPC: 56236 Injured Sailor (spawn)
-- NPC: 60897 Injured Sailor
-- NPC: 60900 Ji summoned at the Hut
-- NPC: 56416 Aysa fighting Vordraka
-- NPC: 55943 Wei Palerage
-- NPC: 55944 Delora Lionheart
-- NPC: 60042 Korga
-- NPC: 60055 Makael Bay
-- NPC: 67186 Provisioner Drog
-- NPC: 67185 Provisioner Phelps
-- NPC: 60895 Alliance Priest
-- NPC: 60729 Aysa at the explosion
-- NPC: 60741 Ji at the explosion
-- NPC: 56417 Aysa after explosion?
-- NPC: 56418 Ji after Vordraka
-- NPC: 57739 Ji After healed Shenzinsu
-- NPC: 56013 Spirit of Master Shang
-- NPC: 60877 Alliance Priest
-- NPC: 60878 Alliance Priest
-- NPC: 60770 Horde Druid
-- NPC: 60834 Horde Druid
-- NPC: 60851 Delora Lionheart (during healing)
-- NPC: 60852 Korga (during healing)
-- NPC: 57317 Skyseeker Sailor (during healing)
-- NPC: 60854 Escaped Horde Crewman (during healing)
-- NPC: 60853 Makael Bay (during healing)
-- NPC: 60873 Skyseeker Sailor (during healing)
-- NPC: 56419 Jojo (during healing)
-- NPC: 60896 Horde Druid

DELETE FROM `creature_template_addon` WHERE `entry` IN (55943, 60042, 67186, 55942, 55944, 60055, 60900, 67185, 55999, 56236, 56362, 56416, 56476, 60895, 55940,
60858, 60780, 56360, 60722, 56007, 56008, 60685, 56009, 60729, 60741, 56417, 56418, 57739, 56013, 60877, 60878, 60770, 60834, 60851, 60852, 57317, 60854, 
53705,  56172, 56174, 60897,60873,56419, 60853, 56000, 60896, 60898,60874);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureId`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, 
`movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
(60874, 0, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, '29266 28559'),
(60898, 0, 0, 0, 3, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
('55943', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '110846'),
('60042', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('67186', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('55942', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('55944', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', ''),
('60055', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('60900', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('67185', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('55999', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '108806'),
('56236', '0', '0', '0', '3', '0', '0', '1', '0', '0', '0', '0', '0', '0', '46598'),
('60897', '0', '0', '0', '3', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('55940', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '115672'),
('56362', '0', '0', '0', '0', '0', '0', '1', '0', '333', '0', '0', '0', '0', ''),
('56416', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '83305'),
('56476', '0', '0', '0', '0', '3', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('60895', '0', '0', '0', '1', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('56007', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('56008', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('56009', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '3', ''),
('56360', '0', '0', '0', '0', '0', '0', '1', '0', '27', '0', '0', '0', '0', ''),
('60685', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('60722', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '96733'),
('60780', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('60858', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('56013', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('56417', '0', '0', '0', '0', '0', '0', '1', '0', '27', '0', '0', '0', '0', ''),
('56418', '0', '0', '0', '0', '0', '0', '1', '0', '510', '0', '0', '0', '0', ''),
('57317', '0', '0', '0', '0', '0', '0', '1', '0', '333', '0', '0', '0', '0', ''),
('57739', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('60729', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '117496'),
('60741', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '117598'),
('60770', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '117950'),
('60834', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '46598'),
('60851', '6085100', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '131501'),
('60852', '6085200', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '131502'),
('60854', '0', '0', '0', '0', '0', '0', '1', '0', '425', '0', '0', '0', '0', ''),
('60877', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '117950'),
('60878', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '46598'),
('53705', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
(56000, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '96733'),
(60896, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
('56172', '0', '0', '0', '0', '3', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('56174', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '120703'),
('56419', '5641900', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '115672'),
('60873', '0', '0', '0', '0', '0', '4', '1', '0', '0', '0', '0', '0', '0', '29266 28559'),
('60853', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '128526');

DELETE FROM `creature_template_difficulty` WHERE `entry` IN (55943, 60042, 67186, 55942, 55944, 60055, 60900, 67185, 55999, 56236, 56362, 56416, 56476, 60895, 55940,
60858, 60780, 56360, 60722, 56007, 56008, 60685, 56009, 60729, 60741, 56417, 56418, 57739, 56013, 60877, 60878, 60770, 60834, 60851, 60852, 57317, 60854, 
53705, 56172, 56174, 60897,60873,56419, 60853, 56000, 60896, 60898);
INSERT INTO `creature_template_difficulty` VALUES
(60898, 0, 0, 0, 80, 4, 1, 1, 1, 0.01, 20571, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60896, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 20575, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(53705, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 32103, 0, 0, 0, 53705, 0, 1, 10, 10, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55940, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28872, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55942, 0, 0, 0, 80, 4, 1, 1, 1, 1, 28868, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55943, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28866, 0, 0, 0, 0, 0, 0, 0, 0, 268435712, 0, 0, 0, 0, 0, 0, 0, 56647),
(55944, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28864, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(56000, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28835, 0, 0, 0, 0, 0, 0, 0, 0, 268435712, 0, 0, 0, 0, 0, 0, 0, 56647),
(55999, 0, 0, 0, 80, 4, 1, 1, 1, 1, 28837, 0, 0, 0, 0, 0, 0, 0, 0, 268435712, 0, 0, 0, 0, 0, 0, 0, 56647),
(56007, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28821, 0, 0, 0, 56007, 56007, 0, 0, 0, 0, 0, 256, 0, 0, 0, 0, 0, 56647),
(56008, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28819, 0, 0, 0, 56008, 56008, 0, 10, 20, 0, 0, 256, 0, 0, 0, 0, 0, 56647),
(56009, 0, 0, 0, 80, 4, 4, 1, 1, 0.2, 28817, 2147483648, 0, 0, 56009, 56009, 0, 20, 30, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(56013, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28810, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(56172, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28494, 1, 0, 0, 56172, 0, 0, 0, 0, 805306368, 0, 0, 0, 0, 0, 0, 0, 56647),
(56174, 0, 0, 0, 80, 4, 0.5, 1, 1, 0.2, 28481, 0, 0, 0, 56174, 0, 56174, 3, 3, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(56236, 0, 0, 0, 80, 4, 1, 1, 1, 1, 28355, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(56360, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28122, 0, 0, 0, 56360, 56360, 0, 0, 0, 0, 0, 256, 0, 0, 0, 0, 0, 56647),
(56362, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28120, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(56416, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28022, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 262144, 0, 0, 0, 0, 0, 56647),
(56417, 0, 0, 0, 80, 4, 1, 1, 1, 1, 28020, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 262144, 0, 0, 0, 0, 0, 56647),
(56418, 0, 0, 0, 80, 4, 1, 1, 1, 1, 28018, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 262144, 0, 0, 0, 0, 0, 56647),
(56419, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 28016, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 262144, 0, 0, 0, 0, 0, 56647),
(56476, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 27911, 1024, 0, 0, 0, 0, 0, 0, 0, 64, 0, 0, 0, 0, 0, 0, 0, 56647),
(57317, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 26603, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(57739, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 26238, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60042, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 22060, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60055, 0, 0, 0, 80, 4, 1, 1, 1, 1, 22029, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60685, 0, 0, 0, 80, 4, 0.5, 1, 1, 0.2, 20951, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 256, 0, 0, 0, 0, 0, 56647),
(60722, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 20879, 0, 0, 0, 0, 0, 0, 0, 0, 256, 0, 256, 0, 0, 0, 0, 0, 56647),
(60729, 0, 0, 0, 80, 4, 1, 1, 1, 1, 20865, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60741, 0, 0, 0, 80, 4, 1, 1, 1, 1, 20831, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60770, 0, 0, 0, 80, 4, 1.2, 1, 1, 0.01, 20770, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60780, 0, 0, 0, 80, 4, 0.5, 1, 1, 0.2, 20750, 0, 0, 0, 60780, 0, 0, 0, 0, 524288, 0, 256, 0, 0, 0, 0, 0, 56647),
(60834, 0, 0, 0, 80, 4, 1.2, 1, 1, 0.2, 20680, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60851, 0, 0, 0, 80, 4, 5, 1, 1, 0.2, 20664, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60852, 0, 0, 0, 80, 4, 5, 1, 1, 0.01, 20662, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60853, 0, 0, 0, 80, 4, 3, 1, 1, 0.01, 20660, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60854, 0, 0, 0, 80, 4, 1, 1, 1, 0.01, 20658, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60858, 0, 0, 0, 80, 4, 0.5, 1, 1, 1, 20650, 0, 0, 0, 60858, 0, 0, 0, 0, 524288, 0, 256, 0, 0, 0, 0, 0, 56647),
(60873, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 20620, 0, 0, 0, 0, 0, 0, 0, 0, 268435712, 0, 0, 0, 0, 0, 0, 0, 56647),
(60877, 0, 0, 0, 80, 4, 1.2, 1, 1, 0.2, 20614, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60878, 0, 0, 0, 80, 4, 1.2, 1, 1, 0.2, 20612, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60895, 0, 0, 0, 80, 4, 1, 1, 1, 0.01, 20577, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60897, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 20573, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60900, 0, 0, 0, 80, 4, 1, 1, 1, 1, 20569, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(67185, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 61213, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(67186, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 61214, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647);

-- Quest Items
DELETE FROM creature_loot_template
WHERE (Entry, Item) IN (
    (54130, 72071),
    (55504, 74296),
    (55601, 74615),
    (55632, 74296),
    (55632, 74615),
    (57466, 74615),
    (55633, 74296),
    (55634, 74615),
    (55640, 74615),
    (56008, 74615),
    (56360, 74615),
    (55946, 74296),
    (55946, 74615),
    (56007, 74296),
    (56007, 74615),
    (60780, 74615)
);

INSERT INTO creature_loot_template
(Entry, ItemType, Item, Chance, QuestRequired, LootMode, GroupId, MinCount, MaxCount, Comment)
VALUES
(54130, 0, 72071, 85, 1, 1, 0, 1, 1, 'Stolen Training Supplies'),
(55504, 0, 74296, 100, 1, 1, 0, 1, 1, 'Stolen Carrot'),
(55601, 0, 74615, 100, 1, 1, 0, 1, 1, 'Paint Soaked Brush'),
(55632, 0, 74296, 100, 1, 1, 0, 1, 1, 'Stolen Carrot'),
(55632, 0, 74615, 100, 1, 1, 0, 1, 1, 'Paint Soaked Brush'),
(57466, 0, 74615, 100, 1, 1, 0, 1, 1, 'Paint Soaked Brush'),
(55633, 0, 74296, 100, 1, 1, 0, 1, 1, 'Stolen Carrot'),
(55634, 0, 74615, 100, 1, 1, 0, 1, 1, 'Paint Soaked Brush'),
(55640, 0, 74615, 100, 1, 1, 0, 1, 1, 'Paint Soaked Brush'),
(56008, 0, 74615, 100, 1, 1, 0, 1, 1, 'Paint Soaked Brush'),
(56360, 0, 74615, 100, 1, 1, 0, 1, 1, 'Paint Soaked Brush'),
(55946, 0, 74296, 100, 1, 1, 0, 1, 1, 'Stolen Carrot'),
(55946, 0, 74615, 100, 1, 1, 0, 1, 1, 'Paint Soaked Brush'),
(56007, 0, 74296, 100, 1, 1, 0, 1, 1, 'Stolen Carrot'),
(56007, 0, 74615, 100, 1, 1, 0, 1, 1, 'Paint Soaked Brush'),
(60780, 0, 74615, 100, 1, 1, 0, 1, 1, 'Paint Soaked Brush');

DELETE FROM `quest_offer_reward` WHERE `ID` IN (30033, 30034, 30035, 30036, 30037, 30038, 29800, 29799, 30767, 29798, 29665, 29797, 29794, 29796, 30590, 29793, 30589, 30591, 29795, 29792, 29791, 29790, 29789, 29788, 29787, 29786, 29785, 29782, 29784, 29780, 29779, 29781, 29783, 29778, 29777, 29776, 29775, 29774, 29772, 29768, 29771, 29770, 29769, 29680, 29679, 29678, 29666, 29677, 29676, 29661, 29662, 29663, 29521, 29423, 29422, 29421, 29664, 29420, 29523, 29418, 29417, 29522, 29414, 29424, 29419, 29410, 29409, 29408, 29524, 29406, 30027, 31012, 31013);
INSERT INTO `quest_offer_reward` (`ID`, `Emote1`, `Emote2`, `Emote3`, `Emote4`, `EmoteDelay1`, `EmoteDelay2`, `EmoteDelay3`, `EmoteDelay4`, `RewardText`, `VerifiedBuild`) VALUES
(30027, 1, 0, 0, 0, 0, 0, 0, 0, 'A fine weapon. It should suit your needs well.$B$BNow let''s put it to use.', 20886), -- 30027
(30033, 1, 0, 0, 0, 0, 0, 0, 0, 'A fine weapon. It should suit your needs well.$B$BNow let''s put it to use.', 20886), -- 30033
(30034, 1, 0, 0, 0, 0, 0, 0, 0, 'A fine weapon. It should suit your needs well.$B$BNow let''s put it to use.', 20886), -- 30034
(30035, 1, 0, 0, 0, 0, 0, 0, 0, 'A fine weapon. It should suit your needs well.$B$BNow let''s put it to use.', 20886), -- 30035
(30036, 1, 0, 0, 0, 0, 0, 0, 0, 'A fine weapon. It should suit your needs well.$B$BNow let''s put it to use.', 20886), -- 30036
(30037, 1, 0, 0, 0, 0, 0, 0, 0, 'A fine weapon. It should suit your needs well.$B$BNow let''s put it to use.', 20886), -- 30037
(30038, 1, 0, 0, 0, 0, 0, 0, 0, 'A fine weapon. It should suit your needs well.$B$BNow let''s put it to use.', 20886), -- 30038
(29800, 1, 0, 0, 0, 0, 0, 0, 0, 'It is good to see you again, $n. You''ve done all that I''ve asked of you and more... you have saved Shen-zin Su.$B$BYou make an old master proud.', 20886), -- 29800
(29799, 1, 0, 0, 0, 0, 0, 0, 0, 'You are a sight to see when under pressure, $n! We wouldn''t have stood a chance without you.$B$BI think it''s working. The flow of blood is slowing. I knew it would work!', 20886), -- 29799
(30767, 1, 0, 0, 0, 0, 0, 0, 0, 'The first step is done, but now we must save Shen-zin Su!', 20886), -- 30767
(29798, 1, 0, 0, 0, 0, 0, 0, 0, '<Aysa is clearly exhausted from the fight.>$B$BYour skills in battle... are unquestionable... $c.$B$B<Aysa catches her breath for a moment.>$B$BDid you save the escaped prisoners?  ...Where is Ji? Is he okay?', 20886), -- 29798
(29665, 1, 0, 0, 0, 0, 0, 0, 0, 'You are a fear to behold upon the battlefield.', 20886), -- 29665
(29797, 1, 0, 0, 0, 0, 0, 0, 0, 'These will do perfectly. I''ll get my medics to work immediately.$B$BThank you.', 20886), -- 29797
(29794, 1, 0, 0, 0, 0, 0, 0, 0, 'I have no idea who you people are, but fate surely smiled on us this day. Many more would be dead if not for the help of you and your friends. I am in your debt.', 20886), -- 29794
(29796, 1, 0, 0, 0, 0, 0, 0, 0, 'Your friend is out fighting to save my crew. Fiercely I might add. I''m afraid your message may have to wait.', 20886), -- 29796
(30590, 1, 0, 0, 0, 0, 0, 0, 0, 'More than enough. I''ve got the main control panel constructed. We should be ready very soon!', 20886), -- 30590
(29793, 1, 0, 0, 0, 0, 0, 0, 0, 'You make me proud, $n. You make all of us proud.', 20886), -- 29793
(30589, 1, 0, 0, 0, 0, 0, 0, 0, 'Korga sent you, eh? Well, I wouldn''t mind doing a number on that ship for my own reasons anyway, but it''s going to take some work.$B$BGive me a moment to think...', 20886), -- 30589
(30591, 1, 0, 0, 0, 0, 0, 0, 0, 'These survivors are in your debt, $c.', 20886), -- 30591
(29795, 1, 0, 0, 0, 0, 0, 0, 0, 'These are excellent! We''ll have them crafted into weapons in no time.', 20886), -- 29795
(29792, 1, 0, 0, 0, 0, 0, 0, 0, 'Evil is rising from the seas and filling the forest. It''s not just tigers anymore... something far more sinister lurks in the shadows, waiting to take any pandaren unwise enough to stray out alone.$B$BYou are not safe here.', 20886), -- 29792
(29791, 1, 0, 0, 0, 0, 0, 0, 0, 'You have undergone a great honor, $c. Have you come back wiser for it?', 20886), -- 29791
(29790, 1, 0, 0, 0, 0, 0, 0, 0, 'I know, $n... I know. Such is the way of things.', 20886), -- 29790
(29789, 1, 0, 0, 0, 0, 0, 0, 0, 'You come through, as you always have. You are truly one of my greatest students. I think you will be something the likes of which the world has never seen.', 20886), -- 29789
(29788, 1, 0, 0, 0, 0, 0, 0, 0, 'The spirits thank you, $c.', 20886), -- 29788
(29787, 1, 0, 0, 0, 0, 0, 0, 0, 'I do not have the strength of body or will that I once did. I''m glad there are noble pandaren like you to fight on for our people. You''ve come far, $n.', 20886), -- 29787
(29786, 1, 0, 0, 0, 0, 0, 0, 0, 'You have helped Dafeng, the spirit of air, find his courage! The final spirit has been restored.', 20886), -- 29786
(29785, 1, 0, 0, 0, 0, 0, 0, 0, 'Dafeng looks frightened as you approach, but settles a bit at the sound of your voice.', 20886), -- 29785
(29782, 1, 0, 0, 0, 0, 0, 0, 0, 'GOOD! A true challenge. Let us put this pillar to the test!', 20886), -- 29782
(29784, 1, 0, 0, 0, 0, 0, 0, 0, 'I''m impressed. Most students that are even capable of reaching this spot do so soaking wet.', 20886), -- 29784
(29780, 1, 0, 0, 0, 0, 0, 0, 0, '$n, I think you''ve got a bit of Huojin in you. Stick with me and I''ll bring you around, I think.', 20886), -- 29780
(29779, 1, 0, 0, 0, 0, 0, 0, 0, 'Invigorating! A good fight always makes me feel better!', 20886), -- 29779
(29781, 1, 0, 0, 0, 0, 0, 0, 0, '$n, I''m thinking we should celebrate with these later. What do you think?', 20886), -- 29781
(29783, 0, 0, 0, 0, 0, 0, 0, 0, 'Good.$B$BMy head, it is harder than the strongest stone.$B$BStep back, and let me dispel your doubts.', 20886), -- 29783
(29778, 1, 0, 0, 0, 0, 0, 0, 0, 'You''re a good $gman:woman;, $n. You have proven that again and again, and I am glad to stand beside you.', 20886), -- 29778
(29777, 1, 0, 0, 0, 0, 0, 0, 0, 'I am very thankful, young one. I will soon begin the slow task of documenting all of our wisdoms again, so that the scrolls can be hung for the benefit of all to come.', 20886), -- 29777
(29776, 1, 0, 0, 0, 0, 0, 0, 0, '$n, give me a hug! Our parting was not so long this time - I quite prefer it that way!$B$BYou''ve been working with Aysa, have you not? Our disciplines have always kept us apart, so I know little of her. Watching her perform her exercises now, I must confess that she possesses a grace and discipline that is strangely alluring.$B$BThere are more pressing thoughts though, aren''t there? $n, let''s take care of this monkey problem while we search for Dafeng.', 20886), -- 29776
(29775, 1, 0, 0, 0, 0, 0, 0, 0, 'You have done well, young student. Three of the four spirits have been returned to the temple. We are closer to the answers we seek.', 20886), -- 29775
(29774, 1, 0, 0, 0, 0, 0, 0, 0, 'YEAH!!! $n, we make quite a team. Persistence over planning, I always say!', 20886), -- 29774
(29772, 1, 0, 0, 0, 0, 0, 0, 0, 'Huh... well... that''s definitely the ringing I expected, but I was hoping for a bit more from Wugou. This guy''s a deep sleeper, that''s for sure.$B$BIt''s alright, $n. I''ve got another idea, and there''s not much preparation needed at all - just how I like it.', 20886), -- 29772
(29768, 1, 0, 0, 0, 0, 0, 0, 0, 'Hah, $n! Now that''s a mallet if I''ve ever seen one! Let''s make some noise.', 20886), -- 29768
(29771, 0, 0, 0, 0, 0, 0, 0, 0, 'Good.$B$BMy head, it is harder than the densest wood.$B$BStep back, and let me dispel your doubts.', 20886), -- 29771
(29770, 1, 0, 0, 0, 0, 0, 0, 0, 'Some of these have mouth-sized chunks bitten out of them! Ah well, we can still make use.$B$BThank you for your help, friend.', 20886), -- 29770
(29769, 1, 0, 0, 0, 0, 0, 0, 0, '$n, that''s the way to do it! You show up, and problems start getting solved. That''s just one of many reasons why I like you!', 20886), -- 29769
(29680, 1, 0, 0, 0, 0, 0, 0, 0, '$n! I wondered when I would see you again!$B$BWell, the plus side is that finding Wugou, the spirit of earth, was easy enough. Waking him... not so much. I think he''s going to wake quite sore from all the pushing and hitting I''ve done trying to rouse him.', 20886), -- 29680
(29679, 1, 0, 0, 0, 0, 0, 0, 0, 'I have to admit, that looked pretty fun from my angle.$B$BAnd you seem to have made a new friend.', 20886), -- 29679
(29678, 1, 0, 0, 0, 0, 0, 0, 0, 'It looks like you have his attention, $n. I think he likes you.', 20886), -- 29678
(29666, 1, 0, 0, 0, 0, 0, 0, 0, 'The water pincers are strange creatures. They seem to be drawn to still water.', 20886), -- 29666
(29677, 1, 0, 0, 0, 0, 0, 0, 0, 'Shu''s desires are not so different from our own. He does not want to be alone. Unwanted, unloved. He longs to be needed, and to share his life with others.$B$BKnowing that, earning his trust is not terribly difficult. He just wants company.', 20886), -- 29677
(29676, 1, 0, 0, 0, 0, 0, 0, 0, 'Visitors are rare up here, but you are welcome. It is nice to meet you, young $c.', 20886), -- 29676
(29661, 1, 0, 0, 0, 0, 0, 0, 0, 'Good. You''re a quick study.', 20886), -- 29661
(29662, 0, 0, 0, 0, 0, 0, 0, 0, 'Good.$B$BNo number of reeds have ever withstood my might.$B$BStep back, and let me dispel your doubts.', 20886), -- 29662
(29663, 1, 0, 0, 0, 0, 0, 0, 0, 'Even now, your skills increase. Your footing becomes more sure. Your blows strike with more force.$B$BDiscipline and practice are the keys to reaching our full potential.', 20886), -- 29663
(29521, 1, 0, 0, 0, 0, 0, 0, 0, 'It''s good to see you again, $n.$B$BThis is the pool of the skunk, as I''m sure you noticed.$B$BOver the many ages of Shen-zin Su, animals have died in some of these magical pools. Through their deaths, their spirits were infused into the waters, and anyone touching those waters will take their form.$B$BThere are several cursed pools here, some more dangerous than others.', 20886), -- 29521
(29423, 1, 0, 0, 0, 0, 0, 0, 0, '$n, you''ve done very well. Huo, the spirit of fire, is here. The temple flames are reignited. You accomplished the challenge I put before you, and with seeming ease.$B$BThere is a greater purpose in bringing Huo to the temple. It is time that I tell you more of things to come.', 20886), -- 29423
(29422, 1, 0, 0, 0, 0, 0, 0, 0, '<Through the glare, it looks like Huo is smiling. He seems to have warmed up to you.>', 20886), -- 29422
(29421, 1, 0, 0, 0, 0, 0, 0, 0, 'Remember always, the superior warrior is modest in his speech, but exceeds in his actions.', 20886), -- 29421
(29664, 1, 0, 0, 0, 0, 0, 0, 0, 'These fires will give you the strength that you''ve not yet obtained. They will illuminate your potential.', 20886), -- 29664
(29420, 1, 0, 0, 0, 0, 0, 0, 0, 'Life isn''t about finding yourself. Life is about creating yourself. There is a path before you, but you choose the trials you will face, and the trials you will overcome.', 20886), -- 29420
(29523, 1, 0, 0, 0, 0, 0, 0, 0, 'You know what it is to seize opportunity. I think you and I are kindred spirits, $n.', 20886), -- 29523
(29418, 1, 0, 0, 0, 0, 0, 0, 0, '$n! You''ve returned, roots in hand! You''re a down-to-business kind of $G guy:gal;, aren''t you? I like that!', 20886), -- 29418
(29417, 1, 0, 0, 0, 0, 0, 0, 0, 'You''ve got some fight in you! I think I like you!', 20886), -- 29417
(29522, 1, 0, 0, 0, 0, 0, 0, 0, 'Hello! You look poised and confident... I like that!$B$BI''ve heard some rather impressive whispers about you from the training grounds. If you''re half as capable as they say, I think you and I are going to be good friends!', 20886), -- 29522
(29414, 1, 0, 0, 0, 0, 0, 0, 0, 'Your path is set before you. You will be the one to rekindle the spirit of fire and bring it to the safety of the temple.$B$BThis will not be the only time you and Aysa work together. She is strong and wise. You can trust in her.', 20886), -- 29414
(29424, 1, 0, 0, 0, 0, 0, 0, 0, 'Thank you! You''re an honorable $c. They''ve taught you well.', 20886), -- 29424
(29419, 1, 0, 0, 0, 0, 0, 0, 0, 'You are too kind, $c.', 20886), -- 29419
(29410, 1, 0, 0, 0, 0, 0, 0, 0, 'You came for Aysa? You... you really shouldn''t interrupt her until she finishes her exercises. She doesn''t speak to anyone until her routine is done.$B$BIn the meantime, could you maybe help me? I had some bad luck with forest sprites.', 20886), -- 29410
(29409, 1, 0, 0, 0, 0, 0, 0, 0, 'You truly impress, $n. It seems certain that the path of the $c is indeed the path for you.', 20886), -- 29409
(29408, 1, 0, 0, 0, 0, 0, 0, 0, 'The fact that you were able to snatch the flame so easily is no small feat.$B$BThe Edict of Temperance is a scroll of wisdom passed down from my elders'' elders. Wisdom from a more peaceful time.$B$BEvery lesson has its time and place, and with darkness on the horizon, the time for this particular wisdom has passed.$B$BThe burning of the scroll is an acceptance of tidings to come and a promise to action. Your hand carried the flame, and I suspect that it will continue to do so in the future.', 20886), -- 29408
(29524, 1, 0, 0, 0, 0, 0, 0, 0, 'Intriguing, my young pupil.$B$BMost of the other trainees have been here for quite some time, but you are able to match them even within this first hour.$B$BThis speaks well of you, but there are yet other lessons I would see you learn.', 20886), -- 29524
(29406, 1, 0, 0, 0, 0, 0, 0, 0, 'Not bad, $n. You may indeed have chosen the proper path.$B$BGood. Let us continue.', 20886), -- 29406
(31012, 273, 0, 0, 0, 0, 0, 0, 0, 'Pandaren... you''re here.$B$BGood.', 18291), -- 31012
(31013, 273, 0, 0, 0, 0, 0, 0, 0, 'I know that few would be willing to leave their homeland to join the Horde. You, and the rest of your people, are welcome among our ranks.$B$BHellscream''s eyes are upon you, pandaren. Welcome to the Horde', 0); -- 31013

DELETE FROM `quest_request_items` WHERE `ID` IN (30033, 30034, 30035, 30036, 30037, 30038, 29797, 29665, 29794, 30590, 29793, 29795, 29789, 29788, 29780, 29781, 29779, 29783, 29777, 29778, 29774, 29772, 29768, 29771, 29770, 29679, 29677, 29666, 29662, 29661, 29663, 29422, 29664, 29523, 29418, 29417, 29424, 29409, 29408, 29524, 29406, 30027);
INSERT INTO `quest_request_items` (`ID`, `EmoteOnComplete`, `EmoteOnIncomplete`, `CompletionText`, `VerifiedBuild`) VALUES
(30027, 1, 0, 'Picking up your first weapon is also a symbolic gesture. Every $c that leaves these training grounds must be ready to take up arms in defense of their fellow pandaren.', 20886), -- 30027
(30033, 1, 0, 'Picking up your first weapon is also a symbolic gesture. Every $c that leaves these training grounds must be ready to take up arms in defense of their fellow pandaren.', 20886), -- 30033
(30034, 1, 0, 'Picking up your first weapon is as much a symbolic gesture as it is a physical one. Every $c that leaves these training grounds must be ready to take up arms in defense of their fellow pandaren.', 20886), -- 30034
(30035, 1, 0, 'Picking up your first weapon is also a symbolic gesture. Every $c that leaves these training grounds must be ready to take up arms in defense of their fellow pandaren.', 20886), -- 30035
(30036, 1, 0, 'Picking up your first weapon is also a symbolic gesture. Every $c that leaves these training grounds must be ready to take up arms in defense of their fellow pandaren.', 20886), -- 30036
(30037, 1, 0, 'Picking up your first weapon is also a symbolic gesture. Every $c that leaves these training grounds must be ready to take up arms in defense of their fellow pandaren.', 20886), -- 30037
(30038, 1, 0, 'Picking up your first weapon is also a symbolic gesture. Every $c that leaves these training grounds must be ready to take up arms in defense of their fellow pandaren.', 20886), -- 30038
(29797, 1, 0, 'Did you find the boxes I spoke of?', 20886), -- 29797
(29665, 1, 0, 'This is a grim site indeed.', 20886), -- 29665
(29794, 1, 0, 'This island wasn''t on any of our charts... we came through a thick mist and ploughed straight into the forest. We didn''t see it coming.', 20886), -- 29794
(30590, 1, 0, 'You''re in one piece. That''s a good sign.$B$BDid you get your hands on what I asked for?', 20886), -- 30590
(29793, 1, 0, 'In all of my years on Shen-zin Su, I''ve never witnessed something so dark as these creatures. I shudder to think of the abyss that they crawled forth from.', 20886), -- 29793
(29795, 1, 0, 'The Horde does not back down from a fight.', 20886), -- 29795
(29789, 1, 0, 'Rest feels good. The burdens of life have become a great weight after so many years.', 20886), -- 29789
(29788, 1, 0, 'It seems that everywhere I turn, the land is troubled. Your efforts have already helped to ease it''s pain, but something greater awaits you before all of this suffering truly behind to subside.', 20886), -- 29788
(29780, 1, 0, 'Following the Huojin discipline often means making the hard decisions. This is one of those moments.', 20886), -- 29780
(29781, 1, 0, 'I''m somewhat impressed they hadn''t set their village on fire with these things.', 20886), -- 29781
(29779, 1, 0, 'The time for kindness is done. We have monkeys to kill.', 20886), -- 29779
(29783, 0, 0, 'The strength of the body cannot exist without the strength of the mind.', 20886), -- 29783
(29777, 1, 0, 'I do not wish to disparage the hozen for their aspirations - each race must walk its own path. In this moment, however, they are bringing harm to our bodies and our culture. That is a line that cannot be crossed.', 20886), -- 29777
(29778, 1, 0, 'Elder Shaopai has long been responsible for documenting the great wisdoms of our people and hanging them for all to see. What amazes me is his fortitude in the light of the great sacrilege these hozen are committing. Many years of work destroyed, and yet he remains calm.$B$BI must admit, that is a quality that I do not possess.', 20886), -- 29778
(29774, 1, 0, 'If I know Shu, he''ll be happy to do this whether it works or not. It''s just his kind of fun.', 20886), -- 29774
(29772, 1, 0, 'Come on, $n! The suspense is killing me!', 20886), -- 29772
(29768, 1, 0, 'Well, they''re as warned as they''re going to get. Do you have the mallet?', 20886), -- 29768
(29771, 0, 0, 'Have you done as I''ve asked?', 20886), -- 29771
(29770, 1, 0, 'Ever since Wugou. the spirit of earth, fell asleep, the dirt is drying out and it''s getting harder and harder to get new seeds to sprout. We need to hold on to any food we can, or the island will have hard times ahead.', 20886), -- 29770
(29679, 1, 0, 'This is your task, $c. See it throug', 20886), -- 29679
(29677, 1, 0, 'The decision to not return to the pools was a hard one. I don''t think Shu ever truly understood. He does not know what it is to get older and weaker.', 20886), -- 29677
(29666, 1, 0, 'Balance is a delicate thing. It leans strongly on the wisdom of the elders. The lessons they preach are not always apparent to younger minds, so you must trust that their knowledge of the past will help us preserve the present.', 20886), -- 29666
(29662, 0, 0, 'Telling you of my strength means nothing. I must show you.', 20886), -- 29662
(29661, 1, 0, 'Have you done as I asked of you?', 20886), -- 29661
(29663, 1, 0, 'Only through discipline can we find harmony. Only through harmony can we reach our true potential as warriors.', 20886), -- 29663
(29422, 1, 0, '<Huo rests quietly within the pool.>', 20886), -- 29422
(29664, 1, 0, 'You are not yet ready to face me. First, you must light the braziers.', 20886), -- 29664
(29523, 1, 0, 'When opportunity rises, it is our responsibility to reach out and grab it. Second guessing is as good as giving up.', 20886), -- 29523
(29418, 1, 0, 'I hope you''re not shy about getting your hands dirty.', 20886), -- 29418
(29417, 1, 0, 'To not act is to abandon reason.', 20886), -- 29417
(29424, 1, 0, 'You needn''t worry about hurting the sprites. They''re really just living plants when it comes down to it - they''ll resprout later.', 20886), -- 29424
(29409, 1, 0, 'Our greatest glory is not in never falling, but in getting up every time we do. Jaomin Ro knows this. He finds strength in every defeat.$B$BYou may yet learn that lesson.', 20886), -- 29409
(29408, 1, 0, 'There will come time when you must trust your instincts and simply act.', 20886), -- 29408
(29524, 1, 0, 'There is a time for meditation and contemplation, and there is a time for action. Diving the balance between the two is one of the greatest wisdoms we seek.', 20886), -- 29524
(29406, 1, 0, 'The training targets are just down the hill. Come back to me once you''re finished.', 20886); -- 29406

DELETE FROM `quest_details` WHERE `ID` IN (30033, 30034, 30035, 30036, 30037, 30038, 31450, 29800, 29799, 30767, 29798, 29665, 29797, 29794, 29796, 30590, 29793, 30589, 29795, 30591, 29792, 29791, 29790, 29789, 29788, 29787, 29786, 29785, 29784, 29782, 29781, 29780, 29779, 29777, 29783, 29778, 29776, 29775, 29774, 29772, 29771, 29768, 29769, 29770, 29680, 29679, 29678, 29666, 29677, 29676, 29663, 29661, 29662, 29521, 29423, 29422, 29421, 29664, 29420, 29523, 29418, 29417, 29522, 29414, 29419, 29424, 29410, 29409, 29408, 29524, 29406, 30027);
INSERT INTO `quest_details` (`ID`, `Emote1`, `Emote2`, `Emote3`, `Emote4`, `EmoteDelay1`, `EmoteDelay2`, `EmoteDelay3`, `EmoteDelay4`, `VerifiedBuild`) VALUES
(30027, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 30027
(30033, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 30033
(30034, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 30034
(30035, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 30035
(30036, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 30036
(30037, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 30037
(30038, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 30038
(31450, 396, 0, 0, 0, 0, 0, 0, 0, 20886), -- 31450
(29800, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29800
(29799, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29799
(30767, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 30767
(29798, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29798
(29665, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29665
(29797, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29797
(29794, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29794
(29796, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29796
(30590, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 30590
(29793, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29793
(30589, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 30589
(29795, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29795
(30591, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 30591
(29792, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29792
(29791, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29791
(29790, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29790
(29789, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29789
(29788, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29788
(29787, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29787
(29786, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29786
(29785, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29785
(29784, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29784
(29782, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29782
(29781, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29781
(29780, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29780
(29779, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29779
(29777, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29777
(29783, 0, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29783
(29778, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29778
(29776, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29776
(29775, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29775
(29774, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29774
(29772, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29772
(29771, 0, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29771
(29768, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29768
(29769, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29769
(29770, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29770
(29680, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29680
(29679, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29679
(29678, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29678
(29666, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29666
(29677, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29677
(29676, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29676
(29663, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29663
(29661, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29661
(29662, 0, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29662
(29521, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29521
(29423, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29423
(29422, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29422
(29421, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29421
(29664, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29664
(29420, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29420
(29523, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29523
(29418, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29418
(29417, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29417
(29522, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29522
(29414, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29414
(29419, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29419
(29424, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29424
(29410, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29410
(29409, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29409
(29408, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29408
(29524, 1, 0, 0, 0, 0, 0, 0, 0, 20886), -- 29524
(29406, 1, 0, 0, 0, 0, 0, 0, 0, 20886); -- 29406
