/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "MovementRequest.h"
#include "Timer.h"
#include <sstream>
#include <thread>
#include <atomic>
#include <stdexcept>
#include "GameTime.h"

namespace Playerbot
{

// ============================================================================
// Request ID Generator
// ============================================================================

uint64 MovementRequest::GenerateRequestId()
{
    static ::std::atomic<uint64> counter{1};
    return counter.fetch_add(1, ::std::memory_order_relaxed);
}

// ============================================================================
// Thread ID Helper
// ============================================================================

uint32 MovementRequest::GetCurrentThreadId()
{
    return static_cast<uint32>(::std::hash<::std::thread::id>{}(::std::this_thread::get_id()));
}

// ============================================================================
// Parameter Struct Equality Operators
// ============================================================================

bool PointMovementParams::operator==(PointMovementParams const& other) const
{
    // Compare positions with small epsilon for floating point comparison
    constexpr float EPSILON = 0.001f;

    if (::std::abs(targetPos.GetPositionX() - other.targetPos.GetPositionX()) > EPSILON ||
        ::std::abs(targetPos.GetPositionY() - other.targetPos.GetPositionY()) > EPSILON ||
        ::std::abs(targetPos.GetPositionZ() - other.targetPos.GetPositionZ()) > EPSILON)
        return false;

    if (generatePath != other.generatePath)
        return false;

    // Compare Optional<float> values
    if (finalOrient.has_value() != other.finalOrient.has_value())
        return false;
    if (finalOrient.has_value() && ::std::abs(*finalOrient - *other.finalOrient) > EPSILON)
        return false;

    if (speed.has_value() != other.speed.has_value())
        return false;
    if (speed.has_value() && ::std::abs(*speed - *other.speed) > EPSILON)
        return false;

    if (closeEnoughDistance.has_value() != other.closeEnoughDistance.has_value())
        return false;
    if (closeEnoughDistance.has_value() && ::std::abs(*closeEnoughDistance - *other.closeEnoughDistance) > EPSILON)
        return false;

    return true;
}

bool ChaseMovementParams::operator==(ChaseMovementParams const& other) const
{
    if (targetGuid != other.targetGuid)
        return false;

    // Compare Optional<ChaseRange>
    if (range.has_value() != other.range.has_value())
        return false;
    if (range.has_value())
    {
        constexpr float EPSILON = 0.001f;
        if (::std::abs(range->MinRange - other.range->MinRange) > EPSILON ||
            ::std::abs(range->MinTolerance - other.range->MinTolerance) > EPSILON ||
            ::std::abs(range->MaxRange - other.range->MaxRange) > EPSILON ||
            ::std::abs(range->MaxTolerance - other.range->MaxTolerance) > EPSILON)
            return false;
    }

    // Compare Optional<ChaseAngle>
    if (angle.has_value() != other.angle.has_value())
        return false;
    if (angle.has_value())
    {
        constexpr float EPSILON = 0.001f;
        if (::std::abs(angle->RelativeAngle - other.angle->RelativeAngle) > EPSILON ||
            ::std::abs(angle->Tolerance - other.angle->Tolerance) > EPSILON)
            return false;
    }

    return true;
}

bool FollowMovementParams::operator==(FollowMovementParams const& other) const
{
    constexpr float EPSILON = 0.001f;

    if (targetGuid != other.targetGuid)
        return false;

    if (::std::abs(distance - other.distance) > EPSILON)
        return false;

    // Compare Optional<ChaseAngle>
    if (angle.has_value() != other.angle.has_value())
        return false;
    if (angle.has_value())
    {
        if (::std::abs(angle->RelativeAngle - other.angle->RelativeAngle) > EPSILON ||
            ::std::abs(angle->Tolerance - other.angle->Tolerance) > EPSILON)
            return false;
    }

    // Compare Optional<Milliseconds>
    if (duration.has_value() != other.duration.has_value())
        return false;
    if (duration.has_value() && *duration != *other.duration)
        return false;

    return true;
}

bool JumpMovementParams::operator==(JumpMovementParams const& other) const
{
    constexpr float EPSILON = 0.001f;

    if (::std::abs(targetPos.GetPositionX() - other.targetPos.GetPositionX()) > EPSILON ||
        ::std::abs(targetPos.GetPositionY() - other.targetPos.GetPositionY()) > EPSILON ||
        ::std::abs(targetPos.GetPositionZ() - other.targetPos.GetPositionZ()) > EPSILON)
        return false;

    if (::std::abs(speedXY - other.speedXY) > EPSILON ||
        ::std::abs(speedZ - other.speedZ) > EPSILON)
        return false;

    if (eventId != other.eventId)
        return false;

    return true;
}

// ============================================================================
// MovementRequest Construction
// ============================================================================

MovementRequest::MovementRequest(PlayerBotMovementPriority priority, ::std::string reason)
    : _requestId(GenerateRequestId())
    , _priority(priority)
    , _type(MovementRequestType::NONE)
    , _reason(::std::move(reason))
    , _sourceSystem("")
    , _sourceThreadId(GetCurrentThreadId())
    , _timestamp(GameTime::GetGameTimeMS())
    , _expectedDuration(0)
    , _allowInterrupt(true)
    , _params(::std::monostate{})
{
}

// ============================================================================
// Static Factory Methods
// ============================================================================

MovementRequest MovementRequest::MakePointMovement(
    PlayerBotMovementPriority priority,
    Position const& targetPos,
    bool generatePath,
    Optional<float> finalOrient,
    Optional<float> speed,
    Optional<float> closeEnoughDistance,
    ::std::string reason,
    ::std::string sourceSystem)
{
    MovementRequest req(priority, ::std::move(reason));
    req._type = MovementRequestType::POINT;
    req._sourceSystem = ::std::move(sourceSystem);

    PointMovementParams params;
    params.targetPos = targetPos;
    params.generatePath = generatePath;
    params.finalOrient = finalOrient;
    params.speed = speed;
    params.closeEnoughDistance = closeEnoughDistance;

    req._params = ::std::move(params);
    return req;
}

MovementRequest MovementRequest::MakeChaseMovement(
    PlayerBotMovementPriority priority,
    ObjectGuid targetGuid,
    Optional<ChaseRange> range,
    Optional<ChaseAngle> angle,
    ::std::string reason,
    ::std::string sourceSystem)
{
    MovementRequest req(priority, ::std::move(reason));
    req._type = MovementRequestType::CHASE;
    req._sourceSystem = ::std::move(sourceSystem);

    ChaseMovementParams params;
    params.targetGuid = targetGuid;
    params.range = range;
    params.angle = angle;

    req._params = ::std::move(params);
    return req;
}

MovementRequest MovementRequest::MakeFollowMovement(
    PlayerBotMovementPriority priority,
    ObjectGuid targetGuid,
    float distance,
    Optional<ChaseAngle> angle,
    Optional<Milliseconds> duration,
    ::std::string reason,
    ::std::string sourceSystem)
{
    MovementRequest req(priority, ::std::move(reason));
    req._type = MovementRequestType::FOLLOW;
    req._sourceSystem = ::std::move(sourceSystem);

    FollowMovementParams params;
    params.targetGuid = targetGuid;
    params.distance = distance;
    params.angle = angle;
    params.duration = duration;

    req._params = ::std::move(params);
    return req;
}

MovementRequest MovementRequest::MakeIdleMovement(
    PlayerBotMovementPriority priority,
    ::std::string reason,
    ::std::string sourceSystem)
{
    MovementRequest req(priority, ::std::move(reason));
    req._type = MovementRequestType::IDLE;
    req._sourceSystem = ::std::move(sourceSystem);
    req._params = IdleMovementParams{};
    return req;
}

MovementRequest MovementRequest::MakeJumpMovement(
    PlayerBotMovementPriority priority,
    Position const& targetPos,
    float speedXY,
    float speedZ,
    uint32 eventId,
    ::std::string reason,
    ::std::string sourceSystem)
{
    MovementRequest req(priority, ::std::move(reason));
    req._type = MovementRequestType::JUMP;
    req._sourceSystem = ::std::move(sourceSystem);

    JumpMovementParams params;
    params.targetPos = targetPos;
    params.speedXY = speedXY;
    params.speedZ = speedZ;
    params.eventId = eventId;

    req._params = params;
    return req;
}

// ============================================================================
// NEW: TrinityCore 12.0 Movement Features - Factory Methods
// ============================================================================

MovementRequest MovementRequest::MakeRandomMovement(
    PlayerBotMovementPriority priority,
    Position const& centerPos,
    float wanderDistance,
    Optional<Milliseconds> duration,
    bool forceWalk,
    ::std::string reason,
    ::std::string sourceSystem)
{
    MovementRequest req(priority, ::std::move(reason));
    req._type = MovementRequestType::RANDOM;
    req._sourceSystem = ::std::move(sourceSystem);

    RandomMovementParams params;
    params.centerPos = centerPos;
    params.wanderDistance = wanderDistance;
    params.duration = duration;
    params.forceWalk = forceWalk;

    req._params = ::std::move(params);
    return req;
}

MovementRequest MovementRequest::MakePathMovement(
    PlayerBotMovementPriority priority,
    uint32 pathId,
    bool repeatable,
    Optional<Milliseconds> duration,
    Optional<float> speed,
    bool forceWalk,
    ::std::string reason,
    ::std::string sourceSystem)
{
    MovementRequest req(priority, ::std::move(reason));
    req._type = MovementRequestType::PATH;
    req._sourceSystem = ::std::move(sourceSystem);

    PathMovementParams params;
    params.pathId = pathId;
    params.repeatable = repeatable;
    params.duration = duration;
    params.speed = speed;
    params.forceWalk = forceWalk;

    req._params = ::std::move(params);
    return req;
}

// ============================================================================
// Type-Specific Parameter Getters (Throwing)
// ============================================================================

PointMovementParams const& MovementRequest::GetPointParams() const
{
    if (auto* params = ::std::get_if<PointMovementParams>(&_params))
        return *params;
    throw ::std::runtime_error("MovementRequest::GetPointParams() called on non-POINT request");
}

ChaseMovementParams const& MovementRequest::GetChaseParams() const
{
    if (auto* params = ::std::get_if<ChaseMovementParams>(&_params))
        return *params;
    throw ::std::runtime_error("MovementRequest::GetChaseParams() called on non-CHASE request");
}

FollowMovementParams const& MovementRequest::GetFollowParams() const
{
    if (auto* params = ::std::get_if<FollowMovementParams>(&_params))
        return *params;
    throw ::std::runtime_error("MovementRequest::GetFollowParams() called on non-FOLLOW request");
}

JumpMovementParams const& MovementRequest::GetJumpParams() const
{
    if (auto* params = ::std::get_if<JumpMovementParams>(&_params))
        return *params;
    throw ::std::runtime_error("MovementRequest::GetJumpParams() called on non-JUMP request");
}

IdleMovementParams const& MovementRequest::GetIdleParams() const
{
    if (auto* params = ::std::get_if<IdleMovementParams>(&_params))
        return *params;
    throw ::std::runtime_error("MovementRequest::GetIdleParams() called on non-IDLE request");
}

RandomMovementParams const& MovementRequest::GetRandomParams() const
{
    if (auto* params = ::std::get_if<RandomMovementParams>(&_params))
        return *params;
    throw ::std::runtime_error("MovementRequest::GetRandomParams() called on non-RANDOM request");
}

PathMovementParams const& MovementRequest::GetPathParams() const
{
    if (auto* params = ::std::get_if<PathMovementParams>(&_params))
        return *params;
    throw ::std::runtime_error("MovementRequest::GetPathParams() called on non-PATH request");
}

// ============================================================================
// Safe Parameter Getters (Optional)
// ============================================================================

Optional<PointMovementParams> MovementRequest::TryGetPointParams() const
{
    if (auto* params = ::std::get_if<PointMovementParams>(&_params))
        return *params;
    return {};
}

Optional<ChaseMovementParams> MovementRequest::TryGetChaseParams() const
{
    if (auto* params = ::std::get_if<ChaseMovementParams>(&_params))
        return *params;
    return {};
}

Optional<FollowMovementParams> MovementRequest::TryGetFollowParams() const
{
    if (auto* params = ::std::get_if<FollowMovementParams>(&_params))
        return *params;
    return {};
}

Optional<RandomMovementParams> MovementRequest::TryGetRandomParams() const
{
    if (auto* params = ::std::get_if<RandomMovementParams>(&_params))
        return *params;
    return {};
}

Optional<PathMovementParams> MovementRequest::TryGetPathParams() const
{
    if (auto* params = ::std::get_if<PathMovementParams>(&_params))
        return *params;
    return {};
}

// ============================================================================
// Fluent Setters
// ============================================================================

MovementRequest& MovementRequest::SetSourceSystem(::std::string sourceSystem)
{
    _sourceSystem = ::std::move(sourceSystem);
    return *this;
}

MovementRequest& MovementRequest::SetExpectedDuration(uint32 durationMs)
{
    _expectedDuration = durationMs;
    return *this;
}

MovementRequest& MovementRequest::SetAllowInterrupt(bool allow)
{
    _allowInterrupt = allow;
    return *this;
}

// ============================================================================
// Spatial-Temporal Hashing for Deduplication
// ============================================================================

uint64 MovementRequest::GetSpatialTemporalHash() const
{
    // Hash combines:
    // - Request type (8 bits)
    // - Priority (8 bits)
    // - Spatial position (5-yard grid for POINT) OR target GUID (for CHASE/FOLLOW)

    uint64 hash = 0;

    // Type and priority (lower 16 bits)
    hash |= static_cast<uint64>(static_cast<uint8>(_type)) << 8;
    hash |= static_cast<uint64>(static_cast<uint8>(_priority));

    // Spatial component (upper 48 bits)
    switch (_type)
    {
        case MovementRequestType::POINT:
        {
            auto const& params = GetPointParams();

            // Quantize position to 5-yard grid
            constexpr float GRID_SIZE = 5.0f;
            int32 gridX = static_cast<int32>(params.targetPos.GetPositionX() / GRID_SIZE);
            int32 gridY = static_cast<int32>(params.targetPos.GetPositionY() / GRID_SIZE);
            int32 gridZ = static_cast<int32>(params.targetPos.GetPositionZ() / GRID_SIZE);

            // Combine grid coordinates into hash (16 bits each)
            hash |= (static_cast<uint64>(gridX & 0xFFFF) << 16);
            hash |= (static_cast<uint64>(gridY & 0xFFFF) << 32);
            hash |= (static_cast<uint64>(gridZ & 0xFFFF) << 48);
            break;
        }

        case MovementRequestType::CHASE:
        {
            auto const& params = GetChaseParams();
            // Use lower 48 bits of GUID (index 0 is the lower 64 bits)
            hash |= (params.targetGuid.GetRawValue(0) & 0xFFFFFFFFFFFFULL) << 16;
            break;
        }

        case MovementRequestType::FOLLOW:
        {
            auto const& params = GetFollowParams();
            // Use lower 48 bits of GUID (index 0 is the lower 64 bits)
            hash |= (params.targetGuid.GetRawValue(0) & 0xFFFFFFFFFFFFULL) << 16;
            break;
        }

        case MovementRequestType::IDLE:
            // Idle requests have same hash (all identical)
            break;

        case MovementRequestType::RANDOM:
        {
            auto const& params = GetRandomParams();

            // Quantize center position to 5-yard grid
            constexpr float GRID_SIZE = 5.0f;
            int32 gridX = static_cast<int32>(params.centerPos.GetPositionX() / GRID_SIZE);
            int32 gridY = static_cast<int32>(params.centerPos.GetPositionY() / GRID_SIZE);

            // Include wander distance in hash (quantized to 1-yard increments)
            int32 wanderKey = static_cast<int32>(params.wanderDistance);

            hash |= (static_cast<uint64>(gridX & 0xFFFF) << 16);
            hash |= (static_cast<uint64>(gridY & 0xFFFF) << 32);
            hash |= (static_cast<uint64>(wanderKey & 0xFFFF) << 48);
            break;
        }

        case MovementRequestType::PATH:
        {
            auto const& params = GetPathParams();

            // Path ID is the primary distinguisher
            hash |= (static_cast<uint64>(params.pathId) << 16);
            hash |= (static_cast<uint64>(params.repeatable ? 1 : 0) << 48);
            break;
        }

        default:
            break;
    }

    return hash;
}

// ============================================================================
// Duplicate Detection
// ============================================================================

bool MovementRequest::IsDuplicateOf(MovementRequest const& other) const
{
    // Different types or priorities are never duplicates
    if (_type != other._type || _priority != other._priority)
        return false;

    // Type-specific duplicate checking
    switch (_type)
    {
        case MovementRequestType::POINT:
        {
            auto const& params1 = GetPointParams();
            auto const& params2 = other.GetPointParams();

            // Consider duplicate if within 0.3 yards (spatial proximity threshold)
            constexpr float DUPLICATE_THRESHOLD = 0.3f;

            float dx = params1.targetPos.GetPositionX() - params2.targetPos.GetPositionX();
            float dy = params1.targetPos.GetPositionY() - params2.targetPos.GetPositionY();
            float dz = params1.targetPos.GetPositionZ() - params2.targetPos.GetPositionZ();

            float distSq = dx * dx + dy * dy + dz * dz;

            return distSq < (DUPLICATE_THRESHOLD * DUPLICATE_THRESHOLD);
        }

        case MovementRequestType::CHASE:
        {
            auto const& params1 = GetChaseParams();
            auto const& params2 = other.GetChaseParams();

            // Same target = duplicate
            return params1.targetGuid == params2.targetGuid;
        }

        case MovementRequestType::FOLLOW:
        {
            auto const& params1 = GetFollowParams();
            auto const& params2 = other.GetFollowParams();

            // Same target = duplicate
            return params1.targetGuid == params2.targetGuid;
        }

        case MovementRequestType::IDLE:
            // All idle requests are duplicates of each other
            return true;

        case MovementRequestType::RANDOM:
        {
            auto const& params1 = GetRandomParams();
            auto const& params2 = other.GetRandomParams();

            // Consider duplicate if center is within 5 yards and wander distance similar
            constexpr float CENTER_THRESHOLD = 5.0f;
            constexpr float WANDER_THRESHOLD = 2.0f;

            float dx = params1.centerPos.GetPositionX() - params2.centerPos.GetPositionX();
            float dy = params1.centerPos.GetPositionY() - params2.centerPos.GetPositionY();
            float distSq = dx * dx + dy * dy;

            if (distSq > (CENTER_THRESHOLD * CENTER_THRESHOLD))
                return false;

            if (::std::abs(params1.wanderDistance - params2.wanderDistance) > WANDER_THRESHOLD)
                return false;

            return true;
        }

        case MovementRequestType::PATH:
        {
            auto const& params1 = GetPathParams();
            auto const& params2 = other.GetPathParams();

            // Same path ID = duplicate
            return params1.pathId == params2.pathId;
        }

        default:
            return false;
    }
}

// ============================================================================
// Debug String
// ============================================================================

::std::string MovementRequest::ToString() const
{
    ::std::ostringstream oss;

    oss << "MovementRequest{";
    oss << "id=" << _requestId;
    oss << ", priority=" << static_cast<int>(_priority);
    oss << " (" << MovementPriorityMapper::GetPriorityName(_priority) << ")";

    oss << ", type=";
    switch (_type)
    {
        case MovementRequestType::NONE:      oss << "NONE"; break;
        case MovementRequestType::POINT:     oss << "POINT"; break;
        case MovementRequestType::CHASE:     oss << "CHASE"; break;
        case MovementRequestType::FOLLOW:    oss << "FOLLOW"; break;
        case MovementRequestType::IDLE:      oss << "IDLE"; break;
        case MovementRequestType::JUMP:      oss << "JUMP"; break;
        case MovementRequestType::CHARGE:    oss << "CHARGE"; break;
        case MovementRequestType::KNOCKBACK: oss << "KNOCKBACK"; break;
        case MovementRequestType::CUSTOM:    oss << "CUSTOM"; break;
        case MovementRequestType::RANDOM:    oss << "RANDOM"; break;
        case MovementRequestType::PATH:      oss << "PATH"; break;
        default:                              oss << "UNKNOWN"; break;
    }

    if (!_reason.empty())
        oss << ", reason=\"" << _reason << "\"";

    if (!_sourceSystem.empty())
        oss << ", source=" << _sourceSystem;

    oss << ", threadId=" << _sourceThreadId;
    oss << ", timestamp=" << _timestamp;

    if (_expectedDuration > 0)
        oss << ", duration=" << _expectedDuration << "ms";

    oss << ", canInterrupt=" << (_allowInterrupt ? "yes" : "no");

    // Type-specific parameters
    switch (_type)
    {
        case MovementRequestType::POINT:
        {
            auto const& params = GetPointParams();
            oss << ", pos=(" << params.targetPos.GetPositionX() << ", "
                << params.targetPos.GetPositionY() << ", "
                << params.targetPos.GetPositionZ() << ")";
            if (params.finalOrient.has_value())
                oss << ", orient=" << *params.finalOrient;
            break;
        }

        case MovementRequestType::CHASE:
        {
            auto const& params = GetChaseParams();
            oss << ", target=" << params.targetGuid.ToString();
            break;
        }

        case MovementRequestType::FOLLOW:
        {
            auto const& params = GetFollowParams();
            oss << ", target=" << params.targetGuid.ToString();
            oss << ", distance=" << params.distance;
            break;
        }

        case MovementRequestType::RANDOM:
        {
            auto const& params = GetRandomParams();
            oss << ", center=(" << params.centerPos.GetPositionX() << ", "
                << params.centerPos.GetPositionY() << ", "
                << params.centerPos.GetPositionZ() << ")";
            oss << ", wanderDist=" << params.wanderDistance;
            if (params.duration.has_value())
                oss << ", duration=" << params.duration->count() << "ms";
            oss << ", walk=" << (params.forceWalk ? "yes" : "no");
            break;
        }

        case MovementRequestType::PATH:
        {
            auto const& params = GetPathParams();
            oss << ", pathId=" << params.pathId;
            oss << ", repeatable=" << (params.repeatable ? "yes" : "no");
            if (params.speed.has_value())
                oss << ", speed=" << *params.speed;
            break;
        }

        default:
            break;
    }

    oss << "}";
    return oss.str();
}

// ============================================================================
// Comparison Operators
// ============================================================================

bool MovementRequest::operator==(MovementRequest const& other) const
{
    // Request IDs must match
    if (_requestId != other._requestId)
        return false;

    // All core fields must match
    if (_priority != other._priority ||
        _type != other._type ||
        _reason != other._reason ||
        _sourceSystem != other._sourceSystem ||
        _sourceThreadId != other._sourceThreadId ||
        _timestamp != other._timestamp ||
        _expectedDuration != other._expectedDuration ||
        _allowInterrupt != other._allowInterrupt)
        return false;

    // Compare variant parameters
    return _params == other._params;
}

} // namespace Playerbot
