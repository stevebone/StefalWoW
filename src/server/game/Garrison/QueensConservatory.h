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
 *   => one wildseed plot per researched tier, 5 at full build; at most 4 catalyst links on a plot.
 *
 *   Research costs (GarrTalentRank 1352-1356 + GarrTalentCost, both agree):
 *       T1 1500 x1813 + 6 x1810 @3600s, T2 5000 + 12 @43200s, T3 10000 + 22 @86400s,
 *       T4 12500 + 40 @86400s, T5 15000 + 70 @86400s.  (1813 Reservoir Anima, 1810 Redeemed Soul.)
 *   The generic Garrison::LearnTalent/ResearchTalent engine already charges and times these; this class
 *   deliberately does not re-implement any of it, it only reads the resulting talent ranks.
 *
 *   Harvest reward: GameObject 350978 "Queen's Conservatory Cache" (GAMEOBJECT_TYPE_CHEST, lockId 3218,
 *   chestLoot 350978). `integ_world.gameobject_loot_template` entry 350978 already holds the real 41-row
 *   Shadowlands drop table (Novice's/Journeyman's/Artisan's/Spirit-Tender's Satchels, Ardenweald pets,
 *   weapons, Snapper/Gulper Souls). That template - not a hand-written list - is what a harvest pays out.
 *
 *   The catalysts are GameObjects 353652 "Catalyst of Power", 353653 "Catalyst of Renewal" and
 *   353654 "Catalyst of Might" (all GAMEOBJECT_TYPE_GOOBER). They exist as templates in `integ_world`
 *   but have zero spawns.
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
 *   * which catalyst combination changes the yield and how.
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
    // MaxTiers of GarrTalentTree 319, i.e. the most wildseed plots a fully researched Conservatory has.
    CONSERVATORY_MAX_PLOTS              = 5,
    // GameObject 350978 "Queen's Conservatory Cache" - its chestLoot (gameobject_loot_template 350978) is the
    // stock harvest payout when a wildseed row does not name its own reward object.
    CONSERVATORY_DEFAULT_REWARD_GO      = 350978,
    // The two auras C_ArdenwealdGardening.GetGardenData() reads, taken straight out of the client's own code
    // (immediates 0x540E4 / 0x540F0 in GetGardenData). Both rows exist in this build's SpellName/SpellMisc.
    SPELL_CONSERVATORY_GARDEN_COUNTS    = 344292,   // Points[0] = growing, Points[1] = ready to harvest
    SPELL_CONSERVATORY_GARDEN_TIMER     = 344304    // remaining duration = seconds to the next maturation
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
    CONSERVATORY_ERROR_INVALID_CATALYST,
    CONSERVATORY_ERROR_CATALYST_SLOT_TAKEN,
    CONSERVATORY_ERROR_NO_CATALYST_PLOTS    // catalyst plots arrive with tier 2 ("Initial Growth")
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
    std::array<uint32, CONSERVATORY_MAX_CATALYSTS> Catalysts = { };  // GameObject entries 353652/353653/353654
    ConservatoryPlotState State = CONSERVATORY_PLOT_EMPTY;

    bool IsOccupied() const { return State != CONSERVATORY_PLOT_EMPTY; }
    uint32 CountCatalysts() const;
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
    // Researched tiers of GarrTalentTree 319 = number of usable wildseed plots (see header comment).
    uint32 GetPlotCount() const;
    // Catalyst plots unlock with tier 2, talent 1087 "Initial Growth".
    bool HasCatalystPlots() const;
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
    // Attaches a catalyst GameObject entry to a growing wildseed. Persists.
    ConservatoryError AttachCatalyst(uint8 plotId, uint8 slot, uint32 catalystGoEntry);
    // Rolls the reward chest's loot template for the owner, empties the plot. Persists.
    ConservatoryError HarvestWildseed(uint8 plotId);

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
