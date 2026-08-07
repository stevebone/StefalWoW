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

uint32 QueensConservatory::GetPlotCount() const
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

    // One wildseed plot per completed tier - see the ladder quoted in QueensConservatory.h. A talent counts as
    // completed at Rank >= 1, the same test the rest of the sanctum uses.
    uint32 unlocked = 0;
    for (GarrTalentEntry const* talent : *talents)
        if (Garrison::Talent const* owned = garrison->GetTalent(talent->ID))
            if (owned->Rank >= 1)
                ++unlocked;

    return std::min<uint32>(unlocked, CONSERVATORY_MAX_PLOTS);
}

bool QueensConservatory::IsAccessible() const
{
    return GetPlotCount() > 0;
}

bool QueensConservatory::HasCatalystPlots() const
{
    // Talent 1087 "Initial Growth" (tier 1, the second tier researched) is the one that "[g]rants you access to
    // catalyst plots", so catalysts exist from two researched tiers upwards.
    return GetPlotCount() >= 2;
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

    if (plotCount < wildseed->RequiredTier)
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

ConservatoryError QueensConservatory::AttachCatalyst(uint8 plotId, uint8 slot, uint32 catalystGoEntry)
{
    if (!_owner || _owner->GetActiveCovenant() != COVENANT_ID_NIGHT_FAE)
        return CONSERVATORY_ERROR_NOT_NIGHT_FAE;

    if (!HasCatalystPlots())
        return CONSERVATORY_ERROR_NO_CATALYST_PLOTS;

    if (slot >= CONSERVATORY_MAX_CATALYSTS)
        return CONSERVATORY_ERROR_INVALID_CATALYST;

    auto itr = _plots.find(plotId);
    if (itr == _plots.end() || itr->second.State != CONSERVATORY_PLOT_GROWING)
        return CONSERVATORY_ERROR_PLOT_EMPTY;

    // The catalysts are real GameObject templates (353652 Catalyst of Power / 353653 Catalyst of Renewal /
    // 353654 Catalyst of Might). Requiring the entry to exist keeps the column honest without pinning the set.
    if (!sObjectMgr->GetGameObjectTemplate(catalystGoEntry))
        return CONSERVATORY_ERROR_INVALID_CATALYST;

    ConservatoryPlot& plot = itr->second;
    if (plot.Catalysts[slot])
        return CONSERVATORY_ERROR_CATALYST_SLOT_TAKEN;

    plot.Catalysts[slot] = catalystGoEntry;
    MarkChanged();
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

    uint32 rewardGoEntry = CONSERVATORY_DEFAULT_REWARD_GO;
    if (ConservatoryWildseedTemplate const* wildseed = sGarrisonMgr.GetConservatoryWildseed(plot.WildseedEntry))
        if (wildseed->RewardGameObjectId)
            rewardGoEntry = wildseed->RewardGameObjectId;

    // Pay out the reward chest's own loot template (GameObject 350978 "Queen's Conservatory Cache" ->
    // gameobject_loot_template 350978), pushed straight into the bags the same way a chest's push-loot is.
    uint32 lootId = 0;
    if (GameObjectTemplate const* goTemplate = sObjectMgr->GetGameObjectTemplate(rewardGoEntry))
        lootId = goTemplate->GetLootId();

    if (!lootId)
    {
        TC_LOG_ERROR("garrison", "QueensConservatory: reward GameObject {} for wildseed {} has no loot template; "
            "player {} harvested nothing.", rewardGoEntry, plot.WildseedEntry, _owner->GetGUID().ToString());
        return CONSERVATORY_ERROR_UNKNOWN_WILDSEED;
    }

    Map* map = _owner->GetMap();
    Loot harvestLoot(map, _owner->GetGUID(), LOOT_CHEST, nullptr);
    harvestLoot.FillLoot(lootId, LootTemplates_Gameobject, _owner, true, false, LOOT_MODE_DEFAULT,
        ItemBonusMgr::GetContextForPlayer(map ? map->GetMapDifficulty() : nullptr, _owner));
    harvestLoot.AutoStore(_owner, NULL_BAG, NULL_SLOT, true);

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
