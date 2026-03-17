/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotIdleBehaviorManager.h"
#include "BotMovementUtil.h"
#include "Player.h"
#include "MotionMaster.h"
#include "Log.h"
#include "Random.h"
#include "ChatTextBuilder.h"

namespace Playerbot
{

// ============================================================================
// Construction
// ============================================================================

BotIdleBehaviorManager::BotIdleBehaviorManager(Player* bot)
    : _bot(bot)
    , _lastRandomSeed(std::chrono::steady_clock::now())
{
    if (!_bot)
        throw std::invalid_argument("BotIdleBehaviorManager: bot cannot be null");

    InitializeDefaultConfigs();

    TC_LOG_DEBUG("module.playerbot.idle",
        "BotIdleBehaviorManager: Created for bot {} (GUID: {})",
        _bot->GetName(), _bot->GetGUID().ToString());
}

// ============================================================================
// Idle Behavior Control
// ============================================================================

void BotIdleBehaviorManager::SetIdleContext(IdleContext context)
{
    if (_context == context)
        return;

    IdleContext oldContext = _context;
    _context = context;

    TC_LOG_DEBUG("module.playerbot.idle",
        "BotIdleBehaviorManager: Bot {} context changed from {} to {}",
        _bot->GetName(),
        static_cast<int>(oldContext),
        static_cast<int>(context));

    // If we're already idle, restart with new context parameters
    if (_state != IdleBehaviorState::INACTIVE)
    {
        StopIdleBehavior();
        StartIdleBehavior();
    }
}

bool BotIdleBehaviorManager::StartIdleBehavior(Optional<Position> centerPos)
{
    if (!_enabled)
    {
        TC_LOG_DEBUG("module.playerbot.idle",
            "BotIdleBehaviorManager: Bot {} - idle behavior disabled",
            _bot->GetName());
        return false;
    }

    if (!_bot || !_bot->IsInWorld())
    {
        TC_LOG_DEBUG("module.playerbot.idle",
            "BotIdleBehaviorManager: Bot {} not in world, cannot start idle",
            _bot->GetName());
        return false;
    }

    if (_context == IdleContext::NONE)
    {
        TC_LOG_DEBUG("module.playerbot.idle",
            "BotIdleBehaviorManager: Bot {} has no idle context set",
            _bot->GetName());
        return false;
    }

    // Set center position
    if (centerPos.has_value())
    {
        _centerPosition = *centerPos;
    }
    else
    {
        _centerPosition.Relocate(_bot->GetPositionX(), _bot->GetPositionY(), _bot->GetPositionZ());
    }

    // Correct Z to ground
    BotMovementUtil::CorrectPositionToGround(_bot, _centerPosition);

    // Start with a pause before first wander (more natural)
    StartPause();

    TC_LOG_DEBUG("module.playerbot.idle",
        "BotIdleBehaviorManager: Bot {} started idle behavior (context: {}, center: {:.1f}, {:.1f}, {:.1f})",
        _bot->GetName(),
        static_cast<int>(_context),
        _centerPosition.GetPositionX(),
        _centerPosition.GetPositionY(),
        _centerPosition.GetPositionZ());

    return true;
}

void BotIdleBehaviorManager::StopIdleBehavior()
{
    if (_state == IdleBehaviorState::INACTIVE)
        return;

    // Stop any wandering movement
    if (_state == IdleBehaviorState::WANDERING)
    {
        BotMovementUtil::StopWanderingOrPath(_bot);
    }

    _state = IdleBehaviorState::INACTIVE;
    _currentStateDuration = 0;
    _targetStateDuration = 0;

    TC_LOG_DEBUG("module.playerbot.idle",
        "BotIdleBehaviorManager: Bot {} stopped idle behavior",
        _bot->GetName());
}

bool BotIdleBehaviorManager::IsIdleBehaviorActive() const
{
    return _state != IdleBehaviorState::INACTIVE;
}

// ============================================================================
// Update
// ============================================================================

void BotIdleBehaviorManager::Update(uint32 diff)
{
    if (!_enabled || _state == IdleBehaviorState::INACTIVE)
        return;

    if (!ShouldContinueIdleBehavior())
    {
        StopIdleBehavior();
        return;
    }

    // Update state duration
    _currentStateDuration += diff;

    // Handle state transitions
    switch (_state)
    {
        case IdleBehaviorState::PAUSED:
        {
            if (_currentStateDuration >= _targetStateDuration)
            {
                // Pause complete - maybe emote, then start wandering
                IdleBehaviorConfig const& config = _configs[static_cast<uint8>(_context)];

                if (config.canEmote && roll_chance_f(config.emoteChance * 100.0f))
                {
                    PerformIdleEmote();
                }

                StartWander();
            }
            break;
        }

        case IdleBehaviorState::WANDERING:
        {
            // Check if wandering has finished
            if (!BotMovementUtil::IsWandering(_bot))
            {
                // Wander complete - start pause
                StartPause();
            }
            else if (_currentStateDuration >= _targetStateDuration)
            {
                // Wander duration exceeded - stop and pause
                BotMovementUtil::StopWanderingOrPath(_bot);
                StartPause();
            }
            break;
        }

        case IdleBehaviorState::EMOTING:
        {
            // Emote state is brief - transition to pause
            if (_currentStateDuration >= 2000)  // 2 second emote
            {
                StartPause();
            }
            break;
        }

        case IdleBehaviorState::SITTING:
        {
            // Check if we should stand up
            if (_currentStateDuration >= _targetStateDuration)
            {
                // Stand up (TODO: implement proper stand up)
                StartPause();
            }
            break;
        }

        default:
            break;
    }
}

// ============================================================================
// Configuration
// ============================================================================

void BotIdleBehaviorManager::SetContextConfig(IdleContext context, IdleBehaviorConfig const& config)
{
    if (context == IdleContext::NONE)
        return;

    _configs[static_cast<uint8>(context)] = config;

    TC_LOG_DEBUG("module.playerbot.idle",
        "BotIdleBehaviorManager: Bot {} - updated config for context {}",
        _bot->GetName(), static_cast<int>(context));
}

IdleBehaviorConfig BotIdleBehaviorManager::GetContextConfig(IdleContext context) const
{
    if (context == IdleContext::NONE)
        return IdleBehaviorConfig{};

    return _configs[static_cast<uint8>(context)];
}

// ============================================================================
// Internal Methods
// ============================================================================

void BotIdleBehaviorManager::InitializeDefaultConfigs()
{
    // TOWN_IDLE: Relaxed wandering in town
    _configs[static_cast<uint8>(IdleContext::TOWN_IDLE)] = {
        .wanderRadius = 8.0f,
        .minWanderDuration = Milliseconds(15000),
        .maxWanderDuration = Milliseconds(45000),
        .minIdlePause = Milliseconds(5000),
        .maxIdlePause = Milliseconds(15000),
        .forceWalk = true,
        .canSitDown = true,
        .canEmote = true,
        .emoteChance = 0.15f
    };

    // GROUP_WAIT: Waiting for group members
    _configs[static_cast<uint8>(IdleContext::GROUP_WAIT)] = {
        .wanderRadius = 5.0f,
        .minWanderDuration = Milliseconds(10000),
        .maxWanderDuration = Milliseconds(30000),
        .minIdlePause = Milliseconds(3000),
        .maxIdlePause = Milliseconds(10000),
        .forceWalk = true,
        .canSitDown = false,
        .canEmote = true,
        .emoteChance = 0.1f
    };

    // QUEST_WAIT: Waiting at quest location
    _configs[static_cast<uint8>(IdleContext::QUEST_WAIT)] = {
        .wanderRadius = 4.0f,
        .minWanderDuration = Milliseconds(8000),
        .maxWanderDuration = Milliseconds(20000),
        .minIdlePause = Milliseconds(2000),
        .maxIdlePause = Milliseconds(8000),
        .forceWalk = true,
        .canSitDown = false,
        .canEmote = false,
        .emoteChance = 0.0f
    };

    // COMBAT_READY: Ready for combat, minimal movement
    _configs[static_cast<uint8>(IdleContext::COMBAT_READY)] = {
        .wanderRadius = 2.0f,
        .minWanderDuration = Milliseconds(5000),
        .maxWanderDuration = Milliseconds(10000),
        .minIdlePause = Milliseconds(8000),
        .maxIdlePause = Milliseconds(15000),
        .forceWalk = true,
        .canSitDown = false,
        .canEmote = false,
        .emoteChance = 0.0f
    };

    // FISHING: Near fishing spot
    _configs[static_cast<uint8>(IdleContext::FISHING)] = {
        .wanderRadius = 3.0f,
        .minWanderDuration = Milliseconds(10000),
        .maxWanderDuration = Milliseconds(30000),
        .minIdlePause = Milliseconds(30000),
        .maxIdlePause = Milliseconds(120000),  // Long pauses for fishing
        .forceWalk = true,
        .canSitDown = true,
        .canEmote = true,
        .emoteChance = 0.05f
    };

    // GUARD_PATROL: Random patrol
    _configs[static_cast<uint8>(IdleContext::GUARD_PATROL)] = {
        .wanderRadius = 15.0f,
        .minWanderDuration = Milliseconds(20000),
        .maxWanderDuration = Milliseconds(60000),
        .minIdlePause = Milliseconds(5000),
        .maxIdlePause = Milliseconds(15000),
        .forceWalk = true,
        .canSitDown = false,
        .canEmote = false,
        .emoteChance = 0.0f
    };

    // INSTANCE_WAIT: Limited movement inside instance
    _configs[static_cast<uint8>(IdleContext::INSTANCE_WAIT)] = {
        .wanderRadius = 3.0f,
        .minWanderDuration = Milliseconds(5000),
        .maxWanderDuration = Milliseconds(15000),
        .minIdlePause = Milliseconds(5000),
        .maxIdlePause = Milliseconds(20000),
        .forceWalk = true,
        .canSitDown = false,
        .canEmote = false,
        .emoteChance = 0.0f
    };

    // REST_AREA: Resting at inn/city
    _configs[static_cast<uint8>(IdleContext::REST_AREA)] = {
        .wanderRadius = 5.0f,
        .minWanderDuration = Milliseconds(10000),
        .maxWanderDuration = Milliseconds(30000),
        .minIdlePause = Milliseconds(30000),
        .maxIdlePause = Milliseconds(90000),
        .forceWalk = true,
        .canSitDown = true,
        .canEmote = true,
        .emoteChance = 0.2f
    };
}

bool BotIdleBehaviorManager::StartWander()
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    IdleBehaviorConfig const& config = _configs[static_cast<uint8>(_context)];

    // Calculate wander duration
    Milliseconds duration = GetRandomDuration(config.minWanderDuration, config.maxWanderDuration);

    // Start wandering using TrinityCore 12.0's MoveRandom for players
    bool success = BotMovementUtil::MoveRandomAroundPosition(
        _bot,
        _centerPosition,
        config.wanderRadius,
        duration,
        config.forceWalk);

    if (success)
    {
        _state = IdleBehaviorState::WANDERING;
        _currentStateDuration = 0;
        _targetStateDuration = static_cast<uint32>(duration.count());
        ++_totalWanders;

        TC_LOG_DEBUG("module.playerbot.idle",
            "BotIdleBehaviorManager: Bot {} started wandering (radius: {:.1f}yd, duration: {}ms)",
            _bot->GetName(), config.wanderRadius, duration.count());
    }

    return success;
}

void BotIdleBehaviorManager::StartPause()
{
    IdleBehaviorConfig const& config = _configs[static_cast<uint8>(_context)];

    // Calculate pause duration
    Milliseconds duration = GetRandomDuration(config.minIdlePause, config.maxIdlePause);

    _state = IdleBehaviorState::PAUSED;
    _currentStateDuration = 0;
    _targetStateDuration = static_cast<uint32>(duration.count());

    TC_LOG_DEBUG("module.playerbot.idle",
        "BotIdleBehaviorManager: Bot {} pausing for {}ms",
        _bot->GetName(), duration.count());
}

void BotIdleBehaviorManager::PerformIdleEmote()
{
    if (!_bot || !_bot->IsInWorld())
        return;

    // List of idle emotes
    static const uint32 idleEmotes[] = {
        EMOTE_ONESHOT_TALK,
        EMOTE_ONESHOT_BOW,
        EMOTE_ONESHOT_WAVE,
        EMOTE_ONESHOT_CHEER,
        EMOTE_ONESHOT_SALUTE,
        EMOTE_ONESHOT_FLEX,
        EMOTE_ONESHOT_SHY,
        EMOTE_ONESHOT_POINT
    };

    // Pick random emote
    uint32 emoteIndex = urand(0, sizeof(idleEmotes) / sizeof(idleEmotes[0]) - 1);
    uint32 emote = idleEmotes[emoteIndex];

    _bot->HandleEmoteCommand(emote);

    _state = IdleBehaviorState::EMOTING;
    _currentStateDuration = 0;
    _targetStateDuration = 2000;  // 2 seconds for emote
    ++_totalEmotes;

    TC_LOG_DEBUG("module.playerbot.idle",
        "BotIdleBehaviorManager: Bot {} performed emote {}",
        _bot->GetName(), emote);
}

Milliseconds BotIdleBehaviorManager::GetRandomDuration(Milliseconds min, Milliseconds max)
{
    if (min >= max)
        return min;

    uint32 minMs = static_cast<uint32>(min.count());
    uint32 maxMs = static_cast<uint32>(max.count());

    return Milliseconds(urand(minMs, maxMs));
}

bool BotIdleBehaviorManager::ShouldContinueIdleBehavior() const
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    // Don't idle during combat
    if (_bot->IsInCombat())
        return false;

    // Don't idle while dead
    if (_bot->isDead())
        return false;

    // Don't idle while casting
    if (_bot->IsNonMeleeSpellCast(false))
        return false;

    // Don't idle while stunned/incapacitated
    if (_bot->HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING))
        return false;

    // Don't idle while mounted (let mount behavior handle this)
    if (_bot->IsMounted())
        return false;

    // Don't idle while in a vehicle
    if (_bot->GetVehicle())
        return false;

    // Check if too far from center (something moved us)
    float distFromCenter = _bot->GetExactDist2d(_centerPosition.GetPositionX(), _centerPosition.GetPositionY());
    IdleBehaviorConfig const& config = _configs[static_cast<uint8>(_context)];
    if (distFromCenter > config.wanderRadius * 3.0f)
    {
        TC_LOG_DEBUG("module.playerbot.idle",
            "BotIdleBehaviorManager: Bot {} too far from idle center ({:.1f}yd > {:.1f}yd limit)",
            _bot->GetName(), distFromCenter, config.wanderRadius * 3.0f);
        return false;
    }

    return true;
}

} // namespace Playerbot
