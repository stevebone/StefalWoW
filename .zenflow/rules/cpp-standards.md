# C++ Coding Standards for Playerbot

These rules MUST be followed for all C++ code in this project.

## Naming Conventions

```cpp
// Classes: PascalCase
class PlayerbotAI { };
class BotMovementManager { };

// Methods: camelCase
void updateCombat();
bool canCastSpell();

// Member variables: m_ prefix + camelCase
uint32 m_lastUpdateTime;
std::string m_botName;

// Static members: s_ prefix
static uint32 s_instanceCount;

// Constants: ALL_CAPS
constexpr uint32 MAX_BOT_COUNT = 5000;
static const float DEFAULT_FOLLOW_DISTANCE = 3.0f;

// Enums: PascalCase enum, ALL_CAPS values
enum class BotState
{
    IDLE,
    COMBAT,
    TRAVELING
};
```

## Modern C++20 Practices

```cpp
// Use auto for complex types
auto it = m_bots.find(guid);
auto& [key, value] = *it;

// Use range-based for loops
for (const auto& bot : m_bots)
{
    // ...
}

// Use smart pointers
std::unique_ptr<BotAI> ai = std::make_unique<BotAI>();
std::shared_ptr<BotSession> session = std::make_shared<BotSession>();

// Use std::optional for nullable values
std::optional<uint32> findSpellId(const std::string& name);

// Use constexpr where possible
constexpr uint32 calculateMaxHealth(uint8 level)
{
    return level * 100;
}
```

## Memory Management

```cpp
// NEVER use raw new/delete
// BAD:
Player* bot = new Player();
delete bot;

// GOOD:
auto bot = std::make_unique<Player>();
// Automatically cleaned up

// For containers of pointers
std::vector<std::unique_ptr<BotAI>> m_bots;  // GOOD
std::vector<BotAI*> m_bots;                   // BAD
```

## Thread Safety

```cpp
// Use std::shared_mutex for read-heavy data
mutable std::shared_mutex m_mutex;

// Read access
{
    std::shared_lock lock(m_mutex);
    return m_data;  // Multiple readers OK
}

// Write access
{
    std::unique_lock lock(m_mutex);
    m_data = newValue;  // Exclusive access
}

// Follow lock hierarchy: Map > Grid > Object > Session
// NEVER acquire higher-level lock while holding lower-level
```

## Error Handling

```cpp
// Use TC_LOG macros
TC_LOG_ERROR("playerbot", "Failed to load bot {}: {}", guid, error);
TC_LOG_WARN("playerbot", "Bot {} has invalid state", guid);
TC_LOG_INFO("playerbot", "Bot {} logged in", guid);
TC_LOG_DEBUG("playerbot", "Bot {} updating AI", guid);

// Check pointers before use
if (Player* bot = ObjectAccessor::FindPlayer(guid))
{
    if (bot->IsInWorld())
    {
        // Safe to use
    }
}
```

## Include Order

```cpp
// 1. Corresponding header
#include "BotAI.h"

// 2. Project headers
#include "PlayerbotMgr.h"
#include "BotSession.h"

// 3. TrinityCore headers
#include "Player.h"
#include "Spell.h"
#include "WorldPacket.h"

// 4. Standard library
#include <vector>
#include <memory>
#include <string>
```

## Code Formatting

```cpp
// Braces on same line for control structures
if (condition) {
    // ...
}

// Braces on new line for functions/classes
void BotAI::Update()
{
    // ...
}

class BotManager
{
public:
    // ...
};

// Single statement if - still use braces
if (bot->IsDead())
{
    return;
}
```
