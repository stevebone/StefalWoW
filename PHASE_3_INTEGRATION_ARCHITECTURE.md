# Phase 3 System Integration Architecture

**Date**: 2025-11-10
**Status**: Integration Planning Complete - Implementation Pending
**Objective**: Unify all Phase 3 economic systems (Profession, Gathering, Auction, Banking) into cohesive bot economy

---

## Executive Summary

The PlayerBot module has **4 independent economic systems** that currently operate in isolation:
1. **ProfessionManager** (SINGLETON) - Crafting, recipe learning, skill management
2. **GatheringManager** (BehaviorManager) - Node detection, resource harvesting
3. **AuctionManager** (BehaviorManager) - Market analysis, auction trading
4. **BankingManager** (MISSING - only GuildBankManager exists)

**Integration Goal**: Connect these systems so bots can:
- Gather materials → Craft items → Sell on Auction House → Store gold in bank
- Buy materials from AH → Craft for profit → Reinvest earnings
- Optimize profession leveling path based on market prices

---

## 1. Current Architecture Assessment

### 1.1 ProfessionManager (SINGLETON Pattern)

**Status**: ✅ PRODUCTION-READY (after orphaned null check removal)
**Pattern**: Singleton (`ProfessionManager::instance()`)
**Thread Safety**: ❌ NO (uses per-bot instance data maps, no BehaviorManager integration)
**Update Method**: Called from `BotAI::UpdateAI()` line 3179

**Capabilities**:
```cpp
// Profession Management
bool LearnProfession(Player* player, ProfessionType profession);
uint16 GetProfessionSkill(Player* player, ProfessionType profession);
std::vector<ProfessionSkillInfo> GetPlayerProfessions(Player* player);

// Recipe System
bool LearnRecipe(Player* player, uint32 recipeId);
bool KnowsRecipe(Player* player, uint32 recipeId);
std::vector<RecipeInfo> GetCraftableRecipes(Player* player, ProfessionType profession);
RecipeInfo const* GetOptimalLevelingRecipe(Player* player, ProfessionType profession);

// Crafting Automation
bool CraftItem(Player* player, RecipeInfo const& recipe, uint32 quantity);
void QueueCraft(Player* player, uint32 recipeId, uint32 quantity);
bool HasMaterialsForRecipe(Player* player, RecipeInfo const& recipe);
std::vector<std::pair<uint32, uint32>> GetMissingMaterials(Player* player, RecipeInfo const& recipe);

// Skill Analysis
float GetSkillUpChance(Player* player, RecipeInfo const& recipe);
bool ShouldCraftForSkillUp(Player* player, RecipeInfo const& recipe);
```

**Integration Points**:
- Used by `FarmingCoordinator` for skill tracking
- Used by `ProfessionAuctionBridge` for craft → sell pipeline (EXISTS but needs review)
- Called from `BotAI::Update()` for automation

**Architecture Issue**:
- SINGLETON pattern conflicts with BehaviorManager design pattern used everywhere else
- No throttling (called every update, not interval-based)
- No mutex protection for shared state

---

### 1.2 GatheringManager (BehaviorManager Pattern)

**Status**: ✅ PRODUCTION-READY
**Pattern**: Extends `BehaviorManager`
**Thread Safety**: ✅ YES (atomic state flags, lock-free spatial grids)
**Update Interval**: 1000ms (1 second throttled updates)

**Capabilities**:
```cpp
// Node Detection
std::vector<GatheringNode> ScanForNodes(float range = 40.0f);
GatheringNode const* FindNearestNode(GatheringNodeType nodeType = GatheringNodeType::NONE);
bool CanGatherFromNode(GatheringNode const& node);

// Gathering Actions
bool GatherFromNode(GatheringNode const& node);
bool CastGatheringSpell(GatheringNode const& node);
bool LootNode(GameObject* gameObject);
bool SkinCreature(Creature* creature);

// Pathfinding
bool PathToNode(GatheringNode const& node);
bool IsInGatheringRange(GatheringNode const& node);

// Configuration
void SetGatheringEnabled(bool enable);
void SetProfessionEnabled(GatheringNodeType nodeType, bool enable);
void SetPrioritizeSkillUps(bool prioritize);

// Statistics
GatheringStatistics const& GetStatistics();
```

**Integration Points**:
- Used by `FarmingCoordinator` for automated material collection
- Needs connection to ProfessionManager for "gather materials I need to craft" logic

**Architecture Strength**:
- Perfect BehaviorManager implementation
- Lock-free spatial grid queries (Phase 5F integration)
- Atomic state flags for fast queries

---

### 1.3 AuctionManager (BehaviorManager Pattern)

**Status**: ✅ PRODUCTION-READY (after orphaned null check removal)
**Pattern**: Extends `BehaviorManager`
**Thread Safety**: ✅ YES (std::recursive_mutex, AuctionEventBus)
**Update Interval**: 5000ms (5 second throttled updates)

**Capabilities**:
```cpp
// Market Analysis
MarketSnapshot AnalyzeMarket(Player* bot, uint32 itemId);
uint64 GetRecommendedSellPrice(Player* bot, uint32 itemId);
std::vector<FlipOpportunity> FindFlipOpportunities(Player* bot);

// Auction Actions
bool PostAuction(Player* bot, uint32 itemId, uint32 stackSize, uint64 buyout, uint64 bid, uint32 duration);
bool BidOnAuction(Player* bot, uint32 auctionId, uint64 bidAmount);
bool BuyoutAuction(Player* bot, uint32 auctionId);
bool CancelAuction(Player* bot, uint32 auctionId);

// Automation Strategies
void ExecuteFlippingStrategy(Player* bot);
void ExecuteCommodityStrategy(Player* bot);
void ExecuteUndercutStrategy(Player* bot);

// Configuration
void SetStrategyEnabled(TradingStrategy strategy, bool enabled);
void SetMinProfitMargin(float margin);
void SetMaxAuctionDuration(uint32 hours);

// Statistics
AuctionStatistics const& GetStatistics();
```

**Integration Points**:
- Used by `ProfessionAuctionBridge` for selling crafted items (EXISTS)
- Needs connection to ProfessionManager for "buy materials I need" logic
- Event-driven via `AuctionEventBus` (auction sold, auction bought, etc.)

**Architecture Strength**:
- Event-driven architecture (AuctionEventBus)
- 6 trading strategies (flipping, commodity, undercut, snipe, crafting, flip)
- Market analysis with price volatility tracking

---

### 1.4 BankingManager (MISSING)

**Status**: ❌ DOES NOT EXIST
**What Exists**:
- `GuildBankManager` - Guild banking only
- `InteractionManager::AccessBank()` - Manual bank access

**What's Needed**:
```cpp
// Personal Banking Automation (NEEDS IMPLEMENTATION)
class BankingManager : public BehaviorManager
{
    // Auto-deposit
    bool DepositGold(Player* bot, uint64 amount);
    bool DepositItem(Player* bot, Item* item);
    void AutoDepositJunk(Player* bot);
    void AutoDepositCraftingMaterials(Player* bot);

    // Auto-withdraw
    bool WithdrawGold(Player* bot, uint64 amount);
    bool WithdrawItem(Player* bot, uint32 itemId, uint32 quantity);
    std::vector<Item*> WithdrawCraftingMaterials(Player* bot, RecipeInfo const& recipe);

    // Configuration
    void SetAutoDepositEnabled(bool enable);
    void SetMinGoldToKeep(uint64 amount);  // Keep X gold in bags, bank the rest

    // Queries
    uint64 GetBankGold(Player* bot);
    uint32 GetBankItemCount(Player* bot, uint32 itemId);
};
```

**Priority**: MEDIUM (nice-to-have, not critical for Phase 3 integration)

---

## 2. Integration Bridges Required

### 2.1 ProfessionAuctionBridge (PARTIALLY EXISTS)

**Status**: EXISTS at `/home/user/TrinityCore/src/modules/Playerbot/Professions/ProfessionAuctionBridge.cpp`
**Needs**: Code review and potential updates

**Purpose**: Connect crafting to auction selling

**Current Implementation**:
```cpp
// EXISTS - Review needed
class ProfessionAuctionBridge
{
    std::vector<std::pair<uint32, uint32>> GetMissingMaterialsForRecipe(...);
    bool ShouldCraftItemForProfit(...);
    bool CalculateCraftingProfit(...);
};
```

**Enhancement Needed**: Auto-sell crafted items

---

### 2.2 GatheringMaterialsBridge (NEEDS IMPLEMENTATION)

**Status**: DOES NOT EXIST
**Purpose**: Connect gathering to crafting material requirements

**Proposed API**:
```cpp
class GatheringMaterialsBridge
{
public:
    /**
     * @brief Get materials needed for bot's current crafting queue
     * @param bot Player bot
     * @return Vector of (itemId, quantity) pairs needed
     */
    std::vector<std::pair<uint32, uint32>> GetNeededMaterials(Player* bot);

    /**
     * @brief Check if gathered item is useful for bot's professions
     * @param bot Player bot
     * @param itemId Item gathered
     * @return true if item is needed for known recipes
     */
    bool IsItemNeededForCrafting(Player* bot, uint32 itemId);

    /**
     * @brief Prioritize gathering nodes based on material needs
     * @param bot Player bot
     * @param nodes Available gathering nodes
     * @return Sorted nodes (highest priority first)
     */
    std::vector<GatheringNode> PrioritizeNodesByNeeds(
        Player* bot,
        std::vector<GatheringNode> const& nodes);

    /**
     * @brief Trigger gathering session for specific material
     * @param bot Player bot
     * @param itemId Material to gather
     * @param quantity Amount needed
     * @return true if gathering session started
     */
    bool StartGatheringForMaterial(Player* bot, uint32 itemId, uint32 quantity);
};
```

**Integration Flow**:
```
ProfessionManager::QueueCraft(recipeId, qty)
  → GetMissingMaterials(recipe)
    → GatheringMaterialsBridge::StartGatheringForMaterial(itemId, qty)
      → GatheringManager::SetPrioritizeSkillUps(false)  // Prioritize materials over skill-ups
      → GatheringManager::ScanForNodes()
      → GatheringMaterialsBridge::PrioritizeNodesByNeeds(nodes)
        → GatheringManager::GatherFromNode(prioritized_node)
```

---

### 2.3 AuctionMaterialsBridge (NEEDS IMPLEMENTATION)

**Status**: DOES NOT EXIST
**Purpose**: Auto-buy materials from AH when gathering is not feasible

**Proposed API**:
```cpp
class AuctionMaterialsBridge
{
public:
    /**
     * @brief Buy materials needed for recipe from auction house
     * @param bot Player bot
     * @param recipe Recipe that needs materials
     * @return true if all materials purchased
     */
    bool BuyMaterialsForRecipe(Player* bot, RecipeInfo const& recipe);

    /**
     * @brief Check if buying material is cheaper than gathering
     * @param bot Player bot
     * @param itemId Material item ID
     * @param quantity Amount needed
     * @return true if AH price < (time to gather × gold/hour rate)
     */
    bool IsBuyingCheaperThanGathering(Player* bot, uint32 itemId, uint32 quantity);

    /**
     * @brief Get best source for material (gather, craft, or buy)
     * @param bot Player bot
     * @param itemId Material item ID
     * @return MaterialSource enum (GATHER, CRAFT, BUY_AH, VENDOR)
     */
    MaterialSource GetBestMaterialSource(Player* bot, uint32 itemId);
};
```

---

## 3. System Integration Workflow

### 3.1 Automated Profession Leveling with Market Integration

**User Goal**: "Level Blacksmithing to 450 with profit"

**System Flow**:
```
1. ProfessionManager::AutoLevelProfession(player, BLACKSMITHING)
   ↓
2. GetOptimalLevelingRecipe(player, BLACKSMITHING)
   → Returns: "Thorium Bar" (skill 275-300, orange recipe)
   ↓
3. Check materials: GetMissingMaterials(recipe)
   → Needs: 200× Thorium Ore
   ↓
4. GatheringMaterialsBridge::GetBestMaterialSource(THORIUM_ORE)
   → Option A: Gather in Un'Goro Crater (30 min, free)
   → Option B: Buy from AH (5000g, instant)
   → Decision: GATHER (bot has time, no gold)
   ↓
5. GatheringManager::StartGatheringForMaterial(THORIUM_ORE, 200)
   → PathToNode(Un'Goro mining nodes)
   → GatherFromNode() × 50 nodes
   → Loot 200× Thorium Ore
   ↓
6. ProfessionManager::CraftItem(recipe, 100)  // 200 ore → 100 bars
   → Skill 275 → 295 (+20 skill points)
   ↓
7. AuctionManager::PostAuction(THORIUM_BAR, stackSize=20, buyout=market_price)
   → Post 5 stacks of 20 bars
   → Total value: 8000g
   ↓
8. AuctionEventBus::OnAuctionSold(itemId=THORIUM_BAR, profit=7500g)
   → Update statistics
   → Reinvest 5000g for next recipe
```

**Profit**: 7500g gold - 30 min gathering time = ~15,000 gold/hour
**Skill Gain**: +20 skill points
**Efficiency**: Fully automated gathering → crafting → selling pipeline

---

### 3.2 On-Demand Crafting with Material Acquisition

**User Goal**: "Craft 10× Titansteel Bar for raid consumables"

**System Flow**:
```
1. ProfessionManager::CraftItem(TITANSTEEL_BAR_RECIPE, 10)
   ↓
2. GetMissingMaterials(recipe)
   → Needs: 10× Titanium Bar, 10× Eternal Fire, 10× Eternal Earth, 10× Eternal Shadow
   ↓
3. For each material:
   a) Check inventory: GetItemCount()
   b) If missing, check best source:
      - GatheringMaterialsBridge::GetBestMaterialSource()
      - AuctionMaterialsBridge::IsBuyingCheaperThanGathering()
   ↓
4. Acquisition strategy:
   - Titanium Bar: BUY from AH (rare spawn, not worth farming)
   - Eternal Fire: GATHER in Wintergrasp (common, fast)
   - Eternal Earth: CRAFT from 10× Crystallized Earth (check AH for crystals)
   - Eternal Shadow: BUY from AH (dungeon drop, can't farm solo)
   ↓
5. Execute acquisition:
   a) AuctionManager::BuyoutAuction(TITANIUM_BAR) × 10
   b) GatheringManager::StartGatheringForMaterial(ETERNAL_FIRE, 10)
   c) AuctionManager::BuyoutAuction(CRYSTALLIZED_EARTH) × 100
   d) ProfessionManager::CraftItem(ETERNAL_EARTH_RECIPE, 10)
   e) AuctionManager::BuyoutAuction(ETERNAL_SHADOW) × 10
   ↓
6. All materials acquired → CraftItem(TITANSTEEL_BAR, 10)
   ↓
7. Store in bank or use for raid
```

**Time**: 15 min (10 min gathering + 5 min AH purchases)
**Cost**: 2000g (AH purchases)
**Automation**: 100% hands-free material acquisition

---

## 4. Implementation Roadmap

### Phase 3A: Code Review & Cleanup (COMPLETED ✅)
- [x] Fix ProfessionManager.cpp orphaned null checks (100+)
- [x] Fix AuctionManager.cpp orphaned null checks (80+)
- [x] Verify GatheringManager.cpp is production-ready
- [x] Assess FarmingCoordinator integration state

### Phase 3B: Bridge Implementation (NEXT)
1. **Review ProfessionAuctionBridge** (1-2 hours)
   - Verify current functionality
   - Add auto-sell feature for crafted items
   - Test with all 14 professions

2. **Implement GatheringMaterialsBridge** (4-6 hours)
   - Create new file: `GatheringMaterialsBridge.h/cpp`
   - Implement GetNeededMaterials()
   - Implement IsItemNeededForCrafting()
   - Implement PrioritizeNodesByNeeds()
   - Add to CMakeLists.txt
   - Full integration testing

3. **Implement AuctionMaterialsBridge** (4-6 hours)
   - Create new file: `AuctionMaterialsBridge.h/cpp`
   - Implement BuyMaterialsForRecipe()
   - Implement IsBuyingCheaperThanGathering() with time-value analysis
   - Implement GetBestMaterialSource() decision algorithm
   - Add to CMakeLists.txt
   - Full integration testing

4. **Optional: Personal BankingManager** (8-10 hours)
   - Extends BehaviorManager
   - Auto-deposit gold/items
   - Auto-withdraw materials
   - Integration with all other managers

### Phase 3C: System Integration Testing (FINAL)
1. Test Scenario 1: Full profession leveling (Alchemy 1-450)
2. Test Scenario 2: Profit-driven crafting (flip Titansteel Bars)
3. Test Scenario 3: On-demand raid consumables
4. Test Scenario 4: Multi-bot coordinated farming + crafting
5. Performance testing: 1000 bots with all systems active
6. Memory profiling: Ensure no leaks in bridge classes

---

## 5. Architecture Recommendations

### 5.1 Should ProfessionManager Migrate to BehaviorManager?

**Current**: SINGLETON pattern
**Proposed**: BehaviorManager pattern (like GatheringManager and AuctionManager)

**Pros of Migration**:
- ✅ Architectural consistency across all managers
- ✅ Throttled updates (reduce CPU usage)
- ✅ Built-in performance metrics
- ✅ OnInitialize/OnUpdate/OnShutdown lifecycle
- ✅ Thread-safe by design

**Cons of Migration**:
- ❌ Large refactoring effort (4-8 hours)
- ❌ Risk of breaking existing integrations (FarmingCoordinator, ProfessionAuctionBridge)
- ❌ Need to migrate per-bot instance data to member variables

**Recommendation**: **DEFER** to Phase 4
- Current SINGLETON pattern works fine for now
- All new bridges can wrap the singleton
- Migrate later when we have full test coverage

---

### 5.2 Event-Driven vs Polling Integration

**Current Approach**: Polling (managers check state every update)

**Proposed Enhancement**: Event-driven (managers subscribe to events)

**Events to Add**:
```cpp
// ProfessionManager events
OnRecipeLearned(Player* bot, uint32 recipeId);
OnCraftingStarted(Player* bot, uint32 recipeId, uint32 quantity);
OnCraftingCompleted(Player* bot, uint32 itemId, uint32 quantity);
OnMaterialsNeeded(Player* bot, std::vector<std::pair<uint32, uint32>> materials);

// GatheringManager events
OnNodeDetected(Player* bot, GatheringNode const& node);
OnGatheringCompleted(Player* bot, uint32 itemId, uint32 quantity);
OnGatheringFailed(Player* bot, GatheringNode const& node);

// AuctionManager events (ALREADY EXISTS via AuctionEventBus)
OnAuctionPosted(Player* bot, uint32 auctionId, uint32 itemId);
OnAuctionSold(Player* bot, uint32 itemId, uint64 profit);
OnAuctionBought(Player* bot, uint32 itemId, uint64 cost);
```

**Benefits**:
- Reduces coupling between systems
- Enables reactive workflows (e.g., auto-sell when crafting completes)
- Better performance (no polling overhead)

**Recommendation**: Implement in Phase 3B

---

## 6. Deletion List

### 6.1 Advanced/EconomyManager.* (DUPLICATE - DELETE)

**Reason**: Broken duplicate of economy systems

**Files to Delete**:
- `/home/user/TrinityCore/src/modules/Playerbot/Advanced/EconomyManager.h`
- `/home/user/TrinityCore/src/modules/Playerbot/Advanced/EconomyManager.cpp`

**Issues**:
- 100+ syntax errors (stubs only)
- God class anti-pattern (auction + crafting + gathering + banking in one class)
- No thread safety
- Never implemented (only header declarations)

**Keep Instead**:
- `Economy/AuctionManager.*` (production-ready, BehaviorManager pattern)
- `Professions/ProfessionManager.*` (production-ready, SINGLETON pattern)
- `Professions/GatheringManager.*` (production-ready, BehaviorManager pattern)

---

## 7. Testing Checklist

### Unit Tests
- [ ] ProfessionManager::GetMissingMaterials() accuracy
- [ ] GatheringMaterialsBridge::PrioritizeNodesByNeeds() sorting
- [ ] AuctionMaterialsBridge::IsBuyingCheaperThanGathering() calculation
- [ ] BankingManager::AutoDepositJunk() item filtering

### Integration Tests
- [ ] Full profession leveling flow (1-450 all professions)
- [ ] Material acquisition decision tree (gather vs buy vs craft)
- [ ] Auction posting after crafting automation
- [ ] Cross-manager event propagation

### Performance Tests
- [ ] 1000 bots with all managers active (CPU < 50%)
- [ ] Bridge method latency (all < 1ms)
- [ ] Memory usage with 5000 bots (no leaks)

### Edge Case Tests
- [ ] No materials available (gathering or AH)
- [ ] Insufficient gold to buy materials
- [ ] Profession not learned but recipe queued
- [ ] Bank full when auto-deposit triggered

---

## 8. Summary

**Current State**:
- 3 production-ready managers (Profession, Gathering, Auction)
- 1 missing manager (personal Banking)
- 1 existing bridge (ProfessionAuctionBridge - needs review)
- 2 needed bridges (GatheringMaterials, AuctionMaterials)

**Next Steps**:
1. ✅ Fix compilation blockers (DONE - 180+ orphaned null checks removed)
2. ▶️ Review ProfessionAuctionBridge (NEXT)
3. ⏳ Implement GatheringMaterialsBridge
4. ⏳ Implement AuctionMaterialsBridge
5. ⏳ Delete Advanced/EconomyManager duplicate
6. ⏳ Optional: Implement personal BankingManager
7. ⏳ Full integration testing

**Estimated Completion**: 12-20 hours of development + testing

---

**Document Version**: 1.0
**Last Updated**: 2025-11-10
**Next Review**: After Phase 3B completion
