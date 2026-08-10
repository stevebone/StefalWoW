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

#include "PlayerTaxi.h"
#include "ConditionMgr.h"
#include "DB2Stores.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "StringConvert.h"
#include "TaxiPackets.h"
#include <iomanip>
#include <sstream>

PlayerTaxi::PlayerTaxi() = default;
PlayerTaxi::PlayerTaxi(PlayerTaxi const& other) = default;
PlayerTaxi::PlayerTaxi(PlayerTaxi&& other) noexcept = default;
PlayerTaxi& PlayerTaxi::operator=(PlayerTaxi const& other) = default;
PlayerTaxi& PlayerTaxi::operator=(PlayerTaxi&& other) noexcept = default;
PlayerTaxi::~PlayerTaxi() = default;

void PlayerTaxi::InitTaxiNodesForLevel(uint32 race, uint32 chrClass, uint8 level)
{
    // class specific initial known nodes
    TaxiMask const& factionMask = Player::TeamForRace(race) == HORDE ? sHordeTaxiNodesMask : sAllianceTaxiNodesMask;
    switch (chrClass)
    {
        case CLASS_DEATH_KNIGHT:
        {
            for (std::size_t i = 0; i < m_taximask.size(); ++i)
                m_taximask[i] |= sOldContinentsNodesMask[i] & factionMask[i];
            break;
        }
    }

    // race specific initial known nodes: capital and taxi hub masks
    switch (race)
    {
        case RACE_HUMAN:
        case RACE_DWARF:
        case RACE_NIGHTELF:
        case RACE_GNOME:
        case RACE_DRAENEI:
        case RACE_WORGEN:
        case RACE_PANDAREN_ALLIANCE:
            SetTaximaskNode(2);     // Stormwind, Elwynn
            SetTaximaskNode(6);     // Ironforge, Dun Morogh
            SetTaximaskNode(26);    // Lor'danel, Darkshore
            SetTaximaskNode(27);    // Rut'theran Village, Teldrassil
            SetTaximaskNode(49);    // Moonglade (Alliance)
            SetTaximaskNode(94);    // The Exodar
            SetTaximaskNode(456);   // Dolanaar, Teldrassil
            SetTaximaskNode(457);   // Darnassus, Teldrassil
            SetTaximaskNode(582);   // Goldshire, Elwynn
            SetTaximaskNode(589);   // Eastvale Logging Camp, Elwynn
            SetTaximaskNode(619);   // Kharanos, Dun Morogh
            SetTaximaskNode(620);   // Gol'Bolar Quarry, Dun Morogh
            SetTaximaskNode(624);   // Azure Watch, Azuremyst Isle
            break;
        case RACE_ORC:
        case RACE_UNDEAD_PLAYER:
        case RACE_TAUREN:
        case RACE_TROLL:
        case RACE_BLOODELF:
        case RACE_GOBLIN:
        case RACE_PANDAREN_HORDE:
            SetTaximaskNode(11);    // Undercity, Tirisfal
            SetTaximaskNode(22);    // Thunder Bluff, Mulgore
            SetTaximaskNode(23);    // Orgrimmar, Durotar
            SetTaximaskNode(69);    // Moonglade (Horde)
            SetTaximaskNode(82);    // Silvermoon City
            SetTaximaskNode(384);   // The Bulwark, Tirisfal
            SetTaximaskNode(402);   // Bloodhoof Village, Mulgore
            SetTaximaskNode(460);   // Brill, Tirisfal Glades
            SetTaximaskNode(536);   // Sen'jin Village, Durotar
            SetTaximaskNode(537);   // Razor Hill, Durotar
            SetTaximaskNode(625);   // Fairbreeze Village, Eversong Woods
            SetTaximaskNode(631);   // Falconwing Square, Eversong Woods
            break;
    }

    // new continent starting masks (It will be accessible only at new map)
    switch (Player::TeamForRace(race))
    {
        case ALLIANCE: SetTaximaskNode(100); break;
        case HORDE:    SetTaximaskNode(99);  break;
        default:
            break;
    }

    // level dependent taxi hubs
    if (level >= 68)
        SetTaximaskNode(213);                               //Shattered Sun Staging Area
}

bool PlayerTaxi::LoadTaxiMask(std::string const& data)
{
    bool warn = false;
    std::vector<std::string_view> tokens = Trinity::Tokenize(data, ' ', false);
    for (size_t index = 0; (index < m_taximask.size()) && (index < tokens.size()); ++index)
    {
        if (Optional<uint32> mask = Trinity::StringTo<uint32>(tokens[index]))
        {
            // load and set bits only for existing taxi nodes
            m_taximask[index] = sTaxiNodesMask[index] & *mask;
            if (m_taximask[index] != *mask)
                warn = true;
        }
        else
        {
            m_taximask[index] = 0;
            warn = true;
        }
    }
    return !warn;
}

void PlayerTaxi::AppendTaximaskTo(WorldPackets::Taxi::ShowTaxiNodes& data, bool all)
{
    if (all)
    {
        data.CanLandNodes = sTaxiNodesMask;              // all existed nodes
        data.CanUseNodes = sTaxiNodesMask;
    }
    else
    {
        data.CanLandNodes = m_taximask;                  // known nodes - where the player may land (incl. early landing)
        data.CanUseNodes = m_taximask;                   // widened by AppendConditionUnlockedNodesTo, see PlayerTaxi.h
    }
}

bool PlayerTaxi::IsNodeUnlockedByCondition(uint32 nodeidx, Player const* player)
{
    if (!player)
        return false;

    TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(nodeidx);
    if (!node)
        return false;

    // Only nodes that actually publish an availability condition can be unlocked without discovery. Every other
    // node keeps its existing behaviour exactly - this must not widen anything Blizzard did not gate.
    if (node->ConditionID <= 0)
        return false;

    // ConditionMgr::IsPlayerMeetingCondition answers "true" for a PlayerCondition it cannot find, which is the
    // right default when a condition merely decorates something but is exactly wrong here - an unresolvable
    // condition would hand the node to everyone. Refuse to unlock what cannot be evaluated.
    if (!sPlayerConditionStore.LookupEntry(node->ConditionID))
        return false;

    if (!ConditionMgr::IsPlayerMeetingCondition(player, node->ConditionID))
        return false;

    // VisibilityConditionID hides the node on the flight map even when ConditionID would allow it, so a node that
    // fails it must not be offered either. Same fail-closed rule for an unresolvable condition.
    if (node->VisibilityConditionID)
    {
        if (!sPlayerConditionStore.LookupEntry(node->VisibilityConditionID))
            return false;

        if (!ConditionMgr::IsPlayerMeetingCondition(player, node->VisibilityConditionID))
            return false;
    }

    // A node the player's faction cannot see on the flight map must not be offered either - the discovery mask
    // implies this today (you cannot learn a node of the other faction), the condition path has to state it.
    switch (player->GetTeam())
    {
        case HORDE:    return node->GetFlags().HasFlag(TaxiNodeFlags::ShowOnHordeMap);
        case ALLIANCE: return node->GetFlags().HasFlag(TaxiNodeFlags::ShowOnAllianceMap);
        default:       return false;
    }
}

void PlayerTaxi::AppendConditionUnlockedNodesTo(TaxiMask& landNodes, TaxiMask& useNodes,
    TaxiMask const& reachableNodes, Player const* player, std::vector<TaxiConditionUnlockReport>* report)
{
    for (TaxiNodesEntry const* node : sTaxiNodesStore)
    {
        if (!node || !node->ConditionID)
            continue;

        uint32 field = uint32((node->ID - 1) / (sizeof(TaxiMask::value_type) * 8));
        TaxiMask::value_type submask = TaxiMask::value_type(1 << ((node->ID - 1) % (sizeof(TaxiMask::value_type) * 8)));

        if (field >= reachableNodes.size())
            continue;

        TaxiConditionUnlockReport entry;
        entry.NodeID = node->ID;
        entry.ConditionID = node->ConditionID;
        entry.VisibilityConditionID = node->VisibilityConditionID;
        entry.Flags = node->Flags;
        entry.InReachableMask = (reachableNodes[field] & submask) != 0;
        entry.AlreadyOffered = (useNodes[field] & submask) != 0;

        // Skip anything the flight master cannot route to anyway (this also keeps the number of condition
        // evaluations down to the handful of gated nodes that share a network with the current node) and
        // anything already offered through the discovery mask.
        if (!entry.InReachableMask || entry.AlreadyOffered)
        {
            if (report && entry.InReachableMask)
                report->push_back(entry);
            continue;
        }

        entry.ConditionPassed = IsNodeUnlockedByCondition(node->ID, player);
        if (entry.ConditionPassed)
        {
            // CanLandNodes is what the flight map iterates to place pins; CanUseNodes only decides
            // whether an already-placed pin is selectable. Setting just the latter left the node
            // undrawn, which is why a Kyrian at the Eternal Gateway saw Elysian Hold and nothing else
            // even though the server had already worked out that three more were open to him.
            landNodes[field] |= submask;
            useNodes[field] |= submask;
            entry.BitSet = (landNodes[field] & submask) != 0 && (useNodes[field] & submask) != 0;
        }

        TC_LOG_DEBUG("taxi.condition", "taxi node {} cond={} viscond={} flags=0x{:X} reachable={} known={} passed={} bitSet={}",
            entry.NodeID, entry.ConditionID, entry.VisibilityConditionID, uint32(entry.Flags),
            entry.InReachableMask, entry.AlreadyOffered, entry.ConditionPassed, entry.BitSet);

        if (report)
            report->push_back(entry);
    }

    // Hidden routing hubs.
    //
    // Some networks are not point-to-point: Bastion's covenant teleporters all run through two invisible hub
    // nodes (2627 "Ground Points Hub", 2628 "Ground Hub"), so TaxiPath carries 2625->2628, 2628->2630,
    // 2630->2627, 2627->2625 and so on, but NO 2625->2630. Argus (1985-1987), the 9.2 Resonant Peaks network
    // (2732) and the 10.0 travel network (2835/2843) are built the same way - which is exactly the set
    // TaxiNodesEntry::IsPartOfTaxiNetwork() has to whitelist by id, because they carry no
    // ShowOnAllianceMap/ShowOnHordeMap flag of their own.
    //
    // The client resolves the route itself and only sends CMSG_ACTIVATE_TAXI once it has one. With the hubs
    // absent from both masks it cannot get from the gateway to any destination, so it refuses locally with
    // "There is no direct path to that destination!" and the server never hears about the click - which is why
    // this read as a server routing bug while the server-side masks were provably correct for every
    // *destination* (qword 41 = 0x3E3: 2625/2626/2630-2634 all set, hubs 2627/2628 clear).
    //
    // Identified structurally rather than by another hardcoded id list: a node the taxi network accepts but
    // which publishes no faction map flag is by definition infrastructure, never a destination. That also means
    // setting CanLandNodes for it cannot draw a stray pin - the client places pins from those same flags.
    for (TaxiNodesEntry const* node : sTaxiNodesStore)
    {
        if (!node || !node->IsPartOfTaxiNetwork())
            continue;

        if (node->GetFlags().HasFlag(TaxiNodeFlags::ShowOnAllianceMap) || node->GetFlags().HasFlag(TaxiNodeFlags::ShowOnHordeMap))
            continue;   // a real, displayable destination - covered by the discovery mask and the pass above

        uint32 field = uint32((node->ID - 1) / (sizeof(TaxiMask::value_type) * 8));
        TaxiMask::value_type submask = TaxiMask::value_type(1 << ((node->ID - 1) % (sizeof(TaxiMask::value_type) * 8)));
        if (field >= reachableNodes.size())
            continue;

        // Only hubs on the current node's own network, so this never widens anything the flight master could
        // not route through anyway.
        if (!(reachableNodes[field] & submask))
            continue;

        landNodes[field] |= submask;
        useNodes[field] |= submask;

        TC_LOG_DEBUG("taxi.condition", "taxi hub {} flags=0x{:X} added to both masks (hidden routing infrastructure)",
            node->ID, uint32(node->Flags));
    }
}

std::string PlayerTaxi::DescribeMaskQword(TaxiMask const& mask, uint32 qwordIndex)
{
    static_assert(sizeof(TaxiMask::value_type) == 1, "the qword reassembly below assumes a byte-wide mask element");

    uint64 value = 0;
    for (uint32 i = 0; i < 8; ++i)
    {
        std::size_t byteIndex = std::size_t(qwordIndex) * 8 + i;
        if (byteIndex < mask.size())
            value |= uint64(mask[byteIndex]) << (i * 8);
    }

    std::ostringstream ss;
    ss << "qword " << qwordIndex << " (nodes " << (qwordIndex * 64 + 1) << ".." << (qwordIndex * 64 + 64) << ") = 0x"
       << std::hex << std::setw(16) << std::setfill('0') << value << std::dec << " ->";

    if (!value)
        ss << " <none>";
    else
        for (uint32 bit = 0; bit < 64; ++bit)
            if (value & (UI64LIT(1) << bit))
                ss << ' ' << (qwordIndex * 64 + bit + 1);

    return ss.str();
}

bool PlayerTaxi::LoadTaxiDestinationsFromString(const std::string& values, uint32 team)
{
    ClearTaxiDestinations();

    std::vector<std::string_view> tokens = Trinity::Tokenize(values, ' ', false);
    auto itr = tokens.begin();
    if (itr != tokens.end())
    {
        if (Optional<uint32> faction = Trinity::StringTo<uint32>(*itr))
            m_flightMasterFactionId = *faction;
        else
            return false;
    }
    else
        return false;

    while ((++itr) != tokens.end())
    {
        if (Optional<uint32> node = Trinity::StringTo<uint32>(*itr))
            AddTaxiDestination(*node);
        else
            return false;
    }

    if (m_TaxiDestinations.empty())
        return true;

    // Check integrity
    if (m_TaxiDestinations.size() < 2)
        return false;

    for (size_t i = 1; i < m_TaxiDestinations.size(); ++i)
    {
        uint32 cost;
        uint32 path;
        sObjectMgr->GetTaxiPath(m_TaxiDestinations[i - 1], m_TaxiDestinations[i], path, cost);
        if (!path)
            return false;
    }

    // can't load taxi path without mount set (quest taxi path?) - unless the source node publishes no mount at
    // all for either team (teleport-style nodes such as the covenant sanctum transport network), which is a
    // legitimately mountless path rather than broken data.
    if (!sObjectMgr->GetTaxiMountDisplayId(GetTaxiSource(), team, true))
    {
        TaxiNodesEntry const* sourceNode = sTaxiNodesStore.LookupEntry(GetTaxiSource());
        if (!sourceNode || sourceNode->MountCreatureID[0] || sourceNode->MountCreatureID[1])
            return false;
    }

    return true;
}

std::string PlayerTaxi::SaveTaxiDestinationsToString()
{
    if (m_TaxiDestinations.empty())
        return "";

    ASSERT(m_TaxiDestinations.size() >= 2);

    std::ostringstream ss;
    ss << m_flightMasterFactionId << ' ';

    for (size_t i = 0; i < m_TaxiDestinations.size(); ++i)
        ss << m_TaxiDestinations[i] << ' ';

    return ss.str();
}

void PlayerTaxi::AddTaxiDestination(uint32 dest)
{
    m_TaxiDestinations.push_back(dest);
}

uint32 PlayerTaxi::GetCurrentTaxiPath() const
{
    if (m_TaxiDestinations.size() < 2)
        return 0;

    uint32 path;
    uint32 cost;

    sObjectMgr->GetTaxiPath(m_TaxiDestinations[0], m_TaxiDestinations[1], path, cost);

    return path;
}

std::ostringstream& operator<<(std::ostringstream& ss, PlayerTaxi const& taxi)
{
    for (std::size_t i = 0; i < taxi.m_taximask.size(); ++i)
        ss << uint32(taxi.m_taximask[i]) << ' ';
    return ss;
}

bool PlayerTaxi::RequestEarlyLanding()
{
    if (m_TaxiDestinations.size() <= 2)
        return false;

    // start from first destination - m_TaxiDestinations[0] is the current starting node
    for (std::deque<uint32>::iterator it = ++m_TaxiDestinations.begin(); it != m_TaxiDestinations.end(); ++it)
    {
        if (IsTaximaskNodeKnown(*it))
        {
            if (++it == m_TaxiDestinations.end())
                return false;   // if we are left with only 1 known node on the path don't change the spline, its our final destination anyway

            m_TaxiDestinations.erase(it, m_TaxiDestinations.end());
            return true;
        }
    }

    return false;
}

FactionTemplateEntry const* PlayerTaxi::GetFlightMasterFactionTemplate() const
{
    return sFactionTemplateStore.LookupEntry(m_flightMasterFactionId);
}
