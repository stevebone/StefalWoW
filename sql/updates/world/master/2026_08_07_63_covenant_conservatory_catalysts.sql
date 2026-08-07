--
-- Queen's Conservatory - CATALYSTS and per-catalyst-combination YIELD (Night Fae unique sanctum feature).
--
-- WHAT THIS FILE FIXES
-- --------------------
-- 2026_08_07_60 and QueensConservatory.h named GameObjects 353652 "Catalyst of Power" / 353653 "Catalyst of
-- Renewal" / 353654 "Catalyst of Might" as the Conservatory catalysts. They are NOT. Verified against this
-- build's data:
--   * `integ_world.gameobject_template` gives them displayIds 64892 / 64893 / 64894, which resolve through
--     GameObjectDisplayInfo to FileDataIDs 3153293 / 3153291 / 3153299 =
--         world/expansion08/doodads/vampire/9vm_vampire_bottle03.m2
--         world/expansion08/doodads/vampire/9vm_vampire_bottle01.m2
--         world/expansion08/doodads/vampire/9vm_vampire_bottle03_empty01.m2
--     i.e. Revendreth vampire bottles, and all three sit in AreaTable 10413 Revendreth (map 2222). The
--     Queen's Conservatory is AreaTable 13367 on map 2363. Coincidental name, wrong expansion zone.
--   * They are GAMEOBJECT_TYPE_GOOBER with Data0 (lockId) 43 and zero spawns.
-- Consequence in the shipped engine: `.garrison conservatory catalyst` validated an arbitrary
-- gameobject_template entry, consumed nothing and changed nothing that any later code read - a silent no-op.
--
-- THE REAL CATALYSTS ARE ITEMS  [CLIENT - 12.0.7.68275 M:\WorldofWarcraft\dbc\enUS]
-- ---------------------------------------------------------------------------------
-- Sibling set of the Spirit items authored by 2026_08_07_62; they share the Use spell 323169
-- "Infuse Catalyst" ("Infuse an Anima Catalyst Plot with this catalyst."). Each id was checked to exist in
-- BOTH Item.db2 (176832 resolves through the section-0 copy table, not the id list) and ItemSparse.db2,
-- which is exactly the pair ObjectMgr::LoadItemTemplates() joins, so every catalystItemId below resolves to
-- a real sObjectMgr->GetItemTemplate().
--
--   176921 "Temporal Leaves"      spell 323725 (SpellName.db2 verified)
--       ItemSparse.Description_lang: "These enchanted leaves reduce the Wildseed of Regrowth process by
--       1 day."                                        -> effectType 1 TIME_DELTA, effectValue -86400 s.
--   176922 "Wild Nightbloom"      spell 323787 (SpellName.db2 verified)
--       "This magical bloom increases the yield of crafting materials offered by reborn spirits and
--       catalysts grown from catalyst seeds by 100%."   -> effectType 3 YIELD_QUANTITY (satchel size).
--   176832 "Wildseed Root Grain"  spell 336307 (SpellName.db2 verified)
--       "This special root increases the quality of rewards offered by a spirit upon rebirth."
--                                                      -> effectType 2 YIELD_QUALITY (satchel tier).
--
-- Also [CLIENT]: the Wildseed of Regrowth (creature_template 165466) and the Anima Catalyst Plot
-- (creature_template 165480) are CREATURES, not GameObjects - a second reason the old GameObject-entry
-- validation could not model the real system.
--
-- THE YIELD LADDER
-- ----------------
-- [WEB] Wowhead, "Night Fae Covenant Queen's Conservatory" guide, Rewards table:
--     no catalyst        -> Novice's Satchel
--     1x Root Grain      -> Journeyman's Satchel  + tier-1 weapon appearances
--     2x / 3x Root Grain -> Artisan's Satchel     + tier-2 appearances, pets and mounts
--     4x Root Grain      -> Spirit-Tender's Satchel + Spirit Tender's Pack + top mounts
--     1x / 2x / 3x Wild Nightbloom -> satchel size upgraded to Large / Stuffed / Overflowing
-- [DB, derivable] `integ_world.gameobject_loot_template` 350978 flattens all 40 outcomes into ONE table, so
-- today every harvest rolls every satchel tier and size regardless of catalysts. Its twelve satchel rows are
-- exactly a (quality tier x size) grid, and the grid cells are named unambiguously, so each
-- (rootGrainCount, nightbloomCount) pair maps to exactly one satchel item with no guessing:
--
--            nightbloom 0        nightbloom 1              nightbloom 2                nightbloom 3
--   rg 0     180974 Novice's     180981 Novice's Large     180985 Novice's Stuffed     180989 Novice's Overflow.
--   rg 1     180975 Journeym.    180980 Journeym. Large    180984 Journeym. Stuffed    180988 Journeym. Overflow.
--   rg 2     180976 Artisan's    180979 Artisan's Large    180983 Artisan's Stuffed    - 5 links, unreachable -
--   rg 3     180976 Artisan's    180979 Artisan's Large    - 5 links, unreachable -    - 6 links, unreachable -
--   rg 4     180977 Sp.-Tender's - 5 links, unreachable -  - 6 links, unreachable -    - 7 links, unreachable -
--
-- The empty cells are not gaps. A pod holds at most FOUR catalyst links (talent 1090 "Final Forms": the last
-- wildseed "can benefit from four possible catalyst links"), so any cell with rootGrain + nightbloom > 4 can
-- never occur. And that bound explains the item data exactly: ItemSparse.db2 was scanned for every item whose
-- name contains "Satchel", and the twelve above are the COMPLETE Conservatory satchel set in this build -
-- there is no "Artisan's Overflowing Satchel" and no "Spirit-Tender's Large/Stuffed/Overflowing Satchel".
-- The set of satchels Blizzard shipped is precisely the set of combinations four links can produce, which is
-- independent corroboration of both the 4-link cap and the tier/size reading of the two catalysts.
-- So all fourteen reachable cells are assigned, none is guessed, and none is missing.
-- QueensConservatory::AttachCatalyst still refuses any attach whose resulting set has no yield row
-- (CONSERVATORY_ERROR_NO_YIELD_FOR_COMBINATION), so a pod can never be left unharvestable if this data is
-- later edited.
--
-- WHAT IS NOT ASSIGNABLE, AND IS THEREFORE LEFT UNGATED
-- -----------------------------------------------------
-- 350978's other 28 rows are 12 weapons, 10 pets/mounts, 2 quest-required Souls and the 3 "Spirit Tender's"
-- items. The guide's phrases "tier-1 weapon appearances" and "tier-2 appearances" name no items, and neither
-- the item names nor their quality in `gameobject_loot_template` separate them into tiers. So 25 of those
-- rows are copied VERBATIM (same Chance/LootMode/GroupId/Min/MaxCount) into every per-outcome table: their
-- gating is unknown, and leaving them exactly as they are today cannot make anything unobtainable.
-- The exception is the three items literally named for the top tier -
--     181302 Spirit Tender's Branches, 181306 Spirit Tender's Bulb, 181310 Spirit Tender's Pack
-- - which are written only into the rootGrain=4 tables, matching the guide's explicit
-- "4x Root Grain -> Spirit-Tender's Satchel + Spirit Tender's Pack". Branches and Bulb are assigned by the
-- shared "Spirit Tender's" name family (the same tier name as the 180977 satchel), not by a cited source.
--
-- Loot ids are NOT invented ad hoc: entry = 35097800 + rootGrainCount*10 + nightbloomCount, a deterministic
-- function of the source table id 350978 and the outcome coordinates. The whole block 35097800-35097899 was
-- verified free of any existing `gameobject_loot_template` and `gameobject_template` row in `integ_world`.
-- Source table 350978 is left untouched - the physical "Queen's Conservatory Cache" chest still uses it, and
-- an empty `garrison_conservatory_yield` makes the engine fall back to it exactly as before.
--
-- Idempotent.
--

CREATE TABLE IF NOT EXISTS `garrison_conservatory_catalyst` (
  `catalystItemId` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Item consumed to link the catalyst to a wildseed pod',
  `effectType`     TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '1 TIME_DELTA, 2 YIELD_QUALITY, 3 YIELD_QUANTITY',
  `effectValue`    INT NOT NULL DEFAULT 0 COMMENT 'TIME_DELTA: seconds added to maturesAt (negative shortens). YIELD_*: unused, the linked count is what matters',
  `maxPerPlot`     TINYINT UNSIGNED NOT NULL DEFAULT 4 COMMENT 'How many of THIS catalyst one pod may hold',
  `spellId`        INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Provenance only: the aura whose description defines the effect',
  `comment`        VARCHAR(255) NULL,
  PRIMARY KEY (`catalystItemId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Queens Conservatory catalyst items (see 2026_08_07_63)';

CREATE TABLE IF NOT EXISTS `garrison_conservatory_yield` (
  `spiritItemId`    INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = applies to every spirit; otherwise garrison_conservatory_wildseed.wildseedEntry',
  `rootGrainCount`  TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Number of YIELD_QUALITY catalysts linked to the pod',
  `nightbloomCount` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Number of YIELD_QUANTITY catalysts linked to the pod',
  `lootId`          INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'gameobject_loot_template.Entry rolled on harvest',
  `comment`         VARCHAR(255) NULL,
  PRIMARY KEY (`spiritItemId`,`rootGrainCount`,`nightbloomCount`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Queens Conservatory catalyst combination -> harvest loot table';

-- maxPerPlot: Wild Nightbloom is capped at 3 because the size ladder it drives has exactly three named steps
-- (Large / Stuffed / Overflowing) and no fourth satchel size exists in this build's item data. Root Grain is
-- capped at 4 by the guide's own top step (4x). Temporal Leaves has no sourced cap, so it takes the pod's own
-- structural maximum of 4 links.
DELETE FROM `garrison_conservatory_catalyst` WHERE `catalystItemId` IN (176921,176922,176832);
INSERT INTO `garrison_conservatory_catalyst`
  (`catalystItemId`,`effectType`,`effectValue`,`maxPerPlot`,`spellId`,`comment`) VALUES
(176921, 1, -86400, 4, 323725, 'Temporal Leaves - reduce the Wildseed of Regrowth process by 1 day'),
(176922, 3,      0, 3, 323787, 'Wild Nightbloom - increases the yield of crafting materials by 100% (satchel size)'),
(176832, 2,      0, 4, 336307, 'Wildseed Root Grain - increases the quality of rewards offered by a spirit upon rebirth (satchel tier)');

DELETE FROM `gameobject_loot_template` WHERE `Entry` BETWEEN 35097800 AND 35097899;

-- 35097800 : rootGrain=0 nightbloom=0 -> Novice's Satchel (guaranteed) + 25 ungated cosmetic rows
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(35097800,0,180974,100,0,3,0,1,1,'Novice''s Satchel'),
(35097800,0,179499,0.397068,0,3,0,1,1,'Nightwillow Barb'),
(35097800,0,179563,0.256597,0,3,0,1,1,'Heartwood Stem'),
(35097800,0,179585,0.215494,0,3,0,1,1,'Nightwillow Shortbow'),
(35097800,0,180414,0.330823,0,3,0,1,1,'Wakener''s Runestag'),
(35097800,0,180603,0.618947,0,3,0,1,1,'Violet Dredwing Pup'),
(35097800,0,180628,0.742257,0,3,0,1,1,'Pearlwing Heron'),
(35097800,0,180639,0.729088,0,3,0,1,1,'Dusty Sporeflutterer'),
(35097800,0,180723,0.219884,0,3,0,1,1,'Enchanted Wakener''s Runestag'),
(35097800,0,180814,0.572655,0,3,0,1,1,'Sable'),
(35097800,0,180815,0.552702,0,3,0,1,1,'Brightscale Hatchling'),
(35097800,0,180954,0.256597,0,3,0,1,1,'Crypt Watcher''s Spire'),
(35097800,0,180956,0.229062,0,3,0,1,1,'Axeblade Blunderbuss'),
(35097800,0,180963,0.381903,0,3,0,1,1,'Crypt Keeper''s Vessel'),
(35097800,0,181168,0.575848,0,3,0,1,1,'Corpulent Bonetusk'),
(35097800,0,181225,0.281738,0,3,0,1,1,'Crossbow of Contemplative Calm'),
(35097800,0,181228,0.274954,0,3,0,1,1,'Temple Guard''s Partisan'),
(35097800,0,181229,0.220283,0,3,0,1,1,'Tranquil''s Censer'),
(35097800,0,181234,0.506411,0,3,0,1,1,'Dutybound Spellblade'),
(35097800,0,181264,0.587022,0,3,0,1,1,'Plaguelouse Larva'),
(35097800,0,181313,0.408242,1,3,0,1,1,'Snapper Soul'),
(35097800,0,181314,0.386293,1,3,0,1,1,'Gulper Soul'),
(35097800,0,181315,0.620942,0,3,0,1,1,'Bloodfeaster Spiderling'),
(35097800,0,181317,0.210306,0,3,0,1,1,'Dauntless Duskrunner'),
(35097800,0,181323,0.201926,0,3,0,1,1,'Blightclutched Greatstaff'),
(35097800,0,181326,0.202724,0,3,0,1,1,'Bloodstained Hacksaw');

-- 35097801 : rootGrain=0 nightbloom=1 -> Novice's Large Satchel (guaranteed) + 25 ungated cosmetic rows
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(35097801,0,180981,100,0,3,0,1,1,'Novice''s Large Satchel'),
(35097801,0,179499,0.397068,0,3,0,1,1,'Nightwillow Barb'),
(35097801,0,179563,0.256597,0,3,0,1,1,'Heartwood Stem'),
(35097801,0,179585,0.215494,0,3,0,1,1,'Nightwillow Shortbow'),
(35097801,0,180414,0.330823,0,3,0,1,1,'Wakener''s Runestag'),
(35097801,0,180603,0.618947,0,3,0,1,1,'Violet Dredwing Pup'),
(35097801,0,180628,0.742257,0,3,0,1,1,'Pearlwing Heron'),
(35097801,0,180639,0.729088,0,3,0,1,1,'Dusty Sporeflutterer'),
(35097801,0,180723,0.219884,0,3,0,1,1,'Enchanted Wakener''s Runestag'),
(35097801,0,180814,0.572655,0,3,0,1,1,'Sable'),
(35097801,0,180815,0.552702,0,3,0,1,1,'Brightscale Hatchling'),
(35097801,0,180954,0.256597,0,3,0,1,1,'Crypt Watcher''s Spire'),
(35097801,0,180956,0.229062,0,3,0,1,1,'Axeblade Blunderbuss'),
(35097801,0,180963,0.381903,0,3,0,1,1,'Crypt Keeper''s Vessel'),
(35097801,0,181168,0.575848,0,3,0,1,1,'Corpulent Bonetusk'),
(35097801,0,181225,0.281738,0,3,0,1,1,'Crossbow of Contemplative Calm'),
(35097801,0,181228,0.274954,0,3,0,1,1,'Temple Guard''s Partisan'),
(35097801,0,181229,0.220283,0,3,0,1,1,'Tranquil''s Censer'),
(35097801,0,181234,0.506411,0,3,0,1,1,'Dutybound Spellblade'),
(35097801,0,181264,0.587022,0,3,0,1,1,'Plaguelouse Larva'),
(35097801,0,181313,0.408242,1,3,0,1,1,'Snapper Soul'),
(35097801,0,181314,0.386293,1,3,0,1,1,'Gulper Soul'),
(35097801,0,181315,0.620942,0,3,0,1,1,'Bloodfeaster Spiderling'),
(35097801,0,181317,0.210306,0,3,0,1,1,'Dauntless Duskrunner'),
(35097801,0,181323,0.201926,0,3,0,1,1,'Blightclutched Greatstaff'),
(35097801,0,181326,0.202724,0,3,0,1,1,'Bloodstained Hacksaw');

-- 35097802 : rootGrain=0 nightbloom=2 -> Novice's Stuffed Satchel (guaranteed) + 25 ungated cosmetic rows
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(35097802,0,180985,100,0,3,0,1,1,'Novice''s Stuffed Satchel'),
(35097802,0,179499,0.397068,0,3,0,1,1,'Nightwillow Barb'),
(35097802,0,179563,0.256597,0,3,0,1,1,'Heartwood Stem'),
(35097802,0,179585,0.215494,0,3,0,1,1,'Nightwillow Shortbow'),
(35097802,0,180414,0.330823,0,3,0,1,1,'Wakener''s Runestag'),
(35097802,0,180603,0.618947,0,3,0,1,1,'Violet Dredwing Pup'),
(35097802,0,180628,0.742257,0,3,0,1,1,'Pearlwing Heron'),
(35097802,0,180639,0.729088,0,3,0,1,1,'Dusty Sporeflutterer'),
(35097802,0,180723,0.219884,0,3,0,1,1,'Enchanted Wakener''s Runestag'),
(35097802,0,180814,0.572655,0,3,0,1,1,'Sable'),
(35097802,0,180815,0.552702,0,3,0,1,1,'Brightscale Hatchling'),
(35097802,0,180954,0.256597,0,3,0,1,1,'Crypt Watcher''s Spire'),
(35097802,0,180956,0.229062,0,3,0,1,1,'Axeblade Blunderbuss'),
(35097802,0,180963,0.381903,0,3,0,1,1,'Crypt Keeper''s Vessel'),
(35097802,0,181168,0.575848,0,3,0,1,1,'Corpulent Bonetusk'),
(35097802,0,181225,0.281738,0,3,0,1,1,'Crossbow of Contemplative Calm'),
(35097802,0,181228,0.274954,0,3,0,1,1,'Temple Guard''s Partisan'),
(35097802,0,181229,0.220283,0,3,0,1,1,'Tranquil''s Censer'),
(35097802,0,181234,0.506411,0,3,0,1,1,'Dutybound Spellblade'),
(35097802,0,181264,0.587022,0,3,0,1,1,'Plaguelouse Larva'),
(35097802,0,181313,0.408242,1,3,0,1,1,'Snapper Soul'),
(35097802,0,181314,0.386293,1,3,0,1,1,'Gulper Soul'),
(35097802,0,181315,0.620942,0,3,0,1,1,'Bloodfeaster Spiderling'),
(35097802,0,181317,0.210306,0,3,0,1,1,'Dauntless Duskrunner'),
(35097802,0,181323,0.201926,0,3,0,1,1,'Blightclutched Greatstaff'),
(35097802,0,181326,0.202724,0,3,0,1,1,'Bloodstained Hacksaw');

-- 35097803 : rootGrain=0 nightbloom=3 -> Novice's Overflowing Satchel (guaranteed) + 25 ungated cosmetic rows
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(35097803,0,180989,100,0,3,0,1,1,'Novice''s Overflowing Satchel'),
(35097803,0,179499,0.397068,0,3,0,1,1,'Nightwillow Barb'),
(35097803,0,179563,0.256597,0,3,0,1,1,'Heartwood Stem'),
(35097803,0,179585,0.215494,0,3,0,1,1,'Nightwillow Shortbow'),
(35097803,0,180414,0.330823,0,3,0,1,1,'Wakener''s Runestag'),
(35097803,0,180603,0.618947,0,3,0,1,1,'Violet Dredwing Pup'),
(35097803,0,180628,0.742257,0,3,0,1,1,'Pearlwing Heron'),
(35097803,0,180639,0.729088,0,3,0,1,1,'Dusty Sporeflutterer'),
(35097803,0,180723,0.219884,0,3,0,1,1,'Enchanted Wakener''s Runestag'),
(35097803,0,180814,0.572655,0,3,0,1,1,'Sable'),
(35097803,0,180815,0.552702,0,3,0,1,1,'Brightscale Hatchling'),
(35097803,0,180954,0.256597,0,3,0,1,1,'Crypt Watcher''s Spire'),
(35097803,0,180956,0.229062,0,3,0,1,1,'Axeblade Blunderbuss'),
(35097803,0,180963,0.381903,0,3,0,1,1,'Crypt Keeper''s Vessel'),
(35097803,0,181168,0.575848,0,3,0,1,1,'Corpulent Bonetusk'),
(35097803,0,181225,0.281738,0,3,0,1,1,'Crossbow of Contemplative Calm'),
(35097803,0,181228,0.274954,0,3,0,1,1,'Temple Guard''s Partisan'),
(35097803,0,181229,0.220283,0,3,0,1,1,'Tranquil''s Censer'),
(35097803,0,181234,0.506411,0,3,0,1,1,'Dutybound Spellblade'),
(35097803,0,181264,0.587022,0,3,0,1,1,'Plaguelouse Larva'),
(35097803,0,181313,0.408242,1,3,0,1,1,'Snapper Soul'),
(35097803,0,181314,0.386293,1,3,0,1,1,'Gulper Soul'),
(35097803,0,181315,0.620942,0,3,0,1,1,'Bloodfeaster Spiderling'),
(35097803,0,181317,0.210306,0,3,0,1,1,'Dauntless Duskrunner'),
(35097803,0,181323,0.201926,0,3,0,1,1,'Blightclutched Greatstaff'),
(35097803,0,181326,0.202724,0,3,0,1,1,'Bloodstained Hacksaw');

-- 35097810 : rootGrain=1 nightbloom=0 -> Journeyman's Satchel (guaranteed) + 25 ungated cosmetic rows
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(35097810,0,180975,100,0,3,0,1,1,'Journeyman''s Satchel'),
(35097810,0,179499,0.397068,0,3,0,1,1,'Nightwillow Barb'),
(35097810,0,179563,0.256597,0,3,0,1,1,'Heartwood Stem'),
(35097810,0,179585,0.215494,0,3,0,1,1,'Nightwillow Shortbow'),
(35097810,0,180414,0.330823,0,3,0,1,1,'Wakener''s Runestag'),
(35097810,0,180603,0.618947,0,3,0,1,1,'Violet Dredwing Pup'),
(35097810,0,180628,0.742257,0,3,0,1,1,'Pearlwing Heron'),
(35097810,0,180639,0.729088,0,3,0,1,1,'Dusty Sporeflutterer'),
(35097810,0,180723,0.219884,0,3,0,1,1,'Enchanted Wakener''s Runestag'),
(35097810,0,180814,0.572655,0,3,0,1,1,'Sable'),
(35097810,0,180815,0.552702,0,3,0,1,1,'Brightscale Hatchling'),
(35097810,0,180954,0.256597,0,3,0,1,1,'Crypt Watcher''s Spire'),
(35097810,0,180956,0.229062,0,3,0,1,1,'Axeblade Blunderbuss'),
(35097810,0,180963,0.381903,0,3,0,1,1,'Crypt Keeper''s Vessel'),
(35097810,0,181168,0.575848,0,3,0,1,1,'Corpulent Bonetusk'),
(35097810,0,181225,0.281738,0,3,0,1,1,'Crossbow of Contemplative Calm'),
(35097810,0,181228,0.274954,0,3,0,1,1,'Temple Guard''s Partisan'),
(35097810,0,181229,0.220283,0,3,0,1,1,'Tranquil''s Censer'),
(35097810,0,181234,0.506411,0,3,0,1,1,'Dutybound Spellblade'),
(35097810,0,181264,0.587022,0,3,0,1,1,'Plaguelouse Larva'),
(35097810,0,181313,0.408242,1,3,0,1,1,'Snapper Soul'),
(35097810,0,181314,0.386293,1,3,0,1,1,'Gulper Soul'),
(35097810,0,181315,0.620942,0,3,0,1,1,'Bloodfeaster Spiderling'),
(35097810,0,181317,0.210306,0,3,0,1,1,'Dauntless Duskrunner'),
(35097810,0,181323,0.201926,0,3,0,1,1,'Blightclutched Greatstaff'),
(35097810,0,181326,0.202724,0,3,0,1,1,'Bloodstained Hacksaw');

-- 35097811 : rootGrain=1 nightbloom=1 -> Journeyman's Large Satchel (guaranteed) + 25 ungated cosmetic rows
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(35097811,0,180980,100,0,3,0,1,1,'Journeyman''s Large Satchel'),
(35097811,0,179499,0.397068,0,3,0,1,1,'Nightwillow Barb'),
(35097811,0,179563,0.256597,0,3,0,1,1,'Heartwood Stem'),
(35097811,0,179585,0.215494,0,3,0,1,1,'Nightwillow Shortbow'),
(35097811,0,180414,0.330823,0,3,0,1,1,'Wakener''s Runestag'),
(35097811,0,180603,0.618947,0,3,0,1,1,'Violet Dredwing Pup'),
(35097811,0,180628,0.742257,0,3,0,1,1,'Pearlwing Heron'),
(35097811,0,180639,0.729088,0,3,0,1,1,'Dusty Sporeflutterer'),
(35097811,0,180723,0.219884,0,3,0,1,1,'Enchanted Wakener''s Runestag'),
(35097811,0,180814,0.572655,0,3,0,1,1,'Sable'),
(35097811,0,180815,0.552702,0,3,0,1,1,'Brightscale Hatchling'),
(35097811,0,180954,0.256597,0,3,0,1,1,'Crypt Watcher''s Spire'),
(35097811,0,180956,0.229062,0,3,0,1,1,'Axeblade Blunderbuss'),
(35097811,0,180963,0.381903,0,3,0,1,1,'Crypt Keeper''s Vessel'),
(35097811,0,181168,0.575848,0,3,0,1,1,'Corpulent Bonetusk'),
(35097811,0,181225,0.281738,0,3,0,1,1,'Crossbow of Contemplative Calm'),
(35097811,0,181228,0.274954,0,3,0,1,1,'Temple Guard''s Partisan'),
(35097811,0,181229,0.220283,0,3,0,1,1,'Tranquil''s Censer'),
(35097811,0,181234,0.506411,0,3,0,1,1,'Dutybound Spellblade'),
(35097811,0,181264,0.587022,0,3,0,1,1,'Plaguelouse Larva'),
(35097811,0,181313,0.408242,1,3,0,1,1,'Snapper Soul'),
(35097811,0,181314,0.386293,1,3,0,1,1,'Gulper Soul'),
(35097811,0,181315,0.620942,0,3,0,1,1,'Bloodfeaster Spiderling'),
(35097811,0,181317,0.210306,0,3,0,1,1,'Dauntless Duskrunner'),
(35097811,0,181323,0.201926,0,3,0,1,1,'Blightclutched Greatstaff'),
(35097811,0,181326,0.202724,0,3,0,1,1,'Bloodstained Hacksaw');

-- 35097812 : rootGrain=1 nightbloom=2 -> Journeyman's Stuffed Satchel (guaranteed) + 25 ungated cosmetic rows
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(35097812,0,180984,100,0,3,0,1,1,'Journeyman''s Stuffed Satchel'),
(35097812,0,179499,0.397068,0,3,0,1,1,'Nightwillow Barb'),
(35097812,0,179563,0.256597,0,3,0,1,1,'Heartwood Stem'),
(35097812,0,179585,0.215494,0,3,0,1,1,'Nightwillow Shortbow'),
(35097812,0,180414,0.330823,0,3,0,1,1,'Wakener''s Runestag'),
(35097812,0,180603,0.618947,0,3,0,1,1,'Violet Dredwing Pup'),
(35097812,0,180628,0.742257,0,3,0,1,1,'Pearlwing Heron'),
(35097812,0,180639,0.729088,0,3,0,1,1,'Dusty Sporeflutterer'),
(35097812,0,180723,0.219884,0,3,0,1,1,'Enchanted Wakener''s Runestag'),
(35097812,0,180814,0.572655,0,3,0,1,1,'Sable'),
(35097812,0,180815,0.552702,0,3,0,1,1,'Brightscale Hatchling'),
(35097812,0,180954,0.256597,0,3,0,1,1,'Crypt Watcher''s Spire'),
(35097812,0,180956,0.229062,0,3,0,1,1,'Axeblade Blunderbuss'),
(35097812,0,180963,0.381903,0,3,0,1,1,'Crypt Keeper''s Vessel'),
(35097812,0,181168,0.575848,0,3,0,1,1,'Corpulent Bonetusk'),
(35097812,0,181225,0.281738,0,3,0,1,1,'Crossbow of Contemplative Calm'),
(35097812,0,181228,0.274954,0,3,0,1,1,'Temple Guard''s Partisan'),
(35097812,0,181229,0.220283,0,3,0,1,1,'Tranquil''s Censer'),
(35097812,0,181234,0.506411,0,3,0,1,1,'Dutybound Spellblade'),
(35097812,0,181264,0.587022,0,3,0,1,1,'Plaguelouse Larva'),
(35097812,0,181313,0.408242,1,3,0,1,1,'Snapper Soul'),
(35097812,0,181314,0.386293,1,3,0,1,1,'Gulper Soul'),
(35097812,0,181315,0.620942,0,3,0,1,1,'Bloodfeaster Spiderling'),
(35097812,0,181317,0.210306,0,3,0,1,1,'Dauntless Duskrunner'),
(35097812,0,181323,0.201926,0,3,0,1,1,'Blightclutched Greatstaff'),
(35097812,0,181326,0.202724,0,3,0,1,1,'Bloodstained Hacksaw');

-- 35097813 : rootGrain=1 nightbloom=3 -> Journeyman's Overflowing Satchel (guaranteed) + 25 ungated cosmetic rows
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(35097813,0,180988,100,0,3,0,1,1,'Journeyman''s Overflowing Satchel'),
(35097813,0,179499,0.397068,0,3,0,1,1,'Nightwillow Barb'),
(35097813,0,179563,0.256597,0,3,0,1,1,'Heartwood Stem'),
(35097813,0,179585,0.215494,0,3,0,1,1,'Nightwillow Shortbow'),
(35097813,0,180414,0.330823,0,3,0,1,1,'Wakener''s Runestag'),
(35097813,0,180603,0.618947,0,3,0,1,1,'Violet Dredwing Pup'),
(35097813,0,180628,0.742257,0,3,0,1,1,'Pearlwing Heron'),
(35097813,0,180639,0.729088,0,3,0,1,1,'Dusty Sporeflutterer'),
(35097813,0,180723,0.219884,0,3,0,1,1,'Enchanted Wakener''s Runestag'),
(35097813,0,180814,0.572655,0,3,0,1,1,'Sable'),
(35097813,0,180815,0.552702,0,3,0,1,1,'Brightscale Hatchling'),
(35097813,0,180954,0.256597,0,3,0,1,1,'Crypt Watcher''s Spire'),
(35097813,0,180956,0.229062,0,3,0,1,1,'Axeblade Blunderbuss'),
(35097813,0,180963,0.381903,0,3,0,1,1,'Crypt Keeper''s Vessel'),
(35097813,0,181168,0.575848,0,3,0,1,1,'Corpulent Bonetusk'),
(35097813,0,181225,0.281738,0,3,0,1,1,'Crossbow of Contemplative Calm'),
(35097813,0,181228,0.274954,0,3,0,1,1,'Temple Guard''s Partisan'),
(35097813,0,181229,0.220283,0,3,0,1,1,'Tranquil''s Censer'),
(35097813,0,181234,0.506411,0,3,0,1,1,'Dutybound Spellblade'),
(35097813,0,181264,0.587022,0,3,0,1,1,'Plaguelouse Larva'),
(35097813,0,181313,0.408242,1,3,0,1,1,'Snapper Soul'),
(35097813,0,181314,0.386293,1,3,0,1,1,'Gulper Soul'),
(35097813,0,181315,0.620942,0,3,0,1,1,'Bloodfeaster Spiderling'),
(35097813,0,181317,0.210306,0,3,0,1,1,'Dauntless Duskrunner'),
(35097813,0,181323,0.201926,0,3,0,1,1,'Blightclutched Greatstaff'),
(35097813,0,181326,0.202724,0,3,0,1,1,'Bloodstained Hacksaw');

-- 35097820 : rootGrain=2 nightbloom=0 -> Artisan's Satchel (guaranteed) + 25 ungated cosmetic rows
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(35097820,0,180976,100,0,3,0,1,1,'Artisan''s Satchel'),
(35097820,0,179499,0.397068,0,3,0,1,1,'Nightwillow Barb'),
(35097820,0,179563,0.256597,0,3,0,1,1,'Heartwood Stem'),
(35097820,0,179585,0.215494,0,3,0,1,1,'Nightwillow Shortbow'),
(35097820,0,180414,0.330823,0,3,0,1,1,'Wakener''s Runestag'),
(35097820,0,180603,0.618947,0,3,0,1,1,'Violet Dredwing Pup'),
(35097820,0,180628,0.742257,0,3,0,1,1,'Pearlwing Heron'),
(35097820,0,180639,0.729088,0,3,0,1,1,'Dusty Sporeflutterer'),
(35097820,0,180723,0.219884,0,3,0,1,1,'Enchanted Wakener''s Runestag'),
(35097820,0,180814,0.572655,0,3,0,1,1,'Sable'),
(35097820,0,180815,0.552702,0,3,0,1,1,'Brightscale Hatchling'),
(35097820,0,180954,0.256597,0,3,0,1,1,'Crypt Watcher''s Spire'),
(35097820,0,180956,0.229062,0,3,0,1,1,'Axeblade Blunderbuss'),
(35097820,0,180963,0.381903,0,3,0,1,1,'Crypt Keeper''s Vessel'),
(35097820,0,181168,0.575848,0,3,0,1,1,'Corpulent Bonetusk'),
(35097820,0,181225,0.281738,0,3,0,1,1,'Crossbow of Contemplative Calm'),
(35097820,0,181228,0.274954,0,3,0,1,1,'Temple Guard''s Partisan'),
(35097820,0,181229,0.220283,0,3,0,1,1,'Tranquil''s Censer'),
(35097820,0,181234,0.506411,0,3,0,1,1,'Dutybound Spellblade'),
(35097820,0,181264,0.587022,0,3,0,1,1,'Plaguelouse Larva'),
(35097820,0,181313,0.408242,1,3,0,1,1,'Snapper Soul'),
(35097820,0,181314,0.386293,1,3,0,1,1,'Gulper Soul'),
(35097820,0,181315,0.620942,0,3,0,1,1,'Bloodfeaster Spiderling'),
(35097820,0,181317,0.210306,0,3,0,1,1,'Dauntless Duskrunner'),
(35097820,0,181323,0.201926,0,3,0,1,1,'Blightclutched Greatstaff'),
(35097820,0,181326,0.202724,0,3,0,1,1,'Bloodstained Hacksaw');

-- 35097821 : rootGrain=2 nightbloom=1 -> Artisan's Large Satchel (guaranteed) + 25 ungated cosmetic rows
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(35097821,0,180979,100,0,3,0,1,1,'Artisan''s Large Satchel'),
(35097821,0,179499,0.397068,0,3,0,1,1,'Nightwillow Barb'),
(35097821,0,179563,0.256597,0,3,0,1,1,'Heartwood Stem'),
(35097821,0,179585,0.215494,0,3,0,1,1,'Nightwillow Shortbow'),
(35097821,0,180414,0.330823,0,3,0,1,1,'Wakener''s Runestag'),
(35097821,0,180603,0.618947,0,3,0,1,1,'Violet Dredwing Pup'),
(35097821,0,180628,0.742257,0,3,0,1,1,'Pearlwing Heron'),
(35097821,0,180639,0.729088,0,3,0,1,1,'Dusty Sporeflutterer'),
(35097821,0,180723,0.219884,0,3,0,1,1,'Enchanted Wakener''s Runestag'),
(35097821,0,180814,0.572655,0,3,0,1,1,'Sable'),
(35097821,0,180815,0.552702,0,3,0,1,1,'Brightscale Hatchling'),
(35097821,0,180954,0.256597,0,3,0,1,1,'Crypt Watcher''s Spire'),
(35097821,0,180956,0.229062,0,3,0,1,1,'Axeblade Blunderbuss'),
(35097821,0,180963,0.381903,0,3,0,1,1,'Crypt Keeper''s Vessel'),
(35097821,0,181168,0.575848,0,3,0,1,1,'Corpulent Bonetusk'),
(35097821,0,181225,0.281738,0,3,0,1,1,'Crossbow of Contemplative Calm'),
(35097821,0,181228,0.274954,0,3,0,1,1,'Temple Guard''s Partisan'),
(35097821,0,181229,0.220283,0,3,0,1,1,'Tranquil''s Censer'),
(35097821,0,181234,0.506411,0,3,0,1,1,'Dutybound Spellblade'),
(35097821,0,181264,0.587022,0,3,0,1,1,'Plaguelouse Larva'),
(35097821,0,181313,0.408242,1,3,0,1,1,'Snapper Soul'),
(35097821,0,181314,0.386293,1,3,0,1,1,'Gulper Soul'),
(35097821,0,181315,0.620942,0,3,0,1,1,'Bloodfeaster Spiderling'),
(35097821,0,181317,0.210306,0,3,0,1,1,'Dauntless Duskrunner'),
(35097821,0,181323,0.201926,0,3,0,1,1,'Blightclutched Greatstaff'),
(35097821,0,181326,0.202724,0,3,0,1,1,'Bloodstained Hacksaw');

-- 35097822 : rootGrain=2 nightbloom=2 -> Artisan's Stuffed Satchel (guaranteed) + 25 ungated cosmetic rows
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(35097822,0,180983,100,0,3,0,1,1,'Artisan''s Stuffed Satchel'),
(35097822,0,179499,0.397068,0,3,0,1,1,'Nightwillow Barb'),
(35097822,0,179563,0.256597,0,3,0,1,1,'Heartwood Stem'),
(35097822,0,179585,0.215494,0,3,0,1,1,'Nightwillow Shortbow'),
(35097822,0,180414,0.330823,0,3,0,1,1,'Wakener''s Runestag'),
(35097822,0,180603,0.618947,0,3,0,1,1,'Violet Dredwing Pup'),
(35097822,0,180628,0.742257,0,3,0,1,1,'Pearlwing Heron'),
(35097822,0,180639,0.729088,0,3,0,1,1,'Dusty Sporeflutterer'),
(35097822,0,180723,0.219884,0,3,0,1,1,'Enchanted Wakener''s Runestag'),
(35097822,0,180814,0.572655,0,3,0,1,1,'Sable'),
(35097822,0,180815,0.552702,0,3,0,1,1,'Brightscale Hatchling'),
(35097822,0,180954,0.256597,0,3,0,1,1,'Crypt Watcher''s Spire'),
(35097822,0,180956,0.229062,0,3,0,1,1,'Axeblade Blunderbuss'),
(35097822,0,180963,0.381903,0,3,0,1,1,'Crypt Keeper''s Vessel'),
(35097822,0,181168,0.575848,0,3,0,1,1,'Corpulent Bonetusk'),
(35097822,0,181225,0.281738,0,3,0,1,1,'Crossbow of Contemplative Calm'),
(35097822,0,181228,0.274954,0,3,0,1,1,'Temple Guard''s Partisan'),
(35097822,0,181229,0.220283,0,3,0,1,1,'Tranquil''s Censer'),
(35097822,0,181234,0.506411,0,3,0,1,1,'Dutybound Spellblade'),
(35097822,0,181264,0.587022,0,3,0,1,1,'Plaguelouse Larva'),
(35097822,0,181313,0.408242,1,3,0,1,1,'Snapper Soul'),
(35097822,0,181314,0.386293,1,3,0,1,1,'Gulper Soul'),
(35097822,0,181315,0.620942,0,3,0,1,1,'Bloodfeaster Spiderling'),
(35097822,0,181317,0.210306,0,3,0,1,1,'Dauntless Duskrunner'),
(35097822,0,181323,0.201926,0,3,0,1,1,'Blightclutched Greatstaff'),
(35097822,0,181326,0.202724,0,3,0,1,1,'Bloodstained Hacksaw');

-- 35097830 : rootGrain=3 nightbloom=0 -> Artisan's Satchel (guaranteed) + 25 ungated cosmetic rows
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(35097830,0,180976,100,0,3,0,1,1,'Artisan''s Satchel'),
(35097830,0,179499,0.397068,0,3,0,1,1,'Nightwillow Barb'),
(35097830,0,179563,0.256597,0,3,0,1,1,'Heartwood Stem'),
(35097830,0,179585,0.215494,0,3,0,1,1,'Nightwillow Shortbow'),
(35097830,0,180414,0.330823,0,3,0,1,1,'Wakener''s Runestag'),
(35097830,0,180603,0.618947,0,3,0,1,1,'Violet Dredwing Pup'),
(35097830,0,180628,0.742257,0,3,0,1,1,'Pearlwing Heron'),
(35097830,0,180639,0.729088,0,3,0,1,1,'Dusty Sporeflutterer'),
(35097830,0,180723,0.219884,0,3,0,1,1,'Enchanted Wakener''s Runestag'),
(35097830,0,180814,0.572655,0,3,0,1,1,'Sable'),
(35097830,0,180815,0.552702,0,3,0,1,1,'Brightscale Hatchling'),
(35097830,0,180954,0.256597,0,3,0,1,1,'Crypt Watcher''s Spire'),
(35097830,0,180956,0.229062,0,3,0,1,1,'Axeblade Blunderbuss'),
(35097830,0,180963,0.381903,0,3,0,1,1,'Crypt Keeper''s Vessel'),
(35097830,0,181168,0.575848,0,3,0,1,1,'Corpulent Bonetusk'),
(35097830,0,181225,0.281738,0,3,0,1,1,'Crossbow of Contemplative Calm'),
(35097830,0,181228,0.274954,0,3,0,1,1,'Temple Guard''s Partisan'),
(35097830,0,181229,0.220283,0,3,0,1,1,'Tranquil''s Censer'),
(35097830,0,181234,0.506411,0,3,0,1,1,'Dutybound Spellblade'),
(35097830,0,181264,0.587022,0,3,0,1,1,'Plaguelouse Larva'),
(35097830,0,181313,0.408242,1,3,0,1,1,'Snapper Soul'),
(35097830,0,181314,0.386293,1,3,0,1,1,'Gulper Soul'),
(35097830,0,181315,0.620942,0,3,0,1,1,'Bloodfeaster Spiderling'),
(35097830,0,181317,0.210306,0,3,0,1,1,'Dauntless Duskrunner'),
(35097830,0,181323,0.201926,0,3,0,1,1,'Blightclutched Greatstaff'),
(35097830,0,181326,0.202724,0,3,0,1,1,'Bloodstained Hacksaw');

-- 35097831 : rootGrain=3 nightbloom=1 -> Artisan's Large Satchel (guaranteed) + 25 ungated cosmetic rows
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(35097831,0,180979,100,0,3,0,1,1,'Artisan''s Large Satchel'),
(35097831,0,179499,0.397068,0,3,0,1,1,'Nightwillow Barb'),
(35097831,0,179563,0.256597,0,3,0,1,1,'Heartwood Stem'),
(35097831,0,179585,0.215494,0,3,0,1,1,'Nightwillow Shortbow'),
(35097831,0,180414,0.330823,0,3,0,1,1,'Wakener''s Runestag'),
(35097831,0,180603,0.618947,0,3,0,1,1,'Violet Dredwing Pup'),
(35097831,0,180628,0.742257,0,3,0,1,1,'Pearlwing Heron'),
(35097831,0,180639,0.729088,0,3,0,1,1,'Dusty Sporeflutterer'),
(35097831,0,180723,0.219884,0,3,0,1,1,'Enchanted Wakener''s Runestag'),
(35097831,0,180814,0.572655,0,3,0,1,1,'Sable'),
(35097831,0,180815,0.552702,0,3,0,1,1,'Brightscale Hatchling'),
(35097831,0,180954,0.256597,0,3,0,1,1,'Crypt Watcher''s Spire'),
(35097831,0,180956,0.229062,0,3,0,1,1,'Axeblade Blunderbuss'),
(35097831,0,180963,0.381903,0,3,0,1,1,'Crypt Keeper''s Vessel'),
(35097831,0,181168,0.575848,0,3,0,1,1,'Corpulent Bonetusk'),
(35097831,0,181225,0.281738,0,3,0,1,1,'Crossbow of Contemplative Calm'),
(35097831,0,181228,0.274954,0,3,0,1,1,'Temple Guard''s Partisan'),
(35097831,0,181229,0.220283,0,3,0,1,1,'Tranquil''s Censer'),
(35097831,0,181234,0.506411,0,3,0,1,1,'Dutybound Spellblade'),
(35097831,0,181264,0.587022,0,3,0,1,1,'Plaguelouse Larva'),
(35097831,0,181313,0.408242,1,3,0,1,1,'Snapper Soul'),
(35097831,0,181314,0.386293,1,3,0,1,1,'Gulper Soul'),
(35097831,0,181315,0.620942,0,3,0,1,1,'Bloodfeaster Spiderling'),
(35097831,0,181317,0.210306,0,3,0,1,1,'Dauntless Duskrunner'),
(35097831,0,181323,0.201926,0,3,0,1,1,'Blightclutched Greatstaff'),
(35097831,0,181326,0.202724,0,3,0,1,1,'Bloodstained Hacksaw');

-- 35097840 : rootGrain=4 nightbloom=0 -> Spirit-Tender's Satchel (guaranteed) + 28 ungated cosmetic rows
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(35097840,0,180977,100,0,3,0,1,1,'Spirit-Tender''s Satchel'),
(35097840,0,179499,0.397068,0,3,0,1,1,'Nightwillow Barb'),
(35097840,0,179563,0.256597,0,3,0,1,1,'Heartwood Stem'),
(35097840,0,179585,0.215494,0,3,0,1,1,'Nightwillow Shortbow'),
(35097840,0,180414,0.330823,0,3,0,1,1,'Wakener''s Runestag'),
(35097840,0,180603,0.618947,0,3,0,1,1,'Violet Dredwing Pup'),
(35097840,0,180628,0.742257,0,3,0,1,1,'Pearlwing Heron'),
(35097840,0,180639,0.729088,0,3,0,1,1,'Dusty Sporeflutterer'),
(35097840,0,180723,0.219884,0,3,0,1,1,'Enchanted Wakener''s Runestag'),
(35097840,0,180814,0.572655,0,3,0,1,1,'Sable'),
(35097840,0,180815,0.552702,0,3,0,1,1,'Brightscale Hatchling'),
(35097840,0,180954,0.256597,0,3,0,1,1,'Crypt Watcher''s Spire'),
(35097840,0,180956,0.229062,0,3,0,1,1,'Axeblade Blunderbuss'),
(35097840,0,180963,0.381903,0,3,0,1,1,'Crypt Keeper''s Vessel'),
(35097840,0,181168,0.575848,0,3,0,1,1,'Corpulent Bonetusk'),
(35097840,0,181225,0.281738,0,3,0,1,1,'Crossbow of Contemplative Calm'),
(35097840,0,181228,0.274954,0,3,0,1,1,'Temple Guard''s Partisan'),
(35097840,0,181229,0.220283,0,3,0,1,1,'Tranquil''s Censer'),
(35097840,0,181234,0.506411,0,3,0,1,1,'Dutybound Spellblade'),
(35097840,0,181264,0.587022,0,3,0,1,1,'Plaguelouse Larva'),
(35097840,0,181313,0.408242,1,3,0,1,1,'Snapper Soul'),
(35097840,0,181314,0.386293,1,3,0,1,1,'Gulper Soul'),
(35097840,0,181315,0.620942,0,3,0,1,1,'Bloodfeaster Spiderling'),
(35097840,0,181317,0.210306,0,3,0,1,1,'Dauntless Duskrunner'),
(35097840,0,181323,0.201926,0,3,0,1,1,'Blightclutched Greatstaff'),
(35097840,0,181326,0.202724,0,3,0,1,1,'Bloodstained Hacksaw'),
(35097840,0,181302,0.341598,0,3,0,1,1,'Spirit Tender''s Branches'),
(35097840,0,181306,0.340002,0,3,0,1,1,'Spirit Tender''s Bulb'),
(35097840,0,181310,0.234649,0,3,0,1,1,'Spirit Tender''s Pack');

DELETE FROM `garrison_conservatory_yield` WHERE `spiritItemId` = 0;
INSERT INTO `garrison_conservatory_yield` (`spiritItemId`,`rootGrainCount`,`nightbloomCount`,`lootId`,`comment`) VALUES
(0,0,0,35097800,'Novice''s Satchel'),
(0,0,1,35097801,'Novice''s Large Satchel'),
(0,0,2,35097802,'Novice''s Stuffed Satchel'),
(0,0,3,35097803,'Novice''s Overflowing Satchel'),
(0,1,0,35097810,'Journeyman''s Satchel'),
(0,1,1,35097811,'Journeyman''s Large Satchel'),
(0,1,2,35097812,'Journeyman''s Stuffed Satchel'),
(0,1,3,35097813,'Journeyman''s Overflowing Satchel'),
(0,2,0,35097820,'Artisan''s Satchel'),
(0,2,1,35097821,'Artisan''s Large Satchel'),
(0,2,2,35097822,'Artisan''s Stuffed Satchel'),
(0,3,0,35097830,'Artisan''s Satchel'),
(0,3,1,35097831,'Artisan''s Large Satchel'),
(0,4,0,35097840,'Spirit-Tender''s Satchel');
