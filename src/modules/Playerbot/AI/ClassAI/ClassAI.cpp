/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * REFACTORED ClassAI Implementation - Combat Only
 *
 * This implementation provides:
 * 1. Combat-only specialization without interfering with base behaviors
 * 2. No movement control - delegated to BotAI strategies
 * 3. No throttling that breaks following
 * 4. Clean integration with BotAI update chain
 */

#include "ClassAI.h"
#include "GameTime.h"
#include "Movement/BotMovementUtil.h"
#include "ActionPriority.h"
#include "Common/CooldownManager.h"
#include "ResourceManager.h"
#include "../Combat/CombatBehaviorIntegration.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Map.h"
#include "Object.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Group.h"
#include "SpellAuras.h"
#include "SpellHistory.h"
#include "MotionMaster.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Cell.h"
#include "CellImpl.h"
#include "../../Spatial/SpatialGridManager.h"
#include <chrono>
#include "../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries
#include "../../Packets/SpellPacketBuilder.h"  // PHASE 0 WEEK 3: Packet-based spell casting
#include "BaselineRotationManager.h"  // Centralized baseline rotation for low-level bots
#include "../../Session/BotSession.h"  // For thread-safe facing via QueueFacingTarget

namespace Playerbot
{

// Static instance of BaselineRotationManager for low-level bot handling
// Initialized once, thread-safe for read operations
static BaselineRotationManager s_baselineRotationManager;

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

ClassAI::ClassAI(Player* bot) : BotAI(bot),
    _currentCombatTarget(nullptr),
    _inCombat(false),
    _combatTime(0),
    _lastTargetSwitch(0)
{
    // Initialize component managers for class-specific mechanics
    _actionQueue = ::std::make_unique<ActionPriorityQueue>();
    _cooldownManager = ::std::make_unique<CooldownManager>();
    _resourceManager = ::std::make_unique<ResourceManager>(bot);

    // Initialize unified combat behavior system
    // This provides advanced combat coordination across all managers
    // CRITICAL: Do NOT access bot->GetName() in constructor!
    // Bot's internal data (m_name) is not initialized during constructor chain.
    // Accessing it causes ACCESS_VIOLATION crash in string construction.
    try {
        _combatBehaviors = ::std::make_unique<CombatBehaviorIntegration>(bot);
        // Logging with GetName() deferred to first Update() when bot IsInWorld()
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("playerbot.classai", "Failed to initialize CombatBehaviorIntegration: {}",
                     e.what());
        _combatBehaviors = nullptr;
    }

    // ClassAI creation logging deferred to first Update() when bot IsInWorld()
}

ClassAI::~ClassAI() = default;

// ============================================================================
// COMBAT UPDATE - Called by BotAI when in combat
// ============================================================================

void ClassAI::OnCombatUpdate(uint32 diff)
{
    // CRITICAL: This method is called BY BotAI::UpdateAI() when in combat
    // It does NOT replace UpdateAI(), it extends it for combat only
    if (!GetBot() || !GetBot()->IsAlive())
        return;

    // CRITICAL FIX: Safety check for worker thread access during bot destruction
    // CombatManager::SetInCombatWith crashes when accessing combat refs if bot
    // has been removed from the world but pointer is still valid
    if (!GetBot()->IsInWorld())
        return;

    // CRITICAL: Execute pending spell if ready (every frame, like players)
    // This mirrors Player::Update() which calls CanExecutePendingSpellCastRequest()
    // and ExecutePendingSpellCastRequest() every frame - see Player.cpp:946-947

    // DIAGNOSTIC: Log spell queue check
    // QW-4 FIX: Removed static - now uses instance member _lastSpellQueueLog
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastSpellQueueLog > 500) // Every 500ms
    {
        TC_LOG_ERROR("module.playerbot.classai", " OnCombatUpdate: Checking spell queue for bot {} - hasPending={}",
                    GetBot()->GetName(), _pendingSpellCastRequest != nullptr);
        _lastSpellQueueLog = currentTime;
    }

    if (CanExecutePendingSpell())
    {
        TC_LOG_ERROR("module.playerbot.classai", " OnCombatUpdate: Calling ExecutePendingSpell for bot {}",
                    GetBot()->GetName());
        ExecutePendingSpell();
    }

    // Update unified combat behavior system
    // This manages all advanced combat behaviors including interrupts, defensive actions,
    // crowd control, target prioritization, and emergency responses
    if (_combatBehaviors)
    {
        _combatBehaviors->Update(diff);
    }

    // DIAGNOSTIC: Log that OnCombatUpdate is being called
    // QW-4 FIX: Removed static - now uses instance member _lastCombatLog
    if (currentTime - _lastCombatLog > 2000) // Every 2 seconds (reuse currentTime from spell queue check)
    {
        TC_LOG_ERROR("module.playerbot", " ClassAI::OnCombatUpdate: Bot {} - currentTarget={}, combatTime={}ms, behaviors={}",
                     GetBot()->GetName(),
                     _currentCombatTarget ? _currentCombatTarget->GetName() : "NONE",
                     _combatTime,
                     _combatBehaviors ? "active" : "inactive");
        _lastCombatLog = currentTime;
    }

    // Update component managers
    _cooldownManager->Update(diff);
    _combatTime += diff;

    // Update combat state
    UpdateCombatState(diff);

    // Update targeting - select best target
    // If combat behaviors suggest a priority target, use it
    if (_combatBehaviors)
    {
        if (Unit* priorityTarget = _combatBehaviors->GetPriorityTarget())
        {
            if (priorityTarget != _currentCombatTarget && GetBot()->IsValidAttackTarget(priorityTarget))
            {
                OnTargetChanged(priorityTarget);
            }
        }
        else
        {
            UpdateTargeting();
        }
    }
    else
    {
        UpdateTargeting();
    }

    // Check for emergency actions from combat behavior system
    // These take priority over normal rotation (interrupts, defensives, etc.)
    if (_combatBehaviors && _combatBehaviors->HandleEmergencies())
    {
        // Emergency action was taken, skip normal rotation this update
        TC_LOG_DEBUG("playerbot.classai", "Bot {} handled emergency action, skipping rotation",
                     GetBot()->GetName());
        return;
    }

    // Check for high-priority combat behaviors before rotation
    if (_combatBehaviors && _combatBehaviors->HasPendingAction())
    {
        RecommendedAction action = _combatBehaviors->GetNextAction();
        if (RequiresImmediateAction(action.urgency))
        {
            bool executed = ExecuteRecommendedAction(action);
            _combatBehaviors->RecordActionResult(action, executed);

            if (executed && IsEmergencyAction(action.urgency))
            {
                // Emergency or critical action executed, skip normal rotation
                TC_LOG_DEBUG("playerbot.classai", "Bot {} executed {} urgency action: {} ({})",
                             GetBot()->GetName(), GetUrgencyName(action.urgency),
                             GetActionName(action.type), action.reason.c_str());
                return;
            }
        }
    }

    // Class-specific combat updates
    // CRITICAL FIX: Safety check for worker thread access during target destruction
    // CombatManager::SetInCombatWith crashes when accessing _pvpRefs map if target
    // has been removed from the world but pointer is still valid
    if (_currentCombatTarget && _currentCombatTarget->IsInWorld())
    {
        TC_LOG_TRACE("module.playerbot", "Calling UpdateRotation for {} (class {}) with target {}",
                     GetBot()->GetName(), GetBot()->GetClass(), _currentCombatTarget->GetName());

        // ========================================================================
        // CENTRALIZED BASELINE ROTATION CHECK - Enterprise-grade dispatch logic
        // ========================================================================
        // This is the SINGLE point where we decide whether to use baseline rotation
        // or specialization-specific rotation. By checking here at the dispatch level:
        //
        // 1. ALL spec classes automatically get baseline handling - no code duplication
        // 2. Spec classes can assume they only receive calls when bot has talents
        // 3. Single source of truth for the "should use baseline?" decision
        // 4. Clean separation: BaselineRotationManager handles levels 1-9,
        //    spec classes handle level 10+ with talents
        //
        // The baseline rotation should be used when:
        // - Bot is level < 10 (no specialization available)
        // - Bot is level 10+ but hasn't chosen a specialization yet
        //
        // This prevents the bug where spec-specific AI (like AfflictionWarlockRefactored)
        // would try to use abilities the bot doesn't have yet.
        // ========================================================================

        // ========================================================================
        // THREAD-SAFE FACING - Queue facing request for main thread execution
        // ========================================================================
        // SetFacingToObject() manipulates movement splines which is NOT thread-safe.
        // This code runs on worker threads (ThreadPool) but movement APIs require main thread.
        //
        // Solution: Queue the facing request via BotSession::QueueFacingTarget()
        // The main thread (BotWorldSessionMgr::ProcessAllDeferredPackets) will call
        // BotSession::ProcessPendingFacing() to safely execute SetFacingToObject().
        //
        // CRITICAL FIX (2024-12-16): Do NOT call QueueFacingTarget when chasing!
        // ChaseMovementGenerator already handles facing the target automatically.
        // Calling SetFacingToObject while chasing manipulates movement splines,
        // interfering with the smooth chase motion and causing stuttering.
        // This was the ROOT CAUSE of low-level bot combat stuttering.
        // ========================================================================
        // Note: IsInWorld() check already done at outer scope (line 198)
        {
            // Check if bot is already chasing - ChaseMovementGenerator handles facing
            ::MotionMaster* motionMaster = GetBot()->GetMotionMaster();
            ::MovementGeneratorType currentMotion = motionMaster ?
                motionMaster->GetCurrentMovementGeneratorType(MOTION_SLOT_ACTIVE) : IDLE_MOTION_TYPE;

            // Only manually face target when NOT chasing (e.g., standing still casting)
            // Chase movement already includes automatic facing toward target
            if (currentMotion != CHASE_MOTION_TYPE)
            {
                // Get the BotSession to queue facing request
                WorldSession* session = GetBot()->GetSession();
                if (BotSession* botSession = dynamic_cast<BotSession*>(session))
                {
                    botSession->QueueFacingTarget(_currentCombatTarget->GetGUID());
                }
            }
        }

        if (BaselineRotationManager::ShouldUseBaselineRotation(GetBot()))
        {
            TC_LOG_DEBUG("module.playerbot.baseline",
                         "Bot {} (level {}, class {}) using BASELINE rotation - no spec yet",
                         GetBot()->GetName(), GetBot()->GetLevel(), GetBot()->GetClass());

            // Handle auto-specialization attempt at level 10
            // This will select an optimal spec when the bot reaches level 10
            s_baselineRotationManager.HandleAutoSpecialization(GetBot());

            // Execute the baseline rotation (class-appropriate abilities for levels 1-9)
            bool executed = s_baselineRotationManager.ExecuteBaselineRotation(GetBot(), _currentCombatTarget);

            if (!executed)
            {
                TC_LOG_TRACE("module.playerbot.baseline",
                             "Bot {} baseline rotation returned false - relying on auto-attack",
                             GetBot()->GetName());
            }

            // Skip the specialized rotation - baseline handles everything for low-level bots
            // Update cooldowns still applies (shared functionality)
            UpdateCooldowns(diff);
            return;
        }

        // ========================================================================
        // SPECIALIZATION ROTATION - Bot has talents, use spec-specific AI
        // ========================================================================

        // NOTE: Combat movement (chase, positioning, range management) is delegated to
        // CombatMovementStrategy which provides superior role-based positioning.
        // ClassAI now focuses solely on ability rotation and cooldown management.

        TC_LOG_TRACE("module.playerbot", "Bot {} calling specialized UpdateRotation()",
                     GetBot()->GetName());

        // Update class-specific rotation (spec classes can assume talents exist)
        try
        {
            UpdateRotation(_currentCombatTarget);
        }
        catch (::std::exception const& e)
        {
            TC_LOG_ERROR("module.playerbot", "EXCEPTION in UpdateRotation for {}: {}",
                         GetBot()->GetName(), e.what());
        }
        catch (...)
        {
            TC_LOG_ERROR("module.playerbot", "UNKNOWN EXCEPTION in UpdateRotation for {}",
                         GetBot()->GetName());
        }

        // Update class-specific cooldowns
        UpdateCooldowns(diff);
    }
    else
    {
        TC_LOG_ERROR("module.playerbot", " NO TARGET in combat for {}, applying buffs instead", GetBot()->GetName());

        // No target in combat - try to apply buffs
        UpdateBuffs();
    }
}

// ============================================================================
// NON-COMBAT UPDATE - Called when bot is NOT in combat
// ============================================================================

void ClassAI::OnNonCombatUpdate(uint32 /*diff*/)
{
    Player* bot = GetBot();
    if (!bot || !bot->IsAlive())
        return;

    // Don't do anything while casting (e.g., pet summon has 6s cast time)
    if (bot->HasUnitState(UNIT_STATE_CASTING))
        return;

    // ========================================================================
    // BASELINE BOT HANDLING (levels 1-9 or no specialization)
    // ========================================================================
    // CRITICAL FIX: Low-level bots need ApplyBaselineBuffs() called out of combat
    // for pet summoning (warlocks), buff application, and other preparation.
    // This was completely missing before, causing warlock pets to never summon!
    if (BaselineRotationManager::ShouldUseBaselineRotation(bot))
    {
        TC_LOG_DEBUG("module.playerbot.baseline",
                     "Bot {} (level {}) applying BASELINE buffs out of combat",
                     bot->GetName(), bot->GetLevel());

        s_baselineRotationManager.ApplyBaselineBuffs(bot);
        return; // Baseline handling complete
    }

    // ========================================================================
    // SPECIALIZED BOT HANDLING (level 10+ with specialization)
    // ========================================================================
    // For spec bots, call the virtual UpdateBuffs() which is overridden per class
    // Example: AfflictionWarlockRefactored::UpdateBuffs() calls EnsurePetActive()
    UpdateBuffs();
}

// ============================================================================
// COMBAT STATE MANAGEMENT// ============================================================================

void ClassAI::OnCombatStart(::Unit* target)
{
    // Called by BotAI when entering combat
    _inCombat = true;
    _combatTime = 0;
    _currentCombatTarget = target;
    TC_LOG_DEBUG("playerbot.classai", "Bot {} entering combat with {}",
                 GetBot()->GetName(), target ? target->GetName() : "unknown");

    // CRITICAL FIX: Initiate auto-attack when entering combat
    // In WoW, casting spells does NOT automatically start auto-attack
    // Must explicitly call Attack() to begin melee/ranged auto-attacking
    if (target && GetBot())
    {
        if (!GetBot()->IsInCombatWith(target))
        {
            GetBot()->SetInCombatWith(target);
            target->SetInCombatWith(GetBot());
        }

        // Start auto-attack (true = melee, but works for ranged too)
        GetBot()->Attack(target, true);

        TC_LOG_DEBUG("playerbot.classai", "Bot {} initiated auto-attack on {}",                     GetBot()->GetName(), target->GetName());
    }

    // Notify combat behavior system
    if (_combatBehaviors)
    {
        _combatBehaviors->OnCombatStart();
        TC_LOG_DEBUG("playerbot.classai", "CombatBehaviorIntegration notified of combat start for bot {}",
                     GetBot()->GetName());
    }

    // Let BotAI handle base combat start logic
    BotAI::OnCombatStart(target);
}

void ClassAI::OnCombatEnd()
{
    // Called by BotAI when leaving combat
    _inCombat = false;
    _combatTime = 0;
    _currentCombatTarget = nullptr;

    TC_LOG_DEBUG("playerbot.classai", "Bot {} leaving combat", GetBot()->GetName());

    // Notify combat behavior system
    if (_combatBehaviors)
    {
        _combatBehaviors->OnCombatEnd();
        TC_LOG_DEBUG("playerbot.classai", "CombatBehaviorIntegration notified of combat end for bot {}",
                     GetBot()->GetName());
    }

    // Let BotAI handle base combat end logic
    BotAI::OnCombatEnd();
}

void ClassAI::OnTargetChanged(::Unit* newTarget)
{
    if (!newTarget)
    {
        return;
    }

    _currentCombatTarget = newTarget;
    _lastTargetSwitch = _combatTime;

    TC_LOG_DEBUG("playerbot.classai", "Bot {} switching target to {}",
                 GetBot()->GetName(), newTarget ? newTarget->GetName() : "none");

    // FIX FOR ISSUE #3: Explicitly set facing for melee combat
    // Melee bots must face their target to attack properly
    if (newTarget && GetBot())
    {
        float optimalRange = GetOptimalRange(newTarget);
                if (!newTarget)
                {
                    return;
                }

        // Melee classes (optimal range <= 5 yards) need to face target
    if (optimalRange <= 5.0f)
        {
            GetBot()->SetFacingToObject(newTarget);
            TC_LOG_TRACE("module.playerbot.classai",
                "Bot {} (melee) now facing target {} (FIX FOR ISSUE #3)",
                GetBot()->GetName(), newTarget->GetName());
        }
    }}

// ============================================================================
// TARGETING
// ============================================================================

void ClassAI::UpdateTargeting()
{
    // Select best combat target
    ::Unit* bestTarget = GetBestAttackTarget();

    if (bestTarget != _currentCombatTarget)
    {
        OnTargetChanged(bestTarget);
    }
}::Unit* ClassAI::GetBestAttackTarget()
{
    if (!GetBot())
        return nullptr;

    // Priority 1: Current victim
    if (::Unit* victim = GetBot()->GetVictim())        return victim;

    // Priority 2: Group leader's target (FIX FOR ISSUE #2)
    // Bots should assist the group leader's target for coordinated combat
    if (Group* group = GetBot()->GetGroup())
    {
        ObjectGuid leaderGuid = group->GetLeaderGUID();        // Find leader in group members (avoid ObjectAccessor for thread safety)
    for (GroupReference const& itr : group->GetMembers())
        {
            if (Player* member = itr.GetSource())
            {
                if (member->GetGUID() == leaderGuid)                {
                    // Found leader - get their target
    if (::Unit* leaderTarget = member->GetVictim())                    {
                        if (GetBot()->IsValidAttackTarget(leaderTarget))
                        {
                            TC_LOG_TRACE("module.playerbot.classai",
                                "Bot {} assisting leader {} target: {} (FIX FOR ISSUE #2)",
                                GetBot()->GetName(), member->GetName(), leaderTarget->GetName());
                            return leaderTarget;
                        }
                    }
                    break;
                }
            }
        }
    }

    // Priority 3: Selected target (if different from victim)
    // FIX #20: Avoid ObjectAccessor::GetUnit() to prevent TrinityCore deadlock
    // Check if bot has a selected target that's not the current victim
    ObjectGuid targetGuid = GetBot()->GetTarget();
    if (!targetGuid.IsEmpty())
    {
        // Check if victim matches selected target (no ObjectAccessor needed)
    if (::Unit* victim = GetBot()->GetVictim())
        {
            if (victim->GetGUID() == targetGuid && GetBot()->IsValidAttackTarget(victim))                return victim;
        }
        // Selected target is different from victim - skip to avoid ObjectAccessor call
        // GetNearestEnemy() will handle finding a new target
    }

    // Priority 4: Nearest hostile
    return GetNearestEnemy();
}

::Unit* ClassAI::GetNearestEnemy(float maxRange)
{
    if (!GetBot())
        return nullptr;

    ::Unit* nearestEnemy = nullptr;
    float nearestDistanceSq = maxRange * maxRange; // Use squared distance for comparison

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = GetBot()->GetMap();
    if (!map)
        return nullptr;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        // Grid not yet created for this map - create it on demand
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return nullptr;
    }    // Query nearby creature GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        GetBot()->GetPosition(), maxRange);

    // Resolve GUIDs to Unit pointers and find nearest enemy
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_target = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), guid);
        ::Unit* target = nullptr;
        if (snapshot_target)
        {
            // CRITICAL FIX: Actually assign the target pointer!
            // BUG: Previous code retrieved snapshot but never assigned target variable
            // This caused target to always be nullptr, making bots unable to find ANY targets
            // This is why 2.4M "in combat but no victim" warnings occurred - target selection was broken!
            target = ObjectAccessor::GetCreature(*GetBot(), guid);
        }
        if (!target || !GetBot()->IsValidAttackTarget(target))
            continue;

        float distanceSq = GetBot()->GetExactDistSq(target);
        if (distanceSq < nearestDistanceSq)
        {
            nearestDistanceSq = distanceSq;
            nearestEnemy = target;
        }
    }

    return nearestEnemy;
}

// ============================================================================
// HEALING SUPPORT
// ============================================================================

::Unit* ClassAI::GetBestHealTarget()
{
    if (!GetBot() || !GetBot()->GetGroup())
        return GetBot(); // Self-heal if not in group

    ::Unit* lowestHealthTarget = nullptr;
    float lowestHealthPct = 100.0f;

    // Check group members
    Group* group = GetBot()->GetGroup();
    for (GroupReference const& itr : group->GetMembers())
    {
        if (Player* member = itr.GetSource())        {
            if (!member->IsAlive() || !member->IsWithinDistInMap(GetBot(), 40.0f))
                continue;

            float healthPct = member->GetHealthPct();
            if (healthPct < lowestHealthPct)
            {
                lowestHealthPct = healthPct;
                lowestHealthTarget = member;
            }
        }
    }

    return lowestHealthTarget ? lowestHealthTarget : GetBot();
}

::Unit* ClassAI::GetLowestHealthAlly(float maxRange)
{
    // Similar to GetBestHealTarget but with range parameter
    return GetBestHealTarget();
}

// ============================================================================
// COMBAT STATE TRACKING
// ============================================================================

void ClassAI::UpdateCombatState(uint32 diff)
{
    // Track combat metrics and state
    // This is for internal class AI tracking, not for movement or base behaviors
}

// ============================================================================
// COOLDOWN MANAGEMENT
// ============================================================================

void ClassAI::UpdateCooldowns(uint32 diff)
{
    // Default implementation - derived classes can override
    // This is for tracking class-specific ability cooldowns
}

bool ClassAI::CanUseAbility(uint32 spellId)
{
    return IsSpellReady(spellId) && HasEnoughResource(spellId);
}

bool ClassAI::IsSpellReady(uint32 spellId)
{
    if (!spellId || !GetBot())
        return false;

    // Check if spell is on cooldown (note: Common/CooldownManager.h doesn't have IsGCDReady method)
    return _cooldownManager->IsReady(spellId);
}

// ============================================================================
// SPELL UTILITIES
// ============================================================================

bool ClassAI::IsInRange(::Unit* target, uint32 spellId)
{
    if (!target || !spellId || !GetBot())
        return false;

    float range = GetSpellRange(spellId);
    if (range <= 0.0f)
        return true; // No range restriction

    float rangeSq = range * range;
    return GetBot()->GetExactDistSq(target) <= rangeSq;
}

bool ClassAI::HasLineOfSight(::Unit* target)
{
    if (!target || !GetBot())
        return false;

    return GetBot()->IsWithinLOSInMap(target);
}

bool ClassAI::IsSpellUsable(uint32 spellId)
{
    if (!spellId || !GetBot())
        return false;

    // Check if bot knows the spell
    if (!GetBot()->HasSpell(spellId))
    {
        TC_LOG_DEBUG("playerbot.classai.spell",
            "IsSpellUsable: Bot {} does NOT have spell {} - needs to learn it",
            GetBot()->GetName(), spellId);
        return false;
    }

    // Check if spell is ready (cooldown/GCD)
    if (!IsSpellReady(spellId))
    {
        TC_LOG_TRACE("playerbot.classai.spell",
            "IsSpellUsable: Bot {} spell {} not ready (on cooldown or GCD)",
            GetBot()->GetName(), spellId);
        return false;
    }

    // Check resource requirements
    if (!HasEnoughResource(spellId))
    {
        TC_LOG_TRACE("playerbot.classai.spell",
            "IsSpellUsable: Bot {} spell {} - not enough resources",
            GetBot()->GetName(), spellId);
        return false;
    }

    return true;
}

float ClassAI::GetSpellRange(uint32 spellId)
{
    if (!spellId)
        return 0.0f;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return 0.0f;

    return spellInfo->GetMaxRange();
}
uint32 ClassAI::GetSpellCooldown(uint32 spellId)
{
    if (!spellId || !GetBot())
        return 0;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return 0;

    auto remaining = GetBot()->GetSpellHistory()->GetRemainingCooldown(spellInfo);
    return remaining.count();
}

// ============================================================================
// SPELL QUEUEING SYSTEM - Enterprise-grade spell casting
// ============================================================================
// Mirrors TrinityCore's player spell queueing for proper validation timing
// See Player.cpp:30904-31051 for reference implementation

bool ClassAI::RequestBotSpellCast(uint32 spellId, ::Unit* target)
{
    if (!GetBot())
        return false;

    // Get spell info for validation
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, GetBot()->GetMap()->GetDifficultyID());
    if (!spellInfo)
    {
        TC_LOG_TRACE("module.playerbot.classai", "Bot {} RequestBotSpellCast: Invalid spell ID {}",
                    GetBot()->GetName(), spellId);
        return false;
    }

    // Check if we can queue this spell (GCD, current cast, etc.)
    // Mirrors Player::CanRequestSpellCast()
    if (!CanRequestBotSpellCast(spellId))
    {
        TC_LOG_TRACE("module.playerbot.classai", "Bot {} cannot queue spell {} - GCD/cast time > 400ms",
                    GetBot()->GetName(), spellId);
        return false;
    }

    // Cancel any existing pending spell (like players - only one queued at a time)
    if (_pendingSpellCastRequest)
    {
        TC_LOG_TRACE("module.playerbot.classai", "Bot {} canceling previous pending spell {} to queue {}",
                    GetBot()->GetName(), _pendingSpellCastRequest->spellId, spellId);
        CancelPendingSpell();
    }

    // Queue the new spell
    _pendingSpellCastRequest = ::std::make_unique<BotSpellCastRequest>(spellId, target);
    TC_LOG_DEBUG("module.playerbot.classai", "Bot {} queued spell {} targeting {}",
                GetBot()->GetName(), spellId,
                target ? target->GetName() : "self");

    // Try to execute immediately if conditions are met
    // Mirrors Player::RequestSpellCast() behavior
    if (CanExecutePendingSpell())
    {
        ExecutePendingSpell();
    }

    return true;
}

bool ClassAI::CanRequestBotSpellCast(uint32 spellId) const
{
    if (!GetBot())
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, GetBot()->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return false;

    // Check global cooldown (must be ≤400ms remaining to queue)
    // Mirrors Player::CanRequestSpellCast() - Player.cpp:30935-30946
    if (GetBot()->GetSpellHistory()->GetRemainingGlobalCooldown(spellInfo) > Milliseconds(SPELL_QUEUE_TIME_WINDOW_MS))
    {
        TC_LOG_TRACE("module.playerbot.classai", "Bot {} CanRequestBotSpellCast: GCD > 400ms for spell {}",
                    GetBot()->GetName(), spellId);
        return false;
    }

    // Check current spell casts (must be ≤400ms remaining to queue)
    for (CurrentSpellTypes spellSlot : { CURRENT_MELEE_SPELL, CURRENT_GENERIC_SPELL })
    {
        if (Spell const* spell = GetBot()->GetCurrentSpell(spellSlot))
        {
            if (Milliseconds(spell->GetRemainingCastTime()) > Milliseconds(SPELL_QUEUE_TIME_WINDOW_MS))
            {
                TC_LOG_TRACE("module.playerbot.classai", "Bot {} CanRequestBotSpellCast: Spell cast time > 400ms for spell {}",
                            GetBot()->GetName(), spellId);
                return false;
            }
        }
    }

    return true;
}

bool ClassAI::CanExecutePendingSpell() const
{
    // CRITICAL FIX: Check for null BEFORE accessing any member
    if (!_pendingSpellCastRequest)
        return false;

    if (!GetBot())
        return false;

    // DIAGNOSTIC: Log every check to trace execution flow (now safe since we checked for null above)
    TC_LOG_ERROR("module.playerbot.classai", " CanExecutePendingSpell: Bot {} has pending spell {} queued",
                GetBot()->GetName(), _pendingSpellCastRequest->spellId);

    // CRITICAL FIX: Don't check UNIT_STATE_CASTING for bots    // Unlike players who have packet-driven spell casting, bots queue spells
    // and then ExecutePendingSpell() calls Spell::prepare() which sets the state.
    // Checking for UNIT_STATE_CASTING creates a catch-22 where the bot waits
    // forever for a spell to finish that never started.
    //
    // For bots, we only need to check:
    // 1. Is there actually a spell currently being cast? (GetCurrentSpell)
    // 2. Is GCD ready?
    //
    // If no current spell and GCD is ready, execute the pending spell.

    // Check if bot is currently casting a different spell
    if (Spell const* currentSpell = GetBot()->GetCurrentSpell(CURRENT_GENERIC_SPELL))
    {
        TC_LOG_ERROR("module.playerbot.classai", "CanExecutePendingSpell: Bot {} CURRENTLY CASTING spell {}, waiting",
                     GetBot()->GetName(), currentSpell->GetSpellInfo()->Id);
        return false;
    }

    TC_LOG_ERROR("module.playerbot.classai", " CanExecutePendingSpell: Bot {} NOT casting, checking GCD", GetBot()->GetName());

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(_pendingSpellCastRequest->spellId,
                                                          GetBot()->GetMap()->GetDifficultyID());

    TC_LOG_ERROR("module.playerbot.classai", " CanExecutePendingSpell: Spell info valid, checking GCD");

    // Wait for global cooldown to expire completely (not just ≤400ms)
    auto gcdRemaining = GetBot()->GetSpellHistory()->GetRemainingGlobalCooldown(spellInfo);
    if (gcdRemaining > 0ms)
    {
        TC_LOG_ERROR("module.playerbot.classai", " CanExecutePendingSpell: Bot {} GCD NOT READY ({} ms remaining) for spell {}",
                    GetBot()->GetName(), gcdRemaining.count(), _pendingSpellCastRequest->spellId);
        return false;    }

    TC_LOG_ERROR("module.playerbot.classai", " CanExecutePendingSpell: Bot {} READY TO EXECUTE spell {}",
                GetBot()->GetName(), _pendingSpellCastRequest->spellId);

    return true;
}

void ClassAI::ExecutePendingSpell()
{
    if (!_pendingSpellCastRequest || !GetBot())
        return;

    Player* bot = GetBot();
    uint32 spellId = _pendingSpellCastRequest->spellId;

    // Get spell info
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, bot->GetMap()->GetDifficultyID());
    if (!spellInfo)
    {
        TC_LOG_ERROR("module.playerbot.classai", "Bot {} ExecutePendingSpell: Invalid spell ID {}",
                    bot->GetName(), spellId);
        CancelPendingSpell();
        return;
    }

    // Validate target is still valid
    ::Unit* target = _pendingSpellCastRequest->target;
    if (!_pendingSpellCastRequest->isSelfCast)
    {
        if (!target || !target->IsInWorld() || target->isDead())
        {
            TC_LOG_DEBUG("module.playerbot.classai", "Bot {} ExecutePendingSpell: Target invalid for spell {}, canceling",
                        bot->GetName(), spellId);
            CancelPendingSpell();
            return;
        }
    }
    else
    {
        target = bot; // Self-cast
    }

    // CRITICAL FIX: Don't call StopMoving() manually - causes position desync
    // TrinityCore's spell system automatically handles movement interruption via Spell::CheckCast()
    // When we manually call StopMoving(), it clears the movement spline which causes:
    // 1. Client shows movement (2-6 yards)
    // 2. Server clears spline and resets position
    // 3. Client teleports back (the "blink back" bug)
    //
    // The spell system will interrupt movement if needed during Spell::prepare()
    // This is handled in Spell::CheckCast() → SPELL_FAILED_MOVING
    // Let the spell system do its job - don't interfere with movement here

    // CRITICAL: Face the target before casting (required for spell validation)
    // Players auto-face when casting, bots need to do it explicitly
    if (target && target != bot)    {
        bot->SetFacingToObject(target);
        TC_LOG_ERROR("module.playerbot.classai", " Bot {} facing target {} before spell cast",
                    bot->GetName(), target->GetName());
    }

    // Create spell cast targets
    // Mirrors Player::ExecutePendingSpellCastRequest() - Player.cpp:30948-31051
    SpellCastTargets targets;
    targets.SetUnitTarget(target);

    // CRITICAL: Use TRIGGERED_NONE - no flags, proper validation timing
    // This is the KEY difference from the workaround approach
    // Target validation happens in Spell::prepare(), not when queuing
    TriggerCastFlags triggerFlags = TRIGGERED_NONE;

    // Create Spell object (exactly like players do)
    // See Player.cpp:31038 - Spell* spell = new Spell(castingUnit, spellInfo, triggerFlag);
    Spell* spell = new Spell(bot, spellInfo, triggerFlags);    // Prepare the spell (this is where proper validation happens at the right time)
    // See Player.cpp:31048 - spell->prepare(targets);
    // This will handle:
    // - Resource consumption
    // - Target validation at execute time (not queue time!)
    // - Range/LOS checks
    // - Cast time processing
    // - Combat state management
    ::SpellCastResult result = spell->prepare(targets);
    uint32 queuedDuration = GameTime::GetGameTimeMS() - _pendingSpellCastRequest->queuedAtTime;

    if (result == SPELL_CAST_OK)
    {
        TC_LOG_ERROR("module.playerbot.classai", " Bot {} executed queued spell {} on {} - queued for {}ms",
                    bot->GetName(), spellId,
                    target ? target->GetName() : "self", queuedDuration);
    }
    else
    {
        TC_LOG_ERROR("module.playerbot.classai", " Bot {} spell {} failed with result {} - queued for {}ms",                    bot->GetName(), spellId,
                    uint32(result), queuedDuration);
    }

    // Clear pending request
    _pendingSpellCastRequest = nullptr;
}

void ClassAI::CancelPendingSpell()
{
    if (_pendingSpellCastRequest && GetBot())
    {
        TC_LOG_TRACE("module.playerbot.classai", "Bot {} canceled pending spell {}",
                    GetBot()->GetName(), _pendingSpellCastRequest->spellId);
    }

    _pendingSpellCastRequest = nullptr;
}

// ============================================================================
// SPELL CASTING
// ============================================================================

::SpellCastResult ClassAI::CastSpell(uint32 spellId, ::Unit* target /*= nullptr*/)
{
    TC_LOG_ERROR("playerbot.classai.spell", "ClassAI::CastSpell ENTRY: spell={} target={} bot={}",
                 spellId, target ? target->GetName() : "NULL", GetBot() ? GetBot()->GetName() : "NULL");

    // If no target specified, self-cast
    if (!target)
        target = GetBot();

    if (!target || !spellId || !GetBot())
    {
        TC_LOG_ERROR("playerbot.classai.spell", "ClassAI::CastSpell FAILED: target={} spellId={} bot={}",
                     target != nullptr, spellId, GetBot() != nullptr);
        return SPELL_FAILED_ERROR;
    }

    // ARCHITECTURE: Packet-based spell casting (SpellPacketBuilder) replaces direct CastSpell API.
    // Direct API calls from worker threads are unsafe — packets are queued for main-thread processing.
    // All 39 class specializations use this thread-safe spell casting path.

    // Pre-validation (ClassAI-specific checks before packet building)
    if (!IsSpellUsable(spellId))
    {
        TC_LOG_ERROR("playerbot.classai.spell",
                     "ClassAI::CastSpell FAILED: spell {} not usable for bot {}",
                     spellId, GetBot()->GetName());
        return SPELL_FAILED_NOT_READY;
    }

    if (!IsInRange(target, spellId))
    {
        TC_LOG_ERROR("playerbot.classai.spell",
                     "ClassAI::CastSpell FAILED: spell {} target out of range for bot {}",
                     spellId, GetBot()->GetName());
        return SPELL_FAILED_OUT_OF_RANGE;
    }

    if (!HasLineOfSight(target))
    {
        TC_LOG_ERROR("playerbot.classai.spell",
                     "ClassAI::CastSpell FAILED: spell {} target no LOS for bot {}",
                     spellId, GetBot()->GetName());
        return SPELL_FAILED_LINE_OF_SIGHT;
    }

    // Get spell info for validation and cooldown tracking
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
    {
        TC_LOG_TRACE("playerbot.classai.spell",
                     "ClassAI spell {} not found in spell data for bot {}",
                     spellId, GetBot()->GetName());
        return SPELL_FAILED_SPELL_UNAVAILABLE;
    }

    // Build packet with validation
    SpellPacketBuilder::BuildOptions options;
    options.skipGcdCheck = false;      // Respect GCD
    options.skipResourceCheck = false; // Check mana/energy/rage
    options.skipTargetCheck = false;   // Check target validity
    options.skipStateCheck = false;    // Check caster state
    options.skipRangeCheck = false;    // Check spell range (double-check after ClassAI check)
    options.logFailures = true;        // Log validation failures

    auto result = SpellPacketBuilder::BuildCastSpellPacket(GetBot(), spellId, target, options);
    if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
    {
        // CRITICAL FIX: Actually send the packet to the session!
        // Previously the packet was built but never queued, so spells were never cast.
        // This is why bots were only doing auto-attack with 1 damage - no abilities fired!
        if (result.packet && GetBot()->GetSession())
        {
            // TrinityCore 12.0: QueuePacket now takes WorldPacket&& instead of WorldPacket*
            GetBot()->GetSession()->QueuePacket(std::move(*result.packet));
            TC_LOG_ERROR("playerbot.classai.spell",
                         "🎯 ClassAI QUEUED spell {} packet for bot {} targeting {}",
                         spellId, GetBot()->GetName(), target->GetName());
        }
        else
        {
            TC_LOG_ERROR("playerbot.classai.spell",
                         "❌ ClassAI spell {} packet build succeeded but packet/session is null for bot {}",
                         spellId, GetBot()->GetName());
            return SPELL_FAILED_ERROR;
        }

        // Optimistic resource consumption and cooldown tracking
        // (Will be validated again on main thread, but tracking here for ClassAI responsiveness)
        ConsumeResource(spellId);

        // Track cooldown if pre-registered in CooldownManager
        if (_cooldownManager && _cooldownManager->IsTracked(spellId))
            _cooldownManager->Trigger(spellId);

        TC_LOG_DEBUG("playerbot.classai.spell",
                     "ClassAI queued CMSG_CAST_SPELL for spell {} (bot: {}, target: {})",
                     spellId, GetBot()->GetName(), target->GetName());
        return SPELL_CAST_OK;
    }
    else
    {
        // Validation failed - packet not queued
        TC_LOG_TRACE("playerbot.classai.spell",
                     "ClassAI spell {} validation failed for bot {}: {} ({})",
                     spellId, GetBot()->GetName(),
                     static_cast<uint8>(result.result),
                     result.failureReason);
        return SPELL_FAILED_ERROR;
    }
}

// REMOVED: bool ClassAI::CastSpell(uint32 spellId) - not declared in header
// Use CastSpell(spellId, GetBot()) directly for self-cast

// ============================================================================
// AURA UTILITIES
// ============================================================================

bool ClassAI::HasAura(uint32 spellId, ::Unit* target)
{
    ::Unit* checkTarget = target ? target : GetBot();
    if (!checkTarget)
        return false;

    return checkTarget->HasAura(spellId);
}

uint32 ClassAI::GetAuraStacks(uint32 spellId, ::Unit* target)
{
    ::Unit* checkTarget = target ? target : GetBot();
    if (!checkTarget)
        return 0;

    if (Aura* aura = checkTarget->GetAura(spellId))        return aura->GetStackAmount();

    return 0;
}

uint32 ClassAI::GetAuraRemainingTime(uint32 spellId, ::Unit* target)
{
    ::Unit* checkTarget = target ? target : GetBot();
    if (!checkTarget)
        return 0;

    if (Aura* aura = checkTarget->GetAura(spellId))        return aura->GetDuration();

    return 0;
}

// ============================================================================
// MOVEMENT QUERIES (READ-ONLY)
// ============================================================================

bool ClassAI::IsMoving() const
{
    return GetBot() && GetBot()->isMoving();
}

bool ClassAI::IsInMeleeRange(::Unit* target) const
{
    if (!target || !GetBot())
        return false;

    return GetBot()->IsWithinMeleeRange(target);
}

bool ClassAI::ShouldMoveToTarget(::Unit* target) const
{
    if (!target || !GetBot())
        return false;

    // ClassAI doesn't control movement, just provides information
    // Actual movement is handled by BotAI strategies
    float optimalRange = const_cast<ClassAI*>(this)->GetOptimalRange(target);
    float currentDistance = ::std::sqrt(GetBot()->GetExactDistSq(target)); // Calculate once from squared distance

    return currentDistance > optimalRange;
}

float ClassAI::GetDistanceToTarget(::Unit* target) const
{
    if (!target || !GetBot())
        return 0.0f;

    return ::std::sqrt(GetBot()->GetExactDistSq(target)); // Calculate once from squared distance
}

// ============================================================================
// POSITIONING PREFERENCES
// ============================================================================

Position ClassAI::GetOptimalPosition(::Unit* target)
{
    if (!target || !GetBot())
        return Position();

    // Default implementation - stay at optimal range
    float optimalRange = GetOptimalRange(target);
    float angle = GetBot()->GetRelativeAngle(target);

    Position pos;
    pos.m_positionX = target->GetPositionX() - optimalRange * ::std::cos(angle);
    pos.m_positionY = target->GetPositionY() - optimalRange * ::std::sin(angle);    pos.m_positionZ = target->GetPositionZ();    pos.SetOrientation(target->GetOrientation());

    return pos;
}

// ============================================================================
// PERFORMANCE METRICS
// ============================================================================

void ClassAI::RecordPerformanceMetric(::std::string const& metric, uint32 value)
{
    // Record class-specific performance metrics
    TC_LOG_TRACE("playerbot.performance", "ClassAI metric {} = {} for bot {}",
                 metric, value, GetBot() ? GetBot()->GetName() : "null");
}

// ============================================================================// COMBAT BEHAVIOR INTEGRATION
// ============================================================================

bool ClassAI::ExecuteRecommendedAction(const RecommendedAction& action)
{
    if (!GetBot() || !action.target || action.spellId == 0)
    {
        TC_LOG_TRACE("playerbot.classai", "ExecuteRecommendedAction: Invalid parameters - bot={}, target={}, spell={}",                     GetBot() ? "valid" : "null", action.target ? "valid" : "null", action.spellId);
        return false;
    }

    // Log the recommended action execution attempt
    TC_LOG_DEBUG("playerbot.classai", "Bot {} executing {} action: spell {} on {} (reason: {})",
                 GetBot()->GetName(), GetActionName(action.type), action.spellId,
                 action.target->GetName(), action.reason.c_str());

    // Check if the spell can be used
    if (!IsSpellUsable(action.spellId))
    {
        TC_LOG_TRACE("playerbot.classai", "Bot {} cannot use spell {} - not usable",
                     GetBot()->GetName(), action.spellId);
        return false;
    }

    // Check range to target
    if (!IsInRange(action.target, action.spellId))
    {
        TC_LOG_TRACE("playerbot.classai", "Bot {} cannot cast spell {} - target out of range",
                     GetBot()->GetName(), action.spellId);

        // For movement-related actions, we might want to move closer
    if (action.type == CombatActionType::MOVEMENT)
        {
            // Movement is handled by BotAI strategies, just log the need if position is valid
    if (action.position.m_positionX != 0.0f || action.position.m_positionY != 0.0f)
            {
                TC_LOG_DEBUG("playerbot.classai", "Bot {} needs to move to position ({}, {}, {}) for action",
                             GetBot()->GetName(), action.position.m_positionX,
                             action.position.m_positionY, action.position.m_positionZ);
            }
        }
        return false;
    }

    // Check line of sight
    if (!HasLineOfSight(action.target))
    {        TC_LOG_TRACE("playerbot.classai", "Bot {} cannot cast spell {} - no line of sight",
                     GetBot()->GetName(), action.spellId);
        return false;
    }

    // Handle different action types with appropriate logic
    bool success = false;
    switch (action.type)
    {
        case CombatActionType::INTERRUPT:
        {
            // Interrupt requires special handling - face target quickly
            GetBot()->SetFacingToObject(action.target);
            success = CastSpell(action.spellId, action.target);
            if (success)
            {
                TC_LOG_INFO("playerbot.classai", "Bot {} successfully interrupted {} with spell {}",
                            GetBot()->GetName(), action.target->GetName(), action.spellId);
            }
            break;
        }

        case CombatActionType::DEFENSIVE:
        {
            // Defensive actions often target self or allies
            Unit* defTarget = action.target == GetBot() ? nullptr : action.target;
            if (defTarget)
                success = CastSpell(action.spellId, defTarget);
            else
                success = CastSpell(action.spellId);  // Self-cast
    if (success)
            {
                TC_LOG_INFO("playerbot.classai", "Bot {} activated defensive ability {} on {}",
                            GetBot()->GetName(), action.spellId,
                            defTarget ? defTarget->GetName() : "self");
            }
            break;
        }

        case CombatActionType::CROWD_CONTROL:
        {
            // CC requires careful targeting
    if (action.target != _currentCombatTarget)  // Don't CC our main target
            {
                success = CastSpell(action.spellId, action.target);
                if (success)
                {
                    TC_LOG_INFO("playerbot.classai", "Bot {} applied crowd control {} to {}",
                                GetBot()->GetName(), action.spellId, action.target->GetName());
                }
            }
            break;
        }

        case CombatActionType::EMERGENCY:
        {
            // Emergency actions are highest priority - try to force cast
            success = CastSpell(action.spellId, action.target);
            if (success)
            {
                TC_LOG_WARN("playerbot.classai", "Bot {} executed EMERGENCY action: {} on {}",
                            GetBot()->GetName(), action.spellId, action.target->GetName());
            }
            break;
        }

        case CombatActionType::COOLDOWN:
        {
            // Major cooldowns
            success = CastSpell(action.spellId, action.target);
            if (success)
            {
                TC_LOG_INFO("playerbot.classai", "Bot {} activated cooldown {} on {}",
                            GetBot()->GetName(), action.spellId, action.target->GetName());
            }
            break;
        }

        case CombatActionType::TARGET_SWITCH:
        {
            // Target switch is handled by OnTargetChanged, just validate
    if (action.target && action.target != _currentCombatTarget)            {
                OnTargetChanged(action.target);
                success = true;
                TC_LOG_INFO("playerbot.classai", "Bot {} switched target to {}",
                            GetBot()->GetName(), action.target->GetName());
            }
            break;
        }

        case CombatActionType::CONSUMABLE:
        {
            // Consumables (potions, healthstones, etc.)
            // These typically don't have a target or target self
            success = CastSpell(action.spellId);
            if (success)
            {
                TC_LOG_INFO("playerbot.classai", "Bot {} used consumable {}",
                            GetBot()->GetName(), action.spellId);
            }
            break;
        }

        case CombatActionType::MOVEMENT:
        {
            // Movement is handled by BotAI strategies, log the request
            TC_LOG_DEBUG("playerbot.classai", "Bot {} requested movement action to ({}, {}, {})",
                         GetBot()->GetName(), action.position.m_positionX,
                         action.position.m_positionY, action.position.m_positionZ);
            // Return true to indicate the request was acknowledged
            success = true;
            break;
        }

        case CombatActionType::ROTATION:
        default:
        {
            // Normal rotation ability
            success = CastSpell(action.spellId, action.target);
            if (success)
            {
                TC_LOG_TRACE("playerbot.classai", "Bot {} cast rotation spell {} on {}",
                             GetBot()->GetName(), action.spellId, action.target->GetName());
            }
            break;
        }
    }

    // Record metrics if successful
    if (success)
    {
        RecordPerformanceMetric("recommended_action_success", 1);
    }
    else
    {
        RecordPerformanceMetric("recommended_action_fail", 1);
        TC_LOG_TRACE("playerbot.classai", "Bot {} failed to execute {} action: {} on {}",
                     GetBot()->GetName(), GetActionName(action.type), action.spellId,                     action.target->GetName());
    }

    return success;
}

} // namespace Playerbot