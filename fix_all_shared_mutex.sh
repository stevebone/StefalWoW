#!/bin/bash
# DEADLOCK FIX #16: Replace ALL std::shared_mutex with std::recursive_mutex
# This comprehensive fix addresses ALL remaining shared_mutex instances

echo "=== DEADLOCK FIX #16: Comprehensive Mutex Replacement ==="
echo ""

# List of all header files with std::shared_mutex (excluding already fixed)
HEADER_FILES=(
    "src/modules/Playerbot/AI/BotAI_Refactored.h"
    "src/modules/Playerbot/AI/ClassAI/CombatSpecializationTemplates.h"
    "src/modules/Playerbot/AI/ClassAI/PositionStrategyBase.h"
    "src/modules/Playerbot/AI/ClassAI/Warriors/ProtectionSpecialization.h"
    "src/modules/Playerbot/AI/Combat/BotThreatManager.h"
    "src/modules/Playerbot/AI/Combat/CombatAIIntegrator.h"
    "src/modules/Playerbot/AI/Combat/FormationManager.h"
    "src/modules/Playerbot/AI/Combat/InterruptAwareness.h"
    "src/modules/Playerbot/AI/Combat/InterruptCoordinator.h"
    "src/modules/Playerbot/AI/Combat/InterruptCoordinatorFixed.h"
    "src/modules/Playerbot/AI/Combat/KitingManager.h"
    "src/modules/Playerbot/AI/Combat/LineOfSightManager.h"
    "src/modules/Playerbot/AI/Combat/MechanicAwareness.h"
    "src/modules/Playerbot/AI/Combat/ObstacleAvoidanceManager.h"
    "src/modules/Playerbot/AI/Combat/PathfindingManager.h"
    "src/modules/Playerbot/AI/Combat/PositionManager.h"
    "src/modules/Playerbot/AI/Combat/RoleBasedCombatPositioning.h"
    "src/modules/Playerbot/AI/Combat/TargetSelector.h"
)

# Corresponding CPP files
CPP_FILES=(
    "src/modules/Playerbot/AI/Combat/BotThreatManager.cpp"
    "src/modules/Playerbot/AI/Combat/CombatAIIntegrator.cpp"
    "src/modules/Playerbot/AI/Combat/FormationManager.cpp"
    "src/modules/Playerbot/AI/Combat/InterruptAwareness.cpp"
    "src/modules/Playerbot/AI/Combat/KitingManager.cpp"
    "src/modules/Playerbot/AI/Combat/LineOfSightManager.cpp"
    "src/modules/Playerbot/AI/Combat/MechanicAwareness.cpp"
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
        # Replace std::unique_lock<std::shared_mutex> with std::lock_guard<std::recursive_mutex>
        sed -i 's/std::unique_lock<std::shared_mutex> lock/std::lock_guard<std::recursive_mutex> lock/g' "$file"
        # Replace std::shared_lock<std::shared_mutex> with std::lock_guard<std::recursive_mutex>
        sed -i 's/std::shared_lock<std::shared_mutex> lock/std::lock_guard<std::recursive_mutex> lock/g' "$file"
        # Replace std::lock_guard<std::shared_mutex> with std::lock_guard<std::recursive_mutex>
        sed -i 's/std::lock_guard<std::shared_mutex>/std::lock_guard<std::recursive_mutex>/g' "$file"
    fi
done
echo ""

echo "Step 3: Verification - checking for remaining std::shared_mutex..."
REMAINING=$(grep -r "std::shared_mutex" src/modules/Playerbot/AI/ --include="*.h" --include="*.cpp" | grep -v "// " | wc -l)
echo "  Remaining std::shared_mutex instances: $REMAINING"
echo ""

if [ "$REMAINING" -eq 0 ]; then
    echo "✅ SUCCESS: All std::shared_mutex instances replaced!"
else
    echo "⚠️  WARNING: $REMAINING instances still remain"
    grep -r "std::shared_mutex" src/modules/Playerbot/AI/ --include="*.h" --include="*.cpp" | grep -v "// " | head -20
fi

echo ""
echo "=== Mutex Replacement Complete ==="
