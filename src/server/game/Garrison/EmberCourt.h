/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EmberCourt_h__
#define EmberCourt_h__

#include "DatabaseEnvFwd.h"
#include "Define.h"
#include <ctime>
#include <unordered_map>
#include <vector>

class Player;

/*
 * The Ember Court - the Venthyr covenant's unique sanctum feature (P6.4 of the covenant plan).
 *
 * Every id below is read out of 12.0.7.68275 client DB2 or `integ_world`. The "NOT DERIVABLE" block at the
 * bottom lists exactly what is not, and why refusing beats guessing.
 *
 * ---------------------------------------------------------------------------------------------------------
 * THE CLIENT PROTOCOL: THERE ISN'T ONE - established before a line of this was written
 * ---------------------------------------------------------------------------------------------------------
 * The Ember Court adds NO opcode, NO Lua API, NO C_ namespace, NO addon and NO event to the 12.0.7.68275
 * client. Ten datasets were searched and every zero was method-validated against a known-present control
 * before it was believed:
 *   * `wow_offsets_68275.json` (14,571 lua_function + 2,542 c_binding): ember/court/guest/venthyr/soiree/
 *     revendreth/rsvp = 0. Controls `C_CovenantSanctumUI` 1, `C_Garrison` 1, `Soulbind` 28, `Renown` 21.
 *   * `wow_string_xrefs_68275.json` (181,200 xrefs): the ONLY "ember" literal in the whole table is
 *     `LE_FRAME_TUTORIAL_EMBER_COURT_MAP`. Controls `CovenantSanctum` 9, `garrtalent` 47.
 *   * `wow_dump.bin` (113 MB), ASCII and UTF-16LE: `EmberCourt` 0, `Ember Court` 0, `EMBER_COURT` 1 (that
 *     same CVar-bitfield tutorial tag). Controls `CovenantSanctum` 7, `GarrTalentFeatureSubtype` 2.
 *   * the 68275 UI source tree: no `Blizzard_EmberCourt*` addon exists; the single Ember-Court-aware file is
 *     `Blizzard_ObjectiveTracker/Blizzard_ScenarioObjectiveTracker.lua`, and all 14 of its matching lines are
 *     one tutorial HelpTip keyed on `EMBER_COURT_TUTORIAL_WIDGET_SET_ID = 461`. Control: 269 `C_` namespaces
 *     enumerated, of which `C_CovenantSanctumUI`/`C_Covenants`/`C_Soulbinds`/`C_Garrison` resolve and ZERO
 *     match ember/court/guest/soiree/tribute.
 *   * `all_cmsg_layouts_68275.json`, `all_smsg_layouts_68275.json`, `OPCODES_MASTER_12_0_7_68275.json` and
 *     TC's own `Opcodes.h`: 0 for every target term, against 4 / 10 / 14 / 13 present control opcodes.
 *   * `integ_world.playerchoice` + `playerchoice_response`: 0 Ember Court rows (every raw hit is
 *     "Contribute"). Control: `Question LIKE '%covenant%'` -> 3 rows (choice 644 "Which covenant will you
 *     join?").
 *   (`wow_opcode_dispatch_68275.json` was DISCARDED: all of its controls returned 0 too, so its zero is
 *   method-level noise, not evidence. The other three opcode datasets carry the negative.)
 *
 * That is the same answer the three finished siblings got, for the same structural reason:
 * `Enum.GarrTalentFeatureType.SanctumUnique = 5` is ONE shared slot for all four covenants' unique features
 * and the only thing identifying this one is `Enum.GarrTalentFeatureSubtype.Revendreth = 2` (both enums
 * recovered twice over - from the client's enum registrar at RVA 0xE78350/0xE79ACE and from
 * `Blizzard_APIDocumentationGenerated/GarrisonConstantsDocumentation.lua`). The feature's own display name is
 * the GlobalString `COVENANT_SANCTUM_FEATURE_VENTHYR` = "The Ember Court" (GlobalStrings.db2 row 43085), which
 * is why "Ember Court" is not in the executable at all.
 *
 * So the feature reaches the client over machinery that already exists:
 *   1. the UPGRADE TREE is an ordinary garrison talent tree. `C_CovenantSanctumUI.GetFeatures()` hands the
 *      client `{garrTalentTreeID, featureType, uiOrder}`; featureType 5 binds tree 324 to the shared
 *      `UniqueUpgrade` frame, which renders through `C_Garrison.GetTalentTreeInfo/GetTalentInfo` and buys
 *      through `CMSG_GARRISON_RESEARCH_TALENT`, refreshed by the generic `GARRISON_TALENT_*` events. The
 *      generic Garrison engine already speaks all of that; this class re-implements none of it.
 *   2. the PARTY ITSELF is a SCENARIO plus a UIWidget set. `ScenarioObjectiveTrackerStageMixin` calls
 *      `RegisterForWidgetSet(widgetSetID)` off the current stage, and every guest/attribute/tribute readout is
 *      generic `C_UIWidgetManager` data. That is independently confirmed from the DB2 side: ScenarioStep 4483
 *      ("The Court") carries WidgetSetID **461** - the exact constant the UI file hardcodes.
 *   3. INVITATIONS are gossip / quest / PlayerChoice, server-side, with no bespoke wire at all.
 * This class therefore sends no packet of its own and adds no opcode.
 *
 * ---------------------------------------------------------------------------------------------------------
 * THE UNLOCK LADDER (GarrTalentTree 324)
 * ---------------------------------------------------------------------------------------------------------
 *   GarrTalentTree 324 "The Ember Court"
 *       GarrTypeID 111 (covenant sanctum), MaxTiers 5, Flags 0, CurrencyID 0, PlayerConditionID 0,
 *       FeatureTypeIndex 5 (= GARR_TALENT_FEATURE_UNIQUE / Enum.GarrTalentFeatureType.SanctumUnique),
 *       FeatureSubtypeIndex 2 (= CovenantID / Enum.GarrTalentFeatureSubtype.Revendreth).
 *       NOTE it is `GarrTalentTreeType = 1` (Classic), where 319/320/321 are all type 0 (Tiers) - the one
 *       structural difference between this tree and its three siblings. That is why nothing below is keyed on
 *       "how many talents are researched": each unlock is read off ITS OWN talent, found by GarrTalent.Tier
 *       inside the resolved tree, so researching out of order cannot mis-grant a slot.
 *
 *   Its five talents are an unlock ladder, not perks - GarrTalentRank.PerkSpellID is 0 on all five ranks
 *   (1089, 1094, 1095, 1093, 1096). CONFIRMED by reading the rows, not assumed.
 *   The talents' own descriptions are a complete specification, and everything this class derives is quoted
 *   from them verbatim:
 *       1111 "A New Court"       Tier 0 (PlayerConditionID 84025 - the same tier-0 gate tree 320/321 share)
 *            "Re-establish the Ember Court, and start on a journey to restore it to its former glory. Help
 *             THEOTAR build this new court that will expand the influence of the Venthyr covenant across
 *             Revendreth and beyond."
 *       1113 "Homegrown Help"    Tier 1
 *            "You will now be able to create A DREDGER BUTLER to assist you in hosting the Ember Court."
 *       1114 "Court Influencer"  Tier 2
 *            "Now you can expand the guest list, allowing you to invite A THIRD GUEST to each Ember Court."
 *       1112 "Discerning Taste"  Tier 3
 *            "Attract the attention of even the most discerning guests, allowing you to invite A FOURTH GUEST
 *             to each Ember Court."
 *       1115 "The Professionals" Tier 4
 *            "You are now able to hire FIVE SPECIALIST STAFF who will help you foster the perfect atmosphere
 *             in each and every Ember Court event."
 *   => the guest list is 2 slots at the bottom of the ladder, 3 once "Court Influencer" is in, 4 once
 *      "Discerning Taste" is. It is 2 and not some other number because tier 2 grants "a THIRD guest": the
 *      count before it is stated, not chosen. That reading is independently confirmed by the scenario, which
 *      ships FOUR alternative Tribute steps - CriteriaTree 84848 "Step 3A (1 Guest)", 87110 "3B (2 Guests)",
 *      87112 "3C (3 Guests)", 87114 "3D (4 Guests)" - i.e. 1-4 guests is the whole supported range.
 *
 *   Research costs (GarrTalentRank, currency 1813 Reservoir Anima):
 *       Tier 0 rank 1089  1500 @3600s      Tier 1 rank 1094   5000 @43200s
 *       Tier 2 rank 1095 10000 @86400s     Tier 3 rank 1093  12500 @86400s (PerkPlayerConditionID 70102)
 *       Tier 4 rank 1096 15000 @86400s
 *   The generic Garrison::LearnTalent/ResearchTalent engine already charges and times these; this class only
 *   reads the resulting ranks.
 *
 * ---------------------------------------------------------------------------------------------------------
 * THE ACTIVITY IS A SCENARIO IN SINFALL, AND ITS STEPS NAME THE WHOLE LOOP
 * ---------------------------------------------------------------------------------------------------------
 * The plan (§A22/§D.10) recorded this feature as having "no DB2 representation". It has a great deal:
 *
 *   Scenario 1791 "The Ember Court"  Type 0, Flags 2, UiTextureKitID 5349.  LFGDungeons 2063 -> 1791.
 *       ScenarioStep 4463 OrderIndex 0 "Last-Minute Preparations"  CriteriaTree 84766  WidgetSetID 459
 *                         "Prepare the court and issue orders to your staff."
 *       ScenarioStep 4483 OrderIndex 1 "The Court"                 CriteriaTree 84846  WidgetSetID 461
 *                         "Ensure that your guests enjoy the party!"
 *       ScenarioStep 4484/4601/4602/4603 OrderIndex 2 "Tribute"    CriteriaTree 84848 / 87110 / 87112 / 87114
 *                         "Collect the tribute left by your guests."   (one variant per 1/2/3/4 guests)
 *       ScenarioStep 4527 OrderIndex 3 "The After Party"           CriteriaTree 85576
 *                         "Unwind and enjoy the leftovers. Stay as long as you like!"
 *   Scenario 1820 "Ember Court Rehearsal" is the tutorial twin (LFGDungeons 2088), steps 4585-4593, and its
 *   step text is where the attribute vocabulary is stated in plain English: "Temel likes a CLEAN court",
 *   "Theotar likes a FORMAL court, so offer him some tea", "Watchmaster Boromod likes a CASUAL court, so
 *   start a fight to entertain him", "Not everyone prefers the court to be Formal."
 *
 *   AreaTable 13329 ZoneName "SinfallScenario" / AreaName "The Ember Court", ContinentID 2222,
 *   ParentAreaID 10413 (Revendreth). UiMap 1644 "Ember Court" (ParentUiMapID 1525 = Revendreth).
 *   The venue is therefore NOT a separate instance map - it is an area of map 2222, the same map the Sinfall
 *   sanctum sits on (area 12917).
 *
 *   Reputation: Faction 2445 "The Ember Court" ("A new court has opened in Revendreth, where Prince Renathal
 *   attempts to unite allies from across the Shadowlands"), fed by currency 1837 "The Ember Court"
 *   ("Increases the reputation of the The Ember Court") via CurrencyContainer 160 "Ember Court Prestige".
 *   PlayerCondition 84504/84505/84506/84507 are its Friendly/Honored/Revered/Exalted gates.
 *   Currency 1820 "Infused Ruby" (MaxQty 100) is the supply currency dredgers trade goods for.
 *
 * ---------------------------------------------------------------------------------------------------------
 * THE SIXTEEN GUESTS, AND WHY THIS ROSTER IS DATA AND NOT A GUESS
 * ---------------------------------------------------------------------------------------------------------
 * Achievement 14723 "Be Our Guest" ("Host the following guests at your Ember Court") hangs off CriteriaTree
 * 87983, whose sixteen children 87984-87999 name the guests in a fixed OrderIndex 0-15. Achievement 14724
 * "People Pleaser" ("Help the following guests reach the ELATED mood level") repeats the identical sixteen
 * under tree 88000. Each child points at a Criteria of Type 27 (= CriteriaType::CompleteQuest, verified
 * against TC's own DBCEnums.h) whose Asset is a hidden per-guest credit quest.
 *
 * Independently, `integ_world` carries sixteen quests literally titled "RSVP: <Guest>", and they are the same
 * sixteen names in the same set, each started AND ended by that guest's own creature. That is a two-source
 * agreement, so the roster below is derived twice over:
 *
 *   idx  guest                     RSVP quest  creature  hosted-credit  elated-credit
 *    0   Baroness Vashj              61174      162487      62487          62503
 *    1   Lady Moonberry              61354      172098      62488          62504
 *    2   Mikanikos                   61173      171647      62489          62505
 *    3   The Countess                60948      171106      62490          62506
 *    4   Alexandros Mograine         61255      171933      62491          62508
 *    5   Hunt-Captain Korayn         61109      171319      62492          62509
 *    6   Polemarch Adrestes          61123      171385      62493          62510
 *    7   Rendle and Cudgelface       61059      171190      62494          62507
 *    8   Choofa                      61139      160814      62495          62511
 *    9   Cryptkeeper Kassir          60236      163073      62496          62512
 *   10   Droman Aliothe              61129      160894      62497          62513
 *   11   Grandmaster Vole            61092      163019      62498          62514
 *   12   Kleia and Pelagos           61256      171951      62499          62515
 *   13   Plague Deviser Marileth     61105      159930      62500          62516
 *   14   Sika                        61130      166577      62501          62517
 *   15   Stonehead                   60916      157199      62502          62518
 * (the elated-credit column is NOT in guest order for indices 4 and 7 - 62507 belongs to Rendle and 62508 to
 * Alexandros. That is what the rows say; it is reproduced rather than "tidied".)
 *
 * A guest is UNLOCKED for invitation by completing that guest's "RSVP: <Guest>" quest. That is the game's own
 * mechanism, already present in this world DB, so this class tests the quest rather than inventing a flag.
 *
 * ---------------------------------------------------------------------------------------------------------
 * THE FIVE ATTRIBUTES
 * ---------------------------------------------------------------------------------------------------------
 * Achievement 14726 "It's Certainly Never Boring" ("Host Ember Courts with the following attributes at high
 * levels") hangs off CriteriaTree 88023, whose ten children 88024-88033 are, in OrderIndex order:
 *     Messy, Clean, Safe, Dangerous, Humble, Decadent, Relaxing, Exciting, Casual, Formal.
 * They pair into five two-ended axes, and UiWidgetVisualization names the axes and their direction explicitly:
 *     1438 "Capture Bar - 1. Cleanliness (Messy>Clean)"
 *     1440 "Capture Bar - 2. Danger (Safe>Dangerous)"
 *     1437 "Capture Bar - 3. Decadence (Humble>Decadent)"
 *     1439 "Capture Bar - 4. Excitement (Relaxing>Exciting)"
 *     1435 "Capture Bar - 5. Formality (Casual>Formal)"
 * plus 1398 "Attributes - Capture Bar", 1408 "Attributes - Icons", 1400 "Happiness - Honor Bar",
 * 1405/1406 "Happiness - Box / Nameplate". The axis ORDER above is the client's own numbering, and it is what
 * EmberCourtAttribute encodes.
 *
 * ---------------------------------------------------------------------------------------------------------
 * THE MOOD LADDER AND EVERY GUEST'S TASTE - BOTH PUBLISHED, BOTH READ FROM THE CLIENT
 * ---------------------------------------------------------------------------------------------------------
 * The mood scale has FIVE rungs, and it is published three independent ways that agree exactly:
 *     Miserable < Uncomfortable < Happy < Very Happy < Elated
 *   * `SpellName` 327199 "UI: Miserable [DNT]", 327200 "UI: Uncomfortable", 327201 "UI: Happy",
 *     327781 "UI: Very Happy", 327202 "UI: Elated" (327781 sits outside the contiguous run; that is what the
 *     rows say). 346342 "UI: Angry" exists but belongs only to item 184534 "Entitled Guest", a joke item.
 *   * `UiWidgetStringSource.Value_lang` carries literal "Mood: <Rung>" strings, and a count over that table
 *     returns EXACTLY those five rungs and no sixth (12-13 rows each, one per guest).
 *   * `ManifestInterfaceData` ships UI_EmberCourt-Emoji-{Miserable,Uncomfortable,Happy,VeryHappy,Elated}.blp
 *     as FileDataIDs 3750310-3750314.
 * "Elated", the one rung Achievement 14724 names, is therefore the TOP of the ladder = 5. (It is genuinely
 * absent from GlobalStrings.db2 - that check was right, it was just looking in the wrong table.)
 *
 * Each guest's taste is published too, on that guest's own mood-icon item in `ItemSparse`: the item's
 * Display_lang is the guest's name and its Description_lang is a literal "Likes: <poles>" list. The item name
 * binds the row to the guest directly, so nothing here depends on an assumed ordering:
 *     178886 Baroness Vashj          Likes: Dangerous, Decadent, Exciting
 *     181338 Lady Moonberry          Likes: Messy, Exciting, Casual
 *     181339 Mikanikos               Likes: Clean, Safe, Humble
 *     181340 The Countess            Likes: Decadent, Relaxing, Formal
 *     181341 Alexandros Mograine     Likes: Safe, Humble
 *     181342 Hunt-Captain Korayn     Likes: Dangerous, Casual
 *     178887 Polemarch Adrestes      Likes: Clean, Formal
 *     181343 Rendle and Cudgelface   Likes: Messy, Relaxing
 *     178888 Choofa                  Likes: Exciting
 *     178889 Cryptkeeper Kassir      Likes: Formal
 *     181344 Droman Aliothe          Likes: Relaxing
 *     181345 Grandmaster Vole        Likes: Dangerous
 *     181346 Kleia and Pelagos       Likes: Humble
 *     181347 Plague Deviser Marileth Likes: Messy
 *     181348 Sika                    Likes: Clean
 *     181349 Stonehead               Likes: Casual
 * (178677 Prince Renathal "Formal, Clean, Safe" and 181390/181391/181392 Temel/Theotar/Boromod
 * "Clean"/"Formal"/"Casual" are the HOST and the rehearsal cast, not guests, so they are not in the roster -
 * but note 181390-181392 reproduce the rehearsal's own step text exactly, which cross-validates the reading.)
 *
 * The five axes are confirmed a third time by the client's own "Adjust World State" spells - 321808
 * Cleanliness, 321809 Danger, 321810 Decadence, 321811 Excitement, 321812 Formality (plus 322728 Bonus
 * Happiness). Their order is the order EmberCourtAttribute uses, so the enum is not a local invention.
 *
 * ---------------------------------------------------------------------------------------------------------
 * NOT DERIVABLE OFFLINE - deliberately left as data, never guessed
 * ---------------------------------------------------------------------------------------------------------
 *   * WHAT EACH GUEST DISLIKES. Only "Likes:" strings exist. A scan of all 172,667 ItemSparse descriptions for
 *     "dislike" returns four unrelated items, so the method works and the absence is real. Assuming a guest
 *     dislikes the opposite pole of what it likes would be a guess, so it is not assumed - the dislike is the
 *     one column of the world table `garrison_ember_court_guest`, which SHIPS EMPTY.
 *   * THE NUMERIC HAPPINESS THRESHOLDS behind the five rungs. The rung shown is driven by per-guest,
 *     per-rung shown-state WorldStates (190 widgets over WorldState 32788-33058) that the SERVER sets; no
 *     constant anywhere says how much happiness earns which rung. So this class never COMPUTES a mood - a
 *     mood is only ever reported to it by a real court completion, and it is validated against the ladder.
 *   * THE TRIBUTE PAYOUT. There is no tribute-chest gameobject at all: `gameobject_template` has ZERO rows
 *     whose name contains "Ember Court" and `GameObjects.db2` has none on map 2222 (control: 449 rows on map
 *     2222, all Bastion WMO labels; 2,953 on map 0). With no chest there is no `gameobject_loot_template`,
 *     and every one of the sixteen Ember Court `ModifierTree` ids is ABSENT from a 230,888-row table that is
 *     dense around them (4,693 rows in 145000-152000) - i.e. the rating->reward logic is server-side only.
 *     Nothing here awards tribute.
 *   * THE COURT COOLDOWN. Spell 336617 "Ember Court Timer" has DurationIndex 0, no `SpellCooldowns` row, no
 *     `SpellCategories` row and no `SpellAuraOptions` row, and its own description ("You have limited time to
 *     help Temel clean up the court before your first guest arrives!") shows it is the REHEARSAL PREP timer,
 *     not a lockout. No interval is enforced. `lastCourtTime` is recorded and exposed instead.
 *   * THE SUPPLY COSTS. Not implemented here at all - nothing in this class buys or charges for anything.
 *     (For whoever does build it: the supplies are the sixteen "Contract:" items 176126-176141, which have no
 *     vendor row and are earned from the 24 "Restock:" quests at 5 Infused Ruby each - quest 62078 costs 20.)
 *
 *   * AND THE VENUE ITSELF IS NOT AUTHORED IN THIS WORLD DB. Method-validated:
 *         SELECT COUNT(*) FROM scenarios WHERE map = 2222;                 -> 0   (317 rows exist overall)
 *         SELECT COUNT(*) FROM creature  WHERE id  = 164966;               -> 0   (Temel, The Party Herald)
 *         SELECT COUNT(*) FROM creature  WHERE id IN (165453,165490,165493,165494,165496); -> 0  (all 5 staff)
 *         SELECT COUNT(*) FROM creature  WHERE areaId = 13329;             -> 0   (control: area 12917 -> 130)
 *         SELECT COUNT(*) FROM gameobject WHERE areaId = 13329;            -> 0
 *         34 `creature_template` rows are named "Ember Court Socialite"/"Ember Court Noble" and ALL have 0 spawns.
 *     Because of that StartCourt REFUSES with EMBER_COURT_ERROR_NO_VENUE_CONTENT rather than starting a party
 *     that could never be attended, and a court is NEVER auto-completed: the only way attendance is recorded
 *     is a real completion (or an explicit GM command, which is a GM tool, not a fallback). Authoring the
 *     `scenarios` row plus the Sinfall spawns turns it on with no code change here.
 */

enum EmberCourtConstants : uint32
{
    // GarrTalentTree.FeatureSubtypeIndex of tree 324 / client Enum.GarrTalentFeatureSubtype.Revendreth.
    COVENANT_ID_VENTHYR                 = 2,
    // MaxTiers of GarrTalentTree 324.
    EMBER_COURT_MAX_TIERS               = 5,
    // Scenario 1791 "The Ember Court" (LFGDungeons 2063); 1820 "Ember Court Rehearsal" is the tutorial twin.
    EMBER_COURT_SCENARIO_ID             = 1791,
    EMBER_COURT_REHEARSAL_SCENARIO_ID   = 1820,
    // AreaTable 13329 "SinfallScenario" / "The Ember Court", ContinentID 2222, ParentAreaID 10413.
    EMBER_COURT_MAP_ID                  = 2222,
    EMBER_COURT_AREA_ID                 = 13329,
    // CriteriaTree 87983 "Be Our Guest" has exactly sixteen children, OrderIndex 0-15.
    EMBER_COURT_GUEST_COUNT             = 16,
    // Talent 1114 grants "a THIRD guest", so the count before any guest-list talent is two.
    EMBER_COURT_BASE_GUEST_SLOTS        = 2,
    // Talent 1115 "The Professionals": "hire FIVE specialist staff".
    EMBER_COURT_STAFF_SLOTS             = 5,
    // GarrTalent.Tier inside tree 324. Each unlock is keyed on its own talent, not on a researched count,
    // because 324 is a Classic tree (GarrTalentTreeType 1) and can be researched out of order.
    EMBER_COURT_TIER_UNLOCK             = 0,    // 1111 "A New Court"
    EMBER_COURT_TIER_BUTLER             = 1,    // 1113 "Homegrown Help"     - the dredger butler
    EMBER_COURT_TIER_THIRD_GUEST        = 2,    // 1114 "Court Influencer"   - a third guest
    EMBER_COURT_TIER_FOURTH_GUEST       = 3,    // 1112 "Discerning Taste"   - a fourth guest
    EMBER_COURT_TIER_STAFF              = 4,    // 1115 "The Professionals"  - five specialist staff
    // Faction 2445 "The Ember Court", fed by currency 1837 via CurrencyContainer 160 "Ember Court Prestige".
    EMBER_COURT_FACTION_ID              = 2445,
    EMBER_COURT_CURRENCY_ID             = 1837,
    // Currency 1820 "Infused Ruby" (MaxQty 100) - what dredgers trade Court goods for.
    EMBER_COURT_SUPPLY_CURRENCY_ID      = 1820
};

// The five attribute axes, in the client's own numbering (UiWidgetVisualization 1438/1440/1437/1439/1435,
// "Capture Bar - N. <Axis> (<low pole>><high pole>)"). The pole names are CriteriaTree 88024-88033, the
// children of Achievement 14726 "It's Certainly Never Boring".
enum EmberCourtAttribute : uint8
{
    EMBER_COURT_ATTRIBUTE_NONE          = 0,
    EMBER_COURT_ATTRIBUTE_CLEANLINESS   = 1,    // Messy    <-> Clean
    EMBER_COURT_ATTRIBUTE_DANGER        = 2,    // Safe     <-> Dangerous
    EMBER_COURT_ATTRIBUTE_DECADENCE     = 3,    // Humble   <-> Decadent
    EMBER_COURT_ATTRIBUTE_EXCITEMENT    = 4,    // Relaxing <-> Exciting
    EMBER_COURT_ATTRIBUTE_FORMALITY     = 5,    // Casual   <-> Formal
    EMBER_COURT_ATTRIBUTE_MAX           = 5
};

// Which end of an axis. Every axis is a two-ended capture bar, never a single scalar - that is what the
// "(Messy>Clean)" naming in UiWidgetVisualization means, and what the rehearsal's "Not everyone prefers the
// court to be Formal" states in words.
enum EmberCourtAttributePole : uint8
{
    EMBER_COURT_POLE_NONE   = 0,
    EMBER_COURT_POLE_LOW    = 1,    // Messy / Safe / Humble / Relaxing / Casual
    EMBER_COURT_POLE_HIGH   = 2     // Clean / Dangerous / Decadent / Exciting / Formal
};

// The mood ladder, five rungs. Published three ways that agree exactly - SpellName 327199/327200/327201/
// 327781/327202 ("UI: <Rung> [DNT]"), the literal "Mood: <Rung>" strings in UiWidgetStringSource (a count
// over that table returns exactly these five and no sixth), and the five
// UI_EmberCourt-Emoji-<Rung>.blp icons (FileDataID 3750310-3750314). "Elated" is the rung Achievement 14724
// "People Pleaser" requires, and it is the top.
enum EmberCourtMood : uint8
{
    EMBER_COURT_MOOD_NONE           = 0,    // never hosted / not reported
    EMBER_COURT_MOOD_MISERABLE      = 1,
    EMBER_COURT_MOOD_UNCOMFORTABLE  = 2,
    EMBER_COURT_MOOD_HAPPY          = 3,
    EMBER_COURT_MOOD_VERY_HAPPY     = 4,
    EMBER_COURT_MOOD_ELATED         = 5,
    EMBER_COURT_MOOD_MAX            = 5
};

enum EmberCourtError : uint32
{
    EMBER_COURT_OK = 0,
    EMBER_COURT_ERROR_NOT_VENTHYR,          // owner is not pledged to the Venthyr
    EMBER_COURT_ERROR_NOT_UNLOCKED,         // GarrTalentTree 324 tier 0 (talent 1111 "A New Court") not researched
    EMBER_COURT_ERROR_UNKNOWN_GUEST,        // guest index outside 0-15
    EMBER_COURT_ERROR_GUEST_NOT_UNLOCKED,   // that guest's "RSVP: <Guest>" quest has not been completed
    EMBER_COURT_ERROR_GUEST_ALREADY_INVITED,
    EMBER_COURT_ERROR_GUEST_NOT_INVITED,
    EMBER_COURT_ERROR_GUEST_SLOTS_FULL,     // the guest list is as long as the researched talents allow (2-4)
    EMBER_COURT_ERROR_NO_GUESTS_INVITED,    // a court with nobody on the list is not a court
    EMBER_COURT_ERROR_INVALID_MOOD,         // a reported mood outside the five-rung ladder
    EMBER_COURT_ERROR_NO_VENUE_CONTENT      // scenario 1791 / area 13329 unauthored - refuse rather than fake it
};

// One of the sixteen guests. This is DERIVED data (see the roster table in the file header), so it lives in
// code rather than in a world table - exactly like the four Path of Ascension trial difficulties.
struct EmberCourtGuest
{
    uint8       Index           = 0;    // CriteriaTree 87983 child OrderIndex, 0-15
    char const* Name            = "";
    uint32      RsvpQuestId     = 0;    // "RSVP: <Guest>" in `integ_world`; completing it unlocks the guest
    uint32      CreatureId      = 0;    // that guest's creature_template entry
    uint32      HostedCriteriaQuestId = 0;  // Criteria(Type 27).Asset under Achievement 14723 "Be Our Guest"
    uint32      ElatedCriteriaQuestId = 0;  // ... under Achievement 14724 "People Pleaser"
    uint32      MoodItemId  = 0;    // this guest's ItemSparse mood-icon item; its Description is "Likes: ..."
    // What this guest LIKES, indexed by EmberCourtAttribute (so [0] is unused). The value is the pole of that
    // axis the guest enjoys, or EMBER_COURT_POLE_NONE when the guest does not care about that axis. Read
    // verbatim from MoodItemId's ItemSparse Description_lang - see the roster in the file header.
    uint8       LikedPoles[EMBER_COURT_ATTRIBUTE_MAX + 1] = { };
};

// The AUTHORED half of a guest: the ONE thing the client does not publish about them. One row of
// `garrison_ember_court_guest`. What a guest LIKES is client data and lives in EmberCourtGuest above; only
// the DISLIKE is missing from the build, and it is not inferred from the like.
struct EmberCourtGuestTemplate
{
    uint8 GuestIndex        = 0;
    uint8 DislikedAttribute = EMBER_COURT_ATTRIBUTE_NONE;
    uint8 DislikedPole      = EMBER_COURT_POLE_NONE;
};

// One guest's standing with this character.
struct EmberCourtGuestState
{
    uint8  GuestIndex       = 0;
    uint32 TimesHosted      = 0;
    uint8  HighestMood      = 0;    // high-water mark on the unpublished mood scale; 0 = never hosted
    time_t LastHostedTime   = 0;
    bool   Invited          = false;    // on the guest list for the NEXT court
};

class TC_GAME_API EmberCourt
{
public:
    explicit EmberCourt(Player* owner);

    // The sixteen-guest roster, derived twice over (see the file header). Never empty.
    static std::vector<EmberCourtGuest> const& GetGuestRoster();
    static EmberCourtGuest const* GetGuestInfo(uint8 guestIndex);
    static char const* GetAttributeName(EmberCourtAttribute attribute);
    // "Messy"/"Clean" etc - the CriteriaTree 88024-88033 pole names for an axis.
    static char const* GetAttributePoleName(EmberCourtAttribute attribute, EmberCourtAttributePole pole);
    // "Miserable".."Elated" - the five-rung ladder, see EmberCourtMood.
    static char const* GetMoodName(EmberCourtMood mood);
    // Does this guest enjoy that end of that axis? Pure client data (the guest's "Likes:" list).
    static bool IsAttributeLiked(uint8 guestIndex, EmberCourtAttribute attribute, EmberCourtAttributePole pole);
    // Does this guest dislike it? Answers false unless a `garrison_ember_court_guest` row says so - the build
    // publishes no dislikes, and the opposite of a like is NOT assumed to be one.
    static bool IsAttributeDisliked(uint8 guestIndex, EmberCourtAttribute attribute, EmberCourtAttributePole pole);

    // --- lifecycle -------------------------------------------------------------------------------------
    void LoadFromDB(PreparedQueryResult result);
    void SaveToDB(CharacterDatabaseTransaction trans) const;
    // Driven from Garrison::Update's existing 60s tick. Sends nothing - there is no client protocol (see the
    // header); it only trims state a talent reset can invalidate.
    void Update();

    // --- state -----------------------------------------------------------------------------------------
    // Venthyr, with talent 1111 "A New Court" researched.
    bool IsAccessible() const;
    // Researched talents of GarrTalentTree 324 (0-5). Reported for diagnostics only - no unlock is keyed on it.
    uint32 GetResearchedTiers() const;
    // Guests that may be on one guest list: 2, +1 with "Court Influencer", +1 with "Discerning Taste".
    // Zero when the feature is not unlocked at all.
    uint32 GetGuestSlots() const;
    // Talent 1113 "Homegrown Help" - "create a dredger butler to assist you in hosting the Ember Court".
    bool HasDredgerButler() const;
    // Talent 1115 "The Professionals" - "hire five specialist staff". 0 or EMBER_COURT_STAFF_SLOTS.
    uint32 GetStaffSlots() const;

    // A guest may be invited once their own "RSVP: <Guest>" quest has been completed. That is the game's own
    // unlock; no separate flag is stored.
    bool IsGuestUnlocked(uint8 guestIndex) const;
    bool IsGuestInvited(uint8 guestIndex) const;
    std::vector<uint8> GetInvitedGuests() const;
    EmberCourtGuestState const* GetGuestState(uint8 guestIndex) const;
    std::vector<EmberCourtGuestState const*> GetGuestStates() const;

    uint32 GetCourtsHeld() const { return _courtsHeld; }
    // Unix time of the last completed court, 0 if never. NO minimum interval is enforced between courts - the
    // 68275 build publishes none (see NOT DERIVABLE in the header) and one is not invented here.
    time_t GetLastCourtTime() const { return _lastCourtTime; }

    // --- actions ---------------------------------------------------------------------------------------
    // Put a guest on the list for the next court. Validates covenant, unlock, RSVP and slot count. Persists.
    EmberCourtError InviteGuest(uint8 guestIndex);
    // Take a guest back off the list. Persists.
    EmberCourtError UninviteGuest(uint8 guestIndex);
    // Clear the whole guest list.
    void ClearInvitations();

    // Check every gate for actually holding the court. Answers EMBER_COURT_ERROR_NO_VENUE_CONTENT when
    // scenario 1791 / area 13329 is unauthored, so a party is never "started" that nobody could attend.
    // Makes no state change of its own - attendance is only ever recorded by CompleteCourt.
    EmberCourtError StartCourt() const;

    // Record a court that actually happened. `moods` maps guest index -> the mood rung that guest reached on
    // the unpublished scale; a guest absent from the map is recorded as attended at mood 0. Raises each
    // guest's TimesHosted and mood high-water mark, clears the guest list, and awards the guest's "hosted"
    // criteria credit so the client's own achievements pay out. Persists.
    // Refuses everything StartCourt refuses EXCEPT the venue check, so a real scenario completion can call it.
    EmberCourtError CompleteCourt(std::unordered_map<uint8, uint8> const& moods);

private:
    // The unique-feature tree of the owner's covenant: GarrTypeID 111 + FeatureTypeIndex 5 (SanctumUnique) +
    // FeatureSubtypeIndex == CovenantID. For the Venthyr that resolves to tree 324. Read from the DB2 stores
    // rather than hardcoded. Returns 0 when the owner is not Venthyr.
    uint32 GetEmberCourtTreeId() const;
    // Is the talent at `tier` of tree 324 researched (Rank >= 1)? This is the single unlock primitive - see
    // the note on GarrTalentTreeType 1 in the header.
    bool HasTalentAtTier(uint32 tier) const;
    EmberCourtGuestState& GetOrCreateGuestState(uint8 guestIndex);
    void MarkChanged() { _needsSave = true; }

    Player* _owner;
    std::unordered_map<uint8 /*guestIndex*/, EmberCourtGuestState> _guests;
    uint32 _courtsHeld = 0;
    time_t _lastCourtTime = 0;
    bool _needsSave = false;
};

#endif // EmberCourt_h__
