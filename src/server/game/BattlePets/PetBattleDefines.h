/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITYCORE_PET_BATTLE_DEFINES_H
#define TRINITYCORE_PET_BATTLE_DEFINES_H

#include "Define.h"
#include <array>

namespace PetBattles
{

static constexpr uint32 MAX_PET_BATTLE_TEAM_SIZE = 3;
static constexpr uint32 MAX_PET_BATTLE_ABILITIES = 3;
static constexpr uint32 MAX_PET_BATTLE_PLAYERS = 2;
static constexpr uint32 MAX_PET_BATTLE_SLOTS = MAX_PET_BATTLE_TEAM_SIZE;
static constexpr uint32 PET_BATTLE_MAX_ROUND_TIME = 30;       // seconds per turn
static constexpr uint32 PET_BATTLE_MAX_GAME_LENGTH = 1800;    // 30 minutes total
static constexpr uint32 PET_BATTLE_TRAP_ABILITY_ID = 427;     // "Trap" ability used for capture
static constexpr uint32 MAX_PET_BATTLE_AURAS = 10;            // max auras on a single pet
static constexpr uint32 MAX_PET_BATTLE_ENVIRONMENTS = 3;      // battlefield + per-team
static constexpr float  PASSIVE_HUMANOID_HEAL_PCT = 0.04f;    // 4% max HP each round
static constexpr float  PASSIVE_DRAGONKIN_DAMAGE_BONUS = 0.50f;
static constexpr float  PASSIVE_FLYING_SPEED_BONUS = 0.50f;
static constexpr float  PASSIVE_MAGIC_DAMAGE_CAP_PCT = 0.35f; // cannot take more than 35% max HP
static constexpr float  PASSIVE_BEAST_DAMAGE_BONUS = 0.25f;
static constexpr float  PASSIVE_AQUATIC_DOT_REDUCTION = 0.25f;
static constexpr float  PASSIVE_MECHANICAL_REVIVE_PCT = 0.20f; // revive at 20% HP

enum PetBattleTeamIndex : uint8
{
    PET_BATTLE_TEAM_1       = 0,
    PET_BATTLE_TEAM_2       = 1,
};

enum PetBattleType : uint8
{
    PET_BATTLE_TYPE_WILD    = 0,
    PET_BATTLE_TYPE_PVP     = 1,
    PET_BATTLE_TYPE_PVE_NPC = 2,
};

// Pet families/types - matches BattlePetSpeciesEntry::PetTypeEnum
enum PetBattlePetType : int8
{
    PET_TYPE_HUMANOID       = 0,
    PET_TYPE_DRAGONKIN      = 1,
    PET_TYPE_FLYING         = 2,
    PET_TYPE_UNDEAD         = 3,
    PET_TYPE_CRITTER        = 4,
    PET_TYPE_MAGIC          = 5,
    PET_TYPE_ELEMENTAL      = 6,
    PET_TYPE_BEAST          = 7,
    PET_TYPE_AQUATIC        = 8,
    PET_TYPE_MECHANICAL     = 9,
    PET_TYPE_COUNT          = 10,
};

// Input action types from client
enum PetBattleMoveType : uint8
{
    PET_BATTLE_MOVE_USE_ABILITY     = 0,
    PET_BATTLE_MOVE_SWAP_PET        = 1,
    PET_BATTLE_MOVE_TRAP            = 2,
    PET_BATTLE_MOVE_FORFEIT         = 3,
    PET_BATTLE_MOVE_SKIP_TURN       = 4,
};

// Battle states - matches client enum
enum PetBattleState : uint8
{
    PET_BATTLE_STATE_CREATED                = 0,
    PET_BATTLE_STATE_WAITING_PRE_BATTLE     = 1,
    PET_BATTLE_STATE_ROUND_IN_PROGRESS      = 2,
    PET_BATTLE_STATE_WAITING_FOR_FRONT_PET  = 3,
    PET_BATTLE_STATE_CREATED_FAILED         = 4,
    PET_BATTLE_STATE_FINAL_ROUND            = 5,
    PET_BATTLE_STATE_FINISHED               = 6,
};

// Battle result
enum PetBattleResult : uint8
{
    PET_BATTLE_RESULT_TEAM_1_WIN    = 0,
    PET_BATTLE_RESULT_TEAM_2_WIN    = 1,
    PET_BATTLE_RESULT_DRAW          = 2,
    PET_BATTLE_RESULT_FORFEIT       = 3,
};

// Request failure reasons sent via SMSG_PET_BATTLE_REQUEST_FAILED
enum PetBattleRequestFailReason : uint8
{
    PET_BATTLE_REQUEST_FAIL_INVALID_TARGET          = 0,
    PET_BATTLE_REQUEST_FAIL_NOT_ALIVE               = 1,
    PET_BATTLE_REQUEST_FAIL_JOURNAL_LOCK            = 2,
    PET_BATTLE_REQUEST_FAIL_NO_PETS_IN_SLOTS        = 3,
    PET_BATTLE_REQUEST_FAIL_NOT_ENOUGH_PETS         = 4,
    PET_BATTLE_REQUEST_FAIL_ALL_PETS_DEAD           = 5,
    PET_BATTLE_REQUEST_FAIL_IN_BATTLE               = 6,
    PET_BATTLE_REQUEST_FAIL_TARGET_IN_BATTLE        = 7,
    PET_BATTLE_REQUEST_FAIL_NOT_WHILE_IN_COMBAT     = 8,
    PET_BATTLE_REQUEST_FAIL_NOT_WHILE_DEAD          = 9,
    PET_BATTLE_REQUEST_FAIL_NOT_WHILE_FLYING        = 10,
    PET_BATTLE_REQUEST_FAIL_TARGET_NOT_CAPTURABLE   = 11,
    PET_BATTLE_REQUEST_FAIL_NOT_ON_TRANSPORT        = 12,
    PET_BATTLE_REQUEST_FAIL_WILD_PET_TAPPED         = 13,
    PET_BATTLE_REQUEST_FAIL_NOT_HERE                = 14,
    PET_BATTLE_REQUEST_FAIL_NOT_HERE_ON_TRANSPORT   = 15,
    PET_BATTLE_REQUEST_FAIL_NOT_HERE_UNEVEN_GROUND  = 16,
    PET_BATTLE_REQUEST_FAIL_NOT_HERE_OBSTRUCTED     = 17,
    PET_BATTLE_REQUEST_FAIL_NOT_A_TRAINER           = 18,
    PET_BATTLE_REQUEST_FAIL_DECLINED                = 19,
    PET_BATTLE_REQUEST_FAIL_INVALID_LOADOUT         = 20,
    PET_BATTLE_REQUEST_FAIL_INVALID_LOADOUT_ALL_DEAD= 21,
    PET_BATTLE_REQUEST_FAIL_INVALID_LOADOUT_NONE_SLOTTED = 22,
    PET_BATTLE_REQUEST_FAIL_NO_JOURNAL_LOCK         = 23,
};

// Effect flags for combat feedback (strong/weak/crit/miss indicators)
enum PetBattleEffectFlags : uint16
{
    PET_BATTLE_EFFECT_FLAG_MISS    = 0x0002,
    PET_BATTLE_EFFECT_FLAG_CRIT    = 0x0004,
    PET_BATTLE_EFFECT_FLAG_HEAL    = 0x0020,
    PET_BATTLE_EFFECT_FLAG_IMMUNE  = 0x0200,
    PET_BATTLE_EFFECT_FLAG_STRONG  = 0x0400,
    PET_BATTLE_EFFECT_FLAG_WEAK    = 0x0800,
};

// PetBattleEffect action types for round resolution
enum PetBattleEffectType : uint8
{
    PET_BATTLE_EFFECT_SET_HEALTH        = 0,
    PET_BATTLE_EFFECT_AURA_APPLY        = 1,
    PET_BATTLE_EFFECT_AURA_CANCEL       = 2,
    PET_BATTLE_EFFECT_AURA_CHANGE       = 3,
    PET_BATTLE_EFFECT_PET_SWAP          = 4,
    PET_BATTLE_EFFECT_SET_STATE         = 5,
    PET_BATTLE_EFFECT_SET_MAX_HEALTH    = 6,
    PET_BATTLE_EFFECT_SET_SPEED         = 7,
    PET_BATTLE_EFFECT_SET_POWER         = 8,
    PET_BATTLE_EFFECT_TRIGGER_ABILITY   = 9,
    PET_BATTLE_EFFECT_ABILITY_CHANGE    = 10,
    PET_BATTLE_EFFECT_NPC_EMOTE         = 11,
    PET_BATTLE_EFFECT_AURA_PROCESSING_BEGIN = 12,
    PET_BATTLE_EFFECT_AURA_PROCESSING_END   = 13,
    PET_BATTLE_EFFECT_STATUS_CHANGE    = 14,
    PET_BATTLE_EFFECT_REPLACE_PET      = 15,
    PET_BATTLE_EFFECT_OVERRIDE_ABILITY = 16,
    PET_BATTLE_EFFECT_WORLD_STATE_UPDATE = 17,
};

// Aura types for pet battle
enum PetBattleAuraType : uint8
{
    PET_BATTLE_AURA_DOT         = 0,
    PET_BATTLE_AURA_HOT         = 1,
    PET_BATTLE_AURA_BUFF        = 2,
    PET_BATTLE_AURA_DEBUFF      = 3,
    PET_BATTLE_AURA_STUN        = 4,
    PET_BATTLE_AURA_ROOT        = 5,
    PET_BATTLE_AURA_SLEEP       = 6,
};

// Weather/environment effect types
enum PetBattleWeatherType : uint8
{
    PET_BATTLE_WEATHER_NONE         = 0,
    PET_BATTLE_WEATHER_SUNNY        = 1,    // +25% healing
    PET_BATTLE_WEATHER_MOONLIGHT    = 2,    // +25% healing, +10% magic damage
    PET_BATTLE_WEATHER_RAIN         = 3,    // +25% aquatic damage
    PET_BATTLE_WEATHER_SANDSTORM    = 4,    // Damage reduction shield, deals damage
    PET_BATTLE_WEATHER_BLIZZARD     = 5,    // Deals frost damage, -25% speed
    PET_BATTLE_WEATHER_MUD          = 6,    // -25% speed
    PET_BATTLE_WEATHER_CLEANSING    = 7,    // removes all weather
    PET_BATTLE_WEATHER_ARCANE       = 8,    // +25% magic damage
    PET_BATTLE_WEATHER_LIGHTNING    = 9,    // +25% mechanical damage, deals damage
    PET_BATTLE_WEATHER_SCORCHED     = 10,   // +25% dragonkin damage
};

// DB2 effect property IDs - these correspond to BattlePetEffectPropertiesEntry::ID
// The actual effect behavior is determined by the effect properties ID
enum PetBattleAbilityEffectAction : uint16
{
    PET_BATTLE_EFFECT_ACTION_DAMAGE             = 0,
    PET_BATTLE_EFFECT_ACTION_HEAL               = 1,
    PET_BATTLE_EFFECT_ACTION_APPLY_AURA         = 2,
    PET_BATTLE_EFFECT_ACTION_CHANGE_STATE       = 3,
    PET_BATTLE_EFFECT_ACTION_DAMAGE_PERCENTAGE  = 4,
    PET_BATTLE_EFFECT_ACTION_HEAL_PERCENTAGE    = 5,
    PET_BATTLE_EFFECT_ACTION_SET_STATE          = 6,
    PET_BATTLE_EFFECT_ACTION_PET_SWAP           = 7,
    PET_BATTLE_EFFECT_ACTION_CATCH              = 8,
    PET_BATTLE_EFFECT_ACTION_CHANGE_MAX_HEALTH  = 9,
    PET_BATTLE_EFFECT_ACTION_WEATHER_SET        = 10,
    PET_BATTLE_EFFECT_ACTION_STUN               = 11,
    PET_BATTLE_EFFECT_ACTION_PERIODIC_DAMAGE    = 12,
    PET_BATTLE_EFFECT_ACTION_PERIODIC_HEAL      = 13,
    PET_BATTLE_EFFECT_ACTION_DAMAGE_CAPPED      = 14,
    PET_BATTLE_EFFECT_ACTION_HEAL_CAPPED        = 15,
    PET_BATTLE_EFFECT_ACTION_REMOVE_AURA        = 16,
    PET_BATTLE_EFFECT_ACTION_MULTI_TURN_BEGIN   = 17,
    PET_BATTLE_EFFECT_ACTION_MULTI_TURN_END     = 18,
    PET_BATTLE_EFFECT_ACTION_COUNT              = 19,
};

// Type effectiveness matrix [attacker type][defender type]
// 1.0 = normal, 1.5 = strong (super effective), 0.66 = weak (not very effective)
// Values from BattlePetTypeDamageMod.txt GameTable
static constexpr float PET_TYPE_EFFECTIVENESS[PET_TYPE_COUNT][PET_TYPE_COUNT] =
{
    //                        Hum    Drk    Fly    Und    Cri    Mag    Ele    Bst    Aqu    Mec
    /* Humanoid    */ {  1.0f,  1.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.66f, 1.0f,  1.0f },
    /* Dragonkin   */ {  1.0f,  1.0f, 1.0f, 0.66f,1.0f, 1.5f, 1.0f, 1.0f,  1.0f,  1.0f },
    /* Flying      */ {  1.0f, 0.66f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,  1.5f,  1.0f },
    /* Undead      */ {  1.5f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.66f,  1.0f },
    /* Critter     */ { 0.66f,  1.0f, 1.0f, 1.5f, 1.0f, 1.0f, 1.0f, 1.0f,  1.0f,  1.0f },
    /* Magic       */ {  1.0f,  1.0f, 1.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.66f },
    /* Elemental   */ {  1.0f,  1.0f, 1.0f, 1.0f, 0.66f,1.0f, 1.0f, 1.0f,  1.0f,  1.5f },
    /* Beast       */ {  1.0f,  1.0f, 0.66f,1.0f, 1.5f, 1.0f, 1.0f, 1.0f,  1.0f,  1.0f },
    /* Aquatic     */ {  1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 0.66f,1.5f, 1.0f,  1.0f,  1.0f },
    /* Mechanical  */ {  1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.66f,1.5f,  1.0f,  1.0f },
};

inline float GetTypeEffectiveness(PetBattlePetType attackerType, PetBattlePetType defenderType)
{
    if (attackerType < 0 || attackerType >= PET_TYPE_COUNT || defenderType < 0 || defenderType >= PET_TYPE_COUNT)
        return 1.0f;
    return PET_TYPE_EFFECTIVENESS[attackerType][defenderType];
}

// Passive family abilities
// Humanoid: Recovers 4% of max HP each round
// Dragonkin: Deals 50% additional damage on the round after bringing an enemy below 50%
// Flying: Gains 50% speed while above 50% HP
// Undead: Returns to life for one round when killed (once per battle)
// Critter: Immune to stun, root and sleep effects
// Magic: Cannot be dealt more than 35% max HP in a single attack
// Elemental: Ignores all weather effects
// Beast: Deals 25% extra damage when below 50% HP
// Aquatic: Harmful DoT effects reduced by 25%
// Mechanical: Comes back to life once per battle at 20% HP

// Trap status codes for capture validation
enum PetBattleTrapStatus : uint8
{
    PET_BATTLE_TRAP_STATUS_INVALID              = 0,
    PET_BATTLE_TRAP_STATUS_CANT_TRAP_TRAINER    = 1,
    PET_BATTLE_TRAP_STATUS_CANT_TRAP_PVP        = 2,
    PET_BATTLE_TRAP_STATUS_NOT_CAPTURABLE       = 3,
    PET_BATTLE_TRAP_STATUS_NOT_DEAD             = 4,
    PET_BATTLE_TRAP_STATUS_ALREADY_CAPTURED     = 5,
    PET_BATTLE_TRAP_STATUS_TOO_HEALTHY          = 6,
    PET_BATTLE_TRAP_STATUS_JOURNAL_FULL         = 7,
    PET_BATTLE_TRAP_STATUS_READY                = 8,
};

// Pet status flags for battle state display
enum PetBattlePetStatusFlags : uint16
{
    PET_BATTLE_PET_STATUS_TRAPPED           = 0x0001,
    PET_BATTLE_PET_STATUS_STUNNED           = 0x0002,
    PET_BATTLE_PET_STATUS_SWAP_OUT_LOCKED   = 0x0004,
    PET_BATTLE_PET_STATUS_SWAP_IN_LOCKED    = 0x0008,
};

// Input flags sent to the client each round
enum PetBattleInputFlags : uint8
{
    PET_BATTLE_INPUT_FLAG_TURN_IN_PROGRESS  = 0x01,
    PET_BATTLE_INPUT_FLAG_ABILITY_LOCKED    = 0x02,
    PET_BATTLE_INPUT_FLAG_SWAP_LOCKED       = 0x04,
    PET_BATTLE_INPUT_FLAG_WAITING_FOR_PET   = 0x08,
};

// Aura state flags
enum PetBattleAuraStateFlags : uint8
{
    PET_BATTLE_AURA_STATE_NONE          = 0x00,
    PET_BATTLE_AURA_STATE_JUST_APPLIED  = 0x01,
    PET_BATTLE_AURA_STATE_INFINITE      = 0x02,
};

// PvP queue status codes - matches client enum
enum PetBattleQueueStatus : uint8
{
    PET_BATTLE_QUEUE_STATUS_NONE                    = 0,
    PET_BATTLE_QUEUE_STATUS_QUEUED                  = 1,
    PET_BATTLE_QUEUE_STATUS_MATCHMAKING             = 2,
    PET_BATTLE_QUEUE_STATUS_PROPOSAL                = 3,
    PET_BATTLE_QUEUE_STATUS_MATCH_ACCEPTED          = 4,
    PET_BATTLE_QUEUE_STATUS_MATCH_DECLINED          = 5,
    PET_BATTLE_QUEUE_STATUS_MATCH_OPPONENT_DECLINED = 6,
    PET_BATTLE_QUEUE_STATUS_ALREADY_QUEUED          = 7,
    PET_BATTLE_QUEUE_STATUS_REMOVED                 = 8,
    PET_BATTLE_QUEUE_STATUS_JOIN_FAILED             = 9,
    PET_BATTLE_QUEUE_STATUS_JOIN_FAILED_SLOTS       = 10,
    PET_BATTLE_QUEUE_STATUS_JOIN_FAILED_JOURNAL_LOCK= 11,
    PET_BATTLE_QUEUE_STATUS_UPDATE                  = 12,
};

// Crit hit constants
static constexpr float PET_BATTLE_BASE_CRIT_CHANCE = 0.05f;
static constexpr float PET_BATTLE_CRIT_MULTIPLIER  = 1.5f;

// Capture success chance based on target HP percentage
inline float GetCaptureChance(uint16 trapLevel, float healthPct)
{
    // Base chance modified by trap level and target HP
    float baseChance = 0.20f + (trapLevel - 1) * 0.05f;
    // Lower HP = higher capture chance
    float hpModifier = 2.0f - (healthPct / 100.0f) * 1.5f;
    return std::min(baseChance * hpModifier, 1.0f);
}

} // namespace PetBattles

#endif // TRINITYCORE_PET_BATTLE_DEFINES_H
