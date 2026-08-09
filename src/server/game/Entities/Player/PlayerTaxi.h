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

#ifndef PlayerTaxi_h__
#define PlayerTaxi_h__

#include "DBCEnums.h"
#include "Define.h"
#include <deque>
#include <iosfwd>
#include <string>
#include <vector>

class Player;
struct FactionTemplateEntry;
namespace WorldPackets
{
    namespace Taxi
    {
        class ShowTaxiNodes;
    }
}

// One line of "why is (or isn't) this node on the flight map", filled in by
// PlayerTaxi::AppendConditionUnlockedNodesTo when a caller asks for it. Diagnostics only - the
// unlock decision itself does not read this back. `.debug taxinodes` prints it, and the same
// values are written to the (off by default) "taxi.condition" logger on every SendTaxiMenu.
struct TaxiConditionUnlockReport
{
    uint32 NodeID = 0;
    int32 ConditionID = 0;
    uint32 VisibilityConditionID = 0;
    int32 Flags = 0;
    bool InReachableMask = false;   // TaxiPathGraph could route to it from the current node
    bool AlreadyOffered = false;    // the discovery mask already had it, so it was left alone
    bool ConditionPassed = false;   // IsNodeUnlockedByCondition
    bool BitSet = false;            // the bit ended up set in BOTH outgoing masks
};

class TC_GAME_API PlayerTaxi
{
    public:
        PlayerTaxi();
        PlayerTaxi(PlayerTaxi const& other);
        PlayerTaxi(PlayerTaxi&& other) noexcept;
        PlayerTaxi& operator=(PlayerTaxi const& other);
        PlayerTaxi& operator=(PlayerTaxi&& other) noexcept;
        ~PlayerTaxi();

        // Nodes
        void InitTaxiNodesForLevel(uint32 race, uint32 chrClass, uint8 level);
        bool LoadTaxiMask(std::string const& data);

        bool IsTaximaskNodeKnown(uint32 nodeidx) const
        {
            uint32 field = uint32((nodeidx - 1) / (sizeof(TaxiMask::value_type) * 8));
            TaxiMask::value_type submask = TaxiMask::value_type(1 << ((nodeidx - 1) % (sizeof(TaxiMask::value_type) * 8)));
            return (m_taximask[field] & submask) != 0;
        }
        bool SetTaximaskNode(uint32 nodeidx)
        {
            uint32 field = uint32((nodeidx - 1) / (sizeof(TaxiMask::value_type) * 8));
            TaxiMask::value_type submask = TaxiMask::value_type(1 << ((nodeidx - 1) % (sizeof(TaxiMask::value_type) * 8)));
            if ((m_taximask[field] & submask) == 0)
            {
                m_taximask[field] |= submask;
                return true;
            }
            else
                return false;
        }
        void AppendTaximaskTo(WorldPackets::Taxi::ShowTaxiNodes& data, bool all);
        TaxiMask const& GetTaxiMask() const { return m_taximask; }

        // TaxiNodes.ConditionID gates a node's *availability* independently of the discovery mask: the covenant
        // sanctum transport network nodes are offered the moment the matching GarrTalent is researched, without
        // ever having been visited. This is what makes retail's CanUseNodes a strict superset of CanLandNodes.
        // Purely additive - a node without a ConditionID is never unlocked this way, and no node is ever removed.
        static bool IsNodeUnlockedByCondition(uint32 nodeidx, Player const* player);
        // Sets, in BOTH masks, every condition-unlocked node that is also set in `reachableNodes`.
        // Both, because the client draws its flight-map pins from CanLandNodes and only ever uses
        // CanUseNodes to grey out a pin it already has: a node present in CanUseNodes alone is drawn
        // nowhere, so widening that mask on its own is invisible in game.
        static void AppendConditionUnlockedNodesTo(TaxiMask& landNodes, TaxiMask& useNodes,
            TaxiMask const& reachableNodes, Player const* player,
            std::vector<TaxiConditionUnlockReport>* report = nullptr);

        // Diagnostics: render one 64-bit block of an outgoing taxi mask exactly as it goes on the wire -
        // the raw qword plus the TaxiNodes IDs whose bits are set in it. The client reads these masks in
        // uint64 blocks, so a qword index is the unit an argument about "did the bit actually ship" is
        // conducted in. Node ID N lives at bit (N-1), i.e. qword (N-1)/64, bit (N-1)%64.
        static std::string DescribeMaskQword(TaxiMask const& mask, uint32 qwordIndex);
        static uint32 QwordIndexForNode(uint32 nodeidx) { return (nodeidx - 1) / 64; }

        // Destinations
        [[nodiscard]] bool LoadTaxiDestinationsFromString(std::string const& values, uint32 team);
        std::string SaveTaxiDestinationsToString();

        void ClearTaxiDestinations() { m_TaxiDestinations.clear(); }
        void AddTaxiDestination(uint32 dest);
        uint32 GetTaxiSource() const { return m_TaxiDestinations.empty() ? 0 : m_TaxiDestinations.front(); }
        uint32 GetTaxiDestination() const { return m_TaxiDestinations.size() < 2 ? 0 : m_TaxiDestinations[1]; }
        uint32 GetCurrentTaxiPath() const;
        uint32 NextTaxiDestination()
        {
            m_TaxiDestinations.pop_front();
            return GetTaxiDestination();
        }
        bool RequestEarlyLanding();
        std::deque<uint32> const& GetPath() const { return m_TaxiDestinations; }
        bool empty() const { return m_TaxiDestinations.empty(); }
        FactionTemplateEntry const* GetFlightMasterFactionTemplate() const;
        void SetFlightMasterFactionTemplateId(uint32 factionTemplateId) { m_flightMasterFactionId = factionTemplateId; }

        friend std::ostringstream& operator<<(std::ostringstream& ss, PlayerTaxi const& taxi);
    private:
        TaxiMask m_taximask;
        std::deque<uint32> m_TaxiDestinations;
        uint32 m_flightMasterFactionId = 0;
};

std::ostringstream& operator <<(std::ostringstream& ss, PlayerTaxi const& taxi);

#endif // PlayerTaxi_h__
