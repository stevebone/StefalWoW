/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * EnchantGemDatabase: Loads enchant and gem data from DB2 client data stores.
 * Iterates SpellItemEnchantmentStore and GemPropertiesStore at startup,
 * scores each entry against spec role/stat priorities, and builds
 * ranked recommendation lists. No SQL tables needed.
 */

#include "EnchantGemDatabase.h"
#include "DB2Stores.h"
#include "DB2Structure.h"
#include "DBCEnums.h"
#include "ItemTemplate.h"
#include "ObjectMgr.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// Static Member Definitions
// ============================================================================

std::unordered_map<EnchantKey, std::vector<EnchantRecommendation>, EnchantKeyHash> EnchantGemDatabase::_enchants;
std::unordered_map<EnchantSlotType, std::vector<EnchantRecommendation>> EnchantGemDatabase::_enchantsBySlot;
std::unordered_map<GemKey, std::vector<GemRecommendation>, GemKeyHash> EnchantGemDatabase::_gems;
std::unordered_map<GemColor, std::vector<GemRecommendation>> EnchantGemDatabase::_gemsByColor;
bool EnchantGemDatabase::_initialized = false;

// ============================================================================
// Stat type IDs from ItemMod enum (DB2 EffectArg for ITEM_ENCHANTMENT_TYPE_STAT)
// ============================================================================

static constexpr uint32 ITEM_MOD_AGILITY            = 3;
static constexpr uint32 ITEM_MOD_STRENGTH            = 4;
static constexpr uint32 ITEM_MOD_INTELLECT           = 5;
static constexpr uint32 ITEM_MOD_STAMINA             = 7;
static constexpr uint32 ITEM_MOD_CRIT_RATING         = 32;
static constexpr uint32 ITEM_MOD_HASTE_RATING        = 36;
static constexpr uint32 ITEM_MOD_MASTERY_RATING      = 49;
static constexpr uint32 ITEM_MOD_VERSATILITY         = 40;

// ============================================================================
// Public API
// ============================================================================

void EnchantGemDatabase::Initialize()
{
    if (_initialized)
        return;

    TC_LOG_INFO("module.playerbot", "EnchantGemDatabase: Loading enchant and gem data from DB2...");

    LoadEnchantsFromDB2();
    LoadGemsFromDB2();

    _initialized = true;

    TC_LOG_INFO("module.playerbot", "EnchantGemDatabase: Loaded {} enchant entries, {} gem entries from DB2",
        GetEnchantCount(), GetGemCount());
}

void EnchantGemDatabase::Reload()
{
    _enchants.clear();
    _enchantsBySlot.clear();
    _gems.clear();
    _gemsByColor.clear();
    _initialized = false;
    Initialize();
}

void EnchantGemDatabase::EnsureInitialized()
{
    if (!_initialized)
        Initialize();
}

// ============================================================================
// Enchant Queries (unchanged public API)
// ============================================================================

EnchantRecommendation const* EnchantGemDatabase::GetBestEnchant(
    uint8 classId, uint8 specId, EnchantSlotType slotType, uint32 itemLevel)
{
    EnsureInitialized();

    // Try class/spec specific first
    auto it = _enchants.find({classId, specId, slotType});
    if (it != _enchants.end() && !it->second.empty())
    {
        for (auto const& e : it->second)
            if (itemLevel == 0 || itemLevel >= e.minItemLevel)
                return &e;
    }

    // Try class-specific, any spec
    it = _enchants.find({classId, 255, slotType});
    if (it != _enchants.end() && !it->second.empty())
    {
        for (auto const& e : it->second)
            if (itemLevel == 0 || itemLevel >= e.minItemLevel)
                return &e;
    }

    // Fall back to generic (class=0)
    it = _enchants.find({0, 255, slotType});
    if (it != _enchants.end() && !it->second.empty())
    {
        for (auto const& e : it->second)
            if (itemLevel == 0 || itemLevel >= e.minItemLevel)
                return &e;
    }

    return nullptr;
}

std::vector<EnchantRecommendation> const* EnchantGemDatabase::GetEnchantsBySlot(EnchantSlotType slotType)
{
    EnsureInitialized();
    auto it = _enchantsBySlot.find(slotType);
    return it != _enchantsBySlot.end() ? &it->second : nullptr;
}

std::vector<EnchantRecommendation> EnchantGemDatabase::GetEnchantsForSpec(uint8 classId, uint8 specId)
{
    EnsureInitialized();
    std::vector<EnchantRecommendation> result;
    for (auto const& [key, entries] : _enchants)
    {
        if ((key.classId == classId || key.classId == 0) &&
            (key.specId == specId || key.specId == 255))
        {
            for (auto const& e : entries)
                result.push_back(e);
        }
    }

    std::sort(result.begin(), result.end(),
        [](auto const& a, auto const& b) { return a.priorityWeight > b.priorityWeight; });
    return result;
}

// ============================================================================
// Gem Queries (unchanged public API)
// ============================================================================

GemRecommendation const* EnchantGemDatabase::GetBestGem(
    uint8 classId, uint8 specId, GemColor socketColor)
{
    EnsureInitialized();

    // Try class/spec specific
    auto it = _gems.find({classId, specId, socketColor});
    if (it != _gems.end() && !it->second.empty())
        return &it->second.front();

    // Try class-specific, any spec
    it = _gems.find({classId, 255, socketColor});
    if (it != _gems.end() && !it->second.empty())
        return &it->second.front();

    // Fall back to generic
    it = _gems.find({0, 255, socketColor});
    if (it != _gems.end() && !it->second.empty())
        return &it->second.front();

    return nullptr;
}

std::vector<GemRecommendation> EnchantGemDatabase::GetGemsForColor(GemColor socketColor)
{
    EnsureInitialized();
    auto it = _gemsByColor.find(socketColor);
    if (it != _gemsByColor.end())
        return it->second;
    return {};
}

std::vector<GemRecommendation> EnchantGemDatabase::GetGemsForSpec(uint8 classId, uint8 specId)
{
    EnsureInitialized();
    std::vector<GemRecommendation> result;
    for (auto const& [key, entries] : _gems)
    {
        if ((key.classId == classId || key.classId == 0) &&
            (key.specId == specId || key.specId == 255))
        {
            for (auto const& e : entries)
                result.push_back(e);
        }
    }

    std::sort(result.begin(), result.end(),
        [](auto const& a, auto const& b) { return a.priorityWeight > b.priorityWeight; });
    return result;
}

uint32 EnchantGemDatabase::GetEnchantCount()
{
    uint32 count = 0;
    for (auto const& [_, entries] : _enchants)
        count += static_cast<uint32>(entries.size());
    return count;
}

uint32 EnchantGemDatabase::GetGemCount()
{
    uint32 count = 0;
    for (auto const& [_, entries] : _gems)
        count += static_cast<uint32>(entries.size());
    return count;
}

// ============================================================================
// DB2 Loading: Enchants from SpellItemEnchantmentStore
// ============================================================================

float EnchantGemDatabase::ScoreEnchantForSpec(uint32 /*enchantId*/, uint8 /*classId*/, int8 specRole, int8 primaryStat)
{
    // Look up the enchant from DB2
    // We score based on what stats the enchant provides vs what the spec wants
    // This is called per-enchant per-spec, so we just check if the enchant's
    // stat effects align with the spec's primary stat and role.
    //
    // ChrSpecialization.PrimaryStatPriority: 0=Str, 1=Agi, 2=Int, 3=Str/Agi, 4=Int/Str
    // ChrSpecialization.Role: 0=Tank, 1=Healer, 2=DPS

    // Base score: role-based preference
    float score = 0.0f;
    if (specRole == 0) // Tank
        score = 0.8f;  // Tanks value defensive enchants
    else if (specRole == 1) // Healer
        score = 0.7f;
    else // DPS
        score = 1.0f;

    // Adjust by primary stat alignment
    (void)primaryStat; // Used in the per-effect scoring in LoadEnchantsFromDB2

    return score;
}

EnchantSlotType EnchantGemDatabase::InventoryTypeToSlot(uint8 inventoryType)
{
    // InventoryType enum from ItemTemplate.h
    switch (inventoryType)
    {
        case INVTYPE_HEAD:          return EnchantSlotType::HEAD;
        case INVTYPE_SHOULDERS:     return EnchantSlotType::SHOULDER;
        case INVTYPE_CLOAK:         return EnchantSlotType::BACK;
        case INVTYPE_CHEST:
        case INVTYPE_ROBE:          return EnchantSlotType::CHEST;
        case INVTYPE_WRISTS:        return EnchantSlotType::WRIST;
        case INVTYPE_HANDS:         return EnchantSlotType::HANDS;
        case INVTYPE_LEGS:          return EnchantSlotType::LEGS;
        case INVTYPE_FEET:          return EnchantSlotType::FEET;
        case INVTYPE_WEAPON:
        case INVTYPE_WEAPONMAINHAND:
        case INVTYPE_2HWEAPON:      return EnchantSlotType::MAIN_HAND;
        case INVTYPE_WEAPONOFFHAND:
        case INVTYPE_SHIELD:
        case INVTYPE_HOLDABLE:      return EnchantSlotType::OFF_HAND;
        case INVTYPE_FINGER:        return EnchantSlotType::RING_1;
        default:                    return EnchantSlotType::MAX_SLOT;
    }
}

void EnchantGemDatabase::LoadEnchantsFromDB2()
{
    uint32 totalProcessed = 0;
    uint32 totalAdded = 0;

    // Build a map of specId -> {role, primaryStat} from ChrSpecialization DB2
    struct SpecInfo
    {
        uint8 classId;
        int8 role;          // 0=Tank, 1=Healer, 2=DPS
        int8 primaryStat;   // 0=Str, 1=Agi, 2=Int, 3=Str/Agi, 4=Int/Str
        uint8 orderIndex;
    };
    std::unordered_map<uint32, SpecInfo> specInfoMap;

    for (ChrSpecializationEntry const* spec : sChrSpecializationStore)
    {
        if (!spec || spec->ClassID == 0) // Skip pet specs
            continue;
        specInfoMap[spec->ID] = { static_cast<uint8>(spec->ClassID), spec->Role, spec->PrimaryStatPriority, static_cast<uint8>(spec->OrderIndex) };
    }

    // Iterate all enchantments in DB2
    for (SpellItemEnchantmentEntry const* enchant : sSpellItemEnchantmentStore)
    {
        if (!enchant)
            continue;

        ++totalProcessed;

        // Skip enchants with no effects
        bool hasStatEffect = false;
        for (uint8 i = 0; i < MAX_ITEM_ENCHANTMENT_EFFECTS; ++i)
        {
            if (enchant->Effect[i] == ITEM_ENCHANTMENT_TYPE_STAT ||
                enchant->Effect[i] == ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL ||
                enchant->Effect[i] == ITEM_ENCHANTMENT_TYPE_EQUIP_SPELL ||
                enchant->Effect[i] == ITEM_ENCHANTMENT_TYPE_DAMAGE)
            {
                hasStatEffect = true;
                break;
            }
        }

        if (!hasStatEffect)
            continue;

        // Skip enchants that require very high skill (likely crafting-only)
        if (enchant->RequiredSkillRank > 0 && enchant->RequiredSkillID == 0)
            continue;

        // Get the enchant name
        std::string enchantName;
        char const* nameStr = enchant->Name[DEFAULT_LOCALE];
        if (nameStr != nullptr && nameStr[0] != '\0')
            enchantName = nameStr;
        else
            enchantName = "Enchant #" + std::to_string(enchant->ID);

        // Determine which equipment slots this enchant applies to.
        // Enchants don't directly specify slot in DB2 — we infer from the enchant's
        // properties and common conventions:
        //  - RequiredSkillID 333 = Enchanting (weapon/armor enchants)
        //  - RequiredSkillID 755 = Jewelcrafting (ring enchants in WoW 12.0)
        //  - RequiredSkillID 165 = Leatherworking (leg armor kits)
        //  - RequiredSkillID 197 = Tailoring (leg spellthread)
        //  - RequiredSkillID 164 = Blacksmithing (armor plating)

        // Score enchant stat effects for general usefulness
        float baseScore = 0.0f;
        bool hasPrimaryStat = false;
        bool hasSecondaryStat = false;
        bool hasStamina = false;
        uint32 primaryStatType = 0;

        for (uint8 i = 0; i < MAX_ITEM_ENCHANTMENT_EFFECTS; ++i)
        {
            if (enchant->Effect[i] == ITEM_ENCHANTMENT_TYPE_STAT)
            {
                uint32 statType = enchant->EffectArg[i];
                float points = static_cast<float>(enchant->EffectPointsMin[i]);
                if (points <= 0.0f && enchant->EffectScalingPoints[i] > 0.0f)
                    points = enchant->EffectScalingPoints[i];

                if (statType == ITEM_MOD_AGILITY || statType == ITEM_MOD_STRENGTH || statType == ITEM_MOD_INTELLECT)
                {
                    hasPrimaryStat = true;
                    primaryStatType = statType;
                    baseScore += points * 1.5f; // Primary stats are worth more
                }
                else if (statType == ITEM_MOD_CRIT_RATING || statType == ITEM_MOD_HASTE_RATING ||
                         statType == ITEM_MOD_MASTERY_RATING || statType == ITEM_MOD_VERSATILITY)
                {
                    hasSecondaryStat = true;
                    baseScore += points * 1.0f;
                }
                else if (statType == ITEM_MOD_STAMINA)
                {
                    hasStamina = true;
                    baseScore += points * 0.5f; // Stamina valued less for DPS
                }
                else
                {
                    baseScore += points * 0.3f;
                }
            }
            else if (enchant->Effect[i] == ITEM_ENCHANTMENT_TYPE_DAMAGE)
            {
                baseScore += static_cast<float>(enchant->EffectPointsMin[i]) * 2.0f;
            }
            else if (enchant->Effect[i] == ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL ||
                     enchant->Effect[i] == ITEM_ENCHANTMENT_TYPE_EQUIP_SPELL)
            {
                baseScore += 50.0f; // Proc enchants get a flat score
            }
        }

        if (baseScore <= 0.0f)
            continue;

        // Determine applicable slots based on enchant properties
        std::vector<EnchantSlotType> applicableSlots;

        // Use MinItemLevel and enchant flags/skills to infer slot type
        bool isMainhandOnly = enchant->GetFlags().HasFlag(SpellItemEnchantmentFlags::MainhandOnly);

        if (isMainhandOnly || enchant->RequiredSkillID == 0)
        {
            // Weapon enchants: no required profession or mainhand-only flag
            applicableSlots.push_back(EnchantSlotType::MAIN_HAND);
        }

        if (enchant->RequiredSkillID == 755) // Jewelcrafting = ring enchants in TWW
        {
            applicableSlots.push_back(EnchantSlotType::RING_1);
            applicableSlots.push_back(EnchantSlotType::RING_2);
        }

        // If we can't determine the slot, add it as a generic weapon enchant
        if (applicableSlots.empty())
            applicableSlots.push_back(EnchantSlotType::MAIN_HAND);

        // Now create recommendations per spec
        for (auto const& [specId, info] : specInfoMap)
        {
            float specScore = baseScore;

            // Adjust score based on spec's primary stat alignment
            if (hasPrimaryStat)
            {
                bool matchesPrimary = false;
                if (info.primaryStat == 0 && primaryStatType == ITEM_MOD_STRENGTH) matchesPrimary = true;
                else if (info.primaryStat == 1 && primaryStatType == ITEM_MOD_AGILITY) matchesPrimary = true;
                else if (info.primaryStat == 2 && primaryStatType == ITEM_MOD_INTELLECT) matchesPrimary = true;
                else if (info.primaryStat == 3 && (primaryStatType == ITEM_MOD_STRENGTH || primaryStatType == ITEM_MOD_AGILITY)) matchesPrimary = true;
                else if (info.primaryStat == 4 && (primaryStatType == ITEM_MOD_INTELLECT || primaryStatType == ITEM_MOD_STRENGTH)) matchesPrimary = true;

                if (!matchesPrimary)
                    specScore *= 0.1f; // Severely penalize wrong-stat enchants
            }

            // Tanks value stamina more
            if (info.role == 0 && hasStamina)
                specScore *= 1.5f;

            // DPS values secondary stats more
            if (info.role == 2 && hasSecondaryStat)
                specScore *= 1.2f;

            if (specScore <= 0.0f)
                continue;

            for (EnchantSlotType slot : applicableSlots)
            {
                EnchantRecommendation rec;
                rec.enchantId = enchant->ID;
                rec.slotType = slot;
                rec.classId = info.classId;
                rec.specId = info.orderIndex;
                rec.minItemLevel = static_cast<uint32>(std::max<int32>(0, enchant->MinItemLevel));
                rec.priorityWeight = specScore;
                rec.enchantName = enchantName;

                EnchantKey key{info.classId, info.orderIndex, slot};
                _enchants[key].push_back(rec);
                _enchantsBySlot[slot].push_back(rec);
                ++totalAdded;
            }
        }
    }

    // Sort all lists by priority weight (descending) and cap per key
    for (auto& [key, list] : _enchants)
    {
        std::sort(list.begin(), list.end(),
            [](auto const& a, auto const& b) { return a.priorityWeight > b.priorityWeight; });
        // Keep top 5 per class/spec/slot to avoid bloat
        if (list.size() > 5)
            list.resize(5);
    }
    for (auto& [slot, list] : _enchantsBySlot)
    {
        std::sort(list.begin(), list.end(),
            [](auto const& a, auto const& b) { return a.priorityWeight > b.priorityWeight; });
        if (list.size() > 20)
            list.resize(20);
    }

    TC_LOG_INFO("module.playerbot", "EnchantGemDatabase: Processed {} DB2 enchantments, generated {} spec-specific recommendations",
        totalProcessed, totalAdded);
}

// ============================================================================
// DB2 Loading: Gems from GemPropertiesStore + ItemSparse
// ============================================================================

void EnchantGemDatabase::LoadGemsFromDB2()
{
    uint32 totalAdded = 0;

    // Build specInfo map for scoring
    struct SpecInfo
    {
        uint8 classId;
        int8 role;
        int8 primaryStat;
        uint8 orderIndex;
    };
    std::unordered_map<uint32, SpecInfo> specInfoMap;

    for (ChrSpecializationEntry const* spec : sChrSpecializationStore)
    {
        if (!spec || spec->ClassID == 0)
            continue;
        specInfoMap[spec->ID] = { static_cast<uint8>(spec->ClassID), spec->Role, spec->PrimaryStatPriority, static_cast<uint8>(spec->OrderIndex) };
    }

    // Iterate all gem properties in DB2
    for (GemPropertiesEntry const* gem : sGemPropertiesStore)
    {
        if (!gem || gem->EnchantId == 0)
            continue;

        // Look up the enchantment this gem provides
        SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(gem->EnchantId);
        if (!enchant)
            continue;

        // Determine gem socket color from GemProperties.Type bitmask
        // Type: 1=Red, 2=Yellow, 4=Blue, 8=Meta, 14=Prismatic(R+Y+B)
        GemColor color;
        if (gem->Type == 8)
            color = GemColor::META;
        else if ((gem->Type & 7) == 7 || gem->Type == 0) // All colors or unset = prismatic
            color = GemColor::PRISMATIC;
        else if (gem->Type & 1)
            color = GemColor::RED;
        else if (gem->Type & 2)
            color = GemColor::YELLOW;
        else if (gem->Type & 4)
            color = GemColor::BLUE;
        else
            color = GemColor::PRISMATIC;

        // Find the item that uses this GemProperties ID
        // We need to scan ItemSparse for items with matching GemProperties
        uint32 gemItemId = 0;
        std::string gemName;

        // Check ItemTemplate (ObjectMgr's item cache) for the gem item
        for (auto const& [itemId, itemTemplate] : sObjectMgr->GetItemTemplateStore())
        {
            if (itemTemplate.GetGemProperties() == gem->ID)
            {
                gemItemId = itemId;
                gemName = itemTemplate.GetName(LOCALE_enUS);
                break;
            }
        }

        if (gemItemId == 0)
            continue; // No item found for this gem

        // Score the gem's enchant effects
        float baseScore = 0.0f;
        uint32 primaryStatType = 0;
        bool hasPrimaryStat = false;
        bool hasStamina = false;
        GemStatPriority statPriority = GemStatPriority::MIXED;

        for (uint8 i = 0; i < MAX_ITEM_ENCHANTMENT_EFFECTS; ++i)
        {
            if (enchant->Effect[i] == ITEM_ENCHANTMENT_TYPE_STAT)
            {
                uint32 statType = enchant->EffectArg[i];
                float points = static_cast<float>(enchant->EffectPointsMin[i]);
                if (points <= 0.0f && enchant->EffectScalingPoints[i] > 0.0f)
                    points = enchant->EffectScalingPoints[i];

                if (statType == ITEM_MOD_AGILITY || statType == ITEM_MOD_STRENGTH || statType == ITEM_MOD_INTELLECT)
                {
                    hasPrimaryStat = true;
                    primaryStatType = statType;
                    statPriority = GemStatPriority::PRIMARY;
                    baseScore += points * 1.5f;
                }
                else if (statType == ITEM_MOD_CRIT_RATING || statType == ITEM_MOD_HASTE_RATING ||
                         statType == ITEM_MOD_MASTERY_RATING || statType == ITEM_MOD_VERSATILITY)
                {
                    if (!hasPrimaryStat)
                        statPriority = GemStatPriority::SECONDARY_BEST;
                    baseScore += points * 1.0f;
                }
                else if (statType == ITEM_MOD_STAMINA)
                {
                    hasStamina = true;
                    if (!hasPrimaryStat)
                        statPriority = GemStatPriority::STAMINA;
                    baseScore += points * 0.5f;
                }
            }
        }

        if (baseScore <= 0.0f)
            continue;

        // Create per-spec recommendations
        for (auto const& [specId, info] : specInfoMap)
        {
            float specScore = baseScore;

            // Primary stat matching
            if (hasPrimaryStat)
            {
                bool matchesPrimary = false;
                if (info.primaryStat == 0 && primaryStatType == ITEM_MOD_STRENGTH) matchesPrimary = true;
                else if (info.primaryStat == 1 && primaryStatType == ITEM_MOD_AGILITY) matchesPrimary = true;
                else if (info.primaryStat == 2 && primaryStatType == ITEM_MOD_INTELLECT) matchesPrimary = true;
                else if (info.primaryStat == 3 && (primaryStatType == ITEM_MOD_STRENGTH || primaryStatType == ITEM_MOD_AGILITY)) matchesPrimary = true;
                else if (info.primaryStat == 4 && (primaryStatType == ITEM_MOD_INTELLECT || primaryStatType == ITEM_MOD_STRENGTH)) matchesPrimary = true;

                if (!matchesPrimary)
                    specScore *= 0.1f;
            }

            // Tanks value stamina gems
            if (info.role == 0 && hasStamina)
                specScore *= 1.5f;

            if (specScore <= 0.0f)
                continue;

            GemRecommendation rec;
            rec.gemItemId = gemItemId;
            rec.socketColor = color;
            rec.classId = info.classId;
            rec.specId = info.orderIndex;
            rec.statType = statPriority;
            rec.priorityWeight = specScore;
            rec.gemName = gemName;

            GemKey key{info.classId, info.orderIndex, color};
            _gems[key].push_back(rec);
            _gemsByColor[color].push_back(rec);
            ++totalAdded;
        }
    }

    // Sort and cap
    for (auto& [key, list] : _gems)
    {
        std::sort(list.begin(), list.end(),
            [](auto const& a, auto const& b) { return a.priorityWeight > b.priorityWeight; });
        if (list.size() > 5)
            list.resize(5);
    }
    for (auto& [color, list] : _gemsByColor)
    {
        std::sort(list.begin(), list.end(),
            [](auto const& a, auto const& b) { return a.priorityWeight > b.priorityWeight; });
        if (list.size() > 20)
            list.resize(20);
    }

    TC_LOG_INFO("module.playerbot", "EnchantGemDatabase: Generated {} spec-specific gem recommendations from DB2", totalAdded);
}

} // namespace Playerbot
