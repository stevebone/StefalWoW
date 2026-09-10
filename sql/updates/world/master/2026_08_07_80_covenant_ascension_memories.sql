--
-- Path of Ascension - the authored memory roster (Kyrian unique sanctum feature, GarrTalentTree 320).
--
-- WHY THIS TABLE IS EMPTY
-- -----------------------
-- Nearly everything about this feature IS published by the 12.0.7.68275 client, and the core consumes it
-- directly - none of it is repeated here:
--   * the unlock ladder     GarrTalentTree 320 (GarrTypeID 111, MaxTiers 5, Flags 16, UiTextureKitID 5283,
--                           FeatureTypeIndex 5 = SanctumUnique, FeatureSubtypeIndex 1 = Bastion/Kyrian) and
--                           its talents 1091 First Steps / 1092 Sacred Trials / 1093 Continued Training /
--                           1094 Teachings of Wisdom / 1095 Trials of Humility;
--   * the research costs    GarrTalentRank 1394-1398 cross-checked against GarrTalentCost 4419-4423 and
--                           6488-6506: 1500/5000/10000/12500/15000 x currency 1813 Reservoir Anima plus
--                           6/12/22/40/70 x currency 1810 Redeemed Soul, @3600/43200/86400/86400/86400s.
--                           Charged by the generic Garrison talent engine, not by this feature;
--   * the ladder's meaning  the talents' own descriptions, which the core reads literally - 1091 grants "the
--                           capture of SIX Shadowlands memories", 1092 "FOUR MORE" (so capacity is 6 then 10,
--                           and never grows again) plus "access to their SECOND TRIAL" and "weekly quests",
--                           1093 "the REST OF THE TRIALS OF LOYALTY as well as the FIRST OF THE TRIALS OF
--                           WISDOM" plus the Brazier of Lessons Learned, 1094 "the REMAINING TRIALS OF
--                           WISDOM" plus "a SECOND WEEKLY QUEST", 1095 "the final trial for ALL of your
--                           captured memories, the TRIAL OF HUMILITY" plus the Brazier of Inward Reflection;
--   * the four trials       Difficulty.db2 168 "Path of Ascension: Courage", 169 "...: Loyalty",
--                           170 "...: Wisdom", 171 "...: Humility" - all InstanceType 5 (MAP_SCENARIO),
--                           MinPlayers = MaxPlayers = 1, bound to the arena by MapDifficulty 4795-4798.
--                           Their id order is the difficulty order, which the quest text confirms
--                           independently ("in a Trial of Loyalty OR HIGHER", quests 63181-63191);
--   * the arena             Map 2375 "9.0 Bastion Arena - Path of Ascension" (InstanceType 5, ExpansionID 8),
--                           AreaTable 13462 "Ascension Coliseum" (ZoneName "BastionAscensionColiseum");
--   * the scenario          Scenario 1803 "Path of Ascension" with ScenarioStep 4521/4542/4543/4544
--                           (Preparation / Fight / Reward / Reward, CriteriaTrees 85338/85638/85650/85741);
--                           tutorial twin Scenario 1826, steps 4581-4584. LFGDungeons 2084 -> 1803, 2089 -> 1826;
--   * the quests + rewards  88 rows already in `integ_world` under QuestSortID -595
--                           (QUEST_SORT_PATH_OF_ASCENSION), with questgivers already linked: Apolon 168485
--                           gives the ten memory quests 62954 + 63168-63176 and the three champion quests
--                           62951-62953; Artemede 168427 gives 60496-60498 and the weeklies 63181-63191 plus
--                           63192 "Trial of Humility"; Dactylis 168430 "Path of Ascension Crafter" gives the
--                           "Blueprint: ..." chain; Haephus 167745 gives 60489 "The Path of Ascension".
--                           Each memory quest's objective is a KILL CREDIT on that memory's own creature and
--                           carries its own reward (item 184812 for a memory, 184811 for a weekly), so the
--                           core awards the credit and lets the ordinary quest system pay - no reward table
--                           is invented anywhere;
--   * progress counting     Achievement 14340/14342/14343/14344/14345/14346/14348/14349/14351,
--                           "Defeat N boss(es) in the Path of Ascension" for N = 1/3/5/7/12/16/20/24/39.
--
-- What NO 68275 client row anywhere states:
--   1. WHICH memories are the SIX that tier 1 captures, and which four tier 2 adds. The talents give the
--      COUNTS and never the names.
--   2. WHICH memories are the "some" that gain a second trial at tier 2, versus "the rest" at tier 3, versus
--      "the remaining" at tier 4.
-- Neither is recoverable: GarrTalentRank.PerkSpellID is 0 on all five ranks (1394-1398, confirmed, not
-- assumed), no PlayerCondition in the build references talents 1091-1095 beyond 84025 on the tier-0 talent
-- (which tree 321 shares), and the ten memory quests carry no tier or difficulty field.
--
-- That mapping is therefore CONTENT, and it is authored here rather than invented in C++. Until a row exists
-- the engine is deliberately half-inert: the derived half still runs (a Kyrian who researches tier N gets the
-- correct capacity, trial ceiling, weekly-quest slots and brazier count, and `.garrison ascension status`
-- reports them), but no memory can be captured and CaptureMemory answers ASCENSION_ERROR_NO_MEMORY_DATA.
--
-- SEPARATELY AND MORE IMPORTANTLY: THE ARENA ITSELF IS NOT AUTHORED IN THIS WORLD DB.
--   SELECT * FROM scenarios WHERE map = 2375;                        -> 0 rows (317 rows exist overall)
--   SELECT COUNT(*) FROM creature WHERE map = 2375;                  -> 0
--   SELECT COUNT(*) FROM gameobject WHERE map = 2375;                -> 0
--   SELECT COUNT(*) FROM areatrigger WHERE MapId = 2375;             -> 0
--   SELECT * FROM instance_template WHERE map = 2375;                -> 0 rows
--   SELECT * FROM scenario_poi WHERE CriteriaTreeID IN (85338,85638,85650,85741); -> 0 rows
--   Apolon 168485, Artemede 168427 and Dactylis 168430 all have 0 spawns, and every boss creature_template
--   row has an empty ScriptName.
-- On top of that, MapDifficulty 4795-4798 all carry ContentTuningID 0, so the build publishes NO scaling for
-- the four trials, and nothing maps the 2-4 creature_template variants of each boss (e.g. Echthra
-- 172177/172482/172515) to a difficulty - `creature_template_difficulty` has no rows for them.
-- Because of that PathOfAscension::StartTrial refuses with ASCENSION_ERROR_NO_ARENA_CONTENT instead of
-- starting a challenge that could never be finished, and nothing is ever auto-won. Authoring the `scenarios`
-- row plus the Coliseum spawns flips GarrisonMgr::IsAscensionArenaAuthored() and turns entry on with no code
-- change.
--
-- Authoring a memory row is validated at load: `creatureId` MUST exist in `creature_template` (the core
-- credits it on a win, and that credit is what makes the memory's quest pay out), `captureQuestId` must exist
-- if non-zero, `requiredTier` must be 1-5, every trial tier must be 0-5, and a trial may not open before the
-- memory can be captured.
--
-- Columns:
--   memoryId       author-chosen id, referenced by character_garrison_path_of_ascension.memoryId
--   creatureId     the memory's creature_template entry; kill credit is awarded on it when a trial is won
--   captureQuestId the memory's "Path of Ascension: X" quest (QuestSortID -595), 0 = none
--   requiredTier   researched tiers of GarrTalentTree 320 needed before the memory can be captured (1-5)
--   courageTier    researched tiers at which this memory's Trial of Courage  opens (0 = it never offers it)
--   loyaltyTier    ... Trial of Loyalty
--   wisdomTier     ... Trial of Wisdom
--   humilityTier   ... Trial of Humility
--
-- For reference when the roster IS authored, the ten memory quests and their kill-credit creatures - these
-- ARE derived, and are exactly the "six ... four more" the talents promise:
--   62954 Kalisthene             -> 170654      63173 Thran'tiok              -> 172411
--   63168 Echthra                -> 172177      63174 Mad Mortimer            -> 172101
--   63169 Alderyn and Myn'ir     -> 172408      63175 Athanos                 -> 171873
--   63170 Nuuminuuru             -> 172410      63176 Azaruux                 -> 172333
--   63171 Craven Corinth         -> 172412
--   63172 Splinterbark Nightmare -> 172682
-- Which six of those ten belong to tier 1, and which trials each opens at which tier, is precisely the part
-- that is NOT published - so no INSERT is written here.
--
-- Idempotent.
--
CREATE TABLE IF NOT EXISTS `garrison_ascension_memory` (
  `memoryId`       INT UNSIGNED NOT NULL DEFAULT 0,
  `creatureId`     INT UNSIGNED NOT NULL DEFAULT 0,
  `captureQuestId` INT UNSIGNED NOT NULL DEFAULT 0,
  `requiredTier`   TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `courageTier`    TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `loyaltyTier`    TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `wisdomTier`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `humilityTier`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`memoryId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Path of Ascension memory roster (unauthored by design - see file header)';

-- No INSERTs. Adding one is a content decision that needs a Shadowlands-era sniff or a design ruling on the
-- tier-1 six / tier-2 four split and on which memories are "empowered" at which tier; see the file header for
-- exactly which values are missing and which query establishes each absence.
