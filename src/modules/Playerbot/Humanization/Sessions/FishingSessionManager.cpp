/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "FishingSessionManager.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Map.h"
#include "GameTime.h"
#include "Log.h"
#include "SpellMgr.h"
#include "Item.h"
#include "GameObject.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ObjectAccessor.h"
#include "Loot/Loot.h"
#include "SharedDefines.h"
#include <algorithm>

namespace Playerbot
{
namespace Humanization
{

// Fishing emotes for humanization
const std::vector<uint32> FishingSessionManager::FISHING_EMOTES = {
    113,    // EMOTE_ONESHOT_YAWN
    73,     // EMOTE_ONESHOT_DANCE
    23,     // EMOTE_ONESHOT_RUDE
    26,     // EMOTE_ONESHOT_CHEER
    69,     // EMOTE_ONESHOT_TALK
    71,     // EMOTE_ONESHOT_LAUGH
    94,     // EMOTE_ONESHOT_SHY
    7,      // EMOTE_ONESHOT_EAT
    14,     // EMOTE_ONESHOT_YES
    20,     // EMOTE_ONESHOT_POINT
};

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

FishingSessionManager::FishingSessionManager(Player* bot, BotAI* ai)
    : BehaviorManager(bot, ai, 1000, "FishingSessionManager")  // 1 second update
{
    std::random_device rd;
    _rng.seed(rd());
}

FishingSessionManager::~FishingSessionManager()
{
    if (_session.isActive)
    {
        EndSession("Destructor");
    }
}

// ============================================================================
// LIFECYCLE
// ============================================================================

bool FishingSessionManager::OnInitialize()
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return false;

    _session.Reset();

    TC_LOG_DEBUG("module.playerbot.humanization",
        "FishingSessionManager::OnInitialize - Bot {} fishing manager initialized",
        GetBot() ? GetBot()->GetName() : "unknown");

    return true;
}

void FishingSessionManager::OnShutdown()
{
    if (_session.isActive)
    {
        EndSession("Shutdown");
    }

    _stateCallbacks.clear();
    _catchCallbacks.clear();

    // Note: Don't access GetBot() here - it may already be destroyed during shutdown
    TC_LOG_DEBUG("module.playerbot.humanization",
        "FishingSessionManager::OnShutdown - Fishing manager shutdown complete");
}

void FishingSessionManager::OnUpdate(uint32 elapsed)
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return;

    if (!_session.isActive)
        return;

    _session.elapsedTimeMs += elapsed;

    // Check if session should end
    if (_session.elapsedTimeMs >= _session.plannedDurationMs)
    {
        EndSession("Duration complete");
        return;
    }

    // Update current state
    UpdateState(elapsed);
}

// ============================================================================
// SESSION MANAGEMENT
// ============================================================================

bool FishingSessionManager::StartSession(uint32 durationMs, FishingSpot const* spot)
{
    if (_session.isActive)
    {
        TC_LOG_DEBUG("module.playerbot.humanization",
            "FishingSessionManager::StartSession - Bot {} already in session",
            GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    if (!CanFish())
    {
        TC_LOG_DEBUG("module.playerbot.humanization",
            "FishingSessionManager::StartSession - Bot {} cannot fish (no skill/pole)",
            GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    // Select spot
    FishingSpot selectedSpot;
    if (spot && spot->IsValid())
    {
        selectedSpot = *spot;
    }
    else
    {
        selectedSpot = FindNearbySpot();
        if (!selectedSpot.IsValid())
        {
            TC_LOG_DEBUG("module.playerbot.humanization",
                "FishingSessionManager::StartSession - Bot {} found no valid fishing spot",
                GetBot() ? GetBot()->GetName() : "unknown");
            return false;
        }
    }

    // Initialize session
    _session.Reset();
    _session.isActive = true;
    _session.spot = selectedSpot;
    _session.startTimeMs = GetCurrentTimeMs();
    _session.plannedDurationMs = durationMs > 0 ? durationMs : CalculateSessionDuration();
    _session.stateStartTimeMs = GetCurrentTimeMs();

    // Start traveling to spot if not already there
    float distToSpot = GetBot()->GetDistance(selectedSpot.position);
    if (distToSpot > 5.0f)
    {
        TransitionTo(FishingSessionState::TRAVELING);
    }
    else
    {
        TransitionTo(FishingSessionState::SETTING_UP);
    }

    _statistics.totalSessions++;

    NotifyStateChange(true);

    TC_LOG_DEBUG("module.playerbot.humanization",
        "FishingSessionManager::StartSession - Bot {} started fishing session, duration: {} ms",
        GetBot() ? GetBot()->GetName() : "unknown",
        _session.plannedDurationMs);

    return true;
}

void FishingSessionManager::EndSession(std::string const& reason)
{
    if (!_session.isActive)
        return;

    // Update statistics
    _statistics.totalTimeSpentMs += _session.elapsedTimeMs;

    TC_LOG_DEBUG("module.playerbot.humanization",
        "FishingSessionManager::EndSession - Bot {} ended session: {}, casts: {}, catches: {}",
        GetBot() ? GetBot()->GetName() : "unknown",
        reason.empty() ? "none" : reason.c_str(),
        _session.castCount,
        _session.catchCount);

    // Stand up if sitting
    if (_session.isSitting)
    {
        DoStand();
    }

    NotifyStateChange(false);

    _session.Reset();
}

uint32 FishingSessionManager::GetRemainingTime() const
{
    if (!_session.isActive)
        return 0;

    if (_session.elapsedTimeMs >= _session.plannedDurationMs)
        return 0;

    return _session.plannedDurationMs - _session.elapsedTimeMs;
}

float FishingSessionManager::GetProgress() const
{
    if (!_session.isActive || _session.plannedDurationMs == 0)
        return 0.0f;

    return static_cast<float>(_session.elapsedTimeMs) /
           static_cast<float>(_session.plannedDurationMs);
}

// ============================================================================
// FISHING STATE
// ============================================================================

bool FishingSessionManager::IsFishing() const
{
    return _session.isActive &&
           (_session.state == FishingSessionState::CASTING ||
            _session.state == FishingSessionState::WAITING ||
            _session.state == FishingSessionState::LOOTING);
}

// ============================================================================
// STATE MACHINE
// ============================================================================

void FishingSessionManager::TransitionTo(FishingSessionState newState)
{
    if (_session.state == newState)
        return;

    FishingSessionState oldState = _session.state;
    _session.state = newState;
    _session.stateStartTimeMs = GetCurrentTimeMs();
    _session.stateDurationMs = 0;

    TC_LOG_DEBUG("module.playerbot.humanization",
        "FishingSessionManager::TransitionTo - Bot {} state: {} -> {}",
        GetBot() ? GetBot()->GetName() : "unknown",
        GetFishingStateName(oldState),
        GetFishingStateName(newState));
}

void FishingSessionManager::UpdateState(uint32 elapsed)
{
    switch (_session.state)
    {
        case FishingSessionState::IDLE:
            HandleIdleState(elapsed);
            break;
        case FishingSessionState::TRAVELING:
            HandleTravelingState(elapsed);
            break;
        case FishingSessionState::SETTING_UP:
            HandleSettingUpState(elapsed);
            break;
        case FishingSessionState::CASTING:
            HandleCastingState(elapsed);
            break;
        case FishingSessionState::WAITING:
            HandleWaitingState(elapsed);
            break;
        case FishingSessionState::LOOTING:
            HandleLootingState(elapsed);
            break;
        case FishingSessionState::REELING_MISS:
            HandleReelingMissState(elapsed);
            break;
        case FishingSessionState::BREAK:
            HandleBreakState(elapsed);
            break;
        case FishingSessionState::WATCHING_WATER:
            HandleWatchingWaterState(elapsed);
            break;
        case FishingSessionState::EQUIPMENT_CHECK:
            HandleEquipmentCheckState(elapsed);
            break;
        case FishingSessionState::INVENTORY_FULL:
            HandleInventoryFullState(elapsed);
            break;
        case FishingSessionState::ENDING:
            HandleEndingState(elapsed);
            break;
    }
}

void FishingSessionManager::HandleIdleState(uint32 /*elapsed*/)
{
    // In idle state, check if we should start casting
    if (!_session.isActive)
        return;

    // Check for break
    if (ShouldTakeBreak())
    {
        StartBreak();
        return;
    }

    // Check for watching water (humanization)
    if (ShouldWatchWater())
    {
        TransitionTo(FishingSessionState::WATCHING_WATER);
        _session.stateDurationMs = RandomInRange(5000, 15000);  // 5-15 seconds
        return;
    }

    // Ready to cast
    TransitionTo(FishingSessionState::CASTING);
}

void FishingSessionManager::HandleTravelingState(uint32 /*elapsed*/)
{
    if (!GetBot() || !GetAI())
        return;

    float distToSpot = GetBot()->GetDistance(_session.spot.position);

    if (distToSpot <= 5.0f)
    {
        // Arrived at spot
        TransitionTo(FishingSessionState::SETTING_UP);
        return;
    }

    // Move towards spot
    GetAI()->MoveTo(
        _session.spot.position.GetPositionX(),
        _session.spot.position.GetPositionY(),
        _session.spot.position.GetPositionZ());
}

void FishingSessionManager::HandleSettingUpState(uint32 elapsed)
{
    // Setting up: possibly sit down, face water, etc.
    uint32 stateElapsed = GetCurrentTimeMs() - _session.stateStartTimeMs;

    if (stateElapsed < 1000)  // Wait 1 second to "settle"
        return;

    // Maybe sit down
    if (!_session.isSitting && RandomFloat() < _config.sittingChance)
    {
        DoSit();
    }

    // Ready to cast
    TransitionTo(FishingSessionState::CASTING);
}

void FishingSessionManager::HandleCastingState(uint32 /*elapsed*/)
{
    // Check if we need to wait before casting
    uint32 currentTime = GetCurrentTimeMs();
    if (currentTime < _session.lastCastTimeMs + _session.nextCastDelayMs)
        return;

    // Cast fishing line
    if (CastFishingLine())
    {
        _session.lastCastTimeMs = currentTime;
        _session.castCount++;
        _statistics.totalCasts++;

        TransitionTo(FishingSessionState::WAITING);
        _session.stateDurationMs = RandomInRange(
            _config.bobberWaitMinMs, _config.bobberWaitMaxMs);
    }
    else
    {
        // Failed to cast, might be interrupted or no LOS
        _session.nextCastDelayMs = CalculateCastDelay();
    }
}

void FishingSessionManager::HandleWaitingState(uint32 elapsed)
{
    // Check for bobber
    GameObject* bobber = FindFishingBobber();

    if (bobber)
    {
        // Check if bobber has a bite (bobber state changes when fish bites)
        if (bobber->GetGoAnimProgress() > 0)
        {
            // Fish is biting! Decide if we catch or miss
            if (RandomFloat() < _config.missChance)
            {
                HandleMiss();
                TransitionTo(FishingSessionState::REELING_MISS);
                _session.stateDurationMs = RandomInRange(1000, 2000);
            }
            else
            {
                TransitionTo(FishingSessionState::LOOTING);
            }
            return;
        }
    }

    // Check if wait time exceeded (bobber despawned or no bite)
    uint32 stateElapsed = GetCurrentTimeMs() - _session.stateStartTimeMs;
    if (stateElapsed >= _session.stateDurationMs)
    {
        // No bite, prepare for next cast
        _session.nextCastDelayMs = CalculateCastDelay();
        TransitionTo(FishingSessionState::IDLE);
    }

    // Maybe emote while waiting
    if (RandomFloat() < _config.emoteChance * (elapsed / 1000.0f))
    {
        DoRandomEmote();
    }
}

void FishingSessionManager::HandleLootingState(uint32 /*elapsed*/)
{
    // Reel in the catch
    if (ReelIn())
    {
        // Successfully reeled in - HandleCatch will be called via event
        _session.nextCastDelayMs = CalculateCastDelay();
        TransitionTo(FishingSessionState::IDLE);
    }
    else
    {
        // Failed to reel in
        HandleMiss();
        TransitionTo(FishingSessionState::IDLE);
    }
}

void FishingSessionManager::HandleReelingMissState(uint32 /*elapsed*/)
{
    uint32 stateElapsed = GetCurrentTimeMs() - _session.stateStartTimeMs;

    if (stateElapsed >= _session.stateDurationMs)
    {
        // Done reacting to miss
        _session.nextCastDelayMs = CalculateCastDelay();
        TransitionTo(FishingSessionState::IDLE);
    }
}

void FishingSessionManager::HandleBreakState(uint32 /*elapsed*/)
{
    uint32 stateElapsed = GetCurrentTimeMs() - _session.stateStartTimeMs;

    if (stateElapsed >= _session.stateDurationMs)
    {
        // Break is over
        _session.isOnBreak = false;
        _session.lastBreakTimeMs = GetCurrentTimeMs();

        // Stand up if we sat during break
        if (_session.isSitting)
        {
            DoStand();
        }

        TransitionTo(FishingSessionState::IDLE);
    }
}

void FishingSessionManager::HandleWatchingWaterState(uint32 /*elapsed*/)
{
    uint32 stateElapsed = GetCurrentTimeMs() - _session.stateStartTimeMs;

    if (stateElapsed >= _session.stateDurationMs)
    {
        // Done watching, back to fishing
        TransitionTo(FishingSessionState::IDLE);
    }

    // Maybe emote
    if (RandomFloat() < 0.01f)  // 1% per update
    {
        DoRandomEmote();
    }
}

void FishingSessionManager::HandleEquipmentCheckState(uint32 /*elapsed*/)
{
    // For now, just return to idle after a short delay
    uint32 stateElapsed = GetCurrentTimeMs() - _session.stateStartTimeMs;

    if (stateElapsed >= 3000)  // 3 seconds
    {
        TransitionTo(FishingSessionState::IDLE);
    }
}

void FishingSessionManager::HandleInventoryFullState(uint32 /*elapsed*/)
{
    // Inventory full - end session
    EndSession("Inventory full");
}

void FishingSessionManager::HandleEndingState(uint32 /*elapsed*/)
{
    // Session ending cleanup
    EndSession("Session ending");
}

// ============================================================================
// FISHING ACTIONS
// ============================================================================

bool FishingSessionManager::CastFishingLine()
{
    if (!GetBot() || !GetAI())
        return false;

    // Cast fishing spell
    return GetAI()->CastSpell(FISHING_SPELL, GetBot());
}

bool FishingSessionManager::ReelIn()
{
    if (!GetBot())
        return false;

    // Find the bobber
    GameObject* bobber = FindFishingBobber();
    if (!bobber)
        return false;

    // Get loot from bobber
    Loot* loot = bobber->GetLootForPlayer(GetBot());
    if (!loot)
    {
        // Bobber has no loot, just set it to deactivate
        bobber->SetLootState(GO_JUST_DEACTIVATED);
        return false;
    }

    // Send loot to player
    GetBot()->SendLoot(*loot, false);

    // Mark bobber as done
    bobber->SetLootState(GO_JUST_DEACTIVATED);
    return true;
}

void FishingSessionManager::HandleCatch(uint32 itemId)
{
    _session.catchCount++;
    _session.caughtItems.push_back(itemId);
    _statistics.totalCatches++;

    NotifyCatch(itemId, true);

    TC_LOG_DEBUG("module.playerbot.humanization",
        "FishingSessionManager::HandleCatch - Bot {} caught item {}",
        GetBot() ? GetBot()->GetName() : "unknown",
        itemId);
}

void FishingSessionManager::HandleMiss()
{
    _session.missCount++;
    _statistics.totalMisses++;

    NotifyCatch(0, false);

    TC_LOG_DEBUG("module.playerbot.humanization",
        "FishingSessionManager::HandleMiss - Bot {} missed catch",
        GetBot() ? GetBot()->GetName() : "unknown");
}

GameObject* FishingSessionManager::FindFishingBobber() const
{
    if (!GetBot())
        return nullptr;

    // Search for bobber owned by this player
    // Bobbers are GameObjects with specific entry IDs
    std::list<GameObject*> bobbers;
    Trinity::AllGameObjectsWithEntryInRange check(GetBot(), 0, 20.0f);  // 20 yard range
    Trinity::GameObjectListSearcher<Trinity::AllGameObjectsWithEntryInRange> searcher(GetBot(), bobbers, check);
    Cell::VisitGridObjects(GetBot(), searcher, 20.0f);

    for (GameObject* go : bobbers)
    {
        if (go->GetOwnerGUID() == GetBot()->GetGUID() &&
            go->GetGoType() == GAMEOBJECT_TYPE_FISHINGNODE)
        {
            return go;
        }
    }

    return nullptr;
}

// ============================================================================
// SPOT MANAGEMENT
// ============================================================================

FishingSpot FishingSessionManager::FindNearbySpot(float maxDistance) const
{
    FishingSpot bestSpot;

    if (!GetBot())
        return bestSpot;

    // For now, use bot's current position as the spot
    // A full implementation would query a database of known fishing spots
    bestSpot.position = GetBot()->GetPosition();
    bestSpot.zoneId = GetBot()->GetZoneId();
    bestSpot.areaId = GetBot()->GetAreaId();
    bestSpot.minSkill = 1;
    bestSpot.maxSkill = 450;
    bestSpot.name = "Current Location";

    // Check if we're near water
    float waterLevel = GetBot()->GetMap()->GetWaterLevel(
        GetBot()->GetPhaseShift(),
        GetBot()->GetPositionX(),
        GetBot()->GetPositionY());

    if (waterLevel > GetBot()->GetPositionZ() - 5.0f)
    {
        bestSpot.waterLevel = waterLevel;
    }

    return bestSpot;
}

std::vector<FishingSpot> FishingSessionManager::GetSpotsInZone(uint32 zoneId) const
{
    std::vector<FishingSpot> spots;

    // Would query database for known fishing spots in zone
    // For now, return empty - fishing at current location

    return spots;
}

bool FishingSessionManager::IsSpotAppropriate(FishingSpot const& spot) const
{
    if (!spot.IsValid())
        return false;

    uint16 skill = GetFishingSkill();

    // Check if skill is high enough
    if (skill < spot.minSkill)
        return false;

    return true;
}

// ============================================================================
// PERSONALITY
// ============================================================================

void FishingSessionManager::SetPersonality(PersonalityProfile const* personality)
{
    _personality = personality;
}

float FishingSessionManager::GetDurationModifier() const
{
    if (!_personality)
        return 1.0f;

    // Casual players fish longer, hardcore players fish shorter
    return _personality->GetTraits().sessionDurationMultiplier;
}

float FishingSessionManager::GetPatienceModifier() const
{
    if (!_personality)
        return 1.0f;

    // More patient = longer delays, more relaxed
    float patience = 1.0f - _personality->GetTraits().efficiency;
    return 0.5f + patience;  // 0.5 to 1.5 multiplier
}

// ============================================================================
// HUMANIZATION
// ============================================================================

void FishingSessionManager::DoSit()
{
    if (!GetBot())
        return;

    GetBot()->SetStandState(UNIT_STAND_STATE_SIT);
    _session.isSitting = true;
}

void FishingSessionManager::DoStand()
{
    if (!GetBot())
        return;

    GetBot()->SetStandState(UNIT_STAND_STATE_STAND);
    _session.isSitting = false;
}

void FishingSessionManager::DoRandomEmote()
{
    if (!GetBot() || FISHING_EMOTES.empty())
        return;

    uint32 emoteIndex = RandomInRange(0, static_cast<uint32>(FISHING_EMOTES.size() - 1));
    GetBot()->HandleEmoteCommand(static_cast<Emote>(FISHING_EMOTES[emoteIndex]));
    _session.lastEmoteTimeMs = GetCurrentTimeMs();
}

bool FishingSessionManager::ShouldTakeBreak() const
{
    if (!_session.isActive)
        return false;

    uint32 timeSinceLastBreak = GetCurrentTimeMs() - _session.lastBreakTimeMs;

    // Too soon for another break
    if (timeSinceLastBreak < _config.breakIntervalMinMs)
        return false;

    // Probability increases over time
    float probability = static_cast<float>(timeSinceLastBreak - _config.breakIntervalMinMs) /
                        static_cast<float>(_config.breakIntervalMaxMs - _config.breakIntervalMinMs);
    probability = std::clamp(probability, 0.0f, 1.0f);

    return RandomFloat() < probability * 0.1f;  // Max 10% per check
}

void FishingSessionManager::StartBreak()
{
    _session.isOnBreak = true;
    _session.breaksTaken++;

    TransitionTo(FishingSessionState::BREAK);
    _session.stateDurationMs = CalculateBreakDuration();

    // Maybe sit during break
    if (!_session.isSitting && RandomFloat() < 0.7f)
    {
        DoSit();
    }

    TC_LOG_DEBUG("module.playerbot.humanization",
        "FishingSessionManager::StartBreak - Bot {} taking {} ms break",
        GetBot() ? GetBot()->GetName() : "unknown",
        _session.stateDurationMs);
}

bool FishingSessionManager::ShouldWatchWater() const
{
    return RandomFloat() < _config.watchWaterChance;
}

// ============================================================================
// SKILL CHECKS
// ============================================================================

uint16 FishingSessionManager::GetFishingSkill() const
{
    if (!GetBot())
        return 0;

    return GetBot()->GetSkillValue(FISHING_SKILL);
}

bool FishingSessionManager::HasFishingSkill() const
{
    return GetFishingSkill() > 0;
}

bool FishingSessionManager::HasFishingPole() const
{
    if (!GetBot())
        return false;

    // Check main hand for fishing pole
    Item* mainHand = GetBot()->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
    if (!mainHand)
        return false;

    // Fishing poles are typically class 2 (Weapon), subclass 20 (Fishing Poles)
    ItemTemplate const* itemTemplate = mainHand->GetTemplate();
    return itemTemplate && itemTemplate->GetClass() == ITEM_CLASS_WEAPON &&
           itemTemplate->GetSubClass() == ITEM_SUBCLASS_WEAPON_FISHING_POLE;
}

bool FishingSessionManager::CanFish() const
{
    return HasFishingSkill();  // Fishing pole not strictly required in modern WoW
}

// ============================================================================
// UTILITY
// ============================================================================

uint32 FishingSessionManager::CalculateSessionDuration() const
{
    uint32 baseDuration = RandomInRange(_config.minDurationMs, _config.maxDurationMs);
    return static_cast<uint32>(baseDuration * GetDurationModifier());
}

uint32 FishingSessionManager::CalculateCastDelay() const
{
    uint32 baseDelay = RandomInRange(_config.minCastDelayMs, _config.maxCastDelayMs);
    return static_cast<uint32>(baseDelay * GetPatienceModifier());
}

uint32 FishingSessionManager::CalculateBreakDuration() const
{
    return RandomInRange(_config.breakDurationMinMs, _config.breakDurationMaxMs);
}

bool FishingSessionManager::HasInventorySpace() const
{
    if (!GetBot())
        return false;

    return GetBot()->GetFreeInventorySlotCount() >= _config.minFreeSlots;
}

uint32 FishingSessionManager::GetCurrentTimeMs() const
{
    return GameTime::GetGameTimeMS();
}

uint32 FishingSessionManager::RandomInRange(uint32 min, uint32 max) const
{
    if (min >= max)
        return min;

    std::uniform_int_distribution<uint32> dist(min, max);
    return dist(_rng);
}

float FishingSessionManager::RandomFloat() const
{
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(_rng);
}

void FishingSessionManager::NotifyStateChange(bool started)
{
    for (auto const& callback : _stateCallbacks)
    {
        if (callback)
        {
            callback(_session.state, started);
        }
    }
}

void FishingSessionManager::NotifyCatch(uint32 itemId, bool success)
{
    for (auto const& callback : _catchCallbacks)
    {
        if (callback)
        {
            callback(itemId, success);
        }
    }
}

void FishingSessionManager::OnStateChange(FishingCallback callback)
{
    _stateCallbacks.push_back(callback);
}

void FishingSessionManager::OnCatch(CatchCallback callback)
{
    _catchCallbacks.push_back(callback);
}

} // namespace Humanization
} // namespace Playerbot
