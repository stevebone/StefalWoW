/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ProactiveLoSFixer.h"
#include "LineOfSightManager.h"
#include "Player.h"
#include "Unit.h"
#include "Map.h"
#include "Log.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "ObjectAccessor.h"
#include "GameTime.h"
#include "Group.h"
#include "DBCEnums.h"
#include "DB2Stores.h"
#include "../../Movement/BotMovementUtil.h"

namespace Playerbot
{

ProactiveLoSFixer::ProactiveLoSFixer(Player* bot)
    : _bot(bot)
{
}

void ProactiveLoSFixer::Initialize(LineOfSightManager* losMgr)
{
    if (!_bot || !losMgr)
    {
        TC_LOG_ERROR("module.playerbot", "ProactiveLoSFixer::Initialize - null bot or losMgr");
        return;
    }

    _losMgr = losMgr;
    _initialized = true;

    TC_LOG_DEBUG("module.playerbot", "ProactiveLoSFixer: Initialized for bot {}", _bot->GetName());
}

void ProactiveLoSFixer::Update(uint32 diff)
{
    if (!_initialized || !_inCombat)
        return;

    _updateTimer += diff;
    if (_updateTimer < UPDATE_INTERVAL_MS)
        return;
    _updateTimer = 0;

    // Check if pending cast has expired
    if (_pendingCast.IsValid())
    {
        uint32 now = GameTime::GetGameTimeMS();

        if (_pendingCast.IsExpired(now))
        {
            TC_LOG_DEBUG("module.playerbot", "ProactiveLoSFixer: Pending cast for spell {} timed out for bot {}",
                _pendingCast.spellId, _bot->GetName());
            _stats.repositionTimeouts++;
            ClearPendingCast();
            return;
        }

        // Check if we've arrived at the reposition target
        if (_isRepositioning && HasReachedRepositionTarget())
        {
            // Verify LOS from new position
            Unit* target = nullptr;
            if (!_pendingCast.targetGuid.IsEmpty())
                target = ObjectAccessor::GetUnit(*_bot, _pendingCast.targetGuid);

            bool hasLos = false;
            if (target)
                hasLos = _losMgr->CanSeeTarget(target);
            else if (_pendingCast.isGroundTargeted)
                hasLos = _losMgr->CanMoveToPosition(_pendingCast.targetPosition);

            if (hasLos)
            {
                _isRepositioning = false;
                _stats.repositionSuccesses++;
                TC_LOG_DEBUG("module.playerbot", "ProactiveLoSFixer: Bot {} reached LOS position for spell {}",
                    _bot->GetName(), _pendingCast.spellId);
                // Pending cast is now ready - caller should check IsPendingCastReady()
            }
            else
            {
                // Arrived but still no LOS - try finding a new position
                TC_LOG_DEBUG("module.playerbot", "ProactiveLoSFixer: Bot {} arrived but still no LOS for spell {}",
                    _bot->GetName(), _pendingCast.spellId);

                uint32 now2 = GameTime::GetGameTimeMS();
                if (now2 - _lastPositionFindMs >= POSITION_FIND_COOLDOWN_MS)
                {
                    _lastPositionFindMs = now2;
                    Position newPos;
                    if (target)
                        newPos = FindCastPosition(target, _pendingCast.spellMaxRange);
                    else
                        newPos = FindCastPositionForGround(_pendingCast.targetPosition, _pendingCast.spellMaxRange);

                    if (newPos.GetExactDist(_bot) > REPOSITION_ARRIVAL_TOLERANCE)
                    {
                        _pendingCast.repositionTarget = newPos;
                        MoveToLoSPosition(newPos);
                    }
                    else
                    {
                        // No better position found, give up
                        TC_LOG_DEBUG("module.playerbot", "ProactiveLoSFixer: Bot {} cannot find LOS for spell {}, giving up",
                            _bot->GetName(), _pendingCast.spellId);
                        _stats.noPositionFound++;
                        ClearPendingCast();
                    }
                }
            }
        }
    }

    // Healer proactive LOS maintenance
    if (IsHealerRole())
    {
        _healerLoSCheckTimer += UPDATE_INTERVAL_MS;
        if (_healerLoSCheckTimer >= HEALER_LOS_CHECK_INTERVAL_MS)
        {
            _healerLoSCheckTimer = 0;
            // Only check healer LOS if we're not already repositioning for a cast
            if (!_isRepositioning)
            {
                CheckHealerGroupLoS();
            }
        }
    }
}

void ProactiveLoSFixer::OnCombatStart()
{
    _inCombat = true;
    ClearPendingCast();
    _updateTimer = 0;
    _healerLoSCheckTimer = 0;
}

void ProactiveLoSFixer::OnCombatEnd()
{
    _inCombat = false;
    ClearPendingCast();
}

// ============================================================================
// CORE: PRE-CAST LOS CHECK
// ============================================================================

LoSPreCastResult ProactiveLoSFixer::PreCastCheck(uint32 spellId, Unit* target)
{
    if (!_initialized || !_losMgr)
        return LoSPreCastResult::CLEAR; // Fail open - don't block casting

    _stats.totalPreCastChecks++;

    if (!target)
    {
        // Self-cast or no target - LOS not relevant
        return LoSPreCastResult::CLEAR;
    }

    // Check if spell ignores LOS
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return LoSPreCastResult::CLEAR;

    if (DoesSpellIgnoreLos(spellInfo))
    {
        _stats.spellIgnoredLos++;
        return LoSPreCastResult::SPELL_IGNORES;
    }

    // Already repositioning for a different spell? Don't interrupt
    if (_isRepositioning && _pendingCast.IsValid() && _pendingCast.spellId != spellId)
        return LoSPreCastResult::ALREADY_MOVING;

    // Check current LOS to target
    if (_bot->IsWithinLOSInMap(target))
    {
        // Also verify range
        float spellRange = GetEffectiveSpellRange(spellId);
        float dist = ::std::sqrt(_bot->GetExactDistSq(target));
        if (dist <= spellRange || spellRange <= 0.0f)
        {
            _stats.losWasClear++;
            // If we had a pending cast for this spell, clear it since LOS is now good
            if (_pendingCast.IsValid() && _pendingCast.spellId == spellId)
                ClearPendingCast();
            return LoSPreCastResult::CLEAR;
        }
    }

    // LOS is blocked or out of range - need to reposition
    float spellRange = GetEffectiveSpellRange(spellId);
    float maxTheoreticalDist = ::std::sqrt(_bot->GetExactDistSq(target));

    // If the target is way too far even after repositioning, don't bother
    if (maxTheoreticalDist > spellRange + 60.0f && spellRange > 0.0f)
        return LoSPreCastResult::TOO_FAR;

    // Find the best position to cast from
    uint32 now = GameTime::GetGameTimeMS();
    if (now - _lastPositionFindMs < POSITION_FIND_COOLDOWN_MS && _pendingCast.IsValid() && _pendingCast.spellId == spellId)
    {
        // Already searching for this spell, don't spam position finding
        return LoSPreCastResult::REPOSITIONING;
    }

    _lastPositionFindMs = now;
    Position castPos = FindCastPosition(target, spellRange);

    // If castPos is basically where we are, no valid position was found
    if (castPos.GetExactDist(_bot) < REPOSITION_ARRIVAL_TOLERANCE)
    {
        // Try a simpler approach: just move toward the target
        float angle = _bot->GetAbsoluteAngle(target);
        float moveDistance = ::std::min(10.0f, maxTheoreticalDist * 0.5f);
        castPos.m_positionX = _bot->GetPositionX() + moveDistance * ::std::cos(angle);
        castPos.m_positionY = _bot->GetPositionY() + moveDistance * ::std::sin(angle);
        castPos.m_positionZ = _bot->GetPositionZ();

        // Correct Z to ground
        BotMovementUtil::CorrectPositionToGround(_bot, castPos);

        // Verify we'll have LOS from there
        if (!_losMgr->WillHaveLineOfSightAfterMovement(castPos, target))
        {
            _stats.noPositionFound++;
            return LoSPreCastResult::NO_POSITION;
        }
    }

    // Queue the cast and start moving
    _pendingCast.spellId = spellId;
    _pendingCast.targetGuid = target->GetGUID();
    _pendingCast.targetPosition = target->GetPosition();
    _pendingCast.repositionTarget = castPos;
    _pendingCast.queueTimeMs = now;
    _pendingCast.maxWaitMs = MAX_REPOSITION_TIME_MS;
    _pendingCast.spellMaxRange = spellRange;
    _pendingCast.isGroundTargeted = false;

    _isRepositioning = true;
    _stats.repositionAttempts++;

    MoveToLoSPosition(castPos);

    TC_LOG_DEBUG("module.playerbot", "ProactiveLoSFixer: Bot {} repositioning for spell {} (target: {}, dist: {:.1f})",
        _bot->GetName(), spellId, target->GetName(), castPos.GetExactDist(_bot));

    return LoSPreCastResult::REPOSITIONING;
}

LoSPreCastResult ProactiveLoSFixer::PreCastCheckPosition(uint32 spellId, Position const& targetPos)
{
    if (!_initialized || !_losMgr)
        return LoSPreCastResult::CLEAR;

    _stats.totalPreCastChecks++;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return LoSPreCastResult::CLEAR;

    if (DoesSpellIgnoreLos(spellInfo))
    {
        _stats.spellIgnoredLos++;
        return LoSPreCastResult::SPELL_IGNORES;
    }

    // Check LOS to position
    if (_losMgr->CanCastAoEAtPosition(targetPos, spellId))
    {
        float spellRange = GetEffectiveSpellRange(spellId);
        float dist = _bot->GetExactDist(&targetPos);
        if (dist <= spellRange || spellRange <= 0.0f)
        {
            _stats.losWasClear++;
            return LoSPreCastResult::CLEAR;
        }
    }

    // Need to reposition for ground-targeted spell
    float spellRange = GetEffectiveSpellRange(spellId);
    uint32 now = GameTime::GetGameTimeMS();

    if (now - _lastPositionFindMs < POSITION_FIND_COOLDOWN_MS && _pendingCast.IsValid() && _pendingCast.spellId == spellId)
        return LoSPreCastResult::REPOSITIONING;

    _lastPositionFindMs = now;
    Position castPos = FindCastPositionForGround(targetPos, spellRange);

    if (castPos.GetExactDist(_bot) < REPOSITION_ARRIVAL_TOLERANCE)
    {
        _stats.noPositionFound++;
        return LoSPreCastResult::NO_POSITION;
    }

    _pendingCast.spellId = spellId;
    _pendingCast.targetGuid.Clear();
    _pendingCast.targetPosition = targetPos;
    _pendingCast.repositionTarget = castPos;
    _pendingCast.queueTimeMs = now;
    _pendingCast.maxWaitMs = MAX_REPOSITION_TIME_MS;
    _pendingCast.spellMaxRange = spellRange;
    _pendingCast.isGroundTargeted = true;

    _isRepositioning = true;
    _stats.repositionAttempts++;

    MoveToLoSPosition(castPos);

    return LoSPreCastResult::REPOSITIONING;
}

// ============================================================================
// PENDING CAST MANAGEMENT
// ============================================================================

bool ProactiveLoSFixer::IsPendingCastReady() const
{
    if (!_pendingCast.IsValid() || _isRepositioning)
        return false;

    // Pending cast is ready when we've stopped repositioning (arrived at position)
    // and the cast hasn't expired
    uint32 now = GameTime::GetGameTimeMS();
    return !_pendingCast.IsExpired(now);
}

void ProactiveLoSFixer::ClearPendingCast()
{
    _pendingCast.Reset();
    _isRepositioning = false;
}

void ProactiveLoSFixer::CancelPendingCast()
{
    if (_isRepositioning)
    {
        BotMovementUtil::StopMovement(_bot);
    }
    ClearPendingCast();
}

// ============================================================================
// HEALER LOS MAINTENANCE
// ============================================================================

bool ProactiveLoSFixer::CheckHealerGroupLoS()
{
    if (!_losMgr || !_bot)
        return false;

    _stats.healerLoSChecks++;

    Group* group = _bot->GetGroup();
    if (!group)
        return false;

    // Count how many group members we can see
    uint32 totalMembers = 0;
    uint32 visibleMembers = 0;

    for (auto const& slot : group->GetMemberSlots())
    {
        if (slot.guid == _bot->GetGUID())
            continue;

        Player* member = ObjectAccessor::FindPlayer(slot.guid);
        if (!member || !member->IsInWorld() || !member->IsAlive())
            continue;

        // Only check members on the same map
        if (member->GetMapId() != _bot->GetMapId())
            continue;

        totalMembers++;

        float distSq = _bot->GetExactDistSq(member);
        if (distSq > 40.0f * 40.0f) // Beyond heal range
            continue;

        if (_bot->IsWithinLOSInMap(member))
            visibleMembers++;
    }

    if (totalMembers == 0)
        return false;

    float losPct = static_cast<float>(visibleMembers) / static_cast<float>(totalMembers);

    if (losPct < HEALER_MIN_LOS_PCT)
    {
        // Need to reposition to see more group members
        Position healerPos = GetBestHealerPosition();
        if (healerPos.GetExactDist(_bot) > REPOSITION_ARRIVAL_TOLERANCE)
        {
            _stats.healerRepositions++;
            MoveToLoSPosition(healerPos);

            TC_LOG_DEBUG("module.playerbot", "ProactiveLoSFixer: Healer {} repositioning for group LOS "
                "(visible: {}/{}, {:.0f}%)", _bot->GetName(), visibleMembers, totalMembers, losPct * 100.0f);
            return true;
        }
    }

    return false;
}

Position ProactiveLoSFixer::GetBestHealerPosition() const
{
    if (!_losMgr || !_bot)
        return _bot ? _bot->GetPosition() : Position();

    Group* group = _bot->GetGroup();
    if (!group)
        return _bot->GetPosition();

    // Collect group member positions within heal range
    ::std::vector<Position> memberPositions;
    for (auto const& slot : group->GetMemberSlots())
    {
        if (slot.guid == _bot->GetGUID())
            continue;

        Player* member = ObjectAccessor::FindPlayer(slot.guid);
        if (!member || !member->IsInWorld() || !member->IsAlive())
            continue;
        if (member->GetMapId() != _bot->GetMapId())
            continue;

        float distSq = _bot->GetExactDistSq(member);
        if (distSq <= 50.0f * 50.0f) // Slightly beyond heal range to consider
            memberPositions.push_back(member->GetPosition());
    }

    if (memberPositions.empty())
        return _bot->GetPosition();

    // Calculate centroid of group
    float centroidX = 0.0f, centroidY = 0.0f, centroidZ = 0.0f;
    for (Position const& pos : memberPositions)
    {
        centroidX += pos.GetPositionX();
        centroidY += pos.GetPositionY();
        centroidZ += pos.GetPositionZ();
    }
    float count = static_cast<float>(memberPositions.size());
    centroidX /= count;
    centroidY /= count;
    centroidZ /= count;

    // Try positions around the centroid
    Position bestPos = _bot->GetPosition();
    uint32 bestVisible = 0;
    float bestMoveDist = 999.0f;

    Position centroid;
    centroid.m_positionX = centroidX;
    centroid.m_positionY = centroidY;
    centroid.m_positionZ = centroidZ;

    Map* map = _bot->GetMap();
    if (!map)
        return _bot->GetPosition();

    for (float angle = 0.0f; angle < 2.0f * static_cast<float>(M_PI); angle += static_cast<float>(M_PI) / 6.0f)
    {
        for (float dist = 3.0f; dist <= 15.0f; dist += 4.0f)
        {
            Position candidate;
            candidate.m_positionX = centroidX + dist * ::std::cos(angle);
            candidate.m_positionY = centroidY + dist * ::std::sin(angle);
            candidate.m_positionZ = centroidZ;

            // Correct Z to ground
            float groundZ = map->GetHeight(_bot->GetPhaseShift(), candidate.m_positionX,
                                            candidate.m_positionY, candidate.m_positionZ + 10.0f);
            if (groundZ <= INVALID_HEIGHT)
                continue;
            candidate.m_positionZ = groundZ + 0.5f;

            // Count visible members from this position
            uint32 visible = 0;
            for (Position const& memberPos : memberPositions)
            {
                if (_losMgr->HasLineOfSightFromPosition(candidate, nullptr))
                {
                    // Simple distance check as proxy (full LOS check too expensive for each candidate)
                    float memberDist = candidate.GetExactDist(&memberPos);
                    if (memberDist <= 40.0f)
                        visible++;
                }
            }

            float moveDist = candidate.GetExactDist(_bot);
            if (visible > bestVisible || (visible == bestVisible && moveDist < bestMoveDist))
            {
                bestVisible = visible;
                bestMoveDist = moveDist;
                bestPos = candidate;
            }
        }
    }

    return bestPos;
}

// ============================================================================
// QUERIES
// ============================================================================

uint32 ProactiveLoSFixer::GetRepositioningTime() const
{
    if (!_isRepositioning || !_pendingCast.IsValid())
        return 0;

    uint32 now = GameTime::GetGameTimeMS();
    if (now < _pendingCast.queueTimeMs)
        return 0;

    return now - _pendingCast.queueTimeMs;
}

::std::string ProactiveLoSFixer::GetDebugSummary() const
{
    ::std::string summary = "ProactiveLoSFixer: ";
    summary += "checks=" + ::std::to_string(_stats.totalPreCastChecks);
    summary += " clear=" + ::std::to_string(_stats.losWasClear);
    summary += " repos=" + ::std::to_string(_stats.repositionAttempts);
    summary += " success=" + ::std::to_string(_stats.repositionSuccesses);
    summary += " timeout=" + ::std::to_string(_stats.repositionTimeouts);
    summary += " nopos=" + ::std::to_string(_stats.noPositionFound);

    if (_pendingCast.IsValid())
    {
        summary += " [PENDING: spell=" + ::std::to_string(_pendingCast.spellId);
        summary += " time=" + ::std::to_string(GetRepositioningTime()) + "ms]";
    }

    return summary;
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

Position ProactiveLoSFixer::FindCastPosition(Unit* target, float spellRange) const
{
    if (!target || !_losMgr)
        return _bot->GetPosition();

    // First, try LineOfSightManager's smart position finding
    float preferredRange = spellRange > 5.0f ? spellRange * 0.8f : 10.0f;
    Position bestPos = _losMgr->FindBestLineOfSightPosition(target, preferredRange);

    // Verify the found position is within spell range
    if (bestPos.GetExactDist(target) > spellRange && spellRange > 0.0f)
    {
        // Position is out of spell range, try with a tighter radius
        bestPos = _losMgr->FindBestLineOfSightPosition(target, spellRange * 0.9f);
    }

    // If still no good position, try GetClosestUnblockedPosition
    if (bestPos.GetExactDist(_bot) < 1.0f || (bestPos.GetExactDist(target) > spellRange && spellRange > 0.0f))
    {
        bestPos = _losMgr->GetClosestUnblockedPosition(target);
    }

    // Correct Z to ground level
    BotMovementUtil::CorrectPositionToGround(_bot, bestPos);

    return bestPos;
}

Position ProactiveLoSFixer::FindCastPositionForGround(Position const& targetPos, float spellRange) const
{
    if (!_losMgr)
        return _bot->GetPosition();

    // For ground-targeted spells, find a position within range with LOS to the ground target
    Map* map = _bot->GetMap();
    if (!map)
        return _bot->GetPosition();

    Position botPos = _bot->GetPosition();
    Position bestPos = botPos;
    float bestScore = -1.0f;

    for (float angle = 0.0f; angle < 2.0f * static_cast<float>(M_PI); angle += static_cast<float>(M_PI) / 8.0f)
    {
        for (float dist = 3.0f; dist <= spellRange; dist += 3.0f)
        {
            Position candidate;
            candidate.m_positionX = targetPos.GetPositionX() + dist * ::std::cos(angle);
            candidate.m_positionY = targetPos.GetPositionY() + dist * ::std::sin(angle);
            candidate.m_positionZ = targetPos.GetPositionZ();

            float groundZ = map->GetHeight(_bot->GetPhaseShift(), candidate.m_positionX,
                                            candidate.m_positionY, candidate.m_positionZ + 10.0f);
            if (groundZ <= INVALID_HEIGHT)
                continue;
            candidate.m_positionZ = groundZ + 0.5f;

            float rangeDist = candidate.GetExactDist(&targetPos);
            if (rangeDist > spellRange)
                continue;

            if (_losMgr->CanMoveToPosition(candidate))
            {
                float moveDist = candidate.GetExactDist(&botPos);
                float score = 100.0f - moveDist;
                if (score > bestScore)
                {
                    bestScore = score;
                    bestPos = candidate;
                }
            }
        }
    }

    return bestPos;
}

bool ProactiveLoSFixer::HasReachedRepositionTarget() const
{
    if (!_pendingCast.IsValid())
        return false;

    float dist = _bot->GetExactDist(&_pendingCast.repositionTarget);
    return dist <= REPOSITION_ARRIVAL_TOLERANCE;
}

bool ProactiveLoSFixer::DoesSpellIgnoreLos(SpellInfo const* spellInfo) const
{
    if (!spellInfo)
        return false;

    // Check SPELL_ATTR2_IGNORE_LINE_OF_SIGHT
    if (spellInfo->HasAttribute(SPELL_ATTR2_IGNORE_LINE_OF_SIGHT))
        return true;

    // Self-targeted spells don't need LOS
    if (spellInfo->GetMaxRange() <= 0.0f)
        return true;

    return false;
}

float ProactiveLoSFixer::GetEffectiveSpellRange(uint32 spellId) const
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return 0.0f;

    return spellInfo->GetMaxRange();
}

bool ProactiveLoSFixer::IsHealerRole() const
{
    if (!_bot)
        return false;

    ChrSpecializationEntry const* spec = _bot->GetPrimarySpecializationEntry();
    if (!spec)
        return false;

    return spec->GetRole() == ChrSpecializationRole::Healer;
}

bool ProactiveLoSFixer::IsRangedRole() const
{
    if (!_bot)
        return false;

    ChrSpecializationEntry const* spec = _bot->GetPrimarySpecializationEntry();
    if (!spec)
        return false;

    // Healers are ranged
    if (spec->GetRole() == ChrSpecializationRole::Healer)
        return true;

    // Check Ranged or Caster flag
    if (spec->GetFlags().HasFlag(ChrSpecializationFlag::Ranged) ||
        spec->GetFlags().HasFlag(ChrSpecializationFlag::Caster))
        return true;

    return false;
}

void ProactiveLoSFixer::MoveToLoSPosition(Position const& pos)
{
    if (!_bot)
        return;

    BotMovementUtil::MoveToPosition(_bot, pos, 0, 0.5f);
}

} // namespace Playerbot
