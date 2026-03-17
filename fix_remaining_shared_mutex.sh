#!/bin/bash
# DEADLOCK FIX #17: Replace ALL remaining std::shared_mutex outside AI/ subsystem

echo "=== DEADLOCK FIX #17: Final Shared Mutex Replacement ==="
echo ""

# List of all header files with remaining std::shared_mutex
HEADER_FILES=(
    "src/modules/Playerbot/Account/BotAccountMgr.h"
    "src/modules/Playerbot/Database/BotDatabasePool.h"
    "src/modules/Playerbot/Interaction/Core/GossipHandler.h"
    "src/modules/Playerbot/Interaction/Core/InteractionManager.h"
    "src/modules/Playerbot/Interaction/Core/InteractionValidator.h"
    "src/modules/Playerbot/Interaction/Vendors/VendorInteraction.h"
    "src/modules/Playerbot/Lifecycle/BotLifecycleManager.h"
    "src/modules/Playerbot/Lifecycle/BotLifecycleMgr.h"
    "src/modules/Playerbot/Lifecycle/BotScheduler.h"
    "src/modules/Playerbot/Movement/Core/MovementManager.h"
    "src/modules/Playerbot/Movement/Core/MovementValidator.h"
    "src/modules/Playerbot/Movement/Pathfinding/PathfindingAdapter.h"
    "src/modules/Playerbot/Performance/MemoryPool/MemoryPool.h"
)

# Corresponding CPP files
CPP_FILES=(
    "src/modules/Playerbot/Account/BotAccountMgr.cpp"
    "src/modules/Playerbot/Database/BotDatabasePool.cpp"
    "src/modules/Playerbot/Interaction/Core/GossipHandler.cpp"
    "src/modules/Playerbot/Interaction/Core/InteractionManager.cpp"
    "src/modules/Playerbot/Interaction/Core/InteractionManager_COMPLETE_FIX.cpp"
    "src/modules/Playerbot/Interaction/Core/InteractionValidator.cpp"
    "src/modules/Playerbot/Interaction/Vendors/VendorInteraction.cpp"
    "src/modules/Playerbot/Lifecycle/BotLifecycleManager.cpp"
    "src/modules/Playerbot/Lifecycle/BotLifecycleMgr.cpp"
    "src/modules/Playerbot/Lifecycle/BotScheduler.cpp"
    "src/modules/Playerbot/Movement/Core/MovementManager.cpp"
    "src/modules/Playerbot/Movement/Core/MovementValidator.cpp"
    "src/modules/Playerbot/Movement/Pathfinding/PathfindingAdapter.cpp"
)

echo "Step 1: Replacing std::shared_mutex with std::recursive_mutex in headers..."
for file in "${HEADER_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "  Processing: $file"
        sed -i 's/mutable std::shared_mutex/mutable std::recursive_mutex/g' "$file"
    fi
done
echo ""

echo "Step 2: Replacing lock types in CPP files..."
for file in "${CPP_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "  Processing: $file"
        # Replace all lock types with recursive_mutex locks
        sed -i 's/std::unique_lock<std::shared_mutex> lock/std::lock_guard<std::recursive_mutex> lock/g' "$file"
        sed -i 's/std::shared_lock<std::shared_mutex> lock/std::lock_guard<std::recursive_mutex> lock/g' "$file"
        sed -i 's/std::lock_guard<std::shared_mutex>/std::lock_guard<std::recursive_mutex>/g' "$file"
    fi
done
echo ""

echo "Step 3: Verification - checking for remaining std::shared_mutex..."
REMAINING=$(grep -r "std::shared_mutex" src/modules/Playerbot/ --include="*.h" --include="*.cpp" | grep -v "// " | grep -v "AI/" | wc -l)
echo "  Remaining std::shared_mutex instances: $REMAINING"
echo ""

if [ "$REMAINING" -eq 0 ]; then
    echo "✅ SUCCESS: All std::shared_mutex instances replaced!"
else
    echo "⚠️  WARNING: $REMAINING instances still remain"
    grep -r "std::shared_mutex" src/modules/Playerbot/ --include="*.h" --include="*.cpp" | grep -v "// " | grep -v "AI/" | head -20
fi

echo ""
echo "=== Final Mutex Replacement Complete ==="
