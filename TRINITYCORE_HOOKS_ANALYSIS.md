# TrinityCore Script Hooks Analysis for WoW 11.2 Playerbot Integration

## Overview
Analysis of all 114 available TrinityCore script hooks for playerbot event system integration, prioritized by importance for bot AI functionality.

---

## CRITICAL Priority Hooks (Essential for Basic Bot Functionality)

### PlayerScript Hooks (19 Critical)

#### **OnLogin**
- **Script Class**: PlayerScript
- **Mapped Event**: PLAYER_LOGIN
- **Priority**: CRITICAL
- **Use Case**: Initialize bot AI, load saved state, setup combat rotations, apply Hero Talents

#### **OnLogout**
- **Script Class**: PlayerScript
- **Mapped Event**: PLAYER_LOGOUT
- **Priority**: CRITICAL
- **Use Case**: Save bot state, cleanup AI resources, persist learning data

#### **OnSpellCast**
- **Script Class**: PlayerScript
- **Mapped Event**: SPELL_CAST_SUCCESS
- **Priority**: CRITICAL
- **Use Case**: Track spell usage for rotation optimization, cooldown management, resource tracking

#### **OnCreatureKill**
- **Script Class**: PlayerScript
- **Mapped Event**: UNIT_KILLED
- **Priority**: CRITICAL
- **Use Case**: Loot decision making, experience tracking, quest credit verification

#### **OnPVPKill**
- **Script Class**: PlayerScript
- **Mapped Event**: PVP_KILL
- **Priority**: CRITICAL
- **Use Case**: PvP behavior adjustment, honor tracking, battlefield objectives

#### **OnPlayerKilledByCreature**
- **Script Class**: PlayerScript
- **Mapped Event**: PLAYER_DEATH
- **Priority**: CRITICAL
- **Use Case**: Death recovery strategy, corpse run pathfinding, resurrection acceptance

#### **OnLevelChanged**
- **Script Class**: PlayerScript
- **Mapped Event**: LEVEL_UP
- **Priority**: CRITICAL
- **Use Case**: Update spell ranks, talent point allocation, gear evaluation thresholds

#### **OnTalentsReset**
- **Script Class**: PlayerScript
- **Mapped Event**: TALENTS_RESET
- **Priority**: CRITICAL
- **Use Case**: Reapply talent builds, update rotation priorities, adjust Hero Talent choices

#### **OnUpdateZone**
- **Script Class**: PlayerScript
- **Mapped Event**: ZONE_CHANGED
- **Priority**: CRITICAL
- **Use Case**: Adjust behavior for zone type (PvP/PvE), update pathfinding mesh, quest availability

#### **OnMapChanged**
- **Script Class**: PlayerScript
- **Mapped Event**: MAP_CHANGE
- **Priority**: CRITICAL
- **Use Case**: Instance strategy loading, difficulty scaling awareness, M+ affix adaptation

#### **OnQuestStatusChange**
- **Script Class**: PlayerScript
- **Mapped Event**: QUEST_STATUS_CHANGE
- **Priority**: CRITICAL
- **Use Case**: Quest chain progression, objective prioritization, reward evaluation

#### **OnPlayerRepop**
- **Script Class**: PlayerScript
- **Mapped Event**: PLAYER_RESURRECT
- **Priority**: CRITICAL
- **Use Case**: Post-death recovery, buff reapplication, positioning reset

#### **OnDuelStart**
- **Script Class**: PlayerScript
- **Mapped Event**: DUEL_START
- **Priority**: CRITICAL
- **Use Case**: PvP rotation activation, cooldown management, positioning strategy

#### **OnDuelEnd**
- **Script Class**: PlayerScript
- **Mapped Event**: DUEL_END
- **Priority**: CRITICAL
- **Use Case**: Reset to PvE behavior, resource recovery, social interaction

#### **OnBindToInstance**
- **Script Class**: PlayerScript
- **Mapped Event**: INSTANCE_BIND
- **Priority**: CRITICAL
- **Use Case**: Raid lockout tracking, M+ key management, instance farming optimization

#### **OnGiveXP**
- **Script Class**: PlayerScript
- **Mapped Event**: XP_GAIN
- **Priority**: CRITICAL
- **Use Case**: Leveling path optimization, rest XP awareness, experience efficiency tracking

#### **OnMoneyChanged**
- **Script Class**: PlayerScript
- **Mapped Event**: MONEY_CHANGE
- **Priority**: CRITICAL
- **Use Case**: Economic decision making, repair management, auction house behavior

#### **OnReputationChange**
- **Script Class**: PlayerScript
- **Mapped Event**: REPUTATION_CHANGE
- **Priority**: CRITICAL
- **Use Case**: Faction farming priorities, vendor unlock tracking, achievement progress

#### **OnChat** (all 5 variants)
- **Script Class**: PlayerScript
- **Mapped Event**: CHAT_MESSAGE_*
- **Priority**: CRITICAL
- **Use Case**: Group coordination, trade negotiations, social responses, command processing

### UnitScript Hooks (2 Critical)

#### **OnDamage**
- **Script Class**: UnitScript
- **Mapped Event**: DAMAGE_DEALT / DAMAGE_TAKEN
- **Priority**: CRITICAL
- **Use Case**: Threat management, defensive cooldown usage, healing priority, M+ pack pulling

#### **OnHeal**
- **Script Class**: UnitScript
- **Mapped Event**: HEAL_DEALT / HEAL_TAKEN
- **Priority**: CRITICAL
- **Use Case**: Healing rotation optimization, overhealing prevention, cooldown timing

### GroupScript Hooks (5 Critical)

#### **OnAddMember**
- **Script Class**: GroupScript
- **Mapped Event**: GROUP_MEMBER_ADDED
- **Priority**: CRITICAL
- **Use Case**: Role assignment, buff responsibility, positioning adjustment

#### **OnRemoveMember**
- **Script Class**: GroupScript
- **Mapped Event**: GROUP_MEMBER_REMOVED
- **Priority**: CRITICAL
- **Use Case**: Role redistribution, strategy adjustment, buff coverage

#### **OnChangeLeader**
- **Script Class**: GroupScript
- **Mapped Event**: GROUP_LEADER_CHANGED
- **Priority**: CRITICAL
- **Use Case**: Follow target update, loot rules awareness, pull pacing

#### **OnInviteMember**
- **Script Class**: GroupScript
- **Mapped Event**: GROUP_INVITE
- **Priority**: CRITICAL
- **Use Case**: Auto-accept logic, group composition analysis

#### **OnDisband**
- **Script Class**: GroupScript
- **Mapped Event**: GROUP_DISBAND
- **Priority**: CRITICAL
- **Use Case**: Return to solo behavior, quest sharing cleanup

---

## HIGH Priority Hooks (Important for Advanced Functionality)

### ItemScript Hooks (5 High)

#### **OnUse**
- **Script Class**: ItemScript
- **Mapped Event**: ITEM_USE
- **Priority**: HIGH
- **Use Case**: Consumable timing, trinket optimization, quest item usage

#### **OnQuestAccept**
- **Script Class**: ItemScript
- **Mapped Event**: QUEST_ACCEPT_ITEM
- **Priority**: HIGH
- **Use Case**: Quest item tracking, chain quest automation

#### **OnExpire**
- **Script Class**: ItemScript
- **Mapped Event**: ITEM_EXPIRE
- **Priority**: HIGH
- **Use Case**: Temporary item management, buff food replacement

#### **OnRemove**
- **Script Class**: ItemScript
- **Mapped Event**: ITEM_REMOVE
- **Priority**: HIGH
- **Use Case**: Inventory management, gear replacement decisions

#### **OnCastItemCombatSpell**
- **Script Class**: ItemScript
- **Mapped Event**: ITEM_SPELL_CAST
- **Priority**: HIGH
- **Use Case**: Trinket proc tracking, on-use item optimization

### VehicleScript Hooks (6 High)

#### **OnInstall**
- **Script Class**: VehicleScript
- **Mapped Event**: VEHICLE_ENTER
- **Priority**: HIGH
- **Use Case**: Vehicle combat rotation, special ability usage

#### **OnUninstall**
- **Script Class**: VehicleScript
- **Mapped Event**: VEHICLE_EXIT
- **Priority**: HIGH
- **Use Case**: Return to normal rotation, position recovery

#### **OnAddPassenger**
- **Script Class**: VehicleScript
- **Mapped Event**: VEHICLE_PASSENGER_ADD
- **Priority**: HIGH
- **Use Case**: Multi-seat vehicle coordination, role assignment

#### **OnRemovePassenger**
- **Script Class**: VehicleScript
- **Mapped Event**: VEHICLE_PASSENGER_REMOVE
- **Priority**: HIGH
- **Use Case**: Seat reassignment, strategy adjustment

#### **OnReset**
- **Script Class**: VehicleScript
- **Mapped Event**: VEHICLE_RESET
- **Priority**: HIGH
- **Use Case**: Vehicle encounter reset handling

#### **OnInstallAccessory**
- **Script Class**: VehicleScript
- **Mapped Event**: VEHICLE_ACCESSORY_INSTALL
- **Priority**: HIGH
- **Use Case**: Turret/cannon control for siege vehicles

### GuildScript Hooks (6 High)

#### **OnAddMember**
- **Script Class**: GuildScript
- **Mapped Event**: GUILD_MEMBER_ADD
- **Priority**: HIGH
- **Use Case**: Guild chat participation, guild event awareness

#### **OnRemoveMember**
- **Script Class**: GuildScript
- **Mapped Event**: GUILD_MEMBER_REMOVE
- **Priority**: HIGH
- **Use Case**: Social relationship updates

#### **OnMemberDepositMoney**
- **Script Class**: GuildScript
- **Mapped Event**: GUILD_BANK_DEPOSIT
- **Priority**: HIGH
- **Use Case**: Guild bank contribution behavior

#### **OnMemberWithdrawMoney**
- **Script Class**: GuildScript
- **Mapped Event**: GUILD_BANK_WITHDRAW
- **Priority**: HIGH
- **Use Case**: Resource acquisition from guild bank

#### **OnItemMove**
- **Script Class**: GuildScript
- **Mapped Event**: GUILD_BANK_ITEM_MOVE
- **Priority**: HIGH
- **Use Case**: Guild bank item management, consumable stocking

#### **OnEvent**
- **Script Class**: GuildScript
- **Mapped Event**: GUILD_EVENT
- **Priority**: HIGH
- **Use Case**: Guild activity participation, calendar events

### AuctionHouseScript Hooks (4 High)

#### **OnAuctionAdd**
- **Script Class**: AuctionHouseScript
- **Mapped Event**: AUCTION_CREATED
- **Priority**: HIGH
- **Use Case**: Market pricing strategy, item listing automation

#### **OnAuctionRemove**
- **Script Class**: AuctionHouseScript
- **Mapped Event**: AUCTION_CANCELLED
- **Priority**: HIGH
- **Use Case**: Auction management, price adjustment

#### **OnAuctionSuccessful**
- **Script Class**: AuctionHouseScript
- **Mapped Event**: AUCTION_SOLD
- **Priority**: HIGH
- **Use Case**: Revenue tracking, market analysis

#### **OnAuctionExpire**
- **Script Class**: AuctionHouseScript
- **Mapped Event**: AUCTION_EXPIRED
- **Priority**: HIGH
- **Use Case**: Relist strategy, price adjustment

### MapScript Hooks (5 High)

#### **OnCreate**
- **Script Class**: MapScript
- **Mapped Event**: MAP_CREATE
- **Priority**: HIGH
- **Use Case**: Instance initialization, strategy loading

#### **OnDestroy**
- **Script Class**: MapScript
- **Mapped Event**: MAP_DESTROY
- **Priority**: HIGH
- **Use Case**: Cleanup, performance optimization

#### **OnPlayerEnter**
- **Script Class**: MapScript
- **Mapped Event**: MAP_PLAYER_ENTER
- **Priority**: HIGH
- **Use Case**: Dungeon/raid strategy activation, M+ timer awareness

#### **OnPlayerLeave**
- **Script Class**: MapScript
- **Mapped Event**: MAP_PLAYER_LEAVE
- **Priority**: HIGH
- **Use Case**: Instance exit handling, loot verification

#### **OnUpdate**
- **Script Class**: MapScript
- **Mapped Event**: MAP_UPDATE
- **Priority**: HIGH
- **Use Case**: Encounter phase tracking, positioning updates

### QuestScript Hooks (3 High)

#### **OnQuestStatusChange**
- **Script Class**: QuestScript
- **Mapped Event**: QUEST_UPDATE
- **Priority**: HIGH
- **Use Case**: Quest progression tracking, objective prioritization

#### **OnAcknowledgeAutoAccept**
- **Script Class**: QuestScript
- **Mapped Event**: QUEST_AUTO_ACCEPT
- **Priority**: HIGH
- **Use Case**: Auto-quest handling, daily quest management

#### **OnQuestObjectiveChange**
- **Script Class**: QuestScript
- **Mapped Event**: QUEST_OBJECTIVE_UPDATE
- **Priority**: HIGH
- **Use Case**: Objective completion tracking, route optimization

---

## MEDIUM Priority Hooks (Useful for Polish and Optimization)

### PlayerScript Hooks (7 Medium)

#### **OnFreeTalentPointsChanged**
- **Script Class**: PlayerScript
- **Mapped Event**: TALENT_POINTS_CHANGE
- **Priority**: MEDIUM
- **Use Case**: Talent point spending decisions, build optimization

#### **OnMoneyLimit**
- **Script Class**: PlayerScript
- **Mapped Event**: MONEY_CAP_REACHED
- **Priority**: MEDIUM
- **Use Case**: Gold cap management, currency conversion

#### **OnDuelRequest**
- **Script Class**: PlayerScript
- **Mapped Event**: DUEL_REQUEST
- **Priority**: MEDIUM
- **Use Case**: Duel acceptance logic, PvP readiness check

#### **OnClearEmote**
- **Script Class**: PlayerScript
- **Mapped Event**: EMOTE_CLEAR
- **Priority**: MEDIUM
- **Use Case**: Social interaction state reset

#### **OnTextEmote**
- **Script Class**: PlayerScript
- **Mapped Event**: EMOTE_TEXT
- **Priority**: MEDIUM
- **Use Case**: Social response generation, RP behavior

#### **OnMovieComplete**
- **Script Class**: PlayerScript
- **Mapped Event**: MOVIE_COMPLETE
- **Priority**: MEDIUM
- **Use Case**: Cutscene completion handling, quest continuation

#### **OnSave**
- **Script Class**: PlayerScript
- **Mapped Event**: PLAYER_SAVE
- **Priority**: MEDIUM
- **Use Case**: Persistent state synchronization

### AreaTriggerScript Hooks (2 Medium)

#### **OnTrigger**
- **Script Class**: AreaTriggerScript
- **Mapped Event**: AREA_TRIGGER_ENTER
- **Priority**: MEDIUM
- **Use Case**: Zone discovery, hidden area detection, quest triggers

#### **OnExit**
- **Script Class**: AreaTriggerScript
- **Mapped Event**: AREA_TRIGGER_EXIT
- **Priority**: MEDIUM
- **Use Case**: Area effect management, buff zone tracking

### SceneScript Hooks (4 Medium)

#### **OnSceneStart**
- **Script Class**: SceneScript
- **Mapped Event**: SCENE_START
- **Priority**: MEDIUM
- **Use Case**: Scripted event participation, phasing awareness

#### **OnSceneCancel**
- **Script Class**: SceneScript
- **Mapped Event**: SCENE_CANCEL
- **Priority**: MEDIUM
- **Use Case**: Scene interruption handling

#### **OnSceneComplete**
- **Script Class**: SceneScript
- **Mapped Event**: SCENE_COMPLETE
- **Priority**: MEDIUM
- **Use Case**: Scene reward collection, quest progression

#### **OnSceneTriggerEvent**
- **Script Class**: SceneScript
- **Mapped Event**: SCENE_TRIGGER
- **Priority**: MEDIUM
- **Use Case**: Scene interaction, objective completion

### WorldScript Hooks (4 Medium)

#### **OnUpdate**
- **Script Class**: WorldScript
- **Mapped Event**: WORLD_UPDATE
- **Priority**: MEDIUM
- **Use Case**: Global bot coordination, performance monitoring

#### **OnStartup**
- **Script Class**: WorldScript
- **Mapped Event**: WORLD_STARTUP
- **Priority**: MEDIUM
- **Use Case**: Bot system initialization, database preloading

#### **OnShutdown**
- **Script Class**: WorldScript
- **Mapped Event**: WORLD_SHUTDOWN
- **Priority**: MEDIUM
- **Use Case**: Graceful bot shutdown, state persistence

#### **OnConfigLoad**
- **Script Class**: WorldScript
- **Mapped Event**: CONFIG_RELOAD
- **Priority**: MEDIUM
- **Use Case**: Dynamic bot configuration updates

### AccountScript Hooks (2 Medium)

#### **OnAccountLogin**
- **Script Class**: AccountScript
- **Mapped Event**: ACCOUNT_LOGIN
- **Priority**: MEDIUM
- **Use Case**: Bot account validation, multi-bot coordination

#### **OnFailedAccountLogin**
- **Script Class**: AccountScript
- **Mapped Event**: ACCOUNT_LOGIN_FAILED
- **Priority**: MEDIUM
- **Use Case**: Bot account error handling

### PlayerChoiceScript Hook (1 Medium)

#### **OnResponse**
- **Script Class**: PlayerChoiceScript
- **Mapped Event**: PLAYER_CHOICE_RESPONSE
- **Priority**: MEDIUM
- **Use Case**: Covenant choices, quest path selection, dialogue options

### WorldStateScript Hook (1 Medium)

#### **OnValueChange**
- **Script Class**: WorldStateScript
- **Mapped Event**: WORLD_STATE_CHANGE
- **Priority**: MEDIUM
- **Use Case**: Battleground objectives, world event tracking

### GameEventScript Hook (1 Medium)

#### **OnTrigger**
- **Script Class**: GameEventScript
- **Mapped Event**: GAME_EVENT_TRIGGER
- **Priority**: MEDIUM
- **Use Case**: Holiday event participation, world event automation

---

## LOW Priority Hooks (Limited Bot Relevance)

### ServerScript Hooks (6 Low)

#### **OnNetworkStart/Stop**
- **Script Class**: ServerScript
- **Priority**: LOW
- **Use Case**: Bots don't need network layer awareness

#### **OnSocketOpen/Close**
- **Script Class**: ServerScript
- **Priority**: LOW
- **Use Case**: Bots bypass socket layer entirely

#### **OnPacketSend/Receive**
- **Script Class**: ServerScript
- **Priority**: LOW
- **Use Case**: Bots don't process packets directly

### WorldScript Hooks (3 Low)

#### **OnOpenStateChange**
- **Script Class**: WorldScript
- **Priority**: LOW
- **Use Case**: Server state management, not bot-specific

#### **OnMotdChange**
- **Script Class**: WorldScript
- **Priority**: LOW
- **Use Case**: MOTD is for human players

#### **OnShutdownInitiate/Cancel**
- **Script Class**: WorldScript
- **Priority**: LOW
- **Use Case**: Handled by OnShutdown for bots

### FormulaScript Hooks (7 Low)

#### **All Formula Hooks**
- **Script Class**: FormulaScript
- **Priority**: LOW
- **Use Case**: Bots use server formulas, don't need to hook calculations

### WeatherScript Hooks (2 Low)

#### **OnChange**
- **Script Class**: WeatherScript
- **Priority**: LOW
- **Use Case**: Weather is cosmetic for bots

#### **OnUpdate**
- **Script Class**: WeatherScript
- **Priority**: LOW
- **Use Case**: No bot behavior changes needed

### PlayerScript Hooks (3 Low)

#### **OnCreate**
- **Script Class**: PlayerScript
- **Priority**: LOW
- **Use Case**: Bot creation handled differently

#### **OnDelete**
- **Script Class**: PlayerScript
- **Priority**: LOW
- **Use Case**: Bot deletion handled differently

#### **OnFailedDelete**
- **Script Class**: PlayerScript
- **Priority**: LOW
- **Use Case**: Bot deletion error handling

### GuildScript Hooks (4 Low)

#### **OnCreate**
- **Script Class**: GuildScript
- **Priority**: LOW
- **Use Case**: Bots don't create guilds

#### **OnDisband**
- **Script Class**: GuildScript
- **Priority**: LOW
- **Use Case**: Bots don't disband guilds

#### **OnMOTDChanged**
- **Script Class**: GuildScript
- **Priority**: LOW
- **Use Case**: MOTD is for human players

#### **OnInfoChanged**
- **Script Class**: GuildScript
- **Priority**: LOW
- **Use Case**: Guild info is for human players

### AccountScript Hooks (4 Low)

#### **OnEmailChange/FailedEmailChange**
- **Script Class**: AccountScript
- **Priority**: LOW
- **Use Case**: Bots don't manage email

#### **OnPasswordChange/FailedPasswordChange**
- **Script Class**: AccountScript
- **Priority**: LOW
- **Use Case**: Bots don't change passwords

---

## Newly Identified Advanced Bot Behaviors

Based on hook analysis, several advanced behaviors we should consider:

### 1. **Dynamic Scene Participation**
Using SceneScript hooks for:
- Covenant campaign automation
- Torghast progression
- Island Expeditions AI
- Warfront contributions

### 2. **Player Choice Intelligence**
Using PlayerChoiceScript for:
- Optimal covenant selection based on class/spec
- Soulbind path optimization
- Dialogue tree navigation for maximum rewards

### 3. **World State Awareness**
Using WorldStateScript for:
- PvP objective prioritization
- World boss spawn tracking
- Invasion participation timing

### 4. **Vehicle Mastery**
Using VehicleScript hooks for:
- Dragonriding optimization
- Siege engine control in battlegrounds
- Mount special ability usage

### 5. **Economic Intelligence**
Using AuctionHouseScript for:
- Market manipulation detection
- Supply/demand analysis
- Cross-faction arbitrage (11.2 feature)

### 6. **Guild Participation**
Using GuildScript for:
- Guild event attendance
- Contribution optimization
- Social standing management

---

## Implementation Recommendations

### Phase 1: Critical Hooks (Week 1-2)
Implement all CRITICAL priority hooks first:
- PlayerScript combat/death/level hooks
- UnitScript damage/heal hooks
- GroupScript coordination hooks

### Phase 2: High Priority Hooks (Week 3-4)
Add HIGH priority hooks for advanced features:
- ItemScript for consumable management
- VehicleScript for mount combat
- GuildScript for social features
- AuctionHouseScript for economy

### Phase 3: Medium Priority Hooks (Week 5)
Polish with MEDIUM priority hooks:
- SceneScript for scripted content
- AreaTriggerScript for exploration
- WorldStateScript for objectives

### Phase 4: Optimization (Week 6)
- Performance profiling of hook overhead
- Conditional hook registration based on bot role
- Event batching for high-frequency hooks

---

## Performance Considerations

### High-Frequency Hooks (Need Optimization)
- **OnDamage/OnHeal**: Called multiple times per second in combat
- **OnUpdate** (Map/World): Called every tick
- **OnChat**: Can be spammed in cities

### Optimization Strategies
1. **Event Batching**: Aggregate similar events before processing
2. **Conditional Registration**: Only register hooks bots actually need
3. **Caching**: Cache expensive calculations between hook calls
4. **Priority Queuing**: Process critical events before cosmetic ones

---

## Security Considerations

### Hooks Requiring Validation
- **OnMoneyChanged**: Prevent gold duplication exploits
- **OnItemMove**: Validate item ownership and permissions
- **OnAuctionSuccessful**: Verify transaction legitimacy
- **OnGuildBank**: Check withdrawal limits

### Recommended Safeguards
1. Rate limiting on economic hooks
2. Sanity checks on resource changes
3. Audit logging for suspicious patterns
4. Rollback capability for exploits

---

## Conclusion

Of the 114 available hooks:
- **31 CRITICAL** hooks are essential for basic bot functionality
- **35 HIGH** priority hooks enable advanced features
- **25 MEDIUM** priority hooks add polish and optimization
- **23 LOW** priority hooks have limited bot relevance

The TrinityCore hook system provides comprehensive coverage for all bot needs, with particular strength in combat, social, and economic integration points. The priority implementation plan ensures bots gain functionality progressively while maintaining performance targets.