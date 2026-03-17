# TrinityCore Playerbot Workflows

**Document Version**: 1.0
**Last Updated**: 2025-10-16
**Status**: Active Development

## Table of Contents
1. [Bot Lifecycle Workflow](#bot-lifecycle-workflow)
2. [Combat Workflow](#combat-workflow)
3. [Quest Workflow](#quest-workflow)
4. [Group/Dungeon Workflow](#groupdungeon-workflow)
5. [Economic Workflow](#economic-workflow)
6. [Death Recovery Workflow](#death-recovery-workflow)
7. [LFG/LFR Workflow](#lfglfr-workflow)
8. [Profession Workflow](#profession-workflow)

---

## Bot Lifecycle Workflow

### Overview
The bot lifecycle encompasses creation, initialization, world entry, ongoing updates, and eventual cleanup.

### Lifecycle States

```
[NON_EXISTENT] → [CREATED] → [INITIALIZING] → [IN_WORLD] → [LOGGED_OUT] → [DELETED]
```

### Detailed Workflow

#### Phase 1: Account & Character Creation

```
┌─────────────────────────────────────────┐
│ 1. BotPopulationManager::CreateBot()   │
│    - Generate unique bot name           │
│    - Create bot account                 │
│    - Maximum 10 characters per account  │
└──────────────┬──────────────────────────┘
               ▼
┌─────────────────────────────────────────┐
│ 2. Character Creation                   │
│    - Select race/class                  │
│    - Randomize appearance               │
│    - Assign starting equipment          │
│    - Set starting position              │
└──────────────┬──────────────────────────┘
               ▼
┌─────────────────────────────────────────┐
│ 3. Database Persistence                 │
│    - Insert into characters DB          │
│    - Store bot metadata                 │
│    - Set bot flags (IS_BOT = true)      │
└──────────────┬──────────────────────────┘
               ▼
         [CREATED STATE]
```

#### Phase 2: Session Initialization

```
┌─────────────────────────────────────────┐
│ 4. BotWorldSessionMgr::AddSession()    │
│    - Create BotWorldSession (no socket) │
│    - Assign session ID                  │
│    - Set account permissions            │
└──────────────┬──────────────────────────┘
               ▼
┌─────────────────────────────────────────┐
│ 5. Session::LoadCharacter()            │
│    - Query character from DB            │
│    - Load equipment, inventory          │
│    - Load quest log, spells             │
└──────────────┬──────────────────────────┘
               ▼
┌─────────────────────────────────────────┐
│ 6. Create Bot Entity (extends Player)  │
│    - Set IsBot() flag                   │
│    - Initialize stats                   │
│    - No network socket attached         │
└──────────────┬──────────────────────────┘
               ▼
        [INITIALIZING STATE]
```

#### Phase 3: World Entry

```
┌─────────────────────────────────────────┐
│ 7. Map::AddToMap(bot)                  │
│    - Add to spatial grid                │
│    - Register with map systems          │
│    - Visibility system integration      │
└──────────────┬──────────────────────────┘
               ▼
┌─────────────────────────────────────────┐
│ 8. Initialize BotAI                    │
│    - Determine class (Warrior, Mage...)│
│    - Select specialization              │
│    - Initialize ClassAI instance        │
│    - Load talent build                  │
└──────────────┬──────────────────────────┘
               ▼
┌─────────────────────────────────────────┐
│ 9. BotAI::OnBotAdded()                 │
│    - Initialize behavior managers       │
│    - Subscribe to combat events         │
│    - Set initial behavior state         │
└──────────────┬──────────────────────────┘
               ▼
┌─────────────────────────────────────────┐
│ 10. Start Update Loop                   │
│     - Register with World update cycle  │
│     - Assign update slot (staggered)    │
│     - Begin AI decision-making          │
└──────────────┬──────────────────────────┘
               ▼
          [IN_WORLD STATE]
```

#### Phase 4: Active Update Loop

```
┌───────────────────────────────────────────────────────┐
│                   World Update Tick                    │
└─────────────────┬─────────────────────────────────────┘
                  ▼
┌───────────────────────────────────────────────────────┐
│ BotWorldSessionMgr::Update(diff)                      │
│  - Iterate all active bot sessions                     │
│  - Staggered updates (prevent CPU spikes)              │
└─────────────────┬─────────────────────────────────────┘
                  ▼
┌───────────────────────────────────────────────────────┐
│ BotWorldSession::Update(diff)                         │
│  - Process pending packets (minimal)                   │
│  - Update timers                                        │
└─────────────────┬─────────────────────────────────────┘
                  ▼
┌───────────────────────────────────────────────────────┐
│ Bot::Update(diff) [extends Player::Update()]          │
│  - Player base update                                  │
│  - Combat state update                                 │
│  - Movement update                                     │
└─────────────────┬─────────────────────────────────────┘
                  ▼
┌───────────────────────────────────────────────────────┐
│ BotAI::Update(diff)                                    │
│  ┌─────────────────────────────────────────────────┐  │
│  │ 1. Update Behavior Context                      │  │
│  │    - Combat state (in combat, threat level)     │  │
│  │    - Health/mana/resources                      │  │
│  │    - Group status, buffs, debuffs               │  │
│  └─────────────────────────────────────────────────┘  │
│  ┌─────────────────────────────────────────────────┐  │
│  │ 2. Select Behavior (BehaviorPriorityManager)   │  │
│  │    - Evaluate behavior priorities               │  │
│  │    - Resolve conflicts                          │  │
│  │    - Select highest priority behavior           │  │
│  └─────────────────────────────────────────────────┘  │
│  ┌─────────────────────────────────────────────────┐  │
│  │ 3. Execute Actions (ClassAI)                    │  │
│  │    - Combat rotation (if in combat)             │  │
│  │    - Movement (follow, positioning)             │  │
│  │    - Utility (buffing, healing)                 │  │
│  └─────────────────────────────────────────────────┘  │
│  ┌─────────────────────────────────────────────────┐  │
│  │ 4. Update Subsystems                            │  │
│  │    - QuestManager (track objectives)            │  │
│  │    - InventoryManager (consumables)             │  │
│  │    - ResourceManager (mana, energy)             │  │
│  └─────────────────────────────────────────────────┘  │
└─────────────────┬─────────────────────────────────────┘
                  │
                  └──► Repeat every update tick (100ms)
```

#### Phase 5: Logout & Cleanup

```
┌─────────────────────────────────────────┐
│ 11. BotLifecycleManager::RemoveBot()   │
│     - Trigger: Admin command, error, etc│
└──────────────┬──────────────────────────┘
               ▼
┌─────────────────────────────────────────┐
│ 12. BotAI::OnBotRemoved()              │
│     - Cleanup behavior managers         │
│     - Unsubscribe from events           │
│     - Save state                        │
└──────────────┬──────────────────────────┘
               ▼
┌─────────────────────────────────────────┐
│ 13. Map::RemoveFromMap(bot)            │
│     - Remove from spatial grid          │
│     - Cleanup visibility                │
└──────────────┬──────────────────────────┘
               ▼
┌─────────────────────────────────────────┐
│ 14. Save Character to DB                │
│     - Position, health, mana            │
│     - Inventory, equipment              │
│     - Quest progress                    │
└──────────────┬──────────────────────────┘
               ▼
┌─────────────────────────────────────────┐
│ 15. BotWorldSessionMgr::RemoveSession()│
│     - Destroy session                   │
│     - Release resources                 │
└──────────────┬──────────────────────────┘
               ▼
         [LOGGED_OUT STATE]
```

---

## Combat Workflow

### Overview
Combat is the most complex and performance-critical workflow, involving target acquisition, positioning, ability execution, and coordination.

### Combat States

```
[IDLE] → [THREAT_DETECTED] → [ENGAGING] → [IN_COMBAT] → [DISENGAGING] → [IDLE]
```

### Detailed Combat Flow

#### Phase 1: Threat Detection

```
┌─────────────────────────────────────────────────────────┐
│ BotAI::Update() - Every tick                            │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ CombatStateAnalyzer::CheckThreat()                      │
│  - Am I being attacked?                                 │
│  - Are group members being attacked?                    │
│  - Are hostile NPCs in aggro range?                     │
└──────────────┬──────────────────────────────────────────┘
               ▼
      [THREAT_DETECTED] ──Yes──►
               │
               No
               ▼
         [Remain IDLE]
```

#### Phase 2: Target Acquisition

```
┌─────────────────────────────────────────────────────────┐
│ TargetScanner::ScanForTargets()                         │
│  Strategy: Use PreScanCache first, fallback to query    │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ PreScanCache::GetCachedEnemies()                        │
│  - Check if cache valid (<500ms old)                    │
│  - Return cached enemy list                             │
└──────────────┬──────────────────────────────────────────┘
               │
         Cache Valid? ──No──►
               │              ┌──────────────────────────────┐
               Yes            │ GridQueryProcessor::Query()  │
               │              │  - Submit async to ThreadPool│
               │              │  - Worker thread scans grid  │
               │              │  - Returns enemy list        │
               │              │  - Update PreScanCache       │
               │              └──────────────┬───────────────┘
               │                             │
               └─────────────┬───────────────┘
                             ▼
┌─────────────────────────────────────────────────────────┐
│ TargetSelector::SelectBestTarget(enemies)               │
│  Priority Logic:                                         │
│  1. Current attacker (if attacking me)                  │
│  2. Attacking healer (if I'm DPS/tank)                  │
│  3. Crowd-controlled targets (interrupt)                │
│  4. Low health targets (execute)                        │
│  5. Closest enemy                                       │
└──────────────┬──────────────────────────────────────────┘
               ▼
          [Target Selected]
```

#### Phase 3: Positioning

```
┌─────────────────────────────────────────────────────────┐
│ PositionManager::CalculateOptimalPosition(target)       │
│  - Determine role (tank, healer, DPS)                   │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Tank Positioning:                                        │
│  - Face target                                           │
│  - Keep target engaged                                   │
│  - Maintain threat                                       │
│  - Position: 0-5 yards from target                      │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ Melee DPS Positioning:                                   │
│  - Behind target (avoid parry/block)                    │
│  - 0-5 yards from target                                │
│  - Avoid frontal cone attacks                           │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ Ranged DPS Positioning:                                  │
│  - 20-40 yards from target                              │
│  - Line of sight check                                  │
│  - Avoid melee range                                    │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ Healer Positioning:                                      │
│  - 15-40 yards from group center                        │
│  - Line of sight to all group members                   │
│  - Safe from enemy melee                                │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ LineOfSightManager::CheckLOS(target)                    │
│  - Ray cast check                                        │
│  - Obstacle detection                                    │
└──────────────┬──────────────────────────────────────────┘
               ▼
      Is Position Valid? ──No──► ObstacleAvoidanceManager
               │                         │
               Yes                       │
               ▼                         ▼
        [Move to Position] ◄───────────────┘
```

#### Phase 4: Ability Execution

```
┌─────────────────────────────────────────────────────────┐
│ ClassAI::ExecuteCombatRotation()                        │
│  (Class-specific logic: Warrior, Mage, Priest, etc.)    │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ 1. Check Resources                                       │
│    - Mana, energy, rage, runes, etc.                    │
│    - Enough to cast spell?                              │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ 2. Check Cooldowns (CooldownManager)                    │
│    - Is spell off cooldown?                             │
│    - Global cooldown active?                            │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ 3. Check Range & LOS                                     │
│    - Target in range?                                    │
│    - Line of sight clear?                               │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ 4. InterruptManager::CheckInterrupt()                   │
│    - Is target casting important spell?                 │
│    - Do I have interrupt available?                     │
│    - Priority: Heals > Damage > Other                   │
└──────────────┬──────────────────────────────────────────┘
               │
       Interrupt Needed? ──Yes──► Cast Interrupt Spell
               │                         │
               No                        │
               ▼                         │
┌─────────────────────────────────────────────────────────┐
│ 5. Execute Priority Rotation                            │
│    - Cooldowns (major DPS/survival abilities)           │
│    - Builders (generate resources)                      │
│    - Spenders (consume resources)                       │
│    - Filler (when nothing else available)               │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Bot::CastSpell(spell, target)                           │
│  [Uses TrinityCore spell system]                        │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Spell::Cast()                                            │
│  - TrinityCore handles casting                          │
│  - Spell effects applied                                │
│  - Damage/healing calculated                            │
└──────────────┬──────────────────────────────────────────┘
               ▼
        [Ability Executed]
```

#### Phase 5: AoE Decision Making

```
┌─────────────────────────────────────────────────────────┐
│ AoEDecisionManager::ShouldUseAoE()                      │
│  - Count enemies in range                               │
│  - Threshold: 3+ enemies for most classes               │
└──────────────┬──────────────────────────────────────────┘
               ▼
      3+ Enemies? ──Yes──►
               │            ┌───────────────────────────────┐
               No           │ ClassAI::ExecuteAoERotation() │
               │            │  - AoE builders               │
               │            │  - AoE spenders               │
               │            │  - Positioning (stay grouped) │
               │            └───────────────────────────────┘
               ▼
    [Continue Single-Target Rotation]
```

#### Phase 6: Defensive Reactions

```
┌─────────────────────────────────────────────────────────┐
│ DefensiveBehaviorManager::CheckDefensive()              │
│  - Health < 30%? Emergency                              │
│  - Health < 50%? Defensive cooldown                     │
│  - Taking heavy damage? Mitigation                      │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Emergency Actions (Health < 30%):                        │
│  - Healthstone                                           │
│  - Health potion                                         │
│  - Defensive cooldown (Shield Wall, Ice Block, etc.)    │
│  - Call for healer help                                 │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ Defensive Cooldowns (Health < 50%):                      │
│  - Class-specific defensives                            │
│  - Damage reduction abilities                           │
│  - Temporary shields                                    │
└─────────────────────────────────────────────────────────┘
```

#### Phase 7: Combat Exit

```
┌─────────────────────────────────────────────────────────┐
│ CombatStateAnalyzer::CheckCombatEnd()                   │
│  - All enemies dead/fled?                               │
│  - Out of combat timer expired?                         │
└──────────────┬──────────────────────────────────────────┘
               ▼
         [Exit Combat]
               ▼
┌─────────────────────────────────────────────────────────┐
│ Post-Combat Actions:                                     │
│  - Loot corpses (if configured)                         │
│  - Regenerate health/mana                               │
│  - Rebuff (if buffs expired)                            │
│  - Resume previous activity                             │
└─────────────────────────────────────────────────────────┘
```

---

## Quest Workflow

### Overview
Quests are a core PvE activity. Bots discover, accept, track, complete objectives, and turn in quests autonomously.

### Quest States

```
[AVAILABLE] → [ACCEPTED] → [IN_PROGRESS] → [OBJECTIVES_COMPLETE] → [TURNED_IN]
```

### Detailed Quest Flow

#### Phase 1: Quest Discovery

```
┌─────────────────────────────────────────────────────────┐
│ QuestManager::DiscoverQuests() - Periodic check         │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Query Available Quests                                   │
│  Filters:                                                │
│  - Level appropriate (bot level ± 3)                    │
│  - Prerequisites met (previous quests, reputation)      │
│  - Not already completed                                │
│  - Class/race restrictions                              │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ QuestManager::PrioritizeQuests()                        │
│  Priority:                                               │
│  1. Group quests (if in group)                          │
│  2. Nearby quests (minimize travel)                     │
│  3. Chain quests (continue storyline)                   │
│  4. Rewarding quests (gear upgrades, gold)              │
└──────────────┬──────────────────────────────────────────┘
               ▼
        [Quest Candidates Ready]
```

#### Phase 2: Quest Acceptance

```
┌─────────────────────────────────────────────────────────┐
│ QuestManager::AcceptQuest(questId)                      │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Find Quest Giver NPC                                     │
│  - Query world for NPC with quest                       │
│  - Calculate distance                                    │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Move to Quest Giver                                      │
│  - Pathfinding to NPC position                          │
│  - Stop when in interaction range (5 yards)             │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ NPCInteractionManager::InteractWithQuestGiver(npc)      │
│  - Open quest dialogue                                   │
│  - Review quest details                                  │
│  - Accept quest                                          │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Quest Added to Log                                       │
│  - Store quest ID, objectives                           │
│  - Initialize objective tracking                         │
└──────────────┬──────────────────────────────────────────┘
               ▼
          [QUEST ACCEPTED]
```

#### Phase 3: Objective Tracking

```
┌─────────────────────────────────────────────────────────┐
│ QuestManager::Update() - Every update tick              │
│  - Iterate active quests                                │
│  - Check objective progress                             │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Objective Types:                                         │
│  1. Kill X enemies                                      │
│  2. Collect X items                                     │
│  3. Use item on GameObject                              │
│  4. Talk to NPC                                         │
│  5. Escort NPC                                          │
│  6. Exploration (reach location)                        │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ KILL OBJECTIVE WORKFLOW:                                 │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 1. TargetScanner::FindQuestTargets(questId)     │   │
│  │    - Scan for specific NPC type                 │   │
│  │    - Filter by quest requirement                │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 2. Move to Target                                │   │
│  │    - Pathfinding to NPC                          │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 3. Engage in Combat                              │   │
│  │    - Execute combat rotation                     │   │
│  │    - Kill target                                 │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 4. Objective Counter Incremented                 │   │
│  │    - Check: X/Y killed                           │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ COLLECT OBJECTIVE WORKFLOW:                              │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 1. Item Drop from Kills                          │   │
│  │    - Loot after combat                           │   │
│  │    - Or: GameObject collection                   │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 2. InventoryManager::CheckQuestItems()           │   │
│  │    - Count quest items in bags                   │   │
│  │    - Update objective progress                   │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ USE ITEM OBJECTIVE WORKFLOW (e.g., Quest 26391):        │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 1. Find Target GameObject                        │   │
│  │    - Scan for specific GameObject type           │   │
│  │    - Check distance                              │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 2. Move to GameObject                            │   │
│  │    - Within 7.0 yards (interaction range)        │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 3. Use Quest Item on GameObject                  │   │
│  │    - InventoryManager::UseQuestItem()            │   │
│  │    - Trigger GameObject interaction              │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 4. Objective Completed                           │   │
│  │    - Server validates interaction                │   │
│  │    - Objective marked complete                   │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

#### Phase 4: Quest Completion

```
┌─────────────────────────────────────────────────────────┐
│ QuestManager::CheckQuestCompletion(questId)             │
│  - All objectives complete?                             │
└──────────────┬──────────────────────────────────────────┘
               ▼
          [ALL OBJECTIVES COMPLETE]
               ▼
┌─────────────────────────────────────────────────────────┐
│ QuestManager::TurnInQuest(questId)                      │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Find Quest Turn-In NPC                                   │
│  - May be different from quest giver                    │
│  - Calculate distance                                    │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Move to Turn-In NPC                                      │
│  - Pathfinding to NPC                                    │
│  - Stop at interaction range                            │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ NPCInteractionManager::TurnInQuest(npc, questId)        │
│  - Open quest turn-in dialogue                          │
│  - Review rewards                                        │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ QuestManager::SelectReward()                            │
│  Priority:                                               │
│  1. Item level upgrade (equipment)                      │
│  2. Stat priority (primary stat > secondary)            │
│  3. Gold (if no item upgrade)                           │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Quest Completed                                          │
│  - Experience gained                                     │
│  - Reputation increased                                  │
│  - Reward item received                                 │
│  - Quest removed from log                               │
└──────────────┬──────────────────────────────────────────┘
               ▼
          [QUEST TURNED IN]
```

---

## Group/Dungeon Workflow

### Overview
Group content is a major feature. Bots form groups, queue for dungeons via LFG, teleport, and execute coordinated strategies.

### Group States

```
[SOLO] → [GROUPED] → [LFG_QUEUED] → [DUNGEON_READY] → [IN_DUNGEON] → [COMPLETED]
```

### Detailed Group/Dungeon Flow

#### Phase 1: Group Formation

```
┌─────────────────────────────────────────────────────────┐
│ PlayerbotGroupManager::FormGroup()                      │
│  - Manual group formation (admin command)               │
│  - Or: LFG automatic group formation                    │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Validate Group Composition                               │
│  - 1 Tank (Protection, Guardian, Brewmaster, etc.)      │
│  - 1 Healer (Holy, Restoration, Discipline, etc.)       │
│  - 3 DPS (any damage specialization)                    │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Group Created                                            │
│  - Group leader assigned                                │
│  - Roles locked (tank, healer, DPS)                     │
│  - Group chat initialized                               │
└──────────────┬──────────────────────────────────────────┘
               ▼
           [GROUPED STATE]
```

#### Phase 2: LFG Queue

```
┌─────────────────────────────────────────────────────────┐
│ LFGBotManager::QueueForDungeon(dungeonId)               │
│  - Select dungeon (random or specific)                  │
│  - Set roles (tank, healer, DPS)                        │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Add to LFG Queue                                         │
│  - Queue with TrinityCore LFG system                    │
│  - Wait for group to form                               │
└──────────────┬──────────────────────────────────────────┘
               ▼
           [LFG_QUEUED STATE]
               ▼
┌─────────────────────────────────────────────────────────┐
│ Wait for Group (Queue Time)                             │
│  - Continue normal activities (questing, farming)       │
│  - Periodically check queue status                      │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Group Found!                                             │
│  - 5 players matched                                    │
│  - Roles validated                                      │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Auto-Accept Dungeon Invite                               │
│  - Bots always accept (configurable)                    │
└──────────────┬──────────────────────────────────────────┘
               ▼
          [DUNGEON_READY STATE]
```

#### Phase 3: Dungeon Teleport

```
┌─────────────────────────────────────────────────────────┐
│ LFGBotManager::TeleportToDungeon()                      │
│  - Trigger: All players accepted                        │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Bot Teleported to Dungeon Entrance                       │
│  - Uses TrinityCore teleport system                     │
│  - Group assembled at entrance                          │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ DungeonScript::OnEnter()                                │
│  - Load dungeon-specific script                         │
│  - Initialize encounter strategies                      │
└──────────────┬──────────────────────────────────────────┘
               ▼
          [IN_DUNGEON STATE]
```

#### Phase 4: Dungeon Execution

```
┌─────────────────────────────────────────────────────────┐
│ DungeonBehavior::Execute()                              │
│  - Tank: Pull trash packs, bosses                       │
│  - Healer: Keep group alive                             │
│  - DPS: Kill enemies efficiently                        │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ TRASH PULL WORKFLOW:                                     │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 1. Tank: TargetScanner::FindTrashPack()         │   │
│  │    - Identify next pack                          │   │
│  │    - Check patrol paths                          │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 2. Tank: Pull Pack                               │   │
│  │    - Move to pack                                │   │
│  │    - Engage with AoE threat ability              │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 3. Group: Execute Combat Rotation                │   │
│  │    - Tank: Hold aggro, mitigate damage           │   │
│  │    - Healer: Heal tank, group AoE healing        │   │
│  │    - DPS: AoE rotation if 3+ enemies             │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 4. Loot Distribution                             │   │
│  │    - Need/Greed rolls                            │   │
│  │    - Auto-loot for bots                          │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ BOSS ENCOUNTER WORKFLOW:                                 │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 1. EncounterStrategy::LoadBossScript()           │   │
│  │    - Boss-specific mechanics                     │   │
│  │    - Phase transitions                           │   │
│  │    - Special abilities                           │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 2. Tank: Pull Boss                               │   │
│  │    - Position boss correctly                     │   │
│  │    - Face boss away from group (cleaves)         │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 3. Execute Boss Strategy                         │   │
│  │    - Interrupt priority casts                    │   │
│  │    - Dispel debuffs                              │   │
│  │    - Move out of bad zones                       │   │
│  │    - Switch to adds (if mechanics require)       │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 4. Boss Defeated                                 │   │
│  │    - Loot boss                                   │   │
│  │    - Progress to next section                    │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

#### Phase 5: Dungeon Completion

```
┌─────────────────────────────────────────────────────────┐
│ Final Boss Defeated                                      │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Loot Final Boss                                          │
│  - Gear upgrades distributed                            │
│  - Bonus roll (if available)                            │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Dungeon Complete Notification                            │
│  - Experience gained                                     │
│  - Reputation increased                                  │
│  - Valor/currency awarded                               │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Teleport Out of Dungeon                                  │
│  - Return to previous location                          │
│  - Or: Queue for another dungeon                        │
└──────────────┬──────────────────────────────────────────┘
               ▼
           [COMPLETED STATE]
```

---

## Economic Workflow

### Overview
Bots participate in the game economy by buying, selling, and managing gold.

### Economic Activities

1. Vendor Interaction (buy/sell/repair)
2. Auction House participation
3. Gold management
4. Consumable acquisition

### Detailed Economic Flow

#### Vendor Interaction

```
┌─────────────────────────────────────────────────────────┐
│ EconomyManager::InteractWithVendor()                    │
│  - Periodic check (e.g., after dungeon, every hour)     │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Find Nearest Vendor                                      │
│  - Query world for vendor NPC                           │
│  - Filter by vendor type (general, repair, reagents)    │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Move to Vendor                                           │
│  - Pathfinding to vendor                                │
│  - Stop at interaction range                            │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ NPCInteractionManager::OpenVendor(npc)                  │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ 1. SELL JUNK ITEMS                                       │
│    - InventoryManager::GetJunkItems()                   │
│    - Sell all gray items (junk)                         │
│    - Gain gold                                           │
└─────────────────────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ 2. REPAIR EQUIPMENT                                      │
│    - Check durability                                    │
│    - Repair all items                                    │
│    - Deduct gold cost                                    │
└─────────────────────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ 3. BUY CONSUMABLES                                       │
│    - Food (for health regen)                            │
│    - Water (for mana regen)                             │
│    - Potions (health, mana)                             │
│    - Class-specific reagents (rogue poisons, etc.)      │
│    - Quantity: Maintain 20-40 stack                     │
└─────────────────────────────────────────────────────────┘
               ▼
           [Vendor Interaction Complete]
```

#### Auction House Workflow

```
┌─────────────────────────────────────────────────────────┐
│ EconomyManager::AuctionHouseInteraction()               │
│  - Periodic check (e.g., daily)                         │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Travel to Auction House                                  │
│  - Major cities (Stormwind, Orgrimmar, etc.)            │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ SELLING WORKFLOW:                                        │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 1. InventoryManager::GetAuctionableItems()      │   │
│  │    - Valuable items (greens, blues)              │   │
│  │    - Crafting materials                          │   │
│  │    - Exclude: Equipped, quest items, soulbound   │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 2. AuctionManager::PriceCheck(item)              │   │
│  │    - Query current AH prices                     │   │
│  │    - Undercut lowest price by 5%                 │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 3. Post Auction                                  │   │
│  │    - Duration: 12-48 hours                       │   │
│  │    - Deposit cost deducted                       │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ BUYING WORKFLOW:                                         │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 1. AuctionManager::SearchForUpgrades()           │   │
│  │    - Equipment upgrades (higher ilvl)            │   │
│  │    - Budget: Max 10% of total gold               │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 2. Evaluate Purchase                             │   │
│  │    - Compare to current equipment                │   │
│  │    - Calculate stat upgrade                      │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 3. Buy Item                                      │   │
│  │    - Deduct gold                                 │   │
│  │    - Receive item via mail                       │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

---

## Death Recovery Workflow

### Overview
When a bot dies, it must recover by releasing spirit, navigating to corpse, and resurrecting.

### Death Recovery Flow

```
┌─────────────────────────────────────────────────────────┐
│ Bot Dies (Health = 0)                                    │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ DeathRecoveryManager::OnBotDeath()                      │
│  - Record death location                                │
│  - Record death timestamp                               │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Release Spirit                                           │
│  - Auto-release after 5 seconds                         │
│  - Teleport to nearest graveyard                        │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Ghost Form                                               │
│  - 75% movement speed penalty removed (ghost speed buff)│
│  - Can walk through terrain (limited phasing)           │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Navigate to Corpse                                       │
│  - Pathfinding from graveyard to death location         │
│  - Follow waypoints                                      │
│  - Avoid obstacles                                       │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Arrive at Corpse (within 40 yards)                      │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Resurrect at Corpse                                      │
│  - Restore health/mana to 50%                           │
│  - Apply resurrection sickness (if applicable)          │
│  - Remove ghost form                                    │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Post-Resurrection Actions                                │
│  - Eat/drink to restore health/mana                     │
│  - Check durability (repair if needed)                  │
│  - Resume previous activity                             │
└──────────────┬──────────────────────────────────────────┘
               ▼
           [Back to Normal Activity]
```

---

## LFG/LFR Workflow

### Overview
The Looking For Group (LFG) and Looking For Raid (LFR) systems allow bots to automatically queue and participate in group content.

### LFG/LFR Flow

```
┌─────────────────────────────────────────────────────────┐
│ LFGBotManager::QueueForContent()                        │
│  - Select content type (dungeon, raid, scenario)        │
│  - Select difficulty (normal, heroic, mythic)           │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Role Check                                               │
│  - Tank: Can I queue as tank?                           │
│  - Healer: Can I queue as healer?                       │
│  - DPS: Always available                                │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Add to Queue                                             │
│  - Submit to TrinityCore LFG system                     │
│  - Wait for group formation                             │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Queue Status Check (periodic)                            │
│  - Average wait time: 1-10 minutes                      │
│  - Tank: <1 minute                                      │
│  - Healer: 1-3 minutes                                  │
│  - DPS: 5-10 minutes                                    │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Group Formed                                             │
│  - 5 players for dungeon                                │
│  - 10/25 players for raid (LFR)                         │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Auto-Accept Queue Pop                                    │
│  - Bots always accept (configurable timeout)            │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ Teleport to Instance                                     │
│  - See Group/Dungeon Workflow above                     │
└─────────────────────────────────────────────────────────┘
```

---

## Profession Workflow

### Overview
Bots can learn and level professions, gathering materials and crafting items.

### Profession Flow

```
┌─────────────────────────────────────────────────────────┐
│ ProfessionManager::LearnProfession()                    │
│  - Visit profession trainer                             │
│  - Select 2 primary professions                         │
│  - Learn initial recipes                                │
└──────────────┬──────────────────────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────┐
│ GATHERING PROFESSIONS (Mining, Herbalism, Skinning):    │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 1. GatheringManager::ScanForNodes()              │   │
│  │    - Detect nearby gathering nodes               │   │
│  │    - Herb nodes, mineral veins, skinnable corpses│   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 2. Move to Node                                  │   │
│  │    - Pathfinding to node                         │   │
│  │    - Within 5 yards                              │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 3. Gather Resource                               │   │
│  │    - Cast gathering spell                        │   │
│  │    - Loot resource                               │   │
│  │    - Skill points gained                         │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ CRAFTING PROFESSIONS (Blacksmithing, Alchemy, etc.):    │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 1. ProfessionManager::SelectRecipe()             │   │
│  │    - Choose craftable recipe                     │   │
│  │    - Check reagent availability                  │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 2. Craft Item                                    │   │
│  │    - Consume reagents                            │   │
│  │    - Create item                                 │   │
│  │    - Skill points gained                         │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │ 3. Learn New Recipes                             │   │
│  │    - Visit trainer periodically                  │   │
│  │    - Purchase new recipes                        │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

---

## Summary

This document provides comprehensive workflows for all major bot activities:

1. **Bot Lifecycle**: From creation to world entry and ongoing updates
2. **Combat**: Threat detection, targeting, positioning, ability execution
3. **Quest**: Discovery, acceptance, objective tracking, turn-in
4. **Group/Dungeon**: Formation, queue, teleport, execution, completion
5. **Economic**: Vendor interaction, auction house, gold management
6. **Death Recovery**: Release, navigate, resurrect
7. **LFG/LFR**: Queue, role check, group formation, teleport
8. **Professions**: Gathering and crafting workflows

**Next Steps**: See [PLAYERBOT_ENTITIES.md](PLAYERBOT_ENTITIES.md) for entity reference.
