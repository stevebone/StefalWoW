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

#ifndef TRINITY_ARCHAEOLOGYMGR_H
#define TRINITY_ARCHAEOLOGYMGR_H

#include "Define.h"
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

struct ResearchProjectEntry;
struct ResearchSiteEntry;
struct ArchaeologyMgrTestAccess;

class Map;
class PhaseShift;

namespace WorldPackets::Spells
{
    struct SpellWeight;
}

struct ArchaeologySolvePlan
{
    uint32 ProjectID = 0;
    uint32 BranchID = 0;
    uint32 FragmentCurrencyID = 0;
    uint32 FragmentCount = 0;
    uint32 KeystoneItemID = 0;
    uint32 KeystoneCount = 0;
    uint32 RequiredWeight = 0;
};

// Per dig site: which research branch its fragments belong to, and how many finds exhaust it.
// The site->branch theming is not carried by client DB2, so it is server-owned reference data
// loaded from the world table `archaeology_dig_site`.
struct ArchaeologyDigSiteInfo
{
    uint32 BranchID = 0;
    uint8 FindCount = 0;
    std::vector<std::pair<float, float>> Polygon; // dig-site boundary, world X/Y vertices in order
};

// Server-side owner of Archaeology (secondary profession) research data loaded from the client
// DB2 stores. Indexes the dig-site pools per continent so active sites can be assigned to players,
// the site->branch reference data, and the dig-site boundary polygons.
class TC_GAME_API ArchaeologyMgr
{
    private:
        ArchaeologyMgr();
        ~ArchaeologyMgr();

    public:
        ArchaeologyMgr(ArchaeologyMgr const&) = delete;
        ArchaeologyMgr(ArchaeologyMgr&&) = delete;
        ArchaeologyMgr& operator=(ArchaeologyMgr const&) = delete;
        ArchaeologyMgr& operator=(ArchaeologyMgr&&) = delete;

        static ArchaeologyMgr* instance();

        // Index dig sites (ResearchSite.db2) by map. Call once at startup after DB2 stores load.
        void LoadResearchSites();

        // Load the site->branch reference data from `archaeology_dig_site`. Call after LoadResearchSites
        // (needs sResearchSiteStore) and once the world DB is available.
        void LoadDigSiteData();

        // Load server-owned branch policy (the branch-specific retail find GameObject).
        void LoadResearchBranchData();

        // Load dig-site boundary polygons from `archaeology_dig_site_point`. Call after LoadDigSiteData.
        void LoadDigSitePoints();

        // True if the world position (x, y) is inside the dig site's boundary polygon.
        bool IsInsideDigSite(uint32 researchSiteId, float x, float y) const;

        // Generate a uniformly distributed hidden-find location inside the dig-site polygon,
        // rejecting Map samples with invalid ground, liquid, or locally extreme slope.
        // Returns false if the site has no usable polygon or no sample passes rejectors.
        bool GenerateFindLocation(uint32 researchSiteId, float& x, float& y, Map* map, PhaseShift const& phaseShift) const;

        // True if the server has every policy needed to drive this site through Survey and loot.
        bool IsSurveyableDigSite(uint32 researchSiteId) const;

        // Dig-site pool for a continent/map, or nullptr if the map has none.
        std::vector<ResearchSiteEntry const*> const* GetResearchSitesForMap(uint32 mapId) const;

        // Randomly pick up to `count` distinct dig-site IDs from a map's pool (fewer if the pool is
        // smaller), excluding IDs already active for the player. Empty if the map has no dig sites.
        std::vector<uint32> RollResearchSitesForMap(uint32 mapId, uint32 count, std::vector<uint32> const& exclude = {}) const;

        // Pick one random branch-mapped dig site on a map that is not in `exclude` (used to replace an
        // exhausted site). Returns 0 if none are available.
        uint32 RollReplacementSite(uint32 mapId, std::vector<uint32> const& exclude) const;

        // Pick a random research project for a branch: provisional common/rare split +
        // prefer-uncompleted + a provisional rare roll (see PROVISIONAL_RARE_CHANCE_PERCENT).
        // Returns the ResearchProject.db2 ID, or 0 if the branch has none.
        uint32 RollResearchProject(uint32 branchId, std::unordered_set<uint32> const& completed) const;

        // The research project whose solve spell is `spellId` (the spell a player casts to complete it),
        // or nullptr if none. Called only for the archaeology solve spells the script is bound to.
        ResearchProjectEntry const* GetProjectBySpellId(uint32 spellId) const;

        // Build the immutable resource plan represented by a research-project cast. Every accepted
        // quantity and compatibility rule comes from the cast and the loaded Research/item/currency
        // DB2 rows; player ownership is checked separately immediately before consumption.
        std::optional<ArchaeologySolvePlan> BuildSolvePlan(uint32 spellId, std::vector<WorldPackets::Spells::SpellWeight> const& weights) const;

        // Branch/find-count for a dig site, or nullptr if the site has no mapping.
        ArchaeologyDigSiteInfo const* GetDigSiteInfo(uint32 researchSiteId) const;

        // Branch-specific lootable find GameObject, or 0 if the branch is not enabled.
        uint32 GetFindGameObjectId(uint32 researchBranchId) const;

        // True when the server has loaded the branch policy required to drive its complete
        // site/find/project loop.
        bool IsResearchBranchEnabled(uint32 researchBranchId) const;

    private:
        friend struct ArchaeologyMgrTestAccess;

        std::unordered_map<uint32 /*mapId*/, std::vector<ResearchSiteEntry const*>> _researchSitesByMap;
        std::unordered_map<uint32 /*researchSiteId*/, ArchaeologyDigSiteInfo> _digSiteInfo;
        std::unordered_map<uint32 /*researchBranchId*/, uint32 /*findGameObjectId*/> _findGameObjectsByBranch;
};

#define sArchaeologyMgr ArchaeologyMgr::instance()

#endif
