/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Phase 3 God Class Refactoring - Priest-Specific Mock Framework
 *
 * This header extends the base MockFramework with Priest-specific mocks,
 * helpers, and test scenario builders for comprehensive Holy and Shadow
 * Priest specialization testing.
 *
 * Design Principles:
 * - Complete Priest spell coverage (Holy, Shadow, Discipline shared)
 * - Realistic resource simulation (Mana, Insanity, Shadow Orbs)
 * - Accurate DoT/HoT tracking with expiration times
 * - Group healing scenario builders
 * - Performance benchmark utilities
 *
 * Quality Requirements (CLAUDE.md compliance):
 * - NO SHORTCUTS: All mocks fully implemented
 * - NO STUBS: Real behavior simulation
 * - COMPLETE COVERAGE: All Priest mechanics supported
 */

#pragma once

#include "MockFramework.h"
#include "AI/ClassAI/Priests/PriestSpecialization.h"
#include <map>
#include <chrono>

namespace Playerbot
{
namespace Test
{

// ============================================================================
// PRIEST-SPECIFIC ENUMERATIONS
// ============================================================================

enum PriestSpellIds
{
    // Holy Spells
    HOLY_WORD_SERENITY              = 2050,
    HOLY_WORD_SANCTIFY              = 34861,
    HOLY_WORD_CHASTISE              = 88625,
    HOLY_WORD_SALVATION             = 265202,
    DIVINE_HYMN                     = 64843,
    GUARDIAN_SPIRIT                 = 47788,
    SERENDIPITY                     = 63730,
    SPIRIT_OF_REDEMPTION            = 20711,
    EMPOWERED_RENEW                 = 63534,
    CHAKRA_SERENITY                 = 81208,
    CHAKRA_SANCTUARY                = 81206,
    APOTHEOSIS                      = 200183,
    BENEDICTION                     = 193157,
    HOLY_FIRE                       = 14914,
    SMITE                           = 585,

    // Shadow Spells
    SHADOW_FORM                     = 15473,
    VOID_FORM                       = 194249,
    MIND_BLAST                      = 8092,
    SHADOW_WORD_PAIN                = 589,
    VAMPIRIC_TOUCH                  = 34914,
    MIND_FLAY                       = 15407,
    SHADOW_WORD_DEATH               = 32379,
    MIND_SPIKE                      = 73510,
    PSYCHIC_SCREAM                  = 8122,
    VOID_BOLT                       = 205448,
    MIND_SEAR                       = 48045,
    SHADOWFIEND                     = 34433,
    MIND_CONTROL                    = 605,
    DISPERSION                      = 47585,
    VAMPIRIC_EMBRACE                = 15286,
    INSANITY_RESOURCE               = 129197,
    VOIDFORM_BUFF                   = 194249,
    LINGERING_INSANITY              = 197937,
    DARK_ASCENSION                  = 391109,
    VOID_ERUPTION                   = 228260,
    DEVOURING_PLAGUE                = 2944,
    SHADOW_CRASH                    = 205385,

    // Shared Healing Spells
    HEAL                            = 2060,
    FLASH_HEAL                      = 2061,
    GREATER_HEAL                    = 2062,
    RENEW                           = 139,
    PRAYER_OF_HEALING               = 596,
    CIRCLE_OF_HEALING               = 204883,
    BINDING_HEAL                    = 32546,
    PRAYER_OF_MENDING               = 33076,

    // Discipline Spells (for completeness)
    POWER_WORD_SHIELD               = 17,
    PENANCE                         = 47540,
    PAIN_SUPPRESSION                = 33206,
    POWER_INFUSION                  = 10060,
    INNER_FOCUS                     = 14751,
    POWER_WORD_BARRIER              = 62618,

    // Utility Spells
    DISPEL_MAGIC                    = 527,
    MASS_DISPEL                     = 32375,
    FADE                            = 586,
    FEAR_WARD                       = 6346,
    LEVITATE                        = 1706,
    LEAP_OF_FAITH                   = 73325,
    SHACKLE_UNDEAD                  = 9484,

    // Buffs
    POWER_WORD_FORTITUDE            = 21562,
    INNER_FIRE                      = 588,
    SHADOW_PROTECTION               = 976,

    // Debuffs
    WEAKENED_SOUL                   = 6788,
    SHADOW_WORD_PAIN_DEBUFF         = 589,
    VAMPIRIC_TOUCH_DEBUFF           = 34914,
    DEVOURING_PLAGUE_DEBUFF         = 2944
};

// Resource types for Priest
enum PriestResourceType
{
    PRIEST_RESOURCE_MANA            = 0,
    PRIEST_RESOURCE_INSANITY        = 1,
    PRIEST_RESOURCE_SHADOW_ORBS     = 2
};

// Priest healing priority levels (extends base HealPriority)
enum class PriestHealPriority : uint8
{
    TANK_CRITICAL                   = 0,  // Tank below 25% health
    EMERGENCY_ANY                   = 1,  // Any member below 20% health
    TANK_MODERATE                   = 2,  // Tank below 60% health
    GROUP_CRITICAL                  = 3,  // Multiple members below 50%
    SINGLE_MODERATE                 = 4,  // Single member 40-60% health
    MAINTENANCE_HEAL                = 5,  // Top-off healing 60-90%
    BUFF_REFRESH                    = 6,  // Renew refresh, buff upkeep
    NO_HEALING_NEEDED               = 7   // Everyone above 90%
};

// ============================================================================
// MOCK PRIEST PLAYER - Extended MockPlayer with Priest mechanics
// ============================================================================

class MockPriestPlayer : public MockPlayer
{
public:
    MockPriestPlayer()
        : MockPlayer()
        , m_insanity(0)
        , m_inVoidForm(false)
        , m_voidFormStacks(0)
        , m_serendipityStacks(0)
        , m_inShadowForm(false)
        , m_inChakraSerenity(false)
        , m_inChakraSanctuary(false)
    {
        SetClass(CLASS_PRIEST);
        SetMaxPower(POWER_MANA, 20000);
        SetPower(POWER_MANA, 16000); // 80% mana by default
    }

    // Insanity management (Shadow Priest)
    uint32 GetInsanity() const { return m_insanity; }
    void SetInsanity(uint32 insanity) { m_insanity = ::std::min(insanity, 100u); }
    void GenerateInsanity(uint32 amount) { m_insanity = ::std::min(m_insanity + amount, 100u); }
    void ConsumeInsanity(uint32 amount) { m_insanity = (amount > m_insanity) ? 0 : m_insanity - amount; }
    float GetInsanityPercent() const { return (m_insanity / 100.0f) * 100.0f; }

    // Voidform management
    bool IsInVoidForm() const { return m_inVoidForm; }
    void EnterVoidForm() { m_inVoidForm = true; m_voidFormStacks = 1; m_insanity = 100; }
    void ExitVoidForm() { m_inVoidForm = false; m_voidFormStacks = 0; m_insanity = 0; }
    uint32 GetVoidFormStacks() const { return m_voidFormStacks; }
    void AddVoidFormStack() { ++m_voidFormStacks; }

    // Serendipity management (Holy Priest)
    uint32 GetSerendipityStacks() const { return m_serendipityStacks; }
    void SetSerendipityStacks(uint32 stacks) { m_serendipityStacks = ::std::min(stacks, 2u); }
    void AddSerendipityStack() { m_serendipityStacks = ::std::min(m_serendipityStacks + 1, 2u); }
    void ConsumeSerendipity() { m_serendipityStacks = 0; }

    // Shadow Form
    bool IsInShadowForm() const { return m_inShadowForm; }
    void EnterShadowForm() { m_inShadowForm = true; }
    void ExitShadowForm() { m_inShadowForm = false; }

    // Chakra states (Holy Priest)
    bool IsInChakraSerenity() const { return m_inChakraSerenity; }
    bool IsInChakraSanctuary() const { return m_inChakraSanctuary; }
    void EnterChakraSerenity() { m_inChakraSerenity = true; m_inChakraSanctuary = false; }
    void EnterChakraSanctuary() { m_inChakraSanctuary = true; m_inChakraSerenity = false; }
    void ExitChakra() { m_inChakraSerenity = false; m_inChakraSanctuary = false; }

    // DoT tracking
    void ApplyDoT(uint32 spellId, uint32 durationMs)
    {
        auto expirationTime = ::std::chrono::steady_clock::now() + ::std::chrono::milliseconds(durationMs);
        m_dotTimers[spellId] = expirationTime;
    }

    bool HasDoT(uint32 spellId) const
    {
        auto it = m_dotTimers.find(spellId);
        if (it == m_dotTimers.end())
            return false;
        return ::std::chrono::steady_clock::now() < it->second;
    }

    uint32 GetDoTTimeRemaining(uint32 spellId) const
    {
        auto it = m_dotTimers.find(spellId);
        if (it == m_dotTimers.end())
            return 0;

        auto now = ::std::chrono::steady_clock::now();
        if (now >= it->second)
            return 0;

        auto remaining = ::std::chrono::duration_cast<::std::chrono::milliseconds>(it->second - now);
        return static_cast<uint32>(remaining.count());
    }

    void ClearDoTs() { m_dotTimers.clear(); }

    // HoT tracking (similar to DoTs)
    void ApplyHoT(uint32 spellId, uint32 durationMs)
    {
        auto expirationTime = ::std::chrono::steady_clock::now() + ::std::chrono::milliseconds(durationMs);
        m_hotTimers[spellId] = expirationTime;
    }

    bool HasHoT(uint32 spellId) const
    {
        auto it = m_hotTimers.find(spellId);
        if (it == m_hotTimers.end())
            return false;
        return ::std::chrono::steady_clock::now() < it->second;
    }

    uint32 GetHoTTimeRemaining(uint32 spellId) const
    {
        auto it = m_hotTimers.find(spellId);
        if (it == m_hotTimers.end())
            return 0;

        auto now = ::std::chrono::steady_clock::now();
        if (now >= it->second)
            return 0;

        auto remaining = ::std::chrono::duration_cast<::std::chrono::milliseconds>(it->second - now);
        return static_cast<uint32>(remaining.count());
    }

    void ClearHoTs() { m_hotTimers.clear(); }

    // Cooldown management with real timestamps
    void SetSpellCooldown(uint32 spellId, uint32 durationMs)
    {
        auto expirationTime = ::std::chrono::steady_clock::now() + ::std::chrono::milliseconds(durationMs);
        m_cooldowns[spellId] = expirationTime;
    }

    bool IsSpellOnCooldown(uint32 spellId) const
    {
        auto it = m_cooldowns.find(spellId);
        if (it == m_cooldowns.end())
            return false;
        return ::std::chrono::steady_clock::now() < it->second;
    }

    uint32 GetSpellCooldownRemaining(uint32 spellId) const
    {
        auto it = m_cooldowns.find(spellId);
        if (it == m_cooldowns.end())
            return 0;

        auto now = ::std::chrono::steady_clock::now();
        if (now >= it->second)
            return 0;

        auto remaining = ::std::chrono::duration_cast<::std::chrono::milliseconds>(it->second - now);
        return static_cast<uint32>(remaining.count());
    }

    void ClearCooldowns() { m_cooldowns.clear(); }

private:
    // Shadow Priest resources
    uint32 m_insanity;
    bool m_inVoidForm;
    uint32 m_voidFormStacks;

    // Holy Priest mechanics
    uint32 m_serendipityStacks;
    bool m_inChakraSerenity;
    bool m_inChakraSanctuary;

    // Shadow Form
    bool m_inShadowForm;

    // DoT/HoT tracking with expiration times
    ::std::map<uint32, ::std::chrono::steady_clock::time_point> m_dotTimers;
    ::std::map<uint32, ::std::chrono::steady_clock::time_point> m_hotTimers;
    ::std::map<uint32, ::std::chrono::steady_clock::time_point> m_cooldowns;
};

// ============================================================================
// SCENARIO STRUCTURES (Forward declarations and definitions)
// ============================================================================

struct HolyPriestHealingScenario
{
    ::std::shared_ptr<MockPriestPlayer> priest;
    ::std::shared_ptr<MockGroup> group;
    ::std::vector<::std::shared_ptr<MockPriestPlayer>> groupMembers;
    MockGuid tankGuid;
    bool criticalEmergency = false;
};

struct ShadowPriestDPSScenario
{
    ::std::shared_ptr<MockPriestPlayer> priest;
    ::std::shared_ptr<MockUnit> boss;
    bool hasDoTs = false;
};

struct ShadowPriestAoEScenario
{
    ::std::shared_ptr<MockPriestPlayer> priest;
    ::std::vector<::std::shared_ptr<MockUnit>> enemies;
    uint32 enemyCount = 0;
};

struct ShadowPriestBurstScenario
{
    ::std::shared_ptr<MockPriestPlayer> priest;
    ::std::shared_ptr<MockUnit> boss;
    bool inVoidform = false;
};

struct HolyPriestRaidHealingScenario
{
    ::std::shared_ptr<MockPriestPlayer> priest;
    ::std::shared_ptr<MockGroup> group;
    ::std::vector<::std::shared_ptr<MockPriestPlayer>> raidMembers;
    uint32 raidSize = 0;
    uint32 injuredCount = 0;
};

// ============================================================================
// PRIEST TEST SCENARIO BUILDERS
// ============================================================================

class PriestScenarioBuilder
{
public:
    // Holy Priest healing scenario with injured group
    static HolyPriestHealingScenario CreateHolyHealingScenario(
        uint32 groupSize = 5,
        float avgHealthPct = 60.0f,
        bool includeTank = true,
        bool criticalEmergency = false
    )
    {
        HolyPriestHealingScenario scenario;

        // Create Holy Priest healer
        scenario.priest = ::std::make_shared<MockPriestPlayer>();
        scenario.priest->SetLevel(80);
        scenario.priest->SetSpec(1); // Holy

        // Add Holy Priest spells
        scenario.priest->AddSpell(HOLY_WORD_SERENITY);
        scenario.priest->AddSpell(HOLY_WORD_SANCTIFY);
        scenario.priest->AddSpell(DIVINE_HYMN);
        scenario.priest->AddSpell(GUARDIAN_SPIRIT);
        scenario.priest->AddSpell(FLASH_HEAL);
        scenario.priest->AddSpell(GREATER_HEAL);
        scenario.priest->AddSpell(RENEW);
        scenario.priest->AddSpell(PRAYER_OF_HEALING);
        scenario.priest->AddSpell(CIRCLE_OF_HEALING);
        scenario.priest->AddSpell(PRAYER_OF_MENDING);
        scenario.priest->AddSpell(HOLY_FIRE);

        // Create group members
        scenario.group = ::std::make_shared<MockGroup>();
        scenario.group->AddMemberHelper(scenario.priest.get());

        for (uint32 i = 0; i < groupSize - 1; ++i)
        {
            auto member = ::std::make_shared<MockPriestPlayer>();
            member->SetLevel(80);
            member->SetMaxHealth(25000);

            // First member is tank if requested
            if (i == 0 && includeTank)
            {
                member->SetClass(CLASS_WARRIOR);
                member->SetMaxHealth(35000);
                scenario.tankGuid = member->GetGUID();

                if (criticalEmergency)
                    member->SetHealth(6000); // 17% health - critical
                else
                    member->SetHealth(static_cast<uint32>(35000 * (avgHealthPct / 100.0f)));
            }
            else
            {
                // DPS with varied health
                member->SetClass(CLASS_ROGUE + (i % 4));

                if (criticalEmergency && i == 1)
                    member->SetHealth(4000); // 16% health - emergency
                else
                {
                    float variance = ((i % 3) - 1) * 10.0f; // ±10% variance
                    float memberHealthPct = ::std::max(10.0f, ::std::min(100.0f, avgHealthPct + variance));
                    member->SetHealth(static_cast<uint32>(25000 * (memberHealthPct / 100.0f)));
                }
            }

            scenario.groupMembers.push_back(member);
            scenario.group->AddMemberHelper(member.get());
        }

        scenario.criticalEmergency = criticalEmergency;
        return scenario;
    }

    // Shadow Priest DPS scenario with single target
    static ShadowPriestDPSScenario CreateShadowSingleTargetScenario(
        uint32 bossLevel = 83,
        uint32 bossHealth = 500000,
        bool hasPriorDoTs = false
    )
    {
        ShadowPriestDPSScenario scenario;

        // Create Shadow Priest
        scenario.priest = ::std::make_shared<MockPriestPlayer>();
        scenario.priest->SetLevel(80);
        scenario.priest->SetSpec(2); // Shadow
        scenario.priest->EnterShadowForm();

        // Add Shadow Priest spells
        scenario.priest->AddSpell(SHADOW_FORM);
        scenario.priest->AddSpell(VOID_FORM);
        scenario.priest->AddSpell(MIND_BLAST);
        scenario.priest->AddSpell(SHADOW_WORD_PAIN);
        scenario.priest->AddSpell(VAMPIRIC_TOUCH);
        scenario.priest->AddSpell(MIND_FLAY);
        scenario.priest->AddSpell(SHADOW_WORD_DEATH);
        scenario.priest->AddSpell(MIND_SPIKE);
        scenario.priest->AddSpell(VOID_BOLT);
        scenario.priest->AddSpell(SHADOWFIEND);
        scenario.priest->AddSpell(DISPERSION);
        scenario.priest->AddSpell(DARK_ASCENSION);
        scenario.priest->AddSpell(DEVOURING_PLAGUE);

        // Create boss target
        scenario.boss = ::std::make_shared<MockUnit>();
        scenario.boss->SetMaxHealth(bossHealth);
        scenario.boss->SetHealth(bossHealth);
        scenario.boss->SetCombatState(true);

        // Apply prior DoTs if requested
        if (hasPriorDoTs)
        {
            scenario.priest->ApplyDoT(SHADOW_WORD_PAIN, 18000); // 18s remaining
            scenario.priest->ApplyDoT(VAMPIRIC_TOUCH, 15000);   // 15s remaining
        }

        return scenario;
    }

    // Shadow Priest AoE scenario with multiple targets
    static ShadowPriestAoEScenario CreateShadowAoEScenario(
        uint32 enemyCount = 5,
        uint32 enemyHealth = 50000
    )
    {
        ShadowPriestAoEScenario scenario;

        // Create Shadow Priest
        scenario.priest = ::std::make_shared<MockPriestPlayer>();
        scenario.priest->SetLevel(80);
        scenario.priest->SetSpec(2); // Shadow
        scenario.priest->EnterShadowForm();
        scenario.priest->SetInsanity(50); // Mid insanity

        // Add AoE-focused spells
        scenario.priest->AddSpell(SHADOW_FORM);
        scenario.priest->AddSpell(MIND_BLAST);
        scenario.priest->AddSpell(SHADOW_WORD_PAIN);
        scenario.priest->AddSpell(VAMPIRIC_TOUCH);
        scenario.priest->AddSpell(MIND_SEAR);
        scenario.priest->AddSpell(SHADOW_CRASH);
        scenario.priest->AddSpell(VOID_ERUPTION);

        // Create enemy pack
        for (uint32 i = 0; i < enemyCount; ++i)
        {
            auto enemy = ::std::make_shared<MockUnit>();
            enemy->SetMaxHealth(enemyHealth);
            enemy->SetHealth(enemyHealth);
            enemy->SetCombatState(true);
            scenario.enemies.push_back(enemy);
        }

        scenario.enemyCount = enemyCount;
        return scenario;
    }

    // Voidform burst scenario (Shadow Priest at max insanity)
    static ShadowPriestBurstScenario CreateVoidformBurstScenario(
        bool inVoidform = false
    )
    {
        ShadowPriestBurstScenario scenario;

        // Create Shadow Priest at max insanity
        scenario.priest = ::std::make_shared<MockPriestPlayer>();
        scenario.priest->SetLevel(80);
        scenario.priest->SetSpec(2); // Shadow
        scenario.priest->EnterShadowForm();
        scenario.priest->SetInsanity(100); // Max insanity

        if (inVoidform)
        {
            scenario.priest->EnterVoidForm();
            scenario.inVoidform = true;
        }

        // Add all Shadow Priest burst spells
        scenario.priest->AddSpell(VOID_FORM);
        scenario.priest->AddSpell(VOID_ERUPTION);
        scenario.priest->AddSpell(VOID_BOLT);
        scenario.priest->AddSpell(MIND_BLAST);
        scenario.priest->AddSpell(DEVOURING_PLAGUE);
        scenario.priest->AddSpell(SHADOW_WORD_DEATH);
        scenario.priest->AddSpell(SHADOWFIEND);
        scenario.priest->AddSpell(DARK_ASCENSION);

        // Create raid boss
        scenario.boss = ::std::make_shared<MockUnit>();
        scenario.boss->SetMaxHealth(10000000); // 10M health raid boss
        scenario.boss->SetHealth(10000000);
        scenario.boss->SetCombatState(true);

        return scenario;
    }

    // Holy Priest AoE healing scenario (raid-wide damage)
    static HolyPriestRaidHealingScenario CreateRaidHealingScenario(
        uint32 raidSize = 25,
        float avgHealthPct = 50.0f,
        uint32 injuredCount = 15
    )
    {
        HolyPriestRaidHealingScenario scenario;

        // Create Holy Priest
        scenario.priest = ::std::make_shared<MockPriestPlayer>();
        scenario.priest->SetLevel(80);
        scenario.priest->SetSpec(1); // Holy
        scenario.priest->EnterChakraSanctuary(); // AoE healing mode

        // Add raid healing spells
        scenario.priest->AddSpell(HOLY_WORD_SANCTIFY);
        scenario.priest->AddSpell(DIVINE_HYMN);
        scenario.priest->AddSpell(PRAYER_OF_HEALING);
        scenario.priest->AddSpell(CIRCLE_OF_HEALING);
        scenario.priest->AddSpell(RENEW);
        scenario.priest->AddSpell(PRAYER_OF_MENDING);

        // Create raid group
        scenario.group = ::std::make_shared<MockGroup>();
        scenario.raidSize = raidSize;
        scenario.injuredCount = injuredCount;

        for (uint32 i = 0; i < raidSize; ++i)
        {
            auto member = ::std::make_shared<MockPriestPlayer>();
            member->SetLevel(80);
            member->SetMaxHealth(25000);

            // First injuredCount members are injured
            if (i < injuredCount)
            {
                float healthPct = avgHealthPct + ((i % 3) - 1) * 5.0f; // ±5% variance
                member->SetHealth(static_cast<uint32>(25000 * (healthPct / 100.0f)));
            }
            else
            {
                member->SetHealth(25000); // Full health
            }

            scenario.raidMembers.push_back(member);
            scenario.group->AddMemberHelper(member.get());
        }

        return scenario;
    }
};

// ============================================================================
// PRIEST-SPECIFIC TEST ASSERTIONS
// ============================================================================

// DoT/HoT assertions
#define EXPECT_DOT_APPLIED(priest, spellId) \
    EXPECT_TRUE(priest->HasDoT(spellId)) << "Expected DoT " << spellId << " to be applied"

#define EXPECT_DOT_NOT_APPLIED(priest, spellId) \
    EXPECT_FALSE(priest->HasDoT(spellId)) << "Expected DoT " << spellId << " to NOT be applied"

#define EXPECT_HOT_APPLIED(priest, spellId) \
    EXPECT_TRUE(priest->HasHoT(spellId)) << "Expected HoT " << spellId << " to be applied"

#define EXPECT_DOT_TIME_REMAINING(priest, spellId, minMs, maxMs) \
    { \
        uint32 remaining = priest->GetDoTTimeRemaining(spellId); \
        EXPECT_GE(remaining, minMs) << "DoT " << spellId << " remaining: " << remaining << "ms (expected >=" << minMs << "ms)"; \
        EXPECT_LE(remaining, maxMs) << "DoT " << spellId << " remaining: " << remaining << "ms (expected <=" << maxMs << "ms)"; \
    }

// Resource assertions
#define EXPECT_INSANITY_LEVEL(priest, expectedInsanity) \
    EXPECT_EQ(priest->GetInsanity(), expectedInsanity) << "Expected insanity: " << expectedInsanity << ", actual: " << priest->GetInsanity()

#define EXPECT_INSANITY_RANGE(priest, minInsanity, maxInsanity) \
    { \
        uint32 insanity = priest->GetInsanity(); \
        EXPECT_GE(insanity, minInsanity) << "Insanity " << insanity << " below min " << minInsanity; \
        EXPECT_LE(insanity, maxInsanity) << "Insanity " << insanity << " above max " << maxInsanity; \
    }

#define EXPECT_IN_VOIDFORM(priest) \
    EXPECT_TRUE(priest->IsInVoidForm()) << "Expected priest to be in Voidform"

#define EXPECT_NOT_IN_VOIDFORM(priest) \
    EXPECT_FALSE(priest->IsInVoidForm()) << "Expected priest to NOT be in Voidform"

#define EXPECT_VOIDFORM_STACKS(priest, expectedStacks) \
    EXPECT_EQ(priest->GetVoidFormStacks(), expectedStacks) << "Expected Voidform stacks: " << expectedStacks

#define EXPECT_SERENDIPITY_STACKS(priest, expectedStacks) \
    EXPECT_EQ(priest->GetSerendipityStacks(), expectedStacks) << "Expected Serendipity stacks: " << expectedStacks

// State assertions
#define EXPECT_IN_SHADOW_FORM(priest) \
    EXPECT_TRUE(priest->IsInShadowForm()) << "Expected priest to be in Shadowform"

#define EXPECT_IN_CHAKRA_SERENITY(priest) \
    EXPECT_TRUE(priest->IsInChakraSerenity()) << "Expected priest to be in Chakra: Serenity"

#define EXPECT_IN_CHAKRA_SANCTUARY(priest) \
    EXPECT_TRUE(priest->IsInChakraSanctuary()) << "Expected priest to be in Chakra: Sanctuary"

// Cooldown assertions
#define EXPECT_SPELL_ON_COOLDOWN(priest, spellId) \
    EXPECT_TRUE(priest->IsSpellOnCooldown(spellId)) << "Expected spell " << spellId << " to be on cooldown"

#define EXPECT_SPELL_OFF_COOLDOWN(priest, spellId) \
    EXPECT_FALSE(priest->IsSpellOnCooldown(spellId)) << "Expected spell " << spellId << " to be off cooldown"

// Performance benchmarking helpers
class PriestPerformanceBenchmark
{
public:
    static void BenchmarkRotationExecution(
        ::std::function<void()> rotationFunc,
        uint32 iterations = 1000,
        uint32 expectedMaxMicroseconds = 50
    )
    {
        auto start = ::std::chrono::high_resolution_clock::now();

        for (uint32 i = 0; i < iterations; ++i)
        {
            rotationFunc();
        }

        auto end = ::std::chrono::high_resolution_clock::now();
        auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - start).count();
        float avgMicroseconds = static_cast<float>(duration) / static_cast<float>(iterations);

        EXPECT_LE(avgMicroseconds, expectedMaxMicroseconds)
            << "Average execution time: " << avgMicroseconds << "µs, expected <" << expectedMaxMicroseconds << "µs";
    }

    static void BenchmarkTargetSelection(
        ::std::function<void*()> selectionFunc,
        uint32 iterations = 10000,
        uint32 expectedMaxMicroseconds = 10
    )
    {
        auto start = ::std::chrono::high_resolution_clock::now();

        for (uint32 i = 0; i < iterations; ++i)
        {
            volatile void* result = selectionFunc();
            (void)result; // Prevent optimization
        }

        auto end = ::std::chrono::high_resolution_clock::now();
        auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - start).count();
        float avgMicroseconds = static_cast<float>(duration) / static_cast<float>(iterations);

        EXPECT_LE(avgMicroseconds, expectedMaxMicroseconds)
            << "Average target selection time: " << avgMicroseconds << "µs, expected <" << expectedMaxMicroseconds << "µs";
    }
};

} // namespace Test
} // namespace Playerbot
