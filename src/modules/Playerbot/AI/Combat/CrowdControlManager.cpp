/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#include "CrowdControlManager.h"
#include "Player.h"
#include "Unit.h"
#include "Group.h"
#include "ThreatManager.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "Log.h"
#include "Creature.h"
#include "GameTime.h"
#include "DBCEnums.h"  // For MAX_EFFECT_MASK
#include "ObjectAccessor.h"
#include "Core/Events/CombatEventRouter.h"
#include "Core/Events/CombatEvent.h"
#include <algorithm>

namespace Playerbot
{

namespace
{
    // CC spell info for availability checks
    struct CCSpellInfo
    {
        uint32 spellId;
        CrowdControlType ccType;
    };

    // Class-specific CC spell database (used for both GetAvailableCCSpells and HasCCAvailable)
    static const std::unordered_map<uint8, std::vector<CCSpellInfo>> classCCSpells = {
        // Mage
        {CLASS_MAGE, {
            {118, CrowdControlType::INCAPACITATE},      // Polymorph
            {82691, CrowdControlType::INCAPACITATE},    // Ring of Frost
            {122, CrowdControlType::ROOT},              // Frost Nova
            {31661, CrowdControlType::DISORIENT},       // Dragon's Breath
        }},
        // Rogue
        {CLASS_ROGUE, {
            {6770, CrowdControlType::INCAPACITATE},     // Sap
            {1776, CrowdControlType::STUN},             // Gouge
            {2094, CrowdControlType::DISORIENT},        // Blind
            {408, CrowdControlType::STUN},              // Kidney Shot
        }},
        // Hunter
        {CLASS_HUNTER, {
            {187650, CrowdControlType::INCAPACITATE},   // Freezing Trap
            {19386, CrowdControlType::INCAPACITATE},    // Wyvern Sting
            {213691, CrowdControlType::INCAPACITATE},   // Scatter Shot
            {109248, CrowdControlType::STUN},           // Binding Shot
        }},
        // Warlock
        {CLASS_WARLOCK, {
            {5782, CrowdControlType::DISORIENT},        // Fear
            {710, CrowdControlType::INCAPACITATE},      // Banish
            {6789, CrowdControlType::DISORIENT},        // Mortal Coil
            {30283, CrowdControlType::STUN},            // Shadowfury
        }},
        // Priest
        {CLASS_PRIEST, {
            {9484, CrowdControlType::INCAPACITATE},     // Shackle Undead
            {605, CrowdControlType::INCAPACITATE},      // Mind Control
            {8122, CrowdControlType::DISORIENT},        // Psychic Scream
            {200196, CrowdControlType::STUN},           // Holy Word: Chastise
        }},
        // Druid
        {CLASS_DRUID, {
            {339, CrowdControlType::ROOT},              // Entangling Roots
            {2637, CrowdControlType::INCAPACITATE},     // Hibernate
            {99, CrowdControlType::DISORIENT},          // Incapacitating Roar
            {5211, CrowdControlType::STUN},             // Mighty Bash
            {102359, CrowdControlType::ROOT},           // Mass Entanglement
        }},
        // Shaman
        {CLASS_SHAMAN, {
            {51514, CrowdControlType::INCAPACITATE},    // Hex
            {118905, CrowdControlType::STUN},           // Static Charge
            {197214, CrowdControlType::STUN},           // Sundering
        }},
        // Paladin
        {CLASS_PALADIN, {
            {20066, CrowdControlType::INCAPACITATE},    // Repentance
            {853, CrowdControlType::STUN},              // Hammer of Justice
            {115750, CrowdControlType::STUN},           // Blinding Light
            {10326, CrowdControlType::DISORIENT},       // Turn Evil
        }},
        // Death Knight
        {CLASS_DEATH_KNIGHT, {
            {108194, CrowdControlType::STUN},           // Asphyxiate
            {91807, CrowdControlType::STUN},            // Shambling Rush (Ghoul)
            {207167, CrowdControlType::DISORIENT},      // Blinding Sleet
        }},
        // Monk
        {CLASS_MONK, {
            {115078, CrowdControlType::INCAPACITATE},   // Paralysis
            {119381, CrowdControlType::STUN},           // Leg Sweep
            {198909, CrowdControlType::DISORIENT},      // Song of Chi-Ji
        }},
        // Warrior
        {CLASS_WARRIOR, {
            {5246, CrowdControlType::DISORIENT},        // Intimidating Shout
            {132168, CrowdControlType::STUN},           // Shockwave
            {132169, CrowdControlType::STUN},           // Storm Bolt
        }},
        // Demon Hunter
        {CLASS_DEMON_HUNTER, {
            {217832, CrowdControlType::INCAPACITATE},   // Imprison
            {179057, CrowdControlType::STUN},           // Chaos Nova
            {211881, CrowdControlType::STUN},           // Fel Eruption
        }},
        // Evoker
        {CLASS_EVOKER, {
            {360806, CrowdControlType::INCAPACITATE},   // Sleep Walk
            {357210, CrowdControlType::ROOT},           // Deep Breath knockback
        }},
    };

    /**
     * @brief Check if a player has any CC spell available (not on cooldown, has mana)
     * @param player Player to check
     * @return true if player has at least one CC spell ready to use
     */
    bool HasCCAvailable(Player* player)
    {
        if (!player)
            return false;

        uint8 playerClass = player->GetClass();
        auto classIt = classCCSpells.find(playerClass);
        if (classIt == classCCSpells.end())
            return false;

        for (const CCSpellInfo& ccInfo : classIt->second)
        {
            // Check if player knows the spell
            if (!player->HasSpell(ccInfo.spellId))
                continue;

            // Check if spell is on cooldown
            if (player->GetSpellHistory()->HasCooldown(ccInfo.spellId))
                continue;

            // Check if player has enough power to cast
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(ccInfo.spellId, DIFFICULTY_NONE);
            if (!spellInfo)
                continue;

            std::vector<SpellPowerCost> costs = spellInfo->CalcPowerCost(player, spellInfo->GetSchoolMask());
            bool hasPower = true;
            for (SpellPowerCost const& cost : costs)
            {
                if (player->GetPower(cost.Power) < cost.Amount)
                {
                    hasPower = false;
                    break;
                }
            }

            if (hasPower)
                return true;  // Found at least one available CC spell
        }

        return false;
    }
} // anonymous namespace

CrowdControlManager::CrowdControlManager(Player* bot)
    : _bot(bot)
    , _lastUpdate(0)
{
    // Phase 3: Subscribe to combat events for real-time CC tracking
    if (CombatEventRouter::Instance().IsInitialized())
    {
        CombatEventRouter::Instance().Subscribe(this);
        _subscribed = true;
        TC_LOG_DEBUG("playerbots", "CrowdControlManager: Subscribed to CombatEventRouter (event-driven mode)");
    }
    else
    {
        TC_LOG_DEBUG("playerbots", "CrowdControlManager: Initialized in polling mode (CombatEventRouter not ready)");
    }
}

CrowdControlManager::~CrowdControlManager()
{
    // Phase 3: Unsubscribe from combat events
    if (_subscribed && CombatEventRouter::Instance().IsInitialized())
    {
        CombatEventRouter::Instance().Unsubscribe(this);
        _subscribed = false;
        TC_LOG_DEBUG("playerbots", "CrowdControlManager: Unsubscribed from CombatEventRouter");
    }
}

void CrowdControlManager::Update(uint32 diff, const CombatMetrics& metrics)
{
    if (!_bot)
        return;

    // ========================================================================
    // Phase 3 Event-Driven Architecture:
    // - CC tracking updates moved to event handlers (HandleAuraApplied, etc.)
    // - Update() only runs maintenance tasks at reduced frequency
    // ========================================================================

    _maintenanceTimer += diff;

    // Run maintenance at reduced frequency (1Hz instead of 2Hz)
    if (_maintenanceTimer < MAINTENANCE_INTERVAL_MS && !_ccDataDirty)
        return;

    _maintenanceTimer = 0;
    _ccDataDirty = false;

    // Update expired CCs (maintenance task)
    UpdateExpiredCCs();

    // Update DR states (maintenance task)
    uint32 currentTime = GameTime::GetGameTimeMS();
    UpdateDR(currentTime);
}

void CrowdControlManager::Reset()
{
    _activeCCs.clear();
    _lastUpdate = 0;
}

bool CrowdControlManager::ShouldUseCrowdControl()
{
    if (!_bot)
        return false;

    // Get combat enemies
    ::std::vector<Unit*> enemies = GetCombatEnemies();

    // Need at least 2 enemies to justify CC
    if (enemies.size() < 2)
        return false;

    // Check for uncrowded targets
    uint32 uncrowdedCount = 0;
    for (Unit* enemy : enemies)
    {
        if (!IsTargetCCd(enemy))
            ++uncrowdedCount;
    }

    // Need at least 1 uncrowded target
    if (uncrowdedCount == 0)
        return false;

    // Check if bot has CC abilities available
    ::std::vector<uint32> ccSpells = GetAvailableCCSpells();
    return !ccSpells.empty();
}

Unit* CrowdControlManager::GetPriorityTarget()
{
    if (!_bot)
        return nullptr;

    ::std::vector<Unit*> enemies = GetCombatEnemies();

    // Score each target
    ::std::vector<::std::pair<Unit*, float>> scored;

    for (Unit* enemy : enemies)
    {
        // Skip already CC'd targets
    if (IsTargetCCd(enemy))
            continue;

        float priority = CalculateCCPriority(enemy);
        if (priority > 0.0f)
            scored.emplace_back(enemy, priority);
    }

    if (scored.empty())
        return nullptr;

    // Sort by priority (highest first)
    ::std::sort(scored.begin(), scored.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

    return scored.front().first;
}

uint32 CrowdControlManager::GetRecommendedSpell(Unit* target)
{
    if (!target)
        return 0;

    ::std::vector<uint32> spells = GetAvailableCCSpells();

    // Filter spells suitable for target
    for (uint32 spellId : spells)
    {
        if (IsSpellSuitableForTarget(spellId, target))
            return spellId;
    }

    return 0;
}

bool CrowdControlManager::ShouldCC(Unit* target, CrowdControlType type)
{
    if (!target || !_bot)
        return false;

    // Already CC'd?
    if (IsTargetCCd(target))
        return false;

    // Immune?
    if (IsImmune(target, type))
        return false;

    // Valid threat?
    if (target->isDead() || target->IsFriendlyTo(_bot))
        return false;

    return true;
}

void CrowdControlManager::ApplyCC(Unit* target, CrowdControlType type, uint32 duration, Player* bot, uint32 spellId)
{
    if (!target)
        return;

    CCTarget cc;
    cc.target = target;
    cc.type = type;
    cc.duration = duration;
    cc.appliedBy = bot;
    cc.expiryTime = GameTime::GetGameTimeMS() + duration;
    cc.spellId = spellId;

    _activeCCs[target->GetGUID()] = cc;

    TC_LOG_DEBUG("playerbot", "CrowdControlManager: {} applied {} CC on {} for {}ms",
        bot ? bot->GetName() : "unknown",
        static_cast<uint32>(type),
        target->GetName(),
        duration);
}

void CrowdControlManager::RemoveCC(Unit* target)
{
    if (!target)
        return;

    _activeCCs.erase(target->GetGUID());
}

Player* CrowdControlManager::GetChainCCBot(Unit* target)
{
    if (!target || !_bot)
        return nullptr;

    // Check if target needs chain CC soon
    const CCTarget* cc = GetActiveCC(target);
    if (!cc)
        return nullptr;

    // Only chain if within window
    if (cc->GetRemainingTime() > CHAIN_CC_WINDOW)
        return nullptr;

    // In group? Assign to another bot with available CC
    Group* group = _bot->GetGroup();
    if (group)
    {
        // Find another player with CC available
        for (GroupReference& ref : group->GetMembers())
        {
            Player* member = ref.GetSource();
            if (!member || member == cc->appliedBy)
                continue;

            // Check if member has a CC spell available (not on cooldown, has resources)
            if (HasCCAvailable(member))
                return member;
        }
    }

    // Solo or no other CCer: bot reapplies if they have CC available
    if (HasCCAvailable(_bot))
        return _bot;

    return nullptr;
}

bool CrowdControlManager::IsTargetCCd(Unit* target) const
{
    if (!target)
        return false;

    return GetActiveCC(target) != nullptr;
}

const CCTarget* CrowdControlManager::GetActiveCC(Unit* target) const
{
    if (!target)
        return nullptr;

    auto it = _activeCCs.find(target->GetGUID());
    if (it == _activeCCs.end())
        return nullptr;

    // Check if still active
    if (!it->second.IsActive())
        return nullptr;

    return &it->second;
}

::std::vector<Unit*> CrowdControlManager::GetCCdTargets() const
{
    ::std::vector<Unit*> targets;

    for (const auto& [guid, cc] : _activeCCs)
    {
        if (cc.IsActive() && cc.target)
            targets.push_back(cc.target);
    }

    return targets;
}

bool CrowdControlManager::ShouldBreakCC(Unit* target) const
{
    if (!target)
        return false;

    // Check if last enemy
    ::std::vector<Unit*> enemies = GetCombatEnemies();

    uint32 activeEnemies = 0;
    for (Unit* enemy : enemies)
    {
        if (enemy && !enemy->isDead() && !IsTargetCCd(enemy))
            ++activeEnemies;
    }

    // If only CC'd targets left, safe to break
    return activeEnemies == 0;
}

// Private helper functions

::std::vector<Unit*> CrowdControlManager::GetCombatEnemies() const
{
    ::std::vector<Unit*> enemies;

    if (!_bot)
        return enemies;

    ThreatManager& threatMgr = _bot->GetThreatManager();

    for (ThreatReference const* ref : threatMgr.GetUnsortedThreatList())
    {
        if (!ref)
            continue;

        Unit* enemy = ref->GetVictim();
        if (enemy && !enemy->isDead())
            enemies.push_back(enemy);
    }

    return enemies;
}

bool CrowdControlManager::IsImmune(Unit* target, CrowdControlType type) const
{
    if (!target)
        return true;

    // Check for CC immunity based on type
    switch (type)
    {
        case CrowdControlType::STUN:
            return target->HasAuraType(SPELL_AURA_MECHANIC_IMMUNITY);
        case CrowdControlType::INCAPACITATE:
            return target->HasAuraType(SPELL_AURA_MECHANIC_IMMUNITY);
        case CrowdControlType::DISORIENT:
            return target->HasAuraType(SPELL_AURA_MECHANIC_IMMUNITY);
        case CrowdControlType::ROOT:
            return target->HasAuraType(SPELL_AURA_MECHANIC_IMMUNITY);
        case CrowdControlType::SILENCE:
            return target->HasAuraType(SPELL_AURA_MOD_SILENCE);
        default:
            return false;
    }
}

float CrowdControlManager::CalculateCCPriority(Unit* target) const
{
    if (!target || !_bot)
        return 0.0f;

    float priority = 10.0f;  // Base priority

    // Healer = highest priority
    if (target->GetPowerType() == POWER_MANA)
    {
        // Simple heuristic: mana users are likely healers/casters
        priority += 40.0f;
    }

    // Caster bonus
    if (target->GetPowerType() == POWER_MANA)
        priority += 20.0f;

    // High HP = higher priority (will take longer to kill)
    if (target->GetHealthPct() > 80.0f)
        priority += 15.0f;

    // Elite bonus
    if (Creature* creature = target->ToCreature())
    {
        if (creature->IsElite())
            priority += 10.0f;
    }

    // Distance penalty (prefer nearby targets)
    float distance = _bot->GetDistance(target);
    if (distance > 30.0f)
        priority *= 0.7f;

    return priority;
}

::std::vector<uint32> CrowdControlManager::GetAvailableCCSpells() const
{
    ::std::vector<uint32> spells;

    if (!_bot)
        return spells;

    // Get spells for bot's class using shared classCCSpells from anonymous namespace
    uint8 botClass = _bot->GetClass();
    auto classIt = classCCSpells.find(botClass);
    if (classIt == classCCSpells.end())
        return spells;

    // QW-3 FIX: Pre-allocate for class CC spells (typically 4-8 per class)
    spells.reserve(classIt->second.size());

    // Check each CC spell for availability
    for (const CCSpellInfo& ccInfo : classIt->second)
    {
        // Check if bot knows the spell
        if (!_bot->HasSpell(ccInfo.spellId))
            continue;

        // Check if spell is on cooldown
        if (_bot->GetSpellHistory()->HasCooldown(ccInfo.spellId))
            continue;

        // Check if bot has enough power to cast
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(ccInfo.spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            continue;

        std::vector<SpellPowerCost> costs = spellInfo->CalcPowerCost(_bot, spellInfo->GetSchoolMask());
        bool hasPower = true;
        for (SpellPowerCost const& cost : costs)
        {
            if (_bot->GetPower(cost.Power) < cost.Amount)
            {
                hasPower = false;
                break;
            }
        }

        if (hasPower)
            spells.push_back(ccInfo.spellId);
    }

    return spells;
}

bool CrowdControlManager::IsSpellSuitableForTarget(uint32 spellId, Unit* target) const
{
    if (!target || !_bot || spellId == 0)
        return false;

    // Get spell info
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Check if bot knows the spell
    if (!_bot->HasSpell(spellId))
        return false;

    // Check cooldown
    if (_bot->GetSpellHistory()->HasCooldown(spellId))
        return false;

    // Check power cost
    std::vector<SpellPowerCost> costs = spellInfo->CalcPowerCost(_bot, spellInfo->GetSchoolMask());
    for (SpellPowerCost const& cost : costs)
    {
        if (_bot->GetPower(cost.Power) < cost.Amount)
            return false;
    }

    // Check range
    float range = spellInfo->GetMaxRange(false);
    if (_bot->GetDistance(target) > range)
        return false;

    // Check line of sight
    if (!_bot->IsWithinLOSInMap(target))
        return false;

    // Check target type based on creature type
    if (Creature* creature = target->ToCreature())
    {
        uint32 creatureType = creature->GetCreatureType();

        // Check spell mechanic/effect to determine which creature types it works on
        // Polymorph-like spells: work on beasts, humanoids, critters
        if (spellInfo->Mechanic == MECHANIC_POLYMORPH ||
            spellInfo->HasAura(SPELL_AURA_MOD_CONFUSE))
        {
            if (creatureType != CREATURE_TYPE_BEAST &&
                creatureType != CREATURE_TYPE_HUMANOID &&
                creatureType != CREATURE_TYPE_CRITTER)
            {
                return false;
            }
        }

        // Banish: works on demons and elementals
        if (spellInfo->Mechanic == MECHANIC_BANISH)
        {
            if (creatureType != CREATURE_TYPE_DEMON &&
                creatureType != CREATURE_TYPE_ELEMENTAL)
            {
                return false;
            }
        }

        // Shackle: works on undead
        if (spellInfo->Mechanic == MECHANIC_SHACKLE ||
            spellInfo->HasAura(SPELL_AURA_MOD_SHAPESHIFT))
        {
            if (creatureType != CREATURE_TYPE_UNDEAD)
            {
                return false;
            }
        }

        // Fear: works on humanoids and beasts (generally)
        if (spellInfo->Mechanic == MECHANIC_FEAR)
        {
            if (creatureType == CREATURE_TYPE_MECHANICAL ||
                creatureType == CREATURE_TYPE_UNDEAD ||
                creatureType == CREATURE_TYPE_ELEMENTAL)
            {
                return false;
            }
        }
    }

    // Check if target is immune to CC
    // Pass MAX_EFFECT_MASK to check all effects of the spell (API changed in 12.0.7)
    if (target->IsImmunedToSpell(spellInfo, MAX_EFFECT_MASK, _bot))
        return false;

    return true;
}

void CrowdControlManager::UpdateExpiredCCs()
{
    uint32 now = GameTime::GetGameTimeMS();

    for (auto it = _activeCCs.begin(); it != _activeCCs.end();)
    {
        if (!it->second.IsActive() || !it->second.target || it->second.target->isDead())
        {
            TC_LOG_DEBUG("playerbot", "CrowdControlManager: CC on {} expired",
                it->second.target ? it->second.target->GetName() : "unknown");
            it = _activeCCs.erase(it);
        }
        else
            ++it;
    }
}

// ============================================================================
// DIMINISHING RETURNS (DR) TRACKING - Phase 2 Architecture
// ============================================================================

// Spell ID to DR Category mapping
// This is a comprehensive list for WoW 11.x - expand as needed
DRCategory CrowdControlManager::GetDRCategory(uint32 spellId)
{
    // Map commonly used CC spells to their DR categories
    switch (spellId)
    {
        // STUN category
        case 408:       // Kidney Shot
        case 853:       // Hammer of Justice
        case 115750:    // Blinding Light
        case 108194:    // Asphyxiate
        case 5211:      // Mighty Bash
        case 119381:    // Leg Sweep
        case 91807:     // Shambling Rush
        case 30283:     // Shadowfury
        case 109248:    // Binding Shot
        case 118905:    // Static Charge
        case 197214:    // Sundering
        case 200196:    // Holy Word: Chastise
        case 179057:    // Chaos Nova
        case 1776:      // Gouge
            return DRCategory::STUN;

        // INCAPACITATE category
        case 118:       // Polymorph
        case 6770:      // Sap
        case 51514:     // Hex
        case 20066:     // Repentance
        case 2637:      // Hibernate
        case 710:       // Banish
        case 9484:      // Shackle Undead
        case 605:       // Mind Control
        case 115078:    // Paralysis
        case 187650:    // Freezing Trap
        case 19386:     // Wyvern Sting
        case 82691:     // Ring of Frost
        case 213691:    // Scatter Shot
        case 217832:    // Imprison
            return DRCategory::INCAPACITATE;

        // DISORIENT category
        case 8122:      // Psychic Scream
        case 2094:      // Blind
        case 6789:      // Mortal Coil
        case 99:        // Incapacitating Roar
        case 31661:     // Dragon's Breath
        case 207167:    // Blinding Sleet
            return DRCategory::DISORIENT;

        // FEAR category (Warlock fear specifically)
        case 5782:      // Fear
        case 118699:    // Fear (Havoc warlock version)
        case 130616:    // Fear (Pet)
            return DRCategory::FEAR;

        // HORROR category
        case 5484:      // Howl of Terror
        case 6358:      // Seduction
            return DRCategory::HORROR;

        // ROOT category
        case 122:       // Frost Nova
        case 339:       // Entangling Roots
        case 102359:    // Mass Entanglement
        case 116706:    // Disable
        case 45334:     // Feral Charge Root
        case 233395:    // Frozen Center
            return DRCategory::ROOT;

        // SILENCE category
        case 15487:     // Silence
        case 78675:     // Solar Beam
        case 47476:     // Strangulate
        case 199683:    // Last Word
            return DRCategory::SILENCE;

        // DISARM category
        case 236077:    // Disarm
            return DRCategory::DISARM;

        // KNOCKBACK category
        case 132469:    // Typhoon
        case 51490:     // Thunderstorm
        case 202138:    // Sigil of Chains
            return DRCategory::KNOCKBACK;

        default:
            return DRCategory::NONE;
    }
}

float CrowdControlManager::GetDRMultiplier(ObjectGuid target, DRCategory category) const
{
    if (category == DRCategory::NONE)
        return 1.0f;

    auto targetIt = _drTracking.find(target);
    if (targetIt == _drTracking.end())
        return 1.0f;  // No DR history = full duration

    auto categoryIt = targetIt->second.find(category);
    if (categoryIt == targetIt->second.end())
        return 1.0f;  // No DR for this category = full duration

    return categoryIt->second.GetDurationMultiplier();
}

float CrowdControlManager::GetDRMultiplier(ObjectGuid target, uint32 spellId) const
{
    DRCategory category = GetDRCategory(spellId);
    return GetDRMultiplier(target, category);
}

bool CrowdControlManager::IsDRImmune(ObjectGuid target, DRCategory category) const
{
    if (category == DRCategory::NONE)
        return false;

    auto targetIt = _drTracking.find(target);
    if (targetIt == _drTracking.end())
        return false;  // No DR history = not immune

    auto categoryIt = targetIt->second.find(category);
    if (categoryIt == targetIt->second.end())
        return false;  // No DR for this category = not immune

    return categoryIt->second.IsImmune();
}

bool CrowdControlManager::IsDRImmune(ObjectGuid target, uint32 spellId) const
{
    DRCategory category = GetDRCategory(spellId);
    return IsDRImmune(target, category);
}

uint8 CrowdControlManager::GetDRStacks(ObjectGuid target, DRCategory category) const
{
    if (category == DRCategory::NONE)
        return 0;

    auto targetIt = _drTracking.find(target);
    if (targetIt == _drTracking.end())
        return 0;

    auto categoryIt = targetIt->second.find(category);
    if (categoryIt == targetIt->second.end())
        return 0;

    return categoryIt->second.stacks;
}

void CrowdControlManager::OnCCApplied(ObjectGuid target, uint32 spellId)
{
    DRCategory category = GetDRCategory(spellId);
    OnCCApplied(target, category);
}

void CrowdControlManager::OnCCApplied(ObjectGuid target, DRCategory category)
{
    if (category == DRCategory::NONE)
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();
    _drTracking[target][category].Apply(currentTime);

    uint8 stacks = _drTracking[target][category].stacks;
    TC_LOG_DEBUG("playerbot", "CrowdControlManager: DR applied to {} (category: {}, stacks: {})",
        target.ToString(), static_cast<uint8>(category), stacks);
}

void CrowdControlManager::UpdateDR(uint32 currentTime)
{
    // Update all DR states and remove expired ones
    for (auto& targetPair : _drTracking)
    {
        for (auto categoryIt = targetPair.second.begin(); categoryIt != targetPair.second.end();)
        {
            categoryIt->second.Update(currentTime);

            // Remove if reset to 0 stacks
            if (categoryIt->second.stacks == 0 && categoryIt->second.lastApplicationTime > 0)
            {
                TC_LOG_DEBUG("playerbot", "CrowdControlManager: DR reset for {} (category: {})",
                    targetPair.first.ToString(), static_cast<uint8>(categoryIt->first));
                categoryIt = targetPair.second.erase(categoryIt);
            }
            else
            {
                ++categoryIt;
            }
        }
    }

    // Clean up targets with no DR tracking
    for (auto targetIt = _drTracking.begin(); targetIt != _drTracking.end();)
    {
        if (targetIt->second.empty())
            targetIt = _drTracking.erase(targetIt);
        else
            ++targetIt;
    }
}

void CrowdControlManager::ClearDR(ObjectGuid target)
{
    _drTracking.erase(target);
    TC_LOG_DEBUG("playerbot", "CrowdControlManager: Cleared all DR for {}", target.ToString());
}

uint32 CrowdControlManager::GetExpectedDuration(ObjectGuid target, uint32 spellId, uint32 baseDuration) const
{
    float multiplier = GetDRMultiplier(target, spellId);
    return static_cast<uint32>(baseDuration * multiplier);
}

// ============================================================================
// ICombatEventSubscriber Implementation (Phase 3 Event-Driven Architecture)
// ============================================================================

CombatEventType CrowdControlManager::GetSubscribedEventTypes() const
{
    // Subscribe to events relevant for CC tracking:
    // - AURA_APPLIED: Track when CC auras are applied
    // - AURA_REMOVED: Track when CC auras are removed/broken
    // - UNIT_DIED: Clear DR for dead units
    return CombatEventType::AURA_APPLIED |
           CombatEventType::AURA_REMOVED |
           CombatEventType::UNIT_DIED;
}

bool CrowdControlManager::ShouldReceiveEvent(const CombatEvent& event) const
{
    // For aura events, check if it's a CC aura
    if (event.type == CombatEventType::AURA_APPLIED || event.type == CombatEventType::AURA_REMOVED)
    {
        if (event.spellId == 0)
            return false;

        // Check if this is a CC spell
        return IsCCAura(event.spellId);
    }

    // For unit death, always receive to clear DR
    if (event.type == CombatEventType::UNIT_DIED)
    {
        return true;
    }

    return false;
}

void CrowdControlManager::OnCombatEvent(const CombatEvent& event)
{
    // Route event to appropriate handler based on type
    switch (event.type)
    {
        case CombatEventType::AURA_APPLIED:
            HandleAuraApplied(event);
            break;

        case CombatEventType::AURA_REMOVED:
            HandleAuraRemoved(event);
            break;

        case CombatEventType::UNIT_DIED:
            HandleUnitDied(event);
            break;

        default:
            // Ignore unhandled event types
            break;
    }
}

void CrowdControlManager::HandleAuraApplied(const CombatEvent& event)
{
    // When a CC aura is applied, track it
    if (!_bot || event.target.IsEmpty() || event.spellId == 0)
        return;

    // Get CC type from spell
    CrowdControlType ccType = GetCCTypeFromSpell(event.spellId);
    if (ccType == CrowdControlType::MAX)
        return;  // Not a CC spell

    // Find the target unit (use bot as reference for same-map lookup)
    Unit* target = ObjectAccessor::GetUnit(*_bot, event.target);
    if (!target)
        return;

    // Find who applied it (source)
    Player* appliedBy = nullptr;
    if (!event.source.IsEmpty())
    {
        appliedBy = ObjectAccessor::FindPlayer(event.source);
    }

    // Get aura duration
    uint32 duration = event.auraDuration;
    if (duration == 0)
        duration = 8000;  // Default 8 second estimate

    // Apply CC tracking
    ApplyCC(target, ccType, duration, appliedBy, event.spellId);

    // Track DR
    OnCCApplied(event.target, event.spellId);

    // Mark data as dirty
    _ccDataDirty = true;

    TC_LOG_DEBUG("playerbot", "CrowdControlManager: Event - CC applied to {} (spell: {})",
        target->GetName(), event.spellId);
}

void CrowdControlManager::HandleAuraRemoved(const CombatEvent& event)
{
    // When a CC aura is removed, stop tracking it
    if (!_bot || event.target.IsEmpty() || event.spellId == 0)
        return;

    // Find the target unit (use bot as reference for same-map lookup)
    Unit* target = ObjectAccessor::GetUnit(*_bot, event.target);
    if (!target)
        return;

    // Check if we're tracking this CC
    auto it = _activeCCs.find(event.target);
    if (it != _activeCCs.end() && it->second.spellId == event.spellId)
    {
        TC_LOG_DEBUG("playerbot", "CrowdControlManager: Event - CC removed from {} (spell: {})",
            target->GetName(), event.spellId);

        _activeCCs.erase(it);
        _ccDataDirty = true;
    }
}

void CrowdControlManager::HandleUnitDied(const CombatEvent& event)
{
    // When a unit dies, clear all tracking for that unit
    if (!event.source.IsEmpty())
    {
        // Clear DR tracking
        ClearDR(event.source);

        // Clear active CC tracking
        _activeCCs.erase(event.source);

        _ccDataDirty = true;

        TC_LOG_DEBUG("playerbots", "CrowdControlManager: Event - Unit died, cleared tracking for {}",
            event.source.ToString());
    }
}

bool CrowdControlManager::IsCCAura(uint32 spellId) const
{
    // Check if spell is a known CC spell
    // Use the DR category as a proxy - if it has a DR category, it's a CC
    DRCategory category = GetDRCategory(spellId);
    return category != DRCategory::NONE;
}

CrowdControlType CrowdControlManager::GetCCTypeFromSpell(uint32 spellId) const
{
    // Map spell to CC type based on DR category
    DRCategory category = GetDRCategory(spellId);

    switch (category)
    {
        case DRCategory::STUN:
            return CrowdControlType::STUN;

        case DRCategory::INCAPACITATE:
            return CrowdControlType::INCAPACITATE;

        case DRCategory::DISORIENT:
        case DRCategory::FEAR:
        case DRCategory::HORROR:
            return CrowdControlType::DISORIENT;

        case DRCategory::ROOT:
            return CrowdControlType::ROOT;

        case DRCategory::SILENCE:
            return CrowdControlType::SILENCE;

        case DRCategory::DISARM:
            return CrowdControlType::DISARM;

        default:
            return CrowdControlType::MAX;  // Not a CC
    }
}

} // namespace Playerbot
