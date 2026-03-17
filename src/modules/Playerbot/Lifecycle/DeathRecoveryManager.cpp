/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DeathRecoveryManager.h"
#include "Player.h"
#include "Creature.h"
#include "Corpse.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Map.h"
#include "TerrainMgr.h"
#include "GameObject.h"  // For TransportBase and transport handling in manual teleport completion
#include "WorldSession.h"
// MovementPackets.h removed - teleport ACK now handled by BotPacketSimulator
#include "MotionMaster.h"
#include "Log.h"
#include "Config/PlayerbotConfig.h"
#include "Config/PlayerbotConfig.h"
#include "AI/BotAI.h"
#include "Interaction/Core/InteractionManager.h"
#include "GridNotifiers.h"
#include "CellImpl.h"
#include "GameTime.h"  // For corpse reclaim delay check
#include <sstream>
#include "../Spatial/SpatialGridManager.h"  // Lock-free spatial grid for deadlock fix
#include "../Spatial/SpatialGridQueryHelpers.h"  // Thread-safe spatial queries
#include "MiscPackets.h"  // For ReclaimCorpse packet
#include "Opcodes.h"  // For CMSG_RECLAIM_CORPSE
#include "../Session/BotSessionManager.h"  // For BotSessionManager::GetBotSession (safe resurrection)
#include "../Session/BotSession.h"         // For BotSession::QueueSafeResurrection (crash fix)

// PHASE 3 MIGRATION: Movement Arbiter Integration
#include "Movement/UnifiedMovementCoordinator.h"
#include "Movement/Arbiter/MovementRequest.h"
#include "Movement/Arbiter/MovementPriorityMapper.h"

namespace Playerbot
{

// ============================================================================
// DeathRecoveryConfig Implementation
// ============================================================================

DeathRecoveryConfig DeathRecoveryConfig::LoadFromConfig()
{
    DeathRecoveryConfig config;

    config.autoReleaseDelayMs = sPlayerbotConfig->GetInt("Playerbot.AutoReleaseDelay", 5) * 1000;
    config.preferCorpseRun = sPlayerbotConfig->GetBool("Playerbot.PreferCorpseRun", true);
    config.maxCorpseRunDistance = sPlayerbotConfig->GetFloat("Playerbot.MaxCorpseRunDistance", 200.0f);
    config.autoSpiritHealer = sPlayerbotConfig->GetBool("Playerbot.AutoSpiritHealer", true);
    config.allowBattleResurrection = sPlayerbotConfig->GetBool("Playerbot.AllowBattleResurrection", true);
    config.navigationUpdateInterval = sPlayerbotConfig->GetInt("Playerbot.DeathRecovery.NavigationInterval", 500);
    config.corpseDistanceCheckInterval = sPlayerbotConfig->GetInt("Playerbot.DeathRecovery.DistanceCheckInterval", 1000);
    config.skipResurrectionSickness = sPlayerbotConfig->GetBool("Playerbot.SkipResurrectionSickness", false);
    config.spiritHealerSearchRadius = sPlayerbotConfig->GetFloat("Playerbot.SpiritHealerSearchRadius", 150.0f);
    config.resurrectionTimeout = sPlayerbotConfig->GetInt("Playerbot.ResurrectionTimeout", 300) * 1000;
    config.logDebugInfo = sPlayerbotConfig->GetBool("Playerbot.LogDeathRecovery", true);

    return config;
}

// ============================================================================
// DeathRecoveryStatistics Implementation
// ============================================================================

void DeathRecoveryStatistics::RecordDeath()
{
    ++totalDeaths;
}

void DeathRecoveryStatistics::RecordResurrection(ResurrectionMethod method, uint64 recoveryTimeMs, bool hadSickness)
{
    switch (method)
    {
        case ResurrectionMethod::CORPSE_RUN:
            ++corpseResurrections;
            break;
        case ResurrectionMethod::SPIRIT_HEALER:
            ++spiritHealerResurrections;
            if (hadSickness)
                ++resurrectionsWithSickness;
            break;
        case ResurrectionMethod::BATTLE_RESURRECTION:
            ++battleResurrections;
            break;
        default:
            break;
    }

    totalRecoveryTimeMs += recoveryTimeMs;
    uint32 totalResurrections = corpseResurrections + spiritHealerResurrections + battleResurrections;
    if (totalResurrections > 0)
        averageRecoveryTimeMs = totalRecoveryTimeMs / totalResurrections;
}

void DeathRecoveryStatistics::RecordFailure()
{
    ++failedResurrections;
}

::std::string DeathRecoveryStatistics::ToString() const
{
    ::std::ostringstream oss;
    oss << "Death Recovery Statistics:\n"
        << "  Total Deaths: " << totalDeaths << "\n"
        << "  Corpse Resurrections: " << corpseResurrections << "\n"
        << "  Spirit Healer Resurrections: " << spiritHealerResurrections << "\n"
        << "  Battle Resurrections: " << battleResurrections << "\n"
        << "  Failed Resurrections: " << failedResurrections << "\n"
        << "  Average Recovery Time: " << (averageRecoveryTimeMs / 1000.0f) << "s\n"
        << "  Resurrections with Sickness: " << resurrectionsWithSickness;
    return oss.str();
}

// ============================================================================
// DeathRecoveryManager Implementation
// ============================================================================

DeathRecoveryManager::DeathRecoveryManager(Player* bot, BotAI* ai)
    : m_bot(bot)
    , m_ai(ai)
    , m_state(DeathRecoveryState::NOT_DEAD)
    , m_method(ResurrectionMethod::UNDECIDED)
    , m_corpseDistance(-1.0f)
    , m_spiritHealerGuid(ObjectGuid::Empty)
    , m_navigationActive(false)
    , m_releaseTimer(0)
    , m_stateTimer(0)
    , m_retryTimer(0)
    , m_retryCount(0)
    , m_needsTeleportAck(false)
{
    m_config = DeathRecoveryConfig::LoadFromConfig();
    // CRITICAL: No logging with bot->GetName() in constructor/destructor
    // Player's m_name can be corrupted during concurrent access, causing ACCESS_VIOLATION
}

DeathRecoveryManager::~DeathRecoveryManager()
{
    // CRITICAL: No logging in destructors - allocation can throw + player may be invalid
}

// ========================================================================
// LIFECYCLE EVENTS
// ========================================================================
void DeathRecoveryManager::OnDeath()
{
    // CRITICAL DIAGNOSTIC: Log every death attempt with timestamp
    static uint32 deathCounter = 0;
    ++deathCounter;
    TC_LOG_ERROR("playerbot.death", "========================================");
    TC_LOG_ERROR("playerbot.death", " OnDeath() CALLED #{} for bot {}! deathState={}, IsAlive={}, IsGhost={}",
        deathCounter, m_bot ? m_bot->GetName() : "nullptr",
        m_bot ? static_cast<int>(m_bot->getDeathState()) : -1,
        m_bot ? (m_bot->IsAlive() ? "TRUE" : "FALSE") : "null",
        IsGhost() ? "TRUE" : "FALSE");
    TC_LOG_ERROR("playerbot.death", "========================================");
    // No lock needed - death recovery state is per-bot instance data
    if (!ValidateBotState())
    {
        TC_LOG_ERROR("playerbot.death", " OnDeath: ValidateBotState FAILED!");
        return;
    }

    m_stats.RecordDeath();
    m_deathTime = ::std::chrono::steady_clock::now();
    m_method = ResurrectionMethod::UNDECIDED;
    m_spiritHealerGuid = ObjectGuid::Empty;
    m_navigationActive = false;
    m_releaseTimer = m_config.autoReleaseDelayMs;
    m_retryCount = 0;

    // Check if bot is already a ghost (TrinityCore auto-releases spirit on death)
    if (IsGhost())
    {
        // CRITICAL FIX: Clear combat references to prevent use-after-free crash in Map::Update()
        // Ghost bots have invalid CombatReference pointers that cause ACCESS_VIOLATION
        m_bot->GetCombatManager().EndAllPvECombat();

        TC_LOG_ERROR("playerbot.death", " Bot {} DIED! Already a ghost, skipping spirit release. IsAlive={}, IsGhost={}",
            m_bot->GetName(), m_bot->IsAlive(), IsGhost());
        TransitionToState(DeathRecoveryState::GHOST_DECIDING, "Bot died as ghost");
    }
    else
    {
        // NOTE: Do NOT call EndAllPvECombat() here - BuildPlayerRepop() will handle cleanup
        // Calling it here causes double-remove of auras → ASSERT(!aura->IsRemoved()) failure

        TC_LOG_ERROR("playerbot.death", " Bot {} DIED! Initiating death recovery. Auto-release in {}s. IsAlive={}, IsGhost={}",
            m_bot->GetName(), m_config.autoReleaseDelayMs / 1000.0f, m_bot->IsAlive(), IsGhost());
        TransitionToState(DeathRecoveryState::JUST_DIED, "Bot died");
    }
}

void DeathRecoveryManager::OnResurrection()
{
    // No lock needed - death recovery state is per-bot instance data
    if (m_state == DeathRecoveryState::NOT_DEAD)
        return;

    uint64 recoveryTime = GetTimeSinceDeath();
    bool hadSickness = m_method == ResurrectionMethod::SPIRIT_HEALER && WillReceiveResurrectionSickness();

    m_stats.RecordResurrection(m_method, recoveryTime, hadSickness);

    TC_LOG_INFO("playerbot.death", "Bot {} resurrected via {} in {:.2f}s",
        m_bot ? m_bot->GetName() : "nullptr",
        m_method == ResurrectionMethod::CORPSE_RUN ? "corpse" :
        m_method == ResurrectionMethod::SPIRIT_HEALER ? "spirit healer" :
        m_method == ResurrectionMethod::BATTLE_RESURRECTION ? "battle rez" : "unknown",
        recoveryTime / 1000.0f);

    Reset();

    // CRITICAL FIX: Signal BotAI to reset AI state from DEAD to SOLO
    // Without this, bots remain in DEAD state and do nothing after resurrection
    if (m_ai)
    {
        m_ai->OnRespawn();
        TC_LOG_INFO("playerbot.death", "Bot {} AI state reset via OnRespawn() - bot should now resume normal behavior",
            m_bot ? m_bot->GetName() : "nullptr");
    }
    else
    {
        TC_LOG_ERROR("playerbot.death", "Bot {} has no AI reference - cannot reset AI state after resurrection!",
            m_bot ? m_bot->GetName() : "nullptr");
    }
}

void DeathRecoveryManager::Reset()
{
    // No lock needed - death recovery state is per-bot instance data

    m_state = DeathRecoveryState::NOT_DEAD;
    m_method = ResurrectionMethod::UNDECIDED;
    m_corpseDistance = -1.0f;
    m_spiritHealerGuid = ObjectGuid::Empty;
    m_navigationActive = false;
    m_releaseTimer = 0;
    m_stateTimer = 0;
    m_retryTimer = 0;
    m_retryCount = 0;
    m_needsTeleportAck = false;

    // OPTION 3 REFACTOR: No longer manage Ghost flag manually
    // TrinityCore handles Ghost aura (spell 8326) and PLAYER_FLAGS_GHOST automatically
    // We only need to reset our internal state tracking

    LogDebug("Death recovery state reset");
}
// ========================================================================
// MAIN UPDATE LOOP
// ========================================================================

void DeathRecoveryManager::Update(uint32 diff)
{
    // Only update if in death recovery
    if (m_state == DeathRecoveryState::NOT_DEAD)
        return;

    // No lock needed - death recovery state is per-bot instance data
    if (!ValidateBotState())
    {
        TC_LOG_ERROR("playerbot.death", " Bot {} Update: ValidateBotState FAILED!", m_bot ? m_bot->GetName() : "nullptr");
        HandleResurrectionFailure("Bot state validation failed");
        return;
    }

    // Check for timeout
    if (IsResurrectionTimedOut())
    {
        TC_LOG_ERROR("playerbot.death", "⏰ Bot {} Update: Resurrection TIMED OUT!", m_bot->GetName());
        HandleResurrectionFailure("Resurrection timed out");
        return;
    }

    // CLEANUP FIX: Detect bots stuck in old failed state before ghost flag fix
    // Old bug signature: State=RESURRECTION_FAILED (10) with IsGhost=false
    // These bots need to be force-resurrected immediately to unstick them
    // Note: Don't check IsAlive() here because ForceResurrection will reset state to NOT_DEAD,
    // and ResurrectPlayer() doesn't set IsAlive instantly, which would cause repeated triggering
    if (m_state.load() == DeathRecoveryState::RESURRECTION_FAILED && !IsGhost())
    {
        TC_LOG_ERROR("playerbot.death", " Bot {} STUCK in old failed state (State=10, IsGhost=false) - FORCE RESURRECTING!",
            m_bot->GetName());
        ForceResurrection(ResurrectionMethod::SPIRIT_HEALER);
        return;
    }

    // Log state every 5 seconds
    static uint32 logTimer = 0;
    logTimer += diff;
    if (logTimer >= 5000)
    {
        logTimer = 0;
        TC_LOG_ERROR("playerbot.death", " Bot {} Update: State={}, IsAlive={}, IsGhost={}",
            m_bot->GetName(), static_cast<int>(m_state.load()), m_bot->IsAlive(), IsGhost());
    }

    // Handle current state
    switch (m_state.load())
    {
        case DeathRecoveryState::JUST_DIED:
            HandleJustDied(diff);
            break;
        case DeathRecoveryState::RELEASING_SPIRIT:
            HandleReleasingSpirit(diff);
            break;
        case DeathRecoveryState::PENDING_TELEPORT_ACK:
            HandlePendingTeleportAck(diff);
            break;
        case DeathRecoveryState::GHOST_DECIDING:
            HandleGhostDeciding(diff);
            break;
        case DeathRecoveryState::RUNNING_TO_CORPSE:
            HandleRunningToCorpse(diff);
            break;
        case DeathRecoveryState::AT_CORPSE:
            HandleAtCorpse(diff);
            break;
        case DeathRecoveryState::FINDING_SPIRIT_HEALER:
            HandleFindingSpiritHealer(diff);
            break;
        case DeathRecoveryState::MOVING_TO_SPIRIT_HEALER:
            HandleMovingToSpiritHealer(diff);
            break;
        case DeathRecoveryState::AT_SPIRIT_HEALER:
            HandleAtSpiritHealer(diff);
            break;
        case DeathRecoveryState::RESURRECTING:
            HandleResurrecting(diff);
            break;
        case DeathRecoveryState::RESURRECTION_FAILED:
            HandleResurrectionFailed(diff);
            break;
        default:
            break;
    }
}

// ========================================================================
// STATE MACHINE HANDLERS
// ========================================================================

void DeathRecoveryManager::HandleJustDied(uint32 diff)
{
    // Wait for auto-release timer
    if (m_releaseTimer > diff)
    {
        m_releaseTimer -= diff;
        if (m_releaseTimer % 1000 < diff) // Log every second
        {
            TC_LOG_ERROR("playerbot.death", "⏳ Bot {} waiting to release spirit... {:.1f}s remaining",
                m_bot->GetName(), m_releaseTimer / 1000.0f);
        }
        return;
    }
    // Check if bot already released (manually or by another system)
    if (IsGhost())
    {
        TC_LOG_ERROR("playerbot.death", " Bot {} already a ghost, proceeding to decision phase", m_bot->GetName());
        TransitionToState(DeathRecoveryState::GHOST_DECIDING, "Already ghost");
        return;
    }

    // Release spirit
    TC_LOG_ERROR("playerbot.death", " Bot {} auto-release timer expired, releasing spirit...", m_bot->GetName());
    TransitionToState(DeathRecoveryState::RELEASING_SPIRIT, "Auto-release timer expired");
}

void DeathRecoveryManager::HandleReleasingSpirit(uint32 diff)
{
    TC_LOG_ERROR("playerbot.death", " Bot {} attempting to release spirit... IsGhost={}", m_bot->GetName(), IsGhost());
    if (ExecuteReleaseSpirit())
    {
        TC_LOG_ERROR("playerbot.death", " Bot {} spirit released successfully! IsGhost={}", m_bot->GetName(), IsGhost());
        // NOTE: ExecuteReleaseSpirit() may transition to PENDING_TELEPORT_ACK state
        // If not (no teleport needed), it will fall through to GHOST_DECIDING
    }
    else
    {
        // Retry after delay
        m_stateTimer += diff;
        if (m_stateTimer > 2000) // Retry every 2 seconds
        {
            m_stateTimer = 0;
            TC_LOG_ERROR("playerbot.death", " Bot {} retrying spirit release (IsGhost={})", m_bot->GetName(), IsGhost());
        }
    }
}

void DeathRecoveryManager::HandlePendingTeleportAck(uint32 diff)
{
    // The BotPacketSimulator's ACK scanner (runs every 100ms from BotSession::Update(),
    // BEFORE the AI update loop) automatically processes near-teleport ACKs when
    // IsBeingTeleportedNear() is true. This provides the 100ms stabilization delay
    // that prevents the Spell.cpp:603 crash (m_spellModTakingSpell assertion).
    //
    // This state handler just waits for the teleport to complete, then transitions.

    auto now = ::std::chrono::steady_clock::now();
    auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
        now - m_teleportAckTime).count();

    // Wait minimum 200ms for BotPacketSimulator ACK scanner to process the teleport
    // (ACK scanner runs every 100ms, so 200ms guarantees at least one full scan cycle)
    if (elapsed < 200)
    {
        if (elapsed % 50 < diff)
        {
            TC_LOG_TRACE("playerbot.death", "Bot {} waiting for ACK scanner to process teleport... {}ms elapsed",
                m_bot->GetName(), elapsed);
        }
        return;
    }

    // Check if teleport was successfully ACKed by BotPacketSimulator
    if (m_bot && !m_bot->IsBeingTeleportedNear())
    {
        TC_LOG_DEBUG("playerbot.death", "Bot {} teleport ACK processed by BotPacketSimulator ({}ms)",
            m_bot->GetName(), elapsed);
        m_needsTeleportAck = false;
    }
    else if (elapsed > 5000)
    {
        // Safety timeout - teleport should have been ACKed within a few scan cycles
        TC_LOG_ERROR("playerbot.death", "Bot {} teleport ACK timeout after {}ms, forcing transition",
            m_bot->GetName(), elapsed);
        m_needsTeleportAck = false;
    }
    else
    {
        // Still pending - wait for next update cycle
        return;
    }

    // Transition to next state
    TransitionToState(DeathRecoveryState::GHOST_DECIDING, "Teleport ack completed, proceeding to decision");
}

void DeathRecoveryManager::HandleGhostDeciding(uint32 /*diff*/)
{
    TC_LOG_ERROR("playerbot.death", " Bot {} deciding resurrection method...", m_bot->GetName());
    // Check for special cases first (battlegrounds, arenas, etc)
    if (CheckSpecialResurrectionCases())
    {
        TC_LOG_ERROR("playerbot.death", " Bot {} in special zone, using special resurrection", m_bot->GetName());
        return;
    }

    // Make resurrection decision
    DecideResurrectionMethod();
    if (m_method == ResurrectionMethod::CORPSE_RUN)
    {
        TC_LOG_ERROR("playerbot.death", " Bot {} chose CORPSE RUN (distance: {:.1f}y)", m_bot->GetName(), GetCorpseDistance());
        TransitionToState(DeathRecoveryState::RUNNING_TO_CORPSE, "Chose corpse run");
    }
    else if (m_method == ResurrectionMethod::SPIRIT_HEALER)
    {
        TC_LOG_ERROR("playerbot.death", " Bot {} chose SPIRIT HEALER", m_bot->GetName());
        TransitionToState(DeathRecoveryState::FINDING_SPIRIT_HEALER, "Chose spirit healer");
    }
    else
    {
        TC_LOG_ERROR("playerbot.death", " Bot {} FAILED to decide resurrection method!", m_bot->GetName());
        HandleResurrectionFailure("Failed to decide resurrection method");
    }
}

void DeathRecoveryManager::HandleRunningToCorpse(uint32 diff)
{
    // Update corpse distance periodically
    m_stateTimer += diff;
    if (m_stateTimer >= m_config.corpseDistanceCheckInterval)
    {
        m_stateTimer = 0;
        UpdateCorpseDistance();

        if (m_corpseDistance < 0.0f)
        {
            TC_LOG_ERROR("playerbot.death", " Bot {} CRITICAL: Lost corpse location during corpse run!", m_bot->GetName());
            HandleResurrectionFailure("Lost corpse location");
            return;
        }
        TC_LOG_INFO("playerbot.death", " Bot {} distance to corpse: {:.1f} yards (resurrection range: {})",
            m_bot->GetName(), m_corpseDistance, CORPSE_RESURRECTION_RANGE);
    }

    // Check if in range
    if (IsInCorpseRange())
    {
        TC_LOG_INFO("playerbot.death", " Bot {} reached corpse! Distance: {:.1f} yards",
            m_bot->GetName(), m_corpseDistance);
        TransitionToState(DeathRecoveryState::AT_CORPSE, "Reached corpse");
        return;
    }

    // Update navigation periodically
    auto now = ::std::chrono::steady_clock::now();
    auto timeSinceLastNav = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
        now - m_lastNavigationUpdate).count();

    if (timeSinceLastNav >= m_config.navigationUpdateInterval)
    {
        TC_LOG_DEBUG("playerbot.death", "  Bot {} updating navigation to corpse (distance: {:.1f}y)",
            m_bot->GetName(), m_corpseDistance);

        if (NavigateToCorpse())
        {
            m_lastNavigationUpdate = now;
        }
        else
        {
            TC_LOG_ERROR("playerbot.death", " Bot {} CRITICAL: Failed to navigate to corpse!",
                m_bot->GetName());
            HandleResurrectionFailure("Failed to navigate to corpse");
        }
    }
}

void DeathRecoveryManager::HandleAtCorpse(uint32 diff)
{
    m_stateTimer += diff;

    // ============================================================================
    // ENTERPRISE-GRADE PACKET-BASED RESURRECTION SYSTEM (Option B)
    // ============================================================================
    //
    // Design Rationale:
    // -----------------
    // Direct ResurrectPlayer() calls from bot worker threads cause crashes when
    // UpdateAreaDependentAuras() → CastSpell() is called during resurrection.
    // GM .revive command (main thread) works perfectly with same auras.
    //
    // Solution: Queue CMSG_RECLAIM_CORPSE packet for main thread processing.
    // This delegates ALL resurrection logic to TrinityCore's proven packet handler
    // (HandleReclaimCorpse in MiscHandler.cpp) which runs on the main thread.
    //
    // Thread Safety:
    // --------------
    // - QueuePacket() uses LockedQueue<WorldPacket*> with mutex (thread-safe)
    // - HandleReclaimCorpse() executes on main thread (no race conditions)
    // - HandleResurrecting() polls IsAlive() to detect completion
    //
    // Validation Strategy:
    // --------------------
    // Perform basic pre-validation here (corpse exists, ghost flag, distance)
    // HandleReclaimCorpse() performs its own comprehensive validation (7 checks)
    // This prevents unnecessary packet queuing for obviously invalid cases
    // ============================================================================

    // VALIDATION 1: Bot must not be already alive
    if (m_bot->IsAlive())
    {
        TC_LOG_INFO("playerbot.death", " Bot {} is already alive, no resurrection needed",
            m_bot->GetName());
        TransitionToState(DeathRecoveryState::NOT_DEAD, "Already alive");
        return;
    }

    // VALIDATION 2: Must have corpse
    Corpse* corpse = m_bot->GetCorpse();
    if (!corpse)
    {
        TC_LOG_ERROR("playerbot.death", " Bot {} has no corpse! Cannot resurrect.",
            m_bot->GetName());

        // Timeout after 30 seconds
    if (m_stateTimer > 30000)
        {
            TC_LOG_ERROR("playerbot.death", " Bot {} CRITICAL: No corpse after 30 seconds!",
                m_bot->GetName());
            HandleResurrectionFailure("No corpse exists after 30 seconds");
        }
        return;
    }

    // VALIDATION 3: Must be in ghost form (spirit released)
    if (!m_bot->HasPlayerFlag(PLAYER_FLAGS_GHOST))
    {
        TC_LOG_WARN("playerbot.death", "  Bot {} does not have PLAYER_FLAGS_GHOST, cannot resurrect yet",
            m_bot->GetName());

        // Timeout after 30 seconds
    if (m_stateTimer > 30000)
        {
            TC_LOG_ERROR("playerbot.death", " Bot {} CRITICAL: No ghost flag after 30 seconds!",
                m_bot->GetName());
            HandleResurrectionFailure("Spirit not released after 30 seconds");
        }
        return;
    }

    // VALIDATION 4: Check distance to corpse (must be within 39 yards)
    // HandleReclaimCorpse uses corpse->IsWithinDistInMap(player, 39.0f, true)
    float distance = m_bot->GetDistance2d(corpse);
    if (!corpse->IsWithinDistInMap(m_bot, CORPSE_RESURRECTION_RANGE, true))
    {
        TC_LOG_WARN("playerbot.death", "  Bot {} too far from corpse ({:.1f}y > {:.1f}y), moving closer",
            m_bot->GetName(), distance, CORPSE_RESURRECTION_RANGE);

        // Bot moved out of range, return to RUNNING_TO_CORPSE
        TransitionToState(DeathRecoveryState::RUNNING_TO_CORPSE, "Moved out of corpse range");
        m_stateTimer = 0; // Reset timer for next attempt
        return;
    }

    // VALIDATION 5: Check ghost time delay (prevent instant resurrection)
    // HandleReclaimCorpse checks: ghostTime + delay <= currentTime
    time_t ghostTime = corpse->GetGhostTime();
    time_t currentTime = GameTime::GetGameTime();
    uint32 corpseReclaimDelay = m_bot->GetCorpseReclaimDelay(
        corpse->GetType() == CORPSE_RESURRECTABLE_PVP);
    time_t requiredTime = ghostTime + corpseReclaimDelay;

    if (requiredTime > currentTime)
    {
        uint32 remainingSeconds = static_cast<uint32>(requiredTime - currentTime);

        // Log waiting status every 5 seconds
    if (m_stateTimer % 5000 < diff)
        {
            TC_LOG_INFO("playerbot.death", "⏳ Bot {} waiting for ghost time delay ({} seconds remaining)",
                m_bot->GetName(), remainingSeconds);
        }
        return; // Wait for delay to expire
    }
    // ============================================================================
    // ALL VALIDATIONS PASSED - Queue Safe Resurrection (SpawnCorpseBones Crash Fix)
    // ============================================================================
    // CRITICAL FIX: DO NOT use CMSG_RECLAIM_CORPSE packet!
    // HandleReclaimCorpse → SpawnCorpseBones → Map::RemoveWorldObject crashes
    // due to corrupted i_worldObjects tree structure (infinite loop in _Erase).
    //
    // Instead, we use QueueSafeResurrection() which calls ResurrectPlayer() directly
    // on the main thread WITHOUT SpawnCorpseBones. The corpse will decay naturally.
    // ============================================================================

    TC_LOG_INFO("playerbot.death", " Bot {} passed all 5 validation checks, queuing SAFE resurrection (bypasses SpawnCorpseBones crash)",
        m_bot->GetName());

    TC_LOG_DEBUG("playerbot.death", " Bot {} resurrection validation details: IsAlive=false, Corpse=yes, "
        "Ghost=true, Distance={:.1f}y<{:.1f}y, GhostDelay=expired",
        m_bot->GetName(), distance, CORPSE_RESURRECTION_RANGE);

    // Get BotSession for safe resurrection queue
    BotSession* botSession = BotSessionManager::GetBotSession(m_bot->GetSession());
    if (!botSession)
    {
        TC_LOG_ERROR("playerbot.death", " Bot {} CRITICAL: BotSession not found! Cannot queue safe resurrection.",
            m_bot->GetName());
        HandleResurrectionFailure("BotSession not found for safe resurrection");
        return;
    }

    // Queue safe resurrection (thread-safe atomic flag)
    // ProcessPendingSafeResurrection() will execute on main thread:
    // - Validates bot state (IsAlive, ghost flag, corpse distance)
    // - Calls ResurrectPlayer() directly
    // - Does NOT call SpawnCorpseBones (avoids crash)
    // - Corpse will decay naturally via TrinityCore's corpse cleanup
    botSession->QueueSafeResurrection();

    TC_LOG_INFO("playerbot.death", " Bot {} queued SAFE resurrection for main thread "
        "(distance: {:.1f}y, map: {}, corpseMap: {}) - NO SpawnCorpseBones!",
        m_bot->GetName(), distance, m_bot->GetMapId(), corpse->GetMapId());

    // Transition to RESURRECTING state
    // HandleResurrecting() will poll IsAlive() to detect completion
    // 30-second timeout will trigger HandleResurrectionFailure() if needed
    TransitionToState(DeathRecoveryState::RESURRECTING, "Safe resurrection scheduled (no SpawnCorpseBones)");
}

void DeathRecoveryManager::HandleFindingSpiritHealer(uint32 diff)
{
    Creature* spiritHealer = FindNearestSpiritHealer();

    if (spiritHealer)
    {
        m_spiritHealerGuid = spiritHealer->GetGUID();
        m_spiritHealerLocation = WorldLocation(spiritHealer->GetMapId(),
            spiritHealer->GetPositionX(),
            spiritHealer->GetPositionY(),
            spiritHealer->GetPositionZ(),
            spiritHealer->GetOrientation());

        TransitionToState(DeathRecoveryState::MOVING_TO_SPIRIT_HEALER, "Found spirit healer");
    }
    else
    {
        m_stateTimer += diff;
        if (m_stateTimer > 10000) // Retry every 10 seconds
        {
            m_stateTimer = 0;
            LogDebug("No spirit healer found, retrying search");

            // After multiple failures, try corpse run instead
    if (++m_retryCount >= 3)
            {
                TC_LOG_WARN("playerbot.death", "Bot {} cannot find spirit healer, switching to corpse run",
                    m_bot->GetName());
                m_method = ResurrectionMethod::CORPSE_RUN;
                TransitionToState(DeathRecoveryState::RUNNING_TO_CORPSE, "Fallback to corpse run");
            }
        }
    }
}

void DeathRecoveryManager::HandleMovingToSpiritHealer(uint32 /*diff*/)
{
    // PHASE 5D: Thread-safe spatial grid validation
    auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(m_bot, m_spiritHealerGuid);
    Creature* spiritHealer = nullptr;

    if (snapshot && snapshot->IsAlive())
    {
        // Get Creature* for distance check (validated via snapshot first)
        spiritHealer = ObjectAccessor::GetCreature(*m_bot, m_spiritHealerGuid);
    }

    if (!spiritHealer || !spiritHealer->IsAlive())
    {
        m_spiritHealerGuid = ObjectGuid::Empty;
        TransitionToState(DeathRecoveryState::FINDING_SPIRIT_HEALER, "Spirit healer invalid");
        return;
    }

    float distance = m_bot->GetDistance(spiritHealer);
    if (distance <= SPIRIT_HEALER_INTERACTION_RANGE)
    {
        TransitionToState(DeathRecoveryState::AT_SPIRIT_HEALER, "Reached spirit healer");
        return;
    }

    // Update navigation
    auto now = ::std::chrono::steady_clock::now();
    auto timeSinceLastNav = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
        now - m_lastNavigationUpdate).count();

    if (timeSinceLastNav >= m_config.navigationUpdateInterval)
    {
        if (NavigateToSpiritHealer())
        {
            m_lastNavigationUpdate = now;
            LogDebug("Navigating to spirit healer, distance: " + ::std::to_string(distance));
        }
        else
        {
            HandleResurrectionFailure("Failed to navigate to spirit healer");
        }
    }
}
void DeathRecoveryManager::HandleAtSpiritHealer(uint32 diff)
{
    if (InteractWithSpiritHealer())
    {
        TransitionToState(DeathRecoveryState::RESURRECTING, "Interacting with spirit healer");
    }
    else
    {
        m_stateTimer += diff;
        if (m_stateTimer > 5000) // Retry after 5 seconds
        {
            m_stateTimer = 0;
            LogDebug("Retrying spirit healer interaction");
        }
    }
}

void DeathRecoveryManager::HandleResurrecting(uint32 diff)
{
    // Check if bot is now alive
    if (m_bot->IsAlive())
    {
        TC_LOG_INFO("playerbot.death", " Bot {} IS ALIVE! Calling OnResurrection()...",
            m_bot->GetName());
        OnResurrection();
        return;
    }

    // Wait for resurrection to complete (max 30 seconds)
    m_stateTimer += diff;

    // Log waiting status every 5 seconds
    if (m_stateTimer % 5000 < diff)
    {
        TC_LOG_WARN("playerbot.death", "⏳ Bot {} waiting for resurrection... ({:.1f}s elapsed, IsAlive={})",
            m_bot->GetName(), m_stateTimer / 1000.0f, m_bot->IsAlive());
    }

    if (m_stateTimer > 30000)
    {
        // BATTLEGROUND FIX: In BGs, bots are never auto-resurrected because the BG spirit guide
        // requires client-side gossip interaction that bots don't perform. Instead of falling
        // through to RESURRECTION_FAILED → retry → corpse run, actively force-resurrect.
        // This prevents the pathological cycle where bots run to their corpse in BGs.
        if (m_method == ResurrectionMethod::AUTO_RESURRECT && m_bot->InBattleground())
        {
            TC_LOG_INFO("playerbot.death", "Bot {} in BG auto-resurrect: forcing resurrection after 30s wave timer",
                m_bot->GetName());
            if (ForceResurrection(ResurrectionMethod::SPIRIT_HEALER))
                return;
            // ForceResurrection failed (no corpse or no BotSession) — fall through to normal failure path
        }

        TC_LOG_ERROR("playerbot.death", " Bot {} CRITICAL: Resurrection did not complete after 30 seconds! (IsAlive={})",
            m_bot->GetName(), m_bot->IsAlive());
        HandleResurrectionFailure("Resurrection did not complete");
    }
}

void DeathRecoveryManager::HandleResurrectionFailed(uint32 diff)
{
    m_retryTimer += diff;
    if (m_retryTimer >= RETRY_DELAY_MS)
    {
        m_retryTimer = 0;

        // BATTLEGROUND FIX: Never retry via GHOST_DECIDING in BGs.
        // GHOST_DECIDING calls DecideResurrectionMethod() which may choose CORPSE_RUN
        // if InBattleground() returns false (BG ended during wait). Force-resurrect directly.
        if (m_bot && m_bot->InBattleground())
        {
            TC_LOG_INFO("playerbot.death", "Bot {} in BG resurrection retry: forcing resurrection directly",
                m_bot->GetName());
            ForceResurrection(ResurrectionMethod::SPIRIT_HEALER);
            return;
        }

        if (++m_retryCount >= MAX_RETRY_ATTEMPTS)
        {
            TC_LOG_ERROR("playerbot.death", "Bot {} exhausted all resurrection attempts",
                m_bot->GetName());
            m_stats.RecordFailure();

            // Force resurrection as last resort
            ForceResurrection(ResurrectionMethod::SPIRIT_HEALER);
        }
        else
        {
            TC_LOG_WARN("playerbot.death", "Bot {} retrying resurrection (attempt {}/{})",
                m_bot->GetName(), m_retryCount, MAX_RETRY_ATTEMPTS);

            // Reset to decision phase
            TransitionToState(DeathRecoveryState::GHOST_DECIDING, "Retry resurrection");
        }
    }
}
// ========================================================================
// DECISION LOGIC
// ========================================================================

void DeathRecoveryManager::DecideResurrectionMethod()
{
    // Check configuration preference
    if (ShouldDoCorpseRun())
    {
        m_method = ResurrectionMethod::CORPSE_RUN;
        TC_LOG_DEBUG("playerbot.death", "Bot {} chose corpse run (distance: {:.1f}y)",
            m_bot->GetName(), GetCorpseDistance());
    }
    else if (ShouldUseSpiritHealer())
    {
        m_method = ResurrectionMethod::SPIRIT_HEALER;
        TC_LOG_DEBUG("playerbot.death", "Bot {} chose spirit healer",
            m_bot->GetName());
    }
    else
    {
        // Default to corpse run
        m_method = ResurrectionMethod::CORPSE_RUN;
        TC_LOG_WARN("playerbot.death", "Bot {} defaulting to corpse run",
            m_bot->GetName());
    }
}

bool DeathRecoveryManager::ShouldDoCorpseRun() const
{
    if (!m_config.preferCorpseRun)
        return false;

    UpdateCorpseDistance();
    if (m_corpseDistance < 0.0f)
        return false; // No corpse
    if (m_corpseDistance > m_config.maxCorpseRunDistance)
        return false; // Too far

    return true;
}
bool DeathRecoveryManager::ShouldUseSpiritHealer() const
{
    if (!m_config.autoSpiritHealer)
        return false;

    // Use spirit healer if corpse is too far or unreachable
    UpdateCorpseDistance();

    if (m_corpseDistance < 0.0f)
        return true; // No corpse
    if (m_corpseDistance > m_config.maxCorpseRunDistance)
        return true; // Too far

    return false;
}

bool DeathRecoveryManager::CheckSpecialResurrectionCases()
{
    if (!m_bot)
        return false;
    // Battlegrounds/arenas have special resurrection mechanics
    if (m_bot->InBattleground())
    {
        // Battlegrounds typically auto-resurrect
        TC_LOG_DEBUG("playerbot.death", "Bot {} in battleground, using default BG resurrection",
            m_bot->GetName());
        m_method = ResurrectionMethod::AUTO_RESURRECT;
        TransitionToState(DeathRecoveryState::RESURRECTING, "Battleground auto-resurrection");
        return true;
    }

    if (m_bot->InArena())
    {
        // Arenas don't allow resurrection during match
        TC_LOG_DEBUG("playerbot.death", "Bot {} in arena, waiting for match end",
            m_bot->GetName());
        return true; // Stay in current state
    }

    return false;
}

// ========================================================================
// RESURRECTION EXECUTION
// ========================================================================
bool DeathRecoveryManager::ExecuteReleaseSpirit()
{
    if (!m_bot)
        return false;

    // Check if already a ghost
    if (IsGhost())
        return true;

    // OPTION 3 REFACTOR: Let TrinityCore handle Ghost aura (spell 8326) and PLAYER_FLAGS_GHOST
    // BuildPlayerRepop() automatically:
    // 1. Creates corpse
    // 2. Applies Ghost aura (spell 8326) which sets PLAYER_FLAGS_GHOST
    // 3. Sets ghost state properly
    // We never touch Ghost aura or flag manually - TrinityCore manages the entire lifecycle

    // DEBUG: Log position BEFORE BuildPlayerRepop
    Position posBeforeRepop = m_bot->GetPosition();
    TC_LOG_ERROR("playerbot.death", " Bot {} BEFORE BuildPlayerRepop: Map={} Zone={} Pos=({:.2f}, {:.2f}, {:.2f}) Team={}",
        m_bot->GetName(), m_bot->GetMapId(), m_bot->GetZoneId(),
        posBeforeRepop.GetPositionX(), posBeforeRepop.GetPositionY(), posBeforeRepop.GetPositionZ(),
        m_bot->GetTeam());
    // CRITICAL FIX: Check if bot already has Ghost aura before calling BuildPlayerRepop()
    // BuildPlayerRepop() casts Ghost spell (8326) at Player.cpp:4313, but will crash if aura already applied
    // This can happen if bot dies while already a ghost or if resurrection failed
    if (m_bot->HasAura(8326))
    {
        TC_LOG_WARN("playerbot.death",
            " Bot {} already has Ghost aura (8326)! Skipping BuildPlayerRepop() to prevent assertion crash. "
            "This can happen if bot died while already a ghost.",
            m_bot->GetName());

        // Bot is already a ghost - skip BuildPlayerRepop() and proceed to decision phase
        // Return TRUE because this is a successful operation (we successfully avoided the crash)
        TransitionToState(DeathRecoveryState::GHOST_DECIDING, "Bot already has Ghost aura, skipping BuildPlayerRepop()");
        return true;  // Success - we handled the Ghost aura case properly
    }
    // CRITICAL FIX: Prevent double BuildPlayerRepop() race condition with TrinityCore auto-release
    // Problem: TrinityCore's Player::Update() has auto-release logic (Player.cpp:1075-1081):
    //   if (m_deathTimer > 0 && !Instanceable) { m_deathTimer = 0; BuildPlayerRepop(); RepopAtGraveyard(); }
    // If TrinityCore's auto-release fires AFTER we call BuildPlayerRepop(), it will apply Ghost aura (8326) TWICE
    // → SpellAuras.cpp:168 assertion crash: "HasEffect(effIndex) == (!apply)"
    //
    // Solution: Check if deathTimer is about to expire (< 500ms). If yes, let TrinityCore handle it automatically.
    // This prevents us from calling BuildPlayerRepop() manually just before TrinityCore does the same.
    // Trade-off: Bot resurrection may be delayed by up to 500ms, but this prevents 100% of double-call crashes.
    uint32 deathTimer = m_bot->GetDeathTimer();
    if (deathTimer > 0 && deathTimer < 500)
    {
        TC_LOG_WARN("playerbot.death",
            "⏰ Bot {} has death timer {}ms < 500ms - letting TrinityCore auto-release handle BuildPlayerRepop() "
            "to prevent double-call crash. Will wait for next update.",
            m_bot->GetName(), deathTimer);
        return false;  // Try again next update after TrinityCore auto-release fires
    }

    // GHOST SPELL CRASH FIX: Queue CMSG_REPOP_REQUEST packet instead of calling BuildPlayerRepop() directly
    // Direct call executes on bot worker thread → race condition with Map::Update() during Ghost aura application
    // Packet-based approach defers to main thread via PacketDeferralClassifier (already includes CMSG_REPOP_REQUEST)
    //
    // HandleRepopRequest (MiscHandler.cpp:82-83) will execute on main thread:
    //   1. BuildPlayerRepop() - Creates corpse + applies Ghost spell 8326
    //   2. RepopAtGraveyard() - Teleports to graveyard
    //
    // This serializes Ghost spell application with Map::Update(), preventing SpellAuras.cpp:168 crash
    // TrinityCore 12.0: QueuePacket now takes WorldPacket&& instead of WorldPacket*
    WorldPacket repopPacket(CMSG_REPOP_REQUEST, 1);
    repopPacket << uint8(0);  // CheckInstance = false (not in instance recovery)
    m_bot->GetSession()->QueuePacket(std::move(repopPacket));

    TC_LOG_WARN("playerbot.death",
        "Bot {} queued CMSG_REPOP_REQUEST packet for main thread execution (Ghost spell crash fix)",
        m_bot->GetName());

    // BOT AUTO-RESURRECTION DETECTION: Check if BotResurrectionScript auto-resurrected during BuildPlayerRepop()
    // NOTE: With packet-based approach, BuildPlayerRepop() executes asynchronously on main thread
    // We can't check IsAlive() immediately - the packet hasn't been processed yet
    // Instead, we defer this check to next Update() cycle (see GHOST_DECIDING state handling)
    // Old synchronous check removed:
    // if (m_bot->IsAlive())
    // {
    //     TC_LOG_INFO("playerbot.death",
    //         " Bot {} was auto-resurrected during BuildPlayerRepop() by BotResurrectionScript! "
    //         "Health={}/{}, Mana={}/{}, Position=({:.2f}, {:.2f}, {:.2f})",
    //         m_bot->GetName(),
    //         m_bot->GetHealth(), m_bot->GetMaxHealth(),
    //         m_bot->GetPower(POWER_MANA), m_bot->GetMaxPower(POWER_MANA),
    //         m_bot->GetPositionX(), m_bot->GetPositionY(), m_bot->GetPositionZ());
    //
    //     TransitionToState(DeathRecoveryState::NOT_DEAD, "Auto-resurrected at corpse successfully");
    //     return true;  // Exit HandleReleasingSpirit(), death recovery complete
    // }

    // PACKET-BASED APPROACH: HandleRepopRequest will execute asynchronously on main thread
    // The handler will call BuildPlayerRepop() + RepopAtGraveyard() in the correct sequence
    // We transition to PENDING_TELEPORT_ACK state and let the next Update() cycle check the results
    //
    // NOTE: All the following logic (corpse checks, zone validation, RepopAtGraveyard() call, teleport ack)
    // is now handled by HandleRepopRequest on the main thread. We can't execute it here because the
    // packet hasn't been processed yet - everything is asynchronous now.
    //
    // The state machine will handle teleport ack when IsBeingTeleportedNear() becomes true after
    // HandleRepopRequest completes (see PENDING_TELEPORT_ACK state in Update() method)

    TransitionToState(DeathRecoveryState::PENDING_TELEPORT_ACK,
        "Waiting for CMSG_REPOP_REQUEST to execute on main thread");

    TC_LOG_WARN("playerbot.death",
        " Bot {} queued spirit release - waiting for main thread execution (async packet-based approach)",
        m_bot->GetName());

    return true;
}

bool DeathRecoveryManager::NavigateToCorpse()
{
    if (!m_bot)
        return false;

    WorldLocation corpseLocation = GetCorpseLocation();
    if (corpseLocation.GetMapId() == MAPID_INVALID)
        return false;
    // PHASE 3 MIGRATION: Use Movement Arbiter with DEATH_RECOVERY priority (255)
    // Death recovery has the ABSOLUTE HIGHEST priority - must override ALL other movement
    BotAI* botAI = dynamic_cast<BotAI*>(m_bot->GetAI());
    if (botAI && botAI->GetUnifiedMovementCoordinator())
    {
        // CORPSE RUN MOVEMENT FIX: Throttle movement updates to 500ms to prevent spell mod crashes
        // Rapid movement recalculation causes Spell.cpp:603 assertion failure (m_spellModTakingSpell corruption)
        auto now = ::std::chrono::steady_clock::now();
        auto timeSinceLastUpdate = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - m_lastNavigationUpdate);

        if (m_navigationActive && timeSinceLastUpdate.count() < 500)
        {
            // Still within throttle window - don't update movement yet
            TC_LOG_DEBUG("playerbot.death",
                "Bot {} corpse run throttled: {}ms since last update (need 500ms)",
                m_bot->GetName(), timeSinceLastUpdate.count());
            return true; // Navigation still active, just throttled
        }

        // Get corpse object to chase
        Corpse* corpse = m_bot->GetCorpse();
        if (!corpse)
        {
            TC_LOG_ERROR("playerbot.death",
                "DeathRecoveryManager: Bot {} has no corpse to navigate to",
                m_bot->GetName());
            return false;
        }

        // MOVEMENT FIX: Use RequestChaseMovement instead of RequestPointMovement
        // This makes the bot continuously track the corpse GUID rather than moving to a static position
        // Prevents instant resurrection appearance by providing visible corpse run with walking speed
        bool accepted = botAI->RequestChaseMovement(
            PlayerBotMovementPriority::DEATH_RECOVERY,  // Priority 255 - HIGHEST
            corpse->GetGUID(),  // Chase the corpse by GUID
            "Corpse run - death recovery (chase mode)",
            "DeathRecoveryManager");

        if (accepted)
        {
            TC_LOG_DEBUG("playerbot.movement.arbiter",
                "DeathRecoveryManager: Bot {} requested CHASE movement to corpse {} with DEATH_RECOVERY priority (255)",
                m_bot->GetName(),
                corpse->GetGUID().ToString());

            m_navigationActive = true;
            m_lastNavigationUpdate = now; // Update timestamp for 500ms throttling
            return true;
        }
        else
        {
            TC_LOG_WARN("playerbot.movement.arbiter",
                "DeathRecoveryManager: Bot {} corpse run movement request FILTERED (duplicate detected)",
                m_bot->GetName());
            // Still consider navigation active if filtered (likely duplicate)
            m_navigationActive = true;
            return true;
        }
    }
    else
    {
        // FALLBACK: Direct MotionMaster call if arbiter not available
        // This should only happen during transition period
        TC_LOG_WARN("playerbot.movement.arbiter",
            "DeathRecoveryManager: Bot {} has no MovementArbiter - using legacy MovePoint() for corpse run",
            m_bot->GetName());

        // CRITICAL FIX: Clear MotionMaster before calling MovePoint (like mod-playerbot does)
        // This prevents movement spam/cancellation that causes "teleporting" behavior
        // Reference: mod-playerbot MovementActions.cpp:244-248
        MotionMaster* mm = m_bot->GetMotionMaster();
        mm->Clear();

        // CORPSE RACE CONDITION FIX: Use walking speed to prevent instant teleportation
        // Instant movement causes race condition with Map::SendObjectUpdates() when corpse
        // gets deleted while still in _updateObjects set (crash at Map.cpp:1940)
        // Walking speed adds natural delay (~3-5 seconds) between reaching corpse and resurrection
        mm->MovePoint(0,
            corpseLocation.GetPositionX(),
            corpseLocation.GetPositionY(),
            corpseLocation.GetPositionZ(),
            true,   // generatePath = true for proper pathfinding
            {},     // finalOrient - default
            2.5f);  // speed - WALKING SPEED (default run is ~7.0) prevents instant teleport

        m_navigationActive = true;
        return true;
    }
}

bool DeathRecoveryManager::InteractWithCorpse()
{
    // RACE CONDITION FIX: Single mutex-protected critical section
    // Replaces three-layer protection (mutex + debounce + atomic) that had race windows
    ::std::lock_guard<::std::timed_mutex> lock(_resurrectionMutex);
    
    // Check atomic debounce inside mutex protection (prevents TOCTOU race)
    uint64 now = GameTime::GetGameTimeMS();
    uint64 lastAttempt = _lastResurrectionAttemptMs.load(::std::memory_order_acquire);
    
    if (now - lastAttempt < RESURRECTION_DEBOUNCE_MS)
    {
        TC_LOG_WARN("playerbot.death", "Bot {} InteractWithCorpse: Too soon since last attempt ({}ms < {}ms), debouncing",
            m_bot ? m_bot->GetName() : "nullptr", now - lastAttempt, RESURRECTION_DEBOUNCE_MS);
        return false;
    }
    
    // Check atomic resurrection flag inside mutex protection (prevents concurrent resurrections)
    bool expectedFalse = false;
    if (!_resurrectionInProgress.compare_exchange_strong(expectedFalse, true, ::std::memory_order_acq_rel))
    {
        TC_LOG_WARN("playerbot.death", "Bot {} InteractWithCorpse: Resurrection flag already set, rejecting concurrent attempt",
            m_bot ? m_bot->GetName() : "nullptr");
        return false;
    }
    
    // Update last attempt timestamp atomically
    _lastResurrectionAttemptMs.store(now, ::std::memory_order_release);
    
    // RAII guard to reset resurrection flag on exit (exception-safe)
    struct ResurrectionGuard {
        ::std::atomic<bool>& flag;
        ~ResurrectionGuard() { flag.store(false, ::std::memory_order_release); }
    } guard{_resurrectionInProgress};

    // Match TrinityCore's HandleReclaimCorpse validation checks
    if (m_bot->IsAlive())
    {
        TC_LOG_WARN("playerbot.death", " Bot {} already alive, skipping corpse interaction", m_bot->GetName());
        return true; // Not an error, just already alive
    }

    if (m_bot->InArena())
    {
        TC_LOG_DEBUG("playerbot.death", "Bot {} in arena, cannot resurrect at corpse", m_bot->GetName());
        return false;
    }

    // OPTION 3 REFACTOR: No longer check Ghost flag manually
    // TrinityCore's HandleReclaimCorpse will validate the ghost state internally
    // We trust TrinityCore to handle resurrection validation

    Corpse* corpse = m_bot->GetCorpse();
    if (!corpse)
    {
        TC_LOG_ERROR("playerbot.death", " Bot {} has no corpse!", m_bot->GetName());
        return false;
    }

    // Check corpse reclaim delay (30-second delay after death in PvP)
    time_t ghostTime = corpse->GetGhostTime();
    time_t reclaimDelay = m_bot->GetCorpseReclaimDelay(corpse->GetType() == CORPSE_RESURRECTABLE_PVP);
    time_t currentTime = GameTime::GetGameTime();

    if (time_t(ghostTime + reclaimDelay) > currentTime)
    {
        time_t remainingDelay = (ghostTime + reclaimDelay) - currentTime;
        TC_LOG_WARN("playerbot.death", "⏳ Bot {} corpse reclaim delay BLOCKING resurrection: {} seconds remaining (ghostTime={}, delay={}, current={})",
            m_bot->GetName(), remainingDelay, ghostTime, reclaimDelay, currentTime);
        return false; // Must wait for delay to expire
    }
    else
    {
        TC_LOG_INFO("playerbot.death", " Bot {} corpse reclaim delay check PASSED (ghostTime={}, delay={}, current={})",
            m_bot->GetName(), ghostTime, reclaimDelay, currentTime);
    }

    if (!IsInCorpseRange())
    {
        TC_LOG_ERROR("playerbot.death", " Bot {} InteractWithCorpse FAILED: Not in corpse range! Distance: {:.1f} yards (need <= {})",
            m_bot->GetName(), m_corpseDistance, CORPSE_RESURRECTION_RANGE);
        return false;
    }

    TC_LOG_WARN("playerbot.death", "  Bot {} using TrinityCore-mirrored resurrection at corpse (distance: {:.1f}y, deathState BEFORE: {})...",
        m_bot->GetName(), m_corpseDistance, static_cast<int>(m_bot->getDeathState()));

    // CRITICAL FIX: Mirror TrinityCore's HandleReclaimCorpse logic (MiscHandler.cpp:417-446)
    // We can't use the packet handler directly because WorldPackets::Misc::ReclaimCorpse has no default constructor
    // Instead, we replicate the EXACT logic from HandleReclaimCorpse:
    //
    // Handler logic (MiscHandler.cpp:442-445):
    //   _player->ResurrectPlayer(_player->InBattleground() ? 1.0f : 0.5f);
    //   _player->SpawnCorpseBones();
    //
    // This is the PROVEN, tested resurrection flow in TrinityCore
    // OPTION 3 REFACTOR: ResurrectPlayer() automatically handles:
    // - setDeathState(ALIVE) which properly transitions death state
    // - UpdateObjectVisibility() which fixes visibility issues
    // - RemovePlayerFlag(PLAYER_FLAGS_GHOST) via RemoveAurasDueToSpell(8326)
    // - All Ghost aura (spell 8326) lifecycle management
    // We never touch Ghost aura or flag - TrinityCore handles everything
    //
    // Clear movement before resurrection to prevent conflicts (like mod-playerbot does)
    m_bot->GetMotionMaster()->Clear();
    m_bot->StopMoving();

    // DIAGNOSTIC: Log health BEFORE resurrection
    uint32 healthBefore = m_bot->GetHealth();
    uint32 maxHealth = m_bot->GetMaxHealth();
    float restorePercent = m_bot->InBattleground() ? 1.0f : 0.5f;
    DeathState deathStateBefore = m_bot->getDeathState();

    TC_LOG_FATAL("playerbot.death", " Bot {} BEFORE ResurrectPlayer: Health={}/{}, RestorePercent={}, DeathState={}, IsAlive={}, IsGhost={}",
        m_bot->GetName(), healthBefore, maxHealth, restorePercent,
        static_cast<int>(deathStateBefore), m_bot->IsAlive(), IsGhost());

    // SAFE RESURRECTION (v9): Queue via BotSession::QueueSafeResurrection()
    // ============================================================================
    // CRITICAL FIX: DO NOT use CMSG_RECLAIM_CORPSE packet!
    // HandleReclaimCorpse → SpawnCorpseBones → Map::RemoveWorldObject crashes
    // due to corrupted i_worldObjects tree structure (infinite loop in _Erase).
    //
    // Instead, we use QueueSafeResurrection() which calls ResurrectPlayer() directly
    // on the main thread WITHOUT SpawnCorpseBones. The corpse will decay naturally.
    // ============================================================================

    BotSession* botSession = BotSessionManager::GetBotSession(m_bot->GetSession());
    if (!botSession)
    {
        TC_LOG_ERROR("playerbot.death", "Bot {} InteractWithCorpse FAILED: BotSession not found!",
            m_bot->GetName());
        return false;
    }

    // Queue safe resurrection (thread-safe atomic flag)
    botSession->QueueSafeResurrection();

    TC_LOG_WARN("playerbot.death", "Bot {} queued SAFE resurrection (distance: {:.1f}y, deathState: {}) - NO SpawnCorpseBones!",
        m_bot->GetName(), m_corpseDistance, static_cast<int>(deathStateBefore));

    return true;
}

bool DeathRecoveryManager::NavigateToSpiritHealer()
{
    if (!m_bot)
        return false;

    // PHASE 5D: Thread-safe spatial grid validation
    auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(m_bot, m_spiritHealerGuid);
    Creature* spiritHealer = nullptr;

    if (snapshot)
    {
        // Get Creature* for navigation (validated via snapshot first)
        spiritHealer = ObjectAccessor::GetCreature(*m_bot, m_spiritHealerGuid);
    }

    if (!spiritHealer)
        return false;

    // PHASE 3 MIGRATION: Use Movement Arbiter with DEATH_RECOVERY priority (255)
    // Death recovery has the ABSOLUTE HIGHEST priority - must override ALL other movement
    BotAI* botAI = dynamic_cast<BotAI*>(m_bot->GetAI());
    if (botAI && botAI->GetUnifiedMovementCoordinator())
    {
        Position spiritHealerPos(spiritHealer->GetPositionX(),
                                spiritHealer->GetPositionY(),
                                spiritHealer->GetPositionZ(),
                                spiritHealer->GetOrientation());

        // Use Movement Arbiter for priority-based arbitration
        bool accepted = botAI->RequestPointMovement(
            PlayerBotMovementPriority::DEATH_RECOVERY,  // Priority 255 - HIGHEST
            spiritHealerPos,
            "Moving to spirit healer - death recovery",
            "DeathRecoveryManager");

        if (accepted)
        {
            TC_LOG_DEBUG("playerbot.movement.arbiter",
                "DeathRecoveryManager: Bot {} requested spirit healer movement to ({:.2f}, {:.2f}, {:.2f}) with DEATH_RECOVERY priority (255)",
                m_bot->GetName(),
                spiritHealerPos.GetPositionX(),
                spiritHealerPos.GetPositionY(),
                spiritHealerPos.GetPositionZ());

            m_navigationActive = true;
            return true;
        }
        else
        {
            TC_LOG_WARN("playerbot.movement.arbiter",
                "DeathRecoveryManager: Bot {} spirit healer movement request FILTERED (duplicate detected)",
                m_bot->GetName());

            // Still consider navigation active if filtered (likely duplicate)
            m_navigationActive = true;
            return true;
        }
    }
    else
    {
        // FALLBACK: Direct MotionMaster call if arbiter not available
        // This should only happen during transition period
        TC_LOG_WARN("playerbot.movement.arbiter",
            "DeathRecoveryManager: Bot {} has no MovementArbiter - using legacy MovePoint() for spirit healer",
            m_bot->GetName());

        // CRITICAL FIX: Clear MotionMaster before calling MovePoint (like mod-playerbot does)
        // This prevents movement spam/cancellation that causes "teleporting" behavior
        MotionMaster* mm = m_bot->GetMotionMaster();
        mm->Clear();

        mm->MovePoint(0,
            spiritHealer->GetPositionX(),
            spiritHealer->GetPositionY(),
            spiritHealer->GetPositionZ(),
            true);  // generatePath = true for proper pathfinding

        m_navigationActive = true;
        return true;
    }
}

bool DeathRecoveryManager::InteractWithSpiritHealer()
{
    if (!m_bot || !CanInteractWithSpiritHealer())
        return false;

    // PHASE 5D: Thread-safe spatial grid validation
    auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(m_bot, m_spiritHealerGuid);
    Creature* spiritHealer = nullptr;

    if (snapshot)
    {
        // Get Creature* for interaction (validated via snapshot first)
        spiritHealer = ObjectAccessor::GetCreature(*m_bot, m_spiritHealerGuid);
    }

    if (!spiritHealer)
        return false;

    // Use interaction manager if available
    if (m_ai)
    {
        // This will trigger gossip and eventually SPIRIT_HEALER_CONFIRM event
        // which will call ExecuteGraveyardResurrection()
        TC_LOG_DEBUG("playerbot.death", "Bot {} initiating spirit healer gossip",
            m_bot->GetName());
    }

    // Fallback: direct resurrection
    return ExecuteGraveyardResurrection();
}

bool DeathRecoveryManager::ExecuteGraveyardResurrection()
{
    if (!m_bot)
        return false;

    // THREAD SAFETY FIX (v10): DO NOT call SendSpiritResurrect from worker thread!
    //
    // SendSpiritResurrect() modifies player state (ResurrectPlayer, DurabilityLossAll,
    // SpawnCorpseBones, TeleportTo) which causes Map.cpp:686 ACCESS_VIOLATION crash
    // when called from ThreadPool worker during Map::Update iteration.
    //
    // Instead, we set a flag that the main thread will check and execute.
    // The main thread path is BotWorldSessionMgr::ProcessPendingResurrections()
    // which runs AFTER Map::Update completes.
    //
    // IMPORTANT FIX: The previous packet-based approach using CMSG_REPOP_REQUEST
    // does NOT work for bots that are already ghosts. TrinityCore's HandleRepopRequest
    // handler (MiscHandler.cpp:62) returns early if player has PLAYER_FLAGS_GHOST.

    TC_LOG_INFO("playerbot.death", "Bot {} setting pending resurrection flag (main thread will execute)",
        m_bot->GetName());

    // Set flag for main thread to execute resurrection
    _pendingMainThreadResurrection.store(true, ::std::memory_order_release);

    return true;
}

bool DeathRecoveryManager::ExecutePendingMainThreadResurrection()
{
    // Check and clear the flag atomically
    bool expected = true;
    if (!_pendingMainThreadResurrection.compare_exchange_strong(expected, false, ::std::memory_order_acq_rel))
    {
        return false;  // No pending resurrection
    }

    if (!m_bot)
    {
        TC_LOG_ERROR("playerbot.death", "ExecutePendingMainThreadResurrection: Bot is null");
        return false;
    }

    WorldSession* session = m_bot->GetSession();
    if (!session)
    {
        TC_LOG_ERROR("playerbot.death", "Bot {} has no session, cannot perform spirit healer resurrection",
            m_bot->GetName());
        return false;
    }

    TC_LOG_INFO("playerbot.death", "Bot {} executing spirit healer resurrection via SendSpiritResurrect() [MAIN THREAD]",
        m_bot->GetName());

    // SAFE: Called from main thread - SendSpiritResurrect modifies player state
    // This properly resurrects the ghost:
    // 1. ResurrectPlayer(0.5f, true) - resurrects at 50% health with sickness
    // 2. DurabilityLossAll(0.25f, true) - applies 25% durability loss
    // 3. SpawnCorpseBones() - spawns bones at corpse location
    // 4. TeleportTo() if needed - teleports to appropriate graveyard
    session->SendSpiritResurrect();

    return true;
}

// ========================================================================
// STATE QUERIES
// ========================================================================

uint64 DeathRecoveryManager::GetTimeSinceDeath() const
{
    auto now = ::std::chrono::steady_clock::now();
    return ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - m_deathTime).count();
}

bool DeathRecoveryManager::IsGhost() const
{
    // OPTION 3 REFACTOR: Let TrinityCore manage Ghost state entirely
    // We only query the state, never modify it manually
    // TrinityCore automatically sets/clears PLAYER_FLAGS_GHOST via Ghost aura (spell 8326)
    return m_bot && m_bot->HasPlayerFlag(PLAYER_FLAGS_GHOST);
}

WorldLocation DeathRecoveryManager::GetCorpseLocation() const
{
    if (!m_bot)
        return WorldLocation();

    Corpse* corpse = m_bot->GetCorpse();
    if (!corpse)
        return WorldLocation();

    return WorldLocation(corpse->GetMapId(),
        corpse->GetPositionX(),
        corpse->GetPositionY(),
        corpse->GetPositionZ(),
        corpse->GetOrientation());
}

float DeathRecoveryManager::GetCorpseDistance() const
{
    return m_corpseDistance;
}

bool DeathRecoveryManager::IsInCorpseRange() const
{
    return m_corpseDistance >= 0.0f && m_corpseDistance <= CORPSE_RESURRECTION_RANGE;
}

Creature* DeathRecoveryManager::FindNearestSpiritHealer() const
{
    if (!m_bot)
        return nullptr;

    ::std::list<Creature*> spiritHealers;
    Trinity::AllCreaturesOfEntryInRange checker(m_bot, 0, m_config.spiritHealerSearchRadius);
    Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(m_bot, spiritHealers, checker);
    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = m_bot->GetMap();
    if (!map)
        return nullptr;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return nullptr;
    }

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        m_bot->GetPosition(), m_config.spiritHealerSearchRadius);

    // Process results (replace old loop)
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5D: Thread-safe spatial grid validation
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(m_bot, guid);
        Creature* entity = nullptr;

        if (snapshot)
        {
            // Get Creature* for spirit healer check (validated via snapshot first)
            entity = ObjectAccessor::GetCreature(*m_bot, guid);
        }

        if (!entity || !entity->IsAlive())
            continue;

        // CRITICAL FIX: Filter for spirit healers and add to list
        // BUG: Previous code never populated spiritHealers list, causing FindNearestSpiritHealer() to always return nullptr
        // This is why bots got stuck in FINDING_SPIRIT_HEALER state (state 7) without resurrecting
        uint64 npcFlags = entity->GetNpcFlags();
        if ((npcFlags & UNIT_NPC_FLAG_SPIRIT_HEALER) || (npcFlags & UNIT_NPC_FLAG_AREA_SPIRIT_HEALER))
        {
            spiritHealers.push_back(entity);
        }
    }
    // End of spatial grid fix

    Creature* nearest = nullptr;
    float nearestDist = m_config.spiritHealerSearchRadius;

    for (Creature* creature : spiritHealers)
    {
        if (!creature || !creature->IsAlive())
            continue;

        uint64 npcFlags = creature->GetNpcFlags();
        if (!(npcFlags & UNIT_NPC_FLAG_SPIRIT_HEALER) &&
            !(npcFlags & UNIT_NPC_FLAG_AREA_SPIRIT_HEALER))
            continue;

        float dist = m_bot->GetDistance(creature);
        if (dist < nearestDist)
        {
            nearest = creature;
            nearestDist = dist;
        }
    }

    return nearest;
}

bool DeathRecoveryManager::CanInteractWithSpiritHealer() const
{
    if (!m_bot || m_spiritHealerGuid.IsEmpty())
        return false;

    // PHASE 5D: Thread-safe spatial grid validation
    auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(m_bot, m_spiritHealerGuid);
    Creature* spiritHealer = nullptr;

    if (snapshot && snapshot->IsAlive())
    {
        // Get Creature* for distance check (validated via snapshot first)
        spiritHealer = ObjectAccessor::GetCreature(*m_bot, m_spiritHealerGuid);
    }

    if (!spiritHealer || !spiritHealer->IsAlive())
        return false;

    float distance = m_bot->GetDistance(spiritHealer);
    return distance <= SPIRIT_HEALER_INTERACTION_RANGE;
}

// ========================================================================
// RESURRECTION CONTROL
// ========================================================================

bool DeathRecoveryManager::TriggerCorpseResurrection()
{
    // No lock needed - death recovery state is per-bot instance data
    if (m_state == DeathRecoveryState::NOT_DEAD)
        return false;

    m_method = ResurrectionMethod::CORPSE_RUN;
    TransitionToState(DeathRecoveryState::RUNNING_TO_CORPSE, "Manual corpse resurrection");
    return true;
}

bool DeathRecoveryManager::TriggerSpiritHealerResurrection()
{
    // No lock needed - death recovery state is per-bot instance data
    if (m_state == DeathRecoveryState::NOT_DEAD)
        return false;

    m_method = ResurrectionMethod::SPIRIT_HEALER;
    TransitionToState(DeathRecoveryState::FINDING_SPIRIT_HEALER, "Manual spirit healer resurrection");
    return true;
}

bool DeathRecoveryManager::AcceptBattleResurrection(ObjectGuid casterGuid, uint32 /*spellId*/)
{
    if (!m_config.allowBattleResurrection)
        return false;

    if (m_state == DeathRecoveryState::NOT_DEAD)
        return false;

    m_method = ResurrectionMethod::BATTLE_RESURRECTION;
    TransitionToState(DeathRecoveryState::RESURRECTING, "Accepting battle resurrection");

    TC_LOG_INFO("playerbot.death", "Bot {} accepting battle resurrection from {}",
        m_bot->GetName(), casterGuid.ToString());

    return true;
}

bool DeathRecoveryManager::ForceResurrection(ResurrectionMethod method)
{
    // GHOST AURA FIX: Mutex protection to prevent concurrent resurrection attempts
    ::std::unique_lock<::std::timed_mutex> lock(_resurrectionMutex, ::std::chrono::milliseconds(100));
    if (!lock.owns_lock())
    {
        TC_LOG_WARN("playerbot.death", " Bot {} ForceResurrection: Resurrection already in progress, skipping concurrent attempt",
            m_bot ? m_bot->GetName() : "nullptr");
        return false;
    }

    // GHOST AURA FIX: Check atomic resurrection flag
    bool expectedFalse = false;
    if (!_resurrectionInProgress.compare_exchange_strong(expectedFalse, true))
    {
        TC_LOG_WARN("playerbot.death", " Bot {} ForceResurrection: Resurrection flag already set, rejecting concurrent attempt",
            m_bot ? m_bot->GetName() : "nullptr");
        return false;
    }

    // RAII guard to reset resurrection flag on exit
    struct ResurrectionGuard {
        ::std::atomic<bool>& flag;
        ~ResurrectionGuard() { flag.store(false); }
    } guard{_resurrectionInProgress};

    if (!m_bot)
        return false;

    TC_LOG_WARN("playerbot.death", "Bot {} force resurrection via {}",
        m_bot->GetName(),
        method == ResurrectionMethod::CORPSE_RUN ? "corpse" : "spirit healer");

    // SAFE RESURRECTION (v9): Force resurrect via BotSession::QueueSafeResurrection()
    // ============================================================================
    // CRITICAL FIX: DO NOT use CMSG_RECLAIM_CORPSE packet!
    // HandleReclaimCorpse → SpawnCorpseBones → Map::RemoveWorldObject crashes.
    // ============================================================================

    BotSession* botSession = BotSessionManager::GetBotSession(m_bot->GetSession());
    Corpse* corpse = m_bot->GetCorpse();

    if (corpse && botSession)
    {
        // Use safe resurrection (bypasses SpawnCorpseBones crash)
        botSession->QueueSafeResurrection();

        TC_LOG_INFO("playerbot.death", "Bot {} queued SAFE force resurrection (method: {}) - NO SpawnCorpseBones!",
            m_bot->GetName(), static_cast<int>(method));
    }
    else if (!corpse)
    {
        // No corpse - use CMSG_REPOP_REQUEST to resurrect at graveyard
        // This path is safe as it doesn't involve SpawnCorpseBones
        // TrinityCore 12.0: QueuePacket now takes WorldPacket&& instead of WorldPacket*
        WorldPacket repopPacket(CMSG_REPOP_REQUEST, 1);
        repopPacket << uint8(0);
        m_bot->GetSession()->QueuePacket(std::move(repopPacket));

        TC_LOG_INFO("playerbot.death", "Bot {} queued CMSG_REPOP_REQUEST for force resurrection (no corpse)",
            m_bot->GetName());
    }
    else
    {
        TC_LOG_ERROR("playerbot.death", "Bot {} force resurrection FAILED: BotSession not found!",
            m_bot->GetName());
        return false;
    }

    OnResurrection();
    return true;
}

// ========================================================================
// CONFIGURATION
// ========================================================================

void DeathRecoveryManager::SetConfig(DeathRecoveryConfig const& config)
{
    // No lock needed - death recovery state is per-bot instance data
    m_config = config;
}

void DeathRecoveryManager::ReloadConfig()
{
    // No lock needed - death recovery state is per-bot instance data
    m_config = DeathRecoveryConfig::LoadFromConfig();
    TC_LOG_INFO("playerbot.death", "Death recovery configuration reloaded");
}

// ========================================================================
// STATISTICS
// ========================================================================

void DeathRecoveryManager::ResetStatistics()
{
    // No lock needed - death recovery state is per-bot instance data
    m_stats = DeathRecoveryStatistics();
}

void DeathRecoveryManager::LogStatistics() const
{
    TC_LOG_INFO("playerbot.death", "Bot {} death recovery statistics:\n{}",
        m_bot ? m_bot->GetName() : "nullptr",
        m_stats.ToString());
}

// ========================================================================
// VALIDATION & ERROR HANDLING
// ========================================================================

bool DeathRecoveryManager::ValidateBotState() const
{
    if (!m_bot->IsInWorld())
    {
        TC_LOG_ERROR("playerbot.death", "Bot {} is not in world", m_bot->GetName());
        return false;
    }

    return true;
}

bool DeathRecoveryManager::IsResurrectionTimedOut() const
{
    return GetTimeSinceDeath() > m_config.resurrectionTimeout;
}

void DeathRecoveryManager::HandleResurrectionFailure(::std::string const& reason)
{
    TC_LOG_ERROR("playerbot.death", "Bot {} resurrection failed: {}",
        m_bot ? m_bot->GetName() : "nullptr", reason);

    TransitionToState(DeathRecoveryState::RESURRECTION_FAILED, reason);
}

void DeathRecoveryManager::TransitionToState(DeathRecoveryState newState, ::std::string const& reason)
{
    DeathRecoveryState oldState = m_state.load();
    m_state = newState;
    m_lastStateTransition = ::std::chrono::steady_clock::now();
    m_stateTimer = 0;

    if (m_config.logDebugInfo)
    {
        TC_LOG_DEBUG("playerbot.death", "Bot {} death recovery: {} -> {} ({})",
            m_bot ? m_bot->GetName() : "nullptr",
            static_cast<int>(oldState),
            static_cast<int>(newState),
            reason);
    }
}

// ========================================================================
// HELPER METHODS
// ========================================================================

void DeathRecoveryManager::UpdateCorpseDistance() const
{
    WorldLocation corpseLocation = GetCorpseLocation();

    if (corpseLocation.GetMapId() == MAPID_INVALID || !m_bot)
    {
        m_corpseDistance = -1.0f;
        return;
    }

    if (corpseLocation.GetMapId() != m_bot->GetMapId())
    {
        m_corpseDistance = -1.0f; // Different map
        return;
    }

    m_corpseDistance = m_bot->GetExactDist(
        corpseLocation.GetPositionX(),
        corpseLocation.GetPositionY(),
        corpseLocation.GetPositionZ());

    m_lastCorpseDistanceCheck = ::std::chrono::steady_clock::now();
}

bool DeathRecoveryManager::IsInSpecialZone() const
{
    return m_bot && (m_bot->InBattleground() || m_bot->InArena() || (m_bot->GetMap() && m_bot->GetMap()->IsDungeon()));
}

bool DeathRecoveryManager::WillReceiveResurrectionSickness() const
{
    if (!m_bot)
        return false;

    // Resurrection sickness only applies to players level 11+
    return m_bot->GetLevel() > 10 && m_method == ResurrectionMethod::SPIRIT_HEALER;
}

void DeathRecoveryManager::LogDebug(::std::string const& message) const
{
    if (m_config.logDebugInfo)
    {
        TC_LOG_DEBUG("playerbot.death", "Bot {}: {}",
            m_bot ? m_bot->GetName() : "nullptr", message);
    }
}

} // namespace Playerbot
