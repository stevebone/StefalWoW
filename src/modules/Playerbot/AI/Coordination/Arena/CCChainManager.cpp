/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CCChainManager.h"
#include "ArenaCoordinator.h"
#include "AI/Coordination/Messaging/BotMessageBus.h"
#include "AI/Coordination/Messaging/BotMessage.h"
#include "AI/Combat/CrowdControlManager.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Group.h"
#include "GameTime.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot {

// ============================================================================
// CC SPELL DATABASE
// ============================================================================

struct CCSpellInfo
{
    uint32 spellId;
    CCCategory category;
    uint32 baseDuration;  // ms
    uint32 cooldown;      // ms
    float range;
    bool isMelee;
};

static const ::std::vector<CCSpellInfo> CC_SPELLS = {
    // Stuns
    { 853, CCCategory::STUN, 6000, 60000, 10.0f, true },       // Hammer of Justice
    { 408, CCCategory::STUN, 4000, 20000, 5.0f, true },        // Kidney Shot
    { 1833, CCCategory::STUN, 4000, 0, 5.0f, true },           // Cheap Shot
    { 30283, CCCategory::STUN, 5000, 30000, 30.0f, false },    // Shadowfury
    { 89766, CCCategory::STUN, 4000, 30000, 10.0f, true },     // Axe Toss
    { 179057, CCCategory::STUN, 5000, 60000, 20.0f, false },   // Chaos Nova
    { 46968, CCCategory::STUN, 4000, 40000, 8.0f, true },      // Shockwave
    { 91800, CCCategory::STUN, 3000, 45000, 30.0f, false },    // Gnaw (DK pet)
    { 99, CCCategory::STUN, 4000, 50000, 5.0f, true },         // Incapacitating Roar
    { 119381, CCCategory::STUN, 3000, 45000, 8.0f, true },     // Leg Sweep

    // Incapacitates
    { 118, CCCategory::INCAPACITATE, 60000, 0, 30.0f, false },  // Polymorph
    { 6770, CCCategory::INCAPACITATE, 60000, 0, 5.0f, true },   // Sap
    { 20066, CCCategory::INCAPACITATE, 60000, 15000, 30.0f, false }, // Repentance
    { 115078, CCCategory::INCAPACITATE, 60000, 45000, 20.0f, false }, // Paralysis
    { 1776, CCCategory::INCAPACITATE, 4000, 10000, 5.0f, true }, // Gouge
    { 2094, CCCategory::INCAPACITATE, 60000, 120000, 30.0f, false }, // Blind
    { 51514, CCCategory::INCAPACITATE, 60000, 0, 30.0f, false }, // Hex

    // Disorients
    { 31661, CCCategory::DISORIENT, 8000, 45000, 0.0f, false }, // Dragon's Breath
    { 8122, CCCategory::DISORIENT, 8000, 60000, 0.0f, false },  // Psychic Scream

    // Fears
    { 5782, CCCategory::FEAR, 20000, 0, 30.0f, false },        // Fear
    { 5484, CCCategory::FEAR, 8000, 40000, 0.0f, false },      // Howl of Terror
    { 6789, CCCategory::FEAR, 6000, 45000, 30.0f, false },     // Mortal Coil

    // Silences
    { 15487, CCCategory::SILENCE, 4000, 45000, 30.0f, false }, // Silence (Priest)
    { 1330, CCCategory::SILENCE, 3000, 15000, 5.0f, true },    // Garrote
    { 78675, CCCategory::SILENCE, 3000, 60000, 30.0f, false }, // Solar Beam

    // Roots
    { 339, CCCategory::ROOT, 30000, 0, 35.0f, false },         // Entangling Roots
    { 122, CCCategory::ROOT, 8000, 0, 0.0f, false },           // Frost Nova
    { 3355, CCCategory::ROOT, 60000, 0, 35.0f, false }         // Freezing Trap
};

// ============================================================================
// CONSTRUCTOR
// ============================================================================

CCChainManager::CCChainManager(ArenaCoordinator* coordinator, CrowdControlManager* ccManager)
    : _coordinator(coordinator)
    , _ccManager(ccManager)
{
}

void CCChainManager::Initialize()
{
    Reset();

    // Initialize CC abilities for all teammates
    for (const auto& teammate : _coordinator->GetTeammates())
    {
        LoadPlayerCCAbilities(teammate.guid);
    }

    TC_LOG_DEBUG("playerbot", "CCChainManager::Initialize - Initialized");
}

void CCChainManager::Update(uint32 diff)
{
    UpdateCooldowns(diff);

    if (_activeChain.isActive)
    {
        UpdateActiveChain(diff);
    }

    _chainUpdateTimer += diff;
}

void CCChainManager::Reset()
{
    _activeChain.Reset();
    _chainUpdateTimer = 0;
    _pendingRequests.clear();
    _playerCCAbilities.clear();
    _chainsStarted = 0;
    _chainsCompleted = 0;
    _ccApplied = 0;
    _ccBroken = 0;
}

// ============================================================================
// CHAIN MANAGEMENT
// ============================================================================

bool CCChainManager::StartChain(ObjectGuid target, bool forBurst, bool forPeel)
{
    if (_activeChain.isActive)
    {
        TC_LOG_DEBUG("playerbot", "CCChainManager::StartChain - Chain already active");
        return false;
    }

    if (!CanChainTarget(target))
    {
        TC_LOG_DEBUG("playerbot", "CCChainManager::StartChain - Cannot chain target");
        return false;
    }

    // Plan the chain
    _activeChain = PlanOptimalChain(target);
    _activeChain.isActive = true;
    _activeChain.startTime = GameTime::GetGameTimeMS();
    _activeChain.isForBurst = forBurst;
    _activeChain.isForPeel = forPeel;
    _activeChain.currentLinkIndex = 0;

    _chainsStarted++;

    TC_LOG_DEBUG("playerbot", "CCChainManager::StartChain - Started chain #%u with %zu links, expected duration %u ms",
                 _chainsStarted, _activeChain.links.size(), _activeChain.totalDuration);

    // Broadcast CC claim via BotMessageBus
    auto const& teammates = _coordinator->GetTeammates();
    if (!teammates.empty())
    {
        Player* leader = ObjectAccessor::FindPlayer(teammates.front().guid);
        if (leader && leader->GetGroup())
        {
            ObjectGuid groupGuid = leader->GetGroup()->GetGUID();
            ObjectGuid senderGuid = leader->GetGroup()->GetLeaderGUID();

            // Use the first link's spell ID for the CC claim
            uint32 ccSpellId = 0;
            if (!_activeChain.links.empty())
                ccSpellId = _activeChain.links[0].spellId;

            sBotMessageBus->Publish(BotMessage::ClaimCC(senderGuid, groupGuid, target, ccSpellId, ClaimPriority::MEDIUM));
        }
    }

    return true;
}

void CCChainManager::EndChain()
{
    if (!_activeChain.isActive)
        return;

    // Check if chain was completed
    if (_activeChain.currentLinkIndex >= _activeChain.links.size())
    {
        _chainsCompleted++;
    }

    TC_LOG_DEBUG("playerbot", "CCChainManager::EndChain - Chain ended, %zu/%zu links executed",
                 static_cast<size_t>(_activeChain.currentLinkIndex), _activeChain.links.size());

    _activeChain.Reset();
}

// ============================================================================
// CHAIN PLANNING
// ============================================================================

CCChain CCChainManager::PlanChain(ObjectGuid target, uint32 desiredDuration) const
{
    CCChain chain;
    chain.target = target;
    chain.totalDuration = 0;

    uint8 drStacks[8] = { 0 };  // Track DR per category

    while (chain.totalDuration < desiredDuration && chain.totalDuration < _maxChainDuration)
    {
        // Find best next CC
        CCLink link = SelectBestNextCC(target, 2);  // Max 2 DR stacks

        if (link.spellId == 0)
            break;  // No more CCs available

        // Check DR for this category
        uint8 categoryIndex = static_cast<uint8>(link.category);
        if (drStacks[categoryIndex] >= 2)
            continue;  // Skip immune categories

        // Calculate duration with DR
        link.drStack = drStacks[categoryIndex];
        link.expectedDuration = CalculateDRDuration(link.baseDuration, link.drStack);

        // Skip if duration too short
        if (link.expectedDuration < _minCCDuration)
            continue;

        // Schedule the link
        link.scheduledTime = chain.totalDuration > 0 ?
            chain.links.back().scheduledTime + chain.links.back().expectedDuration - static_cast<uint32>(_overlapWindow) :
            0;

        chain.links.push_back(link);
        chain.totalDuration += link.expectedDuration;

        // Increment DR
        drStacks[categoryIndex]++;
    }

    chain.expectedEndTime = chain.startTime + chain.totalDuration;

    return chain;
}

CCChain CCChainManager::PlanOptimalChain(ObjectGuid target) const
{
    // Plan for maximum duration
    return PlanChain(target, _maxChainDuration);
}

bool CCChainManager::CanChainTarget(ObjectGuid target) const
{
    const ArenaEnemy* enemy = _coordinator->GetEnemy(target);
    if (!enemy)
        return false;

    // Can't chain dead targets
    if (enemy->healthPercent <= 0)
        return false;

    // Check if any CC is available
    for (const auto& [player, abilities] : _playerCCAbilities)
    {
        for (const auto& ability : abilities)
        {
            if (ability.readyTime <= GameTime::GetGameTimeMS())
            {
                // Check if this category is immune
                if (!IsImmune(target, ability.category))
                    return true;
            }
        }
    }

    return false;
}

uint32 CCChainManager::GetMaxChainDuration(ObjectGuid target) const
{
    CCChain plannedChain = PlanOptimalChain(target);
    return plannedChain.totalDuration;
}

// ============================================================================
// DR-AWARE CC
// ============================================================================

uint32 CCChainManager::GetExpectedDuration(ObjectGuid target, uint32 spellId) const
{
    CCCategory category = GetSpellCategory(spellId);
    uint8 drStacks = GetDRStacks(target, category);

    // Find base duration
    uint32 baseDuration = 0;
    for (const auto& spell : CC_SPELLS)
    {
        if (spell.spellId == spellId)
        {
            baseDuration = spell.baseDuration;
            break;
        }
    }

    return CalculateDRDuration(baseDuration, drStacks);
}

uint8 CCChainManager::GetDRStacks(ObjectGuid target, CCCategory category) const
{
    // Use Phase 2 CrowdControlManager if available
    if (_ccManager)
    {
        // Map our category to the DR tracker's category
        // This would need proper integration
    }

    // Simplified fallback - always assume 0 stacks
    (void)target;
    (void)category;
    return 0;
}

bool CCChainManager::IsImmune(ObjectGuid target, CCCategory category) const
{
    return GetDRStacks(target, category) >= 3;
}

bool CCChainManager::WillDRExpireSoon(ObjectGuid target, CCCategory category) const
{
    // DR expires after 18 seconds
    // This would check the DR timer from the CrowdControlManager
    (void)target;
    (void)category;
    return false;
}

uint32 CCChainManager::GetTimeUntilDRReset(ObjectGuid target, CCCategory category) const
{
    // Would need DR tracking integration
    (void)target;
    (void)category;
    return 18000;  // Default 18 seconds
}

// ============================================================================
// CHAIN EXECUTION
// ============================================================================

ObjectGuid CCChainManager::GetNextCCer() const
{
    if (!_activeChain.isActive)
        return ObjectGuid::Empty;

    if (_activeChain.currentLinkIndex >= _activeChain.links.size())
        return ObjectGuid::Empty;

    return _activeChain.links[_activeChain.currentLinkIndex].caster;
}

uint32 CCChainManager::GetNextCCSpell() const
{
    if (!_activeChain.isActive)
        return 0;

    if (_activeChain.currentLinkIndex >= _activeChain.links.size())
        return 0;

    return _activeChain.links[_activeChain.currentLinkIndex].spellId;
}

uint32 CCChainManager::GetTimeUntilNextCC() const
{
    if (!_activeChain.isActive)
        return 0;

    if (_activeChain.currentLinkIndex >= _activeChain.links.size())
        return 0;

    const CCLink& link = _activeChain.links[_activeChain.currentLinkIndex];
    uint32 now = GameTime::GetGameTimeMS();
    uint32 scheduledTime = _activeChain.startTime + link.scheduledTime;

    return scheduledTime > now ? scheduledTime - now : 0;
}

void CCChainManager::OnCCApplied(ObjectGuid caster, ObjectGuid target, uint32 spellId)
{
    _ccApplied++;

    if (_activeChain.isActive && _activeChain.target == target)
    {
        // Mark current link as executed
        if (_activeChain.currentLinkIndex < _activeChain.links.size())
        {
            CCLink& link = _activeChain.links[_activeChain.currentLinkIndex];
            if (link.spellId == spellId && link.caster == caster)
            {
                OnLinkExecuted(link, true);
            }
        }
    }

    // Notify coordinator's enemy tracking
    if (ArenaEnemy* enemy = _coordinator->GetEnemyMutable(target))
    {
        enemy->isInCC = true;

        // Calculate CC end time
        uint32 duration = GetExpectedDuration(target, spellId);
        enemy->ccEndTime = GameTime::GetGameTimeMS() + duration;
    }
}

void CCChainManager::OnCCBroken(ObjectGuid target)
{
    _ccBroken++;

    if (_activeChain.isActive && _activeChain.target == target)
    {
        TC_LOG_DEBUG("playerbot", "CCChainManager::OnCCBroken - CC chain broken!");
        EndChain();
    }

    if (ArenaEnemy* enemy = _coordinator->GetEnemyMutable(target))
    {
        enemy->isInCC = false;
    }
}

void CCChainManager::OnCCExpired(ObjectGuid target)
{
    if (_activeChain.isActive && _activeChain.target == target)
    {
        // CC expired naturally - advance to next link
        AdvanceToNextLink();
    }

    if (ArenaEnemy* enemy = _coordinator->GetEnemyMutable(target))
    {
        enemy->isInCC = false;
    }
}

void CCChainManager::OnCCResisted(ObjectGuid caster, ObjectGuid target, uint32 spellId)
{
    if (_activeChain.isActive && _activeChain.target == target)
    {
        if (_activeChain.currentLinkIndex < _activeChain.links.size())
        {
            CCLink& link = _activeChain.links[_activeChain.currentLinkIndex];
            if (link.spellId == spellId && link.caster == caster)
            {
                OnLinkExecuted(link, false);
            }
        }
    }
}

// ============================================================================
// CC REQUESTS
// ============================================================================

void CCChainManager::RequestCC(ObjectGuid requester, ObjectGuid target, uint32 minDuration, uint8 priority)
{
    CCRequest request;
    request.requester = requester;
    request.target = target;
    request.requestTime = GameTime::GetGameTimeMS();
    request.desiredDurationMs = minDuration;
    request.priority = priority;
    request.isFilled = false;

    _pendingRequests.push_back(request);

    // Auto-start chain for high priority requests
    if (priority >= 2 && !_activeChain.isActive)
    {
        StartChain(target, false, false);
    }
}

void CCChainManager::FulfillRequest(const CCRequest& request)
{
    for (auto& pending : _pendingRequests)
    {
        if (pending.target == request.target && !pending.isFilled)
        {
            pending.isFilled = true;
            pending.assignedCCer = request.assignedCCer;
            pending.assignedSpellId = request.assignedSpellId;
            break;
        }
    }
}

void CCChainManager::CancelRequest(ObjectGuid target)
{
    _pendingRequests.erase(
        ::std::remove_if(_pendingRequests.begin(), _pendingRequests.end(),
            [target](const CCRequest& r) { return r.target == target; }),
        _pendingRequests.end());
}

// ============================================================================
// PLAYER CC AVAILABILITY
// ============================================================================

bool CCChainManager::HasCCAvailable(ObjectGuid player) const
{
    auto it = _playerCCAbilities.find(player);
    if (it == _playerCCAbilities.end())
        return false;

    uint32 now = GameTime::GetGameTimeMS();
    for (const auto& ability : it->second)
    {
        if (ability.readyTime <= now)
            return true;
    }

    return false;
}

::std::vector<PlayerCCAbility> CCChainManager::GetAvailableCCSpells(ObjectGuid player) const
{
    ::std::vector<PlayerCCAbility> available;

    auto it = _playerCCAbilities.find(player);
    if (it == _playerCCAbilities.end())
        return available;

    uint32 now = GameTime::GetGameTimeMS();
    for (const auto& ability : it->second)
    {
        if (ability.readyTime <= now)
        {
            available.push_back(ability);
        }
    }

    return available;
}

bool CCChainManager::CanCast(ObjectGuid player, uint32 spellId, ObjectGuid target) const
{
    // Check cooldown
    auto it = _playerCCAbilities.find(player);
    if (it == _playerCCAbilities.end())
        return false;

    for (const auto& ability : it->second)
    {
        if (ability.spellId == spellId)
        {
            if (ability.readyTime > GameTime::GetGameTimeMS())
                return false;

            // Check range and LOS
            if (!IsInRange(player, target, ability.range))
                return false;

            if (ability.requiresLOS && !HasLOS(player, target))
                return false;

            return true;
        }
    }

    return false;
}

uint32 CCChainManager::GetCooldownRemaining(ObjectGuid player, uint32 spellId) const
{
    auto it = _playerCCAbilities.find(player);
    if (it == _playerCCAbilities.end())
        return 0;

    uint32 now = GameTime::GetGameTimeMS();
    for (const auto& ability : it->second)
    {
        if (ability.spellId == spellId)
        {
            return ability.readyTime > now ? ability.readyTime - now : 0;
        }
    }

    return 0;
}

void CCChainManager::OnCooldownUsed(ObjectGuid player, uint32 spellId)
{
    auto it = _playerCCAbilities.find(player);
    if (it == _playerCCAbilities.end())
        return;

    for (auto& ability : it->second)
    {
        if (ability.spellId == spellId)
        {
            ability.readyTime = GameTime::GetGameTimeMS() + ability.cooldown;
            break;
        }
    }
}

// ============================================================================
// CC OVERLAP PREVENTION
// ============================================================================

bool CCChainManager::ShouldOverlapCC() const
{
    // Overlap slightly to prevent gaps
    return true;
}

// ============================================================================
// STATISTICS
// ============================================================================

float CCChainManager::GetChainSuccessRate() const
{
    if (_chainsStarted == 0)
        return 0.0f;

    return static_cast<float>(_chainsCompleted) / _chainsStarted;
}

// ============================================================================
// CHAIN PLANNING (PRIVATE)
// ============================================================================

void CCChainManager::LoadPlayerCCAbilities(ObjectGuid player)
{
    Player* p = ObjectAccessor::FindPlayer(player);
    if (!p)
        return;

    ::std::vector<PlayerCCAbility> abilities;

    // Check which CC spells this player has
    for (const auto& spell : CC_SPELLS)
    {
        if (p->HasSpell(spell.spellId))
        {
            PlayerCCAbility ability;
            ability.spellId = spell.spellId;
            ability.category = spell.category;
            ability.baseDuration = spell.baseDuration;
            ability.cooldown = spell.cooldown;
            ability.readyTime = 0;
            ability.range = spell.range;
            ability.requiresLOS = true;
            ability.isMelee = spell.isMelee;

            abilities.push_back(ability);
        }
    }

    _playerCCAbilities[player] = abilities;
}

void CCChainManager::UpdateCooldowns(uint32 /*diff*/)
{
    // Cooldowns are tracked by readyTime, nothing to update
}

CCLink CCChainManager::SelectBestNextCC(ObjectGuid target, uint8 maxDRStack) const
{
    CCLink best;
    float bestScore = 0.0f;

    for (const auto& [player, abilities] : _playerCCAbilities)
    {
        // Skip players not alive
        const ArenaTeammate* teammate = _coordinator->GetTeammate(player);
        if (!teammate || teammate->healthPercent <= 0)
            continue;

        for (const auto& ability : abilities)
        {
            // Check cooldown
            if (ability.readyTime > GameTime::GetGameTimeMS())
                continue;

            // Check DR
            uint8 drStacks = GetDRStacks(target, ability.category);
            if (drStacks > maxDRStack)
                continue;

            // Check range and LOS
            if (!IsInRange(player, target, ability.range))
                continue;

            // Score this CC option
            uint32 duration = CalculateDRDuration(ability.baseDuration, drStacks);
            float score = static_cast<float>(duration);

            // Prefer longer durations
            if (score > bestScore)
            {
                bestScore = score;
                best.caster = player;
                best.spellId = ability.spellId;
                best.category = ability.category;
                best.baseDuration = ability.baseDuration;
                best.drStack = drStacks;
                best.expectedDuration = duration;
            }
        }
    }

    return best;
}

ObjectGuid CCChainManager::FindBestCCer(ObjectGuid target, CCCategory category, uint8 maxDR) const
{
    for (const auto& [player, abilities] : _playerCCAbilities)
    {
        for (const auto& ability : abilities)
        {
            if (ability.category == category &&
                ability.readyTime <= GameTime::GetGameTimeMS() &&
                IsInRange(player, target, ability.range))
            {
                uint8 dr = GetDRStacks(target, category);
                if (dr <= maxDR)
                    return player;
            }
        }
    }

    return ObjectGuid::Empty;
}

// ============================================================================
// CHAIN EXECUTION (PRIVATE)
// ============================================================================

void CCChainManager::UpdateActiveChain(uint32 /*diff*/)
{
    if (!_activeChain.isActive)
        return;

    // Check if chain target is still valid
    const ArenaEnemy* target = _coordinator->GetEnemy(_activeChain.target);
    if (!target || target->healthPercent <= 0)
    {
        EndChain();
        return;
    }

    // Check if chain is complete
    if (_activeChain.currentLinkIndex >= _activeChain.links.size())
    {
        EndChain();
        return;
    }

    // Check if CC broke unexpectedly
    if (IsChainBroken())
    {
        EndChain();
        return;
    }
}

void CCChainManager::AdvanceToNextLink()
{
    _activeChain.currentLinkIndex++;

    if (_activeChain.currentLinkIndex >= _activeChain.links.size())
    {
        TC_LOG_DEBUG("playerbot", "CCChainManager: Chain complete");
        EndChain();
    }
}

void CCChainManager::OnLinkExecuted(CCLink& link, bool success)
{
    link.executed = true;
    link.successful = success;

    if (success)
    {
        AdvanceToNextLink();
    }
    else
    {
        TC_LOG_DEBUG("playerbot", "CCChainManager: CC link failed (resist/immune)");
        // Try next link anyway
        AdvanceToNextLink();
    }
}

bool CCChainManager::IsChainBroken() const
{
    if (!_activeChain.isActive)
        return true;

    // Check if target is no longer in CC when they should be
    const ArenaEnemy* target = _coordinator->GetEnemy(_activeChain.target);
    if (!target)
        return true;

    // If we're mid-chain and target is not in CC, chain is broken
    if (_activeChain.currentLinkIndex > 0 && !target->isInCC)
    {
        return true;
    }

    return false;
}

// ============================================================================
// DR CALCULATIONS
// ============================================================================

uint32 CCChainManager::CalculateDRDuration(uint32 baseDuration, uint8 drStack) const
{
    // DR reduces duration: 100% -> 50% -> 25% -> immune
    switch (drStack)
    {
        case 0: return baseDuration;
        case 1: return baseDuration / 2;
        case 2: return baseDuration / 4;
        default: return 0;  // Immune
    }
}

CCCategory CCChainManager::GetSpellCategory(uint32 spellId) const
{
    for (const auto& spell : CC_SPELLS)
    {
        if (spell.spellId == spellId)
            return spell.category;
    }

    return CCCategory::STUN;  // Default
}

// ============================================================================
// UTILITY
// ============================================================================

bool CCChainManager::IsInRange(ObjectGuid caster, ObjectGuid target, float range) const
{
    return GetDistance(caster, target) <= range;
}

bool CCChainManager::HasLOS(ObjectGuid caster, ObjectGuid target) const
{
    // Simplified - would need actual LOS check
    (void)caster;
    (void)target;
    return true;
}

float CCChainManager::GetDistance(ObjectGuid a, ObjectGuid b) const
{
    Player* pa = ObjectAccessor::FindPlayer(a);
    Player* pb = ObjectAccessor::FindPlayer(b);

    if (!pa || !pb)
        return 100.0f;  // Large distance if can't find

    return pa->GetDistance(pb);
}

} // namespace Playerbot
