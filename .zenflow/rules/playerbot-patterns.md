# Playerbot-Specific Patterns

These patterns are specific to the TrinityCore Playerbot module and MUST be followed.

## Safe Bot Access

ALWAYS check bot validity before operations:

```cpp
// Pattern 1: Direct access
if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
{
    if (bot->IsInWorld() && !bot->IsBeingTeleported())
    {
        if (PlayerbotAI* ai = bot->GetPlayerbotAI())
        {
            ai->DoSomething();
        }
    }
}

// Pattern 2: From PlayerbotMgr
if (PlayerbotMgr* mgr = sPlayerbotMgr)
{
    if (PlayerbotAI* ai = mgr->GetPlayerbotAI(botGuid))
    {
        if (ai->GetBot() && ai->GetBot()->IsInWorld())
        {
            // Safe to use
        }
    }
}
```

## Bot State Machine

```cpp
// State transitions must be thread-safe
void BotAI::SetState(BotState newState)
{
    std::unique_lock lock(m_stateMutex);
    
    BotState oldState = m_state;
    m_state = newState;
    
    lock.unlock();  // Release before callbacks
    
    OnStateChanged(oldState, newState);
}

// State checks should use shared lock
bool BotAI::IsInCombat() const
{
    std::shared_lock lock(m_stateMutex);
    return m_state == BotState::COMBAT;
}
```

## Combat Rotation Pattern

```cpp
class BotCombatAI
{
public:
    void UpdateCombat(uint32 diff)
    {
        if (!m_bot->IsInCombat())
            return;
            
        m_updateTimer += diff;
        if (m_updateTimer < COMBAT_UPDATE_INTERVAL)
            return;
        m_updateTimer = 0;
        
        // Get target safely
        Unit* target = GetValidTarget();
        if (!target)
            return;
            
        // Execute rotation
        for (const auto& spell : m_rotation)
        {
            if (CanCast(spell.id, target))
            {
                CastSpell(spell.id, target);
                break;
            }
        }
    }
    
private:
    Unit* GetValidTarget()
    {
        Unit* target = m_bot->GetVictim();
        if (!target || !target->IsAlive() || !target->IsInWorld())
            return nullptr;
        return target;
    }
};
```

## Database Async Queries

NEVER block on database queries in main thread:

```cpp
// BAD: Synchronous query
QueryResult result = CharacterDatabase.Query("SELECT ...");

// GOOD: Async query with callback
CharacterDatabase.AsyncQuery("SELECT ...")
    .via(&m_executor)
    .then([this](QueryResult result) {
        if (result)
        {
            ProcessResult(result);
        }
    });

// GOOD: Use prepared statements
CharacterDatabasePreparedStatement* stmt = 
    CharacterDatabase.GetPreparedStatement(CHAR_SEL_BOT_DATA);
stmt->SetData(0, botGuid);

CharacterDatabase.AsyncQuery(stmt)
    .then([](PreparedQueryResult result) {
        // Process async
    });
```

## Spell Casting Pattern

```cpp
bool BotAI::CastSpell(uint32 spellId, Unit* target)
{
    // Validate spell
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return false;
        
    // Check if we can cast
    if (!m_bot->HasEnoughPowerFor(spellInfo))
        return false;
        
    if (m_bot->HasSpellCooldown(spellId))
        return false;
        
    // Check range
    if (target && !m_bot->IsWithinCastRange(spellInfo, target))
        return false;
        
    // Cast the spell
    Spell* spell = new Spell(m_bot, spellInfo, TRIGGERED_NONE);
    
    SpellCastTargets targets;
    if (target)
        targets.SetUnitTarget(target);
    else
        targets.SetUnitTarget(m_bot);
        
    SpellCastResult result = spell->prepare(targets);
    
    return result == SPELL_CAST_OK;
}
```

## Event Handling

```cpp
// Register events with RAII
class BotEventHandler
{
public:
    BotEventHandler(PlayerbotAI* ai)
        : m_ai(ai)
    {
        // Register
        sEventMgr->RegisterHandler(EVENT_PLAYER_DEATH, 
            [this](Event* e) { OnPlayerDeath(e); });
    }
    
    ~BotEventHandler()
    {
        // Auto-unregister
        sEventMgr->UnregisterHandler(this);
    }
    
private:
    PlayerbotAI* m_ai;
};
```

## Configuration Access

```cpp
// Use the config system
bool enableFeature = sPlayerbotConfig->GetBoolValue(
    PlayerbotConfig::PLAYERBOT_ENABLE_FEATURE);
    
uint32 maxBots = sPlayerbotConfig->GetIntValue(
    PlayerbotConfig::PLAYERBOT_MAX_BOTS);
    
float followDist = sPlayerbotConfig->GetFloatValue(
    PlayerbotConfig::PLAYERBOT_FOLLOW_DISTANCE);

// Cache frequently accessed values
void BotAI::LoadConfig()
{
    m_followDistance = sPlayerbotConfig->GetFloatValue(
        PlayerbotConfig::PLAYERBOT_FOLLOW_DISTANCE);
}
```

## Packet Sending (WoW 11.x)

```cpp
// ALWAYS use typed packets
void BotAI::SendChatMessage(const std::string& message)
{
    WorldPackets::Chat::Chat packet;
    packet.Initialize(CHAT_MSG_SAY, LANG_UNIVERSAL, m_bot, m_bot, message);
    m_bot->SendMessageToSet(packet.Write(), true);
}

// For spell packets
void BotAI::SendSpellStart(SpellInfo const* spellInfo)
{
    WorldPackets::Spells::SpellStart packet;
    packet.Cast.CasterGUID = m_bot->GetGUID();
    packet.Cast.SpellID = spellInfo->Id;
    // ... fill other fields
    m_bot->SendMessageToSet(packet.Write(), true);
}
```

## Performance Guidelines

```cpp
// Cache ObjectGuid, don't call GetGUID() repeatedly
ObjectGuid botGuid = m_bot->GetGUID();

// Use reserve() for vectors with known size
std::vector<Player*> nearbyPlayers;
nearbyPlayers.reserve(expectedCount);

// Avoid string concatenation in loops
std::ostringstream ss;
for (const auto& item : items)
{
    ss << item << ", ";
}
std::string result = ss.str();

// Use emplace_back instead of push_back
m_bots.emplace_back(std::make_unique<BotAI>());
```
