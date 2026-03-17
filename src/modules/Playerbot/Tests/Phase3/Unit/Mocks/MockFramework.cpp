/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Phase 3 God Class Refactoring - Mock Framework Implementations
 *
 * Implements constructors and helper methods for mock classes defined in MockFramework.h
 */

#include "MockFramework.h"

namespace Playerbot
{
namespace Test
{

// ============================================================================
// MOCK UNIT IMPLEMENTATIONS
// ============================================================================

MockUnit::MockUnit()
    : m_name("MockUnit")
    , m_entry(0)
    , m_health(1)
    , m_maxHealth(1)
    , m_inCombat(false)
    , m_alive(true)
{
    // Note: m_guid, m_position, m_targetGuid use default initialization
    // Default power values
    m_power[POWER_MANA] = 1000;
    m_maxPower[POWER_MANA] = 1000;
    m_power[POWER_RAGE] = 0;
    m_maxPower[POWER_RAGE] = 100;
    m_power[POWER_ENERGY] = 100;
    m_maxPower[POWER_ENERGY] = 100;

    // Default mock implementations
    ON_CALL(*this, GetHealth()).WillByDefault(::testing::Return(m_health));
    ON_CALL(*this, GetMaxHealth()).WillByDefault(::testing::Return(m_maxHealth));
    ON_CALL(*this, GetHealthPct()).WillByDefault(::testing::Invoke([this]()
    {
        return (static_cast<float>(m_health) / static_cast<float>(m_maxHealth)) * 100.0f;
    }));
    ON_CALL(*this, IsAlive()).WillByDefault(::testing::Return(m_health > 0));
    ON_CALL(*this, IsDead()).WillByDefault(::testing::Return(m_health == 0));
    ON_CALL(*this, IsInCombat()).WillByDefault(::testing::Return(m_inCombat));
    ON_CALL(*this, GetPosition()).WillByDefault(::testing::Return(m_position));
    ON_CALL(*this, GetGUID()).WillByDefault(::testing::Return(m_guid));

    ON_CALL(*this, GetPower(::testing::_)).WillByDefault(::testing::Invoke([this](MockPowers power)
    {
        auto it = m_power.find(power);
        return (it != m_power.end()) ? it->second : 0u;
    }));

    ON_CALL(*this, GetMaxPower(::testing::_)).WillByDefault(::testing::Invoke([this](MockPowers power)
    {
        auto it = m_maxPower.find(power);
        return (it != m_maxPower.end()) ? it->second : 0u;
    }));

    ON_CALL(*this, GetPowerPct(::testing::_)).WillByDefault(::testing::Invoke([this](MockPowers power)
    {
        auto powerIt = m_power.find(power);
        auto maxPowerIt = m_maxPower.find(power);
        if (powerIt != m_power.end() && maxPowerIt != m_maxPower.end() && maxPowerIt->second > 0)
            return (static_cast<float>(powerIt->second) / static_cast<float>(maxPowerIt->second)) * 100.0f;
        return 0.0f;
    }));
}

// ============================================================================
// MOCK PLAYER IMPLEMENTATIONS
// ============================================================================

MockPlayer::MockPlayer()
    : MockUnit()
    , m_class(CLASS_NONE)
    , m_race(1)
    , m_level(1)
    , m_spec(0)
    , m_group(nullptr)
{
    // Default mock implementations
    ON_CALL(*this, GetClass()).WillByDefault(::testing::Return(m_class));
    ON_CALL(*this, GetLevel()).WillByDefault(::testing::Return(m_level));
    ON_CALL(*this, GetSpec()).WillByDefault(::testing::Return(m_spec));
    ON_CALL(*this, IsInGroup()).WillByDefault(::testing::Return(m_group != nullptr));
    ON_CALL(*this, GetGroup()).WillByDefault(::testing::Return(m_group));

    ON_CALL(*this, HasSpell(::testing::_)).WillByDefault(::testing::Invoke([this](uint32 spellId)
    {
        return m_spells.find(spellId) != m_spells.end();
    }));
}

// ============================================================================
// MOCK SPELL INFO IMPLEMENTATIONS
// ============================================================================

MockSpellInfo::MockSpellInfo(uint32 spellId)
    : m_spellId(spellId)
    , m_manaCost(0)
    , m_cooldown(0)
    , m_castTime(0)
    , m_minRange(0.0f)
    , m_maxRange(40.0f)
{
    ON_CALL(*this, GetId()).WillByDefault(::testing::Return(m_spellId));
    ON_CALL(*this, GetManaCost()).WillByDefault(::testing::Return(m_manaCost));
    ON_CALL(*this, GetCooldown()).WillByDefault(::testing::Return(m_cooldown));
    ON_CALL(*this, GetCastTime()).WillByDefault(::testing::Return(m_castTime));
    ON_CALL(*this, GetMinRange()).WillByDefault(::testing::Return(m_minRange));
    ON_CALL(*this, GetMaxRange()).WillByDefault(::testing::Return(m_maxRange));
}

// ============================================================================
// MOCK AURA IMPLEMENTATIONS
// ============================================================================

MockAura::MockAura(uint32 spellId, MockUnit* caster, MockUnit* target)
    : m_spellId(spellId)
    , m_caster(caster)
    , m_target(target)
    , m_duration(0)
    , m_maxDuration(0)
    , m_stacks(1)
{
    ON_CALL(*this, GetId()).WillByDefault(::testing::Return(m_spellId));
    ON_CALL(*this, GetCaster()).WillByDefault(::testing::Return(m_caster));
    ON_CALL(*this, GetTarget()).WillByDefault(::testing::Return(m_target));
    ON_CALL(*this, GetDuration()).WillByDefault(::testing::Return(m_duration));
    ON_CALL(*this, GetMaxDuration()).WillByDefault(::testing::Return(m_maxDuration));
    ON_CALL(*this, GetStackAmount()).WillByDefault(::testing::Return(m_stacks));
}

// ============================================================================
// MOCK GROUP IMPLEMENTATIONS
// ============================================================================

MockGroup::MockGroup()
{
    // Note: m_guid and m_leaderGuid use default initialization
    ON_CALL(*this, GetGUID()).WillByDefault(::testing::Return(m_guid));
    ON_CALL(*this, GetLeaderGUID()).WillByDefault(::testing::Return(m_leaderGuid));
    ON_CALL(*this, GetMembersCount()).WillByDefault(::testing::Invoke([this]()
    {
        return static_cast<uint32>(m_members.size());
    }));
}

// ============================================================================
// MOCK WORLD SESSION IMPLEMENTATIONS
// ============================================================================

MockWorldSession::MockWorldSession()
    : m_player(nullptr)
    , m_isBot(false)
{
    ON_CALL(*this, GetPlayer()).WillByDefault(::testing::Return(m_player));
    ON_CALL(*this, IsBot()).WillByDefault(::testing::Return(m_isBot));
}

// ============================================================================
// MOCK BOT AI IMPLEMENTATIONS
// ============================================================================

MockBotAI::MockBotAI()
    : m_bot(nullptr)
{
    ON_CALL(*this, GetBot()).WillByDefault(::testing::Return(m_bot));
}

// ============================================================================
// MOCK FACTORY IMPLEMENTATIONS
// ============================================================================

::std::atomic<uint64> MockFactory::s_guidCounter{1};

MockGuid MockFactory::GenerateGUID()
{
    // For testing purposes, generate incrementing GUID values
    // Tests don't need real GUID values - they just need unique identifiable objects
    uint64 value = s_guidCounter.fetch_add(1);
    return MockGuid(value);
}

::std::shared_ptr<MockPlayer> MockFactory::CreateMockPlayer(
    uint8 classId,
    uint8 level,
    uint32 spec)
{
    auto player = ::std::make_shared<MockPlayer>();
    player->SetClass(classId);
    player->SetLevel(level);
    player->SetSpec(spec);
    player->SetMaxHealth(25000);
    player->SetHealth(25000);
    player->SetMaxPower(POWER_MANA, 20000);
    player->SetPower(POWER_MANA, 16000); // 80% mana by default
    return player;
}

::std::shared_ptr<MockUnit> MockFactory::CreateMockEnemy(
    uint32 level,
    uint32 health)
{
    auto enemy = ::std::make_shared<MockUnit>();
    enemy->SetMaxHealth(health);
    enemy->SetHealth(health);
    enemy->SetCombatState(true);
    return enemy;
}

::std::shared_ptr<MockUnit> MockFactory::CreateMockAlly(
    uint32 level,
    uint32 health)
{
    auto ally = ::std::make_shared<MockUnit>();
    ally->SetMaxHealth(health);
    ally->SetHealth(health);
    return ally;
}

::std::shared_ptr<MockSpellInfo> MockFactory::CreateMockSpellInfo(
    uint32 spellId,
    uint32 manaCost,
    uint32 cooldown,
    uint32 castTime)
{
    auto spellInfo = ::std::make_shared<MockSpellInfo>(spellId);
    spellInfo->SetManaCost(manaCost);
    spellInfo->SetCooldown(cooldown);
    spellInfo->SetCastTime(castTime);
    return spellInfo;
}

::std::shared_ptr<MockAura> MockFactory::CreateMockAura(
    uint32 spellId,
    MockUnit* caster,
    MockUnit* target,
    uint32 duration)
{
    return ::std::make_shared<MockAura>(spellId, caster, target);
}

::std::shared_ptr<MockGroup> MockFactory::CreateMockGroup(MockPlayer* leader)
{
    auto group = ::std::make_shared<MockGroup>();
    if (leader)
        group->AddMemberHelper(leader);
    return group;
}

::std::shared_ptr<MockWorldSession> MockFactory::CreateMockSession(
    MockPlayer* player,
    bool isBot)
{
    auto session = ::std::make_shared<MockWorldSession>();
    session->SetPlayer(player);
    session->SetIsBot(isBot);
    return session;
}

::std::shared_ptr<MockBotAI> MockFactory::CreateMockBotAI(MockPlayer* bot)
{
    auto botAI = ::std::make_shared<MockBotAI>();
    botAI->SetBot(bot);
    return botAI;
}

CombatScenario MockFactory::CreateCombatScenario(
    uint8 classId,
    uint8 level,
    uint32 enemyCount)
{
    CombatScenario scenario;
    scenario.player = CreateMockPlayer(classId, level);
    scenario.botAI = CreateMockBotAI(scenario.player.get());

    for (uint32 i = 0; i < enemyCount; ++i)
    {
        auto enemy = CreateMockEnemy(level, 10000);
        scenario.enemies.push_back(enemy);
    }

    return scenario;
}

HealingScenario MockFactory::CreateHealingScenario(
    uint8 healerClass,
    uint32 groupSize,
    float avgHealthPct)
{
    HealingScenario scenario;
    scenario.healer = CreateMockPlayer(healerClass, 80);
    scenario.healerAI = CreateMockBotAI(scenario.healer.get());
    scenario.group = CreateMockGroup(scenario.healer.get());

    for (uint32 i = 0; i < groupSize - 1; ++i)
    {
        auto member = CreateMockPlayer(CLASS_WARRIOR, 80);
        member->SetHealth(static_cast<uint32>(member->GetMaxHealth() * (avgHealthPct / 100.0f)));
        scenario.groupMembers.push_back(member);
        scenario.group->AddMemberHelper(member.get());
    }

    return scenario;
}

GroupScenario MockFactory::CreateGroupScenario(
    uint32 groupSize,
    bool hasHealer,
    bool hasTank)
{
    GroupScenario scenario;
    scenario.group = CreateMockGroup(nullptr);

    if (hasTank)
    {
        scenario.tank = CreateMockPlayer(CLASS_WARRIOR, 80);
        scenario.tank->SetMaxHealth(35000);
        scenario.tank->SetHealth(35000);
        scenario.group->AddMemberHelper(scenario.tank.get());
    }

    if (hasHealer)
    {
        scenario.healer = CreateMockPlayer(CLASS_PRIEST, 80);
        scenario.group->AddMemberHelper(scenario.healer.get());
    }

    uint32 dpsCount = groupSize;
    if (hasTank) --dpsCount;
    if (hasHealer) --dpsCount;

    for (uint32 i = 0; i < dpsCount; ++i)
    {
        auto dps = CreateMockPlayer(CLASS_ROGUE + (i % 4), 80);
        scenario.dps.push_back(dps);
        scenario.group->AddMemberHelper(dps.get());
    }

    scenario.boss = CreateMockEnemy(83, 500000);

    return scenario;
}

} // namespace Test
} // namespace Playerbot
