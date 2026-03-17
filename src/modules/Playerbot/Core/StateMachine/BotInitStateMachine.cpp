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

#include "BotInitStateMachine.h"
#include "GameTime.h"
#include "BotAI.h"
#include "Group.h"
#include "World.h"
#include "Log.h"
#include "Timer.h"

namespace Playerbot::StateMachine
{

// ========================================================================
// CONSTRUCTION & DESTRUCTION
// ========================================================================

BotInitStateMachine::BotInitStateMachine(Player* bot)
    : BotStateMachine(bot, BotInitState::CREATED, TransitionPolicy::STRICT)
    , m_startTime(::std::chrono::steady_clock::now())
{
    TC_LOG_DEBUG("module.playerbot.statemachine",
        "BotInitStateMachine created for bot {}",
        bot ? bot->GetName() : "NULL");

    // Set initial flags
    SetFlags(StateFlags::INITIALIZING);
}

BotInitStateMachine::~BotInitStateMachine()
{
    TC_LOG_DEBUG("module.playerbot.statemachine",
        "BotInitStateMachine destroyed for bot {}",
        GetBot() ? GetBot()->GetName() : "NULL");
}

// ========================================================================
// INITIALIZATION CONTROL
// ========================================================================

bool BotInitStateMachine::Start()
{
    if (GetCurrentState() != BotInitState::CREATED)
    {
        TC_LOG_WARNING("module.playerbot.statemachine",
            "Cannot start initialization - already started (current state: {})",
            ToString(GetCurrentState()));
        return false;
    }

    m_startTime = ::std::chrono::steady_clock::now();

    TC_LOG_INFO("module.playerbot.statemachine",
        "Starting initialization sequence for bot {}",
        GetBot() ? GetBot()->GetName() : "NULL");

    auto result = TransitionTo(
        BotInitState::LOADING_CHARACTER,
        "Starting bot initialization sequence"
    );

    return result.IsValid();
}

void BotInitStateMachine::Update(uint32 diff)
{
    // Call base class update
    OnUpdate(diff);

    // If already ready or failed, nothing to do
    if (IsReady() || HasFailed())
        return;

    // Check for overall timeout
    auto now = ::std::chrono::steady_clock::now();
    auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
        now - m_startTime
    ).count();

    if (elapsed > INIT_TIMEOUT_MS)
    {
        TC_LOG_ERROR("module.playerbot.statemachine",
            "Bot {} initialization timeout after {}ms",
            GetBot() ? GetBot()->GetName() : "NULL", elapsed);

        TransitionTo(BotInitState::FAILED, "Initialization timeout");
        return;
    }

    // Check for per-state timeout
    auto stateElapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
        now - m_stateEntryTime
    ).count();

    if (stateElapsed > STATE_TIMEOUT_MS && GetCurrentState() != BotInitState::CREATED)
    {
        TC_LOG_WARNING("module.playerbot.statemachine",
            "Bot {} stuck in state {} for {}ms",
            GetBot() ? GetBot()->GetName() : "NULL",
            ToString(GetCurrentState()), stateElapsed);
    }

    // State-specific update logic
    BotInitState current = GetCurrentState();
    bool readyToAdvance = false;

    switch (current)
    {
        case BotInitState::LOADING_CHARACTER:
            readyToAdvance = HandleLoadingCharacter();
            if (readyToAdvance)
            {
                TransitionTo(BotInitState::IN_WORLD, "Character data loaded");
            }
            break;

        case BotInitState::IN_WORLD:
            readyToAdvance = HandleInWorld();
            if (readyToAdvance)
            {
                TransitionTo(BotInitState::CHECKING_GROUP, "Bot is in world");
            }
            break;

        case BotInitState::CHECKING_GROUP:
            readyToAdvance = HandleCheckingGroup();
            if (readyToAdvance)
            {
                TransitionTo(BotInitState::ACTIVATING_STRATEGIES, "Group check complete");
            }
            break;

        case BotInitState::ACTIVATING_STRATEGIES:
            readyToAdvance = HandleActivatingStrategies();
            if (readyToAdvance)
            {
                TransitionTo(BotInitState::READY, "Initialization complete");
                m_readyTime = ::std::chrono::steady_clock::now();
            }
            break;

        case BotInitState::READY:
            // Initialization complete, nothing to do
            break;

        case BotInitState::FAILED:
            HandleFailed();
            break;

        default:
            TC_LOG_ERROR("module.playerbot.statemachine",
                "Bot {} in unknown state: {}",
                GetBot() ? GetBot()->GetName() : "NULL",
                static_cast<int>(current));
            break;
    }
}

bool BotInitStateMachine::IsReady() const
{
    return GetCurrentState() == BotInitState::READY;
}

bool BotInitStateMachine::HasFailed() const
{
    return GetCurrentState() == BotInitState::FAILED;
}

float BotInitStateMachine::GetProgress() const
{
    BotInitState state = GetCurrentState();

    switch (state)
    {
        case BotInitState::CREATED:               return 0.0f;
        case BotInitState::LOADING_CHARACTER:     return 0.2f;
        case BotInitState::IN_WORLD:              return 0.4f;
        case BotInitState::CHECKING_GROUP:        return 0.6f;
        case BotInitState::ACTIVATING_STRATEGIES: return 0.8f;
        case BotInitState::READY:                 return 1.0f;
        case BotInitState::FAILED:                return 0.0f;
        default:                                  return 0.0f;
    }
}

bool BotInitStateMachine::Retry()
{
    if (GetCurrentState() != BotInitState::FAILED)
    {
        TC_LOG_WARNING("module.playerbot.statemachine",
            "Cannot retry - not in FAILED state (current: {})",
            ToString(GetCurrentState()));
        return false;
    }

    if (m_failedAttempts >= MAX_RETRY_ATTEMPTS)
    {
        TC_LOG_ERROR("module.playerbot.statemachine",
            "Max retry attempts ({}) exceeded for bot {}",
            MAX_RETRY_ATTEMPTS, GetBot() ? GetBot()->GetName() : "NULL");
        return false;
    }

    TC_LOG_INFO("module.playerbot.statemachine",
        "Retrying initialization for bot {} (attempt {}/{})",
        GetBot() ? GetBot()->GetName() : "NULL",
        m_failedAttempts + 1, MAX_RETRY_ATTEMPTS);

    // Reset state data
    m_characterDataLoaded = false;
    m_addedToWorld = false;
    m_groupChecked = false;
    m_strategiesActivated = false;
    m_wasInGroupAtLogin = false;
    m_groupLeaderGuid = ObjectGuid::Empty;

    // Reset to CREATED and start again
    ForceTransition(BotInitState::CREATED, "Retrying initialization");
    return Start();
}

// ========================================================================
// STATE-SPECIFIC QUERIES
// ========================================================================

bool BotInitStateMachine::IsBotInWorld() const
{
    BotInitState state = GetCurrentState();
    return state == BotInitState::IN_WORLD ||
           state == BotInitState::CHECKING_GROUP ||
           state == BotInitState::ACTIVATING_STRATEGIES ||
           state == BotInitState::READY;
}

bool BotInitStateMachine::HasCheckedGroup() const
{
    BotInitState state = GetCurrentState();
    return state == BotInitState::CHECKING_GROUP ||
           state == BotInitState::ACTIVATING_STRATEGIES ||
           state == BotInitState::READY;
}

bool BotInitStateMachine::HasActivatedStrategies() const
{
    BotInitState state = GetCurrentState();
    return state == BotInitState::ACTIVATING_STRATEGIES ||
           state == BotInitState::READY;
}

uint32 BotInitStateMachine::GetInitializationTime() const
{
    auto now = IsReady() ? m_readyTime : ::std::chrono::steady_clock::now();
    auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
        now - m_startTime
    ).count();
    return static_cast<uint32>(elapsed);
}

// ========================================================================
// STATE TRANSITION CALLBACKS
// ========================================================================

void BotInitStateMachine::OnEnter(BotInitState newState, BotInitState previousState)
{
    m_stateEntryTime = ::std::chrono::steady_clock::now();

    TC_LOG_DEBUG("module.playerbot.statemachine",
        "Bot {} entering state {} from {}",
        GetBot() ? GetBot()->GetName() : "NULL",
        ToString(newState), ToString(previousState));

    switch (newState)
    {
        case BotInitState::LOADING_CHARACTER:
            TC_LOG_INFO("module.playerbot.statemachine",
                "Loading character data for bot {}",
                GetBot() ? GetBot()->GetName() : "NULL");
            break;

        case BotInitState::IN_WORLD:
            TC_LOG_INFO("module.playerbot.statemachine",
                "Bot {} added to world, verifying state",
                GetBot() ? GetBot()->GetName() : "NULL");
            break;

        case BotInitState::CHECKING_GROUP:
            TC_LOG_INFO("module.playerbot.statemachine",
                "Checking group membership for bot {}",
                GetBot() ? GetBot()->GetName() : "NULL");
            break;

        case BotInitState::ACTIVATING_STRATEGIES:
            TC_LOG_INFO("module.playerbot.statemachine",
                "Activating strategies for bot {}",
                GetBot() ? GetBot()->GetName() : "NULL");
            break;

        case BotInitState::READY:
            {
                ClearFlags(StateFlags::INITIALIZING);
                SetFlags(StateFlags::READY | StateFlags::SAFE_TO_UPDATE);

                auto elapsed = GetInitializationTime();
                TC_LOG_INFO("module.playerbot.statemachine",
                    "Bot {} initialization complete in {}ms",
                    GetBot() ? GetBot()->GetName() : "NULL", elapsed);
            }
            break;

        case BotInitState::FAILED:
            {
                m_failedAttempts++;
                ClearFlags(StateFlags::INITIALIZING);
                SetFlags(StateFlags::ERROR_STATE);

                TC_LOG_ERROR("module.playerbot.statemachine",
                    "Bot {} initialization failed (attempt {}): {}",
                    GetBot() ? GetBot()->GetName() : "NULL",
                    m_failedAttempts, m_lastErrorReason);
            }
            break;

        default:
            break;
    }
}

void BotInitStateMachine::OnExit(BotInitState currentState, BotInitState nextState)
{
    TC_LOG_DEBUG("module.playerbot.statemachine",
        "Bot {} exiting state {} to {}",
        GetBot() ? GetBot()->GetName() : "NULL",
        ToString(currentState), ToString(nextState));
}

void BotInitStateMachine::OnTransitionFailed(
    BotInitState from,
    BotInitState to,
    TransitionValidation result)
{
    TC_LOG_WARNING("module.playerbot.statemachine",
        "Bot {} failed to transition from {} to {}: {} ({})",
        GetBot() ? GetBot()->GetName() : "NULL",
        ToString(from), ToString(to),
        ToString(result.result), result.reason);

    m_lastErrorReason = ::std::string(result.reason);
    m_lastErrorTime = GameTime::GetGameTimeMS();
}

void BotInitStateMachine::OnUpdate(uint32 diff)
{
    // Base class handles state machine updates
    // Derived class handles state-specific logic in Update()
}

// ========================================================================
// STATE HANDLERS
// ========================================================================

bool BotInitStateMachine::HandleLoadingCharacter()
{
    Player* bot = GetBot();
    if (!bot)
    {
        TC_LOG_ERROR("module.playerbot.statemachine",
            "Bot is null during LOADING_CHARACTER state");
        TransitionTo(BotInitState::FAILED, "Bot pointer is null");
        return false;
    }

    // Check if character data is loaded
    // In TrinityCore, this happens automatically when Player object is created
    // We just verify the bot has valid data
    if (bot->GetGUID().IsEmpty())
    {
        TC_LOG_DEBUG("module.playerbot.statemachine",
            "Bot {} still loading character data...",
            bot->GetName());
        return false; // Still loading
    }

    m_characterDataLoaded = true;
    TC_LOG_DEBUG("module.playerbot.statemachine",
        "Character data loaded for bot {}",
        bot->GetName());
    return true; // Ready to proceed to IN_WORLD
}

bool BotInitStateMachine::HandleInWorld()
{
    Player* bot = GetBot();
    if (!bot)
    {
        TC_LOG_ERROR("module.playerbot.statemachine",
            "Bot is null during IN_WORLD state");
        TransitionTo(BotInitState::FAILED, "Bot pointer is null");
        return false;
    }

    // THIS IS THE KEY CHECK: Bot must be IsInWorld() before we proceed
    if (!bot->IsInWorld())
    {
        TC_LOG_DEBUG("module.playerbot.statemachine",
            "Bot {} waiting to be added to world...",
            bot->GetName());
        return false; // Not in world yet
    }

    m_addedToWorld = true;
    TC_LOG_INFO("module.playerbot.statemachine",
        "Bot {} is now in world (IsInWorld() = true)",
        bot->GetName());
    return true; // Ready to proceed to CHECKING_GROUP
}

bool BotInitStateMachine::HandleCheckingGroup()
{
    Player* bot = GetBot();
    if (!bot || !bot->IsInWorld())
    {
        // This should NEVER happen due to state machine preconditions
        TC_LOG_ERROR("module.playerbot.statemachine",
            "Bot {} is not in world during CHECKING_GROUP state!",
            bot ? bot->GetName() : "NULL");
        TransitionTo(BotInitState::FAILED, "Bot not in world during group check");
        return false;
    }

    // NOW IT'S SAFE: Bot is guaranteed to be IsInWorld()
    // This is THE FIX for Issue #1
    Group* group = bot->GetGroup();
    if (group)
    {
        m_wasInGroupAtLogin = true;
        m_groupLeaderGuid = group->GetLeaderGUID();

        TC_LOG_INFO("module.playerbot.statemachine",
            "Bot {} is already in group at login (leader: {})",
            bot->GetName(), m_groupLeaderGuid.ToString());
        // Additional validation
    if (Player* leader = ObjectAccessor::FindPlayer(m_groupLeaderGuid))
        {
            TC_LOG_DEBUG("module.playerbot.statemachine",
                "Group leader {} is online",
                leader->GetName());
        }
        else
        {
            TC_LOG_WARNING("module.playerbot.statemachine",
                "Group leader {} is not online",
                m_groupLeaderGuid.ToString());
        }
    }
    else
    {
        m_wasInGroupAtLogin = false;
        TC_LOG_DEBUG("module.playerbot.statemachine",
            "Bot {} is not in a group",
            bot->GetName());
    }

    m_groupChecked = true;
    return true; // Ready to proceed to ACTIVATING_STRATEGIES
}

bool BotInitStateMachine::HandleActivatingStrategies()
{
    Player* bot = GetBot();
    BotAI* ai = GetBotAI();

    if (!bot || !ai)
    {
        TC_LOG_ERROR("module.playerbot.statemachine",
            "Bot or AI is null during ACTIVATING_STRATEGIES");
        TransitionTo(BotInitState::FAILED, "Bot or AI is null");
        return false;
    }

    // If bot was in group at login, NOW call OnGroupJoined()
    // This is AFTER IsInWorld() check, so strategy activation will work
    if (m_wasInGroupAtLogin)
    {
        Group* group = GetBotGroup();
        if (group)
        {
            TC_LOG_INFO("module.playerbot.statemachine",
                "Activating group strategies for bot {} (FIX FOR ISSUE #1)",
                bot->GetName());

            // This is the proper timing - bot is fully initialized
            ai->OnGroupJoined(group);

            // Verify follow strategy was activated if there's a leader
    if (Player* leader = ObjectAccessor::FindPlayer(m_groupLeaderGuid))
            {
                if (leader != bot)
                {
                    TC_LOG_DEBUG("module.playerbot.statemachine",
                        "Bot {} should now be following leader {}",
                        bot->GetName(), leader->GetName());
                }
            }
        }
        else
        {
            TC_LOG_WARNING("module.playerbot.statemachine",
                "Bot {} was in group but group no longer exists",
                bot->GetName());
        }
    }

    // Activate other base strategies (idle, etc.)
    ai->ActivateBaseStrategies();

    m_strategiesActivated = true;

    TC_LOG_INFO("module.playerbot.statemachine",
        "All strategies activated for bot {}",
        bot->GetName());

    return true; // Ready to transition to READY
}

bool BotInitStateMachine::HandleFailed()
{
    // Log detailed failure information
    Player* bot = GetBot();
    TC_LOG_ERROR("module.playerbot.statemachine",
        "Bot {} initialization failed at state {}: {}",
        bot ? bot->GetName() : "NULL",
        ToString(GetPreviousState()),
        m_lastErrorReason);

    // Could implement auto-retry logic here if desired
    if (m_failedAttempts < MAX_RETRY_ATTEMPTS)
    {
        TC_LOG_INFO("module.playerbot.statemachine",
            "Bot {} can be retried ({}/{} attempts used)",
            bot ? bot->GetName() : "NULL",
            m_failedAttempts, MAX_RETRY_ATTEMPTS);
    }

    return false; // Stay in FAILED state
}

// ========================================================================
// HELPER METHODS
// ========================================================================

BotAI* BotInitStateMachine::GetBotAI() const
{
    Player* bot = GetBot();
    if (!bot)
        return nullptr;

    // Get the BotAI from the bot
    // This assumes bot->GetBotAI() method exists (added in Phase 1)
    return bot->GetBotAI();
}

Group* BotInitStateMachine::GetBotGroup() const
{
    Player* bot = GetBot();
    if (!bot)
        return nullptr;

    return bot->GetGroup();
}

} // namespace Playerbot::StateMachine