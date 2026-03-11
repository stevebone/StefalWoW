
DELETE FROM `spell_proc` WHERE `SpellId` IN (438590);

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_keep_your_feet_on_the_ground');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(438590, 'spell_warr_keep_your_feet_on_the_ground');

--
-- Pet Battle NPC Trainer Teams - Initial seed data
-- Source: Wowhead NPC pages, creature IDs verified against TDB creature_template
-- All 57 pet creature entries confirmed present with matching names.
--
-- Schema: (npcEntry, slot, speciesId, level, breedId, quality, ability1, ability2, ability3, npcTeamMemberID, creatureId)
-- quality=3 (Rare); breedId=3 (default); npcTeamMemberID=0 (unused)
--

DELETE FROM `battle_pet_npc_team`;

INSERT INTO `battle_pet_npc_team`
    (`npcEntry`, `slot`, `speciesId`, `level`, `breedId`, `quality`, `ability1`, `ability2`, `ability3`, `npcTeamMemberID`, `creatureId`)
VALUES
-- Old MacDonald (65648) -- Westfall, level 3
(65648, 0, 874,  3, 3, 3, 119, 360, 283, 0, 65659),  -- Teensy (Critter)
(65648, 1, 875,  3, 3, 3, 112, 524, 581, 0, 65660),  -- Clucks (Flying)
(65648, 2, 876,  3, 3, 3, 384, 389, 390, 0, 65661),  -- Foe Reaper 800 (Mechanical)

-- Steven Lisbane (63194) -- Northern Stranglethorn, level 9
(63194, 0, 885,  9, 3, 3, 349, 124, 354, 0, 65677),  -- Nanners (Beast)
(63194, 1, 884,  9, 3, 3, 429, 535, 536, 0, 66983),  -- Moonstalker (Beast)
(63194, 2, 883,  9, 3, 3, 432, 538, 431, 0, 66982),  -- Emeralda (Magic)

-- Elena Flutterfly (66412) -- Moonglade, level 17
(66412, 0, 926, 17, 3, 3, 115, 122, 347, 0, 66417),  -- Willow (Dragonkin)
(66412, 1, 925, 17, 3, 3, 114, 460, 463, 0, 66416),  -- Beacon (Magic)
(66412, 2, 924, 17, 3, 3, 420, 506, 162, 0, 66414),  -- Lacewing (Flying)

-- Cassandra Kaboom (66422) -- Southern Barrens, level 11
(66422, 0, 907, 11, 3, 3, 455, 208, 278, 0, 66427),  -- Whirls (Mechanical)
(66422, 1, 908, 11, 3, 3, 777, 640, 645, 0, 66428),  -- Cluckatron (Mechanical)
(66422, 2, 909, 11, 3, 3, 389, 390, 392, 0, 66429),  -- Gizmo (Mechanical)

-- Zoltan (66442) -- Felwood, level 16
(66442, 0, 921, 16, 3, 3, 406, 409, 407, 0, 66443),  -- Ultramus (Magic)
(66442, 1, 922, 16, 3, 3, 473, 474, 475, 0, 66444),  -- Beamer (Magic)
(66442, 2, 923, 16, 3, 3, 202, 208, 644, 0, 66445),  -- Hatewalker (Mechanical)

-- Kortas Darkhammer (66515) -- Searing Gorge, level 15
(66515, 0, 939, 15, 3, 3, 115, 168, 122, 0, 66490),  -- Garnestrasz (Dragonkin)
(66515, 1, 937, 15, 3, 3, 393, 256, 792, 0, 66488),  -- Obsidion (Dragonkin)
(66515, 2, 938, 15, 3, 3, 525, 597, 598, 0, 66489),  -- Veridia (Dragonkin)

-- Everessa (66518) -- Swamp of Sorrows, level 16
(66518, 0, 941, 16, 3, 3, 110, 156, 152, 0, 66492),  -- Anklor (Beast)
(66518, 1, 942, 16, 3, 3, 233, 228, 232, 0, 66493),  -- Croaker (Aquatic)
(66518, 2, 943, 16, 3, 3, 507, 504, 162, 0, 66494),  -- Dampwing (Flying)

-- Lydia Accoste (66522) -- Deadwind Pass, level 19
(66522, 0, 947, 19, 3, 3, 422, 657, 121, 0, 66498),  -- Nightstalker (Undead)
(66522, 1, 948, 19, 3, 3, 210, 592, 476, 0, 66499),  -- Bishibosh (Undead)
(66522, 2, 949, 19, 3, 3, 318, 303, 398, 0, 66500),  -- Jack (Elemental)

-- Narrok (66552) -- Nagrand (Outland), level 22
(66552, 0, 956, 22, 3, 3, 571, 377, 375, 0, 66537),  -- Stompy (Beast)
(66552, 1, 957, 22, 3, 3, 367, 165, 253, 0, 66538),  -- Dramaticus (Critter)
(66552, 2, 958, 22, 3, 3, 233, 228, 232, 0, 66539),  -- Prince Wart (Aquatic)

-- Bloodknight Antari (66557) -- Shadowmoon Valley, level 24
(66557, 0, 962, 24, 3, 3, 608, 764, 751, 0, 66543),  -- Netherbite (Dragonkin)
(66557, 1, 963, 24, 3, 3, 113, 178, 179, 0, 66544),  -- Jadefire (Elemental)
(66557, 2, 964, 24, 3, 3, 484, 486, 488, 0, 66545),  -- Arcanus (Magic)

-- Nearly Headless Jacob (66636) -- Crystalsong Forest, level 25
(66636, 0, 968, 25, 3, 3, 654, 442, 212, 0, 66616),  -- Mort (Undead)
(66636, 1, 969, 25, 3, 3, 256, 471, 650, 0, 66618),  -- Stitch (Undead)
(66636, 2, 970, 25, 3, 3, 218, 468, 780, 0, 66619),  -- Spooky Strangler (Undead)

-- Gutretch (66639) -- Zul'Drak, level 25
(66639, 0, 974, 25, 3, 3, 360, 253, 359, 0, 66624),  -- Blight (Critter)
(66639, 1, 975, 25, 3, 3, 369, 364, 160, 0, 66626),  -- Fleshrender (Beast)
(66639, 2, 976, 25, 3, 3, 369, 364, 159, 0, 66627),  -- Cadavus (Beast)

-- Hyuna of the Shrines (66730) -- Jade Forest, level 25
(66730, 0, 992, 25, 3, 3, 310, 376, 123, 0, 66709),  -- Dor the Wall (Aquatic)
(66730, 1, 993, 25, 3, 3, 155, 156, 159, 0, 66710),  -- Fangor (Beast)
(66730, 2, 994, 25, 3, 3, 632, 420, 270, 0, 66711),  -- Skyshaper (Flying)

-- Mo'ruk (66733) -- Krasarang Wilds, level 25
(66733, 0, 1000, 25, 3, 3, 504, 507, 508, 0, 66714),  -- Lightstalker (Flying)
(66733, 1, 999,  25, 3, 3, 376, 249, 566, 0, 66713),  -- Needleback (Aquatic)
(66733, 2, 998,  25, 3, 3, 369, 160, 159, 0, 66712),  -- Woodcarver (Beast)

-- Farmer Nishi (66734) -- Valley of the Four Winds, level 25
(66734, 0, 995, 25, 3, 3, 369, 160, 159, 0, 66715),  -- Brood of Mothallus (Critter)
(66734, 1, 997, 25, 3, 3, 268, 753, 404, 0, 66718),  -- Siren (Elemental)
(66734, 2, 996, 25, 3, 3, 745, 298, 828, 0, 66716),  -- Toothbreaker (Elemental)

-- Courageous Yon (66738) -- Kun-Lai Summit, level 25
(66738, 0, 1001, 25, 3, 3, 539, 541, 163, 0, 66719),  -- Bleat (Beast)
(66738, 1, 1002, 25, 3, 3, 360, 162, 159, 0, 66720),  -- Lapin (Critter)
(66738, 2, 1003, 25, 3, 3, 524, 170, 581, 0, 66721),  -- Piqua (Flying)

-- Wastewalker Shu (66739) -- Dread Wastes, level 25
(66739, 0, 1009, 25, 3, 3, 511, 509, 513, 0, 66724),  -- Crusher (Aquatic)
(66739, 1, 1007, 25, 3, 3, 315, 158, 566, 0, 66722),  -- Mutilator (Beast)
(66739, 2, 1008, 25, 3, 3, 453, 814, 644, 0, 66723),  -- Pounder (Elemental)

-- Aki the Chosen (66741) -- Vale of Eternal Blossoms, level 25
(66741, 0, 1010, 25, 3, 3, 509, 283, 564, 0, 66725),  -- Whiskers (Aquatic)
(66741, 1, 1011, 25, 3, 3, 204, 347, 122, 0, 66726),  -- Stormlash (Dragonkin)
(66741, 2, 1012, 25, 3, 3, 706, 573, 298, 0, 66728),  -- Chirrup (Critter)

-- Bordin Steadyfist (66815) -- Deepholm, level 25
(66815, 0, 985, 25, 3, 3, 263, 621, 617, 0, 66805),  -- Ruby (Elemental)
(66815, 1, 984, 25, 3, 3, 155, 193, 519, 0, 66804),  -- Crystallus (Critter)
(66815, 2, 983, 25, 3, 3, 484, 488, 606, 0, 66802);  -- Fracture (Elemental)

