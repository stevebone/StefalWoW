--
-- Abomination Factory - authored rows for `garrison_abomination_recipe` (which Abominable Stitching rank unlocks
-- which recipe). The table is created empty by 2026_08_07_70_covenant_abomination_recipes.sql; read that file's
-- header first, it explains why the mapping is content rather than client data.
--
-- The loader (GarrisonMgr::LoadAbominationStitching) validates every row: `spellId` must be a SkillLineAbility of
-- SkillLine 2787 and `requiredRank` must be 1-5, else the row is rejected with an sql.sql error. So a wrong spell id
-- cannot silently take effect.
--
-- TWO SOURCING TIERS ARE USED, AND EVERY ROW BELOW SAYS WHICH ONE IT IS.
--
--   [CLIENT]  Derived from 12.0.7.68275 client data alone, by exact name match between the GarrTalent description
--             of a tier and the SpellName of a recipe. The prior pass declared the whole mapping underivable; that
--             was too pessimistic - four of the five talent descriptions name a specific deliverable, and each named
--             thing is exactly one of the 66 SkillLine-2787 recipes. These four rows need no web source at all.
--
--   [WEB]     Sourced from the public web, with the URL and the quoted line recorded inline. Cross-checked against
--             the [CLIENT] anchors and against the talent prose (which states how MANY constructs arrive per tier).
--
-- The full GarrTalent text the [CLIENT] rows are derived from (GarrTalent.Description_lang, tree 321):
--   1096 tier 0 "Build a Buddy"      -> "...grants you the first rank of Abominable Stitching. Allowing you to build
--                                        your own constructs! Welcome CHORDY and a variety of other useful constructs
--                                        to aid you in gathering resources..."
--   1097 tier 1 "Crafting Limbs"     -> "...the second rank... Gain access to a new battle pet, A CONSTRUCT DISGUISE
--                                        and other useful consumables."
--   1098 tier 2 "Bring Them to Life" -> "...the third rank... Gain the ability to call in Emeni to aid you in combat.
--                                        Allows you to build additional constructs..."     (names no recipe)
--   1099 tier 3 "Forged Friends"     -> "...the fourth rank... two dangerous high-end constructs, as well as A BAG OF
--                                        TREATS TO LURE TWIGIN to your side..."
--   1100 tier 4 "Best Fiends Forever"-> "...the final rank... Welcome the Soulfused Construct to your ranks and
--                                        create the awesome BONESEWN FLESHROC..."
--
-- Idempotent.
--

-- [CLIENT] 325284 Construct Body: "Chordy" -> rank 1.
--          GarrTalent 1096 (tier 0, i.e. rank 1) names Chordy outright, and 325284 is the only skill-2787 recipe
--          whose SpellName contains "Chordy". Corroborated by its reagent cost, which is unique in the whole skill:
--          1 x 183743 Malleable Flesh, against 10-20 x 178061 (+ Superior Parts) for every other construct body.
INSERT INTO `garrison_abomination_recipe` (`spellId`, `requiredRank`) VALUES (325284, 1)
    ON DUPLICATE KEY UPDATE `requiredRank` = VALUES(`requiredRank`);

-- [CLIENT] 331404 Construct Disguise -> rank 2.
--          GarrTalent 1097 (tier 1 = rank 2) says the tier grants "a construct disguise"; 331404 is the only
--          skill-2787 recipe named "Construct Disguise".
INSERT INTO `garrison_abomination_recipe` (`spellId`, `requiredRank`) VALUES (331404, 2)
    ON DUPLICATE KEY UPDATE `requiredRank` = VALUES(`requiredRank`);

-- [CLIENT] 338059 Bag of Twigin Treats -> rank 4.
--          GarrTalent 1099 (tier 3 = rank 4) says the tier grants "a bag of treats to lure Twigin to your side";
--          338059 is the only skill-2787 recipe named "Bag of Twigin Treats".
INSERT INTO `garrison_abomination_recipe` (`spellId`, `requiredRank`) VALUES (338059, 4)
    ON DUPLICATE KEY UPDATE `requiredRank` = VALUES(`requiredRank`);

-- [CLIENT] 338052 Bonesewn Fleshroc -> rank 5.
--          GarrTalent 1100 (tier 4 = rank 5) says the tier lets you "create the awesome Bonesewn Fleshroc"; 338052
--          is the only skill-2787 recipe named "Bonesewn Fleshroc". Corroborated by cost: 50 x 178061 Malleable
--          Flesh + 5 x 183744 Superior Parts, the single most expensive Malleable Flesh cost in the skill line.
INSERT INTO `garrison_abomination_recipe` (`spellId`, `requiredRank`) VALUES (338052, 5)
    ON DUPLICATE KEY UPDATE `requiredRank` = VALUES(`requiredRank`);

--
-- APPENDIX - the construct-body -> hidden tracking-quest map, IS derivable after all (recorded here so it is not
-- re-derived from scratch). Each construct-body recipe carries SPELL_EFFECT_QUEST_COMPLETE (Effect 16) at effect
-- index 1, and EffectMiscValue[0] is the tracking quest. Straight out of SpellEffect.db2, 12.0.7.68275:
--
--     325284 Chordy        -> (none; effect 90 kill-credit 167076 "Chordy's Body" only - the free starter construct)
--     325451 Roseboil      -> 57605      325452 Marz          -> 57611      325453 Flytrap      -> 57597
--     325454 Atticus       -> 58410      325458 Miru          -> 58415      326379 Neena        -> 57604
--     326380 Gas Bag       -> 57608      326406 Professor     -> 57601      326407 Toothpick    -> 58414
--     326408 Mama Tomalin  -> 60216      338037 Iron Phillip  -> 58411      338039 Guillotine   -> 58416
--     338040 Sabrina       -> 57600      338043 Naxx          -> 58413
--   (the other 14 non-construct recipes complete their own tracking quests the same way: 337535 -> 61560,
--    337540 -> 61561, 337554 -> 61562, 338383 -> 61712, 342417 -> 62468 ... 344798 -> 62825.)
--
-- Those quests exist in QuestV2.db2 but have NO `quest_template` row, so the effect currently no-ops. They are NOT
-- authored here: QuestV2 publishes only ID / UniqueBitFlag / UiQuestDetailsThemeID, so while the quest IDs and their
-- meaning are now certain, their LogTitle and flags are not derivable from any client table, and inventing quest text
-- is not something this file will do. The unblock is a Shadowlands-era `quest_template` dump or a sniff; with one,
-- authoring is mechanical because the id->construct mapping above is exact.
--
