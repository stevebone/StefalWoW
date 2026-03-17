/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "HybridAIController.h"
#include "GameTime.h"
#include "AI/BotAI.h"
#include "Utility/UtilityContextBuilder.h"
#include "Utility/Evaluators/CombatEvaluators.h"
#include "ClassAI/ClassBehaviorTreeRegistry.h"
#include "Player.h"
#include "Log.h"

namespace Playerbot
{

HybridAIController::HybridAIController(BotAI* ai, SharedBlackboard* blackboard)
    : _bot(ai), _blackboard(blackboard),
      _utilityAI(::std::make_unique<UtilityAI>()),
      _currentTree(nullptr),
      _currentBehaviorName("None"),
      _lastTreeStatus(BTStatus::INVALID),
      _lastDecisionTime(0),
      _decisionUpdateInterval(500), // 500ms between decisions
      _lastBehaviorChangeTime(0),
      _behaviorChangedThisFrame(false),
      _totalDecisions(0),
      _totalTreeExecutions(0),
      _successfulExecutions(0),
      _failedExecutions(0)
{
}

void HybridAIController::Initialize()
{
    TC_LOG_DEBUG("playerbot.ai", "HybridAIController::Initialize() - Initializing Utility AI and Behavior Tree mappings");

    // Create default utility behaviors
    // Combat behavior
    auto combatBehavior = ::std::make_shared<UtilityBehavior>("Combat");
    combatBehavior->AddEvaluator(::std::make_shared<CombatEngageEvaluator>());
    _utilityAI->AddBehavior(combatBehavior);

    // Healing behavior
    auto healingBehavior = ::std::make_shared<UtilityBehavior>("Healing");
    healingBehavior->AddEvaluator(::std::make_shared<HealAllyEvaluator>());
    _utilityAI->AddBehavior(healingBehavior);

    // Tanking behavior
    auto tankingBehavior = ::std::make_shared<UtilityBehavior>("Tanking");
    tankingBehavior->AddEvaluator(::std::make_shared<TankThreatEvaluator>());
    _utilityAI->AddBehavior(tankingBehavior);

    // Flee behavior
    auto fleeBehavior = ::std::make_shared<UtilityBehavior>("Flee");
    fleeBehavior->AddEvaluator(::std::make_shared<FleeEvaluator>());
    _utilityAI->AddBehavior(fleeBehavior);

    // Mana regeneration behavior
    auto manaRegenBehavior = ::std::make_shared<UtilityBehavior>("ManaRegen");
    manaRegenBehavior->AddEvaluator(::std::make_shared<ManaRegenerationEvaluator>());
    _utilityAI->AddBehavior(manaRegenBehavior);

    // Dispel behavior
    auto dispelBehavior = ::std::make_shared<UtilityBehavior>("Dispel");
    dispelBehavior->AddEvaluator(::std::make_shared<DispelEvaluator>());
    _utilityAI->AddBehavior(dispelBehavior);

    // AoE damage behavior
    auto aoeBehavior = ::std::make_shared<UtilityBehavior>("AoEDamage");
    aoeBehavior->AddEvaluator(::std::make_shared<AoEDamageEvaluator>());
    _utilityAI->AddBehavior(aoeBehavior);

    // Create default behavior-to-tree mappings
    CreateDefaultBehaviorMappings();

    // Phase 5: Register class-specific behavior tree from ClassBehaviorTreeRegistry
    if (_bot && _bot->GetBot())
    {
        Player* player = _bot->GetBot();
        uint8 classId = player->GetClass();
        uint8 spec = uint8(player->GetPrimarySpecialization());

        // Get class-specific tree from registry
        ::std::shared_ptr<BTNode> classTree = ClassBehaviorTreeRegistry::GetTree(
            static_cast<WowClass>(classId), spec);

        if (classTree)
        {
            // Register as custom "class_rotation" behavior
            _customTreeBuilders["class_rotation"] = [classTree]() -> ::std::shared_ptr<BTNode> {
                return classTree;
            };

            // Also map Combat behavior to use class tree instead of generic melee combat
            _customTreeBuilders["Combat"] = [classTree]() -> ::std::shared_ptr<BTNode> {
                return classTree;
            };

            TC_LOG_INFO("playerbot.ai", "HybridAIController: Registered class-specific tree for {} (Class: {}, Spec: {})",
                player->GetName(), uint32(classId), uint32(spec));
        }
        else
        {
            TC_LOG_WARN("playerbot.ai", "HybridAIController: No class tree found for Class {} Spec {}, using default trees",
                uint32(classId), uint32(spec));
        }
    }

    TC_LOG_INFO("playerbot.ai", "HybridAIController initialized: {} behaviors, {} mappings, {} custom builders",
        _utilityAI->GetBehaviors().size(), _behaviorToTreeMap.size(), _customTreeBuilders.size());
}

void HybridAIController::CreateDefaultBehaviorMappings()
{
    // Combat behaviors
    _behaviorToTreeMap["Combat"] = BehaviorTreeFactory::TreeType::MELEE_COMBAT;
    _behaviorToTreeMap["Tanking"] = BehaviorTreeFactory::TreeType::TANK_COMBAT;
    _behaviorToTreeMap["AoEDamage"] = BehaviorTreeFactory::TreeType::MELEE_COMBAT; // Use melee combat tree

    // Healing behaviors
    _behaviorToTreeMap["Healing"] = BehaviorTreeFactory::TreeType::SINGLE_TARGET_HEALING;
    _behaviorToTreeMap["Dispel"] = BehaviorTreeFactory::TreeType::DISPEL_PRIORITY;

    // Movement behaviors
    _behaviorToTreeMap["Flee"] = BehaviorTreeFactory::TreeType::FLEE_TO_SAFETY;

    // Utility behaviors
    _behaviorToTreeMap["ManaRegen"] = BehaviorTreeFactory::TreeType::RESOURCE_MANAGEMENT;

    TC_LOG_DEBUG("playerbot.ai", "Created {} default behavior-to-tree mappings", _behaviorToTreeMap.size());
}

bool HybridAIController::Update(uint32 diff)
{
    if (!_bot)
    {
        TC_LOG_ERROR("playerbot.ai", "HybridAIController::Update() - Bot AI is null");
        return false;
    }

    _behaviorChangedThisFrame = false;

    // Update decision timer
    _lastDecisionTime += diff;

    // Only make decisions every N milliseconds (throttling)
    if (_lastDecisionTime < _decisionUpdateInterval)
    {
        // Execute current tree if we have one
    if (_currentTree)
        {
            ExecuteCurrentTree();
            return true;
        }
        return false;
    }

    // Reset decision timer
    _lastDecisionTime = 0;
    _totalDecisions++;

    // Build utility context
    UtilityContext context = UtilityContextBuilder::Build(_bot, _blackboard);

    // Select best behavior
    UtilityBehavior* selectedBehavior = SelectBehavior(context);

    if (!selectedBehavior)
    {
        TC_LOG_TRACE("playerbot.ai", "HybridAIController::Update() - No behavior selected");
        return false;
    }

    ::std::string selectedBehaviorName = selectedBehavior->GetName();

    // Check if behavior changed
    if (selectedBehaviorName != _currentBehaviorName)
    {
        TC_LOG_DEBUG("playerbot.ai", "Bot {} behavior transition: {} -> {} (utility score: {:.2f})",
            _bot->GetBot()->GetName(), _currentBehaviorName, selectedBehaviorName,
            selectedBehavior->GetCachedScore());

        // Get tree for new behavior
        ::std::shared_ptr<BTNode> newTree = GetTreeForBehavior(selectedBehaviorName);

        // Switch to new behavior tree
        SwitchBehaviorTree(selectedBehaviorName, newTree);
        _behaviorChangedThisFrame = true;
    }

    // Execute current tree
    BTStatus status = ExecuteCurrentTree();

    return status != BTStatus::INVALID;
}

UtilityBehavior* HybridAIController::SelectBehavior(UtilityContext const& context)
{
    if (!_utilityAI)
    {
        TC_LOG_ERROR("playerbot.ai", "HybridAIController::SelectBehavior() - Utility AI is null");
        return nullptr;
    }

    return _utilityAI->SelectBehavior(context);
}

::std::shared_ptr<BTNode> HybridAIController::GetTreeForBehavior(::std::string const& behaviorName)
{
    // Check custom tree builders first
    auto customIt = _customTreeBuilders.find(behaviorName);
    if (customIt != _customTreeBuilders.end())
    {
        TC_LOG_TRACE("playerbot.ai", "Building custom tree for behavior: {}", behaviorName);
        return customIt->second();
    }

    // Check standard mappings
    auto it = _behaviorToTreeMap.find(behaviorName);
    if (it != _behaviorToTreeMap.end())
    {
        TC_LOG_TRACE("playerbot.ai", "Creating factory tree for behavior: {}", behaviorName);
        return BehaviorTreeFactory::CreateTree(it->second);
    }

    TC_LOG_WARN("playerbot.ai", "No tree mapping found for behavior: {}", behaviorName);
    return nullptr;
}

void HybridAIController::SwitchBehaviorTree(::std::string const& behaviorName, ::std::shared_ptr<BTNode> tree)
{
    // Reset old tree if exists
    if (_currentTree)
    {
        _currentTree->Reset();
    }

    // Create new tree container
    _currentTree = ::std::make_unique<BehaviorTree>();
    _currentTree->SetRoot(tree);

    // Update tracking
    _currentBehaviorName = behaviorName;
    _lastBehaviorChangeTime = GameTime::GetGameTimeMS();
    _lastTreeStatus = BTStatus::INVALID;

    TC_LOG_DEBUG("playerbot.ai", "Switched to behavior tree: {}", behaviorName);
}

BTStatus HybridAIController::ExecuteCurrentTree()
{
    if (!_currentTree)
    {
        return BTStatus::INVALID;
    }

    _totalTreeExecutions++;

    BTStatus status = _currentTree->Tick(_bot);
    _lastTreeStatus = status;

    // Track success/failure
    if (status == BTStatus::SUCCESS)
    {
        _successfulExecutions++;
    }
    else if (status == BTStatus::FAILURE)
    {
        _failedExecutions++;
    }

    // Log if status changed
    static BTStatus lastLoggedStatus = BTStatus::INVALID;
    if (status != lastLoggedStatus && status != BTStatus::RUNNING)
    {
        TC_LOG_TRACE("playerbot.ai", "Bot {} tree '{}' status: {}",
            _bot->GetBot()->GetName(), _currentBehaviorName,
            status == BTStatus::SUCCESS ? "SUCCESS" : "FAILURE");
        lastLoggedStatus = status;
    }

    return status;
}

void HybridAIController::Reset()
{
    TC_LOG_DEBUG("playerbot.ai", "HybridAIController::Reset()");

    // Reset current tree
    if (_currentTree)
    {
        _currentTree->Reset();
        _currentTree.reset();
    }

    // Reset utility AI
    if (_utilityAI)
    {
        // Note: UtilityAI doesn't have a Reset() method, so we just clear behaviors
        // and reinitialize
    }

    // Clear state
    _currentBehaviorName = "None";
    _lastTreeStatus = BTStatus::INVALID;
    _lastDecisionTime = 0;
    _lastBehaviorChangeTime = 0;
    _behaviorChangedThisFrame = false;

    // Clear performance tracking
    _totalDecisions = 0;
    _totalTreeExecutions = 0;
    _successfulExecutions = 0;
    _failedExecutions = 0;
}

::std::string HybridAIController::GetCurrentBehaviorName() const
{
    return _currentBehaviorName;
}

BTStatus HybridAIController::GetCurrentTreeStatus() const
{
    return _lastTreeStatus;
}

void HybridAIController::RegisterBehaviorMapping(::std::string const& behaviorName, BehaviorTreeFactory::TreeType treeType)
{
    _behaviorToTreeMap[behaviorName] = treeType;

    TC_LOG_INFO("playerbot.ai", "Registered behavior mapping: {} -> TreeType({})",
        behaviorName, uint32(treeType));
}

void HybridAIController::RegisterCustomBehaviorMapping(::std::string const& behaviorName,
    ::std::function<::std::shared_ptr<BTNode>()> treeBuilder)
{
    _customTreeBuilders[behaviorName] = treeBuilder;

    TC_LOG_INFO("playerbot.ai", "Registered custom behavior mapping: {}", behaviorName);
}

uint32 HybridAIController::GetTimeSinceLastBehaviorChange() const
{
    if (_lastBehaviorChangeTime == 0)
        return 0;

    return getMSTimeDiff(_lastBehaviorChangeTime, GameTime::GetGameTimeMS());
}

} // namespace Playerbot
