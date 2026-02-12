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

// Battle states
enum PetBattleState : uint8
{
    PET_BATTLE_STATE_CREATED                = 0,
    PET_BATTLE_STATE_WAITING_PRE_BATTLE     = 1,
    PET_BATTLE_STATE_WAITING_FOR_ROUND_INPUT= 2,
    PET_BATTLE_STATE_ROUND_IN_PROGRESS      = 3,
    PET_BATTLE_STATE_WAITING_FOR_FRONT_PET  = 4,
    PET_BATTLE_STATE_FINISHED               = 5,
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
    PET_BATTLE_REQUEST_FAIL_NOT_ENOUGH_PETS          = 4,
    PET_BATTLE_REQUEST_FAIL_ALL_PETS_DEAD            = 5,
    PET_BATTLE_REQUEST_FAIL_IN_BATTLE                = 6,
    PET_BATTLE_REQUEST_FAIL_TARGET_IN_BATTLE         = 7,
    PET_BATTLE_REQUEST_FAIL_NOT_WHILE_IN_COMBAT      = 8,
    PET_BATTLE_REQUEST_FAIL_NOT_WHILE_DEAD           = 9,
    PET_BATTLE_REQUEST_FAIL_NOT_WHILE_FLYING         = 10,
    PET_BATTLE_REQUEST_FAIL_TARGET_NOT_CAPTURABLE    = 11,
    PET_BATTLE_REQUEST_FAIL_NOT_ON_TRANSPORT         = 12,
    PET_BATTLE_REQUEST_FAIL_WILD_PET_TAPPED          = 13,
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
};

// Type effectiveness matrix [attacker type][defender type]
// 1.0 = normal, 1.5 = strong (super effective), 0.667 = weak (not very effective)
static constexpr float PET_TYPE_EFFECTIVENESS[PET_TYPE_COUNT][PET_TYPE_COUNT] =
{
    //                        Hum   Drk   Fly   Und   Cri   Mag   Ele   Bst   Aqu   Mec
    /* Humanoid    */ {  1.0f, 1.0f, 1.0f, 1.5f, 1.0f, 1.0f, 1.0f, 0.67f, 1.0f, 1.0f },
    /* Dragonkin   */ {  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.5f, 1.0f, 1.0f, 1.0f, 0.67f },
    /* Flying      */ {  1.0f, 1.0f, 1.0f, 1.0f, 1.5f, 1.0f, 1.0f, 0.67f, 1.0f, 1.0f },
    /* Undead      */ {  1.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.67f, 1.0f },
    /* Critter     */ {  1.0f, 1.0f, 0.67f, 1.0f, 1.0f, 1.0f, 1.0f, 1.5f, 1.0f, 1.0f },
    /* Magic       */ {  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.5f, 0.67f },
    /* Elemental   */ {  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.67f, 1.0f, 1.0f, 1.5f, 1.0f },
    /* Beast       */ {  0.67f, 1.0f, 1.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
    /* Aquatic     */ {  1.0f, 1.0f, 1.0f, 0.67f, 1.0f, 1.5f, 1.0f, 1.0f, 1.0f, 1.0f },
    /* Mechanical  */ {  1.0f, 0.67f, 1.0f, 1.0f, 1.0f, 1.0f, 1.5f, 1.0f, 1.0f, 1.0f },
};

inline float GetTypeEffectiveness(PetBattlePetType attackerType, PetBattlePetType defenderType)
{
    if (attackerType < 0 || attackerType >= PET_TYPE_COUNT || defenderType < 0 || defenderType >= PET_TYPE_COUNT)
        return 1.0f;
    return PET_TYPE_EFFECTIVENESS[attackerType][defenderType];
}

// Passive family abilities
// Humanoid: Recovers 4% of max HP each round when above 50% HP
// Dragonkin: Deals 50% additional damage on the round after bringing an enemy below 50%
// Flying: Gains 50% speed while above 50% HP
// Undead: Returns to life for one round when killed (once per battle)
// Critter: Immune to stun, root and sleep effects
// Magic: Cannot be dealt more than 35% max HP in a single attack
// Elemental: Ignores all weather effects
// Beast: Deals 25% extra damage when below 50% HP
// Aquatic: Harmful DoT effects reduced by 25%
// Mechanical: Comes back to life once per battle at 20% HP

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
