# Quest Automation System - Phase 3 Sprint 3D
## Automated Quest Completion and Chain Management

## Overview

This document details the implementation of intelligent quest automation for bots, including quest pickup, objective tracking, turn-in logic, and quest chain management. The system enables bots to autonomously progress through quest content.

## Core Quest System

### Quest Manager Framework

```cpp
// src/modules/Playerbot/AI/Quest/QuestManager.h

class BotQuestManager {
public:
    struct QuestInfo {
        uint32 questId;
        Quest const* quest;
        QuestStatus status;
        std::vector<QuestObjective> objectives;
        uint32 timeStarted;
        uint8 priority;
        bool isDaily;
        bool isWeekly;
        bool isRepeatable;
    };
    
    struct QuestChain {
        std::vector<uint32> questIds;
        uint32 currentIndex;
        std::string name;
        uint32 finalReward;
    };
    
    BotQuestManager(Player* bot) : _bot(bot) {
        LoadQuestDatabase();
    }
    
    // Quest discovery
    std::vector<Quest const*> FindAvailableQuests();
    std::vector<Quest const*> FindCompletableQuests();
    Quest const* SelectBestQuest();
    
    // Quest acceptance
    bool ShouldAcceptQuest(Quest const* quest);
    void AcceptQuest(Object* questGiver, Quest const* quest);
    void AcceptAllAvailableQuests(Object* questGiver);
    
    // Quest objectives
    void UpdateQuestProgress();
    bool IsObjectiveComplete(uint32 questId, uint8 objectiveIndex);
    Position GetObjectiveLocation(QuestObjective const& objective);
    
    // Quest turn-in
    void TurnInQuest(Object* questGiver, uint32 questId);
    void SelectQuestReward(Quest const* quest);
    
    // Quest chain management
    void StartQuestChain(const std::string& chainName);
    void ProgressQuestChain();
    bool IsChainComplete(const std::string& chainName);
    
private:
    Player* _bot;
    std::vector<QuestInfo> _activeQuests;
    std::map<std::string, QuestChain> _questChains;
    phmap::flat_hash_set<uint32> _completedQuests;
    
    // Quest evaluation
    float EvaluateQuestValue(Quest const* quest) {
        float value = 0.0f;
        
        // Experience value
        if (_bot->getLevel() < MAX_LEVEL) {
            value += quest->GetXPReward(_bot) / 1000.0f;
        }
        
        // Gold value
        value += quest->GetRewMoney() / 10000.0f;
        
        // Item rewards
        for (uint8 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i) {
            if (uint32 itemId = quest->RewardChoiceItemId[i]) {
                if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId)) {
                    if (IsItemUpgrade(proto)) {
                        value += 50.0f;
                    }
                }
            }
        }
        
        // Reputation value
        for (uint8 i = 0; i < QUEST_REPUTATIONS_COUNT; ++i) {
            if (quest->RewardFactionId[i]) {
                value += quest->RewardFactionValue[i] / 100.0f;
            }
        }
        
        // Quest chain value
        if (IsPartOfChain(quest->GetQuestId())) {
            value += 20.0f;
        }
        
        // Distance penalty
        float distance = GetDistanceToQuestObjective(quest);
        value -= distance / 1000.0f;
        
        return value;
    }
    
    bool IsQuestTooHard(Quest const* quest) {
        int32 levelDiff = quest->GetQuestLevel() - _bot->getLevel();
        
        // Skip elite quests if solo
        if (quest->GetType() == QUEST_TYPE_ELITE && !_bot->GetGroup())
            return true;
        
        // Skip quests too high level
        if (levelDiff > 3)
            return true;
        
        // Skip raid quests without raid
        if (quest->GetType() == QUEST_TYPE_RAID && 
            (!_bot->GetGroup() || !_bot->GetGroup()->isRaidGroup()))
            return true;
        
        return false;
    }
};
```

### Quest Objective Handler

```cpp
// src/modules/Playerbot/AI/Quest/QuestObjectiveHandler.h

class QuestObjectiveHandler {
public:
    enum ObjectiveStrategy {
        STRATEGY_KILL,
        STRATEGY_COLLECT,
        STRATEGY_INTERACT,
        STRATEGY_ESCORT,
        STRATEGY_DELIVER,
        STRATEGY_EXPLORE
    };
    
    QuestObjectiveHandler(Player* bot) : _bot(bot) {}
    
    // Objective execution
    void ExecuteObjective(QuestObjective const& objective);
    ObjectiveStrategy DetermineStrategy(QuestObjective const& objective);
    
    // Kill objectives
    void HandleKillObjective(QuestObjective const& objective);
    Creature* FindQuestMob(uint32 entry);
    bool ShouldAttackForQuest(Creature* creature);
    
    // Collection objectives
    void HandleCollectObjective(QuestObjective const& objective);
    GameObject* FindQuestObject(uint32 entry);
    bool CanLootQuestItem(Creature* creature, uint32 itemId);
    
    // Interaction objectives
    void HandleInteractObjective(QuestObjective const& objective);
    void HandleEscortObjective(QuestObjective const& objective);
    
    // Exploration objectives
    void HandleExploreObjective(QuestObjective const& objective);
    bool IsAreaExplored(uint32 areaId);
    
private:
    Player* _bot;
    
    void HandleKillObjective(QuestObjective const& objective) {
        uint32 killCount = objective.Amount;
        uint32 currentCount = _bot->GetQuestObjectiveData(objective);
        
        if (currentCount >= killCount)
            return;
        
        // Find target creatures
        std::list<Creature*> creatures;
        _bot->GetCreatureListWithEntryInGrid(creatures, objective.ObjectID, 100.0f);
        
        // Sort by distance
        creatures.sort([this](Creature* a, Creature* b) {
            return _bot->GetDistance(a) < _bot->GetDistance(b);
        });
        
        // Attack closest
        for (Creature* creature : creatures) {
            if (creature->IsAlive() && !creature->IsInCombat()) {
                _bot->AI()->AttackStart(creature);
                return;
            }
        }
        
        // Move to spawn area if no targets
        if (Position pos = GetObjectiveSpawnLocation(objective)) {
            MoveTo(pos);
        }
    }
    
    void HandleCollectObjective(QuestObjective const& objective) {
        uint32 itemId = objective.ObjectID;
        uint32 required = objective.Amount;
        uint32 current = _bot->GetItemCount(itemId);
        
        if (current >= required)
            return;
        
        // Check if item drops from creatures
        if (auto droppers = GetItemDroppers(itemId)) {
            for (uint32 entry : droppers) {
                if (Creature* creature = FindQuestMob(entry)) {
                    _bot->AI()->AttackStart(creature);
                    return;
                }
            }
        }
        
        // Check for quest objects
        std::list<GameObject*> objects;
        _bot->GetGameObjectListWithEntryInGrid(objects, 
            GetQuestObjectForItem(itemId), 50.0f);
        
        for (GameObject* obj : objects) {
            if (obj->IsWithinDistInMap(_bot, INTERACTION_DISTANCE)) {
                obj->Use(_bot);
                return;
            } else {
                MoveTo(obj->GetPosition());
                return;
            }
        }
    }
    
    void HandleEscortObjective(QuestObjective const& objective) {
        // Find escort NPC
        Creature* escortNPC = nullptr;
        std::list<Creature*> creatures;
        _bot->GetCreatureListWithEntryInGrid(creatures, objective.ObjectID, 30.0f);
        
        for (Creature* creature : creatures) {
            if (creature->AI()->HasEscortState(STATE_ESCORT_ESCORTING)) {
                escortNPC = creature;
                break;
            }
        }
        
        if (!escortNPC) {
            // Start escort
            for (Creature* creature : creatures) {
                if (creature->AI()->HasEscortState(STATE_ESCORT_NONE)) {
                    creature->AI()->Start(false, true, _bot->GetGUID());
                    return;
                }
            }
        } else {
            // Follow escort NPC
            float distance = _bot->GetDistance(escortNPC);
            if (distance > 10.0f) {
                MoveTo(escortNPC->GetPosition());
            }
            
            // Defend NPC if attacked
            if (escortNPC->IsInCombat()) {
                if (Unit* attacker = escortNPC->GetVictim()) {
                    _bot->AI()->AttackStart(attacker);
                }
            }
        }
    }
};
```

## Quest Chain Management

### Quest Chain System

```cpp
// src/modules/Playerbot/AI/Quest/QuestChainManager.h

class QuestChainManager {
public:
    struct ChainNode {
        uint32 questId;
        std::vector<uint32> prerequisites;
        std::vector<uint32> unlocks;
        bool isOptional;
        uint32 minimumLevel;
    };
    
    struct QuestHub {
        uint32 areaId;
        Position center;
        std::vector<uint32> quests;
        uint8 levelRange[2];
        std::string name;
    };
    
    QuestChainManager() {
        LoadQuestChains();
        LoadQuestHubs();
    }
    
    // Chain management
    std::vector<uint32> GetChainQuests(const std::string& chainName);
    uint32 GetNextQuestInChain(uint32 currentQuestId);
    bool IsChainAvailable(const std::string& chainName, Player* bot);
    
    // Hub management
    QuestHub* GetNearestQuestHub(Player* bot);
    QuestHub* GetAppropriateHub(uint8 level);
    void TravelToHub(Player* bot, const QuestHub& hub);
    
    // Zone progression
    std::vector<uint32> GetZoneStoryQuests(uint32 zoneId);
    float GetZoneCompletion(uint32 zoneId, Player* bot);
    
private:
    std::map<std::string, std::vector<ChainNode>> _questChains;
    std::vector<QuestHub> _questHubs;
    
    void LoadQuestChains() {
        // Example: Redridge Mountains chain
        std::vector<ChainNode> redridgeChain;
        
        // They've Wised Up
        redridgeChain.push_back({26503, {}, {26505}, false, 15});
        // Yowler Must Die
        redridgeChain.push_back({26505, {26503}, {26506}, false, 15});
        // Franks and Beans
        redridgeChain.push_back({26506, {26505}, {26511}, false, 15});
        
        _questChains["redridge_story"] = redridgeChain;
        
        // Load more chains...
    }
    
    void LoadQuestHubs() {
        // Goldshire
        QuestHub goldshire;
        goldshire.areaId = 87;
        goldshire.center = Position(-9465.0f, 73.0f, 56.0f, 0.0f);
        goldshire.levelRange[0] = 5;
        goldshire.levelRange[1] = 10;
        goldshire.name = "Goldshire";
        goldshire.quests = {54, 62, 76, 176, 239};
        _questHubs.push_back(goldshire);
        
        // Lakeshire
        QuestHub lakeshire;
        lakeshire.areaId = 69;
        lakeshire.center = Position(-9220.0f, -2150.0f, 71.0f, 0.0f);
        lakeshire.levelRange[0] = 15;
        lakeshire.levelRange[1] = 20;
        lakeshire.name = "Lakeshire";
        lakeshire.quests = {26503, 26505, 26506, 26511};
        _questHubs.push_back(lakeshire);
        
        // Load more hubs...
    }
};
```

## Quest Movement and Navigation

### Quest Path Finding

```cpp
// src/modules/Playerbot/AI/Quest/QuestPathfinder.h

class QuestPathfinder {
public:
    struct QuestLocation {
        Position position;
        uint32 mapId;
        uint32 areaId;
        float radius;
        std::string description;
    };
    
    struct TravelRoute {
        std::vector<Position> waypoints;
        uint32 estimatedTime;
        bool requiresFlight;
        bool requiresShip;
        uint32 totalDistance;
    };
    
    QuestPathfinder(Player* bot) : _bot(bot) {}
    
    // Location finding
    QuestLocation GetQuestStartLocation(uint32 questId);
    QuestLocation GetQuestEndLocation(uint32 questId);
    std::vector<QuestLocation> GetObjectiveLocations(uint32 questId);
    
    // Route planning
    TravelRoute PlanRoute(const Position& destination);
    TravelRoute PlanFlightRoute(uint32 targetTaxiNode);
    bool ShouldUseFlight(const Position& destination);
    
    // Navigation
    void NavigateToQuest(uint32 questId);
    void NavigateToObjective(QuestObjective const& objective);
    void NavigateToTurnIn(uint32 questId);
    
private:
    Player* _bot;
    
    TravelRoute PlanRoute(const Position& destination) {
        TravelRoute route;
        
        float distance = _bot->GetDistance(destination);
        
        // Check if we should fly
        if (distance > 500.0f && _bot->HasSpell(SPELL_FLIGHT_FORM)) {
            route.requiresFlight = true;
            route.waypoints.push_back(destination);
            route.estimatedTime = distance / 30.0f;  // Flight speed
        }
        // Check for boat/zeppelin
        else if (RequiresContinentTravel(destination)) {
            route = PlanContinentRoute(destination);
        }
        // Normal pathfinding
        else {
            PathGenerator path(_bot);
            path.CalculatePath(destination.GetPositionX(), 
                             destination.GetPositionY(), 
                             destination.GetPositionZ());
            
            auto& points = path.GetPath();
            for (auto& point : points) {
                route.waypoints.push_back(Position(point.x, point.y, point.z));
            }
            
            route.totalDistance = path.GetPathLength();
            route.estimatedTime = route.totalDistance / 7.0f;  // Run speed
        }
        
        return route;
    }
    
    bool RequiresContinentTravel(const Position& destination) {
        uint32 currentContinent = _bot->GetMapId();
        uint32 targetContinent = GetContinentFromPosition(destination);
        
        return currentContinent != targetContinent;
    }
    
    TravelRoute PlanContinentRoute(const Position& destination) {
        TravelRoute route;
        
        // Find nearest boat/zeppelin
        TransportRoute* transport = FindBestTransport(destination);
        if (!transport) {
            route.requiresFlight = true;  // Use flight master
            return route;
        }
        
        // Navigate to dock
        route.waypoints.push_back(transport->dockPosition);
        
        // Board transport
        route.requiresShip = true;
        
        // Navigate from arrival dock to destination
        route.waypoints.push_back(transport->arrivalPosition);
        route.waypoints.push_back(destination);
        
        route.estimatedTime = transport->travelTime + 
            _bot->GetDistance(transport->dockPosition) / 7.0f +
            transport->arrivalPosition.GetDistance(destination) / 7.0f;
        
        return route;
    }
};
```

## Quest Reward Selection

### Intelligent Reward Choice

```cpp
// src/modules/Playerbot/AI/Quest/QuestRewardSelector.h

class QuestRewardSelector {
public:
    struct RewardValue {
        uint32 choiceIndex;
        float value;
        bool isUpgrade;
        bool isBiS;
        std::string reason;
    };
    
    QuestRewardSelector(Player* bot) : _bot(bot) {}
    
    // Reward selection
    uint32 SelectBestReward(Quest const* quest);
    RewardValue EvaluateReward(uint32 itemId);
    
    // Specific evaluations
    bool IsItemUseful(ItemTemplate const* proto);
    float CalculateItemScore(ItemTemplate const* proto);
    bool IsBetterThanEquipped(ItemTemplate const* proto, uint8 slot);
    
private:
    Player* _bot;
    
    uint32 SelectBestReward(Quest const* quest) {
        std::vector<RewardValue> rewards;
        
        // Evaluate each choice
        for (uint8 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i) {
            if (uint32 itemId = quest->RewardChoiceItemId[i]) {
                rewards.push_back(EvaluateReward(itemId));
                rewards.back().choiceIndex = i;
            }
        }
        
        if (rewards.empty())
            return 0;
        
        // Sort by value
        std::sort(rewards.begin(), rewards.end(),
            [](const RewardValue& a, const RewardValue& b) {
                return a.value > b.value;
            });
        
        TC_LOG_DEBUG("playerbot", "Bot %s selecting quest reward: %s (value: %.2f)",
            _bot->GetName().c_str(), 
            rewards[0].reason.c_str(),
            rewards[0].value);
        
        return rewards[0].choiceIndex;
    }
    
    RewardValue EvaluateReward(uint32 itemId) {
        RewardValue value = {};
        
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto) {
            value.reason = "Invalid item";
            return value;
        }
        
        // Can't use item
        if (!_bot->CanUseItem(proto)) {
            value.reason = "Cannot use item";
            return value;
        }
        
        // Check if upgrade
        uint8 slot = GetEquipmentSlot(proto);
        if (slot != NULL_SLOT) {
            if (Item* current = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot)) {
                ItemTemplate const* currentProto = current->GetTemplate();
                
                float currentScore = CalculateItemScore(currentProto);
                float newScore = CalculateItemScore(proto);
                
                value.value = newScore - currentScore;
                value.isUpgrade = value.value > 0;
                
                if (value.isUpgrade) {
                    value.reason = "Upgrade over current item";
                } else {
                    value.reason = "Not an upgrade";
                }
            } else {
                // Empty slot
                value.value = CalculateItemScore(proto);
                value.isUpgrade = true;
                value.reason = "Filling empty slot";
            }
        }
        
        // Check for BiS
        if (IsBestInSlot(proto)) {
            value.value += 1000;
            value.isBiS = true;
            value.reason = "Best in Slot item";
        }
        
        // Consumables/tradeskills
        if (proto->Class == ITEM_CLASS_CONSUMABLE ||
            proto->Class == ITEM_CLASS_TRADE_GOODS) {
            value.value = proto->SellPrice / 10000.0f;  // Gold value
            value.reason = "Consumable/Trade good";
        }
        
        return value;
    }
};
```

## Quest Database

### Quest Data Storage

```cpp
// src/modules/Playerbot/AI/Quest/QuestDatabase.h

class QuestDatabase {
public:
    struct QuestData {
        uint32 questId;
        std::vector<Position> objectivePositions;
        std::vector<uint32> requiredItems;
        std::vector<uint32> requiredKills;
        uint32 questGiverEntry;
        Position questGiverPos;
        uint32 questEnderEntry;
        Position questEnderPos;
        float difficulty;
        bool soloable;
        std::string notes;
    };
    
    static QuestDatabase* instance() {
        static QuestDatabase instance;
        return &instance;
    }
    
    // Data access
    QuestData const* GetQuestData(uint32 questId) const;
    std::vector<uint32> GetQuestsForZone(uint32 zoneId) const;
    std::vector<uint32> GetQuestsForLevel(uint8 level) const;
    
    // Data loading
    void LoadQuestData();
    void LoadQuestChains();
    void LoadQuestGivers();
    
private:
    phmap::flat_hash_map<uint32, QuestData> _questData;
    phmap::flat_hash_map<uint32, std::vector<uint32>> _zoneQuests;
    
    void LoadQuestData() {
        // Load from database or hardcoded data
        QueryResult result = WorldDatabase.Query(
            "SELECT quest_id, objective_x, objective_y, objective_z, "
            "difficulty, soloable FROM bot_quest_data");
        
        if (result) {
            do {
                Field* fields = result->Fetch();
                QuestData data;
                data.questId = fields[0].GetUInt32();
                data.objectivePositions.push_back(
                    Position(fields[1].GetFloat(), 
                           fields[2].GetFloat(), 
                           fields[3].GetFloat()));
                data.difficulty = fields[4].GetFloat();
                data.soloable = fields[5].GetBool();
                
                _questData[data.questId] = data;
            } while (result->NextRow());
        }
    }
};
```

## Testing Framework

```cpp
// src/modules/Playerbot/Tests/QuestAutomationTest.cpp

TEST_F(QuestAutomationTest, QuestAcceptance) {
    auto bot = CreateTestBot();
    BotQuestManager questMgr(bot);
    
    // Create test quest giver
    auto questGiver = CreateTestNPC(NPC_QUEST_GIVER);
    
    // Add available quest
    questGiver->AddQuest(QUEST_TEST_KILL);
    
    // Bot should accept quest
    questMgr.AcceptAllAvailableQuests(questGiver);
    
    EXPECT_TRUE(bot->HasQuest(QUEST_TEST_KILL));
}

TEST_F(QuestAutomationTest, ObjectiveCompletion) {
    auto bot = CreateTestBot();
    QuestObjectiveHandler handler(bot);
    
    // Give bot a kill quest
    bot->AddQuest(QUEST_KILL_10_WOLVES, nullptr);
    
    // Spawn target creatures
    for (int i = 0; i < 10; ++i) {
        CreateTestCreature(NPC_WOLF, bot->GetPosition());
    }
    
    // Execute objective
    Quest const* quest = sObjectMgr->GetQuestTemplate(QUEST_KILL_10_WOLVES);
    handler.ExecuteObjective(quest->GetObjective(0));
    
    // Simulate combat...
    
    EXPECT_TRUE(bot->GetQuestStatus(QUEST_KILL_10_WOLVES) == QUEST_STATUS_COMPLETE);
}

TEST_F(QuestAutomationTest, RewardSelection) {
    auto bot = CreateTestBot(CLASS_WARRIOR);
    QuestRewardSelector selector(bot);
    
    // Create test quest with rewards
    Quest const* quest = sObjectMgr->GetQuestTemplate(QUEST_WITH_REWARDS);
    
    uint32 selected = selector.SelectBestReward(quest);
    
    // Warrior should select strength item
    ItemTemplate const* reward = sObjectMgr->GetItemTemplate(
        quest->RewardChoiceItemId[selected]);
    
    EXPECT_TRUE(HasStrengthStat(reward));
}
```

## Next Steps

1. **Implement Quest Manager** - Core quest handling
2. **Add Objective Handler** - Execute quest tasks
3. **Create Chain Manager** - Quest progression
4. **Add Pathfinder** - Quest navigation
5. **Testing** - Quest scenarios

---

**Status**: Ready for Implementation
**Dependencies**: Movement System
**Estimated Time**: Sprint 3D Days 4-6