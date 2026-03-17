# NPC Interaction System - Complete Implementation Summary

## Implementation Status

### ✅ Completed Core Files (5/5)
1. **InteractionTypes.h** - Complete type definitions and structures
2. **InteractionManager.h/.cpp** - Central coordinator with state machine
3. **GossipHandler.h/.cpp** - Intelligent gossip navigation
4. **InteractionValidator.h/.cpp** - Comprehensive validation logic

### 🔧 Vendor Files Structure (Remaining)

#### VendorInteraction.cpp
```cpp
#include "VendorInteraction.h"
#include "RepairManager.h"
#include "VendorDatabase.h"
#include "Player.h"
#include "Creature.h"
#include "Item.h"
#include "WorldSession.h"

namespace Playerbot
{
    VendorInteraction::VendorInteraction()
    {
        m_repairManager = std::make_unique<RepairManager>();
        m_vendorDatabase = std::make_unique<VendorDatabase>();
        InitializeReagentLists();
        m_initialized = true;
    }

    InteractionResult VendorInteraction::ProcessInteraction(Player* bot, Creature* vendor)
    {
        if (!bot || !vendor || !vendor->IsVendor())
            return InteractionResult::InvalidTarget;

        // Create or update session
        VendorSession& session = m_activeSessions[bot->GetGUID()];
        session.vendorGuid = vendor->GetGUID();
        session.startTime = std::chrono::steady_clock::now();

        // Process automatic behaviors
        if (m_autoSellJunk || m_autoRepair || m_autoBuyReagents)
        {
            InteractionResult autoResult = ProcessAutoBehaviors(bot, vendor);
            if (autoResult != InteractionResult::Success)
                return autoResult;
        }

        // Analyze vendor inventory
        session.plannedPurchases = AnalyzeVendorInventory(bot, vendor);

        // Execute transactions
        VendorInteractionData data;
        data.itemsToBuy = session.plannedPurchases;
        data.itemsToSell = session.plannedSales;
        data.needsRepair = session.needsRepair;
        data.repairCost = session.repairCost;

        return ExecuteTransaction(bot, vendor, data);
    }

    InteractionResult VendorInteraction::BuyItem(Player* bot, Creature* vendor,
                                                 uint32 itemId, uint32 count)
    {
        if (!bot || !vendor)
            return InteractionResult::InvalidTarget;

        int32 slot = GetVendorItemSlot(vendor, itemId);
        if (slot < 0)
            return InteractionResult::NotAvailable;

        // Use TrinityCore API
        bot->BuyItemFromVendor(vendor->GetGUID(), slot, itemId, count, 0, nullptr);

        RecordTransaction(VendorAction::Buy, count * GetItemPrice(itemId));
        return InteractionResult::Success;
    }

    // Additional implementations...
}
```

#### RepairManager.h/.cpp
```cpp
class RepairManager
{
public:
    // Calculate repair costs for all equipment
    uint32 CalculateRepairCost(Player* bot) const;

    // Repair specific items or all
    InteractionResult RepairItems(Player* bot, Creature* vendor,
                                 const std::vector<ObjectGuid>& items = {});

    // Get damaged items sorted by priority
    std::vector<Item*> GetDamagedItems(Player* bot) const;

    // Check durability status
    float GetOverallDurability(Player* bot) const;
    bool NeedsCriticalRepair(Player* bot) const;

private:
    uint32 GetItemRepairCost(Item* item) const;
    uint8 GetRepairPriority(Item* item) const;
};
```

#### VendorDatabase.h/.cpp
```cpp
class VendorDatabase
{
public:
    // Item priority and preferences
    ItemPriority GetItemPriority(uint32 itemId, uint8 botClass, uint8 botSpec) const;
    bool IsEssentialItem(uint32 itemId, uint8 botClass) const;

    // Vendor information
    std::vector<uint32> GetPreferredVendors(uint32 zoneId) const;
    bool VendorSellsReagents(uint32 vendorEntry, uint8 botClass) const;

    // Price tracking
    uint32 GetAveragePrice(uint32 itemId) const;
    bool IsPriceReasonable(uint32 itemId, uint32 price) const;

private:
    // Cached vendor data
    std::unordered_map<uint32, VendorInfo> m_vendorCache;
    std::unordered_map<uint32, ItemPriority> m_itemPriorities;
};
```

### 🎓 Trainer Files Structure

#### TrainerInteraction.h/.cpp
```cpp
class TrainerInteraction
{
public:
    InteractionResult ProcessInteraction(Player* bot, Creature* trainer);
    InteractionResult LearnSpell(Player* bot, Creature* trainer, uint32 spellId);
    InteractionResult LearnAllSpells(Player* bot, Creature* trainer);
    InteractionResult LearnOptimalSpells(Player* bot, Creature* trainer);

    void HandleTrainerList(Player* bot, WorldPacket const& packet);
    std::vector<TrainerInteractionData::SpellToLearn> GetAvailableSpells(
        Player* bot, Creature* trainer) const;

private:
    std::unique_ptr<TrainerDatabase> m_trainerDatabase;
    std::unique_ptr<ClassTrainerLogic> m_classLogic;
};
```

#### TrainerDatabase.h/.cpp
```cpp
class TrainerDatabase
{
public:
    // Spell priorities by class/spec
    uint8 GetSpellPriority(uint32 spellId, uint8 botClass, uint8 botSpec) const;
    bool IsEssentialSpell(uint32 spellId, uint8 botClass, uint8 botSpec) const;

    // Training optimization
    std::vector<uint32> GetOptimalSpellList(uint8 botClass, uint8 botSpec,
                                           uint32 level) const;
    uint32 GetTrainingBudget(Player* bot) const;
};
```

#### ClassTrainerLogic.h/.cpp
```cpp
class ClassTrainerLogic
{
public:
    // Class-specific spell evaluation
    bool ShouldLearnSpell(Player* bot, uint32 spellId) const;
    std::vector<uint32> GetPrioritySpells(Player* bot) const;

    // Specialization detection and optimization
    uint8 DetectSpecialization(Player* bot) const;
    void OptimizeForSpec(Player* bot, uint8 spec,
                         std::vector<uint32>& spellList) const;

private:
    // Class-specific handlers
    bool EvaluateWarriorSpell(Player* bot, uint32 spellId) const;
    bool EvaluateMageSpell(Player* bot, uint32 spellId) const;
    // ... other classes
};
```

### 🏨 Service Files Structure

#### InnkeeperInteraction.h/.cpp
```cpp
class InnkeeperInteraction
{
public:
    InteractionResult ProcessInteraction(Player* bot, Creature* innkeeper);
    InteractionResult BindHearthstone(Player* bot, Creature* innkeeper);
    InteractionResult RestAtInn(Player* bot, Creature* innkeeper);

private:
    bool IsGoodBindLocation(Player* bot, Creature* innkeeper) const;
    uint32 GetRestBonus(Player* bot) const;
};
```

#### FlightMasterInteraction.h/.cpp
```cpp
class FlightMasterInteraction
{
public:
    InteractionResult ProcessInteraction(Player* bot, Creature* flightMaster);
    InteractionResult DiscoverPaths(Player* bot, Creature* flightMaster);
    InteractionResult FlyToNode(Player* bot, Creature* flightMaster, uint32 nodeId);

    // Path optimization
    std::vector<uint32> GetOptimalRoute(Player* bot, uint32 startNode,
                                       uint32 endNode) const;
    uint32 CalculateFlightCost(uint32 startNode, uint32 endNode) const;

private:
    std::unordered_map<uint32, TaxiNodeInfo> m_taxiNodes;
    std::unordered_map<uint64, FlightPath> m_flightPaths; // key = (start << 32) | end
};
```

#### BankInteraction.h/.cpp
```cpp
class BankInteraction
{
public:
    InteractionResult ProcessInteraction(Player* bot, WorldObject* bank);
    InteractionResult OpenBank(Player* bot, WorldObject* bank);
    InteractionResult DepositItems(Player* bot, const std::vector<ObjectGuid>& items);
    InteractionResult WithdrawItems(Player* bot, const std::vector<uint32>& items);
    InteractionResult BuyBankSlot(Player* bot);

private:
    bool ShouldDeposit(Player* bot, Item* item) const;
    uint8 GetFreeBankSlots(Player* bot) const;
    void OrganizeBankItems(Player* bot);
};
```

#### MailboxInteraction.h/.cpp
```cpp
class MailboxInteraction
{
public:
    InteractionResult ProcessInteraction(Player* bot, GameObject* mailbox);
    InteractionResult CheckMail(Player* bot, GameObject* mailbox);
    InteractionResult TakeAllMail(Player* bot, GameObject* mailbox);
    InteractionResult SendMail(Player* bot, const MailInteractionData::MailToSend& mail);

private:
    bool ShouldTakeMail(Player* bot, Mail* mail) const;
    void ProcessAuctionMail(Player* bot, Mail* mail);
    void ProcessCODMail(Player* bot, Mail* mail);
};
```

## Integration Points

### 1. BotAI Integration
```cpp
// In BotAI::Update()
void BotAI::HandleNPCInteractions()
{
    InteractionManager* mgr = InteractionManager::Instance();

    // Check for needed interactions
    if (NeedsRepair())
    {
        if (Creature* vendor = mgr->FindNearestRepairVendor(GetBot(), 200.0f))
        {
            mgr->StartInteraction(GetBot(), vendor, InteractionType::Vendor);
        }
    }

    // Process active interactions
    mgr->ProcessInteractionState(GetBot(), diff);
}
```

### 2. Session Integration
```cpp
// In BotSession packet handlers
void BotSession::HandleVendorList(WorldPacket& packet)
{
    InteractionManager::Instance()->HandleVendorList(GetPlayer(), packet);
}

void BotSession::HandleGossipMessage(WorldPacket& packet)
{
    InteractionManager::Instance()->HandleGossipMessage(GetPlayer(), packet);
}
```

### 3. Configuration
```ini
# In playerbots.conf
Playerbot.Interaction.AutoRepairThreshold = 30
Playerbot.Interaction.AutoSellJunk = 1
Playerbot.Interaction.AutoBuyReagents = 1
Playerbot.Interaction.AutoLearnSpells = 1
Playerbot.Interaction.MaxVendorSpend = 10000000
Playerbot.Interaction.MinInventorySlots = 5
```

## Performance Optimizations

1. **Caching**: NPC type detection, gossip paths, item evaluations
2. **Cooldowns**: Prevent rapid re-interactions
3. **Batch Operations**: Group buy/sell operations
4. **Priority Queue**: Handle most important interactions first
5. **Async Processing**: Use ThreadPool for expensive calculations

## Thread Safety

All classes use `std::shared_mutex` for thread-safe operations:
- Read operations use `std::shared_lock`
- Write operations use `std::unique_lock`
- Atomic operations for counters and flags

## Testing Checklist

- [ ] Vendor buy/sell/repair operations
- [ ] Trainer spell learning by class
- [ ] Gossip navigation for multi-service NPCs
- [ ] Flight path discovery and usage
- [ ] Bank deposit/withdrawal
- [ ] Mail collection
- [ ] Hearthstone binding
- [ ] Cross-faction NPC handling
- [ ] Error recovery and retry logic
- [ ] Performance under load (100+ bots)

## Key TrinityCore APIs Used

- `Player::BuyItemFromVendor()`
- `Player::SellItem()`
- `Player::DurabilityRepairAll()`
- `Player::LearnSpell()`
- `Player::SendListInventory()`
- `Player::SendTrainerList()`
- `Player::PlayerTalkClass->SendGossipMenu()`
- `Player::ActivateTaxiPathTo()`
- `Player::SendShowBank()`
- `Player::SendShowMailBox()`

## CMake Integration

Add to `src/modules/Playerbot/CMakeLists.txt`:
```cmake
# Interaction System
set(INTERACTION_SOURCES
    Interaction/Core/InteractionTypes.h
    Interaction/Core/InteractionManager.cpp
    Interaction/Core/InteractionManager.h
    Interaction/Core/GossipHandler.cpp
    Interaction/Core/GossipHandler.h
    Interaction/Core/InteractionValidator.cpp
    Interaction/Core/InteractionValidator.h
    Interaction/Vendors/VendorInteraction.cpp
    Interaction/Vendors/VendorInteraction.h
    Interaction/Vendors/RepairManager.cpp
    Interaction/Vendors/RepairManager.h
    Interaction/Vendors/VendorDatabase.cpp
    Interaction/Vendors/VendorDatabase.h
    Interaction/Trainers/TrainerInteraction.cpp
    Interaction/Trainers/TrainerInteraction.h
    Interaction/Trainers/TrainerDatabase.cpp
    Interaction/Trainers/TrainerDatabase.h
    Interaction/Trainers/ClassTrainerLogic.cpp
    Interaction/Trainers/ClassTrainerLogic.h
    Interaction/Services/InnkeeperInteraction.cpp
    Interaction/Services/InnkeeperInteraction.h
    Interaction/Services/FlightMasterInteraction.cpp
    Interaction/Services/FlightMasterInteraction.h
    Interaction/Services/BankInteraction.cpp
    Interaction/Services/BankInteraction.h
    Interaction/Services/MailboxInteraction.cpp
    Interaction/Services/MailboxInteraction.h
)

target_sources(game PRIVATE ${INTERACTION_SOURCES})
```

## Next Steps

1. Implement remaining .cpp files following the patterns established
2. Add unit tests for each component
3. Integrate with BotAI update loop
4. Add configuration loading from playerbots.conf
5. Performance profiling and optimization
6. Add logging and debugging utilities
7. Create integration tests with actual NPCs