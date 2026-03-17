/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "WarlockAI.h"
#include "AfflictionWarlock.h"
#include "DemonologyWarlock.h"
#include "DestructionWarlock.h"
#include "../SpellValidation_WoW120_Part2.h"
#include "GameTime.h"
#include "../../Combat/CombatBehaviorIntegration.h"
#include "Player.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "SpellAuras.h"
#include "Pet.h"
#include "ObjectAccessor.h"
#include "Map.h"
#include "Group.h"
#include "Log.h"
#include "World.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Cell.h"
#include "CellImpl.h"
#include "SpellHistory.h"
#include "CreatureAI.h"
#include "MotionMaster.h"
#include "../../../AI/Combat/BotThreatManager.h"
#include "../../../AI/Combat/TargetSelector.h"
#include "../../../AI/Combat/PositionManager.h"
#include "../../../AI/Combat/InterruptManager.h"
#include "../BaselineRotationManager.h"
#include "../../../Spatial/SpatialGridManager.h"
#include "../../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries

namespace Playerbot
{

// Destructor must be defined in cpp file where specialization types are complete
WarlockAI::~WarlockAI() = default;

// ============================================================================
// Warlock Spell IDs - Using Central SpellValidation Registry (WoW 12.0)
// ============================================================================
// All spell IDs sourced from SpellValidation_WoW120_Part2.h to maintain single source of truth

// Namespace aliases for cleaner code
namespace WarlockSpells = WoW120Spells::Warlock;
namespace WarlockAffliction = WoW120Spells::Warlock::Affliction;
namespace WarlockDemonology = WoW120Spells::Warlock::Demonology;
namespace WarlockDestruction = WoW120Spells::Warlock::Destruction;
namespace WarlockCommon = WoW120Spells::Warlock::Common;

// Legacy spell ID aliases for backward compatibility with existing code
// These map old WARLOCK_ prefixed names to the central registry

// Core Abilities (All Specs)
static constexpr uint32 WARLOCK_SHADOW_BOLT = WarlockSpells::SHADOW_BOLT;
static constexpr uint32 WARLOCK_SHADOWBOLT = WarlockSpells::SHADOW_BOLT;  // Alias
static constexpr uint32 WARLOCK_CORRUPTION = WarlockSpells::CORRUPTION;
static constexpr uint32 WARLOCK_LIFE_TAP = WarlockSpells::LIFE_TAP;
static constexpr uint32 WARLOCK_DRAIN_LIFE = WarlockSpells::DRAIN_LIFE;
static constexpr uint32 WARLOCK_HEALTH_FUNNEL = WarlockSpells::HEALTH_FUNNEL;
static constexpr uint32 WARLOCK_UNENDING_RESOLVE = WarlockSpells::UNENDING_RESOLVE;
static constexpr uint32 WARLOCK_CREATE_SOULSTONE = WarlockSpells::SOULSTONE;
static constexpr uint32 WARLOCK_CREATE_HEALTHSTONE = WarlockSpells::CREATE_HEALTHSTONE;
static constexpr uint32 WARLOCK_RITUAL_OF_SUMMONING = WarlockSpells::RITUAL_OF_SUMMONING;
static constexpr uint32 WARLOCK_EYE_OF_KILROGG = WarlockSpells::EYE_OF_KILROGG;
static constexpr uint32 WARLOCK_UNENDING_BREATH = WarlockSpells::UNENDING_BREATH;

// Pet Summons
static constexpr uint32 WARLOCK_SUMMON_IMP = WarlockSpells::SUMMON_IMP;
static constexpr uint32 WARLOCK_SUMMON_VOIDWALKER = WarlockSpells::SUMMON_VOIDWALKER;
static constexpr uint32 WARLOCK_SUMMON_SUCCUBUS = WarlockSpells::SUMMON_SUCCUBUS;
static constexpr uint32 WARLOCK_SUMMON_FELHUNTER = WarlockSpells::SUMMON_FELHUNTER;
static constexpr uint32 WARLOCK_SUMMON_FELGUARD = WarlockSpells::SUMMON_FELGUARD;
static constexpr uint32 WARLOCK_SUMMON_INFERNAL = WarlockDestruction::SUMMON_INFERNAL;

// CC and Interrupts
static constexpr uint32 WARLOCK_SPELL_LOCK = WarlockSpells::SPELL_LOCK;
static constexpr uint32 WARLOCK_FEAR = WarlockSpells::FEAR;
static constexpr uint32 WARLOCK_HOWL_OF_TERROR = WarlockSpells::HOWL_OF_TERROR;
static constexpr uint32 WARLOCK_DEATH_COIL = WarlockSpells::MORTAL_COIL;  // Renamed in retail
static constexpr uint32 WARLOCK_SHADOWFURY = WarlockSpells::SHADOWFURY;
static constexpr uint32 WARLOCK_BANISH = WarlockSpells::BANISH;

// Curses
static constexpr uint32 WARLOCK_CURSE_OF_TONGUES = WarlockSpells::CURSE_OF_TONGUES;
static constexpr uint32 WARLOCK_CURSE_OF_WEAKNESS = WarlockSpells::CURSE_OF_WEAKNESS;
static constexpr uint32 WARLOCK_CURSE_OF_EXHAUSTION = WarlockSpells::CURSE_OF_EXHAUSTION;

// Affliction Specialization aliases
static constexpr uint32 WARLOCK_CURSE_OF_AGONY = WarlockAffliction::AGONY;  // Renamed to Agony
static constexpr uint32 WARLOCK_UNSTABLE_AFFLICTION = WarlockAffliction::UNSTABLE_AFFLICTION;
static constexpr uint32 WARLOCK_SEED_OF_CORRUPTION = WarlockAffliction::SEED_OF_CORRUPTION;
static constexpr uint32 WARLOCK_HAUNT = WarlockAffliction::HAUNT;
static constexpr uint32 WARLOCK_DARK_SOUL_MISERY = WarlockAffliction::DARK_SOUL_MISERY;
static constexpr uint32 WARLOCK_DARK_PACT = WarlockAffliction::DARK_PACT;

// Demonology Specialization aliases
static constexpr uint32 WARLOCK_HAND_OF_GULDAN = WarlockDemonology::HAND_OF_GULDAN;

// Destruction Specialization aliases
static constexpr uint32 WARLOCK_IMMOLATE = WarlockDestruction::IMMOLATE;
static constexpr uint32 WARLOCK_CONFLAGRATE = WarlockDestruction::CONFLAGRATE;
static constexpr uint32 WARLOCK_RAIN_OF_FIRE = WarlockDestruction::RAIN_OF_FIRE;
static constexpr uint32 WARLOCK_SHADOWBURN = WarlockDestruction::SHADOWBURN;
static constexpr uint32 WARLOCK_CATACLYSM = WarlockDestruction::CATACLYSM;
static constexpr uint32 WARLOCK_DARK_SOUL_INSTABILITY = WarlockDestruction::DARK_SOUL_INSTABILITY;
static constexpr uint32 WARLOCK_FIRE_AND_BRIMSTONE = WarlockDestruction::FIRE_AND_BRIMSTONE;
static constexpr uint32 WARLOCK_CHAOS_BOLT = WarlockDestruction::CHAOS_BOLT;
static constexpr uint32 WARLOCK_INCINERATE = WarlockDestruction::INCINERATE;

// Constructor with proper member initialization matching the header
WarlockAI::WarlockAI(Player* bot) :
    ClassAI(bot),
    _warlockMetrics{},
    _threatManager(::std::make_unique<BotThreatManager>(bot)),
    _targetSelector(::std::make_unique<TargetSelector>(bot, _threatManager.get())),
    _positionManager(::std::make_unique<PositionManager>(bot, _threatManager.get())),
    _interruptManager(::std::make_unique<InterruptManager>(bot)),
    _currentSoulShards(0),
    _petActive(false),
    _petHealthPercent(0),
    _lastPetCheck(::std::chrono::steady_clock::now()),
    _optimalManaThreshold(0.4f),
    _lowManaMode(false),
    _lastLifeTapTime(0),
    _manaSpent(0),
    _damageDealt(0),
    _soulshardsUsed(0),
    _fearsUsed(0),
    _petsSpawned(0),
    _lastFear(0),
    _lastPetSummon(0)
{
    // Reset metrics
    _warlockMetrics.Reset();

    // Initialize tracking maps
    _dotTracker.clear();
    _petAbilityCooldowns.clear();

    TC_LOG_DEBUG("playerbot.warlock", "WarlockAI initialized for {} with specialization {}",
                 GetBot()->GetName(), static_cast<uint32>(GetBot()->GetPrimarySpecialization()));
}

void WarlockAI::UpdateRotation(::Unit* target)
{
    // CRITICAL: Use module.playerbot logger to prove function entry
    TC_LOG_ERROR("module.playerbot", " WARLOCK UpdateRotation() ENTERED! ");

    Player* bot = GetBot();

    float distance = ::std::sqrt(bot->GetExactDistSq(target)); // Calculate once from squared distance
    TC_LOG_ERROR("module.playerbot", " WarlockAI::UpdateRotation - Bot {} (level {}) attacking {} at {:.1f}yd",

                 bot->GetName(), bot->GetLevel(), target->GetName(), distance);

    // NOTE: Baseline rotation check is now handled at the dispatch level in
    // ClassAI::OnCombatUpdate(). This method is ONLY called when the bot has
    // already chosen a specialization (level 10+ with talents).

    // ========================================================================
    // COMBAT BEHAVIOR INTEGRATION - Full 10 Priority System
    // ========================================================================
    auto* behaviors = GetCombatBehaviors();

    // Update combat metrics
    auto now = ::std::chrono::steady_clock::now();
    if (::std::chrono::duration_cast<::std::chrono::milliseconds>(now - _warlockMetrics.lastUpdate).count() > COMBAT_METRICS_UPDATE_INTERVAL)
    {
        _warlockMetrics.lastUpdate = now;
        UpdateCombatMetrics();
    }

    // Priority 1: Interrupts - Spell Lock (pet ability)
    if (behaviors && behaviors->ShouldInterrupt(target))
    {
        Unit* interruptTarget = behaviors->GetInterruptTarget();
        if (interruptTarget && HandleInterrupt(interruptTarget))
        {

            TC_LOG_DEBUG("playerbot.warlock", "Warlock {} interrupted {} with Spell Lock",

                         bot->GetName(), interruptTarget->GetName());

            return;
        }
    }

    // Priority 2: Defensives - Unending Resolve, Dark Pact, Soul Leech
    if (behaviors && behaviors->NeedsDefensive())
    {
        if (HandleDefensives())
        {

            TC_LOG_DEBUG("playerbot.warlock", "Warlock {} using defensive abilities", bot->GetName());

            return;
        }
    }

// Priority 3: Positioning - Maintain max range
    if (behaviors && behaviors->NeedsRepositioning())
    {
        Position optimalPos = behaviors->GetOptimalPosition();
        // Movement is handled by BotAI strategies, but we can cast instant spells while moving
    if (bot->isMoving())
        {
            // Use instant casts while repositioning
    if (HandleInstantCasts(target))

                return;
        }
    }

// Priority 4: Pet Management - Summon, heal, command
    if (HandlePetManagement())
    {
        TC_LOG_DEBUG("playerbot.warlock", "Warlock {} managing pet", bot->GetName());
        return;
    }

    // Priority 5: Target Switching - Priority targets
    if (behaviors && behaviors->ShouldSwitchTarget())
    {
        Unit* priorityTarget = behaviors->GetPriorityTarget();
        if (priorityTarget && priorityTarget != target)

        {
        // Apply a DoT/Curse to old target before switching
    if (ApplyDoTToTarget(target))

            {

                TC_LOG_DEBUG("playerbot.warlock", "Applied DoT to {} before switching", target->GetName());

            }

            // Switch to priority target

            OnTargetChanged(priorityTarget);

            target = priorityTarget;
            

            TC_LOG_DEBUG("playerbot.warlock", "Warlock {} switching to priority target {}",

                         bot->GetName(), priorityTarget->GetName());
        }
    }

    // Priority 6: Crowd Control - Fear, Banish for secondary targets
    if (behaviors && behaviors->ShouldUseCrowdControl())
    {
        Unit* ccTarget = behaviors->GetCrowdControlTarget();
        if (ccTarget && ccTarget != target && HandleCrowdControl(ccTarget))

        {
        TC_LOG_DEBUG("playerbot.warlock", "Warlock {} crowd controlling {}",

                         bot->GetName(), ccTarget->GetName());

            return;
        }
    }

    // Priority 7: AoE Decisions - Seed of Corruption, Rain of Fire, Cataclysm
    if (behaviors && behaviors->ShouldAOE())
    {
        if (HandleAoERotation(target))
        {

            TC_LOG_DEBUG("playerbot.warlock", "Warlock {} executing AoE rotation", bot->GetName());

            return;
        }
    }

    // Priority 8: Offensive Cooldowns - Dark Soul, Summon Infernal/Doomguard
    if (behaviors && behaviors->ShouldUseCooldowns())
    {
        if (HandleOffensiveCooldowns(target))
        {

            TC_LOG_DEBUG("playerbot.warlock", "Warlock {} using offensive cooldowns", bot->GetName());
        }
    }

// Priority 9: Soul Shard Management - Efficient shard generation and spending
    HandleSoulShardManagement();

    // Priority 10: Normal Rotation - Cast basic damage spells
    // CRITICAL FIX: This was completely empty! Just incremented a counter but never cast anything!
    // For level 10+ warlocks, we execute a simple Shadow Bolt / Corruption rotation
    ExecuteBasicRotation(target);
}

bool WarlockAI::HandleInterrupt(Unit* target)
{
    if (!target || !target->IsNonMeleeSpellCast(false))
        return false;

    Player* bot = GetBot();
    if (!bot)
        return false;

    // Spell Lock - Felhunter ability
    Pet* pet = bot->GetPet();
    if (pet && pet->IsAlive())
    {
        // Check if pet is a Felhunter (entry 417)
    if (pet->GetEntry() == 417 || pet->GetEntry() == 17252) // Felhunter or Fel Guard
        {
            // Command pet to use Spell Lock (19647)
    if (!_petAbilityCooldowns[WARLOCK_SPELL_LOCK] || GameTime::GetGameTimeMS() - _petAbilityCooldowns[WARLOCK_SPELL_LOCK] > 24000)

            {

                pet->CastSpell(CastSpellTargetArg(target), WARLOCK_SPELL_LOCK);

                _petAbilityCooldowns[WARLOCK_SPELL_LOCK] = GameTime::GetGameTimeMS();

                return true;

            }
        }
    }

    // Shadowfury - stun interrupt
    if (bot->HasSpell(WARLOCK_SHADOWFURY) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_SHADOWFURY))
    {
        float distance = ::std::sqrt(bot->GetExactDistSq(target)); // Calculate once from squared distance
    if (distance <= 30.0f)
        {

            bot->CastSpell(CastSpellTargetArg(target), WARLOCK_SHADOWFURY);

            return true;
        }
    }

    return false;
}

bool WarlockAI::HandleDefensives()
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    float healthPct = bot->GetHealthPct();    // Critical health - use strongest defensives
    if (healthPct < 20.0f)
    {
        // Unending Resolve
    if (bot->HasSpell(WARLOCK_UNENDING_RESOLVE) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_UNENDING_RESOLVE))
        {

            bot->CastSpell(CastSpellTargetArg(bot), WARLOCK_UNENDING_RESOLVE);

            return true;
        }

        // Dark Pact
    if (bot->HasSpell(WARLOCK_DARK_PACT) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_DARK_PACT))
        {

            bot->CastSpell(CastSpellTargetArg(bot), WARLOCK_DARK_PACT);

            return true;
        }

        // Healthstone
    if (UseHealthstone())

            return true;
    }

// Low health - use moderate defensives (WoW 12.0)
    if (healthPct < 40.0f)
    {
        // Dark Pact for shield (Affliction)
        if (bot->HasSpell(WARLOCK_DARK_PACT) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_DARK_PACT))
        {
            bot->CastSpell(CastSpellTargetArg(bot), WARLOCK_DARK_PACT);
            return true;
        }

// Mortal Coil for heal + fear (was Death Coil)
    if (bot->HasSpell(WARLOCK_DEATH_COIL) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_DEATH_COIL))

        {
        Unit* nearestEnemy = GetNearestEnemy(8.0f);

            if (nearestEnemy)
            {

                bot->CastSpell(CastSpellTargetArg(nearestEnemy), WARLOCK_DEATH_COIL);

                return true;

            }
        }

        
        // Howl of Terror for AoE fear
    if (bot->HasSpell(WARLOCK_HOWL_OF_TERROR) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_HOWL_OF_TERROR))
        {

            if (GetNearbyEnemyCount(10.0f) >= 2)
            {
            bot->CastSpell(CastSpellTargetArg(bot), WARLOCK_HOWL_OF_TERROR);

                return true;
                }
        }

// Drain Life for healing
        Unit* target = bot->GetVictim();
        if (target && bot->HasSpell(WARLOCK_DRAIN_LIFE) && !bot->IsNonMeleeSpellCast(false))
        {

            bot->CastSpell(CastSpellTargetArg(target), WARLOCK_DRAIN_LIFE);

            return true;
        }
    }

    return false;
}

bool WarlockAI::HandlePetManagement()
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    Pet* pet = bot->GetPet();    // Check if we need to summon a pet
    if (!pet || !pet->IsAlive())
    {
        return SummonPet();
    }

    // Update pet status
    _petActive = true;
    _petHealthPercent = static_cast<uint32>(pet->GetHealthPct());

    // ========================================================================
    // CRITICAL FIX: Ensure bot pets are always in DEFENSIVE mode
    // ========================================================================
    // Pet react state is loaded from database on spawn. If it was saved as PASSIVE,
    // the pet will not defend itself or its master when attacked.
    // Bot pets should ALWAYS be DEFENSIVE so they auto-attack when:
    // - The bot is attacked
    // - The pet itself is attacked
    // - The bot issues an attack command
    // ========================================================================
    if (pet->HasReactState(REACT_PASSIVE))
    {
        TC_LOG_DEBUG("playerbot.warlock", "Warlock {} pet {} was PASSIVE, setting to DEFENSIVE",
                     bot->GetName(), pet->GetName());
        pet->SetReactState(REACT_DEFENSIVE);
    }

    // Heal pet if needed
    if (_petHealthPercent.load() < 50)
    {
        // Health Funnel - keep pet healthy
        if (bot->HasSpell(WARLOCK_HEALTH_FUNNEL) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_HEALTH_FUNNEL))
        {
            bot->CastSpell(CastSpellTargetArg(pet), WARLOCK_HEALTH_FUNNEL);
            return true;
        }
    }
    // Note: Consume Shadows and Demonic Empowerment removed in WoW 12.0

    // Command pet to attack if idle
    Unit* target = bot->GetVictim();
    if (target && (!pet->GetVictim() || pet->GetVictim() != target))
    {
        pet->AI()->AttackStart(target);
    }

    return false;
}

bool WarlockAI::SummonPet()
{
    Player* bot = GetBot();
    if (!bot || bot->IsNonMeleeSpellCast(false))
        return false;

    // Don't summon in combat unless we have enough time
    if (bot->IsInCombat() && GetNearbyEnemyCount(5.0f) > 0)
        return false;

    // ========================================================================
    // CRITICAL FIX: Stop movement before casting summon spells
    // ========================================================================
    // Pet summon spells have 6-second cast times and REQUIRE standing still.
    // If the bot is moving, the cast will fail with SPELL_FAILED_MOVING.
    // We must stop movement and clear ALL movement flags/state.
    // ========================================================================

    // Check for ANY movement state (even if visually not moving, flags may be set)
    // Use TrinityCore 12.0 predefined masks for movement detection
    bool hasMovementFlags = bot->HasUnitMovementFlag(MOVEMENTFLAG_MASK_MOVING | MOVEMENTFLAG_MASK_TURNING);

    if (bot->isMoving() || hasMovementFlags || bot->HasUnitState(UNIT_STATE_MOVING))
    {
        // Aggressively clear ALL movement state
        bot->StopMoving();
        bot->GetMotionMaster()->Clear(MOTION_PRIORITY_NORMAL);
        bot->GetMotionMaster()->MoveIdle();

        // Clear movement flags using TrinityCore 12.0 masks
        bot->RemoveUnitMovementFlag(MOVEMENTFLAG_MASK_MOVING | MOVEMENTFLAG_MASK_TURNING);

        // Clear unit state
        bot->ClearUnitState(UNIT_STATE_MOVING | UNIT_STATE_CHASE | UNIT_STATE_FOLLOW);

        TC_LOG_DEBUG("playerbot.warlock", "Warlock {} clearing movement state to summon pet (isMoving={}, hasFlags={}, hasState={})",
                     bot->GetName(), bot->isMoving(), hasMovementFlags, bot->HasUnitState(UNIT_STATE_MOVING));

        // Return false - we'll try again next update when fully stopped
        return false;
    }

    uint32 summonSpell = 0;
    uint32 spec = static_cast<uint32>(bot->GetPrimarySpecialization());

    // Determine if bot is solo or in a group
    bool isInGroup = bot->GetGroup() != nullptr;
    bool isSolo = !isInGroup;

    // ========================================================================
    // PET SELECTION LOGIC - Context-aware (Solo vs Group)
    // ========================================================================
    // Solo: Prefer tanky pets (Voidwalker/Felguard) - bot needs survivability
    // Group: Can use utility pets (Felhunter for interrupts) or DPS pets (Imp)
    //
    // Spec IDs: 265 = Affliction, 266 = Demonology, 267 = Destruction
    // ========================================================================

    if (static_cast<uint32>(spec) == 266) // Demonology
    {
        // Demonology ALWAYS uses Felguard if available - it's their signature pet
        // Felguard is tanky AND does great damage
        if (bot->HasSpell(WARLOCK_SUMMON_FELGUARD))
            summonSpell = WARLOCK_SUMMON_FELGUARD;
        else
            summonSpell = WARLOCK_SUMMON_VOIDWALKER;
    }
    else if (isSolo)
    {
        // SOLO CONTENT: Voidwalker for ALL non-Demo specs
        // - Voidwalker tanks mobs while Warlock does damage
        // - Affliction DoTs + Voidwalker tank = safe leveling
        // - Destruction burst + Voidwalker tank = safe questing
        if (bot->HasSpell(WARLOCK_SUMMON_VOIDWALKER))
            summonSpell = WARLOCK_SUMMON_VOIDWALKER;
        else if (bot->HasSpell(WARLOCK_SUMMON_IMP))
            summonSpell = WARLOCK_SUMMON_IMP; // Fallback for low level
    }
    else // In group
    {
        // GROUP CONTENT: Utility or DPS pets based on spec
        if (static_cast<uint32>(spec) == 265) // Affliction
        {
            // Felhunter for Spell Lock (interrupt) and Devour Magic (dispel)
            // Useful in dungeons/raids for interrupt rotation
            if (bot->HasSpell(WARLOCK_SUMMON_FELHUNTER))
                summonSpell = WARLOCK_SUMMON_FELHUNTER;
            else
                summonSpell = WARLOCK_SUMMON_IMP;
        }
        else if (static_cast<uint32>(spec) == 267) // Destruction
        {
            // Imp for extra DPS in group content
            // Tank is handling aggro, so Warlock can maximize damage
            summonSpell = WARLOCK_SUMMON_IMP;
        }
    }    
    // Fallback to basic pets if specialized ones aren't available
    if (!bot->HasSpell(summonSpell))
    
    {

        if (bot->HasSpell(WARLOCK_SUMMON_IMP))

            summonSpell = WARLOCK_SUMMON_IMP;
        else if (bot->HasSpell(WARLOCK_SUMMON_VOIDWALKER))

            summonSpell = WARLOCK_SUMMON_VOIDWALKER;
            }
    if (summonSpell && !bot->GetSpellHistory()->HasCooldown(summonSpell))
    {
        // ========================================================================
        // CRITICAL FIX: Basic pet summons do NOT cost soul shards in modern WoW!
        // ========================================================================
        // In WoW 12.0 (The War Within), Summon Imp/Voidwalker/Succubus/Felhunter
        // are FREE to cast - they only have a cast time (6 seconds).
        // Soul Shards are ONLY used for combat abilities (Chaos Bolt, etc.).
        // The previous check for "soulShards >= 10" was blocking ALL pet summons!
        // ========================================================================

        TC_LOG_INFO("playerbot.warlock", "🐾 Warlock {} summoning pet with spell {} (level {})",
                    bot->GetName(), summonSpell, bot->GetLevel());

        SpellCastResult result = bot->CastSpell(CastSpellTargetArg(bot), summonSpell);

        if (result == SPELL_CAST_OK)
        {
            _lastPetSummon = GameTime::GetGameTimeMS();
            _petsSpawned++;
            TC_LOG_INFO("playerbot.warlock", "✅ Warlock {} pet summon started successfully (spell {})",
                        bot->GetName(), summonSpell);
            return true;
        }
        else
        {
            TC_LOG_ERROR("playerbot.warlock", "❌ Warlock {} pet summon FAILED - spell {} result {}",
                         bot->GetName(), summonSpell, static_cast<int>(result));
        }
    }
    else if (summonSpell == 0)
    {
        TC_LOG_ERROR("playerbot.warlock", "❌ Warlock {} has no pet summon spell! Level={}, HasImp={}, HasVW={}",
                     bot->GetName(), bot->GetLevel(),
                     bot->HasSpell(WARLOCK_SUMMON_IMP) ? "YES" : "NO",
                     bot->HasSpell(WARLOCK_SUMMON_VOIDWALKER) ? "YES" : "NO");
    }

    return false;
}

bool WarlockAI::HandleCrowdControl(Unit* target)
{    if (!target)
        return false;

    Player* bot = GetBot();
    if (!bot)
        return false;

    uint32 now = GameTime::GetGameTimeMS();

    // Fear - primary CC
    if (bot->HasSpell(WARLOCK_FEAR) && (now - _lastFear > 5000))
    {
        float distanceSq = bot->GetExactDistSq(target);
        if (!target->HasAura(WARLOCK_FEAR) && distanceSq <= (20.0f * 20.0f)) // 400.0f

        {
        bot->CastSpell(CastSpellTargetArg(target), WARLOCK_FEAR);

            _lastFear = now;
            _fearsUsed++;

            return true;
        }
    }

    // Banish - for demons/elementals
    if (target->GetCreatureType() == CREATURE_TYPE_DEMON ||
        target->GetCreatureType() == CREATURE_TYPE_ELEMENTAL)
    {
        if (bot->HasSpell(WARLOCK_BANISH) && !target->HasAura(WARLOCK_BANISH))
        {

            bot->CastSpell(CastSpellTargetArg(target), WARLOCK_BANISH);

            return true;
        }
    }

    // Curse of Exhaustion - slow for kiting
    if (bot->HasSpell(WARLOCK_CURSE_OF_EXHAUSTION) && !target->HasAura(WARLOCK_CURSE_OF_EXHAUSTION))
    {
        bot->CastSpell(CastSpellTargetArg(target), WARLOCK_CURSE_OF_EXHAUSTION);
        return true;
    }    return false;
}

bool WarlockAI::HandleAoERotation(Unit* target)
{
    if (!target)        return false;

    Player* bot = GetBot();
    if (!bot)
        return false;

    uint32 nearbyEnemies = GetNearbyEnemyCount(15.0f);
    if (nearbyEnemies < 3)
        return false;

    uint32 spec = static_cast<uint32>(bot->GetPrimarySpecialization());        // Seed of Corruption for Affliction (265) or when many enemies
    if (static_cast<uint32>(spec) == 265 || nearbyEnemies >= 4)    {
        if (bot->HasSpell(WARLOCK_SEED_OF_CORRUPTION) && !target->HasAura(WARLOCK_SEED_OF_CORRUPTION))
        {

            bot->CastSpell(CastSpellTargetArg(target), WARLOCK_SEED_OF_CORRUPTION);

            return true;
        }
    }

// Rain of Fire
    if (bot->HasSpell(WARLOCK_RAIN_OF_FIRE) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_RAIN_OF_FIRE))
    {
        // Note: Ground-targeted spell, needs special handling
        bot->CastSpell(CastSpellTargetArg(target), WARLOCK_RAIN_OF_FIRE);
        return true;
    }

    // Cataclysm (if available)
    if (bot->HasSpell(WARLOCK_CATACLYSM) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_CATACLYSM))
    {
        bot->CastSpell(CastSpellTargetArg(target), WARLOCK_CATACLYSM);
        return true;
    }

// Fire and Brimstone for Destruction (267)
    if (static_cast<uint32>(spec) == 267)
    {        if (bot->HasSpell(WARLOCK_FIRE_AND_BRIMSTONE) && !bot->HasAura(WARLOCK_FIRE_AND_BRIMSTONE))
        {

            bot->CastSpell(CastSpellTargetArg(bot), WARLOCK_FIRE_AND_BRIMSTONE);

            return true;
        }
    }

    // Note: Mannoroth's Fury removed in WoW 12.0

    return false;
}

bool WarlockAI::HandleOffensiveCooldowns(Unit* target)
{
    if (!target)        return false;

    Player* bot = GetBot();
    if (!bot)
        return false;

    // Check if target is worth using cooldowns on
    if (target->GetHealthPct() < 20.0f)
        return false;

    bool usedCooldown = false;
    uint32 spec = static_cast<uint32>(bot->GetPrimarySpecialization());        

    // Dark Soul variants based on spec
    // Spec IDs: 265 = Affliction, 266 = Demonology, 267 = Destruction
    if (static_cast<uint32>(spec) == 265) // Affliction
    {
        if (bot->HasSpell(WARLOCK_DARK_SOUL_MISERY) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_DARK_SOUL_MISERY))
        {

            bot->CastSpell(CastSpellTargetArg(bot), WARLOCK_DARK_SOUL_MISERY);

            usedCooldown = true;
        }
    }
    else if (static_cast<uint32>(spec) == 266) // Demonology
    {
        // Summon Demonic Tyrant - main Demonology cooldown in 12.0
        // Note: Metamorphosis and Dark Soul: Knowledge removed (Metamorphosis is Demon Hunter only)
        constexpr uint32 SUMMON_DEMONIC_TYRANT = WarlockDemonology::SUMMON_DEMONIC_TYRANT;
        if (bot->HasSpell(SUMMON_DEMONIC_TYRANT) && !bot->GetSpellHistory()->HasCooldown(SUMMON_DEMONIC_TYRANT))
        {
            bot->CastSpell(CastSpellTargetArg(bot), SUMMON_DEMONIC_TYRANT);
            usedCooldown = true;
        }
    }    else if (static_cast<uint32>(spec) == 267) // Destruction
  
    {        if (bot->HasSpell(WARLOCK_DARK_SOUL_INSTABILITY) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_DARK_SOUL_INSTABILITY))
    {

            bot->CastSpell(CastSpellTargetArg(bot), WARLOCK_DARK_SOUL_INSTABILITY);

            usedCooldown = true;
        }
    }

    // Summon Infernal/Doomguard
    if (GetNearbyEnemyCount(30.0f) >= 3 || target->GetHealthPct() > 80.0f)    {
        // Summon Infernal - main offensive cooldown for Destruction (Doomguard removed in 12.0)
        if (bot->HasSpell(WARLOCK_SUMMON_INFERNAL))
        {
            if (!bot->GetSpellHistory()->HasCooldown(WARLOCK_SUMMON_INFERNAL))
            {
                bot->CastSpell(CastSpellTargetArg(target), WARLOCK_SUMMON_INFERNAL);
                usedCooldown = true;
            }
        }
    }

    return usedCooldown;
}

void WarlockAI::HandleSoulShardManagement()
{
    Player* bot = GetBot();
    if (!bot)
        return;

    // Update soul shard count
    _currentSoulShards = bot->GetItemCount(6265);    // Track soul shard history for optimization
    _soulShardHistory.push(_currentSoulShards.load());
    if (_soulShardHistory.size() > 10)
        _soulShardHistory.pop();

    // Determine if we should conserve shards
    bool shouldConserve = (_currentSoulShards.load() < 3);

    if (shouldConserve)
    {
        TC_LOG_DEBUG("playerbot.warlock", "Soul shard conservation mode active - {} shards remaining",

                     _currentSoulShards.load());
    }

    // Create healthstone if we don't have one
    if (!HasHealthstone() && _currentSoulShards.load() > 5)
    {
        if (bot->HasSpell(WARLOCK_CREATE_HEALTHSTONE) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_CREATE_HEALTHSTONE))
        {

            bot->CastSpell(CastSpellTargetArg(bot), WARLOCK_CREATE_HEALTHSTONE);
        }
    }

    // Create soulstone if needed
    if (!HasSoulstone() && _currentSoulShards.load() > 3)
    {        if (bot->HasSpell(WARLOCK_CREATE_SOULSTONE) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_CREATE_SOULSTONE))
        {

            bot->CastSpell(CastSpellTargetArg(bot), WARLOCK_CREATE_SOULSTONE);
        }
    }
}

// ============================================================================
// Priority 10: Basic Rotation - Pet + Shadow Bolt / Corruption / Drain Life
// ============================================================================
void WarlockAI::ExecuteBasicRotation(Unit* target)
{
    if (!target || !target->IsAlive())
        return;

    Player* bot = GetBot();
    if (!bot || !bot->IsAlive())
        return;

    // Don't cast if already casting
    if (bot->HasUnitState(UNIT_STATE_CASTING))
        return;

    TC_LOG_DEBUG("module.playerbot.warlock", "ExecuteBasicRotation: Bot {} level {} executing rotation on {}",
                 bot->GetName(), bot->GetLevel(), target->GetName());

    // NOTE: Pet summoning is handled in WarlockBaselineRotation::ApplyBuffs()
    // which runs OUT OF COMBAT. Pet summons have 6s cast time - cannot cast in combat!

    // ========================================================================
    // PRIORITY 1: Self-heal with Drain Life if health is low (level 9+)
    // ========================================================================
    float healthPct = bot->GetHealthPct();
    if (healthPct < 50.0f && bot->HasSpell(WARLOCK_DRAIN_LIFE))
    {
        TC_LOG_DEBUG("module.playerbot.warlock", "ExecuteBasicRotation: {} health low ({:.0f}%), using Drain Life",
                     bot->GetName(), healthPct);
        SpellCastResult result = bot->CastSpell(CastSpellTargetArg(target), WARLOCK_DRAIN_LIFE);
        if (result == SPELL_CAST_OK)
        {
            _warlockMetrics.spellsCast++;
            return;
        }
    }

    // ========================================================================
    // PRIORITY 2: Apply Corruption if target doesn't have it (instant cast DoT)
    // ========================================================================
    if (bot->HasSpell(WARLOCK_CORRUPTION))
    {
        bool hasCorruption = target->HasAura(CORRUPTION, bot->GetGUID());
        if (!hasCorruption)
        {
            TC_LOG_DEBUG("module.playerbot.warlock", "ExecuteBasicRotation: {} casting Corruption on {}",
                         bot->GetName(), target->GetName());
            SpellCastResult result = bot->CastSpell(CastSpellTargetArg(target), WARLOCK_CORRUPTION);
            if (result == SPELL_CAST_OK)
            {
                _warlockMetrics.spellsCast++;
                return;
            }
        }
    }

    // ========================================================================
    // PRIORITY 3: Shadow Bolt (main cast-time damage spell)
    // SHADOWBOLT = 686 - This is the base Warlock damage spell
    // ========================================================================
    bool hasShadowBolt = bot->HasSpell(WARLOCK_SHADOWBOLT);
    TC_LOG_INFO("playerbot.warlock", "ExecuteBasicRotation: {} HasSpell(SHADOWBOLT={})={}",
                bot->GetName(), static_cast<uint32>(WARLOCK_SHADOWBOLT), hasShadowBolt ? "YES" : "NO");

    if (hasShadowBolt)
    {
        TC_LOG_INFO("playerbot.warlock", "ExecuteBasicRotation: {} ATTEMPTING Shadow Bolt (ID {}) on {}",
                    bot->GetName(), static_cast<uint32>(WARLOCK_SHADOWBOLT), target->GetName());
        SpellCastResult result = bot->CastSpell(CastSpellTargetArg(target), WARLOCK_SHADOWBOLT);
        TC_LOG_INFO("playerbot.warlock", "ExecuteBasicRotation: {} Shadow Bolt result = {} (OK={})",
                    bot->GetName(), static_cast<int>(result), static_cast<int>(SPELL_CAST_OK));
        if (result == SPELL_CAST_OK)
        {
            _warlockMetrics.spellsCast++;
            return;
        }
        else
        {
            TC_LOG_WARN("playerbot.warlock", "ExecuteBasicRotation: {} Shadow Bolt FAILED with result {}!",
                        bot->GetName(), static_cast<int>(result));
        }
    }
    else
    {
        TC_LOG_WARN("playerbot.warlock", "ExecuteBasicRotation: {} does NOT have Shadow Bolt spell ID {}!",
                    bot->GetName(), static_cast<uint32>(WARLOCK_SHADOWBOLT));
    }

    // ========================================================================
    // FALLBACK: Auto-attack if no spells available
    // ========================================================================
    if (bot->GetVictim() != target)
    {
        bot->Attack(target, true);
        TC_LOG_DEBUG("module.playerbot.warlock", "ExecuteBasicRotation: {} falling back to auto-attack on {}",
                     bot->GetName(), target->GetName());
    }
}

bool WarlockAI::HandleInstantCasts(Unit* target)
{
    if (!target)
        return false;

    Player* bot = GetBot();
    if (!bot)
        return false;

    // Corruption - instant with talent
    if (bot->HasSpell(WARLOCK_CORRUPTION) && !target->HasAura(WARLOCK_CORRUPTION))
    {
        bot->CastSpell(CastSpellTargetArg(target), WARLOCK_CORRUPTION);
        return true;
    }

    // Curse application
    if (ApplyCurse(target))
        return true;

    // Conflagrate for Destruction (267)
    if (static_cast<uint32>(bot->GetPrimarySpecialization()) == 267)    {
        if (bot->HasSpell(WARLOCK_CONFLAGRATE) && target->HasAura(WARLOCK_IMMOLATE))
        {

            if (!bot->GetSpellHistory()->HasCooldown(WARLOCK_CONFLAGRATE))

            {

                bot->CastSpell(CastSpellTargetArg(target), WARLOCK_CONFLAGRATE);

                return true;

            }
        }
    }

    // Shadowburn for low health targets
    if (bot->HasSpell(WARLOCK_SHADOWBURN) && target->GetHealthPct() < 20.0f)
    {
        if (!bot->GetSpellHistory()->HasCooldown(WARLOCK_SHADOWBURN))
        {

            bot->CastSpell(CastSpellTargetArg(target), WARLOCK_SHADOWBURN);

            return true;
        }
    }

    return false;
}

bool WarlockAI::ApplyDoTToTarget(Unit* target)
{
    if (!target)
        return false;

    Player* bot = GetBot();
    if (!bot)
        return false;

    // Track DoT application time
    ObjectGuid targetGuid = target->GetGUID();
    uint32 now = GameTime::GetGameTimeMS();

    // Corruption - primary DoT
    if (bot->HasSpell(WARLOCK_CORRUPTION) && !target->HasAura(WARLOCK_CORRUPTION))
    {
        bot->CastSpell(CastSpellTargetArg(target), WARLOCK_CORRUPTION);
        _dotTracker[targetGuid][CORRUPTION] = now;
        return true;
    }

    // Spec-specific DoTs
    uint32 spec = static_cast<uint32>(bot->GetPrimarySpecialization());
    if (static_cast<uint32>(spec) == 265) // Affliction
    {
        // Unstable Affliction
    if (bot->HasSpell(WARLOCK_UNSTABLE_AFFLICTION) && !target->HasAura(WARLOCK_UNSTABLE_AFFLICTION))
        {

            bot->CastSpell(CastSpellTargetArg(target), WARLOCK_UNSTABLE_AFFLICTION);

            _dotTracker[targetGuid][UNSTABLE_AFFLICTION] = now;
            return true;
        }
        // Haunt
    if (bot->HasSpell(WARLOCK_HAUNT) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_HAUNT))
        {

            bot->CastSpell(CastSpellTargetArg(target), WARLOCK_HAUNT);

            _dotTracker[targetGuid][HAUNT] = now;

            return true;
        }
    }
    else if (static_cast<uint32>(spec) == 267) // Destruction  
    {
        // Immolate
    if (bot->HasSpell(WARLOCK_IMMOLATE) && !target->HasAura(WARLOCK_IMMOLATE))

        {
        bot->CastSpell(CastSpellTargetArg(target), WARLOCK_IMMOLATE);

            _dotTracker[targetGuid][IMMOLATE] = now;

            return true;
        }
    }
    else if (static_cast<uint32>(spec) == 266) // Demonology  
    {
        // Corruption is usually enough, Hand of Gul'dan for AoE
    if (bot->HasSpell(WARLOCK_HAND_OF_GULDAN) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_HAND_OF_GULDAN))
        {

            bot->CastSpell(CastSpellTargetArg(target), WARLOCK_HAND_OF_GULDAN);

            return true;
        }
    }

    return false;
}

bool WarlockAI::ApplyCurse(Unit* target)
{
    if (!target)
        return false;

    Player* bot = GetBot();
    if (!bot)
        return false;

    // Check if target already has a curse (Curse of Elements removed in 12.0)
    if (target->HasAura(WARLOCK_CURSE_OF_AGONY) ||
        target->HasAura(WARLOCK_CURSE_OF_TONGUES) || target->HasAura(WARLOCK_CURSE_OF_WEAKNESS) ||
        target->HasAura(WARLOCK_CURSE_OF_EXHAUSTION))
        return false;

    uint32 curseSpell = 0;

    // Choose appropriate curse based on target and situation
    // Note: Curse of Elements removed in WoW 12.0
    if (target->GetPowerType() == POWER_MANA)
    {
        // Curse of Tongues for casters
        curseSpell = WARLOCK_CURSE_OF_TONGUES;
    }
    else if (static_cast<uint32>(bot->GetPrimarySpecialization()) == 265) // Affliction
    {
        // Curse of Agony (Agony) for Affliction
        curseSpell = WARLOCK_CURSE_OF_AGONY;
    }
    else
    {
        // Curse of Weakness for damage/attack power reduction
        curseSpell = WARLOCK_CURSE_OF_WEAKNESS;
    }

    // Apply curse if available
    if (curseSpell && bot->HasSpell(curseSpell))
    {
        bot->CastSpell(CastSpellTargetArg(target), curseSpell);
        return true;
    }

    return false;
}

Unit* WarlockAI::GetNearestEnemy(float range)
{
    Player* bot = GetBot();
    if (!bot)
        return nullptr;

    ::std::list<Unit*> enemies;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(bot, bot, range);
    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, enemies, check);
    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = bot->GetMap();
    if (!map)
        return nullptr;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)

            return nullptr;
    }

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        bot->GetPosition(), range);

    // Process results (replace old loop)
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(bot, guid);
        Creature* entity = nullptr;
        if (snapshot_entity)
        {

        } 
        if (!entity)

            continue;
        // Original filtering logic goes here
    }
    // End of spatial grid fix

    Unit* nearest = nullptr;
    float minDistSq = range * range; // Squared distance
    for (Unit* enemy : enemies)
    {
        if (!enemy->IsAlive() || !bot->CanSeeOrDetect(enemy))

            continue;

        float distSq = bot->GetExactDistSq(enemy);
        if (distSq < minDistSq)
        {

            minDistSq = distSq;

            nearest = enemy;
        }
    }

    return nearest;
}

uint32 WarlockAI::GetNearbyEnemyCount(float range)
{
    Player* bot = GetBot();
    if (!bot)
        return 0;

    ::std::list<Unit*> enemies;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(bot, bot, range);
    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, enemies, check);
    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = bot->GetMap();
    if (!map)
        return 0;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)

            return 0;
    }

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        bot->GetPosition(), range);

    // Process results (replace old loop)
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(bot, guid);
        Creature* entity = nullptr;
        if (snapshot_entity)
        {

        } 
        if (!entity)

            continue;
        // Original filtering logic goes here
    }
    // End of spatial grid fix

    uint32 count = 0;
    for (Unit* enemy : enemies)
    {
        if (enemy->IsAlive() && bot->CanSeeOrDetect(enemy))

            count++;
    }

    return count;
}

bool WarlockAI::HasHealthstone()
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    // Check for various healthstone item IDs
    uint32 healthstones[] = { 5512, 5511, 5509, 5510, 9421, 19013 };
    for (uint32 itemId : healthstones)
    {
        if (bot->GetItemCount(itemId) > 0)

            return true;
    }

    return false;
}

bool WarlockAI::UseHealthstone()
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    // Try to use healthstone
    uint32 healthstones[] = { 5512, 5511, 5509, 5510, 9421, 19013 };
    for (uint32 itemId : healthstones)
    {
        if (Item* item = bot->GetItemByEntry(itemId))
        {
            // Use modern TrinityCore CastItemUseSpell API
            SpellCastTargets targets;
            targets.SetUnitTarget(bot); // Use healthstone on self

            // WoW 12.0: CastItemUseSpell signature changed to std::array<int32, 3>
            std::array<int32, 3> misc = { 0, 0, 0 };
            bot->CastItemUseSpell(item, targets, ObjectGuid::Empty, misc);

            TC_LOG_DEBUG("playerbot.warlock", "Warlock {} used healthstone {}", bot->GetName(), itemId);

            return true;
        }
    }

    return false;
}

bool WarlockAI::HasSoulstone()
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    // Check for soulstone buff or item
    uint32 soulstones[] = { 5232, 16892, 16893, 16895, 16896 };
    for (uint32 itemId : soulstones)
    {
        if (bot->GetItemCount(itemId) > 0)

            return true;
    }

    // Check if we have soulstone buff on us
    return bot->HasAura(20707); // Soulstone Resurrection
}

void WarlockAI::UpdateCombatMetrics()
{
    // Calculate efficiency metrics
    if (_warlockMetrics.manaSpent > 0)
    {
        _warlockMetrics.manaEfficiency = static_cast<float>(_warlockMetrics.damageDealt) / _warlockMetrics.manaSpent;
    }

// Update pet uptime
    if (_petActive.load())
    {        auto now = ::std::chrono::steady_clock::now();
        auto combatDuration = ::std::chrono::duration_cast<::std::chrono::seconds>(now - _warlockMetrics.combatStartTime).count();
        if (combatDuration > 0)
        {

            _warlockMetrics.petUptime = 100.0f; // Pet is currently active
        }
    }

    // Update DoT uptime
    Player* bot = GetBot();
    if (bot && bot->GetVictim())
    {
        Unit* target = bot->GetVictim();
        int dotCount = 0;
        int totalDots = 3; // Typical number of DoTs
    if (target->HasAura(WARLOCK_CORRUPTION)) dotCount++;
        if (target->HasAura(WARLOCK_CURSE_OF_AGONY)) dotCount++;
        if (target->HasAura(WARLOCK_UNSTABLE_AFFLICTION)) dotCount++;
        if (target->HasAura(WARLOCK_IMMOLATE)) dotCount++;

        _warlockMetrics.dotUptime = (static_cast<float>(dotCount) / totalDots) * 100.0f;
    }
}
// Required virtual function implementations
void WarlockAI::UpdateBuffs()
{
    // NOTE: Baseline buff check is now handled at the dispatch level.
    // This method is only called for level 10+ bots with talents.

    // Update warlock-specific buffs
    UpdateWarlockBuffs();    // Note: Spec-specific buff logic is now handled through the refactored specialization classes
}

void WarlockAI::UpdateCooldowns(uint32 diff)
{
    // Manage warlock-specific cooldowns
    ManageWarlockCooldowns();

    // Note: Spec-specific cooldown logic is now handled through the refactored specialization classes
}

bool WarlockAI::CanUseAbility(uint32 spellId)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    // Check if spell is known and ready
    if (!bot->HasSpell(spellId))
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, bot->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return false;

    // Check if we have enough resources
    if (!HasEnoughResource(spellId))
        return false;

    // Check cooldown
    if (bot->GetSpellHistory()->HasCooldown(spellId))
        return false;

    // Check if we're casting or channeling
    if (bot->IsNonMeleeSpellCast(false, true, true))
        return false;

    // Check soul shard requirements
    for (uint32 i = 0; i < MAX_SPELL_REAGENTS; ++i)
    {
        if (spellInfo->Reagent[i] == 6265) // Soul Shard item ID
        {

            if (bot->GetItemCount(6265) < spellInfo->ReagentCount[i])

                return false;
        }
    }

    return true;
}

void WarlockAI::OnCombatStart(::Unit* target)
{
    ClassAI::OnCombatStart(target);

    // Reset combat metrics
    _warlockMetrics.Reset();

    // Update pet status
    UpdatePetCheck();

    // Clear DoT tracker for new combat
    _dotTracker.clear();

    // Initialize threat management
    if (_threatManager)
    {
        // BotThreatManager handles combat start automatically
    }

    TC_LOG_DEBUG("playerbot.warlock", "Warlock {} entering combat - Spec: {}, Soul Shards: {}",

                 GetBot()->GetName(), GetBot()->GetPrimarySpecialization(), _currentSoulShards.load());
}

void WarlockAI::OnCombatEnd()
{
    ClassAI::OnCombatEnd();

    // Log combat metrics
    auto combatDuration = ::std::chrono::duration_cast<::std::chrono::seconds>(
        ::std::chrono::steady_clock::now() - _warlockMetrics.combatStartTime).count();

    if (combatDuration > 0)
    {
        TC_LOG_DEBUG("playerbot.warlock", "Combat summary for {}: Duration: {}s, Damage: {}, DoT: {}, Pet: {}, Efficiency: {:.2f}",

                     GetBot()->GetName(), combatDuration,

                     _warlockMetrics.damageDealt.load(),

                     _warlockMetrics.dotDamage.load(),

                     _warlockMetrics.petDamage.load(),

                     _warlockMetrics.manaEfficiency.load());
    }

    // Life Tap if needed
    ManageLifeTapTiming();
}

void WarlockAI::OnNonCombatUpdate(uint32 diff)
{
    // CRITICAL FIX: Pet summoning MUST happen out of combat
    // This method is called by BotAI::UpdateAI() when NOT in combat
    // Handles pet summoning, buff maintenance, and out-of-combat preparation

    Player* bot = GetBot();
    if (!bot || !bot->IsAlive())
        return;

    // Priority 1: Pet Management - Summon pet if missing
    // Check if we need to summon a pet (no pet or pet is dead)
    Pet* pet = bot->GetPet();
    if (!pet || !pet->IsAlive())
    {
        // Try to summon pet
        if (SummonPet())
        {
            TC_LOG_DEBUG("playerbot.warlock", "Warlock {} summoned pet out of combat", bot->GetName());
            return; // Pet summoning takes time, don't do anything else this frame
        }
    }
    else
    {
        // Pet is alive - update pet status
        _petActive = true;
        _petHealthPercent = static_cast<uint32>(pet->GetHealthPct());

        // CRITICAL FIX: Ensure bot pets are always in DEFENSIVE mode
        // This check is also in HandlePetManagement() but we need it here for out-of-combat
        if (pet->HasReactState(REACT_PASSIVE))
        {
            TC_LOG_DEBUG("playerbot.warlock", "Warlock {} pet {} was PASSIVE (out of combat), setting to DEFENSIVE",
                         bot->GetName(), pet->GetName());
            pet->SetReactState(REACT_DEFENSIVE);
        }

        // Heal pet if needed (out of combat)
        if (_petHealthPercent.load() < 70)
        {
            // Health Funnel (only if we have good health ourselves)
            if (bot->GetHealthPct() > 80.0f &&
                bot->HasSpell(WARLOCK_HEALTH_FUNNEL) &&
                !bot->GetSpellHistory()->HasCooldown(WARLOCK_HEALTH_FUNNEL))
            {
                bot->CastSpell(CastSpellTargetArg(pet), WARLOCK_HEALTH_FUNNEL);
                TC_LOG_DEBUG("playerbot.warlock", "Warlock {} healing pet with Health Funnel (out of combat)", bot->GetName());
                return;
            }
            // Note: Consume Shadows removed in WoW 12.0
        }
    }

    // Priority 2: Apply buffs
    // Throttle buff checks to every 5 seconds (expensive operations)
    static uint32 lastBuffCheck = 0;
    uint32 now = GameTime::GetGameTimeMS();
    if (now - lastBuffCheck > 5000)
    {
        UpdateBuffs();
        lastBuffCheck = now;
    }

    // Priority 3: Soul Shard Management (create healthstone/soulstone)
    // Only check every 10 seconds
    static uint32 lastShardCheck = 0;
    if (now - lastShardCheck > 10000)
    {
        HandleSoulShardManagement();
        lastShardCheck = now;
    }

    // Priority 4: Mana Management (Life Tap if needed and safe)
    if (bot->GetHealthPct() > 70.0f && bot->GetPowerPct(POWER_MANA) < 50.0f)
    {
        ManageLifeTapTiming();
    }
}

bool WarlockAI::HasEnoughResource(uint32 spellId)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, bot->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return false;

    // Check mana
    auto powerCosts = spellInfo->CalcPowerCost(bot, spellInfo->GetSchoolMask());
    uint32 manaCost = 0;
    for (auto const& cost : powerCosts)
        if (cost.Power == POWER_MANA)

            manaCost = cost.Amount;
    if (bot->GetPower(POWER_MANA) < int32(manaCost))
    {
        _lowManaMode = true;
        return false;
    }

    // Check if we're in low mana mode
    if (bot->GetPowerPct(POWER_MANA) < LOW_MANA_THRESHOLD * 100)
    {
        _lowManaMode = true;
    }
    else if (bot->GetPowerPct(POWER_MANA) > 50)
  
    {
        _lowManaMode = false;
    }

    return true;
}

void WarlockAI::ConsumeResource(uint32 spellId)
{
    Player* bot = GetBot();
    if (!bot)
        return;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, bot->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return;

    // Track mana spent
    auto powerCosts = spellInfo->CalcPowerCost(bot, spellInfo->GetSchoolMask());
    uint32 manaCost = 0;
    for (auto const& cost : powerCosts)
        if (cost.Power == POWER_MANA)

            manaCost = cost.Amount;
    _warlockMetrics.manaSpent += manaCost;

    // Track soul shard usage
    for (uint32 i = 0; i < MAX_SPELL_REAGENTS; ++i)
    {
        if (spellInfo->Reagent[i] == 6265) // Soul Shard
        {

            _warlockMetrics.soulShardsUsed++;

            _currentSoulShards = bot->GetItemCount(6265);

            break;
        }
    }
}

Position WarlockAI::GetOptimalPosition(::Unit* target)
{
    if (!target || !_positionManager)
        return GetBot()->GetPosition();

    // Use position manager for optimal positioning
    return _positionManager->FindRangedPosition(target, GetOptimalRange(target));
}

float WarlockAI::GetOptimalRange(::Unit* target)
{
    if (!target)
        return 25.0f;

    // Warlocks prefer to stay at max casting range
    // Shadow Bolt/Corruption have 30yd max range, stay at 25yd for safety
    return 25.0f; // Safe distance within spell range, allows for movement
}

// Warlock-specific utility implementations
void WarlockAI::UpdateWarlockBuffs()
{
    Player* bot = GetBot();
    if (!bot)
        return;

    // Don't apply buffs while casting (especially important for 6s pet summons)
    if (bot->HasUnitState(UNIT_STATE_CASTING))
        return;

    // Note: Demon Armor, Fel Armor, Soul Link, and Dark Intent removed in WoW 12.0
    // Warlocks no longer have passive armor buffs - they rely on defensive cooldowns instead
    // Soul Link is now a passive talent, not an active ability
}

void WarlockAI::UpdatePetCheck()
{
    auto now = ::std::chrono::steady_clock::now();
    if (::std::chrono::duration_cast<::std::chrono::milliseconds>(now - _lastPetCheck).count() < PET_CHECK_INTERVAL)
        return;

    _lastPetCheck = now;

    Player* bot = GetBot();
    if (!bot)
        return;

    Pet* pet = bot->GetPet();
    _petActive = (pet && pet->IsAlive());

    if (_petActive.load() && pet)
    {
        _petHealthPercent = static_cast<uint32>(pet->GetHealthPct());
    }
    else
    {
        _petHealthPercent = 0;
    }
}

void WarlockAI::UpdateSoulShardCheck()
{
    Player* bot = GetBot();
    if (!bot)
        return;

    _currentSoulShards = bot->GetItemCount(6265);

    // Track soul shard history for optimization
    _soulShardHistory.push(_currentSoulShards.load());
    if (_soulShardHistory.size() > 10)
        _soulShardHistory.pop();
}

void WarlockAI::OptimizeManaManagement()
{
    Player* bot = GetBot();
    if (!bot)
        return;

    float manaPct = bot->GetPowerPct(POWER_MANA);    // Adjust mana threshold based on combat situation
    if (bot->IsInCombat())
    {
        _optimalManaThreshold = 0.3f; // Lower threshold in combat
    }
    else
    {
        _optimalManaThreshold = 0.5f; // Higher threshold out of combat
    }

    // Update low mana mode
    _lowManaMode = (manaPct < _optimalManaThreshold * 100);
}

void WarlockAI::ManageLifeTapTiming()
{
    Player* bot = GetBot();
    if (!bot)
        return;

    uint32 now = GameTime::GetGameTimeMS();

    // Don't Life Tap too frequently
    if (now - _lastLifeTapTime < 3000)
        return;

    float healthPct = bot->GetHealthPct();
    float manaPct = bot->GetPowerPct(POWER_MANA);    // Only Life Tap if health is good and mana is low
    if (healthPct > LIFE_TAP_THRESHOLD * 100 && manaPct < _optimalManaThreshold * 100)
    {
        if (bot->HasSpell(WARLOCK_LIFE_TAP) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_LIFE_TAP))
        {

            bot->CastSpell(CastSpellTargetArg(bot), WARLOCK_LIFE_TAP);

            _lastLifeTapTime = now;

            _warlockMetrics.lifeTapsCast++;

            TC_LOG_DEBUG("playerbot.warlock", "Life Tap cast - Health: {:.1f}%, Mana: {:.1f}%", healthPct, manaPct);
        }
    }
}

void WarlockAI::OptimizePetPositioning()
{
    if (!_petActive.load() || !_positionManager)
        return;

    Player* bot = GetBot();
    Pet* pet = bot ? bot->GetPet() : nullptr;
    if (!pet)
        return;

    Unit* target = bot->GetVictim();
    if (!target)
        return;

    // Position pet based on its type and combat role
    Position optimalPos;
    float distance = 0.0f;

    // Determine optimal position based on pet type
    uint32 spec = static_cast<uint32>(bot->GetPrimarySpecialization());
    if (static_cast<uint32>(spec) == 266 && bot->HasSpell(WARLOCK_SUMMON_FELGUARD)) // Demonology
    {
        // Melee pet - position in front of target
        distance = 3.0f;
        optimalPos = target->GetNearPosition(distance, 0);
    }
    else if (static_cast<uint32>(spec) == 265) // Affliction - Felhunter
  
    {
        // Anti-caster pet - position near casters
        distance = 5.0f;
        optimalPos = target->GetNearPosition(distance, static_cast<float>(M_PI) / 4.0f);
    }
    else // Imp or Voidwalker
    {
        // Ranged or tank pet - position at medium range
        distance = 15.0f;
        optimalPos = target->GetNearPosition(distance, static_cast<float>(M_PI) / 2.0f);
    }

    // Command pet to move if needed
    float distanceSq = pet->GetExactDistSq(optimalPos);
    if (distanceSq > (5.0f * 5.0f)) // 25.0f
    {
        pet->GetMotionMaster()->MovePoint(0, optimalPos);
    }
}

void WarlockAI::HandlePetSpecialAbilities()
{
    if (!_petActive.load())
        return;

    Player* bot = GetBot();
    Pet* pet = bot ? bot->GetPet() : nullptr;
    if (!pet)
        return;

    Unit* target = bot->GetVictim();
    if (!target)
        return;

    // Use pet abilities based on type and situation
    uint32 petEntry = pet->GetEntry();
    uint32 now = GameTime::GetGameTimeMS();

    // Imp abilities
    if (petEntry == 416) // Imp
    {
        // Firebolt is auto-cast, no need to manage
    }
    // Voidwalker abilities
    else if (petEntry == 1860) // Voidwalker
  
    {
        // Torment for threat
    if (!_petAbilityCooldowns[17735] || now - _petAbilityCooldowns[17735] > 5000)
        {

            pet->CastSpell(target, 17735, false); // Torment

            _petAbilityCooldowns[17735] = now;
        }
    }
    // Succubus abilities
    else if (petEntry == 1863) // Succubus
  
    {
        // Lash of Pain
    if (!_petAbilityCooldowns[7814] || now - _petAbilityCooldowns[7814] > 6000)
        {

            pet->CastSpell(target, 7814, false); // Lash of Pain

            _petAbilityCooldowns[7814] = now;
        }
    }
    // Felhunter abilities
    else if (petEntry == 417) // Felhunter
  
    {
        // Devour Magic for dispel
    if (target->HasUnitState(UNIT_STATE_CASTING))
        {

            if (!_petAbilityCooldowns[19505] || now - _petAbilityCooldowns[19505] > 8000)

            {

                pet->CastSpell(target, 19505, false); // Devour Magic

                _petAbilityCooldowns[19505] = now;

            }
        }
    }
    // Felguard abilities
    else if (petEntry == 17252) // Felguard
  
    {
        // Cleave
    if (!_petAbilityCooldowns[30213] || now - _petAbilityCooldowns[30213] > 6000)
        {

            pet->CastSpell(target, 30213, false); // Cleave

            _petAbilityCooldowns[30213] = now;
        }
    }
}

void WarlockAI::ManageWarlockCooldowns()
{
    // Manage major warlock cooldowns
    Player* bot = GetBot();
    if (!bot || !bot->IsInCombat())
        return;

    uint32 spec = static_cast<uint32>(bot->GetPrimarySpecialization());

    // Summon Demonic Tyrant for Demonology (266) - main cooldown in WoW 12.0
    // Note: Demonic Empowerment and Metamorphosis removed in 12.0
    if (spec == 266)
    {
        constexpr uint32 SUMMON_DEMONIC_TYRANT = WarlockDemonology::SUMMON_DEMONIC_TYRANT;
        if (bot->HasSpell(SUMMON_DEMONIC_TYRANT) && !bot->GetSpellHistory()->HasCooldown(SUMMON_DEMONIC_TYRANT))
        {
            // Use in high-pressure situations with many active demons
            if (bot->GetVictim() && bot->GetVictim()->GetHealthPct() > 50)
            {
                bot->CastSpell(CastSpellTargetArg(bot), SUMMON_DEMONIC_TYRANT);
            }
        }
    }
}

void WarlockAI::OptimizeSoulShardUsage()
{
    // Optimize soul shard usage based on availability and need
    ::std::lock_guard lock(_soulShardMutex);

    // Determine conservation mode based on shard count
    bool shouldConserve = (_currentSoulShards.load() < 5);

    if (shouldConserve)
    {
        TC_LOG_DEBUG("playerbot.warlock", "Soul shard conservation mode active - {} shards remaining",

                     _currentSoulShards.load());
    }
}
void WarlockAI::HandleAoESituations()
{
    Player* bot = GetBot();
    if (!bot || !bot->IsInCombat())
        return;

    uint32 nearbyEnemies = GetNearbyEnemyCount(30.0f);

    if (nearbyEnemies >= 3)
    {
        // Seed of Corruption for Affliction (265)
    if (static_cast<uint32>(bot->GetPrimarySpecialization()) == 265 && bot->HasSpell(WARLOCK_SEED_OF_CORRUPTION))
        {

            Unit* target = bot->GetVictim();
            if (target && !target->HasAura(WARLOCK_SEED_OF_CORRUPTION) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_SEED_OF_CORRUPTION))

            {

                bot->CastSpell(CastSpellTargetArg(target), WARLOCK_SEED_OF_CORRUPTION);

            }
        }

        // Rain of Fire for all specs
    if (bot->HasSpell(WARLOCK_RAIN_OF_FIRE) && !bot->GetSpellHistory()->HasCooldown(WARLOCK_RAIN_OF_FIRE))
        {

            Unit* target = bot->GetVictim();
            if (target)

            {

                bot->CastSpell(CastSpellTargetArg(target), WARLOCK_RAIN_OF_FIRE);

            }
        }
    }
}

void WarlockAI::ManageCurseApplication()
{
    Player* bot = GetBot();
    if (!bot || !bot->IsInCombat())
        return;

    Unit* target = bot->GetVictim();
    if (!target)
        return;

    ApplyCurse(target);
}

void WarlockAI::OptimizeDoTRotation()
{
    Player* bot = GetBot();
    if (!bot || !bot->IsInCombat())
        return;

    // Affliction (265) focuses heavily on DoTs
    if (static_cast<uint32>(bot->GetPrimarySpecialization()) != 265)
        return;

    Unit* target = bot->GetVictim();
    if (!target)
        return;

    ApplyDoTToTarget(target);
}

// Helper method implementations
bool WarlockAI::HasEnoughMana(uint32 amount)
{
    Player* bot = GetBot();
    return bot && bot->GetPower(POWER_MANA) >= int32(amount);}

uint32 WarlockAI::GetMana()
{
    Player* bot = GetBot();
    return bot ? bot->GetPower(POWER_MANA) : 0;
}

uint32 WarlockAI::GetMaxMana()
{
    Player* bot = GetBot();
    return bot ? bot->GetMaxPower(POWER_MANA) : 0;
}

float WarlockAI::GetManaPercent()
{
    Player* bot = GetBot();
    return bot ? bot->GetPowerPct(POWER_MANA) : 0.0f;
}

void WarlockAI::UseDefensiveAbilities()
{
    HandleDefensives();
}

void WarlockAI::UseCrowdControl(::Unit* target)
{
    HandleCrowdControl(target);
}

void WarlockAI::UpdatePetManagement()
{
    HandlePetManagement();
}

bool WarlockAI::ShouldConserveMana()
{
    return _lowManaMode;
}

} // namespace Playerbot
