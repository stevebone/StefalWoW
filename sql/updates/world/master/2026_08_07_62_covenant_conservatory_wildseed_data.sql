--
-- Queen's Conservatory - authored wildseed kinds (Night Fae unique sanctum feature, GarrTalentTree 319).
--
-- This file fills the table created empty by 2026_08_07_60_covenant_conservatory_wildseeds.sql, whose header
-- said the plant cost / wildseed identities / maturation time are published by no 12.0.7 DB2. That is still
-- true for the *maturation time*, but it is NOT true for the identities or the cost, and this file corrects
-- that: the wildseed identities and their cost are recoverable from client data. Every value below carries
-- its own source. Values that could only be sourced from the web are tagged [WEB]; values recovered from
-- client data are tagged [CLIENT]. Nothing here is invented; where nothing could be sourced, the row or
-- column is simply not written (see "STILL UNSOURCED" at the bottom).
--
--
-- 1. WHAT A "WILDSEED" IS  [CLIENT]
-- ---------------------------------
-- Retail does not have a "wildseed item". The plot is called a Wildseed of Regrowth and what the player
-- consumes to start a growth is a *Spirit* item. Every such item's Use effect is one and the same spell:
--     ItemSparse/Item -> ItemXItemEffect -> ItemEffect.SpellID = 323617 "Infuse Spirit"
--     Spell 323617 Description_lang = "Infuse a Wildseed of Regrowth with this spirit."
-- Twelve of them exist (4 flavours x 3 qualities) plus three Legendary ones. All fifteen are still present
-- in THIS build's client data - verified directly against 12.0.7.68275
-- M:\WorldofWarcraft\dbc\enUS\ItemSparse.db2 (id list) AND Item.db2 (id list + copy table): all 15 ids
-- resolve in both, which is exactly the pair ObjectMgr::LoadItemTemplates() joins, so every costItemId
-- below passes the loader's sObjectMgr->GetItemTemplate() check. They carry the subtitle "Queen's
-- Conservatory", stack to 200 (20 for the Legendary ones) and cost no gold. So: one Spirit item is consumed
-- per planting, and nothing else is.
--
-- The three Anima-Catalyst items are the sibling set, sharing spell 323169 "Infuse Catalyst"
-- ("Infuse an Anima Catalyst Plot with this catalyst."): 176921 Temporal Leaves, 176922 Wild Nightbloom,
-- 176832 Wildseed Root Grain. They are NOT modelled by this table - see section 4.
--
--
-- 2. PLANT COST  [CLIENT, negative-confirmed]
-- -------------------------------------------
-- costCurrencyId/costCurrencyCount = 0 for every row. The Spirit item itself is the whole cost:
--   * the item's own tooltip has no currency component and no gold buy price (ItemSparse.BuyPrice = 0,
--     SellPrice = 1 copper);
--   * CurrencyTypes.db2 has no Conservatory currency (already established by 2026_08_07_60);
--   * the anima/soul cost that people associate with the Conservatory (1813 Reservoir Anima + 1810 Redeemed
--     Soul) is the *tier research* cost, charged by the generic Garrison talent engine from
--     GarrTalentRank 1352-1356 - not a per-plant cost.
-- Cross-checked against the retail walkthrough, which lists no plant cost at any step of the incubation
-- process: https://www.wowhead.com/guide/night-fae-covenant-queens-conservatory ("Spirit Incubation
-- Process", steps 1-8).
--
--
-- 3. MATURATION TIME  [WEB - no client-data source exists]
-- --------------------------------------------------------
-- Confirmed absent from client data, not merely "not found": the pod's own timer is the aura
-- Spell 323618 "Incubating" ("A spirit is being reborn."), and its SpellMisc.DurationIndex is 3 - the
-- 1-minute placeholder the tooltip on Wowhead still renders ("1 minute remaining"). The real duration is
-- stamped by the server at cast time, exactly as this core's QueensConservatory::RefreshClientState stamps
-- the client-facing countdown aura 344304 (whose DurationIndex is also 3). There is therefore no DB2 row
-- anywhere carrying a per-spirit maturation time, and no 9.x sniff in C:\sniff\ contains any Conservatory
-- traffic (the three Shadowlands captures present are Tazavesh Mythic+ runs). The numbers below are
-- WEB-SOURCED and are marked as such per row. They describe patch 9.0.2 - 9.2.x retail behaviour; no source
-- reports the values ever changing across 9.0/9.1/9.2.
--
--   259200 s = 3 days  -- Uncommon (green) and Rare/"Greater" (blue) Spirits.
--       Rare: "3 days (rare)" - https://wowpetaddiction.blogspot.com/2020/11/shadowlands-pets-from-queens.html
--             corroborated first-hand: "I then plant 2 Greater Spirits (both were Greater Untamed Spirits).
--             When I looted the seed's 3 days later..." -
--             https://us.forums.blizzard.com/en/wow/t/queens-conservatory-planting-a-seed/834938
--       Uncommon: "using an Uncommon Spirit might be the way to go. They take the least amount of time to
--             mature (3 days)" - same blogspot page. SINGLE SOURCE, and that same page leaves the uncommon
--             cell of its own table as "?" - so treat 259200 for the green tier as the weakest number here.
--   604800 s = 7 days  -- Epic/"Divine" (purple) and Legendary (orange) Spirits.
--       "use Divine Dutiful Spirit on Wildseed of Regrowth with 4x Anima Catalyst Plot ...; wait 7 days for
--       incubation" and "use Tranquil Spirit of the Cosmos on Wildseed of Regrowth ...; wait 7 days for
--       Incubation Complete" - http://spmthailand.com/mjdrs/night-fae-queen's-conservatory-guide
--       Consistent with the 7-day upper bound repeated by every guide ("Depending on which quality of Spirit
--       you used, it will take 3-7 days for it to finish growing", blogspot page above; "This ranges from
--       1 hour to 7 days", https://www.wowhead.com/guide/night-fae-covenant-queens-conservatory).
--   No source anywhere reports a 5-day tier, so none is written. If the true ladder turns out to be
--   3/5/7 days, only the Rare rows below change.
--
--
-- 4. CATALYSTS - WHAT THE EVIDENCE SAYS, AND WHY NO CATALYST ROWS ARE WRITTEN HERE
-- --------------------------------------------------------------------------------
-- (a) [CLIENT] GameObjects 353652 "Catalyst of Power" / 353653 "Catalyst of Renewal" / 353654 "Catalyst of
--     Might" - named in QueensConservatory.h as the Conservatory catalysts - are NOT Conservatory objects.
--     Their gameobject_template displayIds 64892/64893/64894 resolve through GameObjectDisplayInfo to
--     FileDataIDs 3153293 / 3153291 / 3153299 =
--         world/expansion08/doodads/vampire/9vm_vampire_bottle03.m2
--         world/expansion08/doodads/vampire/9vm_vampire_bottle01.m2
--         world/expansion08/doodads/vampire/9vm_vampire_bottle03_empty01.m2
--     i.e. Revendreth vampire bottles, and Wowhead places all three in AreaTable 10413 = Revendreth (map
--     2222). The Queen's Conservatory is AreaTable 13367 on map 2363. They are Revendreth props with a
--     coincidental name.
-- (b) [CLIENT] The real catalysts are the three items in section 1, and their plot buffs are ordinary auras
--     whose AuraDescription_lang states exactly what each one does:
--         Spell 323725 "Temporal Leaves"      -> "These enchanted leaves reduce the Wildseed of Regrowth
--                                                 process by 1 day."                  (item 176921)
--         Spell 323787 "Wild Nightbloom"      -> "This magical bloom increases the yield of crafting
--                                                 materials offered by reborn spirits and catalysts grown
--                                                 from catalyst seeds by 100%."       (item 176922)
--         Spell 336307 "Wildseed Root Grain"  -> "This special root increases the quality of rewards
--                                                 offered by a spirit upon rebirth."  (item 176832)
--     So: Temporal Leaves = -86400 s incubation, Wild Nightbloom = +100% material yield, Wildseed Root
--     Grain = +reward quality. That is the catalyst -> yield mapping, and it is client-derived.
-- (c) [CLIENT] The Wildseed of Regrowth (NPC 165466) and the Anima Catalyst Plot (NPC 165480) are
--     *creatures*, not GameObjects. `AttachCatalyst` validating a gameobject_template entry cannot express
--     the real system.
-- (d) [WEB] The reward ladder those catalysts drive (Wowhead guide, "Rewards" table):
--         no catalyst        -> Novice's Satchel
--         1x Root Grain      -> Journeyman's Satchel + tier-1 weapon appearances
--         2x / 3x Root Grain -> Artisan's Satchel   + tier-2 appearances, pets and mounts
--         4x Root Grain      -> Spirit-Tender's Satchel + Spirit Tender's Pack + top mounts
--         1x / 2x / 3x Wild Nightbloom -> satchel size upgraded to Large / Stuffed / Overflowing
--     `integ_world.gameobject_loot_template` 350978 already contains all of those satchels and cosmetics
--     flattened into ONE 41-row table (Novice's 23%, Artisan's 29.7%, Spirit-Tender's 7.3%, Large/Stuffed/
--     Overflowing variants, plus the pet/mount tail). The current schema has exactly one
--     `rewardGameObjectId` per wildseed kind, so it can only ever roll that one flattened table; there is
--     no column through which a catalyst count could select a different loot id. Expressing (b)+(d) needs
--     a schema change, which is deliberately NOT made here - see the accompanying report.
--
--
-- 5. requiredTier = 1 FOR EVERY ROW  [WEB]
-- ----------------------------------------
-- Retail does not gate which Spirit may go into which pod; any pod takes any Spirit. What the tiers gate is
-- the number of pods and how many catalyst plots feed them (Wowhead guide: tier 1 opens the Conservatory
-- with 2 Wildseeds, each further tier adds one, and a fully upgraded Conservatory has 6 Wildseeds and 8
-- Anima-Catalyst plots, distributed as 1 pod with 1 catalyst link, 3 pods with 2, 1 pod with 3 and 1 pod
-- with 4). Higher-quality Spirits are gated by *where they drop*, not by the pod. Setting requiredTier to
-- anything above 1 would invent a restriction retail does not have.
--
--
-- STILL UNSOURCED - deliberately not written
-- ------------------------------------------
--   * Item 177953 "Untamed Spirit" (the stack-20 Quest Item duplicate of 177698, ItemSparse
--     Display_lang identical) has no sourced maturation time. A forum poster mentions receiving "a 1 hour
--     one that was supposed to be for the quest", which is a hint, not a number with a source. No row.
--   * Item 185717 "Slumbering Spirit" ("A spirit which can be nurtured in the Queen's Conservatory") reads
--     like a wildseed but has NO ItemXItemEffect row at all, i.e. no Use effect and no way to infuse it.
--     Not plantable, so no row. Likewise 184779 "Temporal Leaves" is the stack-20 quest duplicate of the
--     catalyst 176921, not a wildseed.
--   * For reference, the client does distinguish two growth states - GlobalStrings 43109/43110
--     SPELL_FAILED_CUSTOM_ERROR_526 "This Wildseed of Regrowth is still incubating." and _527 "...is still
--     growing." - which the engine collapses into CONSERVATORY_PLOT_GROWING. Not a value this table can
--     carry; noted so it is not lost.
--   * A distinct maturation time for Rare vs Uncommon, and for Legendary vs Epic. Every source that gives a
--     number gives 3 days for the low end and 7 days for the high end and nothing in between, so the two
--     pairs are written with the same value rather than a guessed middle step.
--   * Which catalyst item corresponds to which of the three "Catalyst of ..." GameObjects: the question is
--     void, those GameObjects are Revendreth props (section 4a).
--
-- Idempotent.
--

-- Safety net if this file is applied without 2026_08_07_60 (identical definition).
CREATE TABLE IF NOT EXISTS `garrison_conservatory_wildseed` (
  `wildseedEntry`      INT UNSIGNED NOT NULL DEFAULT 0,
  `costCurrencyId`     INT UNSIGNED NOT NULL DEFAULT 0,
  `costCurrencyCount`  INT UNSIGNED NOT NULL DEFAULT 0,
  `costItemId`         INT UNSIGNED NOT NULL DEFAULT 0,
  `costItemCount`      INT UNSIGNED NOT NULL DEFAULT 0,
  `maturationSeconds`  INT UNSIGNED NOT NULL DEFAULT 0,
  `rewardGameObjectId` INT UNSIGNED NOT NULL DEFAULT 350978,
  `requiredTier`       TINYINT UNSIGNED NOT NULL DEFAULT 1,
  PRIMARY KEY (`wildseedEntry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Queen''s Conservatory wildseed kinds';

ALTER TABLE `garrison_conservatory_wildseed`
  COMMENT='Queen''s Conservatory wildseed kinds - one row per Shadowlands Spirit item (see 2026_08_07_62)';

-- wildseedEntry is deliberately the Spirit's own item id, so the row is self-identifying and the entry the
-- player passes to `.garrison conservatory plant` is the item they are spending.
DELETE FROM `garrison_conservatory_wildseed` WHERE `wildseedEntry` IN
(177698,177699,177700,178874,178877,178878,178879,178880,178881,178882,178883,178884,183704,183805,183806);

INSERT INTO `garrison_conservatory_wildseed`
  (`wildseedEntry`,`costCurrencyId`,`costCurrencyCount`,`costItemId`,`costItemCount`,`maturationSeconds`,`rewardGameObjectId`,`requiredTier`) VALUES
-- ---- Uncommon (ItemSparse.OverallQualityID 2). 3 days [WEB, single source - weakest value in this file].
(177698, 0, 0, 177698, 1, 259200, 350978, 1),  -- Untamed Spirit
(178874, 0, 0, 178874, 1, 259200, 350978, 1),  -- Martial Spirit
(178881, 0, 0, 178881, 1, 259200, 350978, 1),  -- Dutiful Spirit
(178882, 0, 0, 178882, 1, 259200, 350978, 1),  -- Prideful Spirit
-- ---- Rare / "Greater" (OverallQualityID 3). 3 days [WEB, two independent sources incl. a first-hand report].
(177699, 0, 0, 177699, 1, 259200, 350978, 1),  -- Greater Untamed Spirit
(178877, 0, 0, 178877, 1, 259200, 350978, 1),  -- Greater Martial Spirit
(178880, 0, 0, 178880, 1, 259200, 350978, 1),  -- Greater Dutiful Spirit
(178883, 0, 0, 178883, 1, 259200, 350978, 1),  -- Greater Prideful Spirit
-- ---- Epic / "Divine" (OverallQualityID 4). 7 days [WEB - walkthrough + the 3-7 day range's upper bound].
(177700, 0, 0, 177700, 1, 604800, 350978, 1),  -- Divine Untamed Spirit
(178878, 0, 0, 178878, 1, 604800, 350978, 1),  -- Divine Martial Spirit
(178879, 0, 0, 178879, 1, 604800, 350978, 1),  -- Divine Dutiful Spirit
(178884, 0, 0, 178884, 1, 604800, 350978, 1),  -- Divine Prideful Spirit
-- ---- Legendary (OverallQualityID 5), one per Conservatory tier-2/3/4 questline. 7 days [WEB].
(183704, 0, 0, 183704, 1, 604800, 350978, 1),  -- Shifting Spirit of Knowledge   (-> Falir the Shifting)
(183805, 0, 0, 183805, 1, 604800, 350978, 1),  -- Tranquil Spirit of the Cosmos  (-> Ohm of Meditation)
(183806, 0, 0, 183806, 1, 604800, 350978, 1);  -- Energetic Spirit of Curiosity  (-> Lia the Curious)
