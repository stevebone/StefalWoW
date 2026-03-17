# Group System Integration - Phase 3 Sprint 3D
## Party, Raid, and Dungeon Behavior

## Overview

This document details the integration of bots with TrinityCore's group systems, including party/raid formation, role assignment, dungeon mechanics, and coordinated group combat strategies.

## Group Management System

### Core Group Manager

```cpp
// src/modules/Playerbot/AI/Group/GroupManager.h

class BotGroupManager {
public:
    struct GroupMember {
        Player* player;
        BotRole role;
        uint8 raidIndex;
        bool isBot;
        bool isLeader;
        bool isAssistant;
        
        // Role-specific data
        Unit* assignedTarget;      // For DPS focus
        Player* assignedHealTarget; // For healers
        Unit* assignedCCTarget;     // For crowd control
    };
    
    struct GroupComposition {
        uint8 tanks;
        uint8 healers;
        uint8 meleeDPS;
        uint8 rangedDPS;
        uint8 total;
        
        bool IsViable() const {
            // Check minimum requirements
            if (total < 5) return total >= 3;  // 3+ for dungeons
            if (total <= 5) return tanks >= 1 && healers >= 1;
            if (total <= 10) return tanks >= 1 && healers >= 2;
            if (total <= 25) return tanks >= 2 && healers >= 5;
            return tanks >= 2 && healers >= 6;
        }
    };
    
    BotGroupManager(Group* group) : _group(group) {
        AnalyzeComposition();
    }
    
    // Role management
    void AssignRoles();
    void OptimizeRoleDistribution();
    BotRole DetermineOptimalRole(Player* bot);
    
    // Target assignment
    void AssignTargets(Unit* primaryTarget);
    void AssignHealingTargets();
    void AssignCrowdControlTargets(std::vector<Unit*>& enemies);
    
    // Group coordination
    void CoordinateBurst();
    void CoordinateDefensives();
    void CoordinateInterrupts(Unit* target);
    
    // Raid-specific
    void AssignRaidPositions();
    void HandleRaidMechanic(uint32 mechanicId);
    
    // Loot management
    void AssignLootMaster();
    bool ShouldRollOnItem(Player* bot, Item* item);
    
private:
    Group* _group;
    std::vector<GroupMember> _members;
    GroupComposition _composition;
    
    void AnalyzeComposition() {
        _composition = {};
        _members.clear();
        
        for (GroupReference* ref = _group->GetFirstMember(); 
             ref != nullptr; ref = ref->next()) {
            if (Player* member = ref->GetSource()) {
                GroupMember info;
                info.player = member;
                info.isBot = member->IsBot();
                info.isLeader = member->GetGUID() == _group->GetLeaderGUID();
                info.role = DetermineRole(member);
                
                switch (info.role) {
                    case ROLE_TANK: _composition.tanks++; break;
                    case ROLE_HEALER: _composition.healers++; break;
                    case ROLE_DPS:
                        if (IsMelee(member))
                            _composition.meleeDPS++;
                        else
                            _composition.rangedDPS++;
                        break;
                }
                
                _composition.total++;
                _members.push_back(info);
            }
        }
    }
    
    BotRole DetermineRole(Player* player) {
        // Check spec-based role
        switch (player->GetSpecialization()) {
            // Tank specs
            case SPEC_WARRIOR_PROTECTION:
            case SPEC_PALADIN_PROTECTION:
            case SPEC_DEATH_KNIGHT_BLOOD:
            case SPEC_DRUID_GUARDIAN:
            case SPEC_MONK_BREWMASTER:
            case SPEC_DEMON_HUNTER_VENGEANCE:
                return ROLE_TANK;
                
            // Healer specs
            case SPEC_PRIEST_HOLY:
            case SPEC_PRIEST_DISCIPLINE:
            case SPEC_PALADIN_HOLY:
            case SPEC_SHAMAN_RESTORATION:
            case SPEC_DRUID_RESTORATION:
            case SPEC_MONK_MISTWEAVER:
                return ROLE_HEALER;
                
            // DPS specs
            default:
                return ROLE_DPS;
        }
    }
};
```

## Dungeon & Raid Behavior

### Dungeon Mechanics Handler

```cpp
// src/modules/Playerbot/AI/Group/DungeonBehavior.h

class DungeonBehavior {
public:
    struct BossStrategy {
        uint32 bossEntry;
        std::vector<uint32> phases;
        std::map<uint32, std::function<void(Unit*)>> mechanics;
        PositioningStrategy positioning;
        std::vector<uint32> priorityAdds;
    };
    
    struct DungeonData {
        uint32 mapId;
        std::vector<BossStrategy> bosses;
        std::map<uint32, std::string> mechanics;
        bool requiresCC;
        uint8 recommendedItemLevel;
    };
    
    DungeonBehavior(Player* bot) : _bot(bot) {
        LoadDungeonStrategies();
    }
    
    // Boss handling
    void HandleBossEncounter(Creature* boss);
    void HandleBossMechanic(uint32 mechanicId, Unit* source);
    void HandlePhaseTransition(Creature* boss, uint32 newPhase);
    
    // Positioning
    Position GetBossPosition(Creature* boss, BotRole role);
    bool ShouldStack();
    bool ShouldSpread(float minDistance = 8.0f);
    
    // Add management
    Unit* SelectPriorityAdd(std::vector<Unit*>& adds);
    bool ShouldAoE(std::vector<Unit*>& targets);
    
    // Dungeon-specific mechanics
    void HandleVoidZone(Position const& center, float radius);
    void HandleDebuff(uint32 debuffId);
    void HandleInterruptibleCast(Unit* caster, uint32 spellId);
    
private:
    Player* _bot;
    std::map<uint32, DungeonData> _dungeonStrategies;
    
    void LoadDungeonStrategies() {
        // Example: Utgarde Keep
        DungeonData uk;
        uk.mapId = 574;
        
        // Prince Keleseth
        BossStrategy keleseth;
        keleseth.bossEntry = 23953;
        keleseth.mechanics[SPELL_SHADOW_BOLT] = [this](Unit* boss) {
            // Interrupt Shadow Bolt
            if (_bot->IsWithinDist(boss, 10.0f)) {
                InterruptCast(boss);
            }
        };
        keleseth.mechanics[SPELL_ICE_TOMB] = [this](Unit* boss) {
            // Break Ice Tomb on allies
            if (Unit* tomb = GetIceTombTarget()) {
                _bot->SetTarget(tomb->GetGUID());
            }
        };
        uk.bosses.push_back(keleseth);
        
        _dungeonStrategies[574] = uk;
        
        // Load more dungeons...
    }
    
    void HandleBossEncounter(Creature* boss) {
        uint32 mapId = _bot->GetMapId();
        auto it = _dungeonStrategies.find(mapId);
        if (it == _dungeonStrategies.end())
            return;
        
        for (const auto& bossStrat : it->second.bosses) {
            if (bossStrat.bossEntry == boss->GetEntry()) {
                ExecuteBossStrategy(boss, bossStrat);
                break;
            }
        }
    }
    
    void ExecuteBossStrategy(Creature* boss, const BossStrategy& strategy) {
        // Check for phase
        uint32 phase = DetermineBossPhase(boss);
        
        // Handle phase-specific mechanics
        for (const auto& [spellId, handler] : strategy.mechanics) {
            if (boss->HasUnitState(UNIT_STATE_CASTING)) {
                if (boss->GetCurrentSpell(CURRENT_GENERIC_SPELL)->m_spellInfo->Id == spellId) {
                    handler(boss);
                }
            }
        }
        
        // Positioning
        Position pos = GetOptimalPosition(boss, strategy.positioning);
        MoveToPosition(pos);
        
        // Handle adds
        auto adds = GetNearbyAdds(boss);
        if (!adds.empty()) {
            for (uint32 priorityAdd : strategy.priorityAdds) {
                for (Unit* add : adds) {
                    if (add->GetEntry() == priorityAdd) {
                        _bot->SetTarget(add->GetGUID());
                        return;
                    }
                }
            }
        }
    }
};
```

### Raid Coordination

```cpp
// src/modules/Playerbot/AI/Group/RaidCoordinator.h

class RaidCoordinator {
public:
    struct RaidAssignment {
        uint8 group;
        Position position;
        Unit* target;
        std::vector<uint32> tasks;
        uint32 cooldownRotation;
    };
    
    struct RaidMechanic {
        uint32 spellId;
        std::string name;
        uint8 playersRequired;
        std::function<void(std::vector<Player*>&)> handler;
    };
    
    RaidCoordinator(Group* raid) : _raid(raid) {}
    
    // Raid organization
    void AssignRaidGroups();
    void AssignRaidMarkers();
    void SetupCooldownRotation();
    
    // Mechanic handling
    void HandleRaidMechanic(const RaidMechanic& mechanic);
    void AssignMechanicPlayers(uint32 mechanicId, uint8 count);
    
    // Raid-wide coordination
    void CallForBloodlust();
    void CallForRaidCooldown(uint32 spellId);
    void CoordinateMovement(const Position& safeSpot);
    
    // Communication
    void AnnouncePhase(uint32 phase);
    void AnnounceMechanic(const std::string& mechanic);
    
private:
    Group* _raid;
    std::map<Player*, RaidAssignment> _assignments;
    std::queue<uint32> _cooldownRotation;
    
    void SetupCooldownRotation() {
        // Organize defensive cooldowns
        std::vector<std::pair<Player*, uint32>> defensiveCDs;
        
        for (GroupReference* ref = _raid->GetFirstMember(); 
             ref != nullptr; ref = ref->next()) {
            if (Player* member = ref->GetSource()) {
                if (member->IsBot()) {
                    // Check for raid cooldowns
                    if (member->HasSpell(SPELL_DIVINE_GUARDIAN))
                        defensiveCDs.push_back({member, SPELL_DIVINE_GUARDIAN});
                    if (member->HasSpell(SPELL_PAIN_SUPPRESSION))
                        defensiveCDs.push_back({member, SPELL_PAIN_SUPPRESSION});
                    if (member->HasSpell(SPELL_POWER_WORD_BARRIER))
                        defensiveCDs.push_back({member, SPELL_POWER_WORD_BARRIER});
                }
            }
        }
        
        // Create rotation
        for (const auto& [player, spell] : defensiveCDs) {
            _cooldownRotation.push(spell);
        }
    }
    
    void HandleRaidMechanic(const RaidMechanic& mechanic) {
        // Get available bots
        std::vector<Player*> availableBots;
        for (GroupReference* ref = _raid->GetFirstMember(); 
             ref != nullptr; ref = ref->next()) {
            if (Player* member = ref->GetSource()) {
                if (member->IsBot() && member->IsAlive()) {
                    availableBots.push_back(member);
                }
            }
        }
        
        // Select required number of bots
        std::vector<Player*> assigned;
        for (uint8 i = 0; i < mechanic.playersRequired && i < availableBots.size(); ++i) {
            assigned.push_back(availableBots[i]);
        }
        
        // Execute mechanic handler
        mechanic.handler(assigned);
    }
};
```

## Role-Specific Behaviors

### Tank Group Behavior

```cpp
// src/modules/Playerbot/AI/Group/TankBehavior.h

class TankGroupBehavior {
public:
    struct TankSwapStrategy {
        uint32 debuffId;
        uint8 maxStacks;
        uint32 swapCooldown;
    };
    
    TankGroupBehavior(Player* tank) : _tank(tank) {}
    
    // Tank coordination
    void CoordinateTankSwap(Player* otherTank, const TankSwapStrategy& strategy);
    void HandleActiveMitigation(float incomingDamage);
    void PositionBoss(Creature* boss, const Position& tankSpot);
    
    // Threat management
    void EstablishThreat(Unit* target);
    void MaintainThreatLead(float targetThreat = 130.0f);
    
    // Add pickup
    void GatherAdds(std::vector<Unit*>& adds);
    void ChainPullPack(std::vector<Creature*>& pack);
    
private:
    Player* _tank;
    uint32 _lastTauntTime = 0;
    
    void CoordinateTankSwap(Player* otherTank, const TankSwapStrategy& strategy) {
        // Check if swap needed
        if (Aura* debuff = _tank->GetAura(strategy.debuffId)) {
            if (debuff->GetStackAmount() >= strategy.maxStacks) {
                // Signal other tank to taunt
                if (getMSTime() - _lastTauntTime > strategy.swapCooldown) {
                    RequestTaunt(otherTank, _tank->GetVictim());
                    _lastTauntTime = getMSTime();
                }
            }
        }
    }
    
    void HandleActiveMitigation(float incomingDamage) {
        float healthPct = _tank->GetHealthPct();
        float blockValue = _tank->GetShieldBlockValue();
        
        // Proactive mitigation
        if (incomingDamage > _tank->GetMaxHealth() * 0.3f) {
            UseDefensiveCooldown();
        }
        
        // Reactive mitigation
        if (healthPct < 50) {
            UseEmergencyCooldown();
        }
        
        // Maintain active mitigation
        MaintainActiveMitigation();
    }
};
```

### Healer Group Behavior

```cpp
// src/modules/Playerbot/AI/Group/HealerBehavior.h

class HealerGroupBehavior {
public:
    struct HealingAssignment {
        enum Type {
            TANK_HEALING,
            RAID_HEALING,
            SPOT_HEALING,
            DISPELLING
        };
        
        Type type;
        std::vector<Player*> targets;
        uint32 priority;
    };
    
    HealerGroupBehavior(Player* healer) : _healer(healer) {}
    
    // Healing coordination
    void CoordinateHealing(const std::vector<Player*>& healers);
    void AssignHealingTargets();
    void HandleRaidDamage();
    
    // Triage
    Player* SelectHealTarget();
    uint32 SelectHealSpell(Player* target);
    bool ShouldUseAreaHeal();
    
    // Cooldown management
    void CoordinateHealingCooldowns();
    bool ShouldUseTranquility();
    bool ShouldUseDivineHymn();
    
private:
    Player* _healer;
    HealingAssignment _assignment;
    
    Player* SelectHealTarget() {
        Player* lowest = nullptr;
        float lowestHealth = 100.0f;
        
        // Priority: Tanks > Other Healers > DPS
        std::vector<std::pair<Player*, float>> candidates;
        
        if (Group* group = _healer->GetGroup()) {
            for (GroupReference* ref = group->GetFirstMember(); 
                 ref != nullptr; ref = ref->next()) {
                if (Player* member = ref->GetSource()) {
                    if (!member->IsAlive())
                        continue;
                    
                    float health = member->GetHealthPct();
                    float priority = CalculateHealPriority(member);
                    
                    candidates.push_back({member, health * priority});
                }
            }
        }
        
        // Sort by weighted health
        std::sort(candidates.begin(), candidates.end(),
            [](const auto& a, const auto& b) {
                return a.second < b.second;
            });
        
        if (!candidates.empty())
            return candidates.front().first;
        
        return nullptr;
    }
    
    float CalculateHealPriority(Player* target) {
        BotRole role = GetRole(target);
        
        switch (role) {
            case ROLE_TANK: return 2.0f;     // Highest priority
            case ROLE_HEALER: return 1.5f;   // Keep healers alive
            case ROLE_DPS: return 1.0f;      // Normal priority
            default: return 0.8f;
        }
    }
};
```

## Loot Distribution

### Intelligent Loot System

```cpp
// src/modules/Playerbot/AI/Group/LootManager.h

class BotLootManager {
public:
    enum LootDecision {
        NEED,
        GREED,
        PASS,
        DISENCHANT
    };
    
    struct ItemValue {
        float statValue;
        float upgradeValue;
        bool isBiS;        // Best in slot
        bool isSetPiece;
        uint8 priority;
    };
    
    BotLootManager(Player* bot) : _bot(bot) {}
    
    // Loot decisions
    LootDecision DecideOnItem(Item* item);
    ItemValue EvaluateItem(Item* item);
    bool IsUpgrade(Item* item);
    
    // Loot distribution
    void HandleLootRoll(Item* item, RollType rollType);
    void HandleMasterLoot(Item* item, Player* master);
    
    // Need/Greed logic
    bool ShouldNeed(Item* item);
    bool ShouldGreed(Item* item);
    
private:
    Player* _bot;
    
    ItemValue EvaluateItem(Item* item) {
        ItemValue value = {};
        
        ItemTemplate const* proto = item->GetTemplate();
        if (!proto)
            return value;
        
        // Check if usable
        if (!_bot->CanUseItem(proto))
            return value;
        
        // Calculate stat value
        value.statValue = CalculateStatValue(proto);
        
        // Compare to current equipment
        if (Item* current = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, 
            GetEquipSlot(proto))) {
            ItemTemplate const* currentProto = current->GetTemplate();
            float currentValue = CalculateStatValue(currentProto);
            value.upgradeValue = value.statValue - currentValue;
        }
        
        // Check BiS lists
        value.isBiS = IsBestInSlot(proto);
        
        // Check set bonuses
        value.isSetPiece = proto->ItemSet > 0;
        
        // Calculate priority
        if (value.isBiS)
            value.priority = 10;
        else if (value.upgradeValue > 20)
            value.priority = 8;
        else if (value.upgradeValue > 10)
            value.priority = 6;
        else if (value.upgradeValue > 0)
            value.priority = 4;
        else
            value.priority = 1;
        
        return value;
    }
    
    float CalculateStatValue(ItemTemplate const* proto) {
        float value = 0.0f;
        
        // Get stat weights for class/spec
        auto weights = GetStatWeights();
        
        for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i) {
            uint32 statType = proto->ItemStat[i].ItemStatType;
            int32 statValue = proto->ItemStat[i].ItemStatValue;
            
            auto it = weights.find(statType);
            if (it != weights.end()) {
                value += statValue * it->second;
            }
        }
        
        // Add base stats
        value += proto->ItemLevel * 0.5f;
        
        return value;
    }
    
    std::map<uint32, float> GetStatWeights() {
        // Return class/spec specific stat weights
        // This would be loaded from configuration
        std::map<uint32, float> weights;
        
        switch (_bot->getClass()) {
            case CLASS_WARRIOR:
                if (IsTank(_bot)) {
                    weights[ITEM_MOD_STAMINA] = 1.0f;
                    weights[ITEM_MOD_DODGE_RATING] = 0.8f;
                    weights[ITEM_MOD_PARRY_RATING] = 0.8f;
                } else {
                    weights[ITEM_MOD_STRENGTH] = 1.0f;
                    weights[ITEM_MOD_CRIT_RATING] = 0.8f;
                    weights[ITEM_MOD_HIT_RATING] = 0.9f;
                }
                break;
            // ... other classes
        }
        
        return weights;
    }
};
```

## Testing Framework

```cpp
// src/modules/Playerbot/Tests/GroupSystemTest.cpp

TEST_F(GroupSystemTest, RoleAssignment) {
    auto group = CreateTestGroup(5);
    BotGroupManager manager(group);
    
    manager.AssignRoles();
    
    // Should have at least 1 tank and 1 healer
    auto comp = manager.GetComposition();
    EXPECT_GE(comp.tanks, 1);
    EXPECT_GE(comp.healers, 1);
    EXPECT_TRUE(comp.IsViable());
}

TEST_F(GroupSystemTest, TankSwap) {
    auto tank1 = CreateTestBot(CLASS_WARRIOR, 80);
    auto tank2 = CreateTestBot(CLASS_PALADIN, 80);
    auto boss = CreateTestBoss();
    
    TankGroupBehavior behavior(tank1);
    
    // Apply debuff stacks
    tank1->AddAura(SPELL_TANK_DEBUFF, tank1);
    for (int i = 0; i < 5; ++i) {
        tank1->GetAura(SPELL_TANK_DEBUFF)->ModStackAmount(1);
    }
    
    // Should trigger tank swap
    TankSwapStrategy strategy;
    strategy.debuffId = SPELL_TANK_DEBUFF;
    strategy.maxStacks = 4;
    
    behavior.CoordinateTankSwap(tank2, strategy);
    
    // Verify taunt was requested
    EXPECT_TRUE(tank2->HasUnitState(UNIT_STATE_CASTING));
}

TEST_F(GroupSystemTest, HealerTriage) {
    auto healer = CreateTestBot(CLASS_PRIEST, 80);
    auto group = CreateTestGroup(5);
    
    // Set health values
    group->GetMember(0)->SetHealth(1000);  // Tank at 10%
    group->GetMember(1)->SetHealth(5000);  // Healer at 50%
    group->GetMember(2)->SetHealth(8000);  // DPS at 80%
    
    HealerGroupBehavior behavior(healer);
    auto target = behavior.SelectHealTarget();
    
    // Should prioritize tank
    EXPECT_EQ(target, group->GetMember(0));
}
```

## Next Steps

1. **Implement Group Manager** - Role assignment system
2. **Add Dungeon Behavior** - Boss strategies
3. **Create Raid Coordinator** - Large group management
4. **Add Loot System** - Intelligent item decisions
5. **Testing** - Group scenarios

---

**Status**: Ready for Implementation
**Dependencies**: Combat System, Movement System
**Estimated Time**: Sprint 3D Days 1-3