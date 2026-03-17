#!/bin/bash
# Fix all void function return errors

echo "================================================================================
"
echo "BATCH FIX - VOID FUNCTION RETURN ERRORS"
echo "================================================================================"

FILES=(
"src/modules/Playerbot/Movement/Pathfinding/PathOptimizer.cpp"
"src/modules/Playerbot/Movement/Pathfinding/PathfindingAdapter.cpp"
"src/modules/Playerbot/AI/ExampleManager.cpp"
"src/modules/Playerbot/Movement/Core/MovementValidator.cpp"
"src/modules/Playerbot/AI/Strategy/CombatMovementStrategy.cpp"
"src/modules/Playerbot/Interaction/FlightMasterManager.cpp"
"src/modules/Playerbot/Interaction/Core/GossipHandler.cpp"
"src/modules/Playerbot/AI/ClassAI/Hunters/HunterAI.cpp"
"src/modules/Playerbot/AI/ClassAI/Warlocks/WarlockAI.cpp"
"src/modules/Playerbot/Game/QuestManager.cpp"
"src/modules/Playerbot/Advanced/GroupCoordinator.cpp"
"src/modules/Playerbot/Economy/AuctionManager.cpp"
"src/modules/Playerbot/Game/NPCInteractionManager.cpp"
"src/modules/Playerbot/AI/Combat/TargetScanner.cpp"
"src/modules/Playerbot/Interaction/Core/InteractionManager.cpp"
"src/modules/Playerbot/Social/TradeManager.cpp"
"src/modules/Playerbot/Advanced/EconomyManager.cpp"
)

TOTAL_FIXED=0

for FILE in "${FILES[@]}"; do
    if [ ! -f "$FILE" ]; then
        echo "  [SKIP] Not found: $FILE"
        continue
    fi

    # Count changes before
    BEFORE=$(grep -c "return false;" "$FILE" 2>/dev/null || echo "0")
    BEFORE=$((BEFORE + $(grep -c "return true;" "$FILE" 2>/dev/null || echo "0")))
    BEFORE=$((BEFORE + $(grep -c "return nullptr;" "$FILE" 2>/dev/null || echo "0")))
    BEFORE=$((BEFORE + $(grep -c "return 0;" "$FILE" 2>/dev/null || echo "0")))
    BEFORE=$((BEFORE + $(grep -c "return {};" "$FILE" 2>/dev/null || echo "0")))

    # Apply fixes (only standalone return statements, not in expressions)
    sed -i 's/^\(\s*\)return false;$/\1return;/g' "$FILE"
    sed -i 's/^\(\s*\)return true;$/\1return;/g' "$FILE"
    sed -i 's/^\(\s*\)return nullptr;$/\1return;/g' "$FILE"
    sed -i 's/^\(\s*\)return 0;$/\1return;/g' "$FILE"
    sed -i 's/^\(\s*\)return {};$/\1return;/g' "$FILE"

    # Count changes after
    AFTER=$(grep -c "return false;" "$FILE" 2>/dev/null || echo "0")
    AFTER=$((AFTER + $(grep -c "return true;" "$FILE" 2>/dev/null || echo "0")))
    AFTER=$((AFTER + $(grep -c "return nullptr;" "$FILE" 2>/dev/null || echo "0")))
    AFTER=$((AFTER + $(grep -c "return 0;" "$FILE" 2>/dev/null || echo "0")))
    AFTER=$((AFTER + $(grep -c "return {};;" "$FILE" 2>/dev/null || echo "0")))

    CHANGES=$((BEFORE - AFTER))

    if [ $CHANGES -gt 0 ]; then
        echo "  [OK] $(basename $FILE) ($CHANGES fixes)"
        TOTAL_FIXED=$((TOTAL_FIXED + CHANGES))
    fi
done

echo ""
echo "================================================================================"
echo "TOTAL: $TOTAL_FIXED void return statements fixed in ${#FILES[@]} files"
echo "================================================================================"
