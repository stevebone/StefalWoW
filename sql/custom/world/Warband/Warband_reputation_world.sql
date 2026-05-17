CREATE TABLE IF NOT EXISTS `warband_reputation_faction` (
  `factionId` smallint unsigned NOT NULL,
  `comment` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`factionId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- The War Within account-wide renown factions
INSERT INTO `warband_reputation_faction` (`factionId`, `comment`) VALUES
(2570, 'Hallowfall Arathi'),
(2590, 'Council of Dornogal'),
(2594, 'The Assembly of the Deeps'),
(2600, 'The Severed Threads');

--
-- Phase 10D - register all post-DF Major Factions as warband-shared.
--
-- Warband Phase 4 (commit c96412ad97) seeded `warband_reputation_faction`
-- with the four TWW 11.0-launch majors (2570, 2590, 2594, 2600). Phase 10
-- extends this to:
--   * the six Dragonflight 10.x majors that retroactively became
--     account-wide in TWW 11.0 (2503/2507/2510/2511/2564/2574),
--   * the four 11.1+ majors (2653/2658/2685/2688),
--   * the five Midnight 12.x majors (2696/2699/2704/2710/2792).
--
-- The Plunderstorm renown faction (2616 Keg Leg Thrasher) is intentionally
-- excluded: it uses a separate renown track (RenownRewardsPlunderstorm.db2)
-- and is mode-bound (Plunderstorm appearances vs Dragonflight-world
-- appearances), so it is never warband-shared.
--
-- Source for retro account-wide list: Warbands rollout, TWW 11.0.0 patch
-- notes (October 2024); per-faction confirmation in
-- C:\dumps\MAJORFACTIONS_DATA_*.md files.
--

INSERT IGNORE INTO `warband_reputation_faction` (`factionId`, `comment`) VALUES
-- Dragonflight 10.x (retroactively account-wide with TWW launch)
(2503, 'Maruuk Centaur'),
(2507, 'Dragonscale Expedition'),
(2510, 'Valdrakken Accord'),
(2511, 'Iskaara Tuskarr'),
(2564, 'Loamm Niffen'),
(2574, 'Dream Wardens'),
-- The War Within 11.1 / 11.2
(2653, 'The Cartels of Undermine'),
(2658, 'The K''aresh Trust'),
(2685, 'Gallagio Loyalty Rewards Club'),
(2688, 'Flame''s Radiance'),
-- Midnight 12.x
(2696, 'Amani Tribe'),
(2699, 'The Singularity'),
(2704, 'Hara''ti'),
(2710, 'Silvermoon Court'),
(2792, 'Ritual Sites');

CREATE TABLE IF NOT EXISTS `major_faction_config` (
  `factionId`               int unsigned NOT NULL,
  `hiddenFromExpansionPage` tinyint(1) NOT NULL DEFAULT 0,
  `displayAsJourney`        tinyint(1) NOT NULL DEFAULT 0,
  `useJourneyRewardTrack`   tinyint(1) NOT NULL DEFAULT 0,
  `useJourneyUnlockToast`   tinyint(1) NOT NULL DEFAULT 0,
  `uiPriority`              int NOT NULL DEFAULT 0,
  `introQuestId`            int unsigned NOT NULL DEFAULT 0,
  `playerCompanionId`       int unsigned NOT NULL DEFAULT 0,
  `renownCampaignId`        int unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`factionId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DELETE FROM `major_faction_config` WHERE `factionId` IN
    (2503,2507,2510,2511,2564,2574,2570,2590,2594,2600,2616,2653,2658,2685,2688,2696,2699,2704,2710,2792);

INSERT INTO `major_faction_config`
    (`factionId`,`hiddenFromExpansionPage`,`displayAsJourney`,`useJourneyRewardTrack`,`useJourneyUnlockToast`,`uiPriority`,`introQuestId`,`playerCompanionId`,`renownCampaignId`) VALUES
-- Dragonflight 10.x. renownCampaignId pulled from each JSON's `campaign.id` block.
(2503, 0, 0, 0, 0, 100, 65795, 0, 166), -- Maruuk Centaur          | DATA_2503 campaign.id=166 (Ohn'ahran Plains)
(2507, 0, 0, 0, 0, 101, 65435, 0, 197), -- Dragonscale Expedition  | DATA_2507 renownCampaignId=197 (intro Campaign 165 separate)
(2510, 0, 0, 0, 0, 102, 66705, 0, 189), -- Valdrakken Accord       | DATA_2510 campaign.id=189 (Thaldraszus)
(2511, 0, 0, 0, 0, 103, 65566, 0, 174), -- Iskaara Tuskarr         | DATA_2511 campaign.id=174 (Azure Span)
(2564, 0, 0, 0, 0, 104, 75292, 0, 203), -- Loamm Niffen            | DATA_2564 campaign.id=203 (Embers of Neltharion) - chapters unmapped in CampaignXQuestLine
(2574, 0, 0, 0, 0, 105, 76558, 0, 231), -- Dream Wardens           | DATA_2574 campaign.id=231 (Guardians of the Dream)
-- Plunderstorm 10.2.6 (no campaign - dedicated Plunderstorm mode)
(2616, 0, 1, 1, 1, 150, 78443, 0,   0), -- Keg Leg Thrasher        | DATA_2616 campaign_id=0
-- The War Within 11.0
(2570, 0, 0, 0, 0, 200, 76246, 0, 238), -- Hallowfall Arathi       | DATA_2570 campaign_id=238
(2590, 0, 0, 0, 0, 201, 76061, 0, 236), -- Council of Dornogal     | DATA_2590 campaign_id=236 (Isle of Dorn)
(2594, 0, 0, 0, 0, 202, 76365, 0, 237), -- Assembly of the Deeps   | DATA_2594 campaign_id=237 (The Ringing Deeps)
(2600, 0, 0, 0, 0, 203, 76502, 0, 239), -- Severed Threads         | DATA_2600 campaign_id=239 (Azj-Kahet)
-- The War Within 11.1 / 11.2
(2653, 0, 0, 0, 0, 210, 85115, 0, 264), -- Cartels of Undermine    | DATA_2653 campaign_id=264 (Undermine, 6 chapters)
(2688, 0, 0, 0, 0, 211, 86715, 0, 267), -- Flame's Radiance        | DATA_2688 campaign_id=267 (Rise of the Red Dawn) [shared w/ Silvermoon]
(2658, 0, 0, 0, 0, 220, 85116, 0, 268), -- K'aresh Trust           | DATA_2658 campaign_id=268 (Lorewalking: Ethereals)
(2685, 0, 0, 0, 0, 221, 85116, 0,   0), -- Gallagio Loyalty (raid) | DATA_2685 campaign_id=0 (no campaign - raid lockout only)
-- Midnight 12.0
(2696, 0, 0, 0, 0, 300, 92010, 0, 270), -- Amani Tribe             | DATA_2696 campaign_id=270 (Midnight - shared 17-chapter campaign)
(2699, 0, 0, 0, 0, 301, 92030, 0, 270), -- The Singularity         | DATA_2699 campaign_id=270 (shared)
(2704, 0, 0, 0, 0, 302, 92020, 0, 270), -- Hara'ti                 | DATA_2704 campaign_id=270 (shared)
(2710, 0, 0, 0, 0, 303, 92040, 0, 267), -- Silvermoon Court        | DATA_2710 campaign_id=267 (shared w/ Flame's Radiance)
(2792, 0, 0, 0, 0, 310, 95390, 0,   0); -- Ritual Sites (aux track)| DATA_2792 campaign_id=0 (no campaign - PTR-flagged uncertain)

CREATE TABLE IF NOT EXISTS `major_faction_renown_npc` (
  `creatureId` int unsigned NOT NULL,
  `factionId`  int unsigned NOT NULL,
  PRIMARY KEY (`creatureId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DELETE FROM `major_faction_renown_npc` WHERE `creatureId` IN
    (193658, 189226, 192880, 193030, 207195, 209664,
     215669, 215522, 215667,
     210996,
     224368, 230445, 228777, 228888,
     240010, 240020, 240030, 240040);

INSERT INTO `major_faction_renown_npc` (`creatureId`, `factionId`) VALUES
-- Dragonflight 10.x
(193658, 2503), -- Quartermaster Huseng (Maruukai, Ohn'ahran Plains) | DATA_2503 line 97
(189226, 2507), -- Cataloger Jakes (Dragonscale Basecamp, Waking Shores) | DATA_2507 line 1837
(192880, 2510), -- Lord Andestrasz (Seat of the Aspects, Valdrakken) | DATA_2510 line 102
(193030, 2511), -- Ikaarn (Iskaara, Azure Span) | DATA_2511 line 101
(207195, 2564), -- Cobaltson (Loamm, Zaralek Cavern) | DATA_2564 line 81
(209664, 2574), -- Vesith Stillsong (Bastion of Faerinaas, Emerald Dream) | DATA_2574 line 90
-- The War Within 11.0
(215669, 2570), -- Auralia Steelstrike (Mereldar, Hallowfall) | DATA_2570 line 179
(215522, 2590), -- Council of Dornogal Quartermaster (Dornogal, Isle of Dorn) | DATA_2590 line 179
(215667, 2594), -- Assembly Quartermaster (The Ringing Deeps) | DATA_2594 line 179
-- Plunderstorm 10.2.6 seasonal renown
(210996, 2616), -- Da'kash Grimledger (Plunder Isle hub) | DATA_2616 line 110
-- The War Within 11.1 / 11.2
(224368, 2653), -- Cartels of Undermine Quartermaster (Undermine) | DATA_2653 line 224
(230445, 2658), -- K'aresh Trust Quartermaster (K'aresh) | DATA_2658 line 147
(228777, 2685), -- Gallagio Loyalty Rewards Club Quartermaster | DATA_2685 line 115
(228888, 2688), -- Flame's Radiance Quartermaster (Hallowfall) | DATA_2688 line 147
-- Midnight 12.0
(240010, 2696), -- Amani Tribe Quartermaster (Zul'Aman) | DATA_2696 line 394
(240020, 2699), -- Singularity Quartermaster (Voidstorm) | DATA_2699 line 394
(240030, 2704), -- Hara'ti Quartermaster (Harandar) | DATA_2704 line 394
(240040, 2710); -- Silvermoon Court Quartermaster Caeris (Silvermoon, Saltheril's Haven) | DATA_2710 line 147

-- =====================================================================
-- 2503 Maruuk Centaur - cache item 205226 "Maruuk Centaur Supplies"
-- DATA_2503 lines 107-124 (paragon.loot[])
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` = 205226;
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(205226, 1, 191784, 100.0, 0, 1, 0, 75,  150, 'Dragon Isles Supplies bag (currency) - guaranteed'),
(205226, 0, 197764, 30.0,  0, 1, 0, 5,   15,  'Awakened Order/Earth/Air (random reagent)'),
(205226, 0, 200466, 5.0,   0, 1, 2, 1,   1,   'Dragon Shard of Knowledge - rare group'),
(205226, 0, 200563, 5.0,   0, 1, 2, 1,   3,   'Bottled Essence (random) - rare group'),
(205226, 0, 198957, 3.0,   0, 1, 1, 1,   1,   'Hoofhelper pet - cosmetic group'),
(205226, 0, 199343, 0.5,   0, 1, 3, 1,   1,   'Primal Infusion - ultra-rare crafting');

-- =====================================================================
-- 2507 Dragonscale Expedition - cache items 204378 (Brimming, 10.0.7) and 199472 (Overflowing, 10.0.0)
-- DATA_2507 lines 1906-1971 + tcSeedingSql.paragonCacheLootTemplate (lines 2138-2149)
-- We seed BOTH cache item IDs with identical loot tables since 10.0.7 retroactively replaced 199472.
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` IN (204378, 199472);
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
-- Guaranteed group 0 (currencies + crests)
(204378, 1, 2032,   100.0, 0, 1, 0, 12,  20,  'Polished Pet Charm (currency)'),
(204378, 0, 86143,  100.0, 0, 1, 0, 5,   8,   'Battle Pet Bandage'),
(204378, 0, 204079, 100.0, 0, 1, 0, 4,   4,   'Drakes Dreaming Crest'),
(204378, 1, 2003,   100.0, 0, 1, 0, 98,  98,  'Dragon Isles Supplies (currency)'),
-- Profession reagent group 1
(204378, 0, 191376, 35.0,  0, 1, 1, 1,   2,   'Awakened Profession Reagent A'),
(204378, 0, 191914, 35.0,  0, 1, 1, 1,   2,   'Awakened Profession Reagent B'),
(204378, 0, 197747, 15.0,  0, 1, 1, 1,   1,   'Awakened Profession Reagent C'),
(204378, 0, 197744, 15.0,  0, 1, 1, 1,   1,   'Awakened Profession Reagent D'),
-- Cosmetic group 2 (pets/mounts)
(204378, 0, 198725, 2.0,   0, 1, 2, 1,   1,   'Gray Marmoni pet'),
(204378, 0, 198726, 2.0,   0, 1, 2, 1,   1,   'Black Skitterbug pet'),
(204378, 0, 205146, 0.5,   0, 1, 2, 1,   1,   'Tamed Skitterfly mount (placeholder ID)'),
(204378, 0, 205147, 0.5,   0, 1, 2, 1,   1,   'Azure Skitterfly mount (placeholder ID)');

-- Mirror 204378 -> 199472 (legacy Overflowing pack, pre-10.0.7)
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`)
    SELECT 199472, `ItemType`, `Item`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`,
           CONCAT('[mirror of 204378] ', COALESCE(`Comment`,''))
    FROM `item_loot_template` WHERE `Entry` = 204378;

-- =====================================================================
-- 2510 Valdrakken Accord - cache item 205227 "Valdrakken Accord Supplies"
-- DATA_2510 lines 111-128
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` = 205227;
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(205227, 1, 191784, 100.0, 0, 1, 0, 75,  150, 'Dragon Isles Supplies bag - guaranteed'),
(205227, 0, 197764, 30.0,  0, 1, 0, 5,   15,  'Awakened Elements (random)'),
(205227, 0, 200466, 5.0,   0, 1, 2, 1,   1,   'Dragon Shard of Knowledge'),
(205227, 0, 200563, 5.0,   0, 1, 2, 1,   3,   'Bottled Essence'),
(205227, 0, 198960, 3.0,   0, 1, 1, 1,   1,   'Crystalline Whelpling pet'),
(205227, 0, 200750, 2.0,   0, 1, 1, 1,   1,   'Mass Prismatic Lasque toy'),
(205227, 0, 199343, 0.5,   0, 1, 3, 1,   1,   'Primal Infusion');

-- =====================================================================
-- 2511 Iskaara Tuskarr - cache item 205228 "Iskaara Tuskarr Supplies"
-- DATA_2511 lines 111-128
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` = 205228;
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(205228, 1, 191784, 100.0, 0, 1, 0, 75,  150, 'Dragon Isles Supplies bag'),
(205228, 0, 197764, 30.0,  0, 1, 0, 5,   15,  'Awakened Elements'),
(205228, 0, 200466, 5.0,   0, 1, 2, 1,   1,   'Dragon Shard of Knowledge'),
(205228, 0, 200563, 5.0,   0, 1, 2, 1,   3,   'Bottled Essence'),
(205228, 0, 198961, 3.0,   0, 1, 1, 1,   1,   'Hopebreaker pet'),
(205228, 0, 200959, 2.0,   0, 1, 1, 1,   1,   'Iskaara Fishing Pole toy'),
(205228, 0, 199343, 0.5,   0, 1, 3, 1,   1,   'Primal Infusion');

-- =====================================================================
-- 2564 Loamm Niffen - cache item 205229 "Loamm Niffen Supplies"
-- DATA_2564 lines 91-110 (10.1 partial mount-drop restore)
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` = 205229;
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(205229, 1, 204359, 100.0, 0, 1, 0, 3,   10,  'Bartering Boulder (currency)'),
(205229, 0, 197764, 30.0,  0, 1, 0, 5,   15,  'Awakened Elements'),
(205229, 0, 204336, 10.0,  0, 1, 0, 1,   3,   'Pterrordax Beak'),
(205229, 0, 200466, 5.0,   0, 1, 2, 1,   1,   'Dragon Shard of Knowledge'),
(205229, 0, 209589, 1.0,   0, 1, 1, 1,   1,   'Snazzy Snail mount'),
(205229, 0, 209602, 3.0,   0, 1, 1, 1,   1,   'Diggory mole machine pet'),
(205229, 0, 209601, 2.0,   0, 1, 1, 1,   1,   'Sniffenseeker toy'),
(205229, 0, 199343, 0.5,   0, 1, 3, 1,   1,   'Primal Infusion');

-- =====================================================================
-- 2574 Dream Wardens - cache item 205230 "Dream Wardens Supplies"
-- DATA_2574 lines 100-119
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` = 205230;
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(205230, 1, 204200, 100.0, 0, 1, 0, 50,  150, 'Flightstones (currency)'),
(205230, 0, 210301, 50.0,  0, 1, 0, 1,   3,   'Dreamsurge Coalescence'),
(205230, 0, 206975, 25.0,  0, 1, 0, 1,   1,   'Whelplings Awakened Crest'),
(205230, 0, 197764, 30.0,  0, 1, 0, 5,   15,  'Awakened Elements'),
(205230, 0, 204464, 10.0,  0, 1, 0, 1,   1,   'Spark of Dreams'),
(205230, 0, 210465, 1.0,   0, 1, 1, 1,   1,   'Renewing Dreamcatcher mount'),
(205230, 0, 210445, 3.0,   0, 1, 1, 1,   1,   'Wee Whimsical Whelpling pet'),
(205230, 0, 210610, 2.0,   0, 1, 1, 1,   1,   'Bursting Seed toy'),
(205230, 0, 199343, 0.5,   0, 1, 3, 1,   1,   'Primal Infusion / Concentrated PI');

-- =====================================================================
-- 2570 Hallowfall Arathi - cache item 222817 "Hallowfall Arathi Cache"
-- DATA_2570 lines 957-1017 (TWW schema; tuple format [type, id, name, chance])
-- gold (type=0 id) rolls are NOT emitted as loot rows - the loot system
-- handles gold via money fields on the source item.
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` = 222817;
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(222817, 0, 219977, 20.0,  0, 1, 1, 1,   1,   'Glimmerogg Champions Cache token'),
(222817, 0, 217907, 5.0,   0, 1, 2, 1,   1,   'Concentrated Primal Infusion'),
(222817, 0, 224784, 2.0,   0, 1, 3, 1,   1,   'Mereldar Drape transmog'),
(222817, 0, 210935, 50.0,  0, 1, 0, 8,   12,  'Encased Riftbreath reagent'),
-- Cosmetic rare group (mount/pet/toy mutually exclusive at ~1.5% combined per JSON)
(222817, 0, 2300,   1.0,   0, 1, 4, 1,   1,   'Ringel-Wing Reaper mount (paragon-only) - MountID=2300, Item.db2 ID pending'),
(222817, 0, 4459,   0.5,   0, 1, 4, 1,   1,   'Beledar Cub pet - SpeciesID=4459, Item.db2 ID pending'),
(222817, 0, 222818, 0.5,   0, 1, 4, 1,   1,   'Hallowfall Memento toy');

-- =====================================================================
-- 2590 Council of Dornogal - cache item 222818 (Paragon Cache; collides with Hallowfall toy id - they are different items)
-- DATA_2590 lines 1001-1049
-- NOTE: 222818 appears in both 2570's toy slot AND as 2590's cache. This is a
-- research data collision; ItemType=0 different objects (the toy goes IN the
-- cache, but 222818-the-cache is a CONTAINER on the same row). The container
-- variant rolls these loot rows; the toy variant is just an inventory item.
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` = 222818;
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(222818, 0, 219977, 20.0,  0, 1, 1, 1,   1,   'Glimmerogg Champions Cache token'),
(222818, 0, 217907, 5.0,   0, 1, 2, 1,   1,   'Concentrated Primal Infusion'),
(222818, 0, 210930, 50.0,  0, 1, 0, 5,   8,   'Mereldar Stone Cut reagent'),
(222818, 0, 2261,   1.0,   0, 1, 4, 1,   1,   'Dornogal Earthwalker mount (paragon-only) - MountID=2261'),
(222818, 0, 4458,   0.5,   0, 1, 4, 1,   1,   'Stone Tinkerer pet - SpeciesID=4458'),
(222818, 0, 222820, 0.5,   0, 1, 4, 1,   1,   'Council of Dornogal Banner toy');

-- =====================================================================
-- 2594 Assembly of the Deeps - cache item 222819
-- DATA_2594 lines 1023-1071
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` = 222819;
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(222819, 0, 219977, 20.0,  0, 1, 1, 1,   1,   'Glimmerogg Champions Cache token'),
(222819, 0, 217907, 5.0,   0, 1, 2, 1,   1,   'Concentrated Primal Infusion'),
(222819, 0, 210932, 50.0,  0, 1, 0, 8,   12,  'Bismuth reagent'),
(222819, 0, 2260,   1.0,   0, 1, 4, 1,   1,   'Loyal Mechgolem mount (paragon-only) - MountID=2260'),
(222819, 0, 4460,   0.5,   0, 1, 4, 1,   1,   'Wax-Sealed Wonder pet - SpeciesID=4460'),
(222819, 0, 222821, 0.5,   0, 1, 4, 1,   1,   'Glimmercog Music Box toy');

-- =====================================================================
-- 2600 Severed Threads - cache item 222822
-- DATA_2600 lines 1397-1445
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` = 222822;
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(222822, 0, 219977, 20.0,  0, 1, 1, 1,   1,   'Glimmerogg Champions Cache token'),
(222822, 0, 217907, 5.0,   0, 1, 2, 1,   1,   'Concentrated Primal Infusion'),
(222822, 0, 210936, 50.0,  0, 1, 0, 8,   12,  'Webbed Resin reagent'),
(222822, 0, 2262,   1.0,   0, 1, 4, 1,   1,   'Severed Husk Anubisath mount (paragon-only) - MountID=2262'),
(222822, 0, 4461,   0.5,   0, 1, 4, 1,   1,   'Hatched Spiderling pet - SpeciesID=4461'),
(222822, 0, 222823, 0.5,   0, 1, 4, 1,   1,   'Threadweavers Cocoon toy');

-- =====================================================================
-- 2653 Cartels of Undermine - cache item 228001
-- DATA_2653 lines 893-935
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` = 228001;
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(228001, 0, 228050, 20.0,  0, 1, 1, 1,   1,   'Slumlords Resonance Crystal token'),
(228001, 0, 227000, 50.0,  0, 1, 0, 5,   10,  'Resonant Goblin Plating reagent'),
(228001, 0, 2400,   2.0,   0, 1, 4, 1,   1,   'Gilded Whaleshark mount (paragon-only) - MountID=2400'),
(228001, 0, 4500,   1.0,   0, 1, 4, 1,   1,   'Plunder Penguin pet - SpeciesID=4500'),
(228001, 0, 228055, 0.5,   0, 1, 4, 1,   1,   'Glimmer-Gold Ingot toy');

-- =====================================================================
-- 2658 K'aresh Trust - cache item 230500
-- DATA_2658 lines 728-776 (mount drop hotfix-tuned to ~3% in 11.2.0)
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` = 230500;
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(230500, 0, 228050, 20.0,  0, 1, 1, 1,   1,   'Slumlords Resonance Crystal token'),
(230500, 0, 230502, 10.0,  0, 1, 2, 1,   1,   'Ethereal Augment Rune (Renown 18 prereq)'),
(230500, 0, 230510, 50.0,  0, 1, 0, 8,   12,  'Crystalline Vivacity reagent'),
(230500, 0, 2410,   3.0,   0, 1, 4, 1,   1,   'Karesh Drifter mount (paragon-only, restored hotfix) - MountID=2410'),
(230500, 0, 4510,   1.0,   0, 1, 4, 1,   1,   'Ethereal Pup pet - SpeciesID=4510'),
(230500, 0, 230501, 0.5,   0, 1, 4, 1,   1,   'Venaris Whisper Stone toy');

-- =====================================================================
-- 2685 Gallagio Loyalty Rewards Club - cache item 231100
-- DATA_2685 lines 784-826 (richer loot than other paragons - raid-tier)
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` = 231100;
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(231100, 0, 228050, 30.0,  0, 1, 1, 1,   1,   'Slumlords Resonance Crystal token'),
(231100, 0, 231110, 50.0,  0, 1, 0, 10,  15,  'Gilded Plating Reagent'),
(231100, 0, 2415,   2.0,   0, 1, 4, 1,   1,   'Gilded Goblin Helicrane mount - MountID=2415'),
(231100, 0, 4515,   1.0,   0, 1, 4, 1,   1,   'Lucky Roulette Wheel pet - SpeciesID=4515'),
(231100, 0, 231101, 0.5,   0, 1, 4, 1,   1,   'Gallagio VIP Pass toy');

-- =====================================================================
-- 2688 Flame's Radiance - cache item 232100
-- DATA_2688 lines 661-703
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` = 232100;
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(232100, 0, 219977, 15.0,  0, 1, 1, 1,   1,   'Glimmerogg Champions Cache token'),
(232100, 0, 210935, 40.0,  0, 1, 0, 5,   8,   'Encased Riftbreath reagent'),
(232100, 0, 2280,   1.5,   0, 1, 4, 1,   1,   'Radiant Sunwalker Wyrm mount - MountID=2280'),
(232100, 0, 4475,   1.0,   0, 1, 4, 1,   1,   'Radiant Cinderling pet - SpeciesID=4475'),
(232100, 0, 232105, 0.5,   0, 1, 4, 1,   1,   'Beledars Spark Lantern toy');

-- =====================================================================
-- 2696 Amani Tribe - cache item 240500
-- DATA_2696 lines 1018-1060 (Midnight 12.0 schema)
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` = 240500;
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(240500, 0, 240800, 20.0,  0, 1, 1, 1,   1,   'Glimmerogg Champions Cache 12.0 token'),
(240500, 0, 240510, 40.0,  0, 1, 0, 5,   10,  'Hallowed Voodoo Reagent'),
(240500, 0, 2500,   2.0,   0, 1, 4, 1,   1,   'AmaniZar War Bear mount (paragon-only) - MountID=2500'),
(240500, 0, 4600,   1.0,   0, 1, 4, 1,   1,   'Loa Cub pet - SpeciesID=4600'),
(240500, 0, 240501, 0.5,   0, 1, 4, 1,   1,   'Amani Headhunters Mask toy');

-- =====================================================================
-- 2699 Singularity - cache item 240520
-- DATA_2699 lines 1062-1104
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` = 240520;
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(240520, 0, 240800, 20.0,  0, 1, 1, 1,   1,   'Glimmerogg Champions Cache token'),
-- NOTE: reagent itemId 240520 in the JSON collides with the cache item id;
-- treated as data error; the reagent is an SQL row on Entry=240520 with Item=240520
-- which would be a self-reference. Emitted as-is per the "do not fabricate" rule
-- and flagged for follow-up in the summary.
(240520, 0, 240520, 40.0,  0, 1, 0, 5,   10,  'Void-Etched Crystal reagent (self-ref - research data error)'),
(240520, 0, 2510,   2.5,   0, 1, 4, 1,   1,   'Cosmic Void Wyrmling mount (paragon-only) - MountID=2510'),
(240520, 0, 4610,   1.0,   0, 1, 4, 1,   1,   'Singularity Mote pet - SpeciesID=4610'),
(240520, 0, 240521, 0.5,   0, 1, 4, 1,   1,   'Cosmic Void Ashwell toy');

-- =====================================================================
-- 2704 Hara'ti - cache item 240510
-- DATA_2704 lines 1106-1154
-- NOTE: 240510 collides with 2696's reagent item id. Same caveat: research
-- data error preserved as-is per spec.
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` = 240510;
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(240510, 0, 240800, 20.0,  0, 1, 1, 1,   1,   'Glimmerogg Champions Cache token'),
(240510, 0, 240515, 40.0,  0, 1, 0, 5,   10,  'Harati Verdant Sap reagent'),
(240510, 0, 2520,   2.5,   0, 1, 4, 1,   1,   'Harandar Verdant Pard mount (paragon-only) - MountID=2520'),
(240510, 0, 4620,   1.0,   0, 1, 4, 1,   1,   'Harati Sapling pet - SpeciesID=4620'),
(240510, 0, 240511, 0.5,   0, 1, 4, 1,   1,   'Fungarian Sack toy'),
(240510, 0, 240512, 0.5,   0, 1, 4, 1,   1,   'Harandar Glowvine Sconce toy');

-- =====================================================================
-- 2710 Silvermoon Court - cache item 240530
-- DATA_2710 lines 794-836
-- =====================================================================
DELETE FROM `item_loot_template` WHERE `Entry` = 240530;
INSERT INTO `item_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(240530, 0, 240800, 20.0,  0, 1, 1, 1,   1,   'Glimmerogg Champions Cache token'),
(240530, 0, 240535, 40.0,  0, 1, 0, 5,   10,  'Sindorei Resonant Crystal reagent'),
(240530, 0, 2530,   2.5,   0, 1, 4, 1,   1,   'Blood Hawkstrider Reborn mount (paragon-only) - MountID=2530'),
(240530, 0, 4630,   1.0,   0, 1, 4, 1,   1,   'Phoenix Hatchling Reborn pet - SpeciesID=4630'),
(240530, 0, 240531, 0.5,   0, 1, 4, 1,   1,   'Saltherils Soiree Invitation toy');

-- Step 1: bind quartermaster creature templates to fresh gossip menus
-- (creature_template_gossip table; one (CreatureID, MenuID) row per NPC).
DELETE FROM `creature_template_gossip` WHERE `CreatureID` IN
    (193658, 189226, 192880, 193030, 207195, 209664,
     215669, 215522, 215667, 210996,
     224368, 230445, 228777, 228888,
     240010, 240020, 240030, 240040)
  AND `MenuID` BETWEEN 70001 AND 70018;

INSERT INTO `creature_template_gossip` (`CreatureID`,`MenuID`,`VerifiedBuild`) VALUES
(193658, 70001, 0),  -- 2503 Maruuk Quartermaster Huseng
(189226, 70002, 0),  -- 2507 Cataloger Jakes
(192880, 70003, 0),  -- 2510 Lord Andestrasz
(193030, 70004, 0),  -- 2511 Ikaarn
(207195, 70005, 0),  -- 2564 Cobaltson
(209664, 70006, 0),  -- 2574 Vesith Stillsong
(215669, 70007, 0),  -- 2570 Auralia Steelstrike
(215522, 70008, 0),  -- 2590 Dornogal QM
(215667, 70009, 0),  -- 2594 Assembly QM
(210996, 70010, 0),  -- 2616 Da'kash Grimledger (Plunderstorm)
(224368, 70011, 0),  -- 2653 Cartels QM
(230445, 70012, 0),  -- 2658 K'aresh QM
(228777, 70013, 0),  -- 2685 Gallagio QM
(228888, 70014, 0),  -- 2688 Flame's Radiance QM
(240010, 70015, 0),  -- 2696 Amani QM
(240020, 70016, 0),  -- 2699 Singularity QM
(240030, 70017, 0),  -- 2704 Hara'ti QM
(240040, 70018, 0);  -- 2710 Silvermoon Caeris

-- Step 2: gossip_menu header rows. TextID 233333 is the canonical
-- "Greetings, champion. The faction stands ready." broadcast string used
-- for renown vendors retail-wide. (Faction-specific lore texts may be
-- substituted later; this is the safe generic seed.)
DELETE FROM `gossip_menu` WHERE `MenuID` BETWEEN 70001 AND 70018;
INSERT INTO `gossip_menu` (`MenuID`,`TextID`,`VerifiedBuild`) VALUES
(70001, 233333, 0),(70002, 233333, 0),(70003, 233333, 0),(70004, 233333, 0),
(70005, 233333, 0),(70006, 233333, 0),(70007, 233333, 0),(70008, 233333, 0),
(70009, 233333, 0),(70010, 233333, 0),(70011, 233333, 0),(70012, 233333, 0),
(70013, 233333, 0),(70014, 233333, 0),(70015, 233333, 0),(70016, 233333, 0),
(70017, 233333, 0),(70018, 233333, 0);

-- Step 3: gossip_menu_option - OptionNpc 53 = GossipOptionNpc::MajorFactionRenown.
DELETE FROM `gossip_menu_option` WHERE `MenuID` BETWEEN 70001 AND 70018;
INSERT INTO `gossip_menu_option`
    (`MenuID`,`GossipOptionID`,`OptionID`,`OptionNpc`,`OptionText`,`OptionBroadcastTextID`,`Language`,`Flags`,`ActionMenuID`,`ActionPoiID`,`GossipNpcOptionID`,`BoxCoded`,`BoxMoney`,`BoxText`,`BoxBroadcastTextID`,`SpellID`,`OverrideIconID`,`VerifiedBuild`) VALUES
(70001, 0, 0, 53, 'I would like to view my standing with the Maruuk Centaur.',         0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0),
(70002, 0, 0, 53, 'I would like to view my standing with the Dragonscale Expedition.', 0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0),
(70003, 0, 0, 53, 'I would like to view my standing with the Valdrakken Accord.',     0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0),
(70004, 0, 0, 53, 'I would like to view my standing with the Iskaara Tuskarr.',       0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0),
(70005, 0, 0, 53, 'I would like to view my standing with the Loamm Niffen.',          0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0),
(70006, 0, 0, 53, 'I would like to view my standing with the Dream Wardens.',         0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0),
(70007, 0, 0, 53, 'I would like to view my standing with the Hallowfall Arathi.',     0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0),
(70008, 0, 0, 53, 'I would like to view my standing with the Council of Dornogal.',   0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0),
(70009, 0, 0, 53, 'I would like to view my standing with the Assembly of the Deeps.', 0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0),
(70010, 0, 0, 53, 'Show me my Plunderstorm renown.',                                  0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0),
(70011, 0, 0, 53, 'I would like to view my standing with the Cartels of Undermine.',  0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0),
(70012, 0, 0, 53, 'I would like to view my standing with the K''aresh Trust.',        0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0),
(70013, 0, 0, 53, 'Show me my Gallagio Loyalty Rewards Club benefits.',               0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0),
(70014, 0, 0, 53, 'I would like to view my standing with Flame''s Radiance.',         0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0),
(70015, 0, 0, 53, 'I would like to view my standing with the Amani Tribe.',           0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0),
(70016, 0, 0, 53, 'I would like to view my standing with the Singularity.',           0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0),
(70017, 0, 0, 53, 'I would like to view my standing with the Hara''ti.',              0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0),
(70018, 0, 0, 53, 'I would like to view my standing with the Silvermoon Court.',      0, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 0);

-- Step 4: gossip_menu_addon - FriendshipFactionID dispatches the renown panel.
DELETE FROM `gossip_menu_addon` WHERE `MenuID` BETWEEN 70001 AND 70018;
INSERT INTO `gossip_menu_addon` (`MenuID`,`FriendshipFactionID`,`LfgDungeonsID`,`VerifiedBuild`) VALUES
(70001, 2503, 0, 0),  -- Maruuk Centaur
(70002, 2507, 0, 0),  -- Dragonscale Expedition
(70003, 2510, 0, 0),  -- Valdrakken Accord
(70004, 2511, 0, 0),  -- Iskaara Tuskarr
(70005, 2564, 0, 0),  -- Loamm Niffen
(70006, 2574, 0, 0),  -- Dream Wardens
(70007, 2570, 0, 0),  -- Hallowfall Arathi
(70008, 2590, 0, 0),  -- Council of Dornogal
(70009, 2594, 0, 0),  -- Assembly of the Deeps
(70010, 2616, 0, 0),  -- Keg Leg Thrasher (Plunderstorm)
(70011, 2653, 0, 0),  -- Cartels of Undermine
(70012, 2658, 0, 0),  -- K'aresh Trust
(70013, 2685, 0, 0),  -- Gallagio Loyalty Rewards Club
(70014, 2688, 0, 0),  -- Flame's Radiance
(70015, 2696, 0, 0),  -- Amani Tribe
(70016, 2699, 0, 0),  -- Singularity
(70017, 2704, 0, 0),  -- Hara'ti
(70018, 2710, 0, 0);  -- Silvermoon Court

-- ---------------------------------------------------------------------
-- pool_template rows: numActive = number of weekly variants live at once
-- ---------------------------------------------------------------------
DELETE FROM `quest_pool_template` WHERE `poolId` BETWEEN 7001 AND 7019;
INSERT INTO `quest_pool_template` (`poolId`,`numActive`,`description`) VALUES
-- Dragonflight 10.x faction weeklies (Aiding the Accord variants - 2507 only)
(7001, 1, 'Dragonscale Expedition: Aiding the Accord weekly rotation (8 variants)'),
-- TWW 11.0 faction-specific weeklies (one per faction)
(7007, 1, 'Hallowfall Arathi: Spreading the Light + Beledar (2 variants)'),
(7008, 1, 'Council of Dornogal: Theater Troupe weekly (single variant)'),
(7009, 1, 'Assembly of the Deeps: Awakening the Machine weekly (single variant)'),
(7010, 1, 'Severed Threads: Pact of General/Vizier/Weaver rotation (3 variants, 1 live per reset)'),
-- TWW 11.1 / 11.2
(7011, 1, 'Cartels of Undermine: Side Gig + Reefwalker (2 variants)'),
(7012, 1, 'K''aresh Trust: Project Eco-Dome + Ethereal Plane Tracker (2 variants)'),
(7013, 1, 'Gallagio Loyalty Rewards Club: Liberation of Undermine first-clear (raid weekly)'),
(7014, 1, 'Flame''s Radiance: Nightfall + Repelling Tides (2 variants)'),
-- Midnight 12.x
(7015, 1, 'Amani Tribe: Abundance weekly (single variant)'),
(7016, 1, 'The Singularity: Stormarion Assault weekly (single variant)'),
(7017, 1, 'Hara''ti: Legends of the Haranir weekly (single variant)'),
(7018, 1, 'Silvermoon Court: Saltheril''s Soirees + Quel''Danas (2 variants)'),
-- Cross-faction shared weeklies
(7019, 1, 'Shared TWW/Midnight universal weekly dungeon (Biergoth 82898 / Brightwing 92600 / dispatcher quest pool)');

-- ---------------------------------------------------------------------
-- quest_pool_members: questId -> poolId mapping.
-- ---------------------------------------------------------------------
DELETE FROM `quest_pool_members` WHERE `poolId` BETWEEN 7001 AND 7019;

-- 7001 Dragonscale Expedition (DATA_2507 lines 1972-2007, "Aiding the Accord" + variants).
-- Each variant rotates weekly; indices 0..7 cycle.
INSERT INTO `quest_pool_members` (`questId`,`poolId`,`poolIndex`,`description`) VALUES
(72068, 7001, 0, 'Aiding the Accord (DF10) [2507 weekly variant 0]'),
(72782, 7001, 1, 'Aiding the Accord: A Feast For All [2507 weekly variant 1]'),
(73055, 7001, 2, 'Aiding the Accord: A Tale of Two Tarrasques [2507 weekly variant 2]'),
(75259, 7001, 3, 'Aiding the Accord: A Worthy Ally [2507 weekly variant 3]'),
(73294, 7001, 4, 'Aiding the Accord: Storms Brewing [2507 weekly variant 4]'),
(72651, 7001, 5, 'Aiding the Accord: The Hunt is On [2507 weekly variant 5]'),
(73061, 7001, 6, 'Aiding the Accord: Trial of Elements [2507 weekly variant 6]'),
(73062, 7001, 7, 'Aiding the Accord: Trial of Flood [2507 weekly variant 7]');

-- 7007 Hallowfall Arathi (DATA_2570 lines 1020-1040).
INSERT INTO `quest_pool_members` (`questId`,`poolId`,`poolIndex`,`description`) VALUES
(82938, 7007, 0, 'Spreading the Light (Hallowfall weekly) [2570]'),
(79984, 7007, 1, 'Hallowfall Beledar weekly [2570]');

-- 7008 Council of Dornogal (DATA_2590 lines 1052-1058).
INSERT INTO `quest_pool_members` (`questId`,`poolId`,`poolIndex`,`description`) VALUES
(81381, 7008, 0, 'Theater Troupe (Dornogal weekly) [2590]');

-- 7009 Assembly of the Deeps (DATA_2594 lines 1074-1080).
INSERT INTO `quest_pool_members` (`questId`,`poolId`,`poolIndex`,`description`) VALUES
(82511, 7009, 0, 'Awakening the Machine (Ringing Deeps weekly) [2594]');

-- 7010 Severed Threads Pact rotation (DATA_2600 lines 1448-1467).
-- Pact rotation: one Pact variant active each weekly reset (the player chooses
-- their Pact, the other two are locked) - hence numActive=1 on the 3-member
-- pool: the quest pool system will pick one index per cycle.
INSERT INTO `quest_pool_members` (`questId`,`poolId`,`poolIndex`,`description`) VALUES
(80571, 7010, 0, 'Pact of the General [2600 Severed Threads]'),
(80572, 7010, 1, 'Pact of the Vizier [2600 Severed Threads]'),
(80573, 7010, 2, 'Pact of the Weaver [2600 Severed Threads]');

-- 7011 Cartels of Undermine (DATA_2653 lines 937-957).
INSERT INTO `quest_pool_members` (`questId`,`poolId`,`poolIndex`,`description`) VALUES
(85120, 7011, 0, 'Side Gig (rotating cartel) [2653 Undermine]'),
(85220, 7011, 1, 'Reefwalker tracker (world boss weekly) [2653 Undermine]');

-- 7012 K'aresh Trust (DATA_2658 lines 778-799).
INSERT INTO `quest_pool_members` (`questId`,`poolId`,`poolIndex`,`description`) VALUES
-- NOTE: K'aresh and Undermine both list quest_id 85120 - K'aresh attaches it to
-- "Project Eco-Dome", Undermine attaches it to "Side Gig". Researcher data
-- error (likely a JSON copy-paste). Since quest_pool_members.questId is
-- PRIMARY KEY, we cannot duplicate. Omit 85120 here; the row above in pool
-- 7011 wins. Kareh's Ethereal Plane Tracker (85230) is the canonical 2658 row.
(85230, 7012, 0, 'Ethereal Plane Tracker [2658 K''aresh]');

-- 7013 Gallagio Loyalty Rewards Club raid weekly (DATA_2685 line 831).
INSERT INTO `quest_pool_members` (`questId`,`poolId`,`poolIndex`,`description`) VALUES
(86200, 7013, 0, 'Liberation of Undermine first-clear bonus [2685 Gallagio raid weekly]');

-- 7014 Flame's Radiance (DATA_2688 lines 705-719).
INSERT INTO `quest_pool_members` (`questId`,`poolId`,`poolIndex`,`description`) VALUES
(86730, 7014, 0, 'Nightfall scenario first weekly clear bonus [2688 Flame''s Radiance]'),
(86740, 7014, 1, 'Repelling Tides weekly [2688 Flame''s Radiance]');

-- 7015 Amani Tribe (DATA_2696 lines 1062-1068).
INSERT INTO `quest_pool_members` (`questId`,`poolId`,`poolIndex`,`description`) VALUES
(92520, 7015, 0, 'Abundance (Midnight weekly) [2696 Amani Tribe]');

-- 7016 The Singularity (DATA_2699 lines 1106-1112).
INSERT INTO `quest_pool_members` (`questId`,`poolId`,`poolIndex`,`description`) VALUES
(92530, 7016, 0, 'Stormarion Assault (Midnight weekly) [2699 Singularity]');

-- 7017 Hara'ti (DATA_2704 lines 1156-1162).
INSERT INTO `quest_pool_members` (`questId`,`poolId`,`poolIndex`,`description`) VALUES
(92540, 7017, 0, 'Legends of the Haranir (weekly) [2704 Hara''ti]');

-- 7018 Silvermoon Court (DATA_2710 lines 838-858).
INSERT INTO `quest_pool_members` (`questId`,`poolId`,`poolIndex`,`description`) VALUES
(92550, 7018, 0, 'Saltheril''s Soirees (weekly) [2710 Silvermoon Court]'),
(92560, 7018, 1, 'March on Quel''Danas (weekly raid prelude) [2710 Silvermoon Court]');

-- 7019 Cross-faction shared TWW/Midnight universal weekly dungeon.
-- Per the JSON data: 82898 is Auditor Biergoth's dispatch (TWW 11.0/11.1/11.2);
-- 92600 is Halduron Brightwing's Midnight equivalent. Both feed rep to a
-- player-chosen faction, hence single shared pool.
INSERT INTO `quest_pool_members` (`questId`,`poolId`,`poolIndex`,`description`) VALUES
(82898, 7019, 0, 'Auditor Biergoth''s Dungeon Quest (TWW universal weekly dungeon)'),
(82900, 7019, 1, 'Khaz Algar Bounty Hunter (TWW weekly world bounty)'),
(92600, 7019, 2, 'Halduron Brightwing''s Dungeon Quest (Midnight universal weekly dungeon)');