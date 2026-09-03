--
-- Covenant sanctum: Transport Network spell grants + Path of Ascension memory roster.
--
-- ============================================================================================================
-- PART 1 - TRANSPORT NETWORK (`garrison_transport_network`)
-- ============================================================================================================
-- The 12 Transport Network talents (trees 308 Kyrian / 309 Venthyr / 307 NightFae / 310 Necrolord) publish
-- ZERO effect fields on every rank row (PerkSpellID 0, GarrAbilityID 0, Points 0 - verified against the raw
-- wago GarrTalentRank CSV @12.0.7.68887; positive control: 56 non-zero PerkSpellIDs elsewhere in the same
-- parse). Which spell each researched tier enables is therefore authored content. Consumed by
-- GarrisonMgr::LoadTransportNetworkSpells -> Garrison::ApplyTransportNetworkPerks: DISCOVER_TAXI carriers are
-- cast once (the taught TaxiNodes row is the persistent capability), verified teleports are learned as
-- castable spells and follow covenant switches like rank perks.
--
-- EVERY row below passed spell-level verification (wago SpellEffect CSV @12.0.7.68887, fetched 2026-08-08):
--   * "Traverse to ..."           Effect 15  (TELEPORT_WITH_SPELL_VISUAL_KIT_LOADING_SCREEN), TargetB 17
--                                 (TARGET_DEST_DB) - destination comes from `spell_target_position`
--   * "Mirror Teleport: ..." /
--     "Teleport: Seat of the
--     Primus"                     Effect 252 (SPELL_EFFECT_TELEPORT_UNITS, modern id), TargetB 17
--   * "Teach Taxi Node: ..."      Effect 154 (SPELL_EFFECT_DISCOVER_TAXI), MiscValue = TaxiNodes id
-- and destination-level verification: the spell either already ships a `spell_target_position` row in this
-- world DB, or one is authored below from a server-shipped coordinate source (never invented).
--
-- SKIP LIST - spells verified as real teleports (or named by the audit) but NOT granted, and why. Each needs
-- either a sniffed/authorable destination or a retail sniff of its tier attribution before it can be added:
--   325621 "Traverse to Forest's Edge"    (talent 1053) teleport ok; no spell_target_position row ships and
--   308436 "Traverse To the Stalks"       (talent 1053) no verifiable world coordinate was found for the
--   325616 "Traverse to the Banks of Life"(talent 1054) destination (no game_tele row, wowhead publishes
--   325618 "Traverse to Gormhive"         (talent 1054) only zone-relative coords) -> casting would fail.
--   323742 "Mirror Teleport: Forgotten Chamber" (Venthyr) same missing-destination situation.
--   311183 "Mirror Teleport: Redelav Tower"     (Venthyr) same; the nearby QuestPath game_tele row is a
--                                               quest transport, not the mirror endpoint.
--   324272 "Teleport: Seat of the Primus" (Zerekriss portal, talent 1051 names Zerekriss) and
--   345729 "Teleport: Seat of the Primus" (Exoramas portal?) - teleports verified but no
--                                               spell_target_position ships and the talent<->spell
--                                               attribution needs a sniff.
--   338624 "Teleport: Seat of the Primus" NOT a teleport at all: single Effect 278 (NYI in core, no
--                                               destination-dependent target) - failed verification.
--   Night Fae talent 1055 "Blossoming Network" (Crumbled Ridge / Eventide Grove / Tirna Scithe) - no matching
--                                               teleport spells were found in SpellName @68887 at all.
--   Kyrian talents 1057/1058 (Sagehaven, the Temples, Terrace of the Collectors) - no verified "Teach Taxi
--                                               Node" spells found for these destinations.
-- What ALSO remains for full retail behavior (out of scope here, documented honestly): the world-side network
-- objects (mushroom rings, mirrors, anima gateways, ziggurat teleporters) with their gossip/spellclick wiring,
-- and the node->talent-rank gating map (needs sniffs; see COVENANT_SANCTUM_AUDIT.md par.4.2).
--
-- Idempotent.
--
CREATE TABLE IF NOT EXISTS `garrison_transport_network` (
  `garrTalentId` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'GarrTalent.db2 id of a Transport Network talent',
  `spellId`      INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'verified teleport or taxi-teach spell',
  `comment`      VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`garrTalentId`, `spellId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Covenant Transport Network research spell grants (authored - client publishes no effect fields)';

DELETE FROM `garrison_transport_network` WHERE `garrTalentId` IN (1047,1048,1049,1050,1051,1052,1053,1054,1055,1056,1057,1058);
INSERT INTO `garrison_transport_network` (`garrTalentId`, `spellId`, `comment`) VALUES
-- Kyrian tree 308
(1056, 344010, 'Step of Faith: Teach Taxi Node Heros Rest (TaxiNodes 2529) + Oribos (2395); DISCOVER_TAXI wago-verified; talent text names Heros Rest'),
-- Venthyr tree 309
(1047, 323738, 'Mirror, Mirror: Mirror Teleport Sinfall (hub return, effect 252 + DEST_DB); destination authored below from game_tele 2101 Sinfall'),
(1047, 346620, 'Mirror, Mirror: Teach Taxi Node Darkhaven (TaxiNodes 2488) + Oribos (2395); DISCOVER_TAXI wago-verified; per audit par.4.1.5'),
(1049, 323763, 'Mirrors Edge: Mirror Teleport Ember Ward (spell_target_position ships); tier attribution approximate - the talents own damaged-mirror text; sniff needed'),
(1049, 319932, 'Mirrors Edge: Mirror Teleport Sanctuary of the Mad (spell_target_position ships); tier attribution approximate; sniff needed'),
-- Night Fae tree 307
(1053, 325602, 'Nurtured Roots: Traverse to the Heart of the Forest (spell_target_position ships); talent text names it'),
(1053, 325614, 'Nurtured Roots: Traverse to Stillglade (spell_target_position ships); talent text names it'),
(1054, 325620, 'Fun with Fungi: Traverse to Elder Stand (spell_target_position ships); talent text names it'),
-- Necrolord tree 310
(1050, 323912, 'Ziggurat Now: Teleport Seat of the Primus (spell_target_position ships); tier attribution approximate - talent unlocks necropolis travel; sniff needed');

-- Mirror Teleport: Sinfall (323738) verified teleport (effect 252 at EffectIndex 1, TargetB 17 TARGET_DEST_DB,
-- wago @68887) with NO shipped destination row. Coordinates are the server-shipped `game_tele` id 2101
-- 'Sinfall' (map 2222: -1867.85, 7599.76, 4193.72) - a DB-sourced value, not a web guess; orientation
-- unknown -> 0. VerifiedBuild 0 marks it as unsniffed authoring.
DELETE FROM `spell_target_position` WHERE `ID` = 323738;
INSERT INTO `spell_target_position` (`ID`, `EffectIndex`, `OrderIndex`, `MapID`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `VerifiedBuild`) VALUES
(323738, 1, 0, 2222, -1867.85, 7599.76, 4193.72, 0, 0);

-- ============================================================================================================
-- PART 2 - PATH OF ASCENSION MEMORY ROSTER (`garrison_ascension_memory`)
-- ============================================================================================================
-- The table (DDL: 2026_08_07_80_covenant_ascension_memories.sql) shipped EMPTY because the client publishes
-- the COUNTS ("six memories" at tier 1, "four more" at tier 2) but never the NAMES, and never which memories
-- gain their higher trials at which tier. This seeds the roster from what IS verifiable, with the derivation
-- of every value labeled:
--
-- DERIVED FROM SHIPPED DATA (client + this world DB - not web):
--   * The ten memories, their kill-credit creatures and capture quests: quest_objectives rows for
--     62954 + 63168-63176 (verified by SELECT against this world DB on 2026-08-08; every creature exists in
--     creature_template, every quest in quest_template).
--   * Capacity 6 at tier 1 and 10 at tier 2: talent 1091/1092 descriptions (client data).
--   * Trial of Humility for ALL memories at tier 5: talent 1095's own text ("the final trial for all of your
--     captured memories").
--
-- WEB-SOURCED (labeled per the project rule; wowhead guide + search excerpts fetched 2026-08-08):
--   * WHICH six are tier 1 and which four are tier 2: quest-id grouping (62954, 63168-63172 = six;
--     63173-63176 = four) matches the talent counts exactly, and the wowhead Path of Ascension guide excerpts
--     confirm the second group ("Thran'tiok is unlockable upon upgrading ... to Tier 2: Sacred Trials",
--     "defeating Thran'tiok, Mad Mortimer, and Athanos ... unlocking the Azaruux trial" at tier 2).
--     https://www.wowhead.com/guide/kyrian-covenant-path-of-ascension
--     NOTE a conflicting fan-blog account orders SOME of the first six behind progressive quest chains -
--     that pacing lives in the quest chain itself (prerequisites on 63168-63172), which the quest system
--     enforces independently of requiredTier; no contradiction with this table.
--
-- INTERPRETED (documented, conservative - NOT sniffed):
--   * The per-memory higher-trial matrix maps the talents' wording to the two capture groups:
--     1092 "some ... their second trial"          -> Loyalty at tier 2 for the tier-1 six
--     1093 "the rest of the Trials of Loyalty
--           as well as the first of the Trials
--           of Wisdom"                            -> Loyalty at tier 3 for the tier-2 four,
--                                                    Wisdom at tier 3 for the tier-1 six
--     1094 "the remaining Trials of Wisdom"       -> Wisdom at tier 4 for the tier-2 four
--     1095 Humility for all                       -> tier 5 everywhere
--     No memory opens a trial EARLIER than this reading allows. If a retail sniff shows a different
--     per-memory split, only these tier numbers change - ids and quests are fixed data.
--
-- UNCERTAIN -> SKIPPED (not invented): nothing else. The arena itself (scenario 1803 / map 2375 spawns and
-- scaling) remains unauthored - see the 2026_08_07_80 header; trials still refuse to START until that
-- content exists, but capture, capacity, weekly-slot and brazier reporting all come alive with these rows.
--
-- Idempotent.
--
DELETE FROM `garrison_ascension_memory` WHERE `memoryId` BETWEEN 1 AND 10;
INSERT INTO `garrison_ascension_memory` (`memoryId`, `creatureId`, `captureQuestId`, `requiredTier`, `courageTier`, `loyaltyTier`, `wisdomTier`, `humilityTier`) VALUES
-- tier-1 six ("First Steps": capture up to six memories)
( 1, 170654, 62954, 1, 1, 2, 3, 5),  -- Kalisthene
( 2, 172177, 63168, 1, 1, 2, 3, 5),  -- Echthra
( 3, 172408, 63169, 1, 1, 2, 3, 5),  -- Alderyn and Myn'ir
( 4, 172410, 63170, 1, 1, 2, 3, 5),  -- Nuuminuuru
( 5, 172412, 63171, 1, 1, 2, 3, 5),  -- Craven Corinth
( 6, 172682, 63172, 1, 1, 2, 3, 5),  -- Splinterbark Nightmare
-- tier-2 four ("Sacred Trials": the capture of four more)
( 7, 172411, 63173, 2, 2, 3, 4, 5),  -- Thran'tiok
( 8, 172101, 63174, 2, 2, 3, 4, 5),  -- Mad Mortimer (creature 172101 'Ortim' - the quest 63174 kill credit)
( 9, 171873, 63175, 2, 2, 3, 4, 5),  -- Athanos
(10, 172333, 63176, 2, 2, 3, 4, 5);  -- Azaruux
