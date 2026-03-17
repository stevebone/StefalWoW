#!/bin/bash
# Fix missing GameTime.h includes for Phase 7A
# Fixes 540 C3861 errors: "GetGameTimeMS": Identifier not found

set -e

echo "=== Phase 7A: Adding GameTime.h includes ==="
echo ""

# List of files from error log that need GameTime.h include
FILES=(
"src/modules/Playerbot/AI/BehaviorTree/Nodes/CombatNodes.h"
"src/modules/Playerbot/AI/BehaviorTree/Nodes/HealingNodes.h"
"src/modules/Playerbot/AI/ClassAI/ActionPriority.h"
"src/modules/Playerbot/AI/ClassAI/BaselineRotationManager.cpp"
"src/modules/Playerbot/AI/ClassAI/ClassAI.h"
"src/modules/Playerbot/AI/ClassAI/Common/StatusEffectTracker.h"
"src/modules/Playerbot/AI/ClassAI/DeathKnights/DiseaseManager.cpp"
"src/modules/Playerbot/AI/ClassAI/DeathKnights/DiseaseManager.h"
"src/modules/Playerbot/AI/ClassAI/DemonHunters/DemonHunterAI.cpp"
"src/modules/Playerbot/AI/ClassAI/Druids/BalanceDruidRefactored.h"
"src/modules/Playerbot/AI/ClassAI/Evokers/EvokerAI.cpp"
"src/modules/Playerbot/AI/ClassAI/Evokers/EvokerAI.h"
"src/modules/Playerbot/AI/ClassAI/Hunters/BeastMasteryHunterRefactored.h"
"src/modules/Playerbot/AI/ClassAI/Hunters/HunterAI.h"
"src/modules/Playerbot/AI/ClassAI/Paladins/PaladinAI.cpp"
"src/modules/Playerbot/AI/ClassAI/Priests/PriestAI.cpp"
"src/modules/Playerbot/AI/ClassAI/ResourceManager.cpp"
"src/modules/Playerbot/AI/ClassAI/ResourceManager.h"
"src/modules/Playerbot/AI/ClassAI/ResourceTypes.h"
"src/modules/Playerbot/AI/ClassAI/Shamans/ShamanAI.cpp"
"src/modules/Playerbot/AI/Combat/CrowdControlManager.h"
"src/modules/Playerbot/AI/Combat/DefensiveManager.h"
"src/modules/Playerbot/AI/Combat/GroupCombatTrigger.cpp"
"src/modules/Playerbot/AI/Combat/InterruptCoordinator.cpp"
"src/modules/Playerbot/AI/Combat/MechanicAwareness.cpp"
"src/modules/Playerbot/AI/Combat/MovementIntegration.h"
"src/modules/Playerbot/AI/Coordination/RaidOrchestrator.cpp"
"src/modules/Playerbot/AI/Coordination/RoleCoordinator.cpp"
"src/modules/Playerbot/AI/Strategy/GroupCombatStrategy.cpp"
"src/modules/Playerbot/AI/Strategy/LootStrategy.cpp"
"src/modules/Playerbot/AI/Strategy/QuestStrategy.cpp"
"src/modules/Playerbot/AI/Strategy/RestStrategy.cpp"
"src/modules/Playerbot/Advanced/AdvancedBehaviorManager.cpp"
"src/modules/Playerbot/Advanced/SocialManager.cpp"
"src/modules/Playerbot/Character/BotCharacterDistribution.cpp"
"src/modules/Playerbot/Chat/BotChatCommandHandler.cpp"
"src/modules/Playerbot/Game/InventoryManager.cpp"
"src/modules/Playerbot/Game/NPCInteractionManager.cpp"
"src/modules/Playerbot/Game/QuestAcceptanceManager.cpp"
"src/modules/Playerbot/Group/RoleAssignment.h"
"src/modules/Playerbot/Movement/Arbiter/MovementArbiter.cpp"
"src/modules/Playerbot/Movement/Arbiter/MovementRequest.cpp"
"src/modules/Playerbot/Movement/LeaderFollowBehavior.cpp"
"src/modules/Playerbot/Movement/QuestPathfinder.cpp"
"src/modules/Playerbot/Professions/ProfessionAuctionBridge.cpp"
"src/modules/Playerbot/Professions/ProfessionManager.cpp"
"src/modules/Playerbot/Quest/DynamicQuestSystem.cpp"
"src/modules/Playerbot/Quest/DynamicQuestSystem.h"
"src/modules/Playerbot/Quest/ObjectiveTracker.cpp"
"src/modules/Playerbot/Quest/ObjectiveTracker.h"
"src/modules/Playerbot/Quest/QuestCompletion.h"
"src/modules/Playerbot/Quest/QuestPickup.cpp"
"src/modules/Playerbot/Quest/QuestPickup.h"
"src/modules/Playerbot/Quest/UnifiedQuestManager.cpp"
"src/modules/Playerbot/Session/BotPacketRelay.cpp"
"src/modules/Playerbot/Session/BotPacketSimulator.cpp"
"src/modules/Playerbot/Session/BotPriorityManager.cpp"
"src/modules/Playerbot/Session/BotWorldSessionMgr.cpp"
"src/modules/Playerbot/Social/AuctionHouse.h"
"src/modules/Playerbot/Social/GuildIntegration.cpp"
"src/modules/Playerbot/Social/GuildIntegration.h"
"src/modules/Playerbot/Social/LootAnalysis.h"
"src/modules/Playerbot/Social/LootCoordination.h"
"src/modules/Playerbot/Social/LootDistribution.cpp"
"src/modules/Playerbot/Social/LootDistribution.h"
"src/modules/Playerbot/Social/MarketAnalysis.cpp"
"src/modules/Playerbot/Social/MarketAnalysis.h"
"src/modules/Playerbot/Social/UnifiedLootManager.cpp"
"src/modules/Playerbot/Social/UnifiedLootManager.h"
)

FIXED=0
SKIPPED=0

for file in "${FILES[@]}"; do
    # Convert Windows path to Unix path if needed
    file=$(echo "$file" | sed 's|C:\\TrinityBots\\TrinityCore\\||' | sed 's|\\|/|g')

    if [ ! -f "$file" ]; then
        echo "SKIP: File not found: $file"
        ((SKIPPED++))
        continue
    fi

    # Check if file already has GameTime.h include
    if grep -q '#include.*GameTime\.h' "$file"; then
        echo "SKIP: $file (already has GameTime.h)"
        ((SKIPPED++))
        continue
    fi

    # Check if file actually uses GameTime::GetGameTimeMS
    if ! grep -q 'GameTime::GetGameTimeMS' "$file"; then
        echo "SKIP: $file (doesn't use GameTime::GetGameTimeMS)"
        ((SKIPPED++))
        continue
    fi

    # Add GameTime.h include after the last #include line
    # Use a temporary file to avoid issues with in-place editing
    awk '/^#include/ {last_include=NR} END {print last_include}' "$file" > /tmp/last_include_line
    LAST_INCLUDE=$(cat /tmp/last_include_line)

    if [ -n "$LAST_INCLUDE" ] && [ "$LAST_INCLUDE" -gt 0 ]; then
        # Insert #include "GameTime.h" after the last include
        sed -i "${LAST_INCLUDE}a\\#include \"GameTime.h\"" "$file"
        echo "FIXED: $file (added GameTime.h after line $LAST_INCLUDE)"
        ((FIXED++))
    else
        echo "SKIP: $file (no #include lines found)"
        ((SKIPPED++))
    fi
done

echo ""
echo "=== Summary ==="
echo "Files fixed: $FIXED"
echo "Files skipped: $SKIPPED"
echo "Total files processed: ${#FILES[@]}"
echo ""
echo "Expected error reduction: ~540 errors (C3861: GetGameTimeMS)"
