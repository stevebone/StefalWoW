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

#include "QueensConservatory.h"
#include "DB2Stores.h"
#include "DB2Structure.h"
#include "DatabaseEnv.h"
#include "GameObject.h"
#include "GameTime.h"
#include "Garrison.h"
#include "GarrisonMgr.h"
#include "ItemBonusMgr.h"
#include "Log.h"
#include "Loot.h"
#include "LootMgr.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include <algorithm>
#include <limits>

uint32 ConservatoryPlot::CountCatalysts() const
{
    return uint32(std::count_if(Catalysts.begin(), Catalysts.end(), [](uint32 c) { return c != 0; }));
}

uint32 ConservatoryPlot::CountCatalyst(uint32 catalystItemId) const
{
    if (!catalystItemId)
        return 0;

    return uint32(std::count(Catalysts.begin(), Catalysts.end(), catalystItemId));
}

// Catalyst links per pod. [WEB, Wowhead "Night Fae Covenant Queen's Conservatory"] a fully upgraded
// Conservatory has "1 pod with 1 catalyst link, 3 pods with 2, 1 pod with 3 and 1 pod with 4", which lines
// up with the talents' own wording: 1089 "Flourishing Beds" is where "[y]ou can now use the wildseed that
// has three possible catalyst connections" and 1090 "Final Forms" adds the pod that "can benefit from four
// possible catalyst links". Pods are handed out in that order, so the caps are positional.
static constexpr std::array<uint8, CONSERVATORY_MAX_PLOTS> CatalystLinkCap = { 1, 2, 2, 2, 3, 4 };

uint8 QueensConservatory::GetCatalystLinkCap(uint8 plotId)
{
    return plotId < CatalystLinkCap.size() ? CatalystLinkCap[plotId] : uint8(0);
}

QueensConservatory::QueensConservatory(Player* owner) : _owner(owner)
{
}

uint32 QueensConservatory::GetConservatoryTreeId() const
{
    if (!_owner || _owner->GetActiveCovenant() != COVENANT_ID_NIGHT_FAE)
        return 0;

    // The unique-feature tree of the owner's covenant: GarrTypeID 111 + FeatureTypeIndex 5 (SanctumUnique) +
    // FeatureSubtypeIndex == CovenantID. For Night Fae that resolves to tree 319 "The Queen's Conservatory".
    if (std::vector<GarrTalentTreeEntry const*> const* trees = sGarrisonMgr.GetTalentTreesForGarrType(GARRISON_TYPE_COVENANT))
        for (GarrTalentTreeEntry const* tree : *trees)
            if (tree->FeatureTypeIndex == GARR_TALENT_FEATURE_UNIQUE && tree->FeatureSubtypeIndex == COVENANT_ID_NIGHT_FAE)
                return tree->ID;

    return 0;
}

uint32 QueensConservatory::GetResearchedTiers() const
{
    uint32 const treeId = GetConservatoryTreeId();
    if (!treeId)
        return 0;

    Garrison const* garrison = _owner->GetGarrison(GARRISON_TYPE_COVENANT);
    if (!garrison)
        return 0;

    std::vector<GarrTalentEntry const*> const* talents = sGarrisonMgr.GetTalentsForTree(treeId);
    if (!talents)
        return 0;

    // A talent counts as completed at Rank >= 1, the same test the rest of the sanctum uses.
    uint32 unlocked = 0;
    for (GarrTalentEntry const* talent : *talents)
        if (Garrison::Talent const* owned = garrison->GetTalent(talent->ID))
            if (owned->Rank >= 1)
                ++unlocked;

    return std::min<uint32>(unlocked, CONSERVATORY_MAX_TIERS);
}

uint32 QueensConservatory::GetPlotCount() const
{
    // tiers + 1, not tiers: "First Planting" restores the Conservatory and tier 1 "Initial Growth" already
    // "activates an additional wildseed", so a tier-1 Conservatory has two pods and a fully researched one
    // has six. Returning tiers capped at 5 - what this used to do - made pod 5, the only one with four
    // catalyst links, unreachable no matter how much was researched.
    uint32 const tiers = GetResearchedTiers();
    if (!tiers)
        return 0;

    return std::min<uint32>(tiers + 1, CONSERVATORY_MAX_PLOTS);
}

bool QueensConservatory::IsAccessible() const
{
    return GetPlotCount() > 0;
}

bool QueensConservatory::HasCatalystPlots() const
{
    // Talent 1087 "Initial Growth" (the second tier researched) is the one that "[g]rants you access to
    // catalyst plots", so catalysts exist from two researched TIERS upwards - expressed in tiers rather than
    // in pods so that the tiers+1 pod ladder cannot shift this gate by accident.
    return GetResearchedTiers() >= CONSERVATORY_CATALYST_PLOTS_TIER;
}

void QueensConservatory::GetCatalystCounts(ConservatoryPlot const& plot, uint8& rootGrainCount, uint8& nightbloomCount) const
{
    rootGrainCount = 0;
    nightbloomCount = 0;

    for (uint32 catalystItemId : plot.Catalysts)
    {
        if (!catalystItemId)
            continue;

        ConservatoryCatalystTemplate const* catalyst = sGarrisonMgr.GetConservatoryCatalyst(catalystItemId);
        if (!catalyst)
            continue;   // reported where it matters (load time, and ResolveHarvestLootId)

        switch (catalyst->EffectType)
        {
            case CONSERVATORY_CATALYST_EFFECT_YIELD_QUALITY:  ++rootGrainCount;  break;
            case CONSERVATORY_CATALYST_EFFECT_YIELD_QUANTITY: ++nightbloomCount; break;
            default: break;   // TIME_DELTA has already been applied to MaturesAt; it does not key the yield
        }
    }
}

ConservatoryPlot const* QueensConservatory::GetPlot(uint8 plotId) const
{
    auto itr = _plots.find(plotId);
    return itr != _plots.end() ? &itr->second : nullptr;
}

std::vector<ConservatoryPlot const*> QueensConservatory::GetPlots() const
{
    std::vector<ConservatoryPlot const*> plots;
    plots.reserve(_plots.size());
    for (auto const& [plotId, plot] : _plots)
        plots.push_back(&plot);

    std::sort(plots.begin(), plots.end(), [](ConservatoryPlot const* l, ConservatoryPlot const* r) { return l->PlotId < r->PlotId; });
    return plots;
}

void QueensConservatory::GetGardenData(uint32& active, uint32& ready, int64& remainingSeconds) const
{
    active = 0;
    ready = 0;
    remainingSeconds = 0;

    time_t const now = GameTime::GetGameTime();
    time_t soonest = 0;

    for (auto const& [plotId, plot] : _plots)
    {
        switch (plot.State)
        {
            case CONSERVATORY_PLOT_GROWING:
                ++active;
                if (!soonest || plot.MaturesAt < soonest)
                    soonest = plot.MaturesAt;
                break;
            case CONSERVATORY_PLOT_READY:
                ++ready;
                break;
            default:
                break;
        }
    }

    if (soonest > now)
        remainingSeconds = int64(soonest - now);
}

void QueensConservatory::RefreshClientState()
{
    // LoadFromDB runs this through Update() while the character is still being assembled; casting then would
    // be both unsafe and pointless. The first in-world tick of Garrison::Update puts the auras up.
    if (!_owner || !_owner->IsInWorld())
        return;

    uint32 active = 0;
    uint32 ready = 0;
    int64 remainingSeconds = 0;
    GetGardenData(active, ready, remainingSeconds);

    // Counters aura. The client reads Points[0]/Points[1] off it, so the two counts are pushed as the first two
    // effect base points. Re-cast rather than poke the live effects so a changed count always reaches the client.
    if (SpellInfo const* countsInfo = sSpellMgr->GetSpellInfo(SPELL_CONSERVATORY_GARDEN_COUNTS, DIFFICULTY_NONE))
    {
        _owner->RemoveAurasDueToSpell(SPELL_CONSERVATORY_GARDEN_COUNTS);
        if (active || ready)
        {
            CastSpellExtraArgs args(TRIGGERED_FULL_MASK);
            args.SetTriggeringSpell(nullptr);
            args.AddSpellMod(SPELLVALUE_BASE_POINT0, int32(active));
            if (countsInfo->GetEffects().size() > 1)
                args.AddSpellMod(SPELLVALUE_BASE_POINT1, int32(ready));
            _owner->CastSpell(_owner, SPELL_CONSERVATORY_GARDEN_COUNTS, args);
        }
    }
    else
        TC_LOG_DEBUG("garrison", "QueensConservatory: spell {} is not in the spell store; the landing-page "
            "garden counters cannot be pushed.", uint32(SPELL_CONSERVATORY_GARDEN_COUNTS));

    // Countdown aura. Its remaining duration IS the number the client shows, so it is stamped explicitly.
    if (sSpellMgr->GetSpellInfo(SPELL_CONSERVATORY_GARDEN_TIMER, DIFFICULTY_NONE))
    {
        _owner->RemoveAurasDueToSpell(SPELL_CONSERVATORY_GARDEN_TIMER);
        if (remainingSeconds > 0)
        {
            _owner->CastSpell(_owner, SPELL_CONSERVATORY_GARDEN_TIMER, TRIGGERED_FULL_MASK);
            if (Aura* timer = _owner->GetAura(SPELL_CONSERVATORY_GARDEN_TIMER))
            {
                int32 const durationMs = int32(std::min<int64>(remainingSeconds, std::numeric_limits<int32>::max() / 1000) * 1000);
                timer->SetMaxDuration(durationMs);
                timer->SetDuration(durationMs);
            }
        }
    }
    else
        TC_LOG_DEBUG("garrison", "QueensConservatory: spell {} is not in the spell store; the landing-page "
            "garden countdown cannot be pushed.", uint32(SPELL_CONSERVATORY_GARDEN_TIMER));
}

void QueensConservatory::Update()
{
    if (!_owner)
        return;

    // Every action method is gated on Night Fae; the tick has to be too. The plots of a character who planted and
    // then switched covenant still load, so without this guard RefreshClientState() would keep stamping the garden
    // auras 344292/344304 - the exact two C_ArdenwealdGardening.GetGardenData() reads - onto, say, a Necrolord,
    // who would see a live wildseed countdown for a garden they can neither plant in nor harvest from. Take the
    // auras down on the way out rather than merely stopping the refresh, so a switch clears the panel instead of
    // freezing yesterday's numbers on it. The plot rows themselves are untouched and return with the covenant.
    if (_owner->GetActiveCovenant() != COVENANT_ID_NIGHT_FAE)
    {
        if (_owner->HasAura(SPELL_CONSERVATORY_GARDEN_COUNTS))
            _owner->RemoveAurasDueToSpell(SPELL_CONSERVATORY_GARDEN_COUNTS);
        if (_owner->HasAura(SPELL_CONSERVATORY_GARDEN_TIMER))
            _owner->RemoveAurasDueToSpell(SPELL_CONSERVATORY_GARDEN_TIMER);
        return;
    }

    time_t const now = GameTime::GetGameTime();
    bool matured = false;

    for (auto& [plotId, plot] : _plots)
    {
        if (plot.State != CONSERVATORY_PLOT_GROWING)
            continue;

        if (plot.MaturesAt && plot.MaturesAt <= now)
        {
            plot.State = CONSERVATORY_PLOT_READY;
            MarkChanged();
            matured = true;
            TC_LOG_DEBUG("garrison", "QueensConservatory: player {} wildseed on plot {} matured.",
                _owner->GetGUID().ToString(), uint32(plotId));
        }
    }

    // The countdown aura has to be restamped while anything is still growing (the client shows the time to the
    // *next* maturation), and once more on the tick a plot flips. An all-empty garden is left alone so the tick
    // does not churn auras for every covenant character that has never planted.
    bool const occupied = std::any_of(_plots.begin(), _plots.end(),
        [](std::pair<uint8 const, ConservatoryPlot> const& entry) { return entry.second.IsOccupied(); });
    if (matured || occupied)
        RefreshClientState();
}

ConservatoryError QueensConservatory::PlantWildseed(uint8 plotId, uint32 wildseedEntry)
{
    if (!_owner || _owner->GetActiveCovenant() != COVENANT_ID_NIGHT_FAE)
        return CONSERVATORY_ERROR_NOT_NIGHT_FAE;

    uint32 const plotCount = GetPlotCount();
    if (!plotCount)
        return CONSERVATORY_ERROR_NOT_UNLOCKED;

    if (plotId >= plotCount)
        return CONSERVATORY_ERROR_INVALID_PLOT;

    ConservatoryPlot& plot = _plots[plotId];
    plot.PlotId = plotId;
    if (plot.IsOccupied())
        return CONSERVATORY_ERROR_PLOT_OCCUPIED;

    // Every number that decides what a plant costs and how long it takes is content, not client data. With no
    // rows authored there is nothing truthful to charge or to time, so the plant is refused rather than run on
    // invented values. See the "NOT DERIVABLE" note in QueensConservatory.h.
    if (sGarrisonMgr.GetConservatoryWildseeds().empty())
        return CONSERVATORY_ERROR_NO_WILDSEED_DATA;

    ConservatoryWildseedTemplate const* wildseed = sGarrisonMgr.GetConservatoryWildseed(wildseedEntry);
    if (!wildseed)
        return CONSERVATORY_ERROR_UNKNOWN_WILDSEED;

    // requiredTier is in RESEARCHED TIERS, not in pods - the two stopped being the same number when the pod
    // ladder became tiers + 1.
    if (GetResearchedTiers() < wildseed->RequiredTier)
        return CONSERVATORY_ERROR_TIER_TOO_LOW;

    // A wildseed with no maturation time would complete the instant it is planted, which is not a loop. Treat a
    // missing duration as unauthored data rather than as "instant".
    if (!wildseed->MaturationSeconds)
        return CONSERVATORY_ERROR_NO_WILDSEED_DATA;

    if (wildseed->CostCurrencyId && wildseed->CostCurrencyCount
        && !_owner->HasCurrency(wildseed->CostCurrencyId, wildseed->CostCurrencyCount))
        return CONSERVATORY_ERROR_CANT_AFFORD;

    if (wildseed->CostItemId && wildseed->CostItemCount
        && !_owner->HasItemCount(wildseed->CostItemId, wildseed->CostItemCount))
        return CONSERVATORY_ERROR_CANT_AFFORD;

    // Charge only once the request is known to be servable.
    if (wildseed->CostCurrencyId && wildseed->CostCurrencyCount)
        _owner->RemoveCurrency(wildseed->CostCurrencyId, int32(wildseed->CostCurrencyCount), CurrencyDestroyReason::Garrison);

    if (wildseed->CostItemId && wildseed->CostItemCount)
        _owner->DestroyItemCount(wildseed->CostItemId, wildseed->CostItemCount, true);

    time_t const now = GameTime::GetGameTime();
    plot.WildseedEntry = wildseedEntry;
    plot.PlantedTime = now;
    plot.MaturesAt = now + time_t(wildseed->MaturationSeconds);
    plot.Catalysts.fill(0);
    plot.State = CONSERVATORY_PLOT_GROWING;
    MarkChanged();
    RefreshClientState();

    TC_LOG_DEBUG("garrison", "QueensConservatory: player {} planted wildseed {} on plot {}, matures at {}.",
        _owner->GetGUID().ToString(), wildseedEntry, uint32(plotId), int64(plot.MaturesAt));

    return CONSERVATORY_OK;
}

ConservatoryError QueensConservatory::AttachCatalyst(uint8 plotId, uint32 catalystItemId)
{
    if (!_owner || _owner->GetActiveCovenant() != COVENANT_ID_NIGHT_FAE)
        return CONSERVATORY_ERROR_NOT_NIGHT_FAE;

    if (!HasCatalystPlots())
        return CONSERVATORY_ERROR_NO_CATALYST_PLOTS;

    auto itr = _plots.find(plotId);
    if (itr == _plots.end() || itr->second.State != CONSERVATORY_PLOT_GROWING)
        return CONSERVATORY_ERROR_PLOT_EMPTY;

    ConservatoryPlot& plot = itr->second;

    // A catalyst is an ITEM (176921 Temporal Leaves / 176922 Wild Nightbloom / 176832 Wildseed Root Grain),
    // and which items those are is content. With nothing authored there is no truthful effect to apply and
    // nothing honest to consume, so the link is refused rather than recorded as a decoration.
    if (sGarrisonMgr.GetConservatoryCatalysts().empty())
        return CONSERVATORY_ERROR_NO_CATALYST_DATA;

    ConservatoryCatalystTemplate const* catalyst = sGarrisonMgr.GetConservatoryCatalyst(catalystItemId);
    if (!catalyst)
        return CONSERVATORY_ERROR_INVALID_CATALYST;

    // Structural cap of this particular pod ({1,2,2,2,3,4}), then the catalyst's own maxPerPlot.
    uint8 const linkCap = GetCatalystLinkCap(plotId);
    if (!linkCap || plot.CountCatalysts() >= linkCap)
        return CONSERVATORY_ERROR_CATALYST_SLOT_TAKEN;

    if (catalyst->MaxPerPlot && plot.CountCatalyst(catalystItemId) >= catalyst->MaxPerPlot)
        return CONSERVATORY_ERROR_CATALYST_LIMIT;

    auto freeSlot = std::find(plot.Catalysts.begin(), plot.Catalysts.end(), 0u);
    if (freeSlot == plot.Catalysts.end())
        return CONSERVATORY_ERROR_CATALYST_SLOT_TAKEN;

    if (!_owner->HasItemCount(catalystItemId, 1))
        return CONSERVATORY_ERROR_NO_CATALYST_ITEM;

    // Refuse now, while the item is still in the player's bags, any combination that has no authored payout.
    // As shipped, `garrison_conservatory_yield` covers all fourteen combinations four links can reach, so
    // this cannot fire; it is the guard that keeps a later edit of that table from stranding a growing pod
    // in a state HarvestWildseed would have to refuse.
    if (!sGarrisonMgr.GetConservatoryYields().empty())
    {
        ConservatoryPlot prospective = plot;
        prospective.Catalysts[std::distance(plot.Catalysts.begin(), freeSlot)] = catalystItemId;

        uint8 rootGrainCount = 0;
        uint8 nightbloomCount = 0;
        GetCatalystCounts(prospective, rootGrainCount, nightbloomCount);

        if (!sGarrisonMgr.GetConservatoryYieldLootId(plot.WildseedEntry, rootGrainCount, nightbloomCount))
            return CONSERVATORY_ERROR_NO_YIELD_FOR_COMBINATION;
    }

    // Every check has passed - only now is anything taken or written.
    _owner->DestroyItemCount(catalystItemId, 1, true);
    *freeSlot = catalystItemId;

    // "These enchanted leaves reduce the Wildseed of Regrowth process by 1 day." A TIME_DELTA catalyst has to
    // move the clock or attaching it is exactly the no-op this change exists to remove. Never rewind past
    // now: a pod that the deltas have already carried to term simply becomes harvestable.
    if (catalyst->EffectType == CONSERVATORY_CATALYST_EFFECT_TIME_DELTA && catalyst->EffectValue && plot.MaturesAt)
    {
        time_t const now = GameTime::GetGameTime();
        plot.MaturesAt = std::max<time_t>(now, plot.MaturesAt + time_t(catalyst->EffectValue));
        if (plot.MaturesAt <= now)
            plot.State = CONSERVATORY_PLOT_READY;
    }

    MarkChanged();
    RefreshClientState();

    TC_LOG_DEBUG("garrison", "QueensConservatory: player {} linked catalyst item {} to plot {} ({}/{} links, "
        "matures at {}).", _owner->GetGUID().ToString(), catalystItemId, uint32(plotId), plot.CountCatalysts(),
        uint32(linkCap), int64(plot.MaturesAt));

    return CONSERVATORY_OK;
}

ConservatoryError QueensConservatory::ResolveHarvestLootId(ConservatoryPlot const& plot, uint32& lootId) const
{
    lootId = 0;

    // With a yield table authored, the catalysts linked to this pod pick the loot table - that is the whole
    // point of attaching them. `garrison_conservatory_yield` prefers a spirit-specific row and falls back to
    // the spiritItemId 0 wildcard.
    if (!sGarrisonMgr.GetConservatoryYields().empty())
    {
        uint8 rootGrainCount = 0;
        uint8 nightbloomCount = 0;
        GetCatalystCounts(plot, rootGrainCount, nightbloomCount);

        lootId = sGarrisonMgr.GetConservatoryYieldLootId(plot.WildseedEntry, rootGrainCount, nightbloomCount);
        if (!lootId)
        {
            TC_LOG_ERROR("garrison", "QueensConservatory: no `garrison_conservatory_yield` row for wildseed {} "
                "with {} quality and {} quantity catalyst(s); refusing to roll a table that does not describe "
                "this harvest.", plot.WildseedEntry, uint32(rootGrainCount), uint32(nightbloomCount));
            return CONSERVATORY_ERROR_NO_YIELD_FOR_COMBINATION;
        }
    }
    else
    {
        // No yield data: fall back to the wildseed's reward chest, i.e. exactly the pre-catalyst behaviour.
        // Reachable only with zero catalysts attached, because AttachCatalyst refuses without catalyst data.
        uint32 rewardGoEntry = CONSERVATORY_DEFAULT_REWARD_GO;
        if (ConservatoryWildseedTemplate const* wildseed = sGarrisonMgr.GetConservatoryWildseed(plot.WildseedEntry))
            if (wildseed->RewardGameObjectId)
                rewardGoEntry = wildseed->RewardGameObjectId;

        if (GameObjectTemplate const* goTemplate = sObjectMgr->GetGameObjectTemplate(rewardGoEntry))
            lootId = goTemplate->GetLootId();

        if (!lootId)
        {
            TC_LOG_ERROR("garrison", "QueensConservatory: reward GameObject {} for wildseed {} has no loot "
                "template.", rewardGoEntry, plot.WildseedEntry);
            return CONSERVATORY_ERROR_NO_LOOT_TEMPLATE;
        }
    }

    // A loot id that has no rows would make FillLoot a no-op and AutoStore hand over nothing, which is the
    // silent success this whole change exists to remove. Refuse instead.
    if (!LootTemplates_Gameobject.HaveLootFor(lootId))
    {
        TC_LOG_ERROR("garrison", "QueensConservatory: gameobject_loot_template {} has no rows; refusing to "
            "harvest wildseed {} rather than pay out nothing.", lootId, plot.WildseedEntry);
        lootId = 0;
        return CONSERVATORY_ERROR_NO_LOOT_TEMPLATE;
    }

    return CONSERVATORY_OK;
}

ConservatoryError QueensConservatory::HarvestWildseed(uint8 plotId)
{
    if (!_owner || _owner->GetActiveCovenant() != COVENANT_ID_NIGHT_FAE)
        return CONSERVATORY_ERROR_NOT_NIGHT_FAE;

    auto itr = _plots.find(plotId);
    if (itr == _plots.end() || !itr->second.IsOccupied())
        return CONSERVATORY_ERROR_PLOT_EMPTY;

    ConservatoryPlot& plot = itr->second;

    // Catch a plot whose timer elapsed since the last tick so a claim never fails on a rounding edge.
    if (plot.State == CONSERVATORY_PLOT_GROWING && plot.MaturesAt && plot.MaturesAt <= GameTime::GetGameTime())
        plot.State = CONSERVATORY_PLOT_READY;

    if (plot.State != CONSERVATORY_PLOT_READY)
        return CONSERVATORY_ERROR_NOT_READY;

    // Resolve the payout BEFORE the plot is cleared, so a refusal leaves the wildseed exactly where it was
    // and the player can claim it once the data is fixed.
    uint32 lootId = 0;
    if (ConservatoryError error = ResolveHarvestLootId(plot, lootId))
        return error;

    Map* map = _owner->GetMap();
    ItemContext const context = ItemBonusMgr::GetContextForPlayer(map ? map->GetMapDifficulty() : nullptr, _owner);
    Loot harvestLoot(map, _owner->GetGUID(), LOOT_CHEST, nullptr);
    if (!harvestLoot.FillLoot(lootId, LootTemplates_Gameobject, _owner, true, false, LOOT_MODE_DEFAULT, context))
    {
        TC_LOG_ERROR("garrison", "QueensConservatory: gameobject_loot_template {} produced nothing for player {}; "
            "the wildseed on plot {} is left planted rather than consumed for no reward.",
            lootId, _owner->GetGUID().ToString(), uint32(plotId));
        return CONSERVATORY_ERROR_NO_LOOT_TEMPLATE;
    }

    // A full bag must not swallow the payout: whatever could not be stored goes out by Postmaster mail, so
    // the plot is only ever emptied against rewards the player actually received.
    if (!harvestLoot.AutoStore(_owner, NULL_BAG, NULL_SLOT, true))
        for (LootItem const& item : harvestLoot.items)
            if (!item.is_looted && item.type == LootItemType::Item && item.itemid)
                _owner->SendItemRetrievalMail(item.itemid, item.count, item.context);

    plot.WildseedEntry = 0;
    plot.PlantedTime = 0;
    plot.MaturesAt = 0;
    plot.Catalysts.fill(0);
    plot.State = CONSERVATORY_PLOT_EMPTY;
    MarkChanged();
    RefreshClientState();

    TC_LOG_DEBUG("garrison", "QueensConservatory: player {} harvested plot {} (loot {}).",
        _owner->GetGUID().ToString(), uint32(plotId), lootId);

    return CONSERVATORY_OK;
}

void QueensConservatory::LoadFromDB(PreparedQueryResult result)
{
    _plots.clear();
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        ConservatoryPlot plot;
        plot.PlotId        = fields[0].GetUInt8();
        plot.WildseedEntry = fields[1].GetUInt32();
        plot.PlantedTime   = fields[2].GetInt64();
        plot.MaturesAt     = fields[3].GetInt64();
        plot.Catalysts[0]  = fields[4].GetUInt32();
        plot.Catalysts[1]  = fields[5].GetUInt32();
        plot.Catalysts[2]  = fields[6].GetUInt32();
        plot.Catalysts[3]  = fields[7].GetUInt32();
        plot.State         = ConservatoryPlotState(fields[8].GetUInt8());

        if (plot.PlotId >= CONSERVATORY_MAX_PLOTS)
        {
            TC_LOG_ERROR("garrison", "QueensConservatory: dropping out-of-range plot {} for player {}.",
                uint32(plot.PlotId), _owner->GetGUID().ToString());
            continue;
        }

        if (plot.State > CONSERVATORY_PLOT_READY)
            plot.State = CONSERVATORY_PLOT_EMPTY;

        // catalyst1..4 hold `garrison_conservatory_catalyst`.catalystItemId. Rows written by the first
        // revision of this feature hold GameObject entries instead (353652/353653/353654), and a catalyst can
        // also be retired from the table under a pod that is still growing. Either way the value no longer
        // means anything, so it is dropped loudly rather than counted towards a yield it does not describe.
        // Only compacts once the catalyst table is loaded; an empty table is left alone so a world DB that
        // has not had 2026_08_07_63 applied does not lose player state.
        if (!sGarrisonMgr.GetConservatoryCatalysts().empty())
        {
            for (uint32& catalystItemId : plot.Catalysts)
            {
                if (!catalystItemId || sGarrisonMgr.GetConservatoryCatalyst(catalystItemId))
                    continue;

                TC_LOG_ERROR("garrison", "QueensConservatory: player {} plot {} references catalyst {}, which is "
                    "not in `garrison_conservatory_catalyst`; the link is dropped.",
                    _owner->GetGUID().ToString(), uint32(plot.PlotId), catalystItemId);
                catalystItemId = 0;
            }

            // Keep the live links packed at the front so the link cap is a simple count.
            auto end = std::stable_partition(plot.Catalysts.begin(), plot.Catalysts.end(),
                [](uint32 c) { return c != 0; });
            std::fill(end, plot.Catalysts.end(), 0u);
        }

        _plots[plot.PlotId] = plot;
    } while (result->NextRow());

    // A wildseed that finished while the character was offline comes back ready, not still growing.
    Update();
}

void QueensConservatory::SaveToDB(CharacterDatabaseTransaction trans) const
{
    // Nothing has changed since the last save and nothing was ever planted - skip the delete/insert churn.
    if (!_needsSave && _plots.empty())
        return;

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON_CONSERVATORY);
    stmt->setUInt64(0, _owner->GetGUID().GetCounter());
    trans->Append(stmt);

    for (auto const& [plotId, plot] : _plots)
    {
        if (!plot.IsOccupied())
            continue;

        uint8 index = 0;
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_GARRISON_CONSERVATORY);
        stmt->setUInt64(index++, _owner->GetGUID().GetCounter());
        stmt->setUInt8(index++, plot.PlotId);
        stmt->setUInt32(index++, plot.WildseedEntry);
        stmt->setInt64(index++, plot.PlantedTime);
        stmt->setInt64(index++, plot.MaturesAt);
        stmt->setUInt32(index++, plot.Catalysts[0]);
        stmt->setUInt32(index++, plot.Catalysts[1]);
        stmt->setUInt32(index++, plot.Catalysts[2]);
        stmt->setUInt32(index++, plot.Catalysts[3]);
        stmt->setUInt8(index++, uint8(plot.State));
        trans->Append(stmt);
    }
}
