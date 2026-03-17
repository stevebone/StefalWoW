# PHASE 3 IMPLEMENTATION PLAN: GATHERING, FARMING & AUCTION HOUSE

**Version**: 1.0
**Date**: 2025-10-04
**Status**: Architecture Complete, Ready for Implementation
**Estimated Duration**: 8-10 weeks

## Overview

Phase 3 expands the profession system with three coordinated subsystems:
1. **GatheringAutomation**: Automated node detection and harvesting (Mining, Herbalism, Skinning, Fishing)
2. **FarmingCoordinator**: Skill-level synchronization with character progression
3. **ProfessionAuctionBridge**: Integration with existing auction house for material/craft sales

## COMPLIANCE WITH CLAUDE.MD

### ✅ File Modification Hierarchy

**Module-Only Implementation** (Tier 1 - PREFERRED):
- All Phase 3 code in `src/modules/Playerbot/Professions/`
- **Zero core modifications required**
- Uses only existing TrinityCore APIs:
  - `WorldObject::FindNearestGameObjectOfType(GAMEOBJECT_TYPE_GATHERING_NODE, range)`
  - `Player::HasSkill()`, `Player::GetSkillValue()`
  - `AuctionHouseMgr` (existing core system)
  - `Item`, `GameObject`, `Creature` manipulation APIs

**Integration Pattern**:
```cpp
// GOOD: Module-only approach
// All code in src/modules/Playerbot/Professions/GatheringAutomation.cpp
GameObject* node = player->FindNearestGameObjectOfType(GAMEOBJECT_TYPE_GATHERING_NODE, 40.0f);
if (node && CanGatherFromNode(player, node))
    GatherFromNode(player, node);
```

### ✅ Quality Requirements

- **Full Implementation**: No shortcuts, no TODOs, no placeholders
- **TrinityCore API Usage**: All GameObject/Player operations use existing APIs
- **Database Research**: GAMEOBJECT_TYPE_GATHERING_NODE (type 50) identified in SharedDefines.h:3149
- **Performance Target**: <0.1% CPU per bot, <10MB memory per bot
- **Comprehensive Error Handling**: All edge cases handled
- **Complete Testing**: Unit tests for each component

### ✅ Integration Points

**Phase 1 & 2 Integration**:
- `ProfessionManager::GetProfessionSkill()` - Get current skill level
- `ProfessionManager::HasProfession()` - Check profession requirements
- `ProfessionManager::GetOptimalLevelingRecipe()` - Get crafting recipes for leveling
- `ProfessionManager::HasMaterialsForRecipe()` - Check material availability

**Existing Playerbot Integration**:
- `Playerbot::AuctionHouse` - Existing auction house system (bridge pattern)
- Bot pathfinding system - Movement to gathering nodes
- Bot inventory management - Material storage

**TrinityCore Integration**:
- `GAMEOBJECT_TYPE_GATHERING_NODE` (50) - Node detection
- `AuctionHouseMgr` - Core auction system
- `SpellMgr` - Gathering spell casting

---

## ARCHITECTURE

### System Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    PHASE 3 PROFESSION SYSTEMS                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────┐    ┌──────────────────┐                 │
│  │ GatheringAuto    │◄───┤ FarmingCoord     │                 │
│  │                  │    │                  │                 │
│  │ - DetectNodes()  │    │ - NeedsFarming() │                 │
│  │ - GatherFrom()   │    │ - GetZone()      │                 │
│  │ - PathToNode()   │    │ - StartSession() │                 │
│  └────────┬─────────┘    └────────┬─────────┘                 │
│           │                       │                            │
│           │    ┌──────────────────▼─────────┐                 │
│           └────►ProfessionAuctionBridge     │                 │
│                │                            │                 │
│                │ - SellExcessMaterials()    │                 │
│                │ - SellCraftedItems()       │                 │
│                │ - BuyMaterialsForLeveling()│                 │
│                └────────────┬───────────────┘                 │
│                             │                                  │
├─────────────────────────────┼──────────────────────────────────┤
│         PHASE 1 & 2         │       EXISTING SYSTEMS           │
├─────────────────────────────┼──────────────────────────────────┤
│  ┌──────────────────┐       │    ┌──────────────────┐         │
│  │ ProfessionMgr    │       └────►AuctionHouse      │         │
│  │                  │            │                  │         │
│  │ - GetSkill()     │            │ - CreateAuction()│         │
│  │ - AutoLevel()    │            │ - BuyoutAuction()│         │
│  │ - CraftItem()    │            │ - GetMarketPrice()│        │
│  └──────────────────┘            └──────────────────┘         │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### File Structure

```
src/modules/Playerbot/Professions/
├── ProfessionManager.h                    [Phase 1 & 2 - COMPLETE]
├── ProfessionManager.cpp                  [Phase 1 & 2 - COMPLETE]
├── GatheringAutomation.h                  [Phase 3 - NEW]
├── GatheringAutomation.cpp                [Phase 3 - NEW]
├── FarmingCoordinator.h                   [Phase 3 - NEW]
├── FarmingCoordinator.cpp                 [Phase 3 - NEW]
├── ProfessionAuctionBridge.h              [Phase 3 - NEW]
└── ProfessionAuctionBridge.cpp            [Phase 3 - NEW]
```

---

## SUBSYSTEM 1: GATHERING AUTOMATION

### Purpose
Automated detection and harvesting of gathering profession nodes:
- **Mining**: Copper/Tin/Iron/Mithril veins (GAMEOBJECT_TYPE_GATHERING_NODE)
- **Herbalism**: Peacebloom/Silverleaf/Mageroyal nodes (GAMEOBJECT_TYPE_GATHERING_NODE)
- **Skinning**: Creature corpses (Creature::lootForBody flag)
- **Fishing**: Schools of fish (GAMEOBJECT_TYPE_FISHINGHOLE)

### API Usage

**Node Detection**:
```cpp
GameObject* GatheringAutomation::FindNearestMiningNode(Player* player, float range)
{
    return player->FindNearestGameObjectOfType(GAMEOBJECT_TYPE_GATHERING_NODE, range);
}
```

**Skill Validation**:
```cpp
bool GatheringAutomation::CanGatherFromNode(Player* player, GameObject* node)
{
    uint16 requiredSkill = GetRequiredSkillForNode(node);
    uint16 currentSkill = player->GetSkillValue(SKILL_MINING); // Or SKILL_HERBALISM
    return currentSkill >= requiredSkill;
}
```

**Gathering Spell Cast**:
```cpp
bool GatheringAutomation::CastGatheringSpell(Player* player, GameObject* node)
{
    uint32 spellId = GetGatheringSpellId(profession, skillLevel);
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Cast spell on node
    player->CastSpell(node, spellId, false);
    return true;
}
```

### Implementation Tasks

1. **GatheringAutomation.cpp Core** (Week 1-2)
   - [ ] `Initialize()` - Load gathering spell database from DB2
   - [ ] `ScanForNodes()` - Detect all gathering nodes in range
   - [ ] `FindNearestNode()` - Get closest harvestable node
   - [ ] `CanGatherFromNode()` - Validate skill + profession
   - [ ] `GetRequiredSkillForNode()` - Parse node skill requirement

2. **Node Detection Implementations** (Week 2-3)
   - [ ] `DetectMiningNodes()` - Find ore veins
   - [ ] `DetectHerbNodes()` - Find herb nodes
   - [ ] `DetectSkinnableCreatures()` - Find lootable corpses
   - [ ] `DetectFishingPools()` - Find fishing schools
   - [ ] `CreateNodeInfo()` - Convert GameObject to GatheringNodeInfo

3. **Gathering Actions** (Week 3-4)
   - [ ] `GatherFromNode()` - Main gathering workflow
   - [ ] `PathToNode()` - Navigate to node (use bot pathfinding)
   - [ ] `CastGatheringSpell()` - Cast mining/herbalism spell
   - [ ] `LootNode()` - Collect gathered items
   - [ ] `SkinCreature()` - Skin creature corpse

4. **Spell Database** (Week 4)
   - [ ] `LoadGatheringSpells()` - Load from SkillLineAbility DB2
   - [ ] `GetGatheringSpellId()` - Get spell for profession + skill level
   - [ ] Parse skill thresholds (1-75, 75-150, etc.)

5. **Performance & Testing** (Week 4)
   - [ ] Inventory space checking
   - [ ] Node caching (avoid re-scanning same nodes)
   - [ ] Statistics tracking
   - [ ] Unit tests for all methods

---

## SUBSYSTEM 2: FARMING COORDINATOR

### Purpose
Synchronize profession skill levels with character progression to prevent skill gaps.

**Algorithm**:
```
Target Skill = Character Level × 5
Skill Gap = Target Skill - Current Skill
If Skill Gap > 25:
    Start Farming Session
    Farm until Current Skill ≥ Target Skill
```

**Example**:
- Level 20 character with 50 Mining skill (target: 100)
- Skill gap: 50 → **TRIGGER FARMING**
- Farm Copper/Tin nodes for 15-30 minutes
- Stop when Mining skill reaches 100

### API Usage

**Skill Analysis**:
```cpp
bool FarmingCoordinator::NeedsFarming(Player* player, ProfessionType profession)
{
    uint16 currentSkill = ProfessionManager::instance()->GetProfessionSkill(player, profession);
    uint16 targetSkill = player->GetLevel() * 5;
    int32 skillGap = targetSkill - currentSkill;
    return skillGap > _profiles[player->GetGUID().GetCounter()].skillGapThreshold; // Default: 25
}
```

**Zone Selection**:
```cpp
FarmingZoneInfo const* FarmingCoordinator::GetOptimalFarmingZone(Player* player, ProfessionType profession)
{
    uint16 skillLevel = ProfessionManager::instance()->GetProfessionSkill(player, profession);

    // Find zones where nodes give skill-ups (skill in orange/yellow range)
    for (auto const& zone : _farmingZones[profession])
    {
        if (skillLevel >= zone.minSkillLevel && skillLevel <= zone.maxSkillLevel)
            return &zone;
    }
    return nullptr;
}
```

### Implementation Tasks

1. **FarmingCoordinator.cpp Core** (Week 5-6)
   - [ ] `Initialize()` - Load farming zone database
   - [ ] `NeedsFarming()` - Check if skill gap exceeds threshold
   - [ ] `GetSkillGap()` - Calculate (target - current) skill
   - [ ] `GetTargetSkillLevel()` - Calculate char_level × 5
   - [ ] `GetProfessionsNeedingFarm()` - Sort by skill gap priority

2. **Farming Session Management** (Week 6)
   - [ ] `StartFarmingSession()` - Initiate farming session
   - [ ] `StopFarmingSession()` - End session and return to original position
   - [ ] `UpdateFarmingSession()` - Track progress
   - [ ] `ShouldEndFarmingSession()` - Check if skill target reached
   - [ ] `CalculateFarmingDuration()` - Estimate time needed based on skill gap

3. **Zone Database** (Week 7)
   - [ ] `LoadFarmingZones()` - Load zone data from hardcoded tables
   - [ ] `InitializeMiningZones()` - Elwynn Forest (1-75), Westfall (75-150), etc.
   - [ ] `InitializeHerbalismZones()` - Tirisfal Glades, Durotar, etc.
   - [ ] `InitializeSkinningZones()` - Any zone with beasts
   - [ ] `GetOptimalFarmingZone()` - Select best zone for skill level

4. **Zone Scoring** (Week 7)
   - [ ] `CalculateZoneScore()` - Score = f(distance, node_density, safety)
   - [ ] Distance penalty: Closer zones preferred
   - [ ] Node density bonus: More nodes = higher score
   - [ ] Safety factor: Avoid contested PvP zones if low level

5. **Material Management** (Week 8)
   - [ ] `HasReachedStockpileTarget()` - Check if enough materials gathered
   - [ ] `GetMaterialCount()` - Count items in inventory
   - [ ] `GetNeededMaterials()` - Query auction house targets
   - [ ] Integration with ProfessionAuctionBridge

6. **Performance & Testing** (Week 8)
   - [ ] Travel system integration
   - [ ] Return to original position after farming
   - [ ] Statistics tracking
   - [ ] Unit tests for all methods

---

## SUBSYSTEM 3: PROFESSION AUCTION BRIDGE

### Purpose
Bridge profession system with existing `Playerbot::AuctionHouse` for material/craft sales.

**Design Pattern**: Bridge Pattern
- **Abstraction**: ProfessionAuctionBridge (profession logic)
- **Implementation**: Playerbot::AuctionHouse (auction operations)
- **Decoupling**: All auction operations delegated to existing system

### API Usage

**Selling Materials**:
```cpp
void ProfessionAuctionBridge::SellExcessMaterials(Player* player)
{
    auto items = GetProfessionItemsInInventory(player, true); // Materials only

    for (auto const& item : items)
    {
        if (ShouldSellMaterial(player, item.itemId, item.stackCount))
        {
            MaterialStockpileConfig const& config = _profiles[player->GetGUID().GetCounter()].materialConfigs[item.itemId];

            // Delegate to existing AuctionHouse
            uint32 price = _auctionHouse->CalculateOptimalListingPrice(player, item.itemId, config.auctionStackSize);
            _auctionHouse->CreateAuction(player, item.itemGuid, config.auctionStackSize, price * 0.95, price, DEFAULT_AUCTION_DURATION);
        }
    }
}
```

**Buying Materials**:
```cpp
void ProfessionAuctionBridge::BuyMaterialsForLeveling(Player* player, ProfessionType profession)
{
    // Get optimal leveling recipe from ProfessionManager
    RecipeInfo const* recipe = ProfessionManager::instance()->GetOptimalLevelingRecipe(player, profession);
    if (!recipe)
        return;

    // Get missing materials
    auto missingMaterials = ProfessionManager::instance()->GetMissingMaterials(player, *recipe);

    for (auto const& [itemId, quantity] : missingMaterials)
    {
        // Check auction house market price
        float marketPrice = _auctionHouse->GetMarketPrice(itemId, quantity);
        uint32 maxPricePerUnit = static_cast<uint32>(marketPrice * 1.1f); // Willing to pay 110% of market

        // Delegate to existing AuctionHouse
        if (_auctionHouse->IsPriceBelowMarket(itemId, maxPricePerUnit))
            PurchaseMaterial(player, itemId, quantity, maxPricePerUnit);
    }
}
```

### Implementation Tasks

1. **ProfessionAuctionBridge.cpp Core** (Week 8-9)
   - [ ] `Initialize()` - Get reference to `Playerbot::AuctionHouse::instance()`
   - [ ] `Update()` - Periodic auction checks
   - [ ] `SellExcessMaterials()` - Main selling workflow
   - [ ] `SellCraftedItems()` - Sell profitable crafts
   - [ ] `BuyMaterialsForLeveling()` - Purchase materials for skill-ups

2. **Material Selling Logic** (Week 9)
   - [ ] `ShouldSellMaterial()` - Check stockpile thresholds
   - [ ] `ListMaterialOnAuction()` - Delegate to AuctionHouse::CreateAuction
   - [ ] `GetOptimalMaterialPrice()` - Delegate to AuctionHouse::CalculateOptimalListingPrice
   - [ ] Stockpile management (keep minStackSize, sell excess)

3. **Crafted Item Selling Logic** (Week 9-10)
   - [ ] `ShouldSellCraftedItem()` - Check profit margin
   - [ ] `CalculateProfitMargin()` - (sale_price - material_cost) / material_cost
   - [ ] `ListCraftedItemOnAuction()` - Delegate to AuctionHouse::CreateAuction
   - [ ] `CalculateMaterialCost()` - Sum reagent market prices

4. **Material Purchasing Logic** (Week 10)
   - [ ] `GetNeededMaterialsForLeveling()` - Query ProfessionManager
   - [ ] `IsMaterialAvailableForPurchase()` - Delegate to AuctionHouse
   - [ ] `PurchaseMaterial()` - Delegate to AuctionHouse::BuyoutAuction
   - [ ] Budget management (respect auctionBudget limit)

5. **Stockpile Configuration** (Week 10)
   - [ ] `LoadDefaultStockpileConfigs()` - Initialize default material configs
   - [ ] `InitializeMiningMaterials()` - Copper Ore, Tin Ore, etc.
   - [ ] `InitializeHerbalismMaterials()` - Peacebloom, Silverleaf, etc.
   - [ ] `InitializeSkinningMaterials()` - Light Leather, Medium Leather, etc.
   - [ ] `InitializeCraftedItemConfigs()` - Common crafted items

6. **Inventory Scanning** (Week 10)
   - [ ] `GetProfessionItemsInInventory()` - Scan bags for materials/crafts
   - [ ] `IsProfessionMaterial()` - Identify gathering materials
   - [ ] `IsCraftedItem()` - Identify profession crafts
   - [ ] `GetCurrentStockpile()` - Count specific item

7. **Performance & Testing** (Week 10)
   - [ ] Integration with existing AuctionHouse
   - [ ] Statistics tracking
   - [ ] Unit tests for all methods

---

## CMAKE INTEGRATION

Update `src/modules/Playerbot/CMakeLists.txt`:

```cmake
# Phase 3 - Gathering, Farming, Auction Integration
Professions/GatheringAutomation.h
Professions/GatheringAutomation.cpp
Professions/FarmingCoordinator.h
Professions/FarmingCoordinator.cpp
Professions/ProfessionAuctionBridge.h
Professions/ProfessionAuctionBridge.cpp
```

---

## INTEGRATION WITH PHASE 1 & 2

### ProfessionManager Integration

**GatheringAutomation** uses:
- `ProfessionManager::HasProfession(player, ProfessionType::MINING)`
- `ProfessionManager::GetProfessionSkill(player, ProfessionType::HERBALISM)`

**FarmingCoordinator** uses:
- `ProfessionManager::GetProfessionSkill(player, profession)` - Get current skill
- `ProfessionManager::GetOptimalLevelingRecipe(player, profession)` - Get leveling recipe
- `ProfessionManager::HasMaterialsForRecipe(player, recipe)` - Check materials

**ProfessionAuctionBridge** uses:
- `ProfessionManager::GetOptimalLevelingRecipe(player, profession)` - Get crafting recipe
- `ProfessionManager::GetMissingMaterials(player, recipe)` - Get needed materials
- `ProfessionManager::CraftItem(player, recipe, 1)` - Craft items to sell

---

## DATABASE / DB2 DEPENDENCIES

### Already Researched

**Phase 2 (Recipe Database)**:
- ✅ `sSkillLineAbilityStore` (SkillLineAbility.db2)
- ✅ `sSpellReagentsStore` (SpellReagents.db2)
- ✅ `SKILL_ALCHEMY` (171), `SKILL_MINING` (186), etc. in SharedDefines.h

**Phase 3 (Gathering Nodes)**:
- ✅ `GAMEOBJECT_TYPE_GATHERING_NODE` (50) in SharedDefines.h:3149
- ✅ `WorldObject::FindNearestGameObjectOfType()` in Object.cpp:2160

### Additional Research Needed

**Gathering Spell IDs**:
- Mining: Need to find spell IDs for Mining (1-75, 75-150, etc.)
- Herbalism: Need to find spell IDs for Herbalism
- Skinning: Need to find spell IDs for Skinning
- **Source**: SkillLineAbility.db2 (already loaded in Phase 2)

**Node Entry IDs**:
- Copper Vein entry ID
- Tin Vein entry ID
- Peacebloom entry ID
- Silverleaf entry ID
- **Source**: GameObject database / gameobject_template

**Zone IDs**:
- Elwynn Forest, Westfall, Duskwood for Alliance
- Tirisfal Glades, Silverpine Forest for Horde
- **Source**: AreaTable.db2

---

## TESTING STRATEGY

### Unit Tests

**GatheringAutomation Tests**:
```cpp
TEST(GatheringAutomation, DetectMiningNodes)
{
    Player* testPlayer = CreateTestPlayer(CLASS_WARRIOR, RACE_HUMAN);
    ProfessionManager::instance()->LearnProfession(testPlayer, ProfessionType::MINING);

    // Spawn test mining node
    GameObject* copperVein = SpawnTestGameObject(COPPER_VEIN_ENTRY, testPlayer->GetPosition(), 10.0f);

    auto nodes = GatheringAutomation::instance()->ScanForNodes(testPlayer, 50.0f);

    EXPECT_EQ(nodes.size(), 1);
    EXPECT_EQ(nodes[0].profession, ProfessionType::MINING);
    EXPECT_TRUE(nodes[0].isActive);
}
```

**FarmingCoordinator Tests**:
```cpp
TEST(FarmingCoordinator, SkillGapCalculation)
{
    Player* testPlayer = CreateTestPlayer(CLASS_WARRIOR, RACE_HUMAN);
    testPlayer->SetLevel(20);
    ProfessionManager::instance()->LearnProfession(testPlayer, ProfessionType::MINING);
    testPlayer->SetSkill(SKILL_MINING, 1, 50, 300); // Skill: 50

    int32 skillGap = FarmingCoordinator::instance()->GetSkillGap(testPlayer, ProfessionType::MINING);

    EXPECT_EQ(skillGap, 50); // Target: 100, Current: 50
    EXPECT_TRUE(FarmingCoordinator::instance()->NeedsFarming(testPlayer, ProfessionType::MINING));
}
```

**ProfessionAuctionBridge Tests**:
```cpp
TEST(ProfessionAuctionBridge, MaterialSellingLogic)
{
    Player* testPlayer = CreateTestPlayer(CLASS_WARRIOR, RACE_HUMAN);

    // Give player 100 Copper Ore (itemId: 2770)
    Item* copperOre = CreateTestItem(2770, 100);
    testPlayer->AddItem(copperOre);

    MaterialStockpileConfig config;
    config.itemId = 2770;
    config.minStackSize = 20;
    config.maxStackSize = 80;

    ProfessionAuctionBridge::instance()->SetMaterialStockpile(testPlayer->GetGUID().GetCounter(), 2770, config);

    // Should sell excess (100 > 80)
    EXPECT_TRUE(ProfessionAuctionBridge::instance()->ShouldSellMaterial(testPlayer, 2770, 100));
}
```

### Integration Tests

1. **End-to-End Gathering**:
   - Spawn bot with Mining profession
   - Spawn copper vein nearby
   - Verify bot detects, paths to, and harvests node
   - Verify loot collection

2. **Farming Session**:
   - Create level 20 bot with skill 50
   - Start farming session
   - Verify bot travels to farming zone
   - Verify bot gathers nodes
   - Verify session ends when skill reaches 100

3. **Auction Integration**:
   - Bot gathers 100 Copper Ore
   - Verify bot lists 80 on auction house (keeps 20)
   - Verify correct pricing (market analysis)
   - Verify auction creation in database

---

## PERFORMANCE TARGETS

### Memory Usage
- **Per-Bot Target**: <10MB additional memory
- **Gathering Node Cache**: 100 nodes × 200 bytes = 20KB
- **Farming Session**: 500 bytes per active session
- **Auction Profile**: 2KB per bot

### CPU Usage
- **Node Scanning**: <0.01% CPU (3-second interval)
- **Farming Checks**: <0.005% CPU (10-second interval)
- **Auction Checks**: <0.005% CPU (10-minute interval)
- **Total Phase 3**: <0.02% CPU per bot

### Database Queries
- **Node Detection**: 0 queries (in-memory GameObject scan)
- **Skill Checks**: Cached in Player object
- **Auction Queries**: Delegated to existing AuctionHouse (already optimized)

---

## RISK MITIGATION

### Identified Risks

1. **Pathfinding to Nodes**: Bot may get stuck on terrain
   - **Mitigation**: Use existing bot pathfinding system, add stuck detection

2. **Auction House Spam**: Bots may flood AH with listings
   - **Mitigation**: `maxActiveAuctions` limit (default: 20), rate limiting

3. **Skill-up RNG**: Random skill-ups may cause farming sessions to run long
   - **Mitigation**: `maxFarmingDuration` timeout (default: 30 minutes)

4. **Zone Selection**: Bots may choose inappropriate zones (PvP, high-level)
   - **Mitigation**: Zone scoring algorithm prioritizes safety and distance

5. **Material Prices**: Market volatility may cause losses
   - **Mitigation**: Delegate to existing AuctionHouse price analysis

---

## ACCEPTANCE CRITERIA

### Phase 3 Complete When:

✅ **Gathering Automation**:
- [ ] Bots detect mining/herb nodes within 40 yards
- [ ] Bots path to and harvest nodes successfully
- [ ] Bots loot gathered items into inventory
- [ ] Bots skip nodes below skill level
- [ ] Statistics track nodes gathered, items looted

✅ **Farming Coordinator**:
- [ ] Bots detect skill gap (target - current > 25)
- [ ] Bots start farming session automatically
- [ ] Bots travel to appropriate farming zone
- [ ] Bots farm until skill target reached
- [ ] Bots return to original position after farming

✅ **Profession Auction Bridge**:
- [ ] Bots sell excess materials on auction house
- [ ] Bots list crafted items at profitable prices
- [ ] Bots buy materials for leveling when needed
- [ ] Bots respect stockpile thresholds
- [ ] Integration with existing AuctionHouse works correctly

✅ **Quality**:
- [ ] Zero core modifications (module-only)
- [ ] All TrinityCore API usage validated
- [ ] Build succeeds with zero errors
- [ ] Unit tests pass (>90% coverage)
- [ ] Performance targets met (<0.02% CPU per bot)
- [ ] Memory usage within budget (<10MB per bot)

✅ **Documentation**:
- [ ] All public methods documented
- [ ] Integration points clearly marked
- [ ] Configuration options explained
- [ ] User guide for enabling/configuring

---

## DEVELOPMENT TIMELINE

### Week 1-2: GatheringAutomation Core
- Implement node detection (mining, herbalism)
- Implement gathering spell casting
- Implement loot collection
- **Deliverable**: Bots can harvest nodes

### Week 3-4: GatheringAutomation Polish
- Add skinning support
- Add fishing support
- Implement inventory management
- Add statistics tracking
- **Deliverable**: Complete gathering system

### Week 5-6: FarmingCoordinator Core
- Implement skill analysis
- Implement farming session management
- Implement zone selection
- **Deliverable**: Bots can start farming sessions

### Week 7-8: FarmingCoordinator Polish
- Load farming zone database
- Implement zone scoring
- Add material management
- Add statistics tracking
- **Deliverable**: Complete farming system

### Week 8-9: ProfessionAuctionBridge Core
- Integrate with existing AuctionHouse
- Implement material selling
- Implement crafted item selling
- **Deliverable**: Bots can sell on AH

### Week 9-10: ProfessionAuctionBridge Polish
- Implement material purchasing
- Load stockpile configurations
- Add profit margin calculation
- Add statistics tracking
- **Deliverable**: Complete auction integration

### Week 10: Integration & Testing
- End-to-end integration tests
- Performance profiling
- Bug fixes
- Documentation
- **Deliverable**: Phase 3 complete and ready to commit

---

## COMMIT MESSAGE TEMPLATE

```
[PlayerBot] PHASE 3 COMPLETE: Gathering, Farming & Auction House Integration

**Gathering Automation**:
- Automated node detection for Mining, Herbalism, Skinning, Fishing
- GAMEOBJECT_TYPE_GATHERING_NODE (type 50) detection within 40 yards
- Pathfinding and spell casting for harvesting
- Inventory management and loot collection
- Statistics: {nodes_gathered} nodes, {items_looted} items collected

**Farming Coordinator**:
- Skill-level synchronization: Target = Character Level × 5
- Automatic farming sessions when skill gap > 25
- Zone selection based on skill level and distance
- Session management with timeouts and return-to-position
- Statistics: {sessions} farming sessions, {skill_points} skill gained

**Profession Auction Bridge**:
- Integration with existing Playerbot::AuctionHouse (bridge pattern)
- Automated selling of excess materials (stockpile thresholds)
- Automated selling of crafted items (profit margin analysis)
- Automated purchasing of materials for leveling
- Statistics: {materials_sold} materials, {crafts_sold} crafts, {gold_earned} gold earned

**Architecture**:
- Module-only implementation (zero core modifications)
- TrinityCore API compliance (FindNearestGameObjectOfType, HasSkill, etc.)
- Integration with Phase 1 & 2 ProfessionManager
- Performance: <0.02% CPU per bot, <10MB memory per bot

**Files Added**:
- src/modules/Playerbot/Professions/GatheringAutomation.h
- src/modules/Playerbot/Professions/GatheringAutomation.cpp
- src/modules/Playerbot/Professions/FarmingCoordinator.h
- src/modules/Playerbot/Professions/FarmingCoordinator.cpp
- src/modules/Playerbot/Professions/ProfessionAuctionBridge.h
- src/modules/Playerbot/Professions/ProfessionAuctionBridge.cpp

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>
```

---

## NEXT STEPS

1. **Get User Approval**: Present this plan for explicit GO/NO-GO decision
2. **Start Week 1**: Begin GatheringAutomation.cpp implementation
3. **Iterative Development**: Implement, test, commit each subsystem
4. **Final Integration**: Week 10 integration and performance validation
5. **Commit Phase 3**: Single comprehensive commit with all features

---

**END OF PHASE 3 IMPLEMENTATION PLAN**
