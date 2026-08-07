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

#ifndef QueensConservatory_h__
#define QueensConservatory_h__

#include "DatabaseEnvFwd.h"
#include "Define.h"
#include "Duration.h"
#include <array>
#include <map>
#include <tuple>
#include <unordered_map>
#include <vector>

class Player;

/*
 * The Queen's Conservatory - the Night Fae covenant's unique sanctum feature (P6 of the covenant plan).
 *
 * WHAT THE DATA SAYS THIS IS (every id below is read out of 12.0.7.68275 client DB2 or `integ_world`;
 * nothing here is invented - see the "NOT DERIVABLE" block further down for what deliberately is not):
 *
 *   GarrTalentTree 319 "The Queen's Conservatory"
 *       GarrTypeID 111 (covenant sanctum), MaxTiers 5,
 *       FeatureTypeIndex 5 (= GARR_TALENT_FEATURE_UNIQUE, client Enum.GarrTalentFeatureType.SanctumUnique),
 *       FeatureSubtypeIndex 3 (= CovenantID, client Enum.GarrTalentFeatureSubtype.Ardenweald).
 *
 *   Its five talents are the unlock ladder, not perks - every GarrTalentRank.PerkSpellID is 0.
 *   Their own descriptions say what each tier does, and that is where the plot count comes from:
 *       1086 "First Planting"    Tier 0  - restores the Conservatory ("nurture the wildseeds ... and prepare
 *                                          them for rebirth"), i.e. the first wildseed plot.
 *       1087 "Initial Growth"    Tier 1  - "Grants you access to catalyst plots, and activates an additional
 *                                          wildseed."
 *       1088 "Nurtured Souls"    Tier 2  - "activates additional wildseed and catalyst plots"
 *       1089 "Flourishing Beds"  Tier 3  - "Grants you access to additional wildseed and catalyst plots. You can
 *                                          now use the wildseed that has three possible catalyst connections."
 *       1090 "Final Forms"       Tier 4  - "activates the final wildseed plot. That Wildseed can benefit from
 *                                          four possible catalyst links."
 *   => the ladder is tiers + 1 wildseed pods, not one per tier: "First Planting" restores the Conservatory
 *   AND tier 1 "Initial Growth" already "activates an additional wildseed", so a tier-1 Conservatory has two
 *   pods and a fully researched one has six. [WEB, Wowhead "Night Fae Covenant Queen's Conservatory": "a
 *   fully upgraded Conservatory has 6 Wildseeds and 8 Anima Catalyst plots ... 1 pod with 1 catalyst link,
 *   3 pods with 2, 1 pod with 3 and 1 pod with 4".] That distribution is CatalystLinkCap below; the old
 *   "one pod per tier, capped at 5" made the 4-link pod unreachable.
 *
 *   Research costs (GarrTalentRank 1352-1356 + GarrTalentCost, both agree):
 *       T1 1500 x1813 + 6 x1810 @3600s, T2 5000 + 12 @43200s, T3 10000 + 22 @86400s,
 *       T4 12500 + 40 @86400s, T5 15000 + 70 @86400s.  (1813 Reservoir Anima, 1810 Redeemed Soul.)
 *   The generic Garrison::LearnTalent/ResearchTalent engine already charges and times these; this class
 *   deliberately does not re-implement any of it, it only reads the resulting talent ranks.
 *
 *   Harvest reward: GameObject 350978 "Queen's Conservatory Cache" (GAMEOBJECT_TYPE_CHEST, lockId 3218,
 *   chestLoot 350978). `integ_world.gameobject_loot_template` entry 350978 holds the real 40-row Shadowlands
 *   drop table (Novice's/Journeyman's/Artisan's/Spirit-Tender's Satchels, Ardenweald pets, weapons,
 *   Snapper/Gulper Souls) flattened into ONE table, i.e. rolling it pays out as if no catalyst had ever been
 *   attached. `garrison_conservatory_yield` (world migration 2026_08_07_63) splits it per catalyst
 *   combination and is what a harvest actually rolls; 350978 remains the fallback when that table is empty.
 *
 *   CATALYSTS ARE ITEMS, NOT GAMEOBJECTS. An earlier revision of this file named GameObjects 353652
 *   "Catalyst of Power" / 353653 "Catalyst of Renewal" / 353654 "Catalyst of Might" as the catalysts. They
 *   are not: their displayIds 64892/64893/64894 resolve through GameObjectDisplayInfo to
 *   world/expansion08/doodads/vampire/9vm_vampire_bottle*.m2 and Wowhead places all three in AreaTable 10413
 *   Revendreth (map 2222), while the Queen's Conservatory is AreaTable 13367 on map 2363. They are
 *   Revendreth props with a coincidental name and zero spawns. The real catalysts are three items sharing
 *   the Use spell 323169 "Infuse Catalyst" ("Infuse an Anima Catalyst Plot with this catalyst"), and each
 *   one's own description states its effect:
 *       176921 Temporal Leaves     (spell 323725) "reduce the Wildseed of Regrowth process by 1 day"
 *       176922 Wild Nightbloom     (spell 323787) "increases the yield of crafting materials ... by 100%"
 *       176832 Wildseed Root Grain (spell 336307) "increases the quality of rewards offered by a spirit"
 *   Both the effect and the reward ladder they drive are data, in `garrison_conservatory_catalyst` and
 *   `garrison_conservatory_yield`; the core hard-codes no item id. Corroboration that the tier/size reading
 *   is right: the twelve "... Satchel" items in this build are exactly the (root grain, nightbloom) pairs a
 *   four-link pod can produce - every missing one (Artisan's Overflowing, Spirit-Tender's Large/Stuffed/
 *   Overflowing) is a combination needing five links or more. Note also that the Wildseed of Regrowth
 *   (creature 165466) and the Anima Catalyst Plot (creature 165480) are CREATURES - validating a catalyst
 *   against gameobject_template, as the first revision did, could not model the real system at all.
 *
 *   Client surface: C_ArdenwealdGardening.IsGardenAccessible() and C_ArdenwealdGardening.GetGardenData()
 *   -> { number active, number ready, time_t remainingSeconds }. That is the whole Lua API - the namespace
 *   has no events and no mutators (Blizzard_ArdenwealdGardening.lua, shown as a tooltip on the covenant
 *   landing page).
 *
 *   HOW THE CLIENT IS FED - recovered by decompiling the client, not guessed. There is no garden opcode and
 *   no garden manager: GetGardenData (RVA 0x9FC9F0 in 12.0.7.68275, image base 0x7FF7B3140000) reads the
 *   local player's own aura list:
 *       active           = int(GetAuraBySpellID(player, 344292)->Points[0])   ; "mov edx, 0x540E4"
 *       ready            = int(GetAuraBySpellID(player, 344292)->Points[1])
 *       remainingSeconds = max(0, GetAuraBySpellID(player, 344304)->ExpireTimeMs - nowMs) * 0.001
 *                                                                              ; "mov edx, 0x540F0"
 *   and it returns nothing at all unless BOTH auras are present. So the server side of the tooltip is just
 *   two ordinary auras kept in sync - which is exactly what RefreshClientState below does.
 *   IsGardenAccessible (RVA 0x9FCF20) takes no server data: it is the generic client UI-system gate
 *   (system id 15) evaluating client DB2 PlayerConditions against already-replicated player state.
 *
 * NOT DERIVABLE OFFLINE - deliberately left as data, never guessed:
 *   * how long a wildseed takes to mature,
 *   * what planting one costs (currency and/or item) and which "wildseed" identities exist,
 *   * which catalyst combination changes the yield and how (the catalyst EFFECTS are client-derived, see
 *     above; the loot table each combination rolls is not, and lives in `garrison_conservatory_yield`).
 *   None of that has any representation in any 68275 DB2 or in `integ_world` (confirmed by inspection of
 *   GarrTalent, GarrTalentRank, GarrTalentCost, GarrTalentSocketProperties, the CharShipment tables and
 *   CurrencyTypes, and of the world DB's quest/loot/gameobject rows). Rather than invent numbers, every one of those values is a
 *   column of the world table `garrison_conservatory_wildseed`. The engine below is complete and runs off
 *   that table; with the table empty, PlantWildseed refuses with CONSERVATORY_ERROR_NO_WILDSEED_DATA and
 *   nothing else in the sanctum changes behaviour. Authoring rows (or a Shadowlands sniff) turns it on.
 */

enum ConservatoryConstants : uint32
{
    // GarrTalentTree.FeatureSubtypeIndex of tree 319 / client Enum.GarrTalentFeatureSubtype.Ardenweald.
    COVENANT_ID_NIGHT_FAE               = 3,
    // "Final Forms" (talent 1090): the last wildseed "can benefit from four possible catalyst links".
    CONSERVATORY_MAX_CATALYSTS          = 4,
    // MaxTiers of GarrTalentTree 319.
    CONSERVATORY_MAX_TIERS              = 5,
    // Wildseed pods at full research = tiers + 1 (see the ladder in the file header).
    CONSERVATORY_MAX_PLOTS              = CONSERVATORY_MAX_TIERS + 1,
    // Researched tiers needed before any catalyst plot exists - talent 1087 "Initial Growth" (tier 2)
    // "[g]rants you access to catalyst plots".
    CONSERVATORY_CATALYST_PLOTS_TIER    = 2,
    // GameObject 350978 "Queen's Conservatory Cache" - its chestLoot (gameobject_loot_template 350978) is the
    // stock harvest payout when a wildseed row does not name its own reward object.
    CONSERVATORY_DEFAULT_REWARD_GO      = 350978,
    // The two auras C_ArdenwealdGardening.GetGardenData() reads, taken straight out of the client's own code
    // (immediates 0x540E4 / 0x540F0 in GetGardenData). Both rows exist in this build's SpellName/SpellMisc.
    SPELL_CONSERVATORY_GARDEN_COUNTS    = 344292,   // Points[0] = growing, Points[1] = ready to harvest
    SPELL_CONSERVATORY_GARDEN_TIMER     = 344304    // remaining duration = seconds to the next maturation
};

// How an attached catalyst changes the pod. Every value is taken from the catalyst item's own description
// text (quoted in the file header and in world migration 2026_08_07_63) - none of it is a guess.
enum ConservatoryCatalystEffect : uint8
{
    CONSERVATORY_CATALYST_EFFECT_NONE           = 0,
    // Shifts the pod's maturesAt by effectValue seconds (Temporal Leaves: -86400 = "reduce ... by 1 day").
    CONSERVATORY_CATALYST_EFFECT_TIME_DELTA     = 1,
    // Raises the reward TIER (Wildseed Root Grain: "increases the quality of rewards"). The number of these
    // linked to the pod is the `rootGrainCount` key of `garrison_conservatory_yield`.
    CONSERVATORY_CATALYST_EFFECT_YIELD_QUALITY  = 2,
    // Raises the reward SIZE (Wild Nightbloom: "increases the yield of crafting materials ... by 100%").
    // The number linked is the `nightbloomCount` key of `garrison_conservatory_yield`.
    CONSERVATORY_CATALYST_EFFECT_YIELD_QUANTITY = 3,
    CONSERVATORY_CATALYST_EFFECT_MAX
};

enum ConservatoryPlotState : uint8
{
    CONSERVATORY_PLOT_EMPTY     = 0,
    CONSERVATORY_PLOT_GROWING   = 1,
    CONSERVATORY_PLOT_READY     = 2
};

enum ConservatoryError : uint32
{
    CONSERVATORY_OK = 0,
    CONSERVATORY_ERROR_NOT_NIGHT_FAE,       // owner is not pledged to the Night Fae
    CONSERVATORY_ERROR_NOT_UNLOCKED,        // GarrTalentTree 319 tier 1 ("First Planting") not researched
    CONSERVATORY_ERROR_INVALID_PLOT,        // plot index >= number of researched tiers
    CONSERVATORY_ERROR_PLOT_OCCUPIED,
    CONSERVATORY_ERROR_PLOT_EMPTY,
    CONSERVATORY_ERROR_NOT_READY,           // still maturing
    CONSERVATORY_ERROR_UNKNOWN_WILDSEED,    // no `garrison_conservatory_wildseed` row with that entry
    CONSERVATORY_ERROR_NO_WILDSEED_DATA,    // the table is empty - see the header comment
    CONSERVATORY_ERROR_TIER_TOO_LOW,        // wildseed requires more Conservatory tiers than are researched
    CONSERVATORY_ERROR_CANT_AFFORD,
    CONSERVATORY_ERROR_INVALID_CATALYST,        // no `garrison_conservatory_catalyst` row with that item id
    CONSERVATORY_ERROR_CATALYST_SLOT_TAKEN,     // this pod's catalyst links are all used up
    CONSERVATORY_ERROR_NO_CATALYST_PLOTS,       // catalyst plots arrive with tier 2 ("Initial Growth")
    CONSERVATORY_ERROR_NO_CATALYST_DATA,        // `garrison_conservatory_catalyst` is empty
    CONSERVATORY_ERROR_CATALYST_LIMIT,          // that catalyst's own maxPerPlot is already reached
    CONSERVATORY_ERROR_NO_CATALYST_ITEM,        // the character does not carry the catalyst item
    CONSERVATORY_ERROR_NO_YIELD_FOR_COMBINATION,// no `garrison_conservatory_yield` row for the resulting set
    CONSERVATORY_ERROR_NO_LOOT_TEMPLATE         // the yield row names a gameobject_loot_template that is absent
};

// One authored catalyst kind, keyed by the ITEM the player spends (world table
// `garrison_conservatory_catalyst`). See the file header for where each value comes from.
struct ConservatoryCatalystTemplate
{
    uint32 CatalystItemId = 0;
    ConservatoryCatalystEffect EffectType = CONSERVATORY_CATALYST_EFFECT_NONE;
    int32  EffectValue    = 0;      // TIME_DELTA: seconds (negative shortens). YIELD_*: unused.
    uint8  MaxPerPlot     = CONSERVATORY_MAX_CATALYSTS;
    uint32 SpellId        = 0;      // provenance only - the aura whose description defines the effect
};

// Key of `garrison_conservatory_yield`: which loot table a harvest rolls for a given catalyst set.
// spiritItemId 0 is the wildcard row that applies to every spirit.
struct ConservatoryYieldKey
{
    uint32 SpiritItemId    = 0;
    uint8  RootGrainCount  = 0;     // number of CONSERVATORY_CATALYST_EFFECT_YIELD_QUALITY links
    uint8  NightbloomCount = 0;     // number of CONSERVATORY_CATALYST_EFFECT_YIELD_QUANTITY links

    friend bool operator<(ConservatoryYieldKey const& l, ConservatoryYieldKey const& r)
    {
        return std::tie(l.SpiritItemId, l.RootGrainCount, l.NightbloomCount)
             < std::tie(r.SpiritItemId, r.RootGrainCount, r.NightbloomCount);
    }
};

// One authored wildseed kind. Every field that the client data does not publish lives here so that it is
// content, not a constant compiled into the core.
struct ConservatoryWildseedTemplate
{
    uint32 WildseedEntry        = 0;    // author-chosen id, referenced by character_garrison_conservatory
    uint32 CostCurrencyId       = 0;    // e.g. 1813 Reservoir Anima; 0 = no currency cost
    uint32 CostCurrencyCount    = 0;
    uint32 CostItemId           = 0;    // e.g. a wildseed item; 0 = no item cost
    uint32 CostItemCount        = 0;
    uint32 MaturationSeconds    = 0;    // 0 = unknown/instant-invalid, plant is refused
    uint32 RewardGameObjectId   = CONSERVATORY_DEFAULT_REWARD_GO;  // chest whose chestLoot is rolled on harvest
    uint8  RequiredTier         = 1;    // researched tiers of tree 319 needed to plant this kind
};

struct ConservatoryPlot
{
    uint8  PlotId       = 0;
    uint32 WildseedEntry = 0;
    time_t PlantedTime  = 0;
    time_t MaturesAt    = 0;
    // `garrison_conservatory_catalyst`.catalystItemId of each linked catalyst (176921/176922/176832 as
    // shipped). 0 = free link. How many of these are usable is CatalystLinkCap[PlotId], not the array size.
    std::array<uint32, CONSERVATORY_MAX_CATALYSTS> Catalysts = { };
    ConservatoryPlotState State = CONSERVATORY_PLOT_EMPTY;

    bool IsOccupied() const { return State != CONSERVATORY_PLOT_EMPTY; }
    uint32 CountCatalysts() const;
    uint32 CountCatalyst(uint32 catalystItemId) const;
};

class TC_GAME_API QueensConservatory
{
public:
    explicit QueensConservatory(Player* owner);

    // --- lifecycle -------------------------------------------------------------------------------------
    void LoadFromDB(PreparedQueryResult result);
    void SaveToDB(CharacterDatabaseTransaction trans) const;
    // Flips matured plots GROWING -> READY. Driven from Garrison::Update's existing 60s tick; also run once
    // right after load so a wildseed that matured while the character was offline comes back ready.
    void Update();

    // --- state -----------------------------------------------------------------------------------------
    // Mirrors C_ArdenwealdGardening.IsGardenAccessible(): Night Fae + at least tier 1 of tree 319.
    bool IsAccessible() const;
    // Researched tiers of GarrTalentTree 319 (talents 1086-1090 at Rank >= 1). 0 when not Night Fae.
    uint32 GetResearchedTiers() const;
    // Usable wildseed pods = researched tiers + 1, capped at CONSERVATORY_MAX_PLOTS (see header comment).
    uint32 GetPlotCount() const;
    // Catalyst plots unlock with tier 2, talent 1087 "Initial Growth".
    bool HasCatalystPlots() const;
    // How many catalysts pod `plotId` may hold: {1, 2, 2, 2, 3, 4}. Talent 1089 "Flourishing Beds" is the
    // pod "with three possible catalyst connections", 1090 "Final Forms" the one with four; the first four
    // pods carry the 1/2/2/2 the retail layout gives them. 0 for a pod that does not exist.
    static uint8 GetCatalystLinkCap(uint8 plotId);
    // Counts the catalysts linked to a pod by their effect type - the two numbers that key
    // `garrison_conservatory_yield`. Unknown catalyst ids (data removed under a planted pod) count for
    // nothing and are reported by the caller rather than silently treated as one of the two.
    void GetCatalystCounts(ConservatoryPlot const& plot, uint8& rootGrainCount, uint8& nightbloomCount) const;
    ConservatoryPlot const* GetPlot(uint8 plotId) const;
    std::vector<ConservatoryPlot const*> GetPlots() const;

    // The three numbers C_ArdenwealdGardening.GetGardenData() returns. remainingSeconds is the time until the
    // *soonest* still-growing wildseed matures (0 when none are growing), matching the addon's single-timer
    // tooltip line GARDENWEALD_STATUS_ACTIVE_COUNT.
    void GetGardenData(uint32& active, uint32& ready, int64& remainingSeconds) const;

    // Push those same three numbers to the client by (re)applying the two auras GetGardenData reads - 344292
    // carrying the counters in its effect points and 344304 carrying the countdown as its remaining duration.
    // Called after every state change and from the periodic tick. Safe on any build/state: if either spell is
    // missing from the spell store, or the garden is empty, the auras are simply removed.
    void RefreshClientState();

    // --- actions ---------------------------------------------------------------------------------------
    // Takes the wildseed's cost, occupies the plot and stamps MaturesAt = now + MaturationSeconds. Persists.
    ConservatoryError PlantWildseed(uint8 plotId, uint32 wildseedEntry);
    // Links a catalyst ITEM to a growing wildseed: validates it against `garrison_conservatory_catalyst`,
    // enforces this pod's link cap and the catalyst's own maxPerPlot, refuses combinations that have no
    // `garrison_conservatory_yield` row (so no pod can ever become unharvestable), CONSUMES one of the item,
    // applies a TIME_DELTA catalyst to MaturesAt straight away, and persists. Nothing is charged and nothing
    // is stored unless every one of those checks passed.
    ConservatoryError AttachCatalyst(uint8 plotId, uint32 catalystItemId);
    // Rolls the loot table `garrison_conservatory_yield` selects for the pod's catalyst set (falling back to
    // the wildseed's reward chest only while that table is empty), then empties the plot. Persists.
    ConservatoryError HarvestWildseed(uint8 plotId);
    // The loot table a harvest of `plot` would roll, and why not if it cannot be resolved. Shared by
    // HarvestWildseed and by `.garrison conservatory status` so the command never claims a payout the
    // harvest would refuse.
    ConservatoryError ResolveHarvestLootId(ConservatoryPlot const& plot, uint32& lootId) const;

private:
    // The owner's Conservatory tree (GarrTypeID 111, FeatureTypeIndex 5, FeatureSubtypeIndex = CovenantID).
    // Resolved from the DB2 stores rather than hard-coding 319, so the same code serves any covenant whose
    // unique feature is later modelled the same way. Returns 0 when the owner is not Night Fae.
    uint32 GetConservatoryTreeId() const;
    void MarkChanged() { _needsSave = true; }

    Player* _owner;
    std::unordered_map<uint8 /*plotId*/, ConservatoryPlot> _plots;
    bool _needsSave = false;
};

#endif // QueensConservatory_h__
