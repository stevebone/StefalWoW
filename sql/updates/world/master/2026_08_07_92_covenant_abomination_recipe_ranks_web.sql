--
-- Abomination Factory - WEB-SOURCED rows for `garrison_abomination_recipe`.
--
-- Kept in a separate file from 2026_08_07_91 on purpose: that file holds only rows derived from 12.0.7.68275 client
-- data, this one holds rows that could only be sourced from the public web. Every row below carries its URL and the
-- quoted sentence it rests on. Nothing here is inferred from reagent cost, spell-id ordering, or "it looks like".
--
-- WHY THE WEB WAS NEEDED AT ALL. Confirmed again while sourcing this: Wowhead does NOT publish a machine-readable
-- rank for these recipes. The tooltip endpoint nether.wowhead.com/tooltip/spell/325284 returns cast time and
-- reagents and nothing else, and the "Recipes" listview Skill column reads literally "Abominable Stitching,
-- Ascension Crafting" with no rank number. warcraft.wiki.gg's Abominable Stitching page has full reagent tables and
-- zero rank mapping. So every row here rests on human-written guide text or player reports, never on scraped data.
--
-- HOW THE WEB CLAIMS WERE CHECKED BEFORE BEING TRUSTED. The client's own GarrTalent descriptions state how MANY
-- things arrive per tier and name a few of them; the web mapping reproduces all of it:
--     rank 1  "Welcome Chordy and a variety of other useful constructs"      -> 5 constructs below. fits.
--     rank 2  "additional constructs... a new battle pet, a construct
--              disguise and other useful consumables"                        -> 4 constructs + Backbone (the pet)
--                                                                               + Construct Disguise (in _91)
--                                                                               + Anima-bound Wraps. fits.
--     rank 3  "call in Emeni to aid you in combat... additional constructs"   -> 3 constructs + Abominable Backup,
--                                                                               which is literally the Emeni summon.
--     rank 4  "TWO dangerous high-end constructs, as well as a bag of treats
--              to lure Twigin"                                               -> EXACTLY two: Iron Phillip and Naxx,
--                                                                               + Bag of Twigin Treats (in _91).
--     rank 5  "the Soulfused Construct... create the awesome Bonesewn
--              Fleshroc"                                                     -> Bonesewn Fleshroc (in _91). The
--                                                                               Soulfused Construct (Unity) is not a
--                                                                               recipe - it is quest-granted, which
--                                                                               is why only 15 construct bodies
--                                                                               exist in SkillLine 2787.
-- Counting the construct bodies: 5 + 4 + 3 + 2 = 14, plus Miru (held below) = 15. The whole roster is accounted for
-- and the per-tier counts match the client prose. Independently, the rank-4 pair is the only two construct bodies
-- needing a bespoke rare component (183475 Indomitable Hide / 183519 Necromantic Oil), each with its own dedicated
-- quest (58379 / 58376) - client data reaching the same answer as the web by a different route.
--
-- The loader validates every spellId against SkillLineAbility of SkillLine 2787 and requiredRank against 1-5, so a
-- bad row is rejected with an sql.sql error rather than silently applied.
--
-- Idempotent.
--

-- ================================ RANK 1 ================================
-- All four corroborated twice: the structured roster comment by "jeykama" (205 upvotes, 2021-01-21, ed. 2021-03-13)
-- on https://www.wowhead.com/guide/necrolord-covenant-abominable-stitching#comments, which groups every construct
-- under a "Rank N" heading; and "Lightingfist" (2020-12-17) on the same page: "You can get Chordy, Flytrap, Marz,
-- Roseboil, and Atticus at lvl 1."
-- (jeykama's roster is used repeatedly below. It is not taken on faith: 8 of its entries are independently
--  corroborated by other sources, and none is contradicted, so it is treated as a reliable source in its own right.)
INSERT INTO `garrison_abomination_recipe` (`spellId`, `requiredRank`) VALUES
    (325451, 1),    -- [WEB] Roseboil. jeykama, under "Rank 1": "Roseboil: Healer with a very strong aoe heal on a fairly short CD"
    (325452, 1),    -- [WEB] Marz.     jeykama, under "Rank 1": "Marz: Tank construct with AOE taunt"
    (325453, 1),    -- [WEB] Flytrap.  jeykama, under "Rank 1": "Flytrap: Rideable construct with very very fast max speed"
    (325454, 1)     -- [WEB] Atticus.  jeykama, under "Rank 1": "Atticus: A vendor that has a daily key and small bag of sinstones";
                    --       also https://wowpetaddiction.blogspot.com/2020/11/shadowlands-pets-from-necrolord-kyrian.html
                    --       "At Tier 1, you should be able to craft Atticus already"
    ON DUPLICATE KEY UPDATE `requiredRank` = VALUES(`requiredRank`);

-- ================================ RANK 2 ================================
INSERT INTO `garrison_abomination_recipe` (`spellId`, `requiredRank`) VALUES
    -- [WEB] Neena. https://www.wowhead.com/spell=326379 - "It appears as you need Stitchyard Rank 2 in order to see
    --       her, and for the key to her cage to drop from the nearby elite." and "The steps to unlock Construct
    --       Body: \"Neena\" ... are: Upgrade your Abomination Stitching to level 2 (or else, you won't be able to
    --       see her)". Third source: jeykama lists Neena under "Rank 2".
    (326379, 2),
    -- [WEB] Sabrina. https://www.wowhead.com/spell=338040 - "Comment by Sillyllama This is unlocked by tier 2
    --       stitching." Corroborated by jeykama, who lists Sabrina under "Rank 2".
    (338040, 2),
    -- [WEB] Professor. Single source: jeykama, under "Rank 2": "Professor: Rideable construct with basic speed with
    --       a skill that grants a stat buff. Hyperaggressive." Committed because that roster is corroborated on 8
    --       other entries and contradicted on none - but it IS single-sourced. First row to re-check if the tiering
    --       ever looks wrong in game.
    (326406, 2),
    -- [WEB] Toothpick. Single source, same roster: jeykama, under "Rank 2": "Toothpick: Does a thunderclap-like aoe
    --       on a very short CD. Looks a bit like Emeni." Same caveat as Professor.
    (326407, 2),
    -- [WEB] Anima-bound Wraps. https://nonethewiserguild.wordpress.com/things-to-do-when-youre-dead/ - "You need at
    --       least rank 2 of the Abominable Stitching to craft the Anima-Bound Wraps". Corroborated by
    --       https://www.wowhead.com/spell=326903 - "Comment by Addy You need both, the Anima Conductor (Tier 1) and
    --       Abomination Factory (Tier 2), to summon Visectus."
    (326903, 2),
    -- [WEB+CLIENT] Backbone. GarrTalent 1097 (rank 2) says the tier grants "a new battle pet" without naming it;
    --       https://www.wowhead.com/guide/necrolord-covenant-abominable-stitching#comments, mentiraloso (2021-01-09)
    --       supplies the name: "After using most of my reserve anima to upgrade the table to second rank, I tried to
    --       create the pet, Backbone, but kept getting an error about not having enough anima." Web names the recipe,
    --       client independently confirms a battle pet arrives at exactly this rank.
    (338057, 2)
    ON DUPLICATE KEY UPDATE `requiredRank` = VALUES(`requiredRank`);

-- ================================ RANK 3 ================================
INSERT INTO `garrison_abomination_recipe` (`spellId`, `requiredRank`) VALUES
    -- [WEB] Mama Tomalin. Wowhead guide body text (https://www.wowhead.com/guide/necrolord-covenant-abominable-stitching):
    --       "Clean White Hat is sold by Mama Tomalin at Butcher's Block once you've created this construct after
    --       upgrading the Stitchyard to tier 3." Plus https://www.wowhead.com/spell=326408 - "Comment by Chaddai Made
    --       after getting 3 rank stitching." Plus guide comment by VolksDK (2021-01-15): "Mama Tomalin unlocks with
    --       rank 3." Strongest-sourced row in this file.
    (326408, 3),
    -- [WEB] Gas Bag. https://www.wowhead.com/spell=326380 - "Comment by Chaddai Got rank 3 stitching and made Gas
    --       Bag." Corroborated by jeykama, who lists Gas Bag under "Rank 3".
    (326380, 3),
    -- [WEB] Guillotine. https://www.wowhead.com/spell=326380 - "Guillotine from rank 3 stitching is much better".
    --       Corroborated by jeykama, who lists Guillotine under "Rank 3".
    (338039, 3),
    -- [WEB+CLIENT] Abominable Backup. GarrTalent 1098 (rank 3): "Gain the ability to call in Emeni to aid you in
    --       combat" - the client says the capability arrives at rank 3 but never names the recipe. Wowhead guide
    --       tier-3 row: "Gain access to call upon Emeni as a bodyguard", and guide comment by Acaulis: "Tier three is
    --       'advertised' as 'Gain access to call upon Emeni as a bodyguard.' ... Now available at the stitching table
    --       is Abominable Backup which does summon Emeni." Also https://www.wowhead.com/item=180264 - "Considering
    --       you can make this at rank 3...".
    (327091, 3)
    ON DUPLICATE KEY UPDATE `requiredRank` = VALUES(`requiredRank`);

-- ================================ RANK 4 ================================
-- The two "dangerous high-end constructs" of GarrTalent 1099. This pair is the best-evidenced assignment in either
-- file: the client says there are exactly two and the web names exactly these two, and a completely independent
-- client route agrees - they are the only construct bodies requiring a bespoke rare component (Iron Phillip needs
-- 183475 Indomitable Hide, Naxx needs 183519 Necromantic Oil) and each of those has its own dedicated quest
-- (58379 "Construct Part: Indomitable Hide", 58376 "Construct Part: Necromantic Oil", both QuestSortID -593).
INSERT INTO `garrison_abomination_recipe` (`spellId`, `requiredRank`) VALUES
    -- [WEB] Iron Phillip. https://www.wowhead.com/spell=338037 - "Comment by drakile Iron Phillip, Craftable at
    --       level 4 Stitchyard." Plus https://www.wowhead.com/spell=338039 - "...even the rank 4 description claims
    --       Naxx and Phillip are high end constructs..." (that commenter is quoting the same in-game talent text
    --       extracted from GarrTalent 1099). Plus jeykama's roster.
    (338037, 4),
    -- [WEB] Naxx. https://www.wowhead.com/spell=338039 - "even the rank 4 description claims Naxx and Phillip are
    --       high end constructs". Corroborated by jeykama, who lists Naxx under "Rank 4".
    (338043, 4)
    ON DUPLICATE KEY UPDATE `requiredRank` = VALUES(`requiredRank`);

--
-- DELIBERATELY NOT AUTHORED - 46 of the 66 recipes. Sources conflict or are silent; a padded table would be worse
-- than a short one, because a wrong row teaches a recipe at the wrong rank and nothing would ever flag it.
--
--   325458 Miru Soulblossom   CONFLICT. Rank 1: jeykama's roster lists Miru under "Rank 1", and Lightingfist says
--                             "the Special Constructs should only req lvl 1". Rank 2: two videos disagree -
--                             youtube.com/watch?v=i7scyswwlfY "2 hidden constructs in Maldraxxus for covenant
--                             members with a level 2 Abomination Factory", and youtube.com/watch?v=afKrtyMkW94
--                             titled "2 Hidden Necrolord Abominations! Abomination Factory Rank 2+" (the two hidden
--                             ones being Miru and Neena). Note Lightingfist's "Special Constructs are all rank 1" is
--                             demonstrably wrong for the other two specialized constructs - Neena is rank 2 and Mama
--                             Tomalin rank 3 - which weakens the rank-1 side, but not enough to call it.
--   338058 Mu'dud             CONFLICT. Wowhead guide: "crafted... after upgrading the Stitchyard to tier 3".
--                             https://www.wowhead.com/spell=338058: "Comment by Yolopanther This crafting recipe is
--                             unlocked when you upgrade your Abomination Factory to Tier 4."
--   338046 Bindings of        AMBIGUOUS. jeykama puts "The Bindings" under Rank 5; a comment on item=183717 mentions
--          Wellbeing          rank 4. Neither is a clean unlock statement.
--   331403 Lil' Eddie         Single uncorroborated source (https://www.wowhead.com/item=180267 "Need tier 3 of the
--                             Abomination Factory unlocked"). Held because rank 2's battle pet is already accounted
--                             for by Backbone; if Lil' Eddie is also a pet the two claims need reconciling first.
--   326762 Bag of Creepy Crawlies, 327090 Restore Construct, 342782 Tossable Head, 342803 Construct's Best Friend,
--   347024 Tighter Stitching  No source states a rank. The last three are 9.0.5/9.1-era additions no guide covers.
--   ...and all 37 "Fashion Accessories" recipes (TradeSkillCategory 1523/1533/1534/1535). No source anywhere states
--                             a stitching rank for any of them; guides list only reagent sources. The two tier-3
--                             mentions that look relevant (Chef Hat's Clean White Hat, Egg Hat's Broken Egg Shells)
--                             gate the VENDOR - Mama Tomalin, who is herself the rank-3 construct - not the recipe.
--
-- Unblock for all of the above: a Shadowlands-era client with the Blizzard_AbominableStitching addon still shipping,
-- or a 9.x sniff of the stitching UI. Until then these stay unauthored and BuildConstruct answers
-- ABOMINATION_FACTORY_ERROR_NO_RECIPE_DATA for them, which is the correct, visible failure.
--
