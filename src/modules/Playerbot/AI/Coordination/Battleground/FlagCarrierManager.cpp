/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "FlagCarrierManager.h"
#include "BattlegroundCoordinator.h"
#include "Player.h"
#include "Group.h"
#include "Log.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "AI/Coordination/Messaging/BotMessageBus.h"
#include "AI/Coordination/Messaging/BotMessage.h"

namespace Playerbot {

// ============================================================================
// WSG Flag Debuff Constants (Alliance/Horde specific)
// ============================================================================
static constexpr uint32 FOCUSED_ASSAULT = 46392;        // Stacks up damage taken
static constexpr uint32 BRUTAL_ASSAULT = 46393;         // Higher stacks version
static constexpr float DAMAGE_PER_STACK = 0.10f;        // 10% per stack
static constexpr float HEALING_REDUCTION_PER_STACK = 0.10f;

// ============================================================================
// CLASS CONSTANTS FOR FC SUITABILITY
// ============================================================================
static constexpr uint8 CLASS_WARRIOR = 1;
static constexpr uint8 CLASS_PALADIN = 2;
static constexpr uint8 CLASS_HUNTER = 3;
static constexpr uint8 CLASS_ROGUE = 4;
static constexpr uint8 CLASS_PRIEST = 5;
static constexpr uint8 CLASS_DEATH_KNIGHT = 6;
static constexpr uint8 CLASS_SHAMAN = 7;
static constexpr uint8 CLASS_MAGE = 8;
static constexpr uint8 CLASS_WARLOCK = 9;
static constexpr uint8 CLASS_MONK = 10;
static constexpr uint8 CLASS_DRUID = 11;
static constexpr uint8 CLASS_DEMON_HUNTER = 12;
static constexpr uint8 CLASS_EVOKER = 13;

// ============================================================================
// CONSTRUCTOR
// ============================================================================

FlagCarrierManager::FlagCarrierManager(BattlegroundCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void FlagCarrierManager::Initialize()
{
    Reset();

    TC_LOG_DEBUG("playerbots.bg", "FlagCarrierManager::Initialize - Initialized for CTF battleground");
}

void FlagCarrierManager::Update(uint32 diff)
{
    // Update escort distances
    UpdateEscortDistances();

    // Cleanup invalid assignments
    CleanupInvalidAssignments();

    // Check if FC needs more escorts
    if (HasFriendlyFC() && NeedsMoreEscorts())
    {
        // BattlegroundCoordinator should handle assigning more escorts
    }

    // Check if FC is in danger
    if (HasFriendlyFC() && IsFCInDanger())
    {
        TC_LOG_DEBUG("playerbots.bg", "FlagCarrierManager::Update - FC is in danger! Health: %.1f%%, Attackers: %u",
            GetFCStatus().healthPercent, GetFCStatus().attackerCount);
    }

    // Check debuff stacks
    if (HasFriendlyFC() && IsFCDebuffCritical())
    {
        TC_LOG_DEBUG("playerbots.bg", "FlagCarrierManager::Update - FC debuff stacks critical: %u",
            GetFCDebuffStacks());
    }
}

void FlagCarrierManager::Reset()
{
    _friendlyFlag = FlagInfo();
    _enemyFlag = FlagInfo();
    _escorts.clear();
    _hunters.clear();
    _defenders.clear();

    TC_LOG_DEBUG("playerbots.bg", "FlagCarrierManager::Reset - Reset all flag state");
}

// ============================================================================
// FLAG STATE
// ============================================================================

FlagState FlagCarrierManager::GetFriendlyFlagState() const
{
    if (!_friendlyFlag.carrierGuid.IsEmpty())
        return FlagState::CARRIED;
    if (_friendlyFlag.isDropped)
        return FlagState::DROPPED;
    return FlagState::AT_BASE;
}

FlagState FlagCarrierManager::GetEnemyFlagState() const
{
    if (!_enemyFlag.carrierGuid.IsEmpty())
        return FlagState::CARRIED;
    if (_enemyFlag.isDropped)
        return FlagState::DROPPED;
    return FlagState::AT_BASE;
}

FlagCarrierStatus FlagCarrierManager::GetFCStatus() const
{
    FlagCarrierStatus status;

    if (!HasFriendlyFC())
        return status;

    status.carrier = GetFriendlyFC();

    // Get FC player
    Player* fc = _coordinator->GetPlayer(status.carrier);
    if (!fc)
        return status;

    // Health
    status.healthPercent = fc->GetHealthPct();

    // Check attackers
    status.attackerCount = 0;
    status.isUnderAttack = fc->IsInCombat();

    if (Unit* victim = fc->GetVictim())
    {
        // FC is being attacked
        status.attackerCount = 1;
    }

    // Count players attacking FC
    for (ObjectGuid guid : _coordinator->GetEnemyPlayers())
    {
        if (Player* enemy = _coordinator->GetPlayer(guid))
        {
            if (enemy->GetVictim() == fc)
                status.attackerCount++;
        }
    }

    status.isUnderAttack = (status.attackerCount > 0);

    // Distance to capture
    status.distanceToCapture = GetDistanceToFriendlyBase();

    // Debuff stacks
    status.debuffStacks = GetFCDebuffStacks();

    // Escorts
    status.escortCount = static_cast<uint8>(_escorts.size());
    status.hasEscorts = (status.escortCount > 0);

    return status;
}

bool FlagCarrierManager::IsFCInDanger() const
{
    if (!HasFriendlyFC())
        return false;

    FlagCarrierStatus status = GetFCStatus();

    // Low health
    if (status.healthPercent < 40.0f)
        return true;

    // Multiple attackers
    if (status.attackerCount >= 2)
        return true;

    // Under attack with no escorts
    if (status.isUnderAttack && !status.hasEscorts)
        return true;

    // Critical debuff stacks
    if (IsFCDebuffCritical())
        return true;

    return false;
}

bool FlagCarrierManager::IsFCNearCapture() const
{
    if (!HasFriendlyFC())
        return false;

    return GetDistanceToFriendlyBase() <= _captureRange;
}

// ============================================================================
// FLAG EVENTS
// ============================================================================

void FlagCarrierManager::OnFlagPickedUp(ObjectGuid player, bool isEnemyFlag)
{
    if (isEnemyFlag)
    {
        // One of our team picked up enemy flag
        _enemyFlag.carrierGuid = player;
        _enemyFlag.isDropped = false;

        TC_LOG_DEBUG("playerbots.bg", "FlagCarrierManager::OnFlagPickedUp - Friendly team picked up enemy flag");
    }
    else
    {
        // Enemy picked up our flag
        _friendlyFlag.carrierGuid = player;
        _friendlyFlag.isDropped = false;

        TC_LOG_DEBUG("playerbots.bg", "FlagCarrierManager::OnFlagPickedUp - Enemy picked up friendly flag");

        // Broadcast focus target on the enemy flag carrier so bots prioritize them
        auto friendlyPlayers = _coordinator->GetFriendlyPlayers();
        if (!friendlyPlayers.empty())
        {
            Player* leader = _coordinator->GetPlayer(friendlyPlayers.front());
            if (leader && leader->GetGroup())
            {
                ObjectGuid groupGuid = leader->GetGroup()->GetGUID();
                BotMessage msg = BotMessage::CommandFocusTarget(leader->GetGUID(), groupGuid, player);
                sBotMessageBus->Publish(msg);
            }
        }
    }
}

void FlagCarrierManager::OnFlagDropped(ObjectGuid player, float x, float y, float z)
{
    bool wasEnemyFC = (_enemyFlag.carrierGuid == player);
    bool wasFriendlyFC = (_friendlyFlag.carrierGuid == player);

    if (wasEnemyFC)
    {
        _enemyFlag.carrierGuid.Clear();
        _enemyFlag.isDropped = true;
        _enemyFlag.droppedPosition.x = x;
        _enemyFlag.droppedPosition.y = y;
        _enemyFlag.droppedPosition.z = z;
        _enemyFlag.dropTime = 0; // Will tick up

        TC_LOG_DEBUG("playerbots.bg", "FlagCarrierManager::OnFlagDropped - Friendly FC dropped enemy flag at (%.1f, %.1f, %.1f)",
            x, y, z);
    }

    if (wasFriendlyFC)
    {
        _friendlyFlag.carrierGuid.Clear();
        _friendlyFlag.isDropped = true;
        _friendlyFlag.droppedPosition.x = x;
        _friendlyFlag.droppedPosition.y = y;
        _friendlyFlag.droppedPosition.z = z;
        _friendlyFlag.dropTime = 0;

        TC_LOG_DEBUG("playerbots.bg", "FlagCarrierManager::OnFlagDropped - EFC dropped friendly flag at (%.1f, %.1f, %.1f)",
            x, y, z);
    }
}

void FlagCarrierManager::OnFlagCaptured(ObjectGuid player)
{
    if (_enemyFlag.carrierGuid == player)
    {
        // Our team capped
        _enemyFlag = FlagInfo();
        _friendlyFlag = FlagInfo();

        // Clear escorts since flag is capped
        _escorts.clear();

        TC_LOG_DEBUG("playerbots.bg", "FlagCarrierManager::OnFlagCaptured - Friendly team captured!");
    }
    else if (_friendlyFlag.carrierGuid == player)
    {
        // Enemy capped
        _friendlyFlag = FlagInfo();
        _enemyFlag = FlagInfo();

        TC_LOG_DEBUG("playerbots.bg", "FlagCarrierManager::OnFlagCaptured - Enemy team captured");
    }
}

void FlagCarrierManager::OnFlagReturned(ObjectGuid player)
{
    // Friendly flag returned to base
    _friendlyFlag = FlagInfo();
    _hunters.clear();

    TC_LOG_DEBUG("playerbots.bg", "FlagCarrierManager::OnFlagReturned - Friendly flag returned to base");
}

void FlagCarrierManager::OnFlagReset(bool isEnemyFlag)
{
    if (isEnemyFlag)
    {
        _enemyFlag = FlagInfo();
        _escorts.clear();
    }
    else
    {
        _friendlyFlag = FlagInfo();
        _hunters.clear();
    }

    TC_LOG_DEBUG("playerbots.bg", "FlagCarrierManager::OnFlagReset - %s flag reset",
        isEnemyFlag ? "Enemy" : "Friendly");
}

// ============================================================================
// FLAG CARRIER SELECTION
// ============================================================================

ObjectGuid FlagCarrierManager::GetBestFCCandidate() const
{
    ObjectGuid bestCandidate;
    float bestScore = 0.0f;

    for (ObjectGuid guid : _coordinator->GetFriendlyPlayers())
    {
        if (!IsValidFC(guid))
            continue;

        float score = GetFCSuitabilityScore(guid);
        if (score > bestScore)
        {
            bestScore = score;
            bestCandidate = guid;
        }
    }

    return bestCandidate;
}

float FlagCarrierManager::GetFCSuitabilityScore(ObjectGuid player) const
{
    if (!IsValidFC(player))
        return 0.0f;

    float score = 0.0f;

    // Weight: Health (30%), Class (30%), Mobility (20%), Survivability (20%)
    score += ScoreFCHealth(player) * 0.30f;
    score += ScoreFCClass(player) * 0.30f;
    score += ScoreFCMobility(player) * 0.20f;
    score += ScoreFCSurvivability(player) * 0.20f;

    return score;
}

bool FlagCarrierManager::ShouldPickUpFlag(ObjectGuid player) const
{
    // Already have FC
    if (HasFriendlyFC())
        return false;

    // Check if player is best candidate
    ObjectGuid bestCandidate = GetBestFCCandidate();
    if (!bestCandidate.IsEmpty() && bestCandidate != player)
    {
        // Only pick up if score is within 20% of best
        float bestScore = GetFCSuitabilityScore(bestCandidate);
        float playerScore = GetFCSuitabilityScore(player);

        if (playerScore < bestScore * 0.8f)
            return false;
    }

    return IsValidFC(player);
}

// ============================================================================
// ESCORT MANAGEMENT
// ============================================================================

void FlagCarrierManager::AssignEscort(ObjectGuid escort)
{
    if (!IsValidEscort(escort))
        return;

    // Check if already escorting
    for (const auto& assignment : _escorts)
    {
        if (assignment.escort == escort)
            return;
    }

    // Check max escorts
    if (_escorts.size() >= _maxEscortCount)
        return;

    EscortAssignment assignment;
    assignment.escort = escort;
    assignment.flagCarrier = GetFriendlyFC();
    assignment.assignTime = 0;
    assignment.distanceToFC = GetDistanceToFC(escort);

    _escorts.push_back(assignment);

    TC_LOG_DEBUG("playerbots.bg", "FlagCarrierManager::AssignEscort - Assigned escort, total: %zu",
        _escorts.size());
}

void FlagCarrierManager::UnassignEscort(ObjectGuid escort)
{
    auto it = std::remove_if(_escorts.begin(), _escorts.end(),
        [escort](const EscortAssignment& a) { return a.escort == escort; });

    if (it != _escorts.end())
    {
        _escorts.erase(it, _escorts.end());
        TC_LOG_DEBUG("playerbots.bg", "FlagCarrierManager::UnassignEscort - Removed escort, remaining: %zu",
            _escorts.size());
    }
}

std::vector<ObjectGuid> FlagCarrierManager::GetEscorts() const
{
    std::vector<ObjectGuid> escorts;
    escorts.reserve(_escorts.size());

    for (const auto& assignment : _escorts)
        escorts.push_back(assignment.escort);

    return escorts;
}

uint32 FlagCarrierManager::GetEscortCount() const
{
    return static_cast<uint32>(_escorts.size());
}

bool FlagCarrierManager::NeedsMoreEscorts() const
{
    if (!HasFriendlyFC())
        return false;

    return _escorts.size() < GetIdealEscortCount();
}

uint32 FlagCarrierManager::GetIdealEscortCount() const
{
    if (!HasFriendlyFC())
        return 0;

    FlagCarrierStatus status = GetFCStatus();

    // Base escort count
    uint32 count = _idealEscortCount;

    // More escorts if under attack
    if (status.isUnderAttack)
        count += 1;

    // More escorts if low health
    if (status.healthPercent < 50.0f)
        count += 1;

    // More escorts if high debuff stacks
    if (status.debuffStacks >= 5)
        count += 1;

    return std::min(count, _maxEscortCount);
}

ObjectGuid FlagCarrierManager::GetBestEscortCandidate() const
{
    if (!HasFriendlyFC())
        return ObjectGuid();

    ObjectGuid bestCandidate;
    float bestScore = 0.0f;

    for (ObjectGuid guid : _coordinator->GetFriendlyPlayers())
    {
        if (!IsValidEscort(guid))
            continue;

        // Skip if already escorting
        bool alreadyEscorting = false;
        for (const auto& assignment : _escorts)
        {
            if (assignment.escort == guid)
            {
                alreadyEscorting = true;
                break;
            }
        }
        if (alreadyEscorting)
            continue;

        // Score based on distance and class
        float distance = GetDistanceToFC(guid);
        float distanceScore = std::max(0.0f, 100.0f - distance);

        // Healers preferred as escorts
        Player* player = _coordinator->GetPlayer(guid);
        float classScore = 50.0f;
        if (player)
        {
            uint8 playerClass = player->GetClass();
            if (playerClass == CLASS_PRIEST || playerClass == CLASS_PALADIN ||
                playerClass == CLASS_DRUID || playerClass == CLASS_SHAMAN ||
                playerClass == CLASS_MONK || playerClass == CLASS_EVOKER)
            {
                classScore = 80.0f;
            }
        }

        float score = distanceScore * 0.6f + classScore * 0.4f;

        if (score > bestScore)
        {
            bestScore = score;
            bestCandidate = guid;
        }
    }

    return bestCandidate;
}

// ============================================================================
// CAPTURE TIMING
// ============================================================================

bool FlagCarrierManager::CanCapture() const
{
    if (!HasFriendlyFC())
        return false;

    // Can only capture if friendly flag is at base
    return GetFriendlyFlagState() == FlagState::AT_BASE;
}

bool FlagCarrierManager::ShouldCapture() const
{
    if (!CanCapture())
        return false;

    // Always capture if FC is in danger
    if (IsFCInDanger())
        return true;

    // Always capture if debuff stacks are high
    if (GetFCDebuffStacks() >= _criticalDebuffStacks)
        return true;

    return true; // Default: capture when possible
}

bool FlagCarrierManager::ShouldWaitForFriendlyFlag() const
{
    if (!HasFriendlyFC())
        return false;

    // If enemy has our flag, might want to wait for return
    if (GetFriendlyFlagState() == FlagState::CARRIED)
    {
        // Wait if FC is healthy and debuff stacks are low
        FlagCarrierStatus status = GetFCStatus();
        if (status.healthPercent > 70.0f && status.debuffStacks < 5)
            return true;
    }

    return false;
}

uint32 FlagCarrierManager::GetEstimatedCaptureTime() const
{
    if (!HasFriendlyFC())
        return 0;

    float distance = GetDistanceToFriendlyBase();

    // Assume average speed of ~8 yards per second
    return static_cast<uint32>(distance / 8.0f * 1000.0f);
}

// ============================================================================
// EFC HUNTING
// ============================================================================

void FlagCarrierManager::AssignHunter(ObjectGuid hunter)
{
    if (!IsValidHunter(hunter))
        return;

    // Check if already hunting
    for (ObjectGuid h : _hunters)
    {
        if (h == hunter)
            return;
    }

    _hunters.push_back(hunter);

    TC_LOG_DEBUG("playerbots.bg", "FlagCarrierManager::AssignHunter - Assigned hunter, total: %zu",
        _hunters.size());
}

void FlagCarrierManager::UnassignHunter(ObjectGuid hunter)
{
    auto it = std::remove(_hunters.begin(), _hunters.end(), hunter);
    if (it != _hunters.end())
    {
        _hunters.erase(it, _hunters.end());
        TC_LOG_DEBUG("playerbots.bg", "FlagCarrierManager::UnassignHunter - Removed hunter, remaining: %zu",
            _hunters.size());
    }
}

std::vector<ObjectGuid> FlagCarrierManager::GetHunters() const
{
    return _hunters;
}

uint32 FlagCarrierManager::GetHunterCount() const
{
    return static_cast<uint32>(_hunters.size());
}

bool FlagCarrierManager::NeedsMoreHunters() const
{
    if (!HasEnemyFC())
        return false;

    return _hunters.size() < _idealHunterCount;
}

ObjectGuid FlagCarrierManager::GetBestHunterCandidate() const
{
    if (!HasEnemyFC())
        return ObjectGuid();

    ObjectGuid bestCandidate;
    float bestScore = 0.0f;

    for (ObjectGuid guid : _coordinator->GetFriendlyPlayers())
    {
        if (!IsValidHunter(guid))
            continue;

        // Skip if already hunting
        bool alreadyHunting = std::find(_hunters.begin(), _hunters.end(), guid) != _hunters.end();
        if (alreadyHunting)
            continue;

        // Score based on distance and class
        float distance = GetDistanceToEFC(guid);
        float distanceScore = std::max(0.0f, 100.0f - distance);

        // DPS classes preferred for hunting
        Player* player = _coordinator->GetPlayer(guid);
        float classScore = 50.0f;
        if (player)
        {
            uint8 playerClass = player->GetClass();
            if (playerClass == CLASS_ROGUE || playerClass == CLASS_WARRIOR ||
                playerClass == CLASS_DEATH_KNIGHT || playerClass == CLASS_DEMON_HUNTER)
            {
                classScore = 80.0f;
            }
        }

        float score = distanceScore * 0.7f + classScore * 0.3f;

        if (score > bestScore)
        {
            bestScore = score;
            bestCandidate = guid;
        }
    }

    return bestCandidate;
}

float FlagCarrierManager::GetEFCThreatLevel() const
{
    if (!HasEnemyFC())
        return 0.0f;

    Player* efc = _coordinator->GetPlayer(GetEnemyFC());
    if (!efc)
        return 0.0f;

    float threat = 0.0f;

    // Base threat from health
    threat += efc->GetHealthPct();

    // Reduced threat if we have hunters on them
    threat -= static_cast<float>(_hunters.size()) * 15.0f;

    // Increased threat based on position (closer to cap = higher threat)
    float distanceToCap = GetDistanceToFriendlyBase();
    if (distanceToCap < 50.0f)
        threat += 50.0f;
    else if (distanceToCap < 100.0f)
        threat += 25.0f;

    return std::clamp(threat, 0.0f, 100.0f);
}

// ============================================================================
// FLAG DEFENSE
// ============================================================================

void FlagCarrierManager::AssignDefender(ObjectGuid defender)
{
    if (std::find(_defenders.begin(), _defenders.end(), defender) != _defenders.end())
        return;

    _defenders.push_back(defender);
}

void FlagCarrierManager::UnassignDefender(ObjectGuid defender)
{
    auto it = std::remove(_defenders.begin(), _defenders.end(), defender);
    if (it != _defenders.end())
        _defenders.erase(it, _defenders.end());
}

std::vector<ObjectGuid> FlagCarrierManager::GetDefenders() const
{
    return _defenders;
}

bool FlagCarrierManager::IsFlagUndefended() const
{
    return _defenders.size() < _minDefenderCount;
}

bool FlagCarrierManager::ShouldReturnToDefense(ObjectGuid player) const
{
    // If flag is undefended and player is not critical elsewhere
    if (!IsFlagUndefended())
        return false;

    // Don't pull from escorts if FC is in danger
    if (HasFriendlyFC() && IsFCInDanger())
        return false;

    // Check if player is escort
    for (const auto& escort : _escorts)
    {
        if (escort.escort == player)
            return false;
    }

    return true;
}

// ============================================================================
// DROPPED FLAG
// ============================================================================

bool FlagCarrierManager::IsFriendlyFlagDropped() const
{
    return _friendlyFlag.isDropped;
}

bool FlagCarrierManager::IsEnemyFlagDropped() const
{
    return _enemyFlag.isDropped;
}

void FlagCarrierManager::GetDroppedFlagPosition(bool isEnemy, float& x, float& y, float& z) const
{
    const FlagInfo& flag = isEnemy ? _enemyFlag : _friendlyFlag;
    x = flag.droppedPosition.x;
    y = flag.droppedPosition.y;
    z = flag.droppedPosition.z;
}

uint32 FlagCarrierManager::GetDroppedFlagTimer(bool isEnemy) const
{
    return isEnemy ? _enemyFlag.dropTime : _friendlyFlag.dropTime;
}

bool FlagCarrierManager::ShouldPickUpDroppedFlag(ObjectGuid player) const
{
    if (IsEnemyFlagDropped())
    {
        // Should pick up our dropped flag if we're valid FC
        return ShouldPickUpFlag(player);
    }

    return false;
}

ObjectGuid FlagCarrierManager::GetClosestToDroppedFlag(bool isEnemy) const
{
    const FlagInfo& flag = isEnemy ? _enemyFlag : _friendlyFlag;
    if (!flag.isDropped)
        return ObjectGuid();

    ObjectGuid closest;
    float closestDistance = std::numeric_limits<float>::max();

    for (ObjectGuid guid : _coordinator->GetFriendlyPlayers())
    {
        Player* player = _coordinator->GetPlayer(guid);
        if (!player || !player->IsAlive())
            continue;

        float dist = player->GetDistance(flag.droppedPosition.x,
            flag.droppedPosition.y, flag.droppedPosition.z);

        if (dist < closestDistance)
        {
            closestDistance = dist;
            closest = guid;
        }
    }

    return closest;
}

// ============================================================================
// DEBUFF TRACKING
// ============================================================================

uint8 FlagCarrierManager::GetFCDebuffStacks() const
{
    if (!HasFriendlyFC())
        return 0;

    Player* fc = _coordinator->GetPlayer(GetFriendlyFC());
    if (!fc)
        return 0;

    // Check for flag debuffs
    if (Aura* aura = fc->GetAura(FOCUSED_ASSAULT))
        return static_cast<uint8>(aura->GetStackAmount());

    if (Aura* aura = fc->GetAura(BRUTAL_ASSAULT))
        return static_cast<uint8>(aura->GetStackAmount());

    return 0;
}

float FlagCarrierManager::GetFCDamageTakenMultiplier() const
{
    uint8 stacks = GetFCDebuffStacks();
    return 1.0f + (stacks * DAMAGE_PER_STACK);
}

float FlagCarrierManager::GetFCHealingReceivedMultiplier() const
{
    uint8 stacks = GetFCDebuffStacks();
    return 1.0f - (stacks * HEALING_REDUCTION_PER_STACK);
}

bool FlagCarrierManager::IsFCDebuffCritical() const
{
    return GetFCDebuffStacks() >= _criticalDebuffStacks;
}

// ============================================================================
// FC SUITABILITY SCORING (PRIVATE)
// ============================================================================

float FlagCarrierManager::ScoreFCHealth(ObjectGuid player) const
{
    Player* p = _coordinator->GetPlayer(player);
    if (!p)
        return 0.0f;

    // Score based on current health and max health
    float healthPct = p->GetHealthPct();
    float maxHealth = static_cast<float>(p->GetMaxHealth());

    // Normalize max health (assume 500k is reference)
    float healthPool = std::min(maxHealth / 500000.0f, 1.5f) * 50.0f;
    float currentHealth = healthPct;

    return healthPool + currentHealth * 0.5f;
}

float FlagCarrierManager::ScoreFCClass(ObjectGuid player) const
{
    Player* p = _coordinator->GetPlayer(player);
    if (!p)
        return 0.0f;

    // FC tier list based on survivability and mobility
    switch (p->GetClass())
    {
        case CLASS_DRUID:       return 100.0f; // Travel form, high mobility
        case CLASS_MONK:        return 90.0f;  // Roll, transcendence
        case CLASS_DEMON_HUNTER:return 85.0f;  // Double jump, dash
        case CLASS_PALADIN:     return 80.0f;  // Bubbles, LoH
        case CLASS_DEATH_KNIGHT:return 75.0f;  // High survivability
        case CLASS_WARRIOR:     return 70.0f;  // Intervene, high armor
        case CLASS_SHAMAN:      return 60.0f;  // Ghost wolf
        case CLASS_HUNTER:      return 55.0f;  // Disengage
        case CLASS_ROGUE:       return 50.0f;  // Stealth can't carry
        case CLASS_WARLOCK:     return 45.0f;  // Gateway utility
        case CLASS_MAGE:        return 40.0f;  // Blink, ice block
        case CLASS_PRIEST:      return 35.0f;  // Body and Soul
        case CLASS_EVOKER:      return 50.0f;  // Hover
        default:                return 50.0f;
    }
}

float FlagCarrierManager::ScoreFCMobility(ObjectGuid player) const
{
    Player* p = _coordinator->GetPlayer(player);
    if (!p)
        return 0.0f;

    // Base mobility score
    float score = 50.0f;

    // Check for speed buffs/abilities
    if (p->HasAuraType(SPELL_AURA_MOD_INCREASE_SPEED))
        score += 20.0f;

    if (p->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED))
        score += 10.0f; // Not as useful in BG

    return std::min(score, 100.0f);
}

float FlagCarrierManager::ScoreFCSurvivability(ObjectGuid player) const
{
    Player* p = _coordinator->GetPlayer(player);
    if (!p)
        return 0.0f;

    float score = 50.0f;

    // Armor contribution
    float armor = static_cast<float>(p->GetArmor());
    score += std::min(armor / 50000.0f * 20.0f, 20.0f);

    // Defensive cooldowns available (simplified check)
    // Real implementation would check specific abilities

    return std::min(score, 100.0f);
}

// ============================================================================
// POSITION TRACKING (PRIVATE)
// ============================================================================

float FlagCarrierManager::GetDistanceToEnemyBase() const
{
    // This would need actual map data for enemy flag room position
    // Placeholder implementation
    if (!HasFriendlyFC())
        return 0.0f;

    Player* fc = _coordinator->GetPlayer(GetFriendlyFC());
    if (!fc)
        return 0.0f;

    // Would calculate distance to enemy flag capture point
    return 100.0f; // Placeholder
}

float FlagCarrierManager::GetDistanceToFriendlyBase() const
{
    if (!HasFriendlyFC())
        return 0.0f;

    Player* fc = _coordinator->GetPlayer(GetFriendlyFC());
    if (!fc)
        return 0.0f;

    // Would calculate distance to friendly flag capture point
    return 100.0f; // Placeholder
}

float FlagCarrierManager::GetDistanceToFC(ObjectGuid player) const
{
    if (!HasFriendlyFC())
        return std::numeric_limits<float>::max();

    Player* fc = _coordinator->GetPlayer(GetFriendlyFC());
    Player* p = _coordinator->GetPlayer(player);

    if (!fc || !p)
        return std::numeric_limits<float>::max();

    return fc->GetDistance(p);
}

float FlagCarrierManager::GetDistanceToEFC(ObjectGuid player) const
{
    if (!HasEnemyFC())
        return std::numeric_limits<float>::max();

    Player* efc = _coordinator->GetPlayer(GetEnemyFC());
    Player* p = _coordinator->GetPlayer(player);

    if (!efc || !p)
        return std::numeric_limits<float>::max();

    return efc->GetDistance(p);
}

// ============================================================================
// UTILITY (PRIVATE)
// ============================================================================

void FlagCarrierManager::UpdateEscortDistances()
{
    for (auto& assignment : _escorts)
    {
        assignment.distanceToFC = GetDistanceToFC(assignment.escort);
    }
}

void FlagCarrierManager::CleanupInvalidAssignments()
{
    // Clean up escorts
    auto escortIt = std::remove_if(_escorts.begin(), _escorts.end(),
        [this](const EscortAssignment& a) { return !IsValidEscort(a.escort); });
    _escorts.erase(escortIt, _escorts.end());

    // Clean up hunters
    auto hunterIt = std::remove_if(_hunters.begin(), _hunters.end(),
        [this](ObjectGuid g) { return !IsValidHunter(g); });
    _hunters.erase(hunterIt, _hunters.end());

    // Clean up defenders
    auto defenderIt = std::remove_if(_defenders.begin(), _defenders.end(),
        [this](ObjectGuid g) {
            Player* p = _coordinator->GetPlayer(g);
            return !p || !p->IsAlive();
        });
    _defenders.erase(defenderIt, _defenders.end());
}

bool FlagCarrierManager::IsValidFC(ObjectGuid player) const
{
    Player* p = _coordinator->GetPlayer(player);
    if (!p || !p->IsAlive())
        return false;

    // Can't carry if already FC
    if (GetFriendlyFC() == player)
        return false;

    // Rogues in stealth can't pick up
    if (p->HasStealthAura())
        return false;

    return true;
}

bool FlagCarrierManager::IsValidEscort(ObjectGuid player) const
{
    Player* p = _coordinator->GetPlayer(player);
    if (!p || !p->IsAlive())
        return false;

    // Can't escort if you're the FC
    if (GetFriendlyFC() == player)
        return false;

    return true;
}

bool FlagCarrierManager::IsValidHunter(ObjectGuid player) const
{
    Player* p = _coordinator->GetPlayer(player);
    if (!p || !p->IsAlive())
        return false;

    // Can't hunt if you're the FC
    if (GetFriendlyFC() == player)
        return false;

    return true;
}

} // namespace Playerbot
