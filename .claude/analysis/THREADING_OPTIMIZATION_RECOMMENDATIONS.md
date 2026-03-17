# Threading Optimization Recommendations

## Current Configuration (Defaults)

```conf
# TrinityCore (worldserver.conf)
MapUpdate.Threads = 1

# Playerbot (playerbots.conf)
Playerbot.ThreadPool.Size = 4
Playerbot.ThreadPool.MaxQueueSize = 1000
Playerbot.ThreadPool.EnableWorkStealing = 1
```

## Recommended Settings

### For Single-Player Offline (1 Map, 10-50 Bots)

```conf
# worldserver.conf
MapUpdate.Threads = 1  # All bots on one map, no benefit from more

# playerbots.conf
Playerbot.ThreadPool.Size = 4  # 4 workers is sufficient
```

### For Multi-Map Scenario (Multiple Maps, 50-100 Bots)

```conf
# worldserver.conf
MapUpdate.Threads = 4  # Parallel map updates (match CPU cores)

# playerbots.conf
Playerbot.ThreadPool.Size = 8  # More workers for more bots
Playerbot.ThreadPool.MaxQueueSize = 2000
```

### For High-Population Server (200+ Bots across many maps)

```conf
# worldserver.conf
MapUpdate.Threads = 8  # Up to 8 parallel map updates

# playerbots.conf
Playerbot.ThreadPool.Size = 16  # Scale with bot count
Playerbot.ThreadPool.MaxQueueSize = 5000
Playerbot.ThreadPool.TaskTimeout = 10000  # Longer timeout for heavy load
```

## Understanding the Benefits

### MapUpdate.Threads > 1

**When it helps:**
- Bots spread across MULTIPLE maps (different continents, dungeons)
- Each map can be updated in parallel
- Reduces total map update time

**When it doesn't help:**
- All bots on SAME map (they're updated sequentially within that map)
- Single-player offline scenarios

### Playerbot.ThreadPool.Size

**When to increase:**
- More bots (roughly 1 thread per 10-25 bots)
- Complex AI decisions
- Heavy use of spatial queries

**Formula:**
```
Recommended threads = min(CPU_CORES, ceil(BOT_COUNT / 15))
```

## Performance Fixes Applied (2026-02-02)

The following optimizations dramatically reduce per-bot overhead:

1. **GenericEventBus**: 50x fewer mutex acquisitions
2. **GameSystemsManager**: 30x fewer manager updates
3. **SpatialGridManager**: Near-instant grid lookups
4. **CombatEventRouter**: Lock-free stats

With these fixes, the thread pools can now work efficiently without being blocked by lock contention.

## Monitoring Performance

Add to worldserver.conf for monitoring:
```conf
# Enable metrics (if using Prometheus/Grafana)
Metric.Enable = 1

# Log slow map updates
Log.Slow.Map = 1
```

Check logs for:
- `ThreadPool wait took Xms` - Should be <500ms
- `Map update time_diff` - Should be <100ms per map
