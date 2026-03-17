/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * DirtyValue<T>: Lazy-evaluation wrapper that recalculates a value only when
 * marked dirty. Eliminates redundant per-tick recomputation for values that
 * change infrequently.
 *
 * Usage:
 *   DirtyValue<float> threatScore([this]() { return ComputeThreatScore(); });
 *   threatScore.Invalidate();       // Mark stale
 *   float val = threatScore.Get();  // Recomputes only if dirty
 *
 * Thread Safety: NOT thread-safe. For per-bot use only.
 */

#pragma once

#include <functional>
#include <optional>

namespace Playerbot
{

template <typename T>
class DirtyValue
{
public:
    using ComputeFn = std::function<T()>;

    /// Construct with a computation function.
    explicit DirtyValue(ComputeFn computeFn)
        : _computeFn(std::move(computeFn))
        , _dirty(true)
    {}

    /// Construct with an initial value and computation function.
    DirtyValue(T initialValue, ComputeFn computeFn)
        : _computeFn(std::move(computeFn))
        , _cached(std::move(initialValue))
        , _dirty(false)
    {}

    /// Default constructor - no compute function, starts dirty.
    DirtyValue()
        : _dirty(true)
    {}

    /// Get the value, recomputing if dirty.
    T const& Get()
    {
        if (_dirty && _computeFn)
        {
            _cached = _computeFn();
            _dirty = false;
        }
        return _cached;
    }

    /// Get the value without recomputing (may be stale).
    T const& GetCached() const
    {
        return _cached;
    }

    /// Check if the value needs recomputation.
    bool IsDirty() const { return _dirty; }

    /// Mark the value as needing recomputation.
    void Invalidate() { _dirty = true; }

    /// Set the value directly (clears dirty flag).
    void Set(T value)
    {
        _cached = std::move(value);
        _dirty = false;
    }

    /// Set a new computation function.
    void SetComputeFn(ComputeFn fn) { _computeFn = std::move(fn); }

    /// Implicit conversion to T (triggers recompute if dirty).
    operator T const&() { return Get(); }

    /// Assignment from T (direct set, clears dirty).
    DirtyValue& operator=(T value)
    {
        Set(std::move(value));
        return *this;
    }

private:
    ComputeFn _computeFn;
    T _cached{};
    bool _dirty{true};
};

/// DirtyValue with a TTL (time-to-live). Automatically invalidates after
/// a specified duration, even if not explicitly marked dirty.
template <typename T>
class TimedDirtyValue
{
public:
    using ComputeFn = std::function<T()>;

    /// Construct with compute function and TTL in milliseconds.
    TimedDirtyValue(ComputeFn computeFn, uint32 ttlMs)
        : _computeFn(std::move(computeFn))
        , _ttlMs(ttlMs)
        , _lastComputeTime(0)
        , _dirty(true)
    {}

    /// Construct with initial value, compute function, and TTL.
    TimedDirtyValue(T initialValue, ComputeFn computeFn, uint32 ttlMs)
        : _computeFn(std::move(computeFn))
        , _cached(std::move(initialValue))
        , _ttlMs(ttlMs)
        , _lastComputeTime(0)
        , _dirty(false)
    {}

    /// Get the value, recomputing if dirty or TTL expired.
    /// @param nowMs Current time in milliseconds (e.g., from getMSTime())
    T const& Get(uint32 nowMs)
    {
        if (_dirty || (nowMs - _lastComputeTime >= _ttlMs))
        {
            if (_computeFn)
            {
                _cached = _computeFn();
                _lastComputeTime = nowMs;
                _dirty = false;
            }
        }
        return _cached;
    }

    /// Get without recomputing.
    T const& GetCached() const { return _cached; }

    /// Check if value is stale.
    bool IsDirty() const { return _dirty; }

    /// Check if TTL has expired.
    bool IsExpired(uint32 nowMs) const { return (nowMs - _lastComputeTime) >= _ttlMs; }

    /// Mark as needing recomputation.
    void Invalidate() { _dirty = true; }

    /// Set directly.
    void Set(T value, uint32 nowMs)
    {
        _cached = std::move(value);
        _lastComputeTime = nowMs;
        _dirty = false;
    }

    /// Update TTL.
    void SetTTL(uint32 ttlMs) { _ttlMs = ttlMs; }

    /// Get current TTL.
    uint32 GetTTL() const { return _ttlMs; }

private:
    ComputeFn _computeFn;
    T _cached{};
    uint32 _ttlMs;
    uint32 _lastComputeTime;
    bool _dirty;
};

} // namespace Playerbot
