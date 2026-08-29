/*
 * DoomCore - Delve System shared data
 * Data-driven table of per-delve info keyed by GossipMenuID.
 * NPC entry 212407 is shared across all delves; routing is by gossip_menu_addon.
 *
 * All values sourced from 12.0.1.66527 packet sniffs + DB2 CSVs.
 */

#ifndef DELVE_DATA_H
#define DELVE_DATA_H

#include "Define.h"
#include "ObjectGuid.h"
#include <unordered_map>

// Tier spells are universal across all delves (from sniff)
static constexpr uint32 MAX_DELVE_TIERS = 11;

static constexpr int32 TIER_SPELL_IDS[MAX_DELVE_TIERS] =
{
    1260938, 1260942, 1260946, 1260950, 1260954,
    1260957, 1260960, 1260963, 1260967, 1260970, 1260973
};

static constexpr char const* TIER_NAMES[MAX_DELVE_TIERS] =
{
    "Tier 1", "Tier 2", "Tier 3", "Tier 4", "Tier 5",
    "Tier 6", "Tier 7", "Tier 8", "Tier 9", "Tier 10", "Tier 11"
};

struct DelveInfo
{
    // Gossip identification (per-spawn via gossip_menu_addon)
    uint32 GossipMenuId;
    int32  LfgDungeonsId;
    int32  BroadcastTextId;
    int32  FirstTierGossipOptionId;     // Tier 1 GossipOptionID (tiers count DOWN)
    uint32 MapId;
    float EntryX, EntryY, EntryZ, EntryO;
    float ExitX, ExitY, ExitZ, ExitO;
    uint32 ActiveScenarioId;
    uint32 RewardScenarioId;

    // Per-delve WorldState 26903 value (controls center spell display in tier selection UI)
    // Sourced from SMSG_UPDATE_WORLD_STATE in the fog wall gossip sniff for each delve.
    int32 UnknownWS26903;
};

static const DelveInfo DELVE_TABLE[] =
{
    // Atal'Aman
    {
        39751, 3025, 292789, 134444,            // Gossip
        2962,                                    // Map
        5134.376f, -5861.9463f, 217.18855f, 0.017530434f,   // Entry pos
        5121.0f, -5861.4f, 217.1f, 3.22f,       // Exit pos (Map 0)
        3147, 3424,                              // Scenarios
        1278258                                  // WS 26903 - Atal'Aman sniff
    },
    // Shadow Enclave
    {
        40277, 3069, 296637, 135336,            // Gossip
        2952,                                    // Map
        -17.399f, 233.831f, 265.446f, 3.678f,   // Entry pos
        4781.7f, -4120.3f, 31.3f, 0.0f,         // Exit pos (Map 0)
        3154, 3424,                              // Scenarios
        1265777                                  // WS 26903 - from Shadow Enclave sniff
    },
};

static constexpr uint32 DELVE_TABLE_SIZE = sizeof(DELVE_TABLE) / sizeof(DELVE_TABLE[0]);

// Lookup by GossipMenuId (used by fog wall NPC)
inline DelveInfo const* GetDelveInfoByGossipMenu(uint32 gossipMenuId)
{
    for (uint32 i = 0; i < DELVE_TABLE_SIZE; ++i)
        if (DELVE_TABLE[i].GossipMenuId == gossipMenuId)
            return &DELVE_TABLE[i];
    return nullptr;
}

// Lookup by MapId (used by instance scripts and exit handlers)
inline DelveInfo const* GetDelveInfoByMap(uint32 mapId)
{
    for (uint32 i = 0; i < DELVE_TABLE_SIZE; ++i)
        if (DELVE_TABLE[i].MapId == mapId)
            return &DELVE_TABLE[i];
    return nullptr;
}

// Shared treasure preview data (from sniff)
struct DelveTreasurePreview { int32 ItemID; int8 ItemContext; };
static constexpr DelveTreasurePreview TREASURE_3_ITEMS[] = { { 257379, 79 }, { 257382, 79 }, { 257386, 107 } };
static constexpr DelveTreasurePreview TREASURE_2_ITEMS[] = { { 257382, 79 }, { 257386, 107 } };

// Pending delve entry data - set in gossip handler, read in OnPlayerEnter
struct PendingDelveEntry
{
    uint32 MapId;
    uint32 TierIndex;   // 0-based
};

// Global pending entries map - thread-safe enough for single-threaded map updates
inline std::unordered_map<ObjectGuid, PendingDelveEntry>& GetPendingDelveEntries()
{
    static std::unordered_map<ObjectGuid, PendingDelveEntry> s_pending;
    return s_pending;
}

// Delve-specific WorldState IDs (from sniff analysis)
enum DelveWorldStates
{
    // Sent on entry (global, from gossip handler)
    WS_DELVE_TIER               = 24430,
    WS_DELVE_IN_DELVE_FLAG      = 26345,
    WS_DELVE_MAP_ID             = 26423,
    WS_DELVE_TIER_SPELL         = 26931,
    WS_DELVE_UNKNOWN_26903      = 26903, // Unknown - sniff value 1265777
    WS_DELVE_LFG_DUNGEONS_ID   = 5029, // Sent in instance OnPlayerEnter
};

#endif // DELVE_DATA_H
