/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "HumanizationManager.h"
#include "Player.h"
#include "Group.h"
#include "WorldSession.h"
#include "RestMgr.h"
#include "Map.h"
#include "Log.h"
#include "SharedDefines.h"
#include "ObjectAccessor.h"
#include "AI/BotAI.h"
#include "Core/Managers/IGameSystemsManager.h"
#include "Movement/UnifiedMovementCoordinator.h"
#include "Movement/Arbiter/MovementRequest.h"
#include "Interaction/InnkeeperInteractionManager.h"
#include "Quest/QuestHubDatabase.h"
#include "Session/BotSessionManager.h"
#include "Creature.h"
#include "Core/PlayerBotHelpers.h"
// Phase 3 Humanization sub-managers
#include "../Activities/AFKSimulator.h"
#include "../Sessions/FishingSessionManager.h"
#include <random>
#include <chrono>

namespace Playerbot
{
namespace Humanization
{

// Static global metrics
HumanizationManager::HumanizationMetrics HumanizationManager::_globalMetrics;

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

HumanizationManager::HumanizationManager(Player* bot)
    : _bot(bot)
    , _botGuid(bot ? bot->GetGUID() : ObjectGuid::Empty)
{
    _sessionManager = std::make_unique<ActivitySessionManager>(bot);

    // Phase 3 Tasks 11-12: Create humanization sub-managers
    // Note: BotAI is obtained from bot during Initialize() when it's available
}

HumanizationManager::~HumanizationManager()
{
    Shutdown();
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void HumanizationManager::Initialize()
{
    if (_initialized)
        return;

    if (!_bot || !_bot->IsInWorld())
    {
        TC_LOG_WARN("module.playerbot.humanization",
            "HumanizationManager::Initialize - Bot not ready");
        return;
    }

    // Check if humanization is enabled globally
    if (!sHumanizationConfig.IsEnabled())
    {
        _enabled = false;
        _state = HumanizationState::DISABLED;
        TC_LOG_DEBUG("module.playerbot.humanization",
            "HumanizationManager::Initialize - Humanization disabled globally");
        return;
    }

    // Initialize session manager
    _sessionManager->Initialize();

    // Phase 3 Tasks 11-12: Initialize humanization sub-managers
    // Get BotAI from the bot using the helper function
    BotAI* botAI = GetBotAI(_bot);

    // Create AFKSimulator (Task 12)
    _afkSimulator = std::make_unique<AFKSimulator>(_bot, botAI);
    if (_afkSimulator)
    {
        _afkSimulator->SetPersonality(&_sessionManager->GetPersonality());
        _afkSimulator->OnInitialize();
    }

    // Create FishingSessionManager (Task 11)
    _fishingSessionManager = std::make_unique<FishingSessionManager>(_bot, botAI);
    if (_fishingSessionManager)
    {
        _fishingSessionManager->SetPersonality(&_sessionManager->GetPersonality());
        _fishingSessionManager->OnInitialize();
    }

    // Start in idle state
    _state = HumanizationState::IDLE;
    _enabled = true;
    _initialized = true;

    TC_LOG_DEBUG("module.playerbot.humanization",
        "HumanizationManager::Initialize - Bot {} initialized with {} personality (AFK: {}, Fishing: {})",
        _bot->GetName(),
        PersonalityProfile::GetTypeName(_sessionManager->GetPersonality().GetType()),
        _afkSimulator ? "enabled" : "disabled",
        _fishingSessionManager ? "enabled" : "disabled");
}

void HumanizationManager::Update(uint32 diff)
{
    if (!_initialized || !_enabled)
        return;

    if (!_bot || !_bot->IsInWorld())
        return;

    // Throttle updates
    _updateTimer += diff;
    if (_updateTimer < UPDATE_INTERVAL)
        return;

    uint32 elapsed = _updateTimer;
    _updateTimer = 0;

    // Cancel pending AFK/break if we entered danger or grouped with humans
    if (_pendingAFK || _pendingBreak || _movingToSafeLocation)
    {
        if (IsInDanger() || IsGroupedWithHumans())
        {
            if (_pendingAFK || _pendingBreak)
            {
                TC_LOG_DEBUG("module.playerbot.humanization",
                    "HumanizationManager::Update - Bot {} canceling pending AFK/break due to danger or group",
                    _bot->GetName());
            }
            _pendingAFK = false;
            _pendingBreak = false;
            _movingToSafeLocation = false;
        }
    }

    // Update session manager
    _sessionManager->Update(elapsed);

    // Update humanization sub-managers (Phase 3 Tasks 11-12)
    if (_afkSimulator)
        _afkSimulator->OnUpdate(elapsed);
    if (_fishingSessionManager)
        _fishingSessionManager->OnUpdate(elapsed);

    // Update state machine
    UpdateStateMachine(elapsed);

    // Periodic AFK check
    _afkCheckTimer += elapsed;
    if (_afkCheckTimer >= AFK_CHECK_INTERVAL)
    {
        _afkCheckTimer = 0;
        CheckAFKTrigger();
    }

    // Check break trigger
    CheckBreakTrigger();

    // Emote check
    _lastEmoteTime += elapsed;
    if (ShouldEmote())
    {
        PerformIdleEmote();
    }
}

void HumanizationManager::Shutdown()
{
    if (!_initialized)
        return;

    // End any active AFK/break
    if (_isAFK)
        EndAFK();

    // Shutdown humanization sub-managers (Phase 3 Tasks 11-12)
    if (_afkSimulator)
    {
        _afkSimulator->OnShutdown();
        _afkSimulator.reset();
    }
    if (_fishingSessionManager)
    {
        _fishingSessionManager->OnShutdown();
        _fishingSessionManager.reset();
    }

    if (_sessionManager)
        _sessionManager->Shutdown();

    _initialized = false;
    _state = HumanizationState::DISABLED;
}

// ============================================================================
// STATE
// ============================================================================

void HumanizationManager::SetEnabled(bool enabled)
{
    _enabled = enabled;

    if (!enabled)
    {
        _state = HumanizationState::DISABLED;
        if (_isAFK)
            EndAFK();
    }
    else if (_initialized)
    {
        _state = HumanizationState::IDLE;
    }
}

bool HumanizationManager::IsOnBreak() const
{
    return _sessionManager && _sessionManager->IsOnBreak();
}

// ============================================================================
// SESSION MANAGEMENT
// ============================================================================

ActivityType HumanizationManager::GetCurrentActivity() const
{
    return _sessionManager ? _sessionManager->GetCurrentActivity() : ActivityType::NONE;
}

ActivityCategory HumanizationManager::GetCurrentCategory() const
{
    return _sessionManager ? _sessionManager->GetCurrentCategory() : ActivityCategory::IDLE;
}

bool HumanizationManager::StartActivity(ActivityType activity, uint32 durationMs)
{
    if (!_enabled || !_sessionManager)
        return false;

    // Can't start activity while AFK
    if (_isAFK)
        return false;

    bool result = _sessionManager->StartSession(activity, durationMs);

    if (result)
    {
        TransitionTo(HumanizationState::ACTIVE);
        _metrics.totalSessions++;
        _globalMetrics.totalSessions++;
    }

    return result;
}

bool HumanizationManager::RequestActivityTransition(ActivityType activity, bool immediate)
{
    if (!_enabled || !_sessionManager)
        return false;

    SessionTransitionRequest request;
    request.targetActivity = activity;
    request.targetDurationMs = 0; // Use default
    request.immediate = immediate;

    return _sessionManager->RequestTransition(request);
}

ActivityType HumanizationManager::GetRecommendedActivity() const
{
    if (!_sessionManager)
        return ActivityType::NONE;

    return _sessionManager->SelectNextActivity();
}

// ============================================================================
// PERSONALITY
// ============================================================================

PersonalityProfile const& HumanizationManager::GetPersonality() const
{
    static PersonalityProfile defaultProfile;
    return _sessionManager ? _sessionManager->GetPersonality() : defaultProfile;
}

void HumanizationManager::SetPersonality(PersonalityProfile const& profile)
{
    if (_sessionManager)
        _sessionManager->SetPersonality(profile);
}

void HumanizationManager::SetPersonalityType(PersonalityType type)
{
    if (_sessionManager)
        _sessionManager->SetPersonality(PersonalityProfile(type));
}

void HumanizationManager::RandomizePersonality()
{
    if (_sessionManager)
    {
        PersonalityProfile profile = PersonalityProfile::CreateRandomProfile();
        _sessionManager->SetPersonality(profile);

        TC_LOG_DEBUG("module.playerbot.humanization",
            "HumanizationManager::RandomizePersonality - Bot {} assigned {} personality",
            _bot ? _bot->GetName() : "unknown",
            PersonalityProfile::GetTypeName(profile.GetType()));
    }
}

// ============================================================================
// BREAK MANAGEMENT
// ============================================================================

bool HumanizationManager::StartBreak(uint32 durationMs)
{
    if (!_enabled || !_sessionManager)
        return false;

    // Safety check: Don't take breaks when grouped or in danger
    if (!CanSafelyGoIdle())
    {
        TC_LOG_DEBUG("module.playerbot.humanization",
            "HumanizationManager::StartBreak - Bot {} cannot safely take break",
            _bot ? _bot->GetName() : "unknown");
        return false;
    }

    bool result = _sessionManager->StartBreak(durationMs);

    if (result)
    {
        TransitionTo(HumanizationState::ON_BREAK);
        _metrics.totalBreaks++;
        _globalMetrics.totalBreaks++;

        TC_LOG_DEBUG("module.playerbot.humanization",
            "HumanizationManager::StartBreak - Bot {} taking break for {}ms",
            _bot ? _bot->GetName() : "unknown",
            durationMs);
    }

    return result;
}

void HumanizationManager::EndBreak()
{
    if (!_sessionManager)
        return;

    _sessionManager->EndBreak();
    _pendingBreak = false;
    _movingToSafeLocation = false;
    TransitionTo(HumanizationState::IDLE);
}

uint32 HumanizationManager::GetRemainingBreakMs() const
{
    return _sessionManager ? _sessionManager->GetRemainingBreakMs() : 0;
}

bool HumanizationManager::ShouldTakeBreak() const
{
    // Safety check first
    if (!CanSafelyGoIdle())
        return false;

    return _sessionManager ? _sessionManager->ShouldTakeBreak() : false;
}

// ============================================================================
// AFK SIMULATION
// ============================================================================

bool HumanizationManager::StartAFK(uint32 durationMs)
{
    if (!_enabled || _isAFK)
        return false;

    auto const& afkConfig = sHumanizationConfig.GetAFKConfig();
    if (!afkConfig.enabled)
        return false;

    // Safety check: Don't go AFK when grouped or in danger
    if (!CanSafelyGoIdle())
    {
        TC_LOG_DEBUG("module.playerbot.humanization",
            "HumanizationManager::StartAFK - Bot {} cannot safely go AFK",
            _bot ? _bot->GetName() : "unknown");
        return false;
    }

    // Calculate duration
    if (durationMs == 0)
    {
        durationMs = GetRandomAFKDuration();
    }

    _isAFK = true;
    _afkStartTime = std::chrono::steady_clock::now();
    _afkDurationMs = durationMs;

    TransitionTo(HumanizationState::AFK);

    _metrics.totalAFKPeriods++;
    _globalMetrics.totalAFKPeriods++;

    TC_LOG_DEBUG("module.playerbot.humanization",
        "HumanizationManager::StartAFK - Bot {} going AFK for {}ms in safe location",
        _bot ? _bot->GetName() : "unknown",
        durationMs);

    // Pause current session
    if (_sessionManager && _sessionManager->HasActiveSession())
    {
        _sessionManager->PauseSession("AFK");
    }

    return true;
}

void HumanizationManager::EndAFK()
{
    if (!_isAFK)
        return;

    auto afkDuration = std::chrono::steady_clock::now() - _afkStartTime;
    uint64 durationMs = static_cast<uint64>(
        std::chrono::duration_cast<std::chrono::milliseconds>(afkDuration).count());

    _metrics.totalAFKTimeMs += durationMs;
    _globalMetrics.totalAFKTimeMs += durationMs;

    _isAFK = false;
    _pendingAFK = false;
    _movingToSafeLocation = false;

    TC_LOG_DEBUG("module.playerbot.humanization",
        "HumanizationManager::EndAFK - Bot {} back from AFK after {}ms",
        _bot ? _bot->GetName() : "unknown",
        durationMs);

    // Resume session if was paused
    if (_sessionManager && _sessionManager->GetCurrentSession() &&
        _sessionManager->GetCurrentSession()->IsPaused())
    {
        _sessionManager->ResumeSession();
        TransitionTo(HumanizationState::ACTIVE);
    }
    else
    {
        TransitionTo(HumanizationState::IDLE);
    }
}

uint32 HumanizationManager::GetRemainingAFKMs() const
{
    if (!_isAFK)
        return 0;

    auto elapsed = std::chrono::steady_clock::now() - _afkStartTime;
    uint32 elapsedMs = static_cast<uint32>(
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());

    return elapsedMs < _afkDurationMs ? _afkDurationMs - elapsedMs : 0;
}

bool HumanizationManager::ShouldGoAFK() const
{
    if (!_enabled || _isAFK)
        return false;

    auto const& afkConfig = sHumanizationConfig.GetAFKConfig();
    if (!afkConfig.enabled)
        return false;

    // Safety check: Don't consider AFK when grouped or in danger
    if (!CanSafelyGoIdle())
        return false;

    // Check personality's AFK frequency
    auto const& personality = GetPersonality();
    float afkChance = personality.GetTraits().afkFrequency;

    // Apply time-of-day modifier
    float timeMultiplier = GetTimeOfDayMultiplier();
    afkChance *= (2.0f - timeMultiplier); // More AFK during low activity periods

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    return dist(rng) < afkChance;
}

// ============================================================================
// SAFETY CHECKS
// ============================================================================

bool HumanizationManager::IsInGroup() const
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    return _bot->GetGroup() != nullptr;
}

bool HumanizationManager::IsGroupedWithHumans() const
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    Group* group = _bot->GetGroup();
    if (!group)
        return false;

    // Check if any group member is a human player (not a bot)
    for (GroupReference const& ref : group->GetMembers())
    {
        Player* member = ref.GetSource();
        if (!member)
            continue;

        // Skip self
        if (member == _bot)
            continue;

        // Check if this member is a human player (not controlled by AI)
        // Human players don't have the bot flag in their session
        if (member->GetSession() && !member->GetSession()->IsBot())
            return true;
    }

    return false;
}

bool HumanizationManager::IsInSafeLocation() const
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    // Check if in inn/tavern (has rest flag)
    if (_bot->GetRestMgr().HasRestFlag(REST_FLAG_IN_TAVERN))
        return true;

    // Check if in city (sanctuary/safe zone)
    if (_bot->IsInSanctuary())
        return true;

    // Check if not in combat and not in dungeon/raid/battleground
    if (_bot->IsInCombat())
        return false;

    // Not safe if in dungeon, raid, or battleground instance
    if (_bot->GetMap() && _bot->GetMap()->IsDungeon())
        return false;

    if (_bot->InBattleground())
        return false;

    if (_bot->InArena())
        return false;

    // Check if in a rest area (outdoor inn vicinity)
    if (_bot->GetRestMgr().HasRestFlag(REST_FLAG_IN_CITY))
        return true;

    // Default: not in a known safe location
    return false;
}

bool HumanizationManager::IsInDanger() const
{
    if (!_bot || !_bot->IsInWorld())
        return true; // Unknown state is dangerous

    // In combat
    if (_bot->IsInCombat())
        return true;

    // Dead
    if (_bot->isDead())
        return true;

    // In dungeon/raid/battleground
    if (_bot->GetMap() && _bot->GetMap()->IsDungeon())
        return true;

    if (_bot->InBattleground())
        return true;

    if (_bot->InArena())
        return true;

    // Low health (below 30%)
    if (_bot->GetHealthPct() < 30.0f)
        return true;

    return false;
}

bool HumanizationManager::CanSafelyGoIdle() const
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    // NEVER go AFK/break when grouped with human players
    if (IsGroupedWithHumans())
    {
        TC_LOG_DEBUG("module.playerbot.humanization",
            "HumanizationManager::CanSafelyGoIdle - Bot {} is grouped with humans, cannot go idle",
            _bot->GetName());
        return false;
    }

    // Don't go idle if in any group (even all-bot groups in instances)
    if (IsInGroup())
    {
        // Exception: solo bot or in very small casual group in open world
        Group* group = _bot->GetGroup();
        if (group && (group->isRaidGroup() || group->GetMembersCount() > 2))
        {
            TC_LOG_DEBUG("module.playerbot.humanization",
                "HumanizationManager::CanSafelyGoIdle - Bot {} is in raid/large group, cannot go idle",
                _bot->GetName());
            return false;
        }
    }

    // Don't go idle if in danger
    if (IsInDanger())
    {
        TC_LOG_DEBUG("module.playerbot.humanization",
            "HumanizationManager::CanSafelyGoIdle - Bot {} is in danger, cannot go idle",
            _bot->GetName());
        return false;
    }

    // Prefer being in a safe location, but allow it if conditions are very safe
    if (!IsInSafeLocation())
    {
        // Allow if in open world with no recent combat
        // For now, require safe location
        TC_LOG_DEBUG("module.playerbot.humanization",
            "HumanizationManager::CanSafelyGoIdle - Bot {} is not in safe location",
            _bot->GetName());
        return false;
    }

    return true;
}

bool HumanizationManager::RequestMoveToSafeLocation()
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    // Already in safe location
    if (IsInSafeLocation())
        return true;

    // Cannot move if in combat or in instance
    if (IsInDanger())
        return false;

    // Get BotAI to access movement system
    BotAI* botAI = BotSessionManager::GetBotAI(_bot->GetSession());
    if (!botAI)
    {
        TC_LOG_WARN("module.playerbot.humanization",
            "HumanizationManager::RequestMoveToSafeLocation - Bot {} has no BotAI",
            _bot->GetName());
        return false;
    }

    // Get movement coordinator
    UnifiedMovementCoordinator* movementCoordinator = nullptr;
    if (botAI->GetGameSystems())
    {
        movementCoordinator = botAI->GetGameSystems()->GetMovementCoordinator();
    }

    if (!movementCoordinator)
    {
        TC_LOG_WARN("module.playerbot.humanization",
            "HumanizationManager::RequestMoveToSafeLocation - Bot {} has no movement coordinator",
            _bot->GetName());
        return false;
    }

    // Priority 1: Find nearest innkeeper (preferred safe location)
    Position targetPosition;
    std::string targetDescription;
    bool foundTarget = false;

    // Use InnkeeperInteractionManager to find nearest inn (DEADLOCK-SAFE)
    InnkeeperInteractionManager innkeeperMgr(_bot);
    Creature* nearestInnkeeper = innkeeperMgr.FindNearestInnkeeper(500.0f);

    if (nearestInnkeeper)
    {
        targetPosition = nearestInnkeeper->GetPosition();
        targetDescription = "nearest inn";
        foundTarget = true;

        TC_LOG_DEBUG("module.playerbot.humanization",
            "HumanizationManager::RequestMoveToSafeLocation - Bot {} found innkeeper at ({:.1f}, {:.1f}, {:.1f})",
            _bot->GetName(),
            targetPosition.GetPositionX(),
            targetPosition.GetPositionY(),
            targetPosition.GetPositionZ());
    }

    // Priority 2: Find nearest quest hub if no inn found
    if (!foundTarget)
    {
        QuestHub const* nearestHub = QuestHubDatabase::Instance().GetNearestQuestHub(_bot);

        if (nearestHub && nearestHub->GetDistanceFrom(_bot) < 1000.0f) // Within 1000 yards
        {
            targetPosition = nearestHub->location;
            targetDescription = "quest hub " + nearestHub->name;
            foundTarget = true;

            TC_LOG_DEBUG("module.playerbot.humanization",
                "HumanizationManager::RequestMoveToSafeLocation - Bot {} found quest hub '{}' at ({:.1f}, {:.1f}, {:.1f})",
                _bot->GetName(),
                nearestHub->name,
                targetPosition.GetPositionX(),
                targetPosition.GetPositionY(),
                targetPosition.GetPositionZ());
        }
    }

    // If no safe location found, log and return false
    if (!foundTarget)
    {
        TC_LOG_DEBUG("module.playerbot.humanization",
            "HumanizationManager::RequestMoveToSafeLocation - Bot {} could not find any safe location nearby",
            _bot->GetName());
        return false;
    }

    // Create movement request to safe location
    // Use EXPLORATION priority (lower priority, can be interrupted by combat)
    MovementRequest request = MovementRequest::MakePointMovement(
        PlayerBotMovementPriority::EXPLORATION,
        targetPosition,
        true,                   // generatePath
        {},                     // no finalOrient
        {},                     // no speed override
        3.0f,                   // closeEnoughDistance (3 yards)
        "Moving to safe location for AFK/break",
        "HumanizationManager"
    );

    // Set expected duration based on distance
    float distance = _bot->GetDistance(targetPosition);
    uint32 expectedDurationMs = static_cast<uint32>(distance / 7.0f * 1000.0f); // Assume ~7 yards/sec
    request.SetExpectedDuration(expectedDurationMs);
    request.SetAllowInterrupt(true); // Allow interruption for combat

    // Submit movement request
    bool accepted = movementCoordinator->RequestMovement(request);

    if (accepted)
    {
        TC_LOG_INFO("module.playerbot.humanization",
            "HumanizationManager::RequestMoveToSafeLocation - Bot {} moving to {} ({:.0f} yards away)",
            _bot->GetName(),
            targetDescription,
            distance);
    }
    else
    {
        TC_LOG_DEBUG("module.playerbot.humanization",
            "HumanizationManager::RequestMoveToSafeLocation - Bot {} movement request to {} was rejected",
            _bot->GetName(),
            targetDescription);
    }

    return accepted;
}

// ============================================================================
// EMOTES
// ============================================================================

bool HumanizationManager::ShouldEmote() const
{
    if (!_enabled || !sHumanizationConfig.EnableIdleEmotes())
        return false;

    // Don't emote during AFK
    if (_isAFK)
        return false;

    // Check cooldown
    if (_lastEmoteTime < _emoteCooldown)
        return false;

    // Check personality's emote frequency
    auto const& personality = GetPersonality();
    float emoteChance = personality.GetTraits().emoteFrequency;

    // Apply config frequency
    emoteChance *= sHumanizationConfig.GetEmoteFrequency();

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    return dist(rng) < emoteChance;
}

Emote HumanizationManager::GetRandomIdleEmote() const
{
    // List of idle-appropriate emotes (EMOTE_ONESHOT_* for animation emotes)
    static const std::vector<Emote> idleEmotes = {
        EMOTE_ONESHOT_TALK,
        EMOTE_ONESHOT_BOW,
        EMOTE_ONESHOT_WAVE,
        EMOTE_ONESHOT_CHEER,
        EMOTE_ONESHOT_SALUTE,
        EMOTE_ONESHOT_FLEX,
        EMOTE_ONESHOT_SHY,
        EMOTE_ONESHOT_POINT
    };

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<size_t> dist(0, idleEmotes.size() - 1);

    return idleEmotes[dist(rng)];
}

void HumanizationManager::PerformIdleEmote()
{
    if (!_bot || !_bot->IsInWorld())
        return;

    Emote emoteId = GetRandomIdleEmote();

    // Perform the emote
    _bot->HandleEmoteCommand(emoteId);

    _lastEmoteTime = 0;
    _metrics.totalEmotes++;
    _globalMetrics.totalEmotes++;

    TC_LOG_DEBUG("module.playerbot.humanization",
        "HumanizationManager::PerformIdleEmote - Bot {} performed emote {}",
        _bot->GetName(),
        emoteId);
}

// ============================================================================
// TIME-OF-DAY
// ============================================================================

float HumanizationManager::GetTimeOfDayMultiplier() const
{
    if (!sHumanizationConfig.EnableTimeOfDayVariation())
        return 1.0f;

    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&time);

    return sHumanizationConfig.GetHourlyActivityMultiplier(localTime->tm_hour);
}

bool HumanizationManager::IsLowActivityPeriod() const
{
    return GetTimeOfDayMultiplier() < 0.5f;
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

void HumanizationManager::UpdateStateMachine(uint32 diff)
{
    switch (_state)
    {
        case HumanizationState::DISABLED:
            // Nothing to do
            break;

        case HumanizationState::IDLE:
            ProcessIdleState(diff);
            break;

        case HumanizationState::ACTIVE:
            ProcessActiveState(diff);
            break;

        case HumanizationState::TRANSITIONING:
            // Session manager handles this
            if (!_sessionManager || !_sessionManager->HasActiveSession())
            {
                TransitionTo(HumanizationState::IDLE);
            }
            else
            {
                TransitionTo(HumanizationState::ACTIVE);
            }
            break;

        case HumanizationState::ON_BREAK:
            if (!IsOnBreak())
            {
                TransitionTo(HumanizationState::IDLE);
            }
            break;

        case HumanizationState::AFK:
            if (GetRemainingAFKMs() == 0)
            {
                EndAFK();
            }
            break;
    }
}

void HumanizationManager::CheckAFKTrigger()
{
    // If already AFK or moving to safe location, skip
    if (_isAFK || _movingToSafeLocation)
        return;

    // If we have a pending AFK request and reached safe location, go AFK
    if (_pendingAFK)
    {
        if (IsInSafeLocation())
        {
            _pendingAFK = false;
            _movingToSafeLocation = false;
            StartAFK();
        }
        return;
    }

    // Check if we should go AFK
    if (!ShouldGoAFK())
        return;

    // Already in safe location? Go AFK immediately
    if (IsInSafeLocation())
    {
        StartAFK();
        return;
    }

    // Not grouped with humans but not in safe location - try to move there
    if (!IsGroupedWithHumans() && !IsInDanger())
    {
        _pendingAFK = true;
        _movingToSafeLocation = RequestMoveToSafeLocation();

        if (_movingToSafeLocation)
        {
            TC_LOG_DEBUG("module.playerbot.humanization",
                "HumanizationManager::CheckAFKTrigger - Bot {} moving to safe location before going AFK",
                _bot ? _bot->GetName() : "unknown");
        }
        else
        {
            // Couldn't find a safe location to move to - cancel pending AFK
            _pendingAFK = false;
            TC_LOG_DEBUG("module.playerbot.humanization",
                "HumanizationManager::CheckAFKTrigger - Bot {} could not find safe location, skipping AFK",
                _bot ? _bot->GetName() : "unknown");
        }
    }
}

void HumanizationManager::CheckBreakTrigger()
{
    // If already on break or moving to safe location, skip
    if (IsOnBreak() || _movingToSafeLocation)
        return;

    // If we have a pending break request and reached safe location, take break
    if (_pendingBreak)
    {
        if (IsInSafeLocation())
        {
            _pendingBreak = false;
            _movingToSafeLocation = false;
            StartBreak();
        }
        return;
    }

    // Check if we should take a break
    if (!ShouldTakeBreak())
        return;

    // Already in safe location? Take break immediately
    if (IsInSafeLocation())
    {
        StartBreak();
        return;
    }

    // Not grouped with humans but not in safe location - try to move there
    if (!IsGroupedWithHumans() && !IsInDanger())
    {
        _pendingBreak = true;
        _movingToSafeLocation = RequestMoveToSafeLocation();

        if (_movingToSafeLocation)
        {
            TC_LOG_DEBUG("module.playerbot.humanization",
                "HumanizationManager::CheckBreakTrigger - Bot {} moving to safe location before taking break",
                _bot ? _bot->GetName() : "unknown");
        }
        else
        {
            // Couldn't find a safe location to move to - cancel pending break
            _pendingBreak = false;
            TC_LOG_DEBUG("module.playerbot.humanization",
                "HumanizationManager::CheckBreakTrigger - Bot {} could not find safe location, skipping break",
                _bot ? _bot->GetName() : "unknown");
        }
    }
}

void HumanizationManager::ProcessIdleState(uint32 /*diff*/)
{
    // Check if session manager started something
    if (_sessionManager && _sessionManager->HasActiveSession())
    {
        TransitionTo(HumanizationState::ACTIVE);
        return;
    }

    // Consider starting an activity
    ActivityType recommended = GetRecommendedActivity();
    if (recommended != ActivityType::NONE)
    {
        // Apply time-of-day modifier
        float timeMultiplier = GetTimeOfDayMultiplier();
        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        if (dist(rng) < timeMultiplier)
        {
            StartActivity(recommended);
        }
    }
}

void HumanizationManager::ProcessActiveState(uint32 /*diff*/)
{
    // Check if session ended
    if (!_sessionManager || !_sessionManager->HasActiveSession())
    {
        _metrics.totalActiveTimeMs += _sessionManager->GetCurrentSession() ?
            _sessionManager->GetCurrentSession()->GetElapsedMs() : 0;
        _globalMetrics.totalActiveTimeMs += _sessionManager->GetCurrentSession() ?
            _sessionManager->GetCurrentSession()->GetElapsedMs() : 0;

        TransitionTo(HumanizationState::IDLE);
    }
}

void HumanizationManager::TransitionTo(HumanizationState newState)
{
    if (_state == newState)
        return;

    TC_LOG_DEBUG("module.playerbot.humanization",
        "HumanizationManager::TransitionTo - Bot {} transitioning from {} to {}",
        _bot ? _bot->GetName() : "unknown",
        static_cast<uint8>(_state),
        static_cast<uint8>(newState));

    _state = newState;
}

uint32 HumanizationManager::GetRandomAFKDuration() const
{
    auto const& afkConfig = sHumanizationConfig.GetAFKConfig();

    // Choose AFK type based on weights
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> typeDist(0, 99);

    int roll = typeDist(rng);

    uint32 minMs, maxMs;
    if (roll < 70) // 70% short
    {
        minMs = afkConfig.shortAFKMinMs;
        maxMs = afkConfig.shortAFKMaxMs;
    }
    else if (roll < 95) // 25% medium
    {
        minMs = afkConfig.mediumAFKMinMs;
        maxMs = afkConfig.mediumAFKMaxMs;
    }
    else // 5% long
    {
        minMs = afkConfig.longAFKMinMs;
        maxMs = afkConfig.longAFKMaxMs;
    }

    std::uniform_int_distribution<uint32> durationDist(minMs, maxMs);
    return durationDist(rng);
}

} // namespace Humanization
} // namespace Playerbot
