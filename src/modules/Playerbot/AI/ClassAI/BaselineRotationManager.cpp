/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Baseline Rotation Manager Implementation - PACKET-BASED MIGRATION
 * PHASE 0 WEEK 3 (2025-10-30): Migrated to packet-based spell casting
 */

#include "BaselineRotationManager.h"
#include "Player.h"
#include "Pet.h"  // For warlock pet summoning in ApplyBuffs
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Unit.h"
#include "ObjectAccessor.h"
#include "SharedDefines.h"
#include "ObjectGuid.h"
#include "Map.h"
#include "Log.h"  // For TC_LOG_DEBUG
#include "WorldSession.h"  // For bot->GetSession()->QueuePacket()
#include "MotionMaster.h"  // For MoveChase - low-level bot movement
#include "../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries
#include "../../Packets/SpellPacketBuilder.h"  // PHASE 0 WEEK 3: Packet-based spell casting
#include "../Cache/AuraStateCache.h"  // Thread-safe aura state cache for worker threads
#include "GameTime.h"

namespace Playerbot
{

// ============================================================================
// BaselineRotationManager Implementation
// ============================================================================

BaselineRotationManager::BaselineRotationManager()
{
    // Initialize baseline abilities for all classes
    InitializeWarriorBaseline();
    InitializePaladinBaseline();
    InitializeHunterBaseline();
    InitializeRogueBaseline();
    InitializePriestBaseline();
    InitializeDeathKnightBaseline();
    InitializeShamanBaseline();
    InitializeMageBaseline();
    InitializeWarlockBaseline();
    InitializeMonkBaseline();
    InitializeDruidBaseline();
    InitializeDemonHunterBaseline();
    InitializeEvokerBaseline();
}

// ============================================================================
// THREAD-SAFE SPELL CASTING
// ============================================================================
// CRITICAL FIX (2026-01-16): Fixed thread safety crash caused by bot worker threads
// calling bot->CastSpell() directly, which modifies Unit aura state while main thread
// (AreaTrigger::Update → Unit::HasAura) is iterating _appliedAuras.
//
// This method uses SpellPacketBuilder to queue a CMSG_CAST_SPELL packet that will
// be processed safely on the main thread.
// ============================================================================

bool BaselineRotationManager::QueueSpellCast(Player* bot, uint32 spellId, ::Unit* target)
{
    if (!bot || !spellId)
        return false;

    // Default to self-cast if no target specified
    if (!target)
        target = bot;

    // Get session for packet queueing
    WorldSession* session = bot->GetSession();
    if (!session)
    {
        TC_LOG_ERROR("playerbot.baseline", "QueueSpellCast: Bot {} has no session, cannot queue spell {}",
                     bot->GetName(), spellId);
        return false;
    }

    // Build packet with validation
    SpellPacketBuilder::BuildOptions options;
    options.skipGcdCheck = false;      // Respect GCD
    options.skipResourceCheck = false; // Check mana/energy/rage
    options.skipRangeCheck = false;    // Check spell range
    options.logFailures = true;        // Log validation failures

    auto result = SpellPacketBuilder::BuildCastSpellPacket(bot, spellId, target, options);

    if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
    {
        if (result.packet)
        {
            // TrinityCore 12.0: QueuePacket now takes WorldPacket&& instead of WorldPacket*
            session->QueuePacket(std::move(*result.packet));
            TC_LOG_TRACE("playerbot.baseline", "QueueSpellCast: Bot {} queued spell {} on {}",
                         bot->GetName(), spellId, target->GetName());
            return true;
        }
    }
    else
    {
        TC_LOG_TRACE("playerbot.baseline", "QueueSpellCast: Spell {} validation failed for bot {}: {}",
                     spellId, bot->GetName(), result.failureReason);
    }

    return false;
}

bool BaselineRotationManager::ShouldUseBaselineRotation(Player* bot)
{
    if (!bot)
        return false;

    // Use baseline rotation for levels 1-9
    uint8 level = bot->GetLevel();
    if (level < 10)
        return true;

    // Also use if bot is level 10+ but hasn't chosen a specialization
    // (This is an edge case that should trigger auto-specialization)
    // FIXED: GetActiveTalentGroup() returns talent GROUP index (0 or 1), not specialization!
    // Use GetPrimarySpecialization() to correctly detect if bot has a spec assigned.
    ChrSpecialization spec = bot->GetPrimarySpecialization();
    return (level >= 10 && spec == ChrSpecialization::None);
}

bool BaselineRotationManager::ExecuteBaselineRotation(Player* bot, ::Unit* target)
{
    if (!bot || !target || !target->IsAlive())
        return false;

    // CRITICAL FIX: Safety check for worker thread access during bot/target destruction
    // CombatManager::SetInCombatWith (line 194) crashes when accessing _pvpRefs map
    // if the target has been removed from the world but pointer is still valid
    if (!bot->IsInWorld() || !target->IsInWorld())
        return false;

    // CRITICAL FIX: Ensure auto-attack is active before attempting rotation
    // Many low-level bots (level 1-9) don't have spells yet and rely on auto-attack
    // Even when spells are available, auto-attack should always be active in combat
    if (!bot->IsInCombatWith(target))
    {
        bot->SetInCombatWith(target);
        target->SetInCombatWith(bot);
    }

    // Initiate auto-attack if not already attacking this target
    // This is critical for low-level bots and serves as a fallback if no spells work
    if (bot->GetVictim() != target)
    {
        bot->Attack(target, true);
        TC_LOG_DEBUG("module.playerbot.baseline",
                     "Bot {} initiated auto-attack on {} (baseline rotation)",
                     bot->GetName(), target->GetName());
    }

    // ========================================================================
    // MOVEMENT HANDLING REMOVED - Delegated to SoloCombatStrategy
    // ========================================================================
    // CRITICAL FIX: Do NOT call MoveChase here!
    // Movement is now handled exclusively by SoloCombatStrategy::UpdateBehavior()
    // Having both BaselineRotationManager and SoloCombatStrategy call MoveChase
    // causes stutter because the motion type might not update immediately,
    // leading to both systems issuing MoveChase in rapid succession.
    //
    // SoloCombatStrategy has a 500ms throttle and proper motion type checking.
    // ========================================================================
    TC_LOG_TRACE("module.playerbot.baseline",
                 "Bot {} BASELINE - movement handled by SoloCombatStrategy",
                 bot->GetName());

    // Get baseline abilities for bot's class - FIX: use GetClass() not getClass()
    auto abilities = GetBaselineAbilities(bot->GetClass());
    if (!abilities || abilities->empty())
    {
        // No spells available - rely on auto-attack (already initiated above)
        return true; // Return true because auto-attack is active
    }

    // Sort abilities by priority (higher priority first)
    ::std::vector<BaselineAbility> sorted = *abilities;
    ::std::sort(sorted.begin(), sorted.end(), [](auto const& a, auto const& b)
    {
        return a.priority > b.priority;
    });

    // Try to cast highest priority available ability
    for (auto const& ability : sorted)
    {
        if (TryCastAbility(bot, target, ability))

            return true;
    }

    // Even if no spell was cast, auto-attack is active, so return true
    return true;
}

void BaselineRotationManager::ApplyBaselineBuffs(Player* bot)
{
    if (!bot)
        return;

    // FIX: use GetClass() not getClass()
    uint8 classId = bot->GetClass();    // Delegate to class-specific buff application
    switch (classId)
    {
        case CLASS_WARRIOR:

            WarriorBaselineRotation::ApplyBuffs(bot);

            break;
        case CLASS_PALADIN:

            PaladinBaselineRotation::ApplyBuffs(bot);

            break;
        case CLASS_HUNTER:

            HunterBaselineRotation::ApplyBuffs(bot);

            break;
        case CLASS_ROGUE:

            RogueBaselineRotation::ApplyBuffs(bot);

            break;
        case CLASS_PRIEST:

            PriestBaselineRotation::ApplyBuffs(bot);

            break;
        case CLASS_DEATH_KNIGHT:

            DeathKnightBaselineRotation::ApplyBuffs(bot);

            break;
        case CLASS_SHAMAN:

            ShamanBaselineRotation::ApplyBuffs(bot);

            break;
        case CLASS_MAGE:

            MageBaselineRotation::ApplyBuffs(bot);

            break;
        case CLASS_WARLOCK:

            WarlockBaselineRotation::ApplyBuffs(bot);

            break;

        case CLASS_MONK:
        MonkBaselineRotation::ApplyBuffs(bot);

            break;
        case CLASS_DRUID:

            DruidBaselineRotation::ApplyBuffs(bot);

            break;
        case CLASS_DEMON_HUNTER:

            DemonHunterBaselineRotation::ApplyBuffs(bot);

            break;
        case CLASS_EVOKER:

            EvokerBaselineRotation::ApplyBuffs(bot);

            break;
    }
}

bool BaselineRotationManager::HandleAutoSpecialization(Player* bot)
{
    if (!bot)
        return false;

    // Only auto-select at level 10+
    if (bot->GetLevel() < 10)
        return false;

    // Check if already has specialization
    // FIX: Use GetActiveTalentGroup() instead of GetUInt32Value(PLAYER_FIELD_CURRENT_SPEC_ID)
    uint8 activeGroup = bot->GetActiveTalentGroup();
    if (activeGroup != 0)
        return false; // Already has spec

    // Select optimal specialization
    uint32 specId = SelectOptimalSpecialization(bot);
    if (specId == 0)
        return false;

    // DESIGN NOTE: Specialization System
    // TrinityCore 12.0 uses ChrSpecialization.db2 for spec definitions.
    // The BotTalentManager handles specialization selection and learning:
    // - BotTalentManager::SelectSpecialization(specId) sets active spec
    // - BotTalentManager::LearnSpecializationSpells() learns spec spells
    //
    // For bots at level 10+ without a spec:
    // 1. The BotAI checks ShouldUseBaselineRotation() each update
    // 2. If true and level >= 10, BotTalentManager triggers auto-spec
    // 3. This method returns true to indicate spec was selected
    //
    // The actual spec activation is handled by BotTalentManager to maintain
    // separation of concerns - BaselineRotationManager handles pre-spec
    // rotations, BotTalentManager handles spec selection and spell learning.

    // Signal to caller that spec selection should proceed via BotTalentManager
    return true;
}

float BaselineRotationManager::GetBaselineOptimalRange(Player* bot)
{
    if (!bot)
        return 5.0f;

    // FIX: use GetClass() not getClass()
    uint8 classId = bot->GetClass();    // Melee classes
    if (classId == CLASS_WARRIOR || classId == CLASS_ROGUE ||
        classId == CLASS_PALADIN || classId == CLASS_DEATH_KNIGHT ||
        classId == CLASS_MONK || classId == CLASS_DEMON_HUNTER)
    {
        return 5.0f;
    }

    // Ranged classes
    return 25.0f;
}

::std::vector<BaselineAbility> const* BaselineRotationManager::GetBaselineAbilities(uint8 classId) const
{
    auto it = _baselineAbilities.find(classId);
    if (it != _baselineAbilities.end())
        return &it->second;
    return nullptr;
}

bool BaselineRotationManager::TryCastAbility(Player* bot, ::Unit* target, BaselineAbility const& ability)
{
    TC_LOG_ERROR("playerbot.baseline", "TryCastAbility: Bot {} trying spell {} on {}",
                 bot->GetName(), ability.spellId, target->GetName());

    if (!CanUseAbility(bot, target, ability))
    {
        TC_LOG_ERROR("playerbot.baseline", "TryCastAbility: CanUseAbility returned FALSE for spell {}", ability.spellId);
        return false;
    }

    // Check cooldown
    auto& botCooldowns = _cooldowns[bot->GetGUID().GetCounter()];
    auto cdIt = botCooldowns.find(ability.spellId);
    if (cdIt != botCooldowns.end() && cdIt->second > GameTime::GetGameTimeMS())
    {
        TC_LOG_ERROR("playerbot.baseline", "TryCastAbility: Spell {} on cooldown", ability.spellId);
        return false; // On cooldown
    }

    // ARCHITECTURE: Packet-based spell casting (SpellPacketBuilder) replaces direct CastSpell API.
    // Direct API calls from worker threads are unsafe — packets are queued for main-thread processing.

    // Get spell info for validation
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(ability.spellId, bot->GetMap()->GetDifficultyID());
    if (!spellInfo)
    {
        TC_LOG_ERROR("playerbot.baseline", "TryCastAbility: Spell {} NOT FOUND in spell data", ability.spellId);
        return false;
    }

    // DIAGNOSTIC: Log spell power cost calculation
    // This helps debug the "mana cost too low" issue
    {
        auto powerCosts = spellInfo->CalcPowerCost(bot, spellInfo->GetSchoolMask());
        for (auto const& cost : powerCosts)
        {
            TC_LOG_ERROR("playerbot.baseline",
                         "TryCastAbility: Spell {} power cost - Type={}, Amount={}, BotMaxMana={}, BotCurrentMana={}, BotCreateMana={}",
                         ability.spellId, static_cast<int>(cost.Power), cost.Amount,
                         bot->GetMaxPower(POWER_MANA), bot->GetPower(POWER_MANA), bot->GetCreateMana());
        }
        // Log raw spell power data from DBC
        for (SpellPowerEntry const* power : spellInfo->PowerCosts)
        {
            if (power)
            {
                TC_LOG_ERROR("playerbot.baseline",
                             "TryCastAbility: Spell {} RAW POWER DATA - ManaCost={}, PowerCostPct={:.4f}, PowerCostMaxPct={:.4f}, PowerType={}",
                             ability.spellId, power->ManaCost, power->PowerCostPct, power->PowerCostMaxPct, static_cast<int>(power->PowerType));
            }
        }
        if (powerCosts.empty())
        {
            TC_LOG_ERROR("playerbot.baseline",
                         "TryCastAbility: Spell {} has NO power costs! BotLevel={}, SpellLevel={}",
                         ability.spellId, bot->GetLevel(), spellInfo->SpellLevel);
        }
    }

    // Determine cast target
    ::Unit* castTarget = ability.requiresMelee ? target : (ability.isDefensive ? bot : target);

    // Build packet with validation
    SpellPacketBuilder::BuildOptions options;
    options.skipGcdCheck = false;      // Respect GCD
    options.skipResourceCheck = false; // Check mana/energy/rage
    options.skipRangeCheck = false;    // Check spell range

    // CRITICAL FIX: Check castTarget BEFORE building packet
    if (!castTarget)
    {
        TC_LOG_TRACE("playerbot.baseline.packets",
                     "Bot {} cannot cast spell {} - no target",
                     bot->GetName(), ability.spellId);
        return false;
    }

    TC_LOG_ERROR("playerbot.baseline", "TryCastAbility: Building packet for spell {} target {}",
                 ability.spellId, castTarget->GetName());

    auto result = SpellPacketBuilder::BuildCastSpellPacket(bot, ability.spellId, castTarget, options);

    TC_LOG_ERROR("playerbot.baseline", "TryCastAbility: BuildCastSpellPacket result={} reason={}",
                 static_cast<uint8>(result.result), result.failureReason);

    if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
    {
        // CRITICAL FIX: The packet must be queued to the session for execution!
        // Without this, the spell cast never happens - the packet is built but never sent.
        if (result.packet && bot->GetSession())
        {
            // TrinityCore 12.0: QueuePacket now takes WorldPacket&& instead of WorldPacket*
            bot->GetSession()->QueuePacket(std::move(*result.packet));
            TC_LOG_ERROR("playerbot.baseline", "TryCastAbility: QUEUED spell {} packet successfully!",
                         ability.spellId);
        }
        else
        {
            TC_LOG_ERROR("playerbot.baseline",
                         "Bot {} spell {} - packet built but session or packet is null!",
                         bot->GetName(), ability.spellId);
            return false;
        }

        // Optimistic cooldown recording (packet will be processed)
        botCooldowns[ability.spellId] = GameTime::GetGameTimeMS() + ability.cooldown;
        return true;
    }
    else
    {
        // Validation failed - packet not queued
        TC_LOG_ERROR("playerbot.baseline",
                     "TryCastAbility: VALIDATION FAILED for spell {} - result={} reason={}",
                     ability.spellId, static_cast<uint8>(result.result), result.failureReason);
        return false;
    }
}

bool BaselineRotationManager::CanUseAbility(Player* bot, ::Unit* target, BaselineAbility const& ability) const
{
    if (!bot || !target)
        return false;

    // CRITICAL FIX: Check if bot actually knows this spell!
    // Without this check, we try to cast spells the bot hasn't learned yet
    if (!bot->HasSpell(ability.spellId))
    {
        TC_LOG_DEBUG("playerbot.baseline", "CanUseAbility: Bot {} does NOT have spell {} in spellbook",
                     bot->GetName(), ability.spellId);
        return false;
    }

    // Level check
    if (bot->GetLevel() < ability.minLevel)
        return false;

    // Range check
    if (ability.requiresMelee)
    {
        if (bot->GetExactDistSq(target) > (5.0f * 5.0f)) // 25.0f

            return false;
    }
    else
    {

        if (bot->GetExactDistSq(target) > (30.0f * 30.0f) || !bot->IsWithinLOSInMap(target)) // 900.0f
        return false;
    }

    // Resource check
    uint32 currentResource = 0;
    // FIX: use GetClass() not getClass()
    switch (bot->GetClass())
    {
        case CLASS_WARRIOR:
        case CLASS_DRUID: // In some forms

            currentResource = bot->GetPower(POWER_RAGE);
            break;
        case CLASS_ROGUE:
        case CLASS_MONK:

            currentResource = bot->GetPower(POWER_ENERGY);
            break;
        case CLASS_HUNTER:

            currentResource = bot->GetPower(POWER_FOCUS);
            break;
        case CLASS_DEATH_KNIGHT:

            currentResource = bot->GetPower(POWER_RUNIC_POWER);
            break;
        default:

            currentResource = bot->GetPower(POWER_MANA);
            break;
    }

    if (currentResource < ability.resourceCost)
        return false;

    return true;
}

uint32 BaselineRotationManager::SelectOptimalSpecialization(Player* bot) const
{
    if (!bot)
        return 0;

    // Default to first spec for each class
    // In a more advanced implementation, this could consider:
    // - Bot's role preference (tank/dps/healer)
    // - Group composition
    // - Stat distribution

    // FIX: use GetClass() not getClass()
    uint8 classId = bot->GetClass();
    switch (classId)
    {
        case CLASS_WARRIOR:

            return 71; // Arms (71), Fury (72), Protection (73)
        case CLASS_PALADIN:

            return 65; // Holy (65), Protection (66), Retribution (70)
        case CLASS_HUNTER:

            return 253; // Beast Mastery (253), Marksmanship (254), Survival (255)
        case CLASS_ROGUE:

            return 259; // Assassination (259), Combat/Outlaw (260), Subtlety (261)
        case CLASS_PRIEST:

            return 256; // Discipline (256), Holy (257), Shadow (258)
        case CLASS_DEATH_KNIGHT:

            return 250; // Blood (250), Frost (251), Unholy (252)
        case CLASS_SHAMAN:

            return 262; // Elemental (262), Enhancement (263), Restoration (264)
        case CLASS_MAGE:

            return 62; // Arcane (62), Fire (63), Frost (64)
        case CLASS_WARLOCK:

            return 265; // Affliction (265), Demonology (266), Destruction (267)
        case CLASS_MONK:

            return 268; // Brewmaster (268), Windwalker (269), Mistweaver (270)
        case CLASS_DRUID:

            return 102; // Balance (102), Feral (103), Guardian (104), Restoration (105)
        case CLASS_DEMON_HUNTER:

            return 577; // Havoc (577), Vengeance (581)
        case CLASS_EVOKER:

            return 1467; // Devastation (1467), Preservation (1468), Augmentation (1473)
        default:

            return 0;
    }
}

// ============================================================================
// Warrior Baseline Abilities
// ============================================================================

// Define spell IDs (these would normally come from a shared header)
constexpr uint32 EXECUTE = 5308;
constexpr uint32 VICTORY_RUSH = 34428;
constexpr uint32 SLAM = 1464;
constexpr uint32 HAMSTRING = 1715;
constexpr uint32 CHARGE = 100;
constexpr uint32 BATTLE_SHOUT = 6673;

void BaselineRotationManager::InitializeWarriorBaseline()
{
    ::std::vector<BaselineAbility> abilities;

    // Priority order: Execute > Victory Rush > Slam > Hamstring
    abilities.emplace_back(EXECUTE, 9, 15, 0, 10.0f, true);           // Execute (high priority if target low health)
    abilities.emplace_back(VICTORY_RUSH, 3, 0, 0, 9.0f, true);        // Victory Rush (free healing)

    abilities.emplace_back(SLAM, 1, 20, 0, 5.0f, true);
    // Slam (rage dump)
    abilities.emplace_back(HAMSTRING, 7, 10, 0, 3.0f, true);          // Hamstring (slow fleeing enemies)
    abilities.emplace_back(CHARGE, 1, 0, 15000, 15.0f, false);        // Charge (engage)

    _baselineAbilities[CLASS_WARRIOR] = ::std::move(abilities);
}

bool WarriorBaselineRotation::ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager)
{
    // THREAD-SAFETY FIX (2026-01-16): Use QueueSpellCast instead of bot->CastSpell()
    // Direct CastSpell calls from worker threads cause race conditions with main thread
    // Unit::HasAura() iteration, causing ACCESS_VIOLATION crashes.

    // Charge if not in melee range (using squared distance for comparison)
    float distSq = bot->GetExactDistSq(target);
    if (distSq > (8.0f * 8.0f) && distSq < (25.0f * 25.0f)) // 64.0f and 625.0f
    {
        if (bot->HasSpell(CHARGE))
        {
            if (BaselineRotationManager::QueueSpellCast(bot, CHARGE, target))
                return true;
        }
    }

    // Execute if target below 20% health
    if (target->GetHealthPct() <= 20.0f && bot->HasSpell(EXECUTE))
    {
        if (bot->GetPower(POWER_RAGE) >= 15)
        {
            if (BaselineRotationManager::QueueSpellCast(bot, EXECUTE, target))
                return true;
        }
    }

    // Victory Rush for healing (requires Victorious proc)
    // THREAD-SAFETY: Use AuraStateCache instead of direct HasAura() call
    // The cache is populated from main thread before worker threads execute
    constexpr uint32 VICTORIOUS_BUFF = 32216;
    if (bot->HasSpell(VICTORY_RUSH) &&
        sAuraStateCache->HasCachedAura(bot->GetGUID(), VICTORIOUS_BUFF, ObjectGuid::Empty))
    {
        if (BaselineRotationManager::QueueSpellCast(bot, VICTORY_RUSH, target))
            return true;
    }

    // Slam as rage dump
    if (bot->HasSpell(SLAM) && bot->GetPower(POWER_RAGE) >= 20)
    {
        if (BaselineRotationManager::QueueSpellCast(bot, SLAM, target))
            return true;
    }

    // Hamstring to prevent fleeing
    if (bot->HasSpell(HAMSTRING) && bot->GetPower(POWER_RAGE) >= 10)
    {
        if (target->GetHealthPct() < 30.0f)
        {
            if (BaselineRotationManager::QueueSpellCast(bot, HAMSTRING, target))
                return true;
        }
    }

    return false;
}

void WarriorBaselineRotation::ApplyBuffs(Player* bot)
{
    // Battle Shout - only cast if not already buffed
    // THREAD-SAFETY: Use AuraStateCache instead of direct HasAura() call
    // The cache is populated from main thread before worker threads execute
    if (bot->HasSpell(BATTLE_SHOUT) &&
        !sAuraStateCache->HasCachedAura(bot->GetGUID(), BATTLE_SHOUT, ObjectGuid::Empty))
    {
        BaselineRotationManager::QueueSpellCast(bot, BATTLE_SHOUT, bot);
    }
}

// ============================================================================
// Paladin Baseline Abilities
// ============================================================================

void BaselineRotationManager::InitializePaladinBaseline()
{
    ::std::vector<BaselineAbility> abilities;

    abilities.emplace_back(35395, 1, 0, 6000, 10.0f, true);    // Crusader Strike
    abilities.emplace_back(20271, 3, 0, 8000, 9.0f, false);    // Judgment
    abilities.emplace_back(85673, 5, 0, 10000, 7.0f, false);   // Word of Glory (self-heal)
    abilities.emplace_back(853, 9, 0, 60000, 5.0f, false);     // Hammer of Justice (CC)

    _baselineAbilities[CLASS_PALADIN] = ::std::move(abilities);
}

bool PaladinBaselineRotation::ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager)
{
    return manager.ExecuteBaselineRotation(bot, target);
}

void PaladinBaselineRotation::ApplyBuffs(Player* bot)
{
    // No baseline buffs for low-level Paladins
}

// ============================================================================
// Hunter Baseline Abilities
// ============================================================================

void BaselineRotationManager::InitializeHunterBaseline()
{
    ::std::vector<BaselineAbility> abilities;

    abilities.emplace_back(19434, 1, 20, 3000, 10.0f, false);  // Aimed Shot
    abilities.emplace_back(185358, 3, 20, 0, 9.0f, false);     // Arcane Shot
    abilities.emplace_back(34026, 5, 30, 7500, 8.0f, false);   // Kill Command
    abilities.emplace_back(56641, 9, 0, 0, 5.0f, false);       // Steady Shot (focus builder)

    _baselineAbilities[CLASS_HUNTER] = ::std::move(abilities);
}

bool HunterBaselineRotation::ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager)
{
    return manager.ExecuteBaselineRotation(bot, target);
}

void HunterBaselineRotation::ApplyBuffs(Player* bot)
{
    // No baseline buffs for low-level Hunters
}

// ============================================================================
// Additional Class Rotations (delegation pattern)
//
// These class-specific ExecuteRotation methods delegate to the manager's
// ExecuteBaselineRotation() which handles the actual rotation logic using
// the baseline abilities initialized for each class. This is an intentional
// design pattern - all class-specific logic is encapsulated in the initialized
// abilities (spell IDs, priorities, costs, cooldowns) while the manager
// provides the common rotation execution framework.
//
// The Warrior class demonstrates an alternative approach with direct spell
// casting logic, which can be used when more complex rotation logic is needed.
// ============================================================================

void BaselineRotationManager::InitializeRogueBaseline()
{
    ::std::vector<BaselineAbility> abilities;
    // WoW 12.0 (The War Within) spell IDs:
    // - Sinister Strike: 193315 (retail), NOT 1752 (classic)
    // - Eviscerate: 196819 (retail)
    abilities.emplace_back(193315, 1, 40, 0, 10.0f, true);  // Sinister Strike (retail)
    abilities.emplace_back(196819, 3, 35, 0, 9.0f, true);   // Eviscerate (finisher)
    _baselineAbilities[CLASS_ROGUE] = ::std::move(abilities);
}

void BaselineRotationManager::InitializePriestBaseline()
{
    ::std::vector<BaselineAbility> abilities;
    abilities.emplace_back(585, 1, 0, 0, 10.0f, false);  // Smite
    abilities.emplace_back(589, 1, 0, 0, 9.0f, false);   // Shadow Word: Pain
    _baselineAbilities[CLASS_PRIEST] = ::std::move(abilities);
}

void BaselineRotationManager::InitializeDeathKnightBaseline()
{
    ::std::vector<BaselineAbility> abilities;
    abilities.emplace_back(49998, 8, 40, 0, 10.0f, true);  // Death Strike
    abilities.emplace_back(45477, 8, 0, 8000, 9.0f, false); // Icy Touch
    _baselineAbilities[CLASS_DEATH_KNIGHT] = ::std::move(abilities);
}

void BaselineRotationManager::InitializeShamanBaseline()
{
    ::std::vector<BaselineAbility> abilities;
    // WoW 12.0 (The War Within) spell IDs:
    // - Lightning Bolt: 188196 (retail), NOT 403 (classic)
    // - Primal Strike: 73899 (retail)
    abilities.emplace_back(188196, 1, 0, 0, 10.0f, false);  // Lightning Bolt (retail)
    abilities.emplace_back(73899, 1, 0, 0, 9.0f, true);     // Primal Strike
    _baselineAbilities[CLASS_SHAMAN] = ::std::move(abilities);
}

void BaselineRotationManager::InitializeMageBaseline()
{
    ::std::vector<BaselineAbility> abilities;
    abilities.emplace_back(116, 1, 0, 0, 10.0f, false);    // Frostbolt
    abilities.emplace_back(133, 1, 0, 0, 9.0f, false);     // Fireball
    _baselineAbilities[CLASS_MAGE] = ::std::move(abilities);
}

void BaselineRotationManager::InitializeWarlockBaseline()
{
    ::std::vector<BaselineAbility> abilities;
    // WoW 12.0 (The War Within) spell IDs:
    // - Shadow Bolt: 686 (same as classic)
    // - Corruption: 172 (same as classic)
    abilities.emplace_back(686, 1, 0, 0, 10.0f, false);    // Shadow Bolt
    abilities.emplace_back(172, 1, 0, 0, 9.0f, false);     // Corruption
    _baselineAbilities[CLASS_WARLOCK] = ::std::move(abilities);
}

void BaselineRotationManager::InitializeMonkBaseline()
{
    ::std::vector<BaselineAbility> abilities;
    abilities.emplace_back(100780, 1, 50, 0, 10.0f, true);  // Tiger Palm
    abilities.emplace_back(100784, 1, 40, 0, 9.0f, true);   // Blackout Kick
    _baselineAbilities[CLASS_MONK] = ::std::move(abilities);
}

void BaselineRotationManager::InitializeDruidBaseline()
{
    ::std::vector<BaselineAbility> abilities;
    abilities.emplace_back(5176, 1, 0, 0, 10.0f, false);    // Wrath
    abilities.emplace_back(8921, 1, 0, 0, 9.0f, false);     // Moonfire
    _baselineAbilities[CLASS_DRUID] = ::std::move(abilities);
}

void BaselineRotationManager::InitializeDemonHunterBaseline()
{
    ::std::vector<BaselineAbility> abilities;
    abilities.emplace_back(162243, 8, 40, 0, 10.0f, true);  // Demon's Bite
    abilities.emplace_back(162794, 8, 40, 0, 9.0f, true);   // Chaos Strike
    _baselineAbilities[CLASS_DEMON_HUNTER] = ::std::move(abilities);
}

void BaselineRotationManager::InitializeEvokerBaseline()
{
    ::std::vector<BaselineAbility> abilities;
    abilities.emplace_back(361469, 1, 0, 0, 10.0f, false);  // Azure Strike
    abilities.emplace_back(361500, 1, 0, 0, 9.0f, false);   // Living Flame
    _baselineAbilities[CLASS_EVOKER] = ::std::move(abilities);
}

// Stub implementations for other baseline rotations
bool RogueBaselineRotation::ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager)
{
    return manager.ExecuteBaselineRotation(bot, target);
}

void RogueBaselineRotation::ApplyBuffs(Player* bot) {}

bool PriestBaselineRotation::ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager)
{
    return manager.ExecuteBaselineRotation(bot, target);
}

void PriestBaselineRotation::ApplyBuffs(Player* bot) {}

bool DeathKnightBaselineRotation::ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager)
{
    return manager.ExecuteBaselineRotation(bot, target);
}

void DeathKnightBaselineRotation::ApplyBuffs(Player* bot) {}

bool ShamanBaselineRotation::ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager)
{
    return manager.ExecuteBaselineRotation(bot, target);
}

void ShamanBaselineRotation::ApplyBuffs(Player* bot) {}

bool MageBaselineRotation::ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager)
{
    return manager.ExecuteBaselineRotation(bot, target);
}

void MageBaselineRotation::ApplyBuffs(Player* bot) {}

bool WarlockBaselineRotation::ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager)
{
    return manager.ExecuteBaselineRotation(bot, target);
}

void WarlockBaselineRotation::ApplyBuffs(Player* bot)
{
    if (!bot || !bot->IsAlive())
        return;

    // Don't summon pets while already casting
    if (bot->HasUnitState(UNIT_STATE_CASTING))
        return;

    // ========================================================================
    // WARLOCK PET SUMMONING - Must happen OUT OF COMBAT
    // Pet summons have 6 second cast time - impossible in combat!
    // Level 3: Imp (ranged fire DPS)
    // Level 10: Voidwalker (melee tank - preferred for solo leveling)
    //
    // THREAD-SAFETY FIX (2026-01-16): Use QueueSpellCast instead of bot->CastSpell()
    // ========================================================================

    // Warlock pet spell IDs (WoW 12.0)
    constexpr uint32 SUMMON_IMP = 688;
    constexpr uint32 SUMMON_VOIDWALKER = 697;

    Pet* pet = bot->GetPet();
    if (pet && pet->IsAlive())
    {
        TC_LOG_DEBUG("playerbot.baseline", "WarlockApplyBuffs: {} already has pet {} (entry {})",
                     bot->GetName(), pet->GetName(), pet->GetEntry());
        return;  // Already have a pet
    }

    // Prefer Voidwalker (level 10+) for solo leveling - it tanks!
    if (bot->GetLevel() >= 10 && bot->HasSpell(SUMMON_VOIDWALKER))
    {
        TC_LOG_INFO("playerbot.baseline", "WarlockApplyBuffs: {} summoning Voidwalker (out of combat)",
                    bot->GetName());
        BaselineRotationManager::QueueSpellCast(bot, SUMMON_VOIDWALKER, bot);
        return;
    }

    // Fallback to Imp (level 3+)
    if (bot->HasSpell(SUMMON_IMP))
    {
        TC_LOG_INFO("playerbot.baseline", "WarlockApplyBuffs: {} summoning Imp (out of combat)",
                    bot->GetName());
        BaselineRotationManager::QueueSpellCast(bot, SUMMON_IMP, bot);
        return;
    }

    TC_LOG_DEBUG("playerbot.baseline", "WarlockApplyBuffs: {} has no pet summon spells yet (level {})",
                 bot->GetName(), bot->GetLevel());
}

bool MonkBaselineRotation::ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager)
{
    return manager.ExecuteBaselineRotation(bot, target);
}

void MonkBaselineRotation::ApplyBuffs(Player* bot) {}

bool DruidBaselineRotation::ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager)
{
    return manager.ExecuteBaselineRotation(bot, target);
}

void DruidBaselineRotation::ApplyBuffs(Player* bot) {}

bool DemonHunterBaselineRotation::ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager)
{
    return manager.ExecuteBaselineRotation(bot, target);
}

void DemonHunterBaselineRotation::ApplyBuffs(Player* bot) {}

bool EvokerBaselineRotation::ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager)
{
    return manager.ExecuteBaselineRotation(bot, target);
}

void EvokerBaselineRotation::ApplyBuffs(Player* bot) {}

} // namespace Playerbot