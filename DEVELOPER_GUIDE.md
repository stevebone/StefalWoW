# TrinityCore PlayerBot Developer Guide

**Version**: 1.0  
**Last Updated**: 2025-11-18  
**Target**: TrinityCore 3.3.5a (WotLK)

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Quest System](#quest-system)
3. [Profession Systems](#profession-systems)
4. [Economy Systems](#economy-systems)
5. [Combat Systems](#combat-systems)
6. [Movement System](#movement-system)
7. [Lifecycle Systems](#lifecycle-systems)
8. [Group Systems](#group-systems)
9. [Decision Systems](#decision-systems)
10. [Core Infrastructure](#core-infrastructure)
11. [Integration Guide](#integration-guide)
12. [Best Practices](#best-practices)

---

## Architecture Overview

### GameSystemsManager Facade

The **GameSystemsManager** is a facade that consolidates all 21 bot manager instances, providing centralized ownership and lifecycle management. This architecture follows the **Facade Pattern** and **Dependency Injection** principles.

```cpp
// Access pattern
BotAI* botAI = ...; // Your bot AI instance
auto* gameSystems = botAI->GetGameSystems();

// Use any system
gameSystems->GetQuestManager()->AcceptQuest(questId);
gameSystems->GetProfessionManager()->CraftItem(recipeId);
gameSystems->GetCombatStateManager()->EnterCombat(target);
```

### Manager Categories

| Category | Managers | Purpose |
|----------|----------|---------|
| **Quest** | 1 | Quest tracking, acceptance, completion |
| **Profession** | 8 | Gathering, crafting, material optimization |
| **Economy** | 2 | Trading, auction house operations |
| **Combat** | 2 | Combat state management, target selection |
| **Movement** | 1 | Pathfinding, movement coordination |
| **Lifecycle** | 1 | Death recovery, corpse retrieval |
| **Group** | 2 | Group formation, invitation handling |
| **Decision** | 4 | AI decision-making, behavior trees |
| **Infrastructure** | 3 | Events, registry, priority management |

**Total**: 21 managers

---

## Quest System

### QuestManager

**Purpose**: Manages quest tracking, acceptance, turn-ins, and objective completion for bots.

**Location**: `src/modules/Playerbot/Game/QuestManager.h`

#### Key Responsibilities

- Quest discovery and evaluation
- Quest acceptance and turn-in automation
- Objective tracking and completion
- Reward selection optimization
- Quest chain navigation

#### Core Methods

##### Quest Discovery

```cpp
/**
 * @brief Find available quests in the world
 * @param searchRadius Radius in yards to search for quest givers
 * @return Vector of available quest IDs
 */
std::vector<uint32> FindAvailableQuests(float searchRadius = 100.0f);

/**
 * @brief Check if a quest is available for the bot
 * @param questId Quest entry ID
 * @return true if quest can be accepted
 */
bool IsQuestAvailable(uint32 questId) const;
```

##### Quest Management

```cpp
/**
 * @brief Accept a quest from a quest giver
 * @param questId Quest entry ID
 * @param questGiverGuid GUID of the NPC or GameObject offering the quest
 * @return true if quest was accepted successfully
 */
bool AcceptQuest(uint32 questId, ObjectGuid questGiverGuid);

/**
 * @brief Turn in a completed quest
 * @param questId Quest entry ID
 * @param rewardChoice Index of reward to select (0-based)
 * @return true if quest was turned in successfully
 */
bool TurnInQuest(uint32 questId, uint8 rewardChoice = 0);

/**
 * @brief Abandon an active quest
 * @param questId Quest entry ID
 * @return true if quest was abandoned
 */
bool AbandonQuest(uint32 questId);
```

##### Quest Tracking

```cpp
/**
 * @brief Get all active quests for the bot
 * @return Vector of active quest IDs
 */
std::vector<uint32> GetActiveQuests() const;

/**
 * @brief Check if a quest is complete and ready for turn-in
 * @param questId Quest entry ID
 * @return true if all objectives are complete
 */
bool IsQuestComplete(uint32 questId) const;

/**
 * @brief Get quest objective status
 * @param questId Quest entry ID
 * @return Map of objective index to completion status
 */
std::map<uint8, QuestObjectiveStatus> GetQuestObjectives(uint32 questId) const;
```

##### Reward Optimization

```cpp
/**
 * @brief Select best reward from quest options
 * @param questId Quest entry ID
 * @return Index of optimal reward (considers item value, usability, class)
 */
uint8 SelectBestReward(uint32 questId) const;
```

#### Usage Example

```cpp
// Quest workflow example
auto* questMgr = gameSystems->GetQuestManager();

// 1. Find available quests
auto availableQuests = questMgr->FindAvailableQuests();

// 2. Accept highest priority quest
for (uint32 questId : availableQuests)
{
    if (questMgr->IsQuestAvailable(questId))
    {
        Creature* questGiver = FindNearestQuestGiver(questId);
        if (questGiver)
        {
            questMgr->AcceptQuest(questId, questGiver->GetGUID());
            break;
        }
    }
}

// 3. Track active quests
auto activeQuests = questMgr->GetActiveQuests();
for (uint32 questId : activeQuests)
{
    if (questMgr->IsQuestComplete(questId))
    {
        // Quest is complete, find turn-in NPC
        uint8 rewardChoice = questMgr->SelectBestReward(questId);
        questMgr->TurnInQuest(questId, rewardChoice);
    }
}
```

---

## Profession Systems

The profession system consists of **8 managers** working together to provide automated profession management, material optimization, and economic decision-making.

### System Architecture

```
ProfessionManager (Core)
    ↓
GatheringManager ←→ GatheringMaterialsBridge
    ↓                       ↓
    └─────> AuctionMaterialsBridge ←─────┘
                    ↓
            ProfessionAuctionBridge
                    ↓
            AuctionHouse (TrinityCore)
```

### ProfessionManager

**Purpose**: Core profession management - skill tracking, recipe learning, crafting automation.

**Location**: `src/modules/Playerbot/Professions/ProfessionManager.h`

**Architecture**: Per-bot instance (Phase 1B refactoring)

#### Key Responsibilities

- Profession skill tracking (current/max levels)
- Recipe database management
- Crafting automation
- Profession leveling optimization
- Material requirement calculations

#### Core Methods

##### Profession Information

```cpp
/**
 * @brief Get all professions for the bot
 * @return Vector of ProfessionInfo structures
 */
std::vector<ProfessionInfo> GetPlayerProfessions() const;

/**
 * @brief Get profession skill level
 * @param profession Profession type
 * @return Current skill level (0 if not learned)
 */
uint32 GetProfessionSkill(ProfessionType profession) const;

/**
 * @brief Check if bot has a specific profession
 * @param profession Profession type
 * @return true if profession is learned
 */
bool HasProfession(ProfessionType profession) const;
```

##### Recipe Management

```cpp
/**
 * @brief Get all recipes for a profession
 * @param profession Profession type
 * @return Vector of RecipeInfo structures
 */
std::vector<RecipeInfo> GetRecipesForProfession(ProfessionType profession) const;

/**
 * @brief Check if bot knows a recipe
 * @param recipeId Recipe spell ID
 * @return true if recipe is learned
 */
bool KnowsRecipe(uint32 recipeId) const;

/**
 * @brief Learn a new recipe
 * @param recipeId Recipe spell ID
 * @return true if recipe was learned
 */
bool LearnRecipe(uint32 recipeId);
```

##### Crafting

```cpp
/**
 * @brief Craft an item from a recipe
 * @param recipeId Recipe spell ID
 * @param count Number of items to craft
 * @return true if crafting was initiated
 */
bool CraftItem(uint32 recipeId, uint32 count = 1);

/**
 * @brief Check if bot has materials for a recipe
 * @param recipeId Recipe spell ID
 * @param count Number of items to craft
 * @return true if all materials are available
 */
bool HasMaterialsForRecipe(uint32 recipeId, uint32 count = 1) const;

/**
 * @brief Get missing materials for a recipe
 * @param recipe RecipeInfo reference
 * @return Vector of pairs (itemId, quantity needed)
 */
std::vector<std::pair<uint32, uint32>> GetMissingMaterials(RecipeInfo const& recipe) const;
```

##### Profession Leveling

```cpp
/**
 * @brief Get optimal recipe for leveling a profession
 * @param profession Profession type
 * @return Pointer to RecipeInfo, nullptr if none available
 */
RecipeInfo const* GetOptimalLevelingRecipe(ProfessionType profession) const;

/**
 * @brief Calculate skill-up chance for a recipe
 * @param recipeId Recipe spell ID
 * @return Probability of skill gain (0.0 - 1.0)
 */
float GetSkillUpChance(uint32 recipeId) const;
```

#### ProfessionInfo Structure

```cpp
struct ProfessionInfo
{
    ProfessionType profession;   // Profession enum
    uint32 currentSkill;         // Current skill level
    uint32 maxSkill;             // Maximum skill level for expansion
    uint32 professionId;         // WoW profession ID
    std::string name;            // Profession name
};
```

#### RecipeInfo Structure

```cpp
struct RecipeInfo
{
    uint32 recipeId;             // Recipe spell ID
    uint32 productItemId;        // Crafted item ID
    uint32 productCount;         // Items created per craft
    ProfessionType profession;   // Required profession
    uint32 requiredSkill;        // Minimum skill level
    uint32 yellowSkill;          // Yellow skill threshold
    uint32 greenSkill;           // Green skill threshold
    uint32 greySkill;            // Grey skill threshold
    
    struct Reagent
    {
        uint32 itemId;           // Material item ID
        uint32 quantity;         // Quantity required
    };
    std::vector<Reagent> reagents; // Required materials
};
```

#### Usage Example

```cpp
auto* profMgr = gameSystems->GetProfessionManager();

// Check professions
auto professions = profMgr->GetPlayerProfessions();
for (auto const& prof : professions)
{
    TC_LOG_INFO("Profession: {} - Skill: {}/{}", 
        prof.name, prof.currentSkill, prof.maxSkill);
}

// Craft items
if (profMgr->HasProfession(ProfessionType::BLACKSMITHING))
{
    uint32 recipeId = 12345; // Iron Sword recipe
    
    if (profMgr->KnowsRecipe(recipeId))
    {
        if (profMgr->HasMaterialsForRecipe(recipeId, 5))
        {
            profMgr->CraftItem(recipeId, 5); // Craft 5 swords
        }
        else
        {
            // Get missing materials
            auto recipe = profMgr->GetRecipeInfo(recipeId);
            auto missing = profMgr->GetMissingMaterials(*recipe);
            // Purchase or gather missing materials
        }
    }
}

// Level profession efficiently
if (profMgr->GetProfessionSkill(ProfessionType::ALCHEMY) < 300)
{
    auto* recipe = profMgr->GetOptimalLevelingRecipe(ProfessionType::ALCHEMY);
    if (recipe)
    {
        float chance = profMgr->GetSkillUpChance(recipe->recipeId);
        TC_LOG_INFO("Crafting {} ({}% skill-up chance)", 
            recipe->productItemId, chance * 100.0f);
        profMgr->CraftItem(recipe->recipeId);
    }
}
```

---

### GatheringManager

**Purpose**: Automates resource gathering (Mining, Herbalism, Skinning).

**Location**: `src/modules/Playerbot/Professions/GatheringManager.h`

#### Key Responsibilities

- Node detection and tracking
- Gathering automation
- Skill-up optimization
- Route planning for gathering
- Material stockpile management

#### Core Methods

```cpp
/**
 * @brief Find gathering nodes in range
 * @param gatheringType Type of gathering (Mining/Herbalism/Skinning)
 * @param searchRadius Search radius in yards
 * @return Vector of node GUIDs
 */
std::vector<ObjectGuid> FindGatheringNodes(GatheringType type, float radius = 100.0f);

/**
 * @brief Gather from a node
 * @param nodeGuid GameObject GUID of the node
 * @return true if gathering was successful
 */
bool GatherFrom(ObjectGuid nodeGuid);

/**
 * @brief Check if bot can gather from a node
 * @param nodeGuid GameObject GUID of the node
 * @return true if skill level is sufficient and node is reachable
 */
bool CanGather(ObjectGuid nodeGuid) const;

/**
 * @brief Get optimal gathering route
 * @param gatheringType Type of gathering
 * @param maxNodes Maximum nodes in route
 * @return Ordered vector of node positions
 */
std::vector<Position> GetGatheringRoute(GatheringType type, uint32 maxNodes = 10);
```

#### Usage Example

```cpp
auto* gatherMgr = gameSystems->GetGatheringManager();

// Find and gather herbs
if (profMgr->HasProfession(ProfessionType::HERBALISM))
{
    auto nodes = gatherMgr->FindGatheringNodes(GatheringType::HERBALISM, 50.0f);
    for (auto nodeGuid : nodes)
    {
        if (gatherMgr->CanGather(nodeGuid))
        {
            // Move to node and gather
            gatherMgr->GatherFrom(nodeGuid);
            break;
        }
    }
}
```

---

### GatheringMaterialsBridge

**Purpose**: Coordinates gathering priorities with crafting material needs.

**Location**: `src/modules/Playerbot/Professions/GatheringMaterialsBridge.h`

**Architecture**: Per-bot instance (Phase 4.1 refactoring)

#### Key Responsibilities

- Prioritize gathering targets based on crafting needs
- Track material inventory vs requirements
- Optimize gathering vs crafting workflow
- Event-driven material demand handling

#### Core Methods

```cpp
/**
 * @brief Prioritize a gathering target based on material needs
 * @param itemId Material item ID
 * @return Priority score (higher = more important)
 */
float PrioritizeGatheringTarget(uint32 itemId);

/**
 * @brief Get priority gathering targets
 * @param maxTargets Maximum number of targets to return
 * @return Ordered vector of material item IDs (highest priority first)
 */
std::vector<uint32> GetPriorityMaterials(uint32 maxTargets = 5);

/**
 * @brief Check if material is needed for active recipes
 * @param itemId Material item ID
 * @param quantity Quantity to check
 * @return true if material is needed
 */
bool IsMaterialNeeded(uint32 itemId, uint32 quantity) const;

/**
 * @brief Get crafting recipes that need a specific material
 * @param itemId Material item ID
 * @return Vector of recipe IDs requiring this material
 */
std::vector<uint32> GetRecipesRequiringMaterial(uint32 itemId) const;
```

#### Event Handling

The bridge subscribes to `ProfessionEventBus` events:

- `MATERIALS_NEEDED` - Triggered when crafting requires materials
- `GATHERING_COMPLETED` - Triggered when gathering finishes

```cpp
// Events are automatically filtered per-bot
// Only events for THIS bot are processed
```

#### Usage Example

```cpp
auto* bridge = gameSystems->GetGatheringMaterialsBridge();

// Check what materials are needed
auto priorities = bridge->GetPriorityMaterials(5);
for (uint32 itemId : priorities)
{
    float priority = bridge->PrioritizeGatheringTarget(itemId);
    TC_LOG_INFO("Need material: {} (priority: {})", itemId, priority);
    
    // Check recipes needing this material
    auto recipes = bridge->GetRecipesRequiringMaterial(itemId);
    TC_LOG_INFO("  Used in {} recipes", recipes.size());
}
```

---

### AuctionMaterialsBridge

**Purpose**: Economic analysis for buy vs gather decisions.

**Location**: `src/modules/Playerbot/Professions/AuctionMaterialsBridge.h`

**Architecture**: Per-bot instance (Phase 4.2 refactoring)

#### Key Responsibilities

- Time-value analysis (gathering time vs buying cost)
- Opportunity cost calculations
- Market price tracking
- Economic strategy execution (cost-optimized, time-optimized, hybrid)

#### Core Methods

##### Economic Analysis

```cpp
/**
 * @brief Determine best source for a material (gather vs buy)
 * @param itemId Material item ID
 * @param quantity Quantity needed
 * @return MaterialSourcingDecision with recommendation
 */
MaterialSourcingDecision GetBestMaterialSource(uint32 itemId, uint32 quantity);

/**
 * @brief Check if buying is cheaper than gathering
 * @param itemId Material item ID
 * @param quantity Quantity needed
 * @return true if buying is more cost-effective
 */
bool IsBuyingCheaperThanGathering(uint32 itemId, uint32 quantity) const;

/**
 * @brief Calculate time cost for gathering materials
 * @param itemId Material item ID
 * @param quantity Quantity needed
 * @return Estimated time in seconds
 */
uint32 CalculateGatheringTimeCost(uint32 itemId, uint32 quantity) const;

/**
 * @brief Calculate gold cost for buying materials
 * @param itemId Material item ID
 * @param quantity Quantity needed
 * @return Estimated cost in copper
 */
uint32 CalculateBuyingCost(uint32 itemId, uint32 quantity) const;
```

##### Strategy Management

```cpp
/**
 * @brief Set economic strategy for the bot
 * @param strategy Strategy to use (COST_OPTIMIZED, TIME_OPTIMIZED, HYBRID, BALANCED)
 */
void SetStrategy(MaterialSourcingStrategy strategy);

/**
 * @brief Get current economic strategy
 * @return Current MaterialSourcingStrategy
 */
MaterialSourcingStrategy GetStrategy() const;

/**
 * @brief Set bot's gold per hour earning rate
 * @param goldPerHour Gold earned per hour (for time-value calculations)
 */
void SetBotGoldPerHour(uint32 goldPerHour);
```

#### MaterialSourcingDecision Structure

```cpp
struct MaterialSourcingDecision
{
    enum class Source
    {
        GATHER,              // Gathering is optimal
        BUY,                 // Buying is optimal
        CRAFT,               // Crafting is optimal
        VENDOR,              // Available from vendor
        UNAVAILABLE          // Not available
    };
    
    Source recommendedSource;
    uint32 estimatedCost;    // Cost in copper
    uint32 estimatedTime;    // Time in seconds
    float confidence;        // Confidence level (0.0 - 1.0)
    std::string reasoning;   // Human-readable explanation
};
```

#### Usage Example

```cpp
auto* bridge = gameSystems->GetAuctionMaterialsBridge();

// Set economic strategy
bridge->SetStrategy(MaterialSourcingStrategy::HYBRID);
bridge->SetBotGoldPerHour(100 * 10000); // 100g per hour

// Analyze material sourcing
uint32 itemId = 2447; // Peacebloom
uint32 quantity = 20;

auto decision = bridge->GetBestMaterialSource(itemId, quantity);

switch (decision.recommendedSource)
{
    case MaterialSourcingDecision::Source::GATHER:
        TC_LOG_INFO("Recommendation: Gather {} x{} (Time: {}s, Cost: {}c)",
            itemId, quantity, decision.estimatedTime, decision.estimatedCost);
        // Initiate gathering
        break;
        
    case MaterialSourcingDecision::Source::BUY:
        TC_LOG_INFO("Recommendation: Buy {} x{} from AH (Cost: {}c, Time: {}s)",
            itemId, quantity, decision.estimatedCost, decision.estimatedTime);
        // Purchase from auction house
        break;
        
    case MaterialSourcingDecision::Source::VENDOR:
        TC_LOG_INFO("Recommendation: Buy {} x{} from vendor (Cost: {}c)",
            itemId, quantity, decision.estimatedCost);
        // Buy from vendor
        break;
}

TC_LOG_INFO("Reasoning: {}", decision.reasoning);
TC_LOG_INFO("Confidence: {}%", decision.confidence * 100.0f);
```

---

### ProfessionAuctionBridge

**Purpose**: Automates auction house operations for professions (selling/buying).

**Location**: `src/modules/Playerbot/Professions/ProfessionAuctionBridge.h`

**Architecture**: Per-bot instance (Phase 4.3 refactoring)

#### Key Responsibilities

- Automatic material selling (excess stockpile)
- Automatic crafted item selling (profit margin analysis)
- Automatic material purchasing (for leveling)
- Stockpile management
- Market price tracking

#### Core Methods

##### Material Selling

```cpp
/**
 * @brief Sell excess materials on auction house
 * Automatically lists materials exceeding stockpile limits
 */
void SellExcessMaterials();

/**
 * @brief Check if material should be sold
 * @param itemId Material item ID
 * @param currentCount Current inventory count
 * @return true if count exceeds stockpile maximum
 */
bool ShouldSellMaterial(uint32 itemId, uint32 currentCount) const;

/**
 * @brief List material on auction house
 * @param itemGuid Item GUID to list
 * @param config Stockpile configuration for the material
 * @return true if listing was successful
 */
bool ListMaterialOnAuction(uint32 itemGuid, MaterialStockpileConfig const& config);

/**
 * @brief Get optimal listing price for material
 * @param itemId Material item ID
 * @param stackSize Stack size to sell
 * @return Suggested listing price in copper
 */
uint32 GetOptimalMaterialPrice(uint32 itemId, uint32 stackSize) const;
```

##### Crafted Item Selling

```cpp
/**
 * @brief Sell crafted items on auction house
 * Automatically lists profitable crafted items
 */
void SellCraftedItems();

/**
 * @brief Check if crafted item should be sold
 * @param itemId Crafted item ID
 * @param materialCost Cost of materials used
 * @return true if profit margin meets minimum threshold
 */
bool ShouldSellCraftedItem(uint32 itemId, uint32 materialCost) const;

/**
 * @brief Calculate profit margin for crafted item
 * @param itemId Crafted item ID
 * @param marketPrice Current market price
 * @param materialCost Cost of materials
 * @return Profit margin as percentage (0.0 - 1.0+)
 */
float CalculateProfitMargin(uint32 itemId, uint32 marketPrice, uint32 materialCost) const;
```

##### Material Purchasing

```cpp
/**
 * @brief Buy materials for profession leveling
 * @param profession Profession to level
 */
void BuyMaterialsForLeveling(ProfessionType profession);

/**
 * @brief Purchase a specific material from auction house
 * @param itemId Material item ID
 * @param quantity Quantity to purchase
 * @param maxPricePerUnit Maximum price willing to pay per unit
 * @return true if purchase was successful
 */
bool PurchaseMaterial(uint32 itemId, uint32 quantity, uint32 maxPricePerUnit);

/**
 * @brief Check if material is available at acceptable price
 * @param itemId Material item ID
 * @param quantity Quantity needed
 * @param maxPricePerUnit Maximum acceptable price
 * @return true if material is available within budget
 */
bool IsMaterialAvailableForPurchase(uint32 itemId, uint32 quantity, uint32 maxPricePerUnit) const;
```

##### Stockpile Management

```cpp
/**
 * @brief Set stockpile configuration for a material
 * @param itemId Material item ID
 * @param config Stockpile configuration (min/max quantities, auction settings)
 */
void SetMaterialStockpile(uint32 itemId, MaterialStockpileConfig const& config);

/**
 * @brief Get current stockpile amount
 * @param itemId Material item ID
 * @return Current count in inventory
 */
uint32 GetCurrentStockpile(uint32 itemId) const;

/**
 * @brief Check if stockpile target is met
 * @param itemId Material item ID
 * @return true if current count >= minimum stockpile
 */
bool IsStockpileTargetMet(uint32 itemId) const;
```

##### Statistics

```cpp
/**
 * @brief Get auction statistics for this bot
 * @return ProfessionAuctionStatistics structure
 */
ProfessionAuctionStatistics const& GetStatistics() const;

/**
 * @brief Get global auction statistics (all bots)
 * @return Global ProfessionAuctionStatistics
 */
static ProfessionAuctionStatistics const& GetGlobalStatistics();

/**
 * @brief Reset auction statistics for this bot
 */
void ResetStatistics();
```

#### Configuration Structures

```cpp
struct MaterialStockpileConfig
{
    uint32 itemId;              // Material item ID
    uint32 minStackSize;        // Minimum stockpile to maintain
    uint32 maxStackSize;        // Maximum before selling excess
    uint32 auctionStackSize;    // Stack size to sell on AH
    bool preferBuyout;          // Use buyout price
};

struct CraftedItemAuctionConfig
{
    uint32 itemId;              // Crafted item ID
    uint32 minProfitMargin;     // Minimum profit in copper
    bool undercutCompetition;   // Undercut existing auctions
    float undercutRate;         // Undercut percentage (0.01 = 1%)
    uint32 maxListingDuration;  // Auction duration in hours
};

struct ProfessionAuctionProfile
{
    bool autoSellEnabled;       // Enable automatic selling
    bool buyMaterialsForLeveling; // Enable automatic buying
    uint32 auctionBudget;       // Gold budget for purchases
    ProfessionAuctionStrategy strategy; // AGGRESSIVE, CONSERVATIVE, BALANCED
    
    std::unordered_map<uint32, MaterialStockpileConfig> materialConfigs;
    std::unordered_map<uint32, CraftedItemAuctionConfig> craftedItemConfigs;
};
```

#### Usage Example

```cpp
auto* bridge = gameSystems->GetProfessionAuctionBridge();

// Configure stockpile for Copper Ore
MaterialStockpileConfig copperConfig;
copperConfig.itemId = 2770;
copperConfig.minStackSize = 40;  // Keep at least 40
copperConfig.maxStackSize = 100; // Sell when exceeds 100
copperConfig.auctionStackSize = 20; // Sell in stacks of 20
copperConfig.preferBuyout = true;

bridge->SetMaterialStockpile(2770, copperConfig);

// Configure crafted item selling
CraftedItemAuctionConfig swordConfig;
swordConfig.itemId = 12345; // Iron Sword
swordConfig.minProfitMargin = 50 * 10000; // 50g minimum profit
swordConfig.undercutCompetition = true;
swordConfig.undercutRate = 0.05f; // 5% undercut
swordConfig.maxListingDuration = 24; // 24 hours

bridge->SetCraftedItemAuction(12345, swordConfig);

// Enable automation
auto profile = bridge->GetAuctionProfile();
profile.autoSellEnabled = true;
profile.buyMaterialsForLeveling = true;
profile.auctionBudget = 1000 * 10000; // 1000g budget
bridge->SetAuctionProfile(profile);

// Manual operations
bridge->SellExcessMaterials();  // Sell excess materials now
bridge->SellCraftedItems();     // Sell profitable crafted items
bridge->BuyMaterialsForLeveling(ProfessionType::BLACKSMITHING);

// Check statistics
auto stats = bridge->GetStatistics();
TC_LOG_INFO("Materials listed: {}", stats.materialsListedCount);
TC_LOG_INFO("Crafted items sold: {}", stats.craftedsListedCount);
TC_LOG_INFO("Materials bought: {}", stats.materialsBought);
TC_LOG_INFO("Gold spent: {}g", stats.goldSpentOnMaterials / 10000);
```

---

## Economy Systems

### TradeManager

**Purpose**: Manages player-to-player trading operations.

**Location**: `src/modules/Playerbot/Social/TradeManager.h`

#### Key Responsibilities

- Trade initiation and acceptance
- Item exchange management
- Gold transactions
- Trade window automation

#### Core Methods

```cpp
/**
 * @brief Initiate a trade with another player
 * @param targetGuid GUID of player to trade with
 * @return true if trade was initiated
 */
bool InitiateTrade(ObjectGuid targetGuid);

/**
 * @brief Accept a trade request
 * @return true if trade was accepted
 */
bool AcceptTrade();

/**
 * @brief Add an item to the trade window
 * @param itemGuid Item GUID to add
 * @param slot Trade slot (0-5)
 * @return true if item was added
 */
bool AddItemToTrade(ObjectGuid itemGuid, uint8 slot);

/**
 * @brief Set gold amount in trade
 * @param copper Amount of gold in copper
 */
void SetTradeGold(uint32 copper);

/**
 * @brief Cancel current trade
 */
void CancelTrade();
```

#### Usage Example

```cpp
auto* tradeMgr = gameSystems->GetTradeManager();

// Initiate trade with nearby player
Player* nearbyPlayer = FindNearestPlayer();
if (nearbyPlayer && tradeMgr->InitiateTrade(nearbyPlayer->GetGUID()))
{
    // Add items to trade
    Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, 0);
    if (item)
        tradeMgr->AddItemToTrade(item->GetGUID(), 0);
    
    // Set gold
    tradeMgr->SetTradeGold(100 * 10000); // 100 gold
    
    // Accept trade
    tradeMgr->AcceptTrade();
}
```

---

### AuctionManager

**Purpose**: Manages auction house operations (buying, selling, market scanning).

**Location**: `src/modules/Playerbot/Economy/AuctionManager.h`

#### Key Responsibilities

- Auction house access
- Item listing and bidding
- Market price analysis
- Auction expiration tracking

#### Core Methods

```cpp
/**
 * @brief List an item on the auction house
 * @param itemGuid Item GUID to list
 * @param stackSize Stack size (for stackable items)
 * @param bidPrice Starting bid in copper
 * @param buyoutPrice Buyout price in copper (0 for no buyout)
 * @param duration Auction duration in hours (12, 24, or 48)
 * @return true if item was listed
 */
bool ListItem(ObjectGuid itemGuid, uint32 stackSize, uint32 bidPrice, uint32 buyoutPrice, uint32 duration);

/**
 * @brief Place a bid on an auction
 * @param auctionId Auction ID
 * @param bidAmount Bid amount in copper
 * @return true if bid was placed
 */
bool PlaceBid(uint32 auctionId, uint32 bidAmount);

/**
 * @brief Buyout an auction
 * @param auctionId Auction ID
 * @return true if buyout was successful
 */
bool BuyoutAuction(uint32 auctionId);

/**
 * @brief Search for auctions of a specific item
 * @param itemId Item entry ID
 * @param maxResults Maximum number of results
 * @return Vector of AuctionInfo structures
 */
std::vector<AuctionInfo> SearchAuctions(uint32 itemId, uint32 maxResults = 50);

/**
 * @brief Get market price for an item
 * @param itemId Item entry ID
 * @return Average market price in copper (0 if no data)
 */
uint32 GetMarketPrice(uint32 itemId) const;

/**
 * @brief Cancel an active auction
 * @param auctionId Auction ID
 * @return true if auction was cancelled
 */
bool CancelAuction(uint32 auctionId);
```

#### Usage Example

```cpp
auto* auctionMgr = gameSystems->GetAuctionManager();

// Sell an item
Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, 0);
if (item)
{
    uint32 marketPrice = auctionMgr->GetMarketPrice(item->GetEntry());
    uint32 bidPrice = marketPrice * 0.95f;
    uint32 buyoutPrice = marketPrice;
    
    auctionMgr->ListItem(item->GetGUID(), 1, bidPrice, buyoutPrice, 24);
}

// Buy an item
auto auctions = auctionMgr->SearchAuctions(2447, 10); // Search for Peacebloom
for (auto const& auction : auctions)
{
    if (auction.buyoutPrice > 0 && auction.buyoutPrice <= 10 * 10000) // Max 10g
    {
        auctionMgr->BuyoutAuction(auction.auctionId);
        break;
    }
}
```

---

## Combat Systems

### CombatStateManager

**Purpose**: Manages combat state transitions and combat flow.

**Location**: `src/modules/Playerbot/Combat/CombatStateManager.h`

#### Key Responsibilities

- Combat state tracking (in/out of combat)
- Threat management
- Combat target selection
- Combat initiation and exit

#### Core Methods

```cpp
/**
 * @brief Check if bot is in combat
 * @return true if currently in combat
 */
bool IsInCombat() const;

/**
 * @brief Enter combat with a target
 * @param targetGuid Target GUID
 * @return true if combat was initiated
 */
bool EnterCombat(ObjectGuid targetGuid);

/**
 * @brief Exit combat
 * @return true if combat was exited
 */
bool ExitCombat();

/**
 * @brief Get current combat target
 * @return Target GUID (null if no target)
 */
ObjectGuid GetCurrentTarget() const;

/**
 * @brief Set combat target
 * @param targetGuid Target GUID
 */
void SetTarget(ObjectGuid targetGuid);

/**
 * @brief Get threat level for a target
 * @param targetGuid Target GUID
 * @return Threat value
 */
float GetThreat(ObjectGuid targetGuid) const;

/**
 * @brief Check if bot should flee from combat
 * @return true if health/mana is critically low
 */
bool ShouldFlee() const;
```

#### Usage Example

```cpp
auto* combatMgr = gameSystems->GetCombatStateManager();

// Enter combat
Creature* enemy = FindNearestEnemy();
if (enemy && !combatMgr->IsInCombat())
{
    combatMgr->EnterCombat(enemy->GetGUID());
}

// During combat
if (combatMgr->IsInCombat())
{
    // Check if should flee
    if (combatMgr->ShouldFlee())
    {
        combatMgr->ExitCombat();
        // Run away logic
    }
    
    // Monitor threat
    ObjectGuid target = combatMgr->GetCurrentTarget();
    float threat = combatMgr->GetThreat(target);
    TC_LOG_DEBUG("Current threat: {}", threat);
}
```

---

### TargetScanner

**Purpose**: Scans for and evaluates potential targets.

**Location**: `src/modules/Playerbot/Combat/TargetScanner.h`

#### Key Responsibilities

- Target detection
- Target prioritization
- Range checking
- Line-of-sight validation

#### Core Methods

```cpp
/**
 * @brief Find all hostile targets in range
 * @param range Search radius in yards
 * @return Vector of hostile creature GUIDs
 */
std::vector<ObjectGuid> FindHostileTargets(float range = 40.0f);

/**
 * @brief Find best target based on criteria
 * @param range Search radius
 * @return GUID of best target (null if none found)
 */
ObjectGuid FindBestTarget(float range = 40.0f);

/**
 * @brief Check if target is valid
 * @param targetGuid Target GUID
 * @return true if target exists, is alive, hostile, and in range
 */
bool IsValidTarget(ObjectGuid targetGuid) const;

/**
 * @brief Get distance to target
 * @param targetGuid Target GUID
 * @return Distance in yards (0 if invalid)
 */
float GetDistanceToTarget(ObjectGuid targetGuid) const;

/**
 * @brief Check if target is in line of sight
 * @param targetGuid Target GUID
 * @return true if LOS is clear
 */
bool HasLineOfSight(ObjectGuid targetGuid) const;

/**
 * @brief Evaluate target priority
 * @param targetGuid Target GUID
 * @return Priority score (higher = better target)
 */
float EvaluateTargetPriority(ObjectGuid targetGuid) const;
```

#### Usage Example

```cpp
auto* scanner = gameSystems->GetTargetScanner();

// Find and select best target
ObjectGuid bestTarget = scanner->FindBestTarget(40.0f);
if (bestTarget)
{
    if (scanner->IsValidTarget(bestTarget) && scanner->HasLineOfSight(bestTarget))
    {
        float distance = scanner->GetDistanceToTarget(bestTarget);
        TC_LOG_INFO("Engaging target at {} yards", distance);
        
        combatMgr->EnterCombat(bestTarget);
    }
}

// Find all hostiles for AOE
auto hostiles = scanner->FindHostileTargets(10.0f);
if (hostiles.size() >= 3)
{
    // Use AOE abilities
    TC_LOG_INFO("Found {} enemies for AOE", hostiles.size());
}
```

---

## Movement System

### UnifiedMovementCoordinator

**Purpose**: Centralized movement control and pathfinding.

**Location**: `src/modules/Playerbot/Movement/UnifiedMovementCoordinator.h`

#### Key Responsibilities

- Pathfinding
- Movement to positions/targets
- Formation management
- Follow behavior
- Obstacle avoidance

#### Core Methods

```cpp
/**
 * @brief Move to a position
 * @param destination Target position
 * @return true if movement was initiated
 */
bool MoveTo(Position const& destination);

/**
 * @brief Move to a target
 * @param targetGuid Target GUID
 * @param distance Desired distance from target
 * @return true if movement was initiated
 */
bool MoveToTarget(ObjectGuid targetGuid, float distance = 0.0f);

/**
 * @brief Follow a target
 * @param targetGuid Target GUID to follow
 * @param distance Follow distance
 * @return true if follow was initiated
 */
bool Follow(ObjectGuid targetGuid, float distance = 5.0f);

/**
 * @brief Stop all movement
 */
void StopMovement();

/**
 * @brief Check if bot is currently moving
 * @return true if moving
 */
bool IsMoving() const;

/**
 * @brief Get current destination
 * @return Destination position (null position if not moving)
 */
Position GetDestination() const;

/**
 * @brief Calculate path to destination
 * @param destination Target position
 * @return Vector of waypoints (empty if no path)
 */
std::vector<Position> CalculatePath(Position const& destination);

/**
 * @brief Check if position is reachable
 * @param position Target position
 * @return true if path exists
 */
bool IsReachable(Position const& position) const;
```

#### Usage Example

```cpp
auto* movement = gameSystems->GetMovementCoordinator();

// Move to a position
Position destination(1234.5f, 5678.9f, 123.4f);
if (movement->IsReachable(destination))
{
    movement->MoveTo(destination);
    
    // Wait for arrival
    while (movement->IsMoving())
    {
        // Update logic
    }
}

// Follow a player
Player* leader = GetGroupLeader();
if (leader)
{
    movement->Follow(leader->GetGUID(), 5.0f); // Follow at 5 yards
}

// Move to combat target
ObjectGuid target = combatMgr->GetCurrentTarget();
if (target)
{
    float optimalRange = 5.0f; // Melee range
    movement->MoveToTarget(target, optimalRange);
}

// Calculate and visualize path
auto path = movement->CalculatePath(destination);
for (auto const& waypoint : path)
{
    TC_LOG_DEBUG("Waypoint: ({}, {}, {})", 
        waypoint.GetPositionX(), waypoint.GetPositionY(), waypoint.GetPositionZ());
}
```

---

## Lifecycle Systems

### DeathRecoveryManager

**Purpose**: Handles bot death, corpse retrieval, and resurrection.

**Location**: `src/modules/Playerbot/Lifecycle/DeathRecoveryManager.h`

#### Key Responsibilities

- Death detection
- Spirit healer interaction
- Corpse retrieval
- Durability management
- Resurrection coordination

#### Core Methods

```cpp
/**
 * @brief Check if bot is dead
 * @return true if dead or ghost
 */
bool IsDead() const;

/**
 * @brief Handle death event
 * Called automatically when bot dies
 */
void OnDeath();

/**
 * @brief Retrieve corpse
 * @return true if corpse was retrieved
 */
bool RetrieveCorpse();

/**
 * @brief Resurrect at spirit healer
 * @return true if resurrection was successful
 */
bool ResurrectAtSpiritHealer();

/**
 * @brief Get distance to corpse
 * @return Distance in yards
 */
float GetDistanceToCorpse() const;

/**
 * @brief Check if corpse is retrievable
 * @return true if bot is in range and can retrieve
 */
bool CanRetrieveCorpse() const;

/**
 * @brief Repair equipment
 * @param repairCost Maximum gold to spend on repairs
 * @return true if repairs were made
 */
bool RepairEquipment(uint32 repairCost);
```

#### Usage Example

```cpp
auto* deathMgr = gameSystems->GetDeathRecoveryManager();

if (deathMgr->IsDead())
{
    // Find and retrieve corpse
    float distance = deathMgr->GetDistanceToCorpse();
    TC_LOG_INFO("Corpse is {} yards away", distance);
    
    if (deathMgr->CanRetrieveCorpse())
    {
        deathMgr->RetrieveCorpse();
    }
    else
    {
        // Move to corpse
        movement->MoveTo(GetCorpsePosition());
    }
}

// After resurrection, repair equipment
if (GetDurabilityPercent() < 50.0f)
{
    uint32 maxRepairCost = 50 * 10000; // Max 50 gold
    deathMgr->RepairEquipment(maxRepairCost);
}
```

---

## Group Systems

### GroupCoordinator

**Purpose**: Manages group/raid mechanics, role assignment, and coordination.

**Location**: `src/modules/Playerbot/Advanced/GroupCoordinator.h`

#### Key Responsibilities

- Group formation and management
- Role assignment (Tank/Healer/DPS)
- Raid coordination
- Dungeon/raid strategy execution

#### Core Methods

```cpp
/**
 * @brief Check if bot is in a group
 * @return true if in group or raid
 */
bool IsInGroup() const;

/**
 * @brief Get bot's role in group
 * @return GroupRole enum (TANK, HEALER, DPS)
 */
GroupRole GetRole() const;

/**
 * @brief Set bot's role
 * @param role GroupRole to assign
 */
void SetRole(GroupRole role);

/**
 * @brief Get group members
 * @return Vector of player GUIDs in group
 */
std::vector<ObjectGuid> GetGroupMembers() const;

/**
 * @brief Get group leader
 * @return Leader GUID
 */
ObjectGuid GetGroupLeader() const;

/**
 * @brief Check if bot is group leader
 * @return true if leader
 */
bool IsGroupLeader() const;

/**
 * @brief Get tank in group
 * @return Tank GUID (null if no tank)
 */
ObjectGuid GetTank() const;

/**
 * @brief Get healers in group
 * @return Vector of healer GUIDs
 */
std::vector<ObjectGuid> GetHealers() const;
```

#### Usage Example

```cpp
auto* groupCoord = gameSystems->GetGroupCoordinator();

if (groupCoord->IsInGroup())
{
    // Get role assignment
    GroupRole role = groupCoord->GetRole();
    
    switch (role)
    {
        case GroupRole::TANK:
            // Tank behavior
            ObjectGuid target = scanner->FindBestTarget();
            combatMgr->EnterCombat(target);
            break;
            
        case GroupRole::HEALER:
            // Healer behavior
            auto members = groupCoord->GetGroupMembers();
            for (auto memberGuid : members)
            {
                Player* member = GetPlayer(memberGuid);
                if (member && member->GetHealthPct() < 70.0f)
                {
                    // Heal member
                }
            }
            break;
            
        case GroupRole::DPS:
            // DPS behavior
            ObjectGuid tank = groupCoord->GetTank();
            Unit* tankTarget = GetTankTarget(tank);
            if (tankTarget)
                combatMgr->EnterCombat(tankTarget->GetGUID());
            break;
    }
}
```

---

### GroupInvitationHandler

**Purpose**: Handles group invitation logic.

**Location**: `src/modules/Playerbot/Group/GroupInvitationHandler.h`

#### Core Methods

```cpp
/**
 * @brief Invite a player to group
 * @param playerGuid Player GUID to invite
 * @return true if invitation was sent
 */
bool InvitePlayer(ObjectGuid playerGuid);

/**
 * @brief Accept a group invitation
 * @param inviterGuid Inviter GUID
 * @return true if invitation was accepted
 */
bool AcceptInvitation(ObjectGuid inviterGuid);

/**
 * @brief Decline a group invitation
 * @param inviterGuid Inviter GUID
 */
void DeclineInvitation(ObjectGuid inviterGuid);

/**
 * @brief Check if bot has pending invitation
 * @return true if invitation is pending
 */
bool HasPendingInvitation() const;
```

#### Usage Example

```cpp
auto* inviteHandler = gameSystems->GetGroupInvitationHandler();

// Accept invitations from friends
if (inviteHandler->HasPendingInvitation())
{
    ObjectGuid inviter = GetPendingInviter();
    if (IsFriend(inviter))
    {
        inviteHandler->AcceptInvitation(inviter);
    }
    else
    {
        inviteHandler->DeclineInvitation(inviter);
    }
}

// Invite nearby players
Player* nearbyPlayer = FindNearbyPlayer();
if (nearbyPlayer && !groupCoord->IsInGroup())
{
    inviteHandler->InvitePlayer(nearbyPlayer->GetGUID());
}
```

---

## Decision Systems

The decision system consists of **4 managers** implementing hybrid AI architecture.

### System Architecture

```
HybridAIController (Orchestrator)
    ↓
    ├─→ DecisionFusionSystem (Strategic)
    ├─→ BehaviorTree (Tactical)
    └─→ ActionPriorityQueue (Execution)
         ↓
    BehaviorPriorityManager (Priority)
```

### DecisionFusionSystem

**Purpose**: Strategic-level decision making using multiple AI techniques.

**Location**: `src/modules/Playerbot/Decision/DecisionFusionSystem.h`

#### Key Responsibilities

- Long-term goal planning
- State evaluation
- Decision fusion from multiple sources
- Strategy selection

#### Core Methods

```cpp
/**
 * @brief Evaluate current situation and make strategic decision
 * @return Strategic decision enum
 */
StrategicDecision EvaluateSituation();

/**
 * @brief Set strategic goal
 * @param goal Goal to pursue
 */
void SetGoal(StrategicGoal goal);

/**
 * @brief Get current strategic goal
 * @return Current StrategicGoal
 */
StrategicGoal GetCurrentGoal() const;

/**
 * @brief Check if goal is achieved
 * @return true if current goal is complete
 */
bool IsGoalAchieved() const;
```

---

### BehaviorTree

**Purpose**: Tactical-level behavior execution using behavior tree architecture.

**Location**: `src/modules/Playerbot/Decision/BehaviorTree.h`

#### Key Responsibilities

- Behavior tree execution
- Condition evaluation
- Action selection
- Tree traversal

#### Core Methods

```cpp
/**
 * @brief Execute behavior tree
 * @return Execution result (SUCCESS, FAILURE, RUNNING)
 */
BehaviorTreeResult Execute();

/**
 * @brief Set active behavior tree
 * @param treeId Tree identifier
 */
void SetActiveTree(uint32 treeId);

/**
 * @brief Get current tree node
 * @return Current node ID
 */
uint32 GetCurrentNode() const;
```

---

### ActionPriorityQueue

**Purpose**: Action execution queue with priority management.

**Location**: `src/modules/Playerbot/Decision/ActionPriorityQueue.h`

#### Core Methods

```cpp
/**
 * @brief Add action to queue
 * @param action Action to execute
 * @param priority Action priority
 */
void EnqueueAction(BotAction const& action, float priority);

/**
 * @brief Get highest priority action
 * @return Next action to execute
 */
BotAction GetNextAction();

/**
 * @brief Clear all actions
 */
void ClearQueue();

/**
 * @brief Get queue size
 * @return Number of pending actions
 */
uint32 GetQueueSize() const;
```

---

### HybridAIController

**Purpose**: Orchestrates all decision systems for cohesive AI behavior.

**Location**: `src/modules/Playerbot/Decision/HybridAIController.h`

#### Core Methods

```cpp
/**
 * @brief Update AI systems
 * @param diff Time delta
 */
void Update(uint32 diff);

/**
 * @brief Set AI mode
 * @param mode AI behavior mode (PASSIVE, DEFENSIVE, AGGRESSIVE)
 */
void SetMode(AIMode mode);

/**
 * @brief Get current AI mode
 * @return Current AIMode
 */
AIMode GetMode() const;
```

#### Usage Example

```cpp
auto* hybridAI = gameSystems->GetHybridAI();
auto* decisionFusion = gameSystems->GetDecisionFusion();
auto* behaviorTree = gameSystems->GetBehaviorTree();
auto* actionQueue = gameSystems->GetActionPriorityQueue();

// Set strategic goal
decisionFusion->SetGoal(StrategicGoal::LEVEL_UP);

// Execute behavior tree
BehaviorTreeResult result = behaviorTree->Execute();

// Process action queue
if (actionQueue->GetQueueSize() > 0)
{
    BotAction action = actionQueue->GetNextAction();
    // Execute action
}

// Set AI mode
hybridAI->SetMode(AIMode::AGGRESSIVE);
```

---

## Core Infrastructure

### EventDispatcher

**Purpose**: Event system for inter-manager communication.

**Location**: `src/modules/Playerbot/Core/Events/EventDispatcher.h`

#### Core Methods

```cpp
/**
 * @brief Subscribe to an event type
 * @param eventType Event type to subscribe to
 * @param callback Callback function
 * @return Subscription ID
 */
uint32 Subscribe(EventType eventType, EventCallback callback);

/**
 * @brief Unsubscribe from events
 * @param subscriptionId Subscription ID from Subscribe()
 */
void Unsubscribe(uint32 subscriptionId);

/**
 * @brief Dispatch an event
 * @param event Event to dispatch
 */
void Dispatch(Event const& event);
```

#### Usage Example

```cpp
auto* eventDispatcher = gameSystems->GetEventDispatcher();

// Subscribe to combat events
uint32 subId = eventDispatcher->Subscribe(
    EventType::COMBAT_ENTER,
    [](Event const& event) {
        TC_LOG_INFO("Entered combat with {}", event.targetGuid);
    }
);

// Dispatch custom event
Event customEvent;
customEvent.type = EventType::CUSTOM;
customEvent.data = "example data";
eventDispatcher->Dispatch(customEvent);

// Cleanup
eventDispatcher->Unsubscribe(subId);
```

---

### ManagerRegistry

**Purpose**: Central registry for all managers (dependency injection).

**Location**: `src/modules/Playerbot/Core/Managers/ManagerRegistry.h`

#### Core Methods

```cpp
/**
 * @brief Register a manager
 * @param managerName Manager identifier
 * @param manager Manager pointer
 */
void RegisterManager(std::string const& managerName, void* manager);

/**
 * @brief Get a manager by name
 * @param managerName Manager identifier
 * @return Manager pointer (cast to appropriate type)
 */
void* GetManager(std::string const& managerName) const;

/**
 * @brief Check if manager is registered
 * @param managerName Manager identifier
 * @return true if registered
 */
bool HasManager(std::string const& managerName) const;
```

---

### BehaviorPriorityManager

**Purpose**: Manages behavior priority and scheduling.

**Location**: `src/modules/Playerbot/BehaviorPriorityManager.h`

#### Core Methods

```cpp
/**
 * @brief Set behavior priority
 * @param behaviorId Behavior identifier
 * @param priority Priority value (higher = more important)
 */
void SetPriority(uint32 behaviorId, float priority);

/**
 * @brief Get behavior priority
 * @param behaviorId Behavior identifier
 * @return Priority value
 */
float GetPriority(uint32 behaviorId) const;

/**
 * @brief Get highest priority behavior
 * @return Behavior ID
 */
uint32 GetHighestPriorityBehavior() const;
```

---

## Integration Guide

### Initial Setup

```cpp
// In BotAI constructor
class BotAI
{
public:
    BotAI(Player* bot)
    {
        // Create GameSystemsManager facade
        _gameSystems = std::make_unique<GameSystemsManager>(bot, this);
    }
    
    void Initialize()
    {
        // Initialize all managers
        _gameSystems->Initialize(_bot);
    }
    
    void Update(uint32 diff)
    {
        // Update all managers
        _gameSystems->Update(diff);
    }
    
private:
    std::unique_ptr<IGameSystemsManager> _gameSystems;
    Player* _bot;
};
```

### Accessing Managers

```cpp
// Always access via GameSystemsManager facade
auto* gameSystems = botAI->GetGameSystems();

// Get specific managers
auto* questMgr = gameSystems->GetQuestManager();
auto* profMgr = gameSystems->GetProfessionManager();
auto* combatMgr = gameSystems->GetCombatStateManager();
```

### Complete Bot Behavior Example

```cpp
void BotAI::ExecuteBehavior()
{
    auto* gameSystems = GetGameSystems();
    
    // 1. Check death status
    auto* deathMgr = gameSystems->GetDeathRecoveryManager();
    if (deathMgr->IsDead())
    {
        deathMgr->RetrieveCorpse();
        return;
    }
    
    // 2. Check combat
    auto* combatMgr = gameSystems->GetCombatStateManager();
    if (combatMgr->IsInCombat())
    {
        ExecuteCombatBehavior();
        return;
    }
    
    // 3. Process quests
    auto* questMgr = gameSystems->GetQuestManager();
    auto activeQuests = questMgr->GetActiveQuests();
    for (uint32 questId : activeQuests)
    {
        if (questMgr->IsQuestComplete(questId))
        {
            questMgr->TurnInQuest(questId);
        }
    }
    
    // 4. Process professions
    auto* profMgr = gameSystems->GetProfessionManager();
    auto professions = profMgr->GetPlayerProfessions();
    for (auto const& prof : professions)
    {
        if (prof.currentSkill < prof.maxSkill)
        {
            // Level profession
            auto* recipe = profMgr->GetOptimalLevelingRecipe(prof.profession);
            if (recipe && profMgr->HasMaterialsForRecipe(recipe->recipeId, 1))
            {
                profMgr->CraftItem(recipe->recipeId);
            }
            else
            {
                // Gather or buy materials
                auto* bridge = gameSystems->GetAuctionMaterialsBridge();
                auto decision = bridge->GetBestMaterialSource(materialId, quantity);
                // Execute decision
            }
        }
    }
    
    // 5. Idle behavior
    ExecuteIdleBehavior();
}

void BotAI::ExecuteCombatBehavior()
{
    auto* gameSystems = GetGameSystems();
    auto* combatMgr = gameSystems->GetCombatStateManager();
    auto* scanner = gameSystems->GetTargetScanner();
    auto* movement = gameSystems->GetMovementCoordinator();
    
    // Get best target
    ObjectGuid target = scanner->FindBestTarget();
    if (!target)
    {
        combatMgr->ExitCombat();
        return;
    }
    
    // Move to target
    float distance = scanner->GetDistanceToTarget(target);
    if (distance > 5.0f)
    {
        movement->MoveToTarget(target, 5.0f);
    }
    
    // Execute rotation
    ExecuteCombatRotation(target);
    
    // Check for flee
    if (combatMgr->ShouldFlee())
    {
        combatMgr->ExitCombat();
        movement->StopMovement();
        // Flee logic
    }
}
```

---

## Best Practices

### 1. Always Use Facade Pattern

```cpp
// ✅ GOOD: Access via facade
auto* manager = gameSystems->GetQuestManager();

// ❌ BAD: Direct singleton access
auto* manager = QuestManager::instance(); // Don't do this!
```

### 2. Check Null Pointers

```cpp
auto* profMgr = gameSystems->GetProfessionManager();
if (profMgr)
{
    profMgr->CraftItem(recipeId);
}
```

### 3. Use Per-Bot Instances

```cpp
// ✅ GOOD: Per-bot methods (no Player* parameter)
profMgr->CraftItem(recipeId);
bridge->GetBestMaterialSource(itemId, quantity);

// ❌ BAD: Old singleton pattern (deprecated)
ProfessionManager::instance()->CraftItem(player, recipeId);
```

### 4. Event Filtering

When subscribing to events, remember that per-bot managers automatically filter events:

```cpp
// Events are automatically filtered by playerGuid
// Only events for THIS bot are processed
void HandleEvent(ProfessionEvent const& event)
{
    if (event.playerGuid != _bot->GetGUID())
        return; // Already filtered by bridge
    
    // Process event
}
```

### 5. Resource Management

Managers are owned by GameSystemsManager via `std::unique_ptr`:

```cpp
// ✅ GOOD: Let unique_ptr handle cleanup
// Automatically destroyed when GameSystemsManager destructs

// ❌ BAD: Manual deletion
delete manager; // Never do this!
```

### 6. Update Frequency

Not all managers need to update every frame:

```cpp
void Update(uint32 diff)
{
    _timer += diff;
    
    // Update every 1 second
    if (_timer >= 1000)
    {
        _timer = 0;
        ExpensiveOperation();
    }
}
```

### 7. Thread Safety

All managers are per-bot and single-threaded:

```cpp
// ✅ GOOD: No mutex needed (per-bot isolation)
_profile.autoSellEnabled = true;

// ❌ BAD: Unnecessary mutex (old singleton pattern)
std::lock_guard lock(_mutex); // Not needed anymore!
```

---

## Troubleshooting

### Common Issues

**Issue**: "Manager returns null"
- **Solution**: Ensure GameSystemsManager is initialized before accessing managers

**Issue**: "Events not firing"
- **Solution**: Check that manager is subscribed to ProfessionEventBus

**Issue**: "Compilation errors with Player* parameters"
- **Solution**: Remove Player* parameters from method calls (per-bot refactoring)

**Issue**: "Data not persisting across updates"
- **Solution**: Ensure data is stored in per-bot members, not local variables

---

## API Reference

For detailed API documentation, see:
- `/src/modules/Playerbot/Core/Managers/IGameSystemsManager.h` - Interface definition
- `/src/modules/Playerbot/Core/Managers/GameSystemsManager.h` - Facade implementation
- Individual manager headers in respective directories

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-11-18 | Initial comprehensive developer guide |
| | | Documented all 21 managers systematically |
| | | Added usage examples for each system |
| | | Included best practices and integration guide |

---

**Document Status**: ✅ Complete
**Maintainer**: TrinityCore PlayerBot Team
**Last Review**: 2025-11-18

---

## Bot Creation and Spawning Systems

### Overview

The bot creation and spawning system manages the entire lifecycle of PlayerBot instances, from initial creation through active gameplay to cleanup and removal.

### System Architecture

```
Bot Creation Flow:
    ↓
BotManager (Singleton)
    ↓
BotFactory → Creates Bot Player Instance
    ↓
BotSession → Creates Session for Bot
    ↓
BotAI → Creates AI Controller
    ↓
GameSystemsManager → Initializes 21 Managers
    ↓
Bot Active in World
```

---

### BotManager

**Purpose**: Central manager for all bot instances in the world.

**Location**: `src/modules/Playerbot/Core/BotManager.h`

**Architecture**: Global singleton (manages all bots server-wide)

#### Key Responsibilities

- Bot instance tracking and registry
- Bot creation and deletion
- Bot-to-player association
- Bot command processing
- Bot persistence (database save/load)

#### Core Methods

##### Bot Creation

```cpp
/**
 * @brief Create a new bot
 * @param owner Player who owns the bot
 * @param name Bot character name
 * @param race Race ID (1=Human, 2=Orc, etc.)
 * @param class_ Class ID (1=Warrior, 2=Paladin, etc.)
 * @param gender Gender (0=Male, 1=Female)
 * @return Pointer to created bot, nullptr on failure
 */
Player* CreateBot(Player* owner, std::string const& name, uint8 race, uint8 class_, uint8 gender);

/**
 * @brief Spawn an existing bot character
 * @param botGuid GUID of bot character
 * @param owner Player who owns the bot
 * @return Pointer to spawned bot, nullptr on failure
 */
Player* SpawnBot(ObjectGuid botGuid, Player* owner);

/**
 * @brief Load bot from database
 * @param botGuid GUID of bot character
 * @return true if bot was loaded successfully
 */
bool LoadBot(ObjectGuid botGuid);
```

##### Bot Management

```cpp
/**
 * @brief Get all bots owned by a player
 * @param ownerGuid Owner player GUID
 * @return Vector of bot player pointers
 */
std::vector<Player*> GetPlayerBots(ObjectGuid ownerGuid) const;

/**
 * @brief Get bot by GUID
 * @param botGuid Bot GUID
 * @return Pointer to bot, nullptr if not found
 */
Player* GetBot(ObjectGuid botGuid) const;

/**
 * @brief Check if character is a bot
 * @param playerGuid Player GUID to check
 * @return true if player is a bot
 */
bool IsBot(ObjectGuid playerGuid) const;

/**
 * @brief Get bot owner
 * @param botGuid Bot GUID
 * @return Owner player GUID (null if no owner)
 */
ObjectGuid GetBotOwner(ObjectGuid botGuid) const;
```

##### Bot Removal

```cpp
/**
 * @brief Remove bot from world (despawn)
 * @param botGuid Bot GUID
 * @return true if bot was removed
 */
bool RemoveBot(ObjectGuid botGuid);

/**
 * @brief Delete bot permanently
 * @param botGuid Bot GUID
 * @return true if bot was deleted
 */
bool DeleteBot(ObjectGuid botGuid);

/**
 * @brief Remove all bots owned by a player
 * @param ownerGuid Owner player GUID
 */
void RemoveAllPlayerBots(ObjectGuid ownerGuid);
```

##### Bot Control

```cpp
/**
 * @brief Set bot AI state
 * @param botGuid Bot GUID
 * @param state AI state (IDLE, ACTIVE, PAUSED, DISABLED)
 */
void SetBotAIState(ObjectGuid botGuid, BotAIState state);

/**
 * @brief Teleport bot to owner
 * @param botGuid Bot GUID
 * @return true if teleport was successful
 */
bool TeleportBotToOwner(ObjectGuid botGuid);

/**
 * @brief Update all bots
 * @param diff Time delta in milliseconds
 */
void Update(uint32 diff);
```

##### Statistics

```cpp
/**
 * @brief Get total number of active bots
 * @return Active bot count
 */
uint32 GetActiveBotCount() const;

/**
 * @brief Get maximum allowed bots per player
 * @return Max bots per player
 */
uint32 GetMaxBotsPerPlayer() const;

/**
 * @brief Set maximum allowed bots per player
 * @param maxBots Maximum number
 */
void SetMaxBotsPerPlayer(uint32 maxBots);
```

#### Usage Example

```cpp
auto* botMgr = BotManager::instance();

// Create a new bot
Player* owner = GetPlayer();
Player* bot = botMgr->CreateBot(
    owner,
    "MyBot",          // Name
    1,                // Human
    1,                // Warrior
    0                 // Male
);

if (bot)
{
    TC_LOG_INFO("Created bot: {} (GUID: {})", bot->GetName(), bot->GetGUID());
    
    // Get owner's bots
    auto bots = botMgr->GetPlayerBots(owner->GetGUID());
    TC_LOG_INFO("{} has {} bots", owner->GetName(), bots.size());
    
    // Control the bot
    botMgr->SetBotAIState(bot->GetGUID(), BotAIState::ACTIVE);
    
    // Teleport bot to owner
    botMgr->TeleportBotToOwner(bot->GetGUID());
}

// Remove a bot
botMgr->RemoveBot(bot->GetGUID());

// Get statistics
uint32 activeBots = botMgr->GetActiveBotCount();
TC_LOG_INFO("Total active bots: {}", activeBots);
```

---

### BotFactory

**Purpose**: Factory pattern for creating bot instances with proper initialization.

**Location**: `src/modules/Playerbot/Core/BotFactory.h`

#### Core Methods

```cpp
/**
 * @brief Create a bot player instance
 * @param name Bot character name
 * @param race Race ID
 * @param class_ Class ID
 * @param gender Gender
 * @param level Starting level (default: 1)
 * @return Pointer to created player instance
 */
Player* CreateBotPlayer(
    std::string const& name,
    uint8 race,
    uint8 class_,
    uint8 gender,
    uint8 level = 1
);

/**
 * @brief Initialize bot equipment
 * @param bot Bot player instance
 * @param level Bot level
 */
void InitializeBotEquipment(Player* bot, uint8 level);

/**
 * @brief Initialize bot spells and abilities
 * @param bot Bot player instance
 */
void InitializeBotSpells(Player* bot);

/**
 * @brief Set bot starting location
 * @param bot Bot player instance
 * @return Starting position
 */
Position GetBotStartingLocation(Player* bot) const;
```

#### Usage Example

```cpp
auto* factory = BotFactory::instance();

// Create a level 20 warrior bot
Player* bot = factory->CreateBotPlayer("WarriorBot", 1, 1, 0, 20);

if (bot)
{
    // Initialize equipment for level 20
    factory->InitializeBotEquipment(bot, 20);
    
    // Initialize spells
    factory->InitializeBotSpells(bot);
    
    // Set starting location
    Position startPos = factory->GetBotStartingLocation(bot);
    bot->Relocate(startPos);
}
```

---

### BotSession

**Purpose**: Session management for bot connections (mimics player sessions).

**Location**: `src/modules/Playerbot/Core/BotSession.h`

#### Key Responsibilities

- Bot-server connection management
- Packet handling (simulated)
- Bot authentication
- Session state tracking

#### Core Methods

```cpp
/**
 * @brief Create a bot session
 * @param bot Player instance for the bot
 * @param owner Owner player
 * @return Pointer to created session
 */
static BotSession* CreateSession(Player* bot, Player* owner);

/**
 * @brief Get bot AI instance
 * @return Pointer to BotAI
 */
BotAI* GetBotAI() const;

/**
 * @brief Update session
 * @param diff Time delta
 */
void Update(uint32 diff);

/**
 * @brief Handle incoming packet (simulated)
 * @param packet WorldPacket to process
 */
void HandlePacket(WorldPacket& packet);

/**
 * @brief Check if session is active
 * @return true if session is connected
 */
bool IsActive() const;

/**
 * @brief Logout bot
 * @param save Save bot to database
 */
void LogoutBot(bool save = true);
```

---

### BotAI

**Purpose**: Core AI controller for bot behavior.

**Location**: `src/modules/Playerbot/Core/BotAI.h`

#### Initialization Flow

```cpp
// BotAI constructor
BotAI::BotAI(Player* bot)
    : _bot(bot)
{
    // Create GameSystemsManager facade (owns all 21 managers)
    _gameSystems = std::make_unique<GameSystemsManager>(bot, this);
}

// Initialize all systems
void BotAI::Initialize()
{
    // Initialize all managers via facade
    _gameSystems->Initialize(_bot);
    
    TC_LOG_INFO("BotAI initialized for bot: {}", _bot->GetName());
}

// Main update loop
void BotAI::Update(uint32 diff)
{
    // Update all managers
    _gameSystems->Update(diff);
    
    // Execute bot behavior
    ExecuteBehavior();
}
```

#### Core Methods

```cpp
/**
 * @brief Get GameSystemsManager facade
 * @return Pointer to IGameSystemsManager
 */
IGameSystemsManager* GetGameSystems() const;

/**
 * @brief Execute bot AI behavior
 * Called every update cycle
 */
void ExecuteBehavior();

/**
 * @brief Handle bot command
 * @param command Command string
 * @param args Command arguments
 */
void HandleCommand(std::string const& command, std::vector<std::string> const& args);

/**
 * @brief Set bot strategy
 * @param strategy Strategy type (FOLLOW, GRIND, QUEST, PVP, etc.)
 */
void SetStrategy(BotStrategy strategy);

/**
 * @brief Get current strategy
 * @return Current BotStrategy
 */
BotStrategy GetStrategy() const;
```

---

### Bot Command System

**Purpose**: Handle player commands to control bots.

**Location**: `src/modules/Playerbot/Commands/BotCommands.h`

#### Available Commands

```cpp
// Bot management commands
.bot add <name> <class> <race>     // Create and spawn a new bot
.bot remove <name>                  // Remove a bot
.bot delete <name>                  // Permanently delete a bot
.bot list                           // List all your bots

// Bot control commands
.bot summon <name>                  // Teleport bot to you
.bot follow                         // Make bots follow you
.bot stay                           // Make bots stay at current position
.bot attack                         // Make bots attack your target

// Bot AI commands
.bot ai <enable|disable>            // Enable/disable bot AI
.bot strategy <strategy>            // Set bot strategy
.bot autoloot <on|off>              // Enable/disable auto-looting

// Bot profession commands
.bot profession learn <prof>        // Learn a profession
.bot profession craft <item>        // Craft an item
.bot gather <on|off>                // Enable/disable auto-gathering

// Bot economy commands
.bot sell                           // Sell junk items
.bot repair                         // Repair equipment
.bot auction <buy|sell>             // Auction house operations
```

#### Command Implementation Example

```cpp
// .bot add command handler
void HandleBotAddCommand(Player* player, std::string const& name, 
                         uint8 classId, uint8 raceId)
{
    auto* botMgr = BotManager::instance();
    
    // Validate inputs
    if (!IsValidClass(classId) || !IsValidRace(raceId))
    {
        player->SendSystemMessage("Invalid class or race");
        return;
    }
    
    // Check bot limit
    auto bots = botMgr->GetPlayerBots(player->GetGUID());
    if (bots.size() >= botMgr->GetMaxBotsPerPlayer())
    {
        player->SendSystemMessage("You have reached the maximum number of bots");
        return;
    }
    
    // Create bot
    Player* bot = botMgr->CreateBot(player, name, raceId, classId, 0);
    
    if (bot)
    {
        player->SendSystemMessage("Bot created successfully: " + name);
        
        // Auto-summon to player
        botMgr->TeleportBotToOwner(bot->GetGUID());
    }
    else
    {
        player->SendSystemMessage("Failed to create bot");
    }
}
```

---

### Bot Database Schema

#### Characters Table Extensions

```sql
-- Bot-specific fields in characters table
ALTER TABLE characters ADD COLUMN is_bot TINYINT(1) DEFAULT 0;
ALTER TABLE characters ADD COLUMN bot_owner_guid BIGINT DEFAULT 0;
ALTER TABLE characters ADD COLUMN bot_ai_state TINYINT DEFAULT 1;

-- Bot configuration table
CREATE TABLE IF NOT EXISTS bot_config (
    bot_guid BIGINT PRIMARY KEY,
    owner_guid BIGINT NOT NULL,
    auto_loot TINYINT(1) DEFAULT 1,
    auto_gather TINYINT(1) DEFAULT 0,
    auto_quest TINYINT(1) DEFAULT 0,
    strategy VARCHAR(32) DEFAULT 'FOLLOW',
    FOREIGN KEY (bot_guid) REFERENCES characters(guid)
);

-- Bot profession configuration
CREATE TABLE IF NOT EXISTS bot_professions (
    bot_guid BIGINT,
    profession_type SMALLINT,
    auto_craft TINYINT(1) DEFAULT 0,
    auto_sell TINYINT(1) DEFAULT 0,
    PRIMARY KEY (bot_guid, profession_type)
);
```

---

### Bot Lifecycle States

```cpp
enum class BotLifecycleState
{
    UNINITIALIZED,      // Bot created but not initialized
    INITIALIZING,       // Bot systems initializing
    READY,              // Bot ready to be spawned
    SPAWNING,           // Bot being added to world
    ACTIVE,             // Bot active in world
    PAUSED,             // Bot AI paused
    DESPAWNING,         // Bot being removed from world
    CLEANUP             // Bot being deleted
};
```

#### State Transition Example

```cpp
void BotManager::TransitionBotState(ObjectGuid botGuid, BotLifecycleState newState)
{
    Player* bot = GetBot(botGuid);
    if (!bot)
        return;
    
    BotLifecycleState oldState = GetBotState(botGuid);
    
    // Validate state transition
    if (!IsValidStateTransition(oldState, newState))
    {
        TC_LOG_ERROR("Invalid state transition: {} -> {}", 
            StateToString(oldState), StateToString(newState));
        return;
    }
    
    // Execute state transition
    switch (newState)
    {
        case BotLifecycleState::INITIALIZING:
            InitializeBot(bot);
            break;
            
        case BotLifecycleState::SPAWNING:
            SpawnBotInWorld(bot);
            break;
            
        case BotLifecycleState::ACTIVE:
            ActivateBotAI(bot);
            break;
            
        case BotLifecycleState::PAUSED:
            PauseBotAI(bot);
            break;
            
        case BotLifecycleState::DESPAWNING:
            DespawnBot(bot);
            break;
            
        case BotLifecycleState::CLEANUP:
            CleanupBot(bot);
            break;
    }
    
    SetBotState(botGuid, newState);
    TC_LOG_DEBUG("Bot {} transitioned: {} -> {}", 
        bot->GetName(), StateToString(oldState), StateToString(newState));
}
```

---

### Complete Bot Creation Example

```cpp
/**
 * @brief Complete workflow for creating and spawning a bot
 */
Player* CreateAndSpawnBot(Player* owner, std::string const& name, 
                         uint8 race, uint8 class_, uint8 level)
{
    auto* botMgr = BotManager::instance();
    auto* factory = BotFactory::instance();
    
    // 1. Create bot player instance
    Player* bot = factory->CreateBotPlayer(name, race, class_, 0, level);
    if (!bot)
    {
        TC_LOG_ERROR("Failed to create bot player instance");
        return nullptr;
    }
    
    // 2. Initialize bot equipment and spells
    factory->InitializeBotEquipment(bot, level);
    factory->InitializeBotSpells(bot);
    
    // 3. Create bot session
    BotSession* session = BotSession::CreateSession(bot, owner);
    if (!session)
    {
        TC_LOG_ERROR("Failed to create bot session");
        delete bot;
        return nullptr;
    }
    
    // 4. Create bot AI
    BotAI* botAI = new BotAI(bot);
    bot->SetBotAI(botAI);
    
    // 5. Initialize AI systems (GameSystemsManager + 21 managers)
    botAI->Initialize();
    
    // 6. Register bot with manager
    botMgr->RegisterBot(bot, owner->GetGUID());
    
    // 7. Save bot to database
    bot->SaveToDB(true, false);
    
    // 8. Add bot to world
    Position spawnPos = factory->GetBotStartingLocation(bot);
    bot->Relocate(spawnPos);
    
    if (!bot->IsInWorld())
    {
        bot->AddToWorld();
    }
    
    // 9. Set initial AI state
    botMgr->SetBotAIState(bot->GetGUID(), BotAIState::ACTIVE);
    
    // 10. Teleport to owner
    botMgr->TeleportBotToOwner(bot->GetGUID());
    
    TC_LOG_INFO("Bot {} created and spawned successfully for owner {}",
        bot->GetName(), owner->GetName());
    
    return bot;
}
```

---

### Bot Cleanup and Removal

```cpp
/**
 * @brief Proper bot cleanup and removal
 */
void RemoveAndCleanupBot(ObjectGuid botGuid)
{
    auto* botMgr = BotManager::instance();
    Player* bot = botMgr->GetBot(botGuid);
    
    if (!bot)
        return;
    
    TC_LOG_INFO("Removing bot: {}", bot->GetName());
    
    // 1. Pause AI
    botMgr->SetBotAIState(botGuid, BotAIState::PAUSED);
    
    // 2. Save to database
    bot->SaveToDB(false, false);
    
    // 3. Remove from group (if in group)
    if (Group* group = bot->GetGroup())
    {
        group->RemoveMember(bot->GetGUID());
    }
    
    // 4. Shutdown GameSystemsManager (destroys all 21 managers)
    if (BotAI* botAI = bot->GetBotAI())
    {
        botAI->GetGameSystems()->Shutdown();
    }
    
    // 5. Remove from world
    if (bot->IsInWorld())
    {
        bot->RemoveFromWorld();
    }
    
    // 6. Cleanup session
    if (BotSession* session = bot->GetBotSession())
    {
        session->LogoutBot(true);
        delete session;
    }
    
    // 7. Cleanup AI
    if (BotAI* botAI = bot->GetBotAI())
    {
        delete botAI;
        bot->SetBotAI(nullptr);
    }
    
    // 8. Unregister from manager
    botMgr->UnregisterBot(botGuid);
    
    TC_LOG_INFO("Bot {} removed and cleaned up", bot->GetName());
}
```

---

### Bot Performance Considerations

#### Memory Management

```cpp
// Per-bot memory footprint estimate
// - Player instance: ~2-4 KB
// - BotAI instance: ~1 KB
// - GameSystemsManager + 21 managers: ~5-10 KB
// - Total per bot: ~8-15 KB

// For 100 bots: ~800 KB - 1.5 MB
// For 1000 bots: ~8-15 MB
```

#### Update Performance

```cpp
// Bot update frequency
const uint32 BOT_UPDATE_INTERVAL = 100; // Update every 100ms

void BotManager::Update(uint32 diff)
{
    _updateTimer += diff;
    
    if (_updateTimer < BOT_UPDATE_INTERVAL)
        return;
    
    _updateTimer = 0;
    
    // Update all active bots
    for (auto& [botGuid, bot] : _activeBots)
    {
        if (bot && bot->IsInWorld())
        {
            bot->Update(BOT_UPDATE_INTERVAL);
        }
    }
}
```

#### Optimization Tips

```cpp
// 1. Limit bot count per player
const uint32 MAX_BOTS_PER_PLAYER = 5;

// 2. Throttle expensive operations
const uint32 PROFESSION_CHECK_INTERVAL = 5000;  // Every 5 seconds
const uint32 AUCTION_CHECK_INTERVAL = 60000;    // Every minute

// 3. Use spatial partitioning for bot updates
// Only update bots near players

// 4. Lazy initialization
// Don't initialize unused managers until needed

// 5. Pool bot instances
// Reuse deleted bot instances instead of creating new ones
```

---

### Best Practices for Bot Systems

#### 1. Always Check Bot Validity

```cpp
// ✅ GOOD
Player* bot = botMgr->GetBot(botGuid);
if (bot && bot->IsInWorld())
{
    bot->Update(diff);
}

// ❌ BAD
Player* bot = botMgr->GetBot(botGuid);
bot->Update(diff); // Crash if bot is null!
```

#### 2. Proper Lifecycle Management

```cpp
// ✅ GOOD: Follow complete lifecycle
CreateBot() → InitializeBot() → SpawnBot() → ... → DespawnBot() → CleanupBot()

// ❌ BAD: Skip initialization
CreateBot() → SpawnBot() // Missing initialization!
```

#### 3. Resource Cleanup

```cpp
// ✅ GOOD: Cleanup in reverse order of creation
Shutdown GameSystemsManager
→ Cleanup BotAI
→ Cleanup BotSession
→ Remove from world
→ Delete bot instance

// ❌ BAD: Skip cleanup steps
delete bot; // Memory leaks!
```

#### 4. Thread Safety

```cpp
// ✅ GOOD: Update bots on main thread only
MainThread::Update()
{
    botMgr->Update(diff); // Safe
}

// ❌ BAD: Update bots from multiple threads
WorkerThread::Update()
{
    botMgr->Update(diff); // NOT THREAD-SAFE!
}
```

---

**Section Complete**: Bot Creation and Spawning Systems

This documentation covers the complete bot lifecycle from creation to cleanup, including all management systems, commands, database schema, and best practices.

