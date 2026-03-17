# Phase 2: AI Framework - Part 4: Class-Specific AI
## Week 14: Class-Specific AI Implementation

### Example: Warrior AI Implementation

#### File: `src/modules/Playerbot/AI/ClassAI/WarriorBotAI.h`
```cpp
#pragma once

#include "BotAI.h"
#include "Strategy.h"
#include "Action.h"

namespace Playerbot
{

class TC_GAME_API WarriorBotAI : public BotAI
{
public:
    WarriorBotAI(Player* bot);
    ~WarriorBotAI() override = default;

    // Warrior-specific methods
    void InitializeStrategies();
    void InitializeActions();
    void InitializeTriggers();

    // Stance management
    void SetStance(uint32 stanceSpellId);
    uint32 GetCurrentStance() const;
    bool IsInBerserkerStance() const;
    bool IsInDefensiveStance() const;
    bool IsInBattleStance() const;

    // Rage management
    uint32 GetRage() const;
    bool HasEnoughRage(uint32 cost) const;
};

// Warrior strategies
class TC_GAME_API WarriorTankStrategy : public CombatStrategy
{
public:
    WarriorTankStrategy() : CombatStrategy("warrior_tank") {}
    void InitializeActions() override;
    void InitializeTriggers() override;
    float GetThreatModifier() const override { return 2.0f; }
};

class TC_GAME_API WarriorDpsStrategy : public CombatStrategy
{
public:
    WarriorDpsStrategy() : CombatStrategy("warrior_dps") {}
    void InitializeActions() override;
    void InitializeTriggers() override;
};

// Warrior actions
class TC_GAME_API ChargeAction : public CombatAction
{
public:
    ChargeAction() : CombatAction("charge") {}
    bool IsPossible(BotAI* ai) const override;
    bool IsUseful(BotAI* ai) const override;
    ActionResult Execute(BotAI* ai, ActionContext const& context) override;
    float GetRange() const override { return 25.0f; }
};

class TC_GAME_API ShieldSlamAction : public SpellAction
{
public:
    ShieldSlamAction() : SpellAction("shield_slam", 23922) {}
    bool IsPossible(BotAI* ai) const override;
    float GetThreat(BotAI* ai) const override { return 100.0f; }
};

class TC_GAME_API BloodthirstAction : public SpellAction
{
public:
    BloodthirstAction() : SpellAction("bloodthirst", 23881) {}
    float GetCooldown() const override { return 4000.0f; }
};

} // namespace Playerbot
```

#### File: `src/modules/Playerbot/AI/ClassAI/WarriorBotAI.cpp`
```cpp
#include "WarriorBotAI.h"
#include "Player.h"
#include "SpellMgr.h"
#include "Log.h"

namespace Playerbot
{

// Warrior spell IDs
enum WarriorSpells
{
    SPELL_BATTLE_STANCE     = 2457,
    SPELL_DEFENSIVE_STANCE  = 71,
    SPELL_BERSERKER_STANCE  = 2458,
    SPELL_CHARGE            = 100,
    SPELL_SHIELD_SLAM       = 23922,
    SPELL_REVENGE           = 6572,
    SPELL_DEVASTATE         = 20243,
    SPELL_BLOODTHIRST       = 23881,
    SPELL_WHIRLWIND         = 1680,
    SPELL_MORTAL_STRIKE     = 12294,
    SPELL_EXECUTE           = 5308,
    SPELL_THUNDER_CLAP      = 6343,
    SPELL_SHIELD_BLOCK      = 2565,
    SPELL_SPELL_REFLECTION  = 23920,
    SPELL_ENRAGED_REGENERATION = 55694,
    SPELL_RECKLESSNESS      = 1719,
    SPELL_BLADESTORM        = 46924
};

WarriorBotAI::WarriorBotAI(Player* bot) : BotAI(bot)
{
    InitializeStrategies();
    InitializeActions();
    InitializeTriggers();
}

void WarriorBotAI::InitializeStrategies()
{
    // Determine role based on spec
    uint8 spec = _bot->GetPrimarySpecialization();
    
    if (spec == 2) // Protection
    {
        AddStrategy(std::make_unique<WarriorTankStrategy>());
    }
    else // Arms or Fury
    {
        AddStrategy(std::make_unique<WarriorDpsStrategy>());
    }
}

void WarriorBotAI::InitializeActions()
{
    // Register warrior-specific actions
    sActionFactory->RegisterAction("charge",
        []() { return std::make_shared<ChargeAction>(); });
    
    sActionFactory->RegisterAction("shield_slam",
        []() { return std::make_shared<ShieldSlamAction>(); });
    
    sActionFactory->RegisterAction("bloodthirst",
        []() { return std::make_shared<BloodthirstAction>(); });
}

void WarriorBotAI::InitializeTriggers()
{
    // Rage-based triggers
    RegisterTrigger(std::make_shared<ValueTrigger>("high_rage", 80.0f));
    
    // Stance triggers
    RegisterTrigger(std::make_shared<CombatTrigger>("need_defensive_stance"));
}

uint32 WarriorBotAI::GetRage() const
{
    return _bot ? _bot->GetPower(POWER_RAGE) : 0;
}

bool WarriorBotAI::HasEnoughRage(uint32 cost) const
{
    return GetRage() >= cost;
}

// Warrior Tank Strategy Implementation
void WarriorTankStrategy::InitializeActions()
{
    // Tank rotation
    AddAction("shield_slam", std::make_shared<ShieldSlamAction>());
    AddAction("revenge", std::make_shared<SpellAction>("revenge", SPELL_REVENGE));
    AddAction("devastate", std::make_shared<SpellAction>("devastate", SPELL_DEVASTATE));
    AddAction("thunder_clap", std::make_shared<SpellAction>("thunder_clap", SPELL_THUNDER_CLAP));
    
    // Defensive abilities
    AddAction("shield_block", std::make_shared<SpellAction>("shield_block", SPELL_SHIELD_BLOCK));
    AddAction("spell_reflection", std::make_shared<SpellAction>("spell_reflection", SPELL_SPELL_REFLECTION));
}

void WarriorTankStrategy::InitializeTriggers()
{
    // Tank-specific triggers
    AddTrigger(std::make_shared<HealthTrigger>("shield_wall", 0.3f));
    AddTrigger(std::make_shared<ThreatTrigger>("losing_aggro"));
}

// Warrior DPS Strategy Implementation  
void WarriorDpsStrategy::InitializeActions()
{
    // DPS rotation
    AddAction("bloodthirst", std::make_shared<BloodthirstAction>());
    AddAction("whirlwind", std::make_shared<SpellAction>("whirlwind", SPELL_WHIRLWIND));
    AddAction("mortal_strike", std::make_shared<SpellAction>("mortal_strike", SPELL_MORTAL_STRIKE));
    AddAction("execute", std::make_shared<SpellAction>("execute", SPELL_EXECUTE));
    
    // Offensive cooldowns
    AddAction("recklessness", std::make_shared<SpellAction>("recklessness", SPELL_RECKLESSNESS));
    AddAction("bladestorm", std::make_shared<SpellAction>("bladestorm", SPELL_BLADESTORM));
}

void WarriorDpsStrategy::InitializeTriggers()
{
    // DPS triggers
    AddTrigger(std::make_shared<HealthTrigger>("execute_range", 0.2f));
    AddTrigger(std::make_shared<CombatTrigger>("burst_window"));
}

// Charge Action Implementation
bool ChargeAction::IsPossible(BotAI* ai) const
{
    Player* bot = ai->GetBot();
    if (!bot)
        return false;
        
    // Check if we have charge
    if (!bot->HasSpell(SPELL_CHARGE))
        return false;
        
    // Check if charge is on cooldown
    if (bot->HasSpellCooldown(SPELL_CHARGE))
        return false;
        
    // Must be in battle stance
    WarriorBotAI* warriorAI = dynamic_cast<WarriorBotAI*>(ai);
    if (warriorAI && !warriorAI->IsInBattleStance())
        return false;
        
    return true;
}

bool ChargeAction::IsUseful(BotAI* ai) const
{
    Unit* target = ai->GetTargetUnit();
    if (!target)
        return false;
        
    Player* bot = ai->GetBot();
    if (!bot)
        return false;
        
    float distance = bot->GetDistance(target);
    
    // Charge is useful if target is between 8 and 25 yards
    return distance >= 8.0f && distance <= 25.0f;
}

ActionResult ChargeAction::Execute(BotAI* ai, ActionContext const& context)
{
    Player* bot = ai->GetBot();
    Unit* target = context.target ? context.target->ToUnit() : ai->GetTargetUnit();
    
    if (!bot || !target)
        return ActionResult::FAILED;
        
    // Cast charge
    if (DoCast(ai, SPELL_CHARGE, target))
    {
        // Move to target after charge
        ai->MoveTo(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());
        return ActionResult::SUCCESS;
    }
    
    return ActionResult::FAILED;
}

} // namespace Playerbot
```

### Additional Class Implementations

#### File: `src/modules/Playerbot/AI/ClassAI/PriestBotAI.h`
```cpp
#pragma once

#include "BotAI.h"
#include "Strategy.h"

namespace Playerbot
{

class TC_GAME_API PriestBotAI : public BotAI
{
public:
    PriestBotAI(Player* bot);
    
    // Priest-specific methods
    void InitializeStrategies();
    void SelectHealTarget();
    uint32 GetBestHealSpell(Unit* target) const;
    bool ShouldDispel(Unit* target) const;
};

class TC_GAME_API PriestHealStrategy : public Strategy
{
public:
    PriestHealStrategy() : Strategy("priest_heal") {}
    
    void InitializeActions() override;
    void InitializeTriggers() override;
    float GetRelevance(BotAI* ai) const override;
    
    // Healing logic
    Unit* SelectHealTarget(BotAI* ai) const;
    float CalculateHealPriority(Unit* target) const;
};

class TC_GAME_API PriestShadowStrategy : public CombatStrategy
{
public:
    PriestShadowStrategy() : CombatStrategy("priest_shadow") {}
    
    void InitializeActions() override;
    void InitializeTriggers() override;
    
    // Shadow-specific
    bool ShouldRefreshDots(Unit* target) const;
    bool IsInShadowform(Player* bot) const;
};

// Priest actions
class TC_GAME_API FlashHealAction : public SpellAction
{
public:
    FlashHealAction() : SpellAction("flash_heal", 2061) {}
    
    bool IsPossible(BotAI* ai) const override;
    bool IsUseful(BotAI* ai) const override;
    ActionResult Execute(BotAI* ai, ActionContext const& context) override;
    
private:
    Unit* SelectTarget(BotAI* ai) const;
};

class TC_GAME_API PowerWordShieldAction : public SpellAction
{
public:
    PowerWordShieldAction() : SpellAction("power_word_shield", 17) {}
    
    bool IsPossible(BotAI* ai) const override;
    bool IsUseful(BotAI* ai) const override;
    
private:
    bool HasWeakenedSoul(Unit* target) const;
};

} // namespace Playerbot
```

### Class AI Registry

#### File: `src/modules/Playerbot/AI/ClassAI/ClassAIRegistry.cpp`
```cpp
#include "ClassAIRegistry.h"
#include "WarriorBotAI.h"
#include "PriestBotAI.h"
#include "MageBotAI.h"
#include "RogueBotAI.h"
// ... other includes

namespace Playerbot
{

void RegisterAllClassAIs()
{
    // Register class AI factories
    sBotAIFactory->RegisterTemplate("warrior_ai",
        [](BotAI* ai) {
            auto warriorAI = static_cast<WarriorBotAI*>(ai);
            warriorAI->InitializeStrategies();
            warriorAI->InitializeActions();
            warriorAI->InitializeTriggers();
        });
        
    sBotAIFactory->RegisterTemplate("priest_ai",
        [](BotAI* ai) {
            auto priestAI = static_cast<PriestBotAI*>(ai);
            priestAI->InitializeStrategies();
        });
        
    // Register for all other classes...
}

std::unique_ptr<BotAI> CreateClassAI(Player* bot)
{
    if (!bot)
        return nullptr;
        
    switch (bot->getClass())
    {
        case CLASS_WARRIOR:
            return std::make_unique<WarriorBotAI>(bot);
        case CLASS_PRIEST:
            return std::make_unique<PriestBotAI>(bot);
        case CLASS_MAGE:
            return std::make_unique<MageBotAI>(bot);
        case CLASS_ROGUE:
            return std::make_unique<RogueBotAI>(bot);
        case CLASS_PALADIN:
            return std::make_unique<PaladinBotAI>(bot);
        case CLASS_HUNTER:
            return std::make_unique<HunterBotAI>(bot);
        case CLASS_SHAMAN:
            return std::make_unique<ShamanBotAI>(bot);
        case CLASS_WARLOCK:
            return std::make_unique<WarlockBotAI>(bot);
        case CLASS_DRUID:
            return std::make_unique<DruidBotAI>(bot);
        case CLASS_DEATH_KNIGHT:
            return std::make_unique<DeathKnightBotAI>(bot);
        case CLASS_MONK:
            return std::make_unique<MonkBotAI>(bot);
        case CLASS_DEMON_HUNTER:
            return std::make_unique<DemonHunterBotAI>(bot);
        case CLASS_EVOKER:
            return std::make_unique<EvokerBotAI>(bot);
        default:
            return std::make_unique<BotAI>(bot);
    }
}

} // namespace Playerbot
```

## Implementation Checklist

### Week 14 Tasks
- [ ] Implement Warrior AI (tank/dps strategies)
- [ ] Implement Priest AI (heal/shadow strategies)
- [ ] Implement Mage AI (frost/fire/arcane)
- [ ] Implement Rogue AI (combat/assassination)
- [ ] Implement Paladin AI (holy/ret/prot)
- [ ] Implement Hunter AI (pet management)
- [ ] Implement Shaman AI (totems/healing)
- [ ] Implement Warlock AI (pets/dots)
- [ ] Implement Druid AI (forms/hybrid)
- [ ] Implement Death Knight AI (runes/diseases)
- [ ] Implement Monk AI (chi/stagger)
- [ ] Implement Demon Hunter AI (momentum)
- [ ] Implement Evoker AI (empower/essence)

### Class-Specific Features
Each class AI should implement:
1. **Resource Management** (rage/mana/energy/runic power/chi)
2. **Rotation Logic** (spell priority)
3. **Cooldown Management** (offensive/defensive)
4. **Role Switching** (tank/heal/dps)
5. **Special Mechanics** (stances/forms/pets)

## Next Parts
- [Part 1: Overview & Architecture](AI_FRAMEWORK_PART1_OVERVIEW.md)
- [Part 2: Action & Trigger Systems](AI_FRAMEWORK_PART2_ACTIONS.md)
- [Part 3: Main AI Controller](AI_FRAMEWORK_PART3_CONTROLLER.md)
- [Part 5: Testing & Configuration](AI_FRAMEWORK_PART5_TESTING.md)

---

**Status**: Ready for implementation
**Dependencies**: Parts 1-3
**Estimated Completion**: Week 14