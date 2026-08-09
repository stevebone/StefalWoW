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

#ifndef GarrisonAutoCombat_h__
#define GarrisonAutoCombat_h__

#include "Define.h"
#include <map>
#include <vector>

struct GarrAutoCombatantEntry;
struct GarrAutoSpellEntry;
struct GarrAutoSpellEffectEntry;
struct GarrFollowerEntry;

// Slots on the Shadowlands Adventures board. These are the client's own GarrAutoBoardIndex values,
// recovered from the 12.0.7.68275 binary's enum-reflection registrars
// (c:/dumps/GARRISON_ENUMS_68275.md, "GarrAutoBoardIndex"): five ally slots 0..4, eight enemy slots
// 5..12, -1 = None. GarrMissionXEncounter.BoardIndex publishes the enemy slots directly (mission 2174's
// two encounters sit at 5 and 6); the ally slots come from the player's placement in the mission UI.
enum GarrAutoBoardIndex : int8
{
    GARR_AUTO_BOARD_NONE                        = -1,
    GARR_AUTO_BOARD_ALLY_LEFT_BACK              = 0,
    GARR_AUTO_BOARD_ALLY_RIGHT_BACK             = 1,
    GARR_AUTO_BOARD_ALLY_LEFT_FRONT             = 2,
    GARR_AUTO_BOARD_ALLY_CENTER_FRONT           = 3,
    GARR_AUTO_BOARD_ALLY_RIGHT_FRONT            = 4,
    GARR_AUTO_BOARD_ENEMY_LEFT_FRONT            = 5,
    GARR_AUTO_BOARD_ENEMY_CENTER_LEFT_FRONT     = 6,
    GARR_AUTO_BOARD_ENEMY_CENTER_RIGHT_FRONT    = 7,
    GARR_AUTO_BOARD_ENEMY_RIGHT_FRONT           = 8,
    GARR_AUTO_BOARD_ENEMY_LEFT_BACK             = 9,
    GARR_AUTO_BOARD_ENEMY_CENTER_LEFT_BACK      = 10,
    GARR_AUTO_BOARD_ENEMY_CENTER_RIGHT_BACK     = 11,
    GARR_AUTO_BOARD_ENEMY_RIGHT_BACK            = 12
};

// True for the five slots a companion may occupy.
inline constexpr bool IsAllyBoardIndex(int32 boardIndex)
{
    return boardIndex >= GARR_AUTO_BOARD_ALLY_LEFT_BACK && boardIndex <= GARR_AUTO_BOARD_ALLY_RIGHT_FRONT;
}

// What the replay calls each event. Client enum GarrAutoMissionEventType, from the 12.0.7.68275
// enum-reflection registrars (c:/dumps/GARRISON_ENUMS_68275.md) and cross-checked against
// GarrisonConstantsDocumentation.lua:176-192. The wire carries these values; AutoCombatEffectType
// below is our internal simulator vocabulary and is NOT interchangeable with them.
enum GarrAutoMissionEventType : uint32
{
    GARR_AUTO_MISSION_EVENT_MELEE_DAMAGE        = 0,
    GARR_AUTO_MISSION_EVENT_RANGE_DAMAGE        = 1,
    GARR_AUTO_MISSION_EVENT_SPELL_MELEE_DAMAGE  = 2,
    GARR_AUTO_MISSION_EVENT_SPELL_RANGE_DAMAGE  = 3,
    GARR_AUTO_MISSION_EVENT_HEAL                = 4,
    GARR_AUTO_MISSION_EVENT_PERIODIC_DAMAGE     = 5,
    GARR_AUTO_MISSION_EVENT_PERIODIC_HEAL       = 6,
    GARR_AUTO_MISSION_EVENT_APPLY_AURA          = 7,
    GARR_AUTO_MISSION_EVENT_REMOVE_AURA         = 8,
    GARR_AUTO_MISSION_EVENT_DIED                = 9
};

// Which aura bucket the board socket files an ApplyAura/RemoveAura event under. Client enum
// GarrAutoPreviewTargetType (GARRISON_ENUMS_68275.md); it is a mask, and the socket switches on it in
// AdventuresSocketMixin:GetCollectionByAuraType (Blizzard_AdventuresBoard.lua:603-612).
enum GarrAutoPreviewTargetType : uint32
{
    GARR_AUTO_PREVIEW_TARGET_NONE   = 0,
    GARR_AUTO_PREVIEW_TARGET_DAMAGE = 1,
    GARR_AUTO_PREVIEW_TARGET_HEAL   = 2,
    GARR_AUTO_PREVIEW_TARGET_BUFF   = 4,
    GARR_AUTO_PREVIEW_TARGET_DEBUFF = 8
};

// GarrFollowerMissionCompleteState, per-companion outcome in SMSG_GARRISON_COMPLETE_MISSION_RESULT
// (GARRISON_ENUMS_68275.md).
enum GarrFollowerMissionCompleteState : uint32
{
    GARR_FOLLOWER_MISSION_COMPLETE_ALIVE                = 0,
    GARR_FOLLOWER_MISSION_COMPLETE_KILLED_BY_FAILURE    = 1,
    GARR_FOLLOWER_MISSION_COMPLETE_SAVED_BY_PREVENT     = 2,
    GARR_FOLLOWER_MISSION_COMPLETE_OUT_OF_DURABILITY    = 3
};

enum AutoCombatEffectType : uint8
{
    AUTO_COMBAT_EFFECT_DAMAGE           = 0,
    AUTO_COMBAT_EFFECT_HEAL             = 1,
    AUTO_COMBAT_EFFECT_BUFF_ATTACK      = 2,
    AUTO_COMBAT_EFFECT_DEBUFF_ATTACK    = 3,
    AUTO_COMBAT_EFFECT_SHIELD           = 4,
    AUTO_COMBAT_EFFECT_AOE_DAMAGE       = 5,
    AUTO_COMBAT_EFFECT_HOT              = 6,
    AUTO_COMBAT_EFFECT_DOT              = 7,
    AUTO_COMBAT_EFFECT_MAX
};

enum AutoCombatTargetType : uint8
{
    AUTO_COMBAT_TARGET_SELF                 = 0,
    AUTO_COMBAT_TARGET_SINGLE_ENEMY         = 1,
    AUTO_COMBAT_TARGET_ALL_ENEMIES          = 2,
    AUTO_COMBAT_TARGET_SINGLE_ALLY          = 3,
    AUTO_COMBAT_TARGET_ALL_ALLIES           = 4,
    AUTO_COMBAT_TARGET_LOWEST_HP_ALLY       = 5,
    AUTO_COMBAT_TARGET_HIGHEST_HP_ENEMY     = 6,
    AUTO_COMBAT_TARGET_RANDOM_ENEMY         = 7,
    AUTO_COMBAT_TARGET_MAX
};

// GarrAutoCombatant.Role, values documented in the GarrAutoCombatant.dbd definition
// (WoWDBDefs, layout 0x6ADAF487 = build 12.0.7.68275).
enum AutoCombatRole : int32
{
    AUTO_COMBAT_ROLE_NONE               = 0,
    AUTO_COMBAT_ROLE_MELEE              = 1,
    AUTO_COMBAT_ROLE_RANGED_PHYSICAL    = 2,
    AUTO_COMBAT_ROLE_RANGED_MAGIC       = 3,
    AUTO_COMBAT_ROLE_HEAL_SUPPORT       = 4,
    AUTO_COMBAT_ROLE_TANK               = 5
};

struct AutoCombatPeriodicEffect
{
    uint32 SpellID = 0;
    int32 Amount = 0;
    int32 RemainingTicks = 0;
    bool IsDamage = true;
    int8 SourceBoardIndex = -1;
};

struct AutoCombatAttackModifier
{
    int32 Amount = 0;
    int32 RemainingRounds = 0;
};

struct TC_GAME_API AutoCombatCombatant
{
    uint32 AutoCombatantID = 0;
    int32 CurrentHealth = 0;
    int32 MaxHealth = 0;
    int32 BaseAttack = 0;
    int8 BoardIndex = -1;
    int32 Role = AUTO_COMBAT_ROLE_NONE;
    int32 AutoAttackSpellID = 0;
    int32 PrimarySpellID = 0;
    int32 SecondarySpellID = 0;
    int32 PassiveSpellID = 0;
    bool IsPlayerSide = false;
    uint64 FollowerDbID = 0;

    int32 ShieldAmount = 0;
    std::map<int32 /*spellID*/, int32 /*remainingCooldown*/> SpellCooldowns;
    std::vector<AutoCombatPeriodicEffect> PeriodicEffects;
    std::vector<AutoCombatAttackModifier> AttackModifiers;

    bool IsAlive() const { return CurrentHealth > 0; }
    int32 GetEffectiveAttack() const;
    void TickPeriodicEffects(struct AutoCombatRound& round);
    void TickCooldowns();
    void TickModifiers();
};

struct AutoCombatEvent
{
    int8 CasterBoardIndex = -1;
    int8 TargetBoardIndex = -1;
    uint32 SpellID = 0;
    int32 Amount = 0;
    uint8 EffectType = 0;
    // The target's health either side of this event. The replay UI draws the health bars from these
    // (FollowerMissionCompleteInfo / GarrisonAutoMissionTargetInfo carry oldHealth/newHealth/maxHealth,
    // GarrisonInfoDocumentation.lua), so they must be captured where the damage/heal is applied - they
    // cannot be reconstructed afterwards.
    int32 TargetOldHealth = 0;
    int32 TargetNewHealth = 0;
    int32 TargetMaxHealth = 0;
    // Context the replay needs to pick the right GarrAutoMissionEventType, captured here because it is
    // only knowable while the event is being produced: the caster's GarrAutoCombatant.Role separates
    // Melee from Range damage, IsAutoAttack separates plain attacks from ability casts, and
    // IsPeriodicTick separates a DoT/HoT *tick* from the cast that applied it (both use EffectType
    // AUTO_COMBAT_EFFECT_DOT/_HOT).
    int32 CasterRole = AUTO_COMBAT_ROLE_NONE;
    // Position of the producing row inside GarrAutoSpellEffect for this spell. The client keys its
    // per-socket aura bookkeeping on (spellID, effectIndex) - AdventuresSocketMixin:AddAura/RemoveAura,
    // Blizzard_AdventuresBoard.lua:570-590 - so two effects of the same spell must not collide.
    uint8 EffectIndex = 0;
    bool IsAutoAttack = false;
    bool IsPeriodicTick = false;
    bool TargetDied = false;
};

struct AutoCombatRound
{
    int32 RoundNumber = 0;
    std::vector<AutoCombatEvent> Events;
};

struct TC_GAME_API AutoCombatResult
{
    bool PlayerWon = false;
    int32 TotalRounds = 0;
    std::vector<AutoCombatRound> CombatLog;
};

class TC_GAME_API GarrisonAutoCombat
{
public:
    static constexpr int32 MAX_ROUNDS = 20;

    static AutoCombatResult SimulateCombat(
        std::vector<AutoCombatCombatant>& playerUnits,
        std::vector<AutoCombatCombatant>& enemyUnits);

    // Health/attack of a GarrAutoCombatant statline at the given level. HealthBase/AttackBase are
    // the level-1 values, the GainPerLevel columns the per-level increment.
    static int32 ScaleHealth(GarrAutoCombatantEntry const* entry, uint32 level);
    static int32 ScaleAttack(GarrAutoCombatantEntry const* entry, uint32 level);

    // followerEntry may be null (callers that only have the runtime follower record). When it
    // carries an AutoCombatantID the combatant is built entirely from GarrAutoCombatant; otherwise
    // the legacy WoD/Legion approximation below is used.
    static AutoCombatCombatant BuildFollowerCombatant(
        GarrFollowerEntry const* followerEntry,
        uint32 followerLevel, uint32 quality, uint32 itemLevelWeapon,
        uint32 itemLevelArmor, int8 boardIndex, uint64 followerDbID);

    static AutoCombatCombatant BuildEnemyCombatant(
        GarrAutoCombatantEntry const* entry, uint32 level, int8 boardIndex);

private:
    static void ProcessTurn(
        AutoCombatCombatant& combatant,
        std::vector<AutoCombatCombatant>& allies,
        std::vector<AutoCombatCombatant>& enemies,
        AutoCombatRound& round);

    static void ResolveSpell(
        AutoCombatCombatant& caster, int32 spellID,
        std::vector<AutoCombatCombatant>& allies,
        std::vector<AutoCombatCombatant>& enemies,
        AutoCombatRound& round);

    static void ResolveEffect(
        AutoCombatCombatant& caster, GarrAutoSpellEffectEntry const* effect,
        AutoCombatCombatant& target, uint32 spellID, uint8 effectIndex,
        AutoCombatRound& round);

    static std::vector<AutoCombatCombatant*> SelectTargets(
        AutoCombatCombatant& caster, uint8 targetType,
        std::vector<AutoCombatCombatant>& allies,
        std::vector<AutoCombatCombatant>& enemies);

    // These take the caster itself rather than only its board index: the replay event has to record the
    // caster's role and whether the hit was an auto-attack, and neither is recoverable later.
    static void ApplyDamage(
        AutoCombatCombatant& target, int32 amount,
        AutoCombatCombatant const& caster, uint32 spellID, uint8 effectType,
        AutoCombatRound& round);

    static void ApplyHealing(
        AutoCombatCombatant& target, int32 amount,
        AutoCombatCombatant const& caster, uint32 spellID,
        AutoCombatRound& round);

    static bool IsTeamAlive(std::vector<AutoCombatCombatant> const& team);
    static AutoCombatCombatant* FindLowestHPAlive(std::vector<AutoCombatCombatant>& team);
    static AutoCombatCombatant* FindHighestHPAlive(std::vector<AutoCombatCombatant>& team);
};

#endif // GarrisonAutoCombat_h__
