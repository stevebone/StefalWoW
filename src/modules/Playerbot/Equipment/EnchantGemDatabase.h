/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * EnchantGemDatabase: Static database of optimal enchants and gems per
 * class/spec/slot. Loaded from DB2 client data at startup — iterates
 * SpellItemEnchantmentStore and GemPropertiesStore, scores each entry
 * against the spec's primary stat and role to build ranked recommendations.
 *
 * No SQL tables needed — all data comes from authoritative WoW client DB2.
 *
 * Follows the InterruptDatabase pattern (static Initialize, static maps,
 * static query methods).
 */

#pragma once

#include "Define.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace Playerbot
{

// ============================================================================
// Enumerations
// ============================================================================

/// Equipment slot categories for enchant lookup
enum class EnchantSlotType : uint8
{
    HEAD = 0,
    SHOULDER,
    BACK,
    CHEST,
    WRIST,
    HANDS,
    LEGS,
    FEET,
    MAIN_HAND,
    OFF_HAND,
    RING_1,
    RING_2,

    MAX_SLOT
};

/// Gem socket color matching
enum class GemColor : uint8
{
    RED = 0,
    BLUE,
    YELLOW,
    PRISMATIC,
    META,
    COGWHEEL,

    MAX_COLOR
};

/// Stat priority category for gem selection
enum class GemStatPriority : uint8
{
    PRIMARY = 0,        // Primary stat gem (Str/Agi/Int)
    SECONDARY_BEST,     // Best secondary stat
    SECONDARY_SECOND,   // Second-best secondary stat
    STAMINA,            // Stamina for tanks
    MIXED,              // Hybrid gem
};

// ============================================================================
// Data Structures
// ============================================================================

/// An enchant recommendation for a specific slot
struct EnchantRecommendation
{
    uint32 enchantId;           // SpellItemEnchantment ID
    EnchantSlotType slotType;
    uint8 classId;              // 0 = any class
    uint8 specId;               // 255 = any spec for that class
    uint32 minItemLevel;        // Minimum item level for this enchant (0 = no min)
    float priorityWeight;       // Higher = more preferred
    std::string enchantName;    // Human-readable name

    EnchantRecommendation()
        : enchantId(0), slotType(EnchantSlotType::MAIN_HAND)
        , classId(0), specId(255), minItemLevel(0)
        , priorityWeight(1.0f) {}

    EnchantRecommendation(uint32 id, EnchantSlotType slot, uint8 cls, uint8 spec,
                          uint32 minIlvl, float weight, std::string name)
        : enchantId(id), slotType(slot), classId(cls), specId(spec)
        , minItemLevel(minIlvl), priorityWeight(weight)
        , enchantName(std::move(name)) {}
};

/// A gem recommendation for a socket color
struct GemRecommendation
{
    uint32 gemItemId;           // Item entry for the gem
    GemColor socketColor;       // Which socket color this fits
    uint8 classId;              // 0 = any class
    uint8 specId;               // 255 = any spec
    GemStatPriority statType;
    float priorityWeight;
    std::string gemName;

    GemRecommendation()
        : gemItemId(0), socketColor(GemColor::PRISMATIC)
        , classId(0), specId(255)
        , statType(GemStatPriority::PRIMARY)
        , priorityWeight(1.0f) {}

    GemRecommendation(uint32 id, GemColor color, uint8 cls, uint8 spec,
                      GemStatPriority stat, float weight, std::string name)
        : gemItemId(id), socketColor(color), classId(cls), specId(spec)
        , statType(stat), priorityWeight(weight)
        , gemName(std::move(name)) {}
};

// ============================================================================
// Lookup key for class/spec/slot combinations
// ============================================================================

struct EnchantKey
{
    uint8 classId;
    uint8 specId;
    EnchantSlotType slotType;

    bool operator==(EnchantKey const& other) const
    {
        return classId == other.classId && specId == other.specId && slotType == other.slotType;
    }
};

struct EnchantKeyHash
{
    std::size_t operator()(EnchantKey const& k) const
    {
        return (static_cast<std::size_t>(k.classId) << 16)
             | (static_cast<std::size_t>(k.specId) << 8)
             | static_cast<std::size_t>(k.slotType);
    }
};

struct GemKey
{
    uint8 classId;
    uint8 specId;
    GemColor socketColor;

    bool operator==(GemKey const& other) const
    {
        return classId == other.classId && specId == other.specId && socketColor == other.socketColor;
    }
};

struct GemKeyHash
{
    std::size_t operator()(GemKey const& k) const
    {
        return (static_cast<std::size_t>(k.classId) << 16)
             | (static_cast<std::size_t>(k.specId) << 8)
             | static_cast<std::size_t>(k.socketColor);
    }
};

// ============================================================================
// EnchantGemDatabase - Static Database
// ============================================================================

class TC_GAME_API EnchantGemDatabase
{
public:
    /// Initialize from DB2 client data stores (SpellItemEnchantment, GemProperties).
    static void Initialize();

    /// Check if initialized.
    static bool IsInitialized() { return _initialized; }

    /// Reload from DB2 data.
    static void Reload();

    // ========================================================================
    // Enchant Queries
    // ========================================================================

    /// Get the best enchant for a class/spec/slot combination.
    /// Falls back to generic (class=0) if no class-specific match.
    static EnchantRecommendation const* GetBestEnchant(
        uint8 classId, uint8 specId, EnchantSlotType slotType, uint32 itemLevel = 0);

    /// Get all enchant recommendations for a slot (any class).
    static std::vector<EnchantRecommendation> const* GetEnchantsBySlot(EnchantSlotType slotType);

    /// Get all enchant recommendations for a class/spec.
    static std::vector<EnchantRecommendation> GetEnchantsForSpec(uint8 classId, uint8 specId);

    // ========================================================================
    // Gem Queries
    // ========================================================================

    /// Get the best gem for a class/spec/socket color.
    /// Falls back to generic if no class-specific match.
    static GemRecommendation const* GetBestGem(
        uint8 classId, uint8 specId, GemColor socketColor);

    /// Get all gem recommendations for a socket color.
    static std::vector<GemRecommendation> GetGemsForColor(GemColor socketColor);

    /// Get all gem recommendations for a class/spec.
    static std::vector<GemRecommendation> GetGemsForSpec(uint8 classId, uint8 specId);

    // ========================================================================
    // Statistics
    // ========================================================================

    /// Get total loaded enchant recommendations.
    static uint32 GetEnchantCount();

    /// Get total loaded gem recommendations.
    static uint32 GetGemCount();

private:
    static void LoadEnchantsFromDB2();
    static void LoadGemsFromDB2();
    static void EnsureInitialized();

    /// Score an enchant's stat effects against a spec's role/stat priorities.
    /// Returns 0.0 if the enchant provides no useful stats.
    static float ScoreEnchantForSpec(uint32 enchantId, uint8 classId, int8 specRole, int8 primaryStat);

    /// Map an InventoryType (from ItemSparse) to our EnchantSlotType.
    static EnchantSlotType InventoryTypeToSlot(uint8 inventoryType);

    // Enchant storage: key -> sorted list (highest priority first)
    static std::unordered_map<EnchantKey, std::vector<EnchantRecommendation>, EnchantKeyHash> _enchants;
    // Also indexed by slot alone (for generic lookups)
    static std::unordered_map<EnchantSlotType, std::vector<EnchantRecommendation>> _enchantsBySlot;

    // Gem storage: key -> sorted list
    static std::unordered_map<GemKey, std::vector<GemRecommendation>, GemKeyHash> _gems;
    // Also indexed by color
    static std::unordered_map<GemColor, std::vector<GemRecommendation>> _gemsByColor;

    static bool _initialized;
};

} // namespace Playerbot
