--
-- Queen's Conservatory - authored wildseed kinds (Night Fae unique sanctum feature, GarrTalentTree 319).
--
-- WHY THIS TABLE IS EMPTY
-- -----------------------
-- Everything the 12.0.7.68275 client publishes about the Conservatory is already consumed by the core:
--   * the unlock ladder   GarrTalentTree 319 "The Queen's Conservatory" (GarrTypeID 111, MaxTiers 5,
--                         FeatureTypeIndex 5 = SanctumUnique, FeatureSubtypeIndex 3 = Ardenweald) and its
--                         talents 1086 First Planting / 1087 Initial Growth / 1088 Nurtured Souls /
--                         1089 Flourishing Beds / 1090 Final Forms;
--   * the research costs  GarrTalentRank 1352-1356 (1500/5000/10000/12500/15000 x currency 1813 Reservoir
--                         Anima @3600/43200/86400/86400/86400s) cross-checked against GarrTalentCost
--                         (+6/12/22/40/70 x currency 1810 Redeemed Soul) - both charged by the existing
--                         generic Garrison talent engine, not by this feature;
--   * the harvest reward  gameobject_template 350978 "Queen's Conservatory Cache" (CHEST, lockId 3218,
--                         chestLoot 350978) whose 40-row gameobject_loot_template already ships in this DB.
--
-- CORRECTION (2026_08_07_63): this file used to name GameObjects 353652 "Catalyst of Power" / 353653
-- "Catalyst of Renewal" / 353654 "Catalyst of Might" as the Conservatory catalysts. They are not - their
-- displayIds 64892/64893/64894 resolve to world/expansion08/doodads/vampire/9vm_vampire_bottle*.m2 and all
-- three sit in AreaTable 10413 Revendreth (map 2222), while the Conservatory is AreaTable 13367 on map 2363.
-- The real catalysts are ITEMS - 176921 Temporal Leaves / 176922 Wild Nightbloom / 176832 Wildseed Root
-- Grain - and they are authored in `garrison_conservatory_catalyst` by
-- 2026_08_07_63_covenant_conservatory_catalysts.sql, together with the per-combination loot tables in
-- `garrison_conservatory_yield`. See that file's header for the full derivation.
--
-- What NO 68275 DB2 and no world-DB row anywhere describes:
--   * how long an individual wildseed takes to mature,
--   * what planting one costs (currency and/or item), and which wildseed identities exist,
--   * which loot table a given catalyst combination pays out (the catalyst EFFECTS are client-derived - each
--     item's own description states them - but the reward tables are not; see 2026_08_07_63).
-- GarrTalent.ActiveDurationSecs is non-zero only for the Channel Anima trees (345-348) and one Dragonflight
-- tree; GarrTalentSocketProperties has three rows and all are SocketType 2 (Conduit); there are no
-- CharShipmentContainer rows with GarrTypeID 111; there is no Conservatory currency in CurrencyTypes.
--
-- Those values are therefore CONTENT, and they are authored here rather than invented in C++. Until a row
-- exists the engine is inert by design: QueensConservatory::PlantWildseed answers
-- CONSERVATORY_ERROR_NO_WILDSEED_DATA and nothing else in the sanctum changes behaviour.
--
-- Columns:
--   wildseedEntry       author-chosen id, referenced by character_garrison_conservatory.wildseedEntry
--   costCurrencyId      CurrencyTypes.db2 id charged on plant (0 = none)
--   costCurrencyCount   amount charged
--   costItemId          item consumed on plant (0 = none)
--   costItemCount       amount consumed
--   maturationSeconds   time from plant to harvestable; 0 means "unknown", and the plant is refused
--   rewardGameObjectId  chest whose loot template is rolled on harvest (default 350978, see above)
--   requiredTier        researched tiers of tree 319 needed to plant this kind (1-5)
--
-- Idempotent.
--
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
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Queen''s Conservatory wildseed kinds (unauthored by design - see file header)';

-- No INSERTs. Adding one is a content decision that needs a Shadowlands-era sniff or a design ruling on the
-- maturation time and plant cost; see the file header for exactly which values are missing.
